///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        frame.h
// Abstract:    
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////



#pragma once

#include <cstdint>
#include <vector>

#include "common.h"

using namespace std;

enum IMAGE_COLOR_FORMAT
{
    IMAGE_COLOR_FORMAT_RGB,
    IMAGE_COLOR_FORMAT_BGR,
    IMAGE_COLOR_FORMAT_YUV420P,
};

struct FrameDesc
{
    int                     FrameHeight;
    int                     FrameWidth;
    IMAGE_COLOR_FORMAT      FrameFmt;
};


class Frame
{
public:
    Frame   ( );
    Frame   ( Frame &frame );
    Frame   ( const uint8_t * pData , FrameDesc desc );
    ~Frame  ( );

    void                setFrame        ( const uint8_t * pData , FrameDesc desc );

    uint8_t *           getDataRow      ( int i , int chnl ) const;

    uint8_t             getPel          ( int i , int j , int chnl ) const;

    uint8_t             &getPelRef      ( int i , int j , int chnl );
  
    uint8_t *           getPelPtr       ( int i , int j , int chnl );

    int                 height          ( int chnl ) const;

    int                 width           ( int chnl ) const;

    size_t              sizeInBytes     ( ) const;

    FrameDesc           getFrameDesc    ( ) const;

    void                copy ( Frame &frame );

    void                fprintf ( string fname );

private:
    FrameDesc               m_frameDesc_t;
    vector<vector<vector<uint8_t>>> m_data;
};