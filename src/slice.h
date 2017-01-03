///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        slice.h
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include "common.h"
#include "macroblock.h"

enum SLICE_TYPE
{
    SLICE_TYPE_I,   // I, IDR only
    SLICE_TYPE_IPCM,
    SLICE_TYPE_P,   // I and/or P
    SLICE_TYPE_B,   // I, P, and/or B
    SLICE_TYPE_SP,  // P and/or I
    SLICE_TYPE_SI,  // I only
    SLICE_TYPE_NUM_TYPES,
};

class Encoder;
enum ENCFRAME_TYPE;

struct RefPic
{
    Frame *                     pFrame;
    int                         DisplayIdx;
    ENCFRAME_TYPE               EncFrameType;
};

struct SliceHeader
{
    COLOR_PLANE_YUV             ColorPlane;
    SLICE_TYPE                  SliceType;
    int                         QP;
    int                         NumMB;
};

struct Slice
{
    Encoder *                   pEncoder;
    Frame *                     pFrame;
    Frame *                     pDPBFrame;
    int                         SliceID;
    SLICE_TYPE                  SliceType;
    int                         NumMB;

    int                         FrameWidth;
    int                         FrameHeight;

    int                         FrameWidthInMB;
    int                         FrameHeightInMB;

    COLOR_PLANE_YUV             ColorPlane;
    vector<Macroblock>          MB;

    vector<RefPic>              List0;
    PicUint8_t *                pInterpRefPic;
    PicUint8_t *                pInterpPic;

    int                         QP;
    int                         ObjQualScore; // Overflow possible if #MB > 2^32 (equivilantly @ > 4K Res)
};


void        getSliceHeader              ( const Slice * pSlice , SliceHeader & sliceHeader );

void        setSliceHeader              ( Slice * pSlice , const SliceHeader & sliceHeader );

void        frameSlicePartition         ( Encoder * pEncoder ,  Frame * pFrame ,  Frame * pDPB , Slice &slice ,
                                          COLOR_PLANE_YUV plane , int sliceID = 0 );

void        frameSlicePartitionN        ( Encoder * pEncoder , Frame * pFrame ,  Frame * pDPB , vector<Slice> &sliceVec ,
                                          COLOR_PLANE_YUV plane , int numSlices );
