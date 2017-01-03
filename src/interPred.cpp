///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        interPred.cpp
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////


#include "interPred.h"
#include "slice.h"
#include "macroblock.h"
#include "transform.h"
#include "encoder.hpp"

#define INTERPRED_LUMA_PIC_INTERP_FACTOR                            4
#define INTERPRED_LUMA_PIC_INTERPFILTER_SIZE                        6
#define INTERPRED_LUMA_PIC_INTERPFILTER_BS                          5 // Bit shift value

#define INTERPRED_CHROMA_PIC_INTERP_FACTOR                          8
#define INTERPRED_CHROMA_PIC_INTERPFILTER_SIZE                      4
#define INTERPRED_CHROMA_PIC_INTERPFILTER_BS                        6 // Bit shift value

#define INTERPRED_DEFAULT_SEARCH_RADIUS                             32

const int g_lumaInterpFilt[INTERPRED_LUMA_PIC_INTERPFILTER_SIZE]    = { 1 , -5 , 20 , 20 , -5 , 1 };

/*
    Function:       interpLuma
    Description:    Creates a quater pel interpolated picture based on the reference samples
                    found in pRefFrame.
    Side-effects:   interpPic is a picture that is 4 x the width and 4 x the height of 
                    the reference picture.
*/
void        interpLuma          ( const Frame * pRefFrame , PicUint8_t &interpPic );

/*
    Function:       interpChroma
    Description:    Creates a quater pel interpolated picture based on the reference samples
                    found in pRefFrame.
    Side-effects:   interpPic is a picture that is 4 x the width and 4 x the height of 
                    the reference picture.
*/
void        interpChroma       ( const Frame * pRefFrame , PicUint8_t &interpPic , COLOR_PLANE_YUV plane );

/*
    Function:       mbMESAD
    Description:    Creates the NxN reference block for the current MB pointed to by pMB using 
                    the pel data found in the Frame object pointed to by pRefFrame. The SAD is
                    then computed for the block generated at the offset position 
                    (xOffset , yOffset).
    Side-effects:   Returns the SAD score computed from the current MB and the corresponding 
                    reference block in pRefFrame.
*/
int         mbMESAD             ( Macroblock * pMB , PicUint8_t * pRefPic , vector<vector<int>> &resid , MotionVector &mv );

/*
    Function:       mbSubBlkMESAD
    Description:    Creates the NxN reference block for the current MB pointed to by pMB using 
                    the pel data found in the Frame object pointed to by pRefFrame. The SAD is
                    then computed for the block generated at the offset position 
                    (xOffset , yOffset).
    Side-effects:   Returns the SAD score computed from the current MB and the corresponding 
                    reference block in pRefFrame.
*/
int         mbSubBlkMESAD      ( Macroblock * pMB , PicUint8_t * pRefPic , vector<vector<int>> &resid ,
                                 int sblkX , int sblkY , int sblkWidth , int sblkHeight , MotionVector &mv );

/*
    Function:       mbMESAD4x4
    Description:    Creates the NxN reference block for the current MB pointed to by pMB using 
                    the pel data found in the Frame object pointed to by pRefFrame. The SAD is
                    then computed for the block generated at the offset position 
                    (xOffset , yOffset).
    Side-effects:   Returns the SAD score computed from the current MB and the corresponding 
                    reference block in pRefFrame as 4x4 subBlocks of the MB.
*/
void        mbMESAD4x4          ( Macroblock * pMB , PicUint8_t * pRefPic , vector<vector<int>> &resid , vector<vector<int>> &sad4x4 , MotionVector &mv );

/*
    Function:       nnsME
    Description:    Returns the minimum SAD after an nLvl nearest neighbor MB search
                    procedure. 
    Side-effects:   The motion vector object mv contains the motion vector producing 
                    the smallest SAD erro and the minimum SAD is returned.
*/
void        nnsME               ( Macroblock * pMB , vector<vector<int>> &sad4x4 , MotionVector &mv );

/*
    Function:       nullSearchME
    Description:    Computes the SAD score for the current MB at the motion vector (0, 0). 
    Side-effects:   The motion vector object mv contains the motion vector producing 
                    the smallest SAD erro and the minimum SAD is returned.
*/
void        nullSearchME        ( Macroblock * pMB , vector<vector<int>> &sad4x4 );

void interpLuma ( const Frame * pRefFrame , PicUint8_t &interpPic )
{
    _ASSERT(NULL != pRefFrame);

    int frameWidth              = pRefFrame->width(COLOR_PLANE_YUV_LUMA);
    int frameHeight             = pRefFrame->height(COLOR_PLANE_YUV_LUMA);

    // Set the interpPic size...
    int interpWidth             = frameWidth * INTERPRED_LUMA_PIC_INTERP_FACTOR;
    int interpHeight            = frameHeight * INTERPRED_LUMA_PIC_INTERP_FACTOR;

    interpPic.resize(interpHeight);
    // Let's create all of the columns now...
    for (int i = 0; i < interpHeight; i++)
        interpPic[i].resize(interpWidth);

    // Interp all of the rows
    for (int i = 0, iRef = 0; i < interpHeight; i += INTERPRED_LUMA_PIC_INTERP_FACTOR, iRef++)
    {
        // Fill all the known values
        for (int j = 0, jRef = 0; j < interpWidth; j += INTERPRED_LUMA_PIC_INTERP_FACTOR, jRef++)
            interpPic[i][j]     = pRefFrame->getPel(iRef, jRef, COLOR_PLANE_YUV_LUMA);

        // For each half-pel location in each row, perform 6-Tap interpolation
        for (int j = 2, jRef = 0; j < interpWidth; j += INTERPRED_LUMA_PIC_INTERP_FACTOR, jRef++)
        {
            int sum             = 0;
            int offset          = -2;
            for (int fj = 0; fj < INTERPRED_LUMA_PIC_INTERPFILTER_SIZE; fj++)
            {
                int jOffset     = jRef + offset + fj;

                if (0 > jOffset)
                    jOffset     = abs(jOffset);
                else if (frameWidth <= jOffset)
                    jOffset     = frameWidth - ( jOffset % frameWidth ) - 1;

                sum             += g_lumaInterpFilt[fj] * pRefFrame->getPel(iRef, jOffset, COLOR_PLANE_YUV_LUMA);
            }

            interpPic[i][j]     = (uint8_t) ( sum >> INTERPRED_LUMA_PIC_INTERPFILTER_BS );
        }

        // For each quater-pel location in each row, perform bi-linear interpolation
        for (int j = 1; j < ( interpWidth - 1 ); j += 2)
            interpPic[i][j]     = ( interpPic[i][j-1] + interpPic[i][j+1] ) >> 1;

        // Don't forget the right edge sample.
        interpPic[i][interpWidth-1]     = interpPic[i][interpWidth-2];
    }

    // Interp all of the columns
    for (int j = 0; j < interpWidth; j ++)
    {
        // For each half-pel location in each col, perform 6-Tap interpolation
        for (int i = 2, iRef = -4; i < interpHeight; i += INTERPRED_LUMA_PIC_INTERP_FACTOR, iRef += INTERPRED_LUMA_PIC_INTERP_FACTOR)
        {
            int sum             = 0;
            for (int fi = 0; fi < INTERPRED_LUMA_PIC_INTERPFILTER_SIZE; fi++)
            {
                int iOffset     = iRef + ( fi * INTERPRED_LUMA_PIC_INTERP_FACTOR );

                if (0 > iOffset)
                    iOffset     = abs(iOffset);
                else if (interpHeight <= iOffset)
                    iOffset     = interpHeight - ( iOffset % interpHeight ) - 1;

                sum             += g_lumaInterpFilt[fi] * interpPic[iOffset][j];
            }

            interpPic[i][j]     = (uint8_t) ( sum >> INTERPRED_LUMA_PIC_INTERPFILTER_BS );
        }

        // For each quater-pel location in each col, perform bi-linear interpolation
        for (int i = 1; i < ( interpHeight - 1 ); i += 2)
            interpPic[i][j]     = ( interpPic[i-1][j] + interpPic[i+1][j] ) >> 1;
        
        // Don't forget the right edge sample.
        interpPic[interpHeight-1][j]     = interpPic[interpHeight-2][j];
    }
}

void interpChroma ( const Frame * pRefFrame , PicUint8_t &interpPic , COLOR_PLANE_YUV plane )
{
    _ASSERT(NULL != pRefFrame);

    int frameWidth              = pRefFrame->width(plane);
    int frameHeight             = pRefFrame->height(plane);

    // Set the interpPic size...
    int interpWidth             = frameWidth * INTERPRED_CHROMA_PIC_INTERP_FACTOR;
    int interpHeight            = frameHeight * INTERPRED_CHROMA_PIC_INTERP_FACTOR;

    interpPic.resize(interpHeight);
    // Let's create all of the columns now...
    for (int i = 0; i < interpHeight; i++)
        interpPic[i].resize(interpWidth);

    // Fill all known samples
    for (int i = 0, iRef = 0; i < interpHeight; i += INTERPRED_CHROMA_PIC_INTERP_FACTOR, iRef++)
        for (int j = 0, jRef = 0; j < interpWidth; j += INTERPRED_CHROMA_PIC_INTERP_FACTOR, jRef++)
            interpPic[i][j]     = pRefFrame->getPel(iRef, jRef, plane);

    // Interp all samples
    for (int i = 0; i < interpHeight; i += INTERPRED_CHROMA_PIC_INTERP_FACTOR)
    {
        int iRefB               = i + INTERPRED_CHROMA_PIC_INTERP_FACTOR;
        if (interpHeight <= iRefB)
            iRefB               = i;

        for (int j = 0; j < interpWidth; j += INTERPRED_CHROMA_PIC_INTERP_FACTOR)
        {
            int jRefR           = j + INTERPRED_CHROMA_PIC_INTERP_FACTOR;
            if (interpWidth <= jRefR)
                jRefR           = j;
            for (int ii = 1; ii < INTERPRED_CHROMA_PIC_INTERP_FACTOR; ii++)
            {
                int si          = i + ii;
                int wy          = (INTERPRED_CHROMA_PIC_INTERP_FACTOR - ii);
                for (int jj = 1; jj < INTERPRED_CHROMA_PIC_INTERP_FACTOR; jj++)
                {
                    int sj      = j + jj;
                    int wx      = (INTERPRED_CHROMA_PIC_INTERP_FACTOR - jj);

                    int sum     = wy * wx * interpPic[i][j] + 
                                  wy * jj * interpPic[i][jRefR] + 
                                  ii * wx * interpPic[iRefB][j] + 
                                  ii * jj * interpPic[iRefB][jRefR];

                    interpPic[si][sj]   = (uint8_t)( sum >> INTERPRED_CHROMA_PIC_INTERPFILTER_BS ); // not using round function
                }
            }
            
        }
    }
}

int mbMESAD ( Macroblock * pMB , PicUint8_t * pRefPic , vector<vector<int>> &resid , MotionVector &mv )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pRefPic);

    // Make sure the search is centered 
    int centerOffset            = pMB->MBWidth / 2;
    int mbGlbXPos               = ( pMB->XPos * pMB->MBWidth ) + mv.X - centerOffset;
    int mbGlbYPos               = ( pMB->YPos * pMB->MBWidth ) + mv.Y - centerOffset;

    int frameWidth              = (int)(*pRefPic)[0].size();
    int frameHeight             = (int)pRefPic->size();

    int mbSAD                   = 0;
    resid.resize(pMB->MBHeight);

    for (int i = 0; i < pMB->MBHeight; i++)
    {
        int yPos                = mbGlbYPos + i;

        // Mirror Y if necessary
        if (frameHeight <= yPos)
            yPos                = 2 * frameHeight - yPos - 1;
        else if (0 > yPos)
            yPos                = abs(yPos);

        resid[i].resize(pMB->MBWidth);
        for (int j = 0; j < pMB->MBWidth; j++)
        {
            int xPos            = mbGlbXPos + j;
            // Mirror X if necessary
            if (frameWidth <= xPos)
                xPos            = frameWidth - ( xPos % frameWidth );
            else if (0 > xPos)
                xPos            = abs(xPos);

            // Get the pel
            uint8_t mbPel       = getMBRefPicPel(pMB, i, j);
            resid[i][j]         = mbPel - (*pRefPic)[yPos][xPos];
            mbSAD               += abs(resid[i][j]);
        }
    }

    return mbSAD;
}

int mbSubBlkMESAD ( Macroblock * pMB , PicUint8_t * pRefPic , vector<vector<int>> &resid ,
                    int sblkX , int sblkY , int sblkWidth , int sblkHeight , MotionVector &mv )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pRefPic);

    int mbSAD                   = 0;

    return mbSAD;
}

void mbMESAD4x4 ( Macroblock * pMB , PicUint8_t * pRefPic , vector<vector<int>> &resid , vector<vector<int>> &sad4x4 , MotionVector &mv )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pRefPic);

    
    int interpFactor            = COLOR_PLANE_YUV_LUMA ==  pMB->ColorPlane ? 
                                    MB_INTERP_FACTOR_LUMA : MB_INTERP_FACTOR_CHROMA;

    int interpMBWidth           = pMB->MBWidth * interpFactor;
    int interpMBHeight          = pMB->MBHeight * interpFactor;

    // Make sure the search is centered
    int mbGlbXPos               = ( pMB->XPos * interpMBWidth ) + mv.X;
    int mbGlbYPos               = ( pMB->YPos * interpMBHeight ) + mv.Y;

    int plane                   = (int)pMB->ColorPlane;
    int frameWidth              = (int)(*pRefPic)[0].size();
    int frameHeight             = (int)pRefPic->size();

    int mbSAD                   = 0;

    vector<vector<int>> interpResid(interpMBWidth);

    for (int i = 0; i < interpMBHeight; i++)
    {
        int yPos                = mbGlbYPos + i;

        // Mirror Y if necessary
        if (frameHeight <= yPos)
            yPos                = frameHeight - ( yPos % frameHeight ) - 1;
        else if (0 > yPos)
            yPos                = abs(yPos);

        interpResid[i].resize(interpMBWidth);
        for (int j = 0; j < interpMBWidth; j++)
        {
            int xPos            = mbGlbXPos + j;
            // Mirror X if necessary
            if (frameWidth <= xPos)
                xPos            = frameWidth - ( xPos % frameWidth ) - 1;
            else if (0 > xPos)
                xPos            = abs(xPos);

            // Get the pel
            interpResid[i][j]   = getMBInterpPicPel(pMB, i, j) - (*pRefPic)[yPos][xPos];
        }
    }

    resid.resize(pMB->MBHeight);
    for (int ii = 0, i = 0; ii < interpMBHeight; ii += interpFactor, i++)
    {
        resid[i].resize(pMB->MBWidth);
        for (int jj = 0, j = 0; jj < interpMBWidth; jj += interpFactor, j++)
            resid[i][j]         = interpResid[ii][jj];
    }

    
    sad4x4.resize(pMB->MBHeight / MACROBLOCK_SIZE_4x4_HEIGHT);
    for (int si = 0, ii = 0; ii < interpMBWidth; ii += (int)( interpMBHeight / sad4x4.size() ), si++)
    {
        sad4x4[si].resize(pMB->MBWidth / MACROBLOCK_SIZE_4x4_WIDTH);
        memset(sad4x4[si].data(), 0, sizeof(int) * sad4x4[si].size());

        for (int sj = 0, jj = 0; jj < interpMBWidth; jj += (int)( interpMBWidth / sad4x4[si].size() ), sj++)
        {
            for (int i = 0; i < ( interpMBHeight / sad4x4.size() ); i++)
                for (int j = 0; j < ( interpMBWidth / sad4x4[si].size() ); j++)
                    sad4x4[si][sj]      += abs(interpResid[ii+i][jj+j]);
        }
    }
}

void nnsME ( Macroblock * pMB , vector<vector<int>> &sad4x4 , vector<vector<MotionVector>> &mv )
{
    _ASSERT(NULL != pMB);
}

void nullSearchME ( Macroblock * pMB , vector<vector<int>> &bestSAD4x4 , vector<vector<MotionVector>> &bestMV4x4 )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);
    _ASSERT(0 != pMB->pSlice->List0.size());

    // This picture is from the DPB
    PicUint8_t * pRefPic        = pMB->pSlice->pInterpRefPic;
    // No residual subdivisions are necessary in this simple mode.
    MBSubBlk<int> mbResid;
    if (COLOR_PLANE_YUV_LUMA == pMB->ColorPlane)
    {
        bestMV4x4.resize(MACROBLOCK_SIZE_4x4_HEIGHT);
        bestSAD4x4.resize(MACROBLOCK_SIZE_4x4_HEIGHT);
    }
    else
    {
        bestMV4x4.resize(MACROBLOCK_SIZE_4x4_HEIGHT / 2);
        bestSAD4x4.resize(MACROBLOCK_SIZE_4x4_HEIGHT / 2);
    }

    for (int i = 0; i < bestMV4x4.size(); i++)
    {
        bestMV4x4[i].resize(bestMV4x4.size());
        bestSAD4x4[i].resize(bestSAD4x4.size());
        memset(&bestSAD4x4[i][0], INT_MAX, sizeof(int) * bestSAD4x4[i].size());
    }

    vector<vector<int>> sad4x4;
    MotionVector mbMV           = {0, 0};
    mbMESAD4x4(pMB, pRefPic, mbResid.Pel, sad4x4, mbMV); 

    // Perform the update to the best MV
    for (int i = 0; i < bestSAD4x4.size(); i++)
    {
        for (int j = 0; j < bestSAD4x4[i].size(); j++)
        {
            if (sad4x4[i][j] < bestSAD4x4[i][j])
            {
                bestSAD4x4[i][j]        = sad4x4[i][j];
                bestMV4x4[i][j]         = mbMV;
            }
        }
    }
    // Determine the best partitioning
    bool bPSkip                 = true;
    for (int i = 0; i < bestSAD4x4.size(); i++)
    {
        for (int j = 0; j < bestSAD4x4[i].size(); j++)
        {
            bPSkip              &= ( 0 == bestMV4x4[i][j].X ) && ( 0 == bestMV4x4[i][j].Y );
        }
    }

    pMB->MBResid.push_back(mbResid);
    pMB->PredMode               = bPSkip ? MB_PRED_MODE_PSKIP : MB_PRED_MODE_P16x16;
    pMB->PartitionType          = MB_PARTITION_16x16;
    pMB->NumMBPart              = (int)pMB->MBResid.size();
    pMB->NumSubBlks             = (int)pMB->MBResid.size();    
}

/*
To support multiple partitions let's first determin the NULL case.
    After we figure out the scores of each sub block grouping,
    we can determine if we need to compute separte mv for sub-block
    regions that have high sad scores. Therefore we will keep track 
    of the best motion vectors for each 4x4 subblock, and perform 
    groupings among partitions that have similar best motion vectors.
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Encode

void interMBEnc ( Macroblock * pMB , INTER_PRED_SEARCH_PATTERN searchPattern )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);

    vector<vector<MotionVector>> bestMV4x4;

    void (* pMESearchFunc)( Macroblock * pMB , vector<vector<int>> &sad4x4 , vector<vector<MotionVector>> &mv );

    pMESearchFunc               = NULL;
    switch (searchPattern)
    {
    case INTER_PRED_SEARCH_PATTERN_FULL:
        break;
    case INTER_PRED_SEARCH_PATTERN_NNS:
        pMESearchFunc           = &nnsME;
        break;
    case INTER_PRED_SEARCH_PATTERN_NULL:
        pMESearchFunc           = &nullSearchME;
        break;
    default:
        break;
    }
    
    vector<vector<int>> sad4x4;

    // Initialize the MB to a default state.
    pMB->NumMBPart          = 1;
    if (COLOR_PLANE_YUV_LUMA == pMB->ColorPlane)
    {
        pMB->PredMode           = MB_PRED_MODE_P16x16;
        pMB->SubBlkWidth        = MACROBLOCK_SIZE_16x16_WIDTH;
        pMB->SubBlkHeight       = MACROBLOCK_SIZE_16x16_HEIGHT;
        pMB->NumSubBlks         = MACROBLOCK_SIZE_16x16_NUM_PEL / ( pMB->SubBlkWidth * pMB->SubBlkHeight );
        pMB->bTrans8x8          = false;
    }
    else
    {
        pMB->PredMode           = MB_PRED_MODE_P8x8;
        pMB->SubBlkWidth        = MACROBLOCK_SIZE_8x8_WIDTH;
        pMB->SubBlkHeight       = MACROBLOCK_SIZE_8x8_HEIGHT;
        pMB->NumSubBlks         = MACROBLOCK_SIZE_8x8_NUM_PEL / ( pMB->SubBlkWidth * pMB->SubBlkHeight );
        pMB->bTrans8x8          = false;
    }

    pMESearchFunc(pMB, sad4x4, pMB->MV);


    // Add the MB to the DPB
    vector<vector<int>> qtcoef;    
    vector<vector<int>> reconResid;
    Frame * pRefFrame;
    int li                      = 0; // Ref Pic List index
    for (int i = (int)pMB->pSlice->List0.size() - 1; i >= 0; i--)
    {
        ENCFRAME_TYPE encFrameType  = pMB->pSlice->List0[i].EncFrameType;   
        if (ENCFRAME_TYPE_I == encFrameType || 
            ENCFRAME_TYPE_IDR == encFrameType || 
            ENCFRAME_TYPE_IPCM == encFrameType)
        {
            pRefFrame           = pMB->pSlice->List0[i].pFrame;
            li                  = i;
            pMB->RefPicIdx      = i;
            break;
        }
    }

    int subBlkWidth             = pMB->SubBlkWidth;
    int subBlkHeight            = pMB->SubBlkHeight;

    vector<vector<int>> dpbSubBlk(subBlkHeight);
    for (int i = 0; i < subBlkHeight; i++)
        dpbSubBlk[i].resize(subBlkWidth);

    for (int si = 0; si < pMB->MBResid.size(); si++)
    {
        transQuantMBResid(pMB->MBResid[si].Pel, qtcoef, pMB->bTrans8x8, pMB->QP);
        // Determine the cbp for the current residual
        invTransQuantMBResid(qtcoef, reconResid, pMB->bTrans8x8, pMB->QP);

        // Add the residual to the DPB ref Pic from List0 or List1
        for (int i = 0; i < subBlkHeight; i++)
            for (int j = 0; j < subBlkWidth; j++)
                dpbSubBlk[i][j]     = reconResid[i][j] + getMBList0SubBlkPel(pMB, li, si, i, j);

        copyMBSubBlkToDPB(pMB, si, dpbSubBlk);
    }

    // For Inter-MB we always have a CBP

}

void interSliceEnc ( Slice * pSlice , INTER_PRED_SEARCH_PATTERN searchPattern )
{
    _ASSERT(NULL != pSlice);

    PicUint8_t * pInterpRefPic          = pSlice->pInterpRefPic;
    PicUint8_t * pInterpPic             = pSlice->pInterpPic;

    _ASSERT(NULL != pInterpRefPic);
    if (0 == pInterpRefPic->size())
    {
        Frame * pRefFrame               = pSlice->List0.back().pFrame;
        Frame * pCurrFrame              = pSlice->pFrame;

        if (COLOR_PLANE_YUV_LUMA == pSlice->ColorPlane)
        {
            interpLuma(pRefFrame, *pInterpRefPic);
            interpLuma(pCurrFrame, *pInterpPic);
        }
        else
        {
            interpChroma(pRefFrame, *pInterpRefPic, pSlice->ColorPlane);
            interpChroma(pCurrFrame, *pInterpPic,  pSlice->ColorPlane);
        }
    }

    for (int i = 0; i < pSlice->NumMB; i++)
    {
        // Unlike in intra mode, we'll let the search
        // determine the best size for the partitioning.
        // The above is just an initialization step.
        interMBEnc(&pSlice->MB[i], searchPattern);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Decode

void interMBDec ( Macroblock * pMB )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);

    int subBlkWidth             = pMB->SubBlkWidth;
    int subBlkHeight            = pMB->SubBlkHeight;
    int li                      = pMB->RefPicIdx;

    vector<vector<int>> dpbSubBlk(subBlkHeight);
    for (int i = 0; i < subBlkHeight; i++)
        dpbSubBlk[i].resize(subBlkWidth);

    for (int si = 0; si < pMB->MBResid.size(); si++)
    {
        // Add the residual to the DPB ref Pic from List0 or List1
        for (int i = 0; i < subBlkHeight; i++)
            for (int j = 0; j < subBlkWidth; j++)
                dpbSubBlk[i][j]     = pMB->MBResid[si].Pel[i][j] + getMBList0SubBlkPel(pMB, li, si, i, j);

        copyMBSubBlkToDPB(pMB, si, dpbSubBlk);
    }
}

void interSliceDec ( Slice * pSlice )
{
    _ASSERT(NULL != pSlice);

    for (int i = 0; i < pSlice->NumMB; i++)
        interMBDec(&pSlice->MB[i]);    
}

