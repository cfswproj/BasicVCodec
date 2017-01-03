///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        macroblock.h
// Abstract:    Function definitions for encoding I-Frames
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include "macroblock.h"
#include "slice.h"


void setMBSliceInfo ( Slice * pSlice , Macroblock * pMB )
{
    pMB->pSlice             = pSlice;
    pMB->SliceID            = pSlice->SliceID;
    pMB->ColorPlane         = pSlice->ColorPlane;
    pMB->QP                 = pSlice->QP;
}

uint8_t getMBRefPicPel ( const Macroblock * pMB , int i , int j )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);
    _ASSERT(NULL != pMB->pSlice->pFrame);

    
    int iPos                    = ( pMB->YPos * pMB->MBHeight ) + i;
    int jPos                    = ( pMB->XPos * pMB->MBWidth ) + j;

    return pMB->pSlice->pFrame->getPel(iPos, jPos, pMB->ColorPlane);
}

uint8_t getMBInterpRefPicPel ( const Macroblock * pMB , int i , int j )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);
    _ASSERT(NULL != pMB->pSlice->pFrame);

    int interpFactor            = COLOR_PLANE_YUV_LUMA ==  pMB->ColorPlane ? 
                                    MB_INTERP_FACTOR_LUMA : MB_INTERP_FACTOR_CHROMA;
    int iPos                    = ( pMB->YPos * pMB->MBHeight * interpFactor ) + i;
    int jPos                    = ( pMB->XPos * pMB->MBWidth * interpFactor ) + j;

    return (*pMB->pSlice->pInterpRefPic)[iPos][jPos];
}

uint8_t getMBInterpPicPel ( const Macroblock * pMB , int i , int j )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);
    _ASSERT(NULL != pMB->pSlice->pFrame);

    int interpFactor            = COLOR_PLANE_YUV_LUMA ==  pMB->ColorPlane ? 
                                    MB_INTERP_FACTOR_LUMA : MB_INTERP_FACTOR_CHROMA;
    int iPos                    = ( pMB->YPos * pMB->MBHeight * interpFactor ) + i;
    int jPos                    = ( pMB->XPos * pMB->MBWidth * interpFactor ) + j;

    return (*pMB->pSlice->pInterpPic)[iPos][jPos];
}

uint8_t getMBDPBPel ( const Macroblock * pMB , int i , int j )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);
    _ASSERT(NULL != pMB->pSlice->pFrame);
    
    int iPos                    = ( pMB->YPos * pMB->MBHeight ) + i;
    int jPos                    = ( pMB->XPos * pMB->MBWidth ) + j;

    return pMB->pSlice->pDPBFrame->getPel(iPos, jPos, pMB->ColorPlane);
}

uint8_t &getMBDPBPelRef ( Macroblock * pMB , int i , int j )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);
    _ASSERT(NULL != pMB->pSlice->pFrame);
    
    int iPos                    = ( pMB->YPos * pMB->MBHeight ) + i;
    int jPos                    = ( pMB->XPos * pMB->MBWidth ) + j;

    return pMB->pSlice->pDPBFrame->getPelRef(iPos, jPos, pMB->ColorPlane);
}

uint8_t * getMBDPBPelPtr ( Macroblock * pMB , int i , int j )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);
    _ASSERT(NULL != pMB->pSlice->pFrame);
    
    int iPos                    = ( pMB->YPos * pMB->MBHeight ) + i;
    int jPos                    = ( pMB->XPos * pMB->MBWidth ) + j;

    return pMB->pSlice->pDPBFrame->getPelPtr(iPos, jPos, pMB->ColorPlane);
}

void copyRefPicMB ( const Macroblock * pMB , vector<vector<uint8_t>> &mbPel )
{
    _ASSERT(NULL != pMB);

    mbPel.resize(pMB->MBHeight);
    for (int i = 0; i < mbPel.size(); i++)
    {
        mbPel[i].resize(pMB->MBWidth);
        for (int j = 0; j < mbPel[i].size(); j++)
            mbPel[i][j]         = getMBRefPicPel(pMB, pMB->YPos+i, pMB->XPos+j);
    }
}

void copyDPBMB ( const Macroblock * pMB , vector<vector<uint8_t>> &mbPel )
{
    _ASSERT(NULL != pMB);

    mbPel.resize(pMB->MBHeight);
    for (int i = 0; i < mbPel.size(); i++)
    {
        mbPel[i].resize(pMB->MBWidth);
        for (int j = 0; j < mbPel[i].size(); j++)
            mbPel[i][j]         = getMBDPBPel(pMB, i, j);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sub-block Operations
uint8_t getMBRefPicSubBlkPel ( const Macroblock * pMB , int si , int i , int j )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);
    _ASSERT(NULL != pMB->pSlice->pFrame);

    int numSubBlkCols           = pMB->MBWidth / pMB->SubBlkWidth;
    int numSubBlkRows           = pMB->MBHeight / pMB->SubBlkHeight;
    int siiOffset               = ( ( si / numSubBlkRows ) * pMB->SubBlkHeight ) + i;
    int sijoffset               = ( ( si % numSubBlkCols ) * pMB->SubBlkWidth ) + j;
 
    return getMBRefPicPel(pMB, siiOffset, sijoffset);
}

uint8_t getMBDPBSubBlkPel ( const Macroblock * pMB , int si , int i , int j )
{   
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);
    _ASSERT(NULL != pMB->pSlice->pFrame);

    int numSubBlkCols           = pMB->MBWidth / pMB->SubBlkWidth;
    int numSubBlkRows           = pMB->MBHeight / pMB->SubBlkHeight;
    int siiOffset               = ( ( si / numSubBlkRows ) * pMB->SubBlkHeight ) + i;
    int sijoffset               = ( ( si % numSubBlkCols ) * pMB->SubBlkWidth ) + j;

    return getMBDPBPel(pMB, siiOffset, sijoffset);
}

uint8_t &getMBDPBSubBlkPelRef ( Macroblock * pMB , int si , int i , int j )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);
    _ASSERT(NULL != pMB->pSlice->pFrame);

    int numSubBlkCols           = pMB->MBWidth / pMB->SubBlkWidth;
    int numSubBlkRows           = pMB->MBHeight / pMB->SubBlkHeight;
    int siiOffset               = ( ( si / numSubBlkRows ) * pMB->SubBlkHeight ) + i;
    int sijoffset               = ( ( si % numSubBlkCols ) * pMB->SubBlkWidth ) + j;

    return getMBDPBPelRef(pMB, siiOffset, sijoffset);
}

uint8_t * getMBDPBSubBlkPelPtr ( Macroblock * pMB , int si , int i , int j )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);
    _ASSERT(NULL != pMB->pSlice->pFrame);

    int numSubBlkCols           = pMB->MBWidth / pMB->SubBlkWidth;
    int numSubBlkRows           = pMB->MBHeight / pMB->SubBlkHeight;
    int siiOffset               = ( ( si / numSubBlkRows ) * pMB->SubBlkHeight ) + i;
    int sijoffset               = ( ( si % numSubBlkCols ) * pMB->SubBlkWidth )+ j;
    
    return getMBDPBPelPtr(pMB, siiOffset, sijoffset);
}

uint8_t getMBList0SubBlkPel ( const Macroblock * pMB , int li , int si , int i , int j )
{
    _ASSERT(NULL != pMB);
    _ASSERT(NULL != pMB->pSlice);
    _ASSERT(NULL != pMB->pSlice->pFrame);

    int numSubBlkCols           = pMB->MBWidth / pMB->SubBlkWidth;
    int numSubBlkRows           = pMB->MBHeight / pMB->SubBlkHeight;
    int siiOffset               = ( ( si / numSubBlkRows ) * pMB->SubBlkHeight ) + i;
    int sijoffset               = ( ( si % numSubBlkCols ) * pMB->SubBlkWidth ) + j;
 
    RefPic * pRefPic            = &pMB->pSlice->List0[li];

    return pRefPic->pFrame->getPel(siiOffset, sijoffset, pMB->ColorPlane);
}

void copyRefPicMBSubBlk ( const Macroblock * pMB , int si , vector<vector<uint8_t>> &mbPel )
{
    _ASSERT(NULL != pMB);

    mbPel.resize(pMB->SubBlkHeight);
    for (int i = 0; i < mbPel.size(); i++)
    {
        mbPel[i].resize(pMB->SubBlkWidth);
        for (int j = 0; j < mbPel[i].size(); j++)
            mbPel[i][j]         = getMBRefPicSubBlkPel(pMB, si, i, j);
    }
}

void copyDPBMBSubBlk ( const Macroblock * pMB , int si , vector<vector<uint8_t>> &mbPel )
{
    _ASSERT(NULL != pMB);

    mbPel.resize(pMB->SubBlkHeight);
    for (int i = 0; i < mbPel.size(); i++)
    {
        mbPel[i].resize(pMB->SubBlkWidth);
        for (int j = 0; j < mbPel[i].size(); j++)
            mbPel[i][j]         = getMBDPBSubBlkPel(pMB, si, i, j);
    }
}

void copyRefPicMBSubBlk ( const Macroblock * pMB , int si , vector<vector<int>> &mbPel )
{
    _ASSERT(NULL != pMB);

    mbPel.resize(pMB->SubBlkHeight);
    for (int i = 0; i < mbPel.size(); i++)
    {
        mbPel[i].resize(pMB->SubBlkWidth);
        for (int j = 0; j < mbPel[i].size(); j++)
        {
            mbPel[i][j]         = (int)getMBRefPicSubBlkPel(pMB, si, i, j);
        }
    }
}

void copyDPBMBSubBlk ( const Macroblock * pMB , int si , vector<vector<int>> &mbPel )
{
    _ASSERT(NULL != pMB);

    mbPel.resize(pMB->SubBlkHeight);
    for (int i = 0; i < mbPel.size(); i++)
    {
        mbPel[i].resize(pMB->SubBlkWidth);
        for (int j = 0; j < mbPel[i].size(); j++)
            mbPel[i][j]         = (int)getMBDPBSubBlkPel(pMB, si, i, j);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1-D Subblock
void copyRefPicMBSubBlk ( const Macroblock * pMB , int si , vector<uint8_t> &mbPel )
{
    _ASSERT(NULL != pMB);

    mbPel.resize(pMB->SubBlkHeight * pMB->SubBlkWidth);
    for (int i = 0; i < pMB->SubBlkHeight; i++)
    {
        for (int j = 0; j < pMB->SubBlkWidth; j++)
        {
            int pos             = i * pMB->SubBlkWidth + j;
            mbPel[pos]          = getMBRefPicSubBlkPel(pMB, si, i, j);
        }
    }
}

void copyDPBMBSubBlk ( const Macroblock * pMB , int si , vector<uint8_t> &mbPel )
{
    _ASSERT(NULL != pMB);

    mbPel.resize(pMB->SubBlkHeight * pMB->SubBlkWidth);
    for (int i = 0; i < pMB->SubBlkHeight; i++)
    {
        for (int j = 0; j < pMB->SubBlkWidth; j++)
        {
            int pos             = i * pMB->SubBlkWidth + j;
            mbPel[pos]          = getMBDPBSubBlkPel(pMB, si, i, j);
        }
    }
}

void copyRefPicMBSubBlk ( const Macroblock * pMB , int si , vector<int> &mbPel )
{
    _ASSERT(NULL != pMB);

    mbPel.resize(pMB->SubBlkHeight * pMB->SubBlkWidth);
    for (int i = 0; i < pMB->SubBlkHeight; i++)
    {
        for (int j = 0; j < pMB->SubBlkWidth; j++)
        {
            int pos             = i * pMB->SubBlkWidth + j;
            mbPel[pos]          = (int)getMBRefPicSubBlkPel(pMB, si, i, j);
        }
    }
}

void copyDPBMBSubBlk ( const Macroblock * pMB , int si , vector<int> &mbPel )
{
    _ASSERT(NULL != pMB);

    mbPel.resize(pMB->SubBlkHeight * pMB->SubBlkWidth);
    for (int i = 0; i < pMB->SubBlkHeight; i++)
    {
        for (int j = 0; j < pMB->SubBlkWidth; j++)
        {
            int pos             = i * pMB->SubBlkWidth + j;
            mbPel[pos]          = (int)getMBDPBSubBlkPel(pMB, si, i, j);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copy MB to Frame

void copyMBSubBlkToDPB ( Macroblock * pMB , int si , const vector<vector<uint8_t>> &mbPel )
{
    _ASSERT(NULL != pMB);

    for (int i = 0; i < mbPel.size(); i++)
        for (int j = 0; j < mbPel[i].size(); j++)
            getMBDPBSubBlkPelRef(pMB, si, i, j)     = mbPel[i][j];
}

void copyMBSubBlkToDPB ( Macroblock * pMB , int si , const vector<vector<int>> &mbPel )
{
    _ASSERT(NULL != pMB);

    for (int i = 0; i < mbPel.size(); i++)
        for (int j = 0; j < mbPel[i].size(); j++)
            getMBDPBSubBlkPelRef(pMB, si, i, j)     = clipUint8_t(mbPel[i][j]);
}

// 1-D operations
void copyMBSubBlkToDPB ( Macroblock * pMB , int si , const vector<uint8_t> &mbPel )
{
    _ASSERT(NULL != pMB);

    for (int i = 0; i < pMB->SubBlkHeight; i++)
    {
        for (int j = 0; j < pMB->SubBlkWidth; j++)
        {
            int pos                                 = i * pMB->SubBlkWidth + j;
            getMBDPBSubBlkPelRef(pMB, si, i, j)     = mbPel[pos];
        }
    }
}

void copyMBSubBlkToDPB ( Macroblock * pMB , int si , const vector<int> &mbPel )
{
    _ASSERT(NULL != pMB);

    for (int i = 0; i < pMB->SubBlkHeight; i++)
    {
        for (int j = 0; j < pMB->SubBlkWidth; j++)
        {
            int pos                                 = i * pMB->SubBlkWidth + j;
            getMBDPBSubBlkPelRef(pMB, si, i, j)     = clipUint8_t(mbPel[pos]);
        }
    }
}