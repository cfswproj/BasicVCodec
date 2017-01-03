///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        encoder.cpp
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////


#include "encoder.hpp"
#include "csc.h"
#include "qualityMetrics.h"
#include "transform.h"
#include "predCoding.h"
#include "syntax.h"

#define NUM_BITS_PER_BYTE                   8
Encoder::Encoder ( )
{
    memset(&m_encDesc, 0, sizeof(EncoderDesc));
    memset(&m_encoderState, 0, sizeof(EncoderState));
    m_bIsInit           = false;
}

Encoder::Encoder ( EncoderDesc desc )
{
    setDesc(desc);    
    memset(&m_encoderState, 0, sizeof(EncoderState));
    m_bIsInit           = false;
}

Encoder::~Encoder ( )
{
    while (!m_dpb.empty())
    {
        if (NULL != m_dpb.back())
            delete m_dpb.back();
        m_dpb.pop_back();
    }
}

void Encoder::setDesc ( EncoderDesc desc )
{
    m_encDesc           = desc;
    m_frameDesc         = m_encDesc.FrameDesc_t;
}

EncoderDesc Encoder::getDesc ( ) const
{
    return m_encDesc;
}

const EncoderState * Encoder::getEncoderState ( ) const
{
    return &m_encoderState;
}

Frame * Encoder::getDPBPtr ( int id ) const
{
    if (m_dpb.size() > id)
        return m_dpb[id];

    return NULL;
}

PicUint8_t * Encoder::getInterpRefPic ( COLOR_PLANE_YUV plane )
{
    if (COLOR_PLANE_YUV_NUM_PLANES > plane)
        return &m_interpRefPic[plane];

    return NULL;
}

PicUint8_t * Encoder::getInterpPic ( COLOR_PLANE_YUV plane )
{
    if (COLOR_PLANE_YUV_NUM_PLANES > plane)
        return &m_interpPic[plane];

    return NULL;
}

const vector<RefPic> * Encoder::getRefPicListPtr ( ) const
{
    return &m_refPics;
}

void Encoder::init ( )
{
    if (m_bIsInit)
        return;
    m_bIsInit                   = true;
}

EncFrameInfo Encoder::encodeFrame ( const Frame &frame , vector<uint8_t> &bitstream )
{
    if (!m_bIsInit)
        return EncFrameInfo();

    EncFrameInfo encFrameInfo;

    FrameDesc yuvFrameDesc;

    yuvFrameDesc                = m_frameDesc;
    yuvFrameDesc.FrameFmt       = IMAGE_COLOR_FORMAT_YUV420P;

    Frame * pYUVFrame           = new Frame(NULL, yuvFrameDesc);

    // Perform CSC
    switch (m_frameDesc.FrameFmt)
    {
    default:
    case IMAGE_COLOR_FORMAT_RGB:
        colorSpaceCvt(&frame, pYUVFrame, CSC_TYPE_RGB_TO_YUV420P_UINT8);

        break;
    case IMAGE_COLOR_FORMAT_BGR:
        colorSpaceCvt(&frame, pYUVFrame, CSC_TYPE_BGR_TO_YUV420P_UINT8);
        break;
    }

    // Create a new decoded picture buffer frame.
    Frame * pDPBFrame           = new Frame(NULL, yuvFrameDesc);
    m_dpb.push_back(pDPBFrame);

    m_dpb.back()->copy(*pYUVFrame);

    const int numSlices         = 1;

    vector<Slice> * pSliceVec   = new vector<Slice>[COLOR_PLANE_YUV_NUM_PLANES];

    for (int c = 0; c < COLOR_PLANE_YUV_NUM_PLANES; c++)
    {
        pSliceVec[c].resize(numSlices);
        // Slice up the new frame
        frameSlicePartitionN(this, pYUVFrame, pDPBFrame, pSliceVec[c], (COLOR_PLANE_YUV)c, numSlices);
        m_sliceVec[c].push_back(&pSliceVec[c]);
    }

    // Perform prediction and transform coding for each MB within the pSliceVec.
    m_sliceCBP.resize(numSlices);
    for (int i = 0; i < numSlices; i++)
    {
        Slice * pSliceY             = &pSliceVec[COLOR_PLANE_YUV_LUMA][i];
        Slice * pSliceCb            = &pSliceVec[COLOR_PLANE_YUV_CHROMA_B][i];
        Slice * pSliceCr            = &pSliceVec[COLOR_PLANE_YUV_CHROMA_R][i];

        predSliceEnc(pSliceY, pSliceCb, pSliceCr);

        transQuantSlice(pSliceY);
        transQuantSlice(pSliceCb);
        transQuantSlice(pSliceCr);

        // Entropy Encode...
        m_sliceCBP[i].resize(pSliceY->MB.size());
        for (int mi = 0; mi < m_sliceCBP[i].size(); mi++)
        {
            m_sliceCBP[i][mi]  = mbCBP(&pSliceY->MB[mi], &pSliceCb->MB[mi], &pSliceCr->MB[mi]);
        }
        if (SLICE_TYPE_B != pSliceY->SliceType ||
            SLICE_TYPE_SP != pSliceY->SliceType ||
            SLICE_TYPE_SI != pSliceY->SliceType)
        {
            // Let's add this decoded picture to this slices reference list
            // For now let's maintain the P-Frame reference lists
            ENCFRAME_TYPE frameType;
            switch (pSliceY->SliceType)
            {
            case SLICE_TYPE_I:
                frameType           = ENCFRAME_TYPE_I;
                break;
            case SLICE_TYPE_IPCM:
                frameType           = ENCFRAME_TYPE_IPCM;
                break;
            case SLICE_TYPE_P:
                frameType           = ENCFRAME_TYPE_P;
                break;
            default:
                break;
            }
            RefPic refPic           = {pDPBFrame, m_encoderState.EncFrameCnt, frameType};
            m_refPics.push_back(refPic);
        }
    }
    
    for (int ci = 0; ci < COLOR_PLANE_YUV_NUM_PLANES; ci++)
        m_interpRefPic[ci].clear();

    for (int ci = 0; ci < COLOR_PLANE_YUV_NUM_PLANES; ci++)
        for (int i = 0; i < pDPBFrame->height(ci); i++)
            for (int j = 0; j < pDPBFrame->width(ci); j++)
                bitstream.push_back(pDPBFrame->getPel(i, j, ci));

    encFrameInfo.BitstreamBytePos       = m_encoderState.EncByteCnt;

    encFrameInfo.NumBytes               = encFrameInfo.BitstreamBytePos;
    // return...
    m_encFrameInfoBuf.push_back(encFrameInfo);

    updateEncoderState();
    return encFrameInfo;
}

void Encoder::encode ( const vector<Frame> &frameSeq , vector<uint8_t> &bitsteram , vector<EncFrameInfo> &encFrameInfo )
{
    if (!m_bIsInit)
        return;

    for (int i = 0; i < frameSeq.size(); i++)
        encFrameInfo.push_back(encodeFrame(frameSeq[i], bitsteram));
}

////////////////////////////////

void Encoder::updateEncoderState ( ) 
{
    if (!m_bIsInit)
        return;

    m_encoderState.EncFrameCnt++;
    m_encoderState.pPrevEncFrameInfo        = m_encoderState.pCurrEncFrameInfo;
    m_encoderState.pCurrEncFrameInfo        = &m_encFrameInfoBuf.back();
    m_encoderState.EncByteCnt               += (int)m_encoderState.pCurrEncFrameInfo->NumBytes;
}

void Encoder::testDecode ( vector<uint8_t> &bitstream )
{

    Frame * pYUVFrame           = new Frame(NULL, m_dpb.back()->getFrameDesc());
    Frame * pDPBFrame           = new Frame(NULL, m_dpb.back()->getFrameDesc());

    vector<Slice> decodedSlice[COLOR_PLANE_YUV_NUM_PLANES];
    int numSlices = 1;
    for (int c = 0; c < COLOR_PLANE_YUV_NUM_PLANES; c++)
    {
        decodedSlice[c].resize(numSlices);
        // Slice up the new frame
        frameSlicePartitionN(this, pYUVFrame, pDPBFrame, decodedSlice[c], (COLOR_PLANE_YUV)c, numSlices);

        // for each slice 
        for (int i = 0; i < decodedSlice[c].size(); i++)
        {
            Slice * pSlice          = &decodedSlice[c][i];

            pSlice->SliceID         = i;
            pSlice->pDPBFrame       = pDPBFrame;
            pSlice->pEncoder        = this;
            pSlice->QP              = m_encDesc.RDDesc.QP;

            pSlice->SliceType       = (*m_sliceVec[c].back())[i].SliceType;

            for (int mi = 0; mi < pSlice->NumMB; mi++)
            {
                Macroblock * pDecMB         = &pSlice->MB[mi];
                const Macroblock * pEncMB   = &(*m_sliceVec[c].back())[i].MB[mi];
                pDecMB->MBHeight            = pEncMB->MBHeight;
                pDecMB->MBWidth             = pEncMB->MBWidth;
                pDecMB->SubBlkHeight        = pEncMB->SubBlkHeight;
                pDecMB->SubBlkWidth         = pEncMB->SubBlkWidth;
                pDecMB->NumSubBlks          = pEncMB->NumSubBlks;
                pDecMB->NumMBPart           = pEncMB->NumMBPart;
                pDecMB->QP                  = pEncMB->QP;
                pDecMB->PredMode            = pEncMB->PredMode;
                pDecMB->IntraModes          = pEncMB->IntraModes;
                pDecMB->RefPicIdx           = pEncMB->RefPicIdx;
                pDecMB->bTrans8x8           = pEncMB->bTrans8x8;

                // Copy the RunLvlSeq data
                pDecMB->RunLvlSeq.resize(pEncMB->RunLvlSeq.size());
                for (int ti = 0; ti < pEncMB->RunLvlSeq.size(); ti++)
                {
                    pDecMB->RunLvlSeq[ti] = pEncMB->RunLvlSeq[ti];
                }
            }
        }
    }

    // Perform prediction and transform coding for each MB within the slice.
    for (int i = 0; i < numSlices; i++)
    {
        Slice * pSliceY             = &decodedSlice[COLOR_PLANE_YUV_LUMA][i];
        Slice * pSliceCb            = &decodedSlice[COLOR_PLANE_YUV_CHROMA_B][i];
        Slice * pSliceCr            = &decodedSlice[COLOR_PLANE_YUV_CHROMA_R][i];

        invTransQuantSlice(pSliceY);
        invTransQuantSlice(pSliceCb);
        invTransQuantSlice(pSliceCr);

        // If our residuals are good then the next step is to add it to the prediction
        // and write to the dpb.
        predSliceLumaDec(pSliceY);
        predSliceChromaDec(pSliceCb);
        predSliceChromaDec(pSliceCr);
    } 

    // Our DPBFrame should be complete!
    // clear the bitstream
    bitstream.clear();
    for (int ci = 0; ci < COLOR_PLANE_YUV_NUM_PLANES; ci++)
        for (int i = 0; i < pDPBFrame->height(ci); i++)
            for (int j = 0; j < pDPBFrame->width(ci); j++)
                bitstream.push_back(pDPBFrame->getPel(i, j, ci));

    m_encFrameInfoBuf.back().NumBytes   = bitstream.size();
    m_encFrameInfoBuf.back().BitstreamBytePos = (int)bitstream.size();
    // Check our decode errors!
#define _DEBUG_CHECK_ERRORS
#ifdef _DEBUG_CHECK_ERRORS

    
        int maxQT = INT_MIN;
        int minQT = INT_MAX;

        
        double frameDiff = 0;
    for (int c = 0; c < COLOR_PLANE_YUV_NUM_PLANES; c++)
    {          

        double zz0Diff = 0;
        double qtcoeffDiff = 0;
        double residDiff = 0;
        for (int i = 0; i < decodedSlice[c].size(); i++)
        {
            for (int mi = 0; mi < decodedSlice[c][i].NumMB; mi++)
            {
                Macroblock * pMB        = &decodedSlice[c][i].MB[mi];
                const Macroblock * pEncMB   = &(*m_sliceVec[c].back())[i].MB[mi];
                for (int ti = 0; ti < pMB->QTCoeffZZSO.size(); ti++)
                {
                    int sz      = (int)pMB->QTCoeffZZSO[ti].size();
                    zz0Diff     += sad<int>(pMB->QTCoeffZZSO[ti].data(), pEncMB->QTCoeffZZSO[ti].data(), sz);
                    qtcoeffDiff     += sad<int>(pMB->QTCoeff[ti].data(), pEncMB->QTCoeff[ti].data(), sz);
                    for (int i = 0; i < sz; i++)
                    {
                        if (maxQT < pMB->QTCoeff[ti][i])
                        {
                            maxQT = pMB->QTCoeff[ti][i];
                        }
                        if (minQT > pMB->QTCoeff[ti][i])
                        {
                            minQT = pMB->QTCoeff[ti][i];
                        }
                    }
                }

                for (int si = 0; si < pMB->MBResid.size(); si++)
                {
                    int sz      = (int)pMB->MBResid[si].Pel.size();
                    for (int i = 0; i < sz; i++)
                        residDiff     += sad<int>(pMB->MBResid[si].Pel[i].data(),
                                                  pEncMB->MBResid[si].Pel[i].data(), 
                                                  (int)pEncMB->MBResid[si].Pel[i].size());
                }
            }
        }

        for (int i = 0; i < pDPBFrame->height(c); i++)
        {
            for (int j = 0; j < pDPBFrame->width(c); j++)
            {
                int l = (*m_sliceVec[c].back())[0].pFrame->getPel(i,j,c);
                int r = pDPBFrame->getPel(i,j,c);
                        frameDiff     += fabs((double)((*m_sliceVec[c].back())[0].pFrame->getPel(i,j,c) - pDPBFrame->getPel(i,j,c)));
            }
        }
    }
#endif

    delete pYUVFrame;
    delete pDPBFrame;

    pYUVFrame = NULL;
    pDPBFrame = NULL;
}