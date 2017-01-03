///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        encoder.hpp
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <cstdint>
#include <vector>

#include "common.h"
#include "frame.hpp"
#include "slice.h"

using namespace std;

enum ENCFRAME_TYPE
{
    ENCFRAME_TYPE_I,
    ENCFRAME_TYPE_IDR,
    ENCFRAME_TYPE_IPCM,
    ENCFRAME_TYPE_P,
    ENCFRAME_TYPE_B,
    ENCFRAME_TYPE_NUM_TYPES,
};

enum ENC_ME_SEARCH_PATTERN
{
    ENC_ME_SEARCH_PATTERN_FULL,
    ENC_ME_SEARCH_PATTERN_NNS,
    ENC_ME_SEARCH_PATTERN_NULL,
    ENC_ME_SEARCH_PATTERN_NUM_PATTERNS,
};

struct EncFrameInfo
{
    size_t                  NumBytes;
    float                   RateDist;
    int                     QP;
    float                   CompressionRate;
    int                     BitstreamBytePos;
};

struct RateDistDesc
{
    int                     QP; // AVC quality parameter
    float                   MinRD;
    float                   MaxRD;
    bool                    bAdaptiveRD;
};

struct EncoderDesc
{
    FrameDesc               FrameDesc_t;
    RateDistDesc            RDDesc;
    vector<ENCFRAME_TYPE>   EncFramePattern;
    ENC_ME_SEARCH_PATTERN   MESearchPattern;
};

struct EncoderState
{
    EncFrameInfo *          pCurrEncFrameInfo;
    EncFrameInfo *          pPrevEncFrameInfo;           
    int                     EncFrameCnt;
    int                     EncByteCnt;
};

class Encoder
{
public:
    Encoder     ( );
    Encoder     ( EncoderDesc desc );
    ~Encoder    ( );

    void                            setDesc                     ( EncoderDesc desc );

    EncoderDesc                     getDesc                     ( ) const;

    const EncoderState *            getEncoderState             ( ) const;

    Frame *                         getDPBPtr                   ( int id ) const;

    PicUint8_t *                    getInterpRefPic             ( COLOR_PLANE_YUV plane );

    PicUint8_t *                    getInterpPic                ( COLOR_PLANE_YUV plane );

    const vector<RefPic> *          getRefPicListPtr            ( ) const;

    void                            pushDPB                     ( const Frame &frame );

    void                            init                        ( );
    /*
        Function:       encodeFrame
        Description:    Encodes a frame based on the encoder parameters set at
                        prior to init()
        Side-effects:   Returns the encoded frame info
    */
    EncFrameInfo                    encodeFrame                 ( const Frame &frame , vector<uint8_t> &bitstream );
    void                            encode                      ( const vector<Frame> &frameSeq , vector<uint8_t> &bitstream ,
                                                                  vector<EncFrameInfo> &encFrameInfo );

    void                            testDecode ( vector<uint8_t> &bitstream );

private:

    void                            updateEncoderState ( );

    vector<Frame *>                 m_dpb;
    vector<RefPic>                  m_refPics;          // All I frames and P frames...
    vector<EncFrameInfo>            m_encFrameInfoBuf;
    EncoderDesc                     m_encDesc;
    FrameDesc                       m_frameDesc;

    EncoderState                    m_encoderState;

    vector<vector<Slice> *>         m_sliceVec[COLOR_PLANE_YUV_NUM_PLANES];

    vector<vector<uint8_t>>         m_sliceCBP;

    PicUint8_t                      m_interpRefPic[COLOR_PLANE_YUV_NUM_PLANES];
    PicUint8_t                      m_interpPic[COLOR_PLANE_YUV_NUM_PLANES];
    bool                            m_bIsInit;
};