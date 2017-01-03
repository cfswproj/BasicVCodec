///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        predCoding.h
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include "common.h"
#include "slice.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Encode
void    predSliceLumaEnc       ( Slice * pSlice );
void    predSliceChromaEnc     ( Slice * pSliceCb , Slice * pSliceCr );
void    predSliceEnc           ( Slice * pSliceY , Slice * pSliceCb , Slice * pSliceCr );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Decode
void    predSliceLumaDec       ( Slice * pSlice );
void    predSliceChromaDec     ( Slice * pSlice );
void    predSliceChromaDec     ( Slice * pSliceCb , Slice * pSliceCr );