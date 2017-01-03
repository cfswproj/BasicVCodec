///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        encoder.cpp
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////


#include "decoder.hpp"
#include "csc.h"

Decoder::Decoder ( )
{
    memset(&m_decDesc, 0, sizeof(DecoderDesc));
    m_bIsInit           = false;
}

Decoder::Decoder ( DecoderDesc desc )
{
   setDesc(desc);
    m_bIsInit           = false;
}

Decoder::~Decoder ( )
{
}

void Decoder::setDesc ( DecoderDesc desc )
{
    m_decDesc       = desc;
    m_frameDesc     = m_decDesc.FrameDesc_t;
}

DecoderDesc Decoder::getDesc ( ) const
{
    return m_decDesc;
}

void Decoder::init ( ) 
{
    if (m_bIsInit)
        return;

    m_bIsInit                   = true;
}

int Decoder::decodeFrame ( const vector<uint8_t> &bitstream , int bsPos , Frame &frame )
{
    if (!m_bIsInit)
        return 0;

    Frame yuvFrame;
    FrameDesc yuvDesc;

    int frameWidth              = m_frameDesc.FrameWidth;
    int frameHeight             = m_frameDesc.FrameHeight;
    int sizeYUVFrameInBytes     = ( frameWidth * frameHeight * 3 ) / 2;

    yuvDesc.FrameFmt            = IMAGE_COLOR_FORMAT_YUV420P;
    yuvDesc.FrameWidth          = frameWidth;
    yuvDesc.FrameHeight         = frameHeight;

    yuvFrame.setFrame(bitstream.data(), yuvDesc); 


    // Entropy Decode
    

    // Decode all MB in slice
    // Perform CSC
    switch (m_frameDesc.FrameFmt)
    {
    default:
    case IMAGE_COLOR_FORMAT_RGB:
        colorSpaceCvt(&yuvFrame, &frame, CSC_TYPE_YUV420P_TO_RGB_UINT8);
        break;
    case IMAGE_COLOR_FORMAT_BGR:
        colorSpaceCvt(&yuvFrame, &frame, CSC_TYPE_YUV420P_TO_BGR_UINT8);
        break;
    }

    bsPos                       += sizeYUVFrameInBytes;
    // return...
    return bsPos;
}

int Decoder::decode ( const vector<uint8_t> &bitstream , int bsPos , vector<Frame> &frameSeq )
{
    if (!m_bIsInit)
        return 0;

    int decPos              = 0;
    for (int i = 0; i < frameSeq.size(); i++)
        decPos  = decodeFrame(bitstream, bsPos, frameSeq[i]);

    return decPos;
}