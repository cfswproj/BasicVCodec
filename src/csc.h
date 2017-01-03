///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        csc.h
// Abstract:    Function declarations for performing various color space conversions.
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stdint.h>

class Frame;
enum CSC_TYPE
{
    CSC_TYPE_RGB_TO_YUV420P_UINT8,
    CSC_TYPE_BGR_TO_YUV420P_UINT8,
    CSC_TYPE_YUV420P_TO_BGR_UINT8,
    CSC_TYPE_YUV420P_TO_RGB_UINT8,
};

void colorSpaceCvt ( const void * pSrc , void * pDst , int width , int height , CSC_TYPE type );


void colorSpaceCvt ( const Frame * pSrc , Frame * pDst , CSC_TYPE type );