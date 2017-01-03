///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        decoder.hpp
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <cstdint>
#include <vector>

#include "common.h"

using namespace std;


struct DecoderDesc
{
    FrameDesc               FrameDesc_t;
};

class Decoder
{
public:
    Decoder     ( );
    Decoder     ( DecoderDesc desc );
    ~Decoder    ( );

    void                            setDesc                     ( DecoderDesc desc );

    DecoderDesc                     getDesc                     ( ) const;

    void                            init                        ( );
    /*
        Function:       decode
        Description:    Decodes a frame from the bitstream
                        prior to init()
        Side-effects:   Returns the encoded frame info
    */
    int                             decode                      ( const vector<uint8_t> &bitstream , int bsPos , vector<Frame> &frameSeq );

     /*
        Function:       decode
        Description:    Decodes a frame from the bitstream
                        prior to init()
        Side-effects:   Returns the encoded frame info
    */
    int                             decodeFrame                 ( const vector<uint8_t> &bitstream , int bsPos , Frame &frame );


private:

    vector<Frame>         m_decFrameBuf;
    DecoderDesc                     m_decDesc;
    FrameDesc                       m_frameDesc;
    bool                            m_bIsInit;
};