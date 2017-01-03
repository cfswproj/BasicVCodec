///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        intraPred.cpp
// Abstract:    F
//
// Author:      Corey Fernando                  (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////


#include "intraPred.h"
#include "slice.h"
#include "transform.h"
#include "qualityMetrics.h"

#define MB_PRED_MODE_I4x4_VAR_THLD                  .7//0.03333f
#define MB_PRED_MODE_I8x8_VAR_THLD                  .5f//0.01667f

/*
     ____ ____ ____
    |  B |  C |  D |
    |____|____|____|
    |  A | MB |
    |____|____|
    
*/
#define INTRA_PRED_SA_16                            16
#define INTRA_PRED_SA_8                             8
#define INTRA_PRED_SA_4                             4

#define NUM_INTRA_PRED_MODE_LUMA_BLOCKS_4x4         16
#define NUM_INTRA_PRED_MODE_LUMA_BLOCKS_8x8         4
#define NUM_INTRA_PRED_MODE_LUMA_BLOCKS_16x16       1

#define NUM_INTRA_PRED_MODE_CHROMA_BLOCKS_4x4       4
#define NUM_INTRA_PRED_MODE_CHROMA_BLOCKS_8x8       1

#define LUMA_VAL_128                                128

template<class T>
float           mean                        ( T * pX , int size );

template<class T>
float           var                         ( T * pX , int size );

float           varUniformDisceteRV         ( int a , int b );

int             intraLumaSearchFull         ( Macroblock * pMB );

int             intra16x16SearchFull        ( Macroblock * pMB );

int             intraChromaSearchFull       ( Macroblock * pMB );

int             intraChromaPred             ( Macroblock * pMB , int * pIntraMode );



void availableModeListLuma ( int * pIntraModes , MB_PRED_MODE predMode , vector<uint8_t> &leftSa , vector<uint8_t> &upSa , int &sz )
{
    int i                       = 0;

    int stride                  = 0;

    if (MB_PRED_MODE_I16x16 == predMode)
    {
        stride                  = INTRA_PRED_SA_16;
        pIntraModes[i++]        = INTRA_PRED_MODE_16x16_DC;
        if (stride <= upSa.size())
        {
            pIntraModes[i++]        = INTRA_PRED_MODE_16x16_VERT;
        }

        if (stride <= leftSa.size())
        {
            pIntraModes[i++]        = INTRA_PRED_MODE_16x16_HRZN;
        }

        if (stride <= leftSa.size() && stride <= upSa.size())
        {
            if (stride < upSa.size())
            {
                pIntraModes[i++]    = INTRA_PRED_MODE_16x16_PLANE;
            }
        }
    }
    else
    {
        if (MB_PRED_MODE_I4x4 == predMode)
            stride                  = INTRA_PRED_SA_4;
        else if (MB_PRED_MODE_I8x8 == predMode)
            stride                  = INTRA_PRED_SA_8;

        pIntraModes[i++]            = INTRA_PRED_MODE_LUMA_DC;
        if (stride <= upSa.size())
        {
            pIntraModes[i++]        = INTRA_PRED_MODE_LUMA_VERT;

            if (2 * stride <= upSa.size())
            {
                pIntraModes[i++]    = INTRA_PRED_MODE_LUMA_DIAGDWNL;
                pIntraModes[i++]    = INTRA_PRED_MODE_LUMA_VERTL;
            }
        }

        if (stride <= leftSa.size())
        {
            pIntraModes[i++]        = INTRA_PRED_MODE_LUMA_HRZN;
            pIntraModes[i++]        = INTRA_PRED_MODE_LUMA_HRZNUP;
        }

        if (stride <= leftSa.size() && stride <= upSa.size())
        {
            if (stride < leftSa.size())
            {
                pIntraModes[i++]    = INTRA_PRED_MODE_LUMA_DIAGDWNR;
                pIntraModes[i++]    = INTRA_PRED_MODE_LUMA_VERTR;
                pIntraModes[i++]    = INTRA_PRED_MODE_LUMA_HRZNDWN;
            }
        }
    }

    sz                          = i;
}

void availableModeListChroma ( int * pIntraModes , MB_PRED_MODE predMode , vector<uint8_t> &leftSa , vector<uint8_t> &upSa , int &sz )
{
    int i                       = 0;
    int stride                  = 0;

    if (MB_PRED_MODE_I4x4 == predMode)
        stride                  = INTRA_PRED_SA_4;
    else if (MB_PRED_MODE_I8x8 == predMode)
        stride                  = INTRA_PRED_SA_8;

    pIntraModes[i++]            = INTRA_PRED_MODE_CHROMA_DC;
    if (stride <= upSa.size())
    {
        pIntraModes[i++]        = INTRA_PRED_MODE_CHROMA_VERT;
    }

    if (stride <= leftSa.size())
    {
        pIntraModes[i++]        = INTRA_PRED_MODE_CHROMA_HRZN;
    }

    if (stride <= leftSa.size() && stride <= upSa.size())
    {
        if (stride < upSa.size())
        {
            pIntraModes[i++]    = INTRA_PRED_MODE_CHROMA_PLANE;
        }
    }
    sz                          = i;
}

template<class T>
float mean ( T * pX , int size )
{
    float mu        = 0;

    _ASSERT(NULL != pX);

    for (int i = 0; i < size; i++)
        mu          += pX[i];

    return (mu / size);
}

template<class T>
float var ( T * pX , int size )
{
    float sig2      = 0;

    _ASSERT(NULL != pX);

    float mu        = mean(pX, size);
    for (int i = 0; i < size; i++)
    {
        float val   = ( pX[i] - mu );

        sig2        += ( val * val );
    }

    return (sig2 / size);
}

float varUniformDisceteRV ( int a , int b )
{
    int range       = ( b - a );
    float sig2      = ( range * ( range + 2 ) ) / 12.0f;

    return sig2;
}

void intraPredLumaSliceEnc ( Slice * pSlice )
{    
    _ASSERT(NULL != pSlice);

    const int numMB             = pSlice->NumMB;
    
    for (int i = 0; i < numMB; i++)
    {
        Macroblock * pMB        = &pSlice->MB[i];
        pMB->NumMBPart          = 1;
        pMB->PredMode           = MB_PRED_MODE_IPCM;
        pMB->SubBlkWidth        = INTRA_PRED_SA_16;
        pMB->SubBlkHeight       = INTRA_PRED_SA_16;
        pMB->NumSubBlks         = NUM_INTRA_PRED_MODE_LUMA_BLOCKS_16x16;
        pMB->bTrans8x8          = false;

        pMB->MBResid.resize(pMB->NumSubBlks);
        pMB->MBDPB.resize(pMB->NumSubBlks);

        for (int si = 0; si < pMB->NumSubBlks; si++)
        {
            // Copy the source data into our residual signal
            // and we're done!
            copyRefPicMBSubBlk(pMB, si, pMB->MBResid[si].Pel);   
            
            // We don't need any copies of MB in the DPB so 
            // let's not waste the time...
        }
    }
}

void intraPredChromaSliceEnc  ( Slice * pSliceCb , Slice * pSliceCr )
{
    _ASSERT(NULL != pSliceCb);
    _ASSERT(NULL != pSliceCr);

    const int numMB                 = pSliceCb->NumMB;
    
    for (int i = 0; i < numMB; i++)
    {
        Macroblock * pMBCb          = &pSliceCb->MB[i];

        pMBCb->NumMBPart            = 1; // There is one logical partition for Intra
        // Set the MB prediction mode.

        pMBCb->PredMode             = MB_PRED_MODE_IPCM;
        pMBCb->SubBlkWidth          = INTRA_PRED_SA_4;
        pMBCb->SubBlkHeight         = INTRA_PRED_SA_4;
        pMBCb->NumSubBlks           = NUM_INTRA_PRED_MODE_CHROMA_BLOCKS_4x4;
        pMBCb->bTrans8x8            = false;

        Macroblock * pMBCr          = &pSliceCr->MB[i];

        pMBCr->NumMBPart            = 1; // There is one logical partition for Intra
        // Set the MB prediction mode.

        pMBCr->PredMode             = MB_PRED_MODE_IPCM;
        pMBCr->SubBlkWidth          = INTRA_PRED_SA_4;
        pMBCr->SubBlkHeight         = INTRA_PRED_SA_4;
        pMBCr->NumSubBlks           = NUM_INTRA_PRED_MODE_CHROMA_BLOCKS_4x4;
        pMBCr->bTrans8x8            = false;

        pMBCb->MBResid.resize(pMBCb->NumSubBlks);
        pMBCb->MBDPB.resize(pMBCb->NumSubBlks);

        pMBCr->MBResid.resize(pMBCr->NumSubBlks);
        pMBCr->MBDPB.resize(pMBCr->NumSubBlks);

        for (int si = 0; si < pMBCb->NumSubBlks; si++)
        {
            // Copy the source data into our residual signal
            // and we're done!
            copyRefPicMBSubBlk(pMBCb, si, pMBCb->MBResid[si].Pel); 
            copyRefPicMBSubBlk(pMBCr, si, pMBCr->MBResid[si].Pel);

            // We don't need any copies of MB in the DPB so 
            // let's not waste the time...       
        }
    }
}

void intraPCMLumaSliceEnc ( Slice * pSlice )
{    
    _ASSERT(NULL != pSlice);

    const int numMB             = pSlice->NumMB;
    
    for (int i = 0; i < numMB; i++)
    {
        Macroblock * pMB        = &pSlice->MB[i];
        pMB->NumMBPart          = 1;
        pMB->PredMode           = MB_PRED_MODE_IPCM;
        pMB->SubBlkWidth        = INTRA_PRED_SA_16;
        pMB->SubBlkHeight       = INTRA_PRED_SA_16;
        pMB->NumSubBlks         = NUM_INTRA_PRED_MODE_LUMA_BLOCKS_16x16;
        pMB->bTrans8x8          = false;


        pMB->MBResid.resize(pMB->NumSubBlks);
        pMB->MBDPB.resize(pMB->NumSubBlks);

        for (int si = 0; si < pMB->NumSubBlks; si++)
        {
            // Copy the source data into our residual signal
            // and we're done!
            copyRefPicMBSubBlk(pMB, si, pMB->MBResid[si].Pel);   
            
            // We don't need any copies of MB in the DPB so 
            // let's not waste the time...
        }
    }
}

void intraPCMChromaSliceEnc  ( Slice * pSliceCb , Slice * pSliceCr )
{
    _ASSERT(NULL != pSliceCb);
    _ASSERT(NULL != pSliceCr);

    const int numMB                 = pSliceCb->NumMB;
    
    for (int i = 0; i < numMB; i++)
    {
        Macroblock * pMBCb          = &pSliceCb->MB[i];

        pMBCb->NumMBPart            = 1; // There is one logical partition for Intra
        // Set the MB prediction mode.

        pMBCb->PredMode             = MB_PRED_MODE_IPCM;
        pMBCb->SubBlkWidth          = INTRA_PRED_SA_4;
        pMBCb->SubBlkHeight         = INTRA_PRED_SA_4;
        pMBCb->NumSubBlks           = NUM_INTRA_PRED_MODE_CHROMA_BLOCKS_4x4;
        pMBCb->bTrans8x8            = false;

        Macroblock * pMBCr          = &pSliceCr->MB[i];

        pMBCr->NumMBPart            = 1; // There is one logical partition for Intra
        // Set the MB prediction mode.

        pMBCr->PredMode             = MB_PRED_MODE_IPCM;
        pMBCr->SubBlkWidth          = INTRA_PRED_SA_4;
        pMBCr->SubBlkHeight         = INTRA_PRED_SA_4;
        pMBCr->NumSubBlks           = NUM_INTRA_PRED_MODE_CHROMA_BLOCKS_4x4;
        pMBCr->bTrans8x8            = false;

        pMBCb->MBResid.resize(pMBCb->NumSubBlks);
        pMBCb->MBDPB.resize(pMBCb->NumSubBlks);

        pMBCr->MBResid.resize(pMBCr->NumSubBlks);
        pMBCr->MBDPB.resize(pMBCr->NumSubBlks);

        for (int si = 0; si < pMBCb->NumSubBlks; si++)
        {
            // Copy the source data into our residual signal
            // and we're done!
            copyRefPicMBSubBlk(pMBCb, si, pMBCb->MBResid[si].Pel); 
            copyRefPicMBSubBlk(pMBCr, si, pMBCr->MBResid[si].Pel);

            // We don't need any copies of MB in the DPB so 
            // let's not waste the time...       
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Decode

void intraPredLumaSliceDec ( Slice * pSlice )
{
    _ASSERT(NULL != pSlice);
    intraPCMLumaSliceDec(pSlice);
}

void intraPredChromaSliceDec ( Slice * pSlice )
{
    _ASSERT(NULL != pSlice);
    intraPCMChromaSliceDec(pSlice);
}

void intraPCMLumaSliceDec ( Slice * pSlice )
{
    _ASSERT(NULL != pSlice);

    const int numMB             = pSlice->NumMB;
    
    for (int i = 0; i < numMB; i++)
    {
        Macroblock * pMB        = &pSlice->MB[i];
        if (( 1 != pMB->NumMBPart ) ||
            ( MB_PRED_MODE_IPCM != pMB->PredMode )  ||
            ( INTRA_PRED_SA_16 != pMB->SubBlkWidth ) ||
            ( INTRA_PRED_SA_16 != pMB->SubBlkHeight ) ||
            ( NUM_INTRA_PRED_MODE_LUMA_BLOCKS_16x16 != pMB->NumSubBlks )
           )
        {
            // AN error code would be nice.
            return;
        }

        // Write to the DPB
        // Copy the residual data straight into our DCP
        // and we're done!
        for (int si = 0; si < pMB->MBResid.size(); si++)
            copyMBSubBlkToDPB(pMB, si, pMB->MBResid[si].Pel);   

        // We don't need any copies of MB in the MBDPB so 
        // let's not waste the time...

    }
}


void intraPCMChromaSliceDec ( Slice * pSlice )
{    
    _ASSERT(NULL != pSlice);

    const int numMB             = pSlice->NumMB;
    
    for (int i = 0; i < numMB; i++)
    {
        Macroblock * pMB        = &pSlice->MB[i];
        if (( 1 != pMB->NumMBPart ) ||
            ( MB_PRED_MODE_IPCM != pMB->PredMode )  ||
            ( INTRA_PRED_SA_4 != pMB->SubBlkWidth ) ||
            ( INTRA_PRED_SA_4 != pMB->SubBlkHeight ) ||
            ( NUM_INTRA_PRED_MODE_CHROMA_BLOCKS_4x4 != pMB->NumSubBlks )
           )
        {
            // AN error code would be nice.
            return;
        }

        // Write to the DPB
        // Copy the residual data straight into our DPB
        // and we're done!
        for (int si = 0; si < pMB->MBResid.size(); si++)
            copyMBSubBlkToDPB(pMB, si, pMB->MBResid[si].Pel);    

        // We don't need any copies of MB in the MBDPB so 
        // let's not waste the time...
    }
}