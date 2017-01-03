///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        predCoding.cpp
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include "predCoding.h"
#include "encoder.hpp"
#include "intraPred.h"
#include "interPred.h"
#include "slice.h"

void setPredMode ( Slice * pSLice );

INTER_PRED_SEARCH_PATTERN encToInterPredSerachPattern ( ENC_ME_SEARCH_PATTERN searchPattern );

void setPredMode ( Slice * pSlice )
{
    _ASSERT(NULL != pSlice);
    // Check the encoder state...
    Encoder * pEncoder                      = pSlice->pEncoder;
    _ASSERT(NULL != pEncoder);
    EncoderDesc encDesc                     = pEncoder->getDesc();
    const EncoderState * pEncoderState      = pEncoder->getEncoderState();

    // Is this the first frame? Has an IDR state been signaled?
    // Are 
    if (0 == pEncoderState->EncFrameCnt)
    {
        // This will be an I-frame. 
        // No need to check the encoder settings
        if (( ENCFRAME_TYPE_I == encDesc.EncFramePattern[0] ) || 
                ( ENCFRAME_TYPE_IDR == encDesc.EncFramePattern[0] )
            )
        {
            pSlice->SliceType                   = SLICE_TYPE_I;
        }
        else if (ENCFRAME_TYPE_IPCM == encDesc.EncFramePattern[0])
        {
            pSlice->SliceType                   = SLICE_TYPE_IPCM;
            pSlice->QP                          = 0;
        }
        return;
    }

    // Check encoder state and see if the user specified
    // an encoding strategy.
    if (0 < encDesc.EncFramePattern.size())
    {
        _ASSERT(( ENCFRAME_TYPE_I == encDesc.EncFramePattern[0] ) || 
                ( ENCFRAME_TYPE_IDR == encDesc.EncFramePattern[0] ) ||
                ( ENCFRAME_TYPE_IPCM == encDesc.EncFramePattern[0] )
               );

        // If the first frame is IPCM, then ALL frames are IPCM.
        if (ENCFRAME_TYPE_IPCM == encDesc.EncFramePattern[0])
        {
            pSlice->SliceType           = SLICE_TYPE_IPCM;
            pSlice->QP                  = 0;
            return;
        }

        int idx                 = pEncoderState->EncFrameCnt % encDesc.EncFramePattern.size();
        ENCFRAME_TYPE encType   = encDesc.EncFramePattern[idx];
        
        switch (encType)
        {
        case ENCFRAME_TYPE_I:
            pSlice->SliceType           = SLICE_TYPE_I;
            break;
        case ENCFRAME_TYPE_IDR:
            pSlice->SliceType           = SLICE_TYPE_I;
            break;
        case ENCFRAME_TYPE_IPCM:
            pSlice->SliceType           = SLICE_TYPE_IPCM;
            pSlice->QP                  = 0;
            break;
        case ENCFRAME_TYPE_P:
            pSlice->SliceType           = SLICE_TYPE_P;
            break;
        case ENCFRAME_TYPE_B:
            pSlice->SliceType           = SLICE_TYPE_B;
            break;
        default:
            break;
        }
        return;
    }
    

    // Otherwise we come up with some encoding strategy on our own...

}

INTER_PRED_SEARCH_PATTERN encToInterPredSerachPattern ( ENC_ME_SEARCH_PATTERN searchPattern )
{
    INTER_PRED_SEARCH_PATTERN interSearchPattern;
       
    switch (searchPattern)
    {
    case ENC_ME_SEARCH_PATTERN_FULL:
        interSearchPattern      = INTER_PRED_SEARCH_PATTERN_FULL;
        break;
    case ENC_ME_SEARCH_PATTERN_NNS:
        interSearchPattern      = INTER_PRED_SEARCH_PATTERN_NNS;
        break;
    case ENC_ME_SEARCH_PATTERN_NULL:
        interSearchPattern      = INTER_PRED_SEARCH_PATTERN_NULL;
        break;
    default:
        interSearchPattern      = INTER_PRED_SEARCH_PATTERN_NULL;
        break;
    }

    return interSearchPattern;
}


void predSliceEnc ( Slice * pSliceY , Slice * pSliceCb , Slice * pSliceCr )
{
    _ASSERT(NULL != pSliceY);
    _ASSERT(NULL != pSliceY->pEncoder);

    _ASSERT(NULL != pSliceCb);
    _ASSERT(NULL != pSliceCb->pEncoder);

    _ASSERT(NULL != pSliceCr);
    _ASSERT(NULL != pSliceCr->pEncoder);

    setPredMode(pSliceY);
    pSliceCb->SliceType                     = pSliceY->SliceType;
    pSliceCr->SliceType                     = pSliceY->SliceType;

    SLICE_TYPE sliceType                    = pSliceY->SliceType;
    Encoder * pEncoder                      = pSliceY->pEncoder;
    INTER_PRED_SEARCH_PATTERN searchPattern;
    switch (sliceType)
    {
    case SLICE_TYPE_I:
        intraPredLumaSliceEnc(pSliceY);
        intraPredChromaSliceEnc(pSliceCb, pSliceCr);
        break;
    case SLICE_TYPE_IPCM:
        intraPCMLumaSliceEnc(pSliceY);
        intraPCMChromaSliceEnc(pSliceCb, pSliceCr);
        break;
    case SLICE_TYPE_P:
        searchPattern       = encToInterPredSerachPattern(pEncoder->getDesc().MESearchPattern);
        interSliceEnc(pSliceY, searchPattern);
        interSliceEnc(pSliceCb, searchPattern);
        interSliceEnc(pSliceCr, searchPattern);
        break;
    case SLICE_TYPE_B:
        break;
    case SLICE_TYPE_SP:
        break;
    case SLICE_TYPE_SI:
        intraPredLumaSliceEnc(pSliceY);
        break;
    default:
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Decode

void predSliceLumaDec ( Slice * pSlice )
{
    _ASSERT(NULL != pSlice);
    
    SLICE_TYPE sliceType                    = pSlice->SliceType;

    switch (sliceType)
    {
    case SLICE_TYPE_I:
        intraPredLumaSliceDec(pSlice);
        break;
    case SLICE_TYPE_IPCM:
        intraPCMLumaSliceDec(pSlice);
        break;
    case SLICE_TYPE_P:
        interSliceDec(pSlice);
        break;
    case SLICE_TYPE_B:
        break;
    case SLICE_TYPE_SP:
        break;
    case SLICE_TYPE_SI:
        intraPredLumaSliceDec(pSlice);
        break;
    default:
        break;
    }
}

void predSliceChromaDec ( Slice * pSlice )
{
    _ASSERT(NULL != pSlice);
    
    SLICE_TYPE sliceType                    = pSlice->SliceType;

    switch (sliceType)
    {
    case SLICE_TYPE_I:
        intraPredChromaSliceDec(pSlice);
        break;
    case SLICE_TYPE_IPCM:
        intraPCMChromaSliceDec(pSlice);
        break;
    case SLICE_TYPE_P:
        interSliceDec(pSlice);
        break;
    case SLICE_TYPE_B:
        break;
    case SLICE_TYPE_SP:
        break;
    case SLICE_TYPE_SI:
        intraPredLumaSliceDec(pSlice);
        break;
    default:
        break;
    }
}