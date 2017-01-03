///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        intraPred.h
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>
#include "macroblock.h"

using namespace std;

enum INTRA_PRED_MODE_LUMA
{
    INTRA_PRED_MODE_LUMA_VERT           = 0,
    INTRA_PRED_MODE_LUMA_HRZN,
    INTRA_PRED_MODE_LUMA_DC,
    INTRA_PRED_MODE_LUMA_DIAGDWNL,
    INTRA_PRED_MODE_LUMA_DIAGDWNR,
    INTRA_PRED_MODE_LUMA_VERTR,
    INTRA_PRED_MODE_LUMA_HRZNDWN,
    INTRA_PRED_MODE_LUMA_VERTL,
    INTRA_PRED_MODE_LUMA_HRZNUP,
    INTRA_PRED_MODE_LUMA_NUM_MODES
};

enum INTRA_PRED_MODE_16x16
{
    INTRA_PRED_MODE_16x16_VERT          = 0,
    INTRA_PRED_MODE_16x16_HRZN,
    INTRA_PRED_MODE_16x16_DC,
    INTRA_PRED_MODE_16x16_PLANE,
    INTRA_PRED_MODE_16x16_NUM_MODES,
};

enum INTRA_PRED_MODE_CHROMA
{
    INTRA_PRED_MODE_CHROMA_DC           = 0,
    INTRA_PRED_MODE_CHROMA_HRZN,
    INTRA_PRED_MODE_CHROMA_VERT,
    INTRA_PRED_MODE_CHROMA_PLANE,
    INTRA_PRED_MODE_CHROMA_NUM_MODES,
};

 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Encode

void    intraPredLumaSliceEnc    ( Slice * pSlice );

void    intraPredChromaSliceEnc  ( Slice * pSliceCb , Slice * pSliceCr );

void    intraPCMLumaSliceEnc    ( Slice * pSlice );

void    intraPCMChromaSliceEnc  ( Slice * pSliceCb , Slice * pSliceCr );

 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Decode
void    intraPredLumaSliceDec   ( Slice * pSlice );

void    intraPredChromaSliceDec ( Slice * pSlice );

void    intraPCMLumaSliceDec    ( Slice * pSlice );

void    intraPCMChromaSliceDec  ( Slice * pSlice );