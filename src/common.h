///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        common.h
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include "frame.hpp"

using namespace std;

#define COLOR_PLANE_BITDEPTH            8

typedef vector<vector<uint8_t>>         PicUint8_t;

enum COLOR_PLANE_YUV
{
    COLOR_PLANE_YUV_LUMA,
    COLOR_PLANE_YUV_CHROMA_B,
    COLOR_PLANE_YUV_CHROMA_R,
    COLOR_PLANE_YUV_NUM_PLANES,
};

enum COLOR_PLANE_RGB
{
    COLOR_PLANE_RGB_R,
    COLOR_PLANE_RGB_G,
    COLOR_PLANE_RGB_B,
    COLOR_PLANE_RGB_NUM_PLANES,
};

enum COLOR_PLANE_BGR
{
    COLOR_PLANE_BGR_B,
    COLOR_PLANE_BGR_G,
    COLOR_PLANE_BGR_R,
    COLOR_PLANE_BGR_NUM_PLANES,
};

template <class T>
uint8_t clipUint8_t ( T val )
{
    T tmp           = 0 > val ? 0 : val;
    uint8_t valOut  = (uint8_t)( 255 < tmp ? 255 : tmp );
    return valOut;
}