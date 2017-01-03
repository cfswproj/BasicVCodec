///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        interPred.h
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

struct Macroblock;
struct Slice;

enum INTER_PRED_SEARCH_PATTERN
{
    INTER_PRED_SEARCH_PATTERN_FULL,
    INTER_PRED_SEARCH_PATTERN_NNS,
    INTER_PRED_SEARCH_PATTERN_NULL,
    INTER_PRED_SEARCH_PATTERN_NUM_PATTERNS,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Encode

void        interMBEnc      ( Macroblock * pMB , INTER_PRED_SEARCH_PATTERN searchPattern );
void        interSliceEnc   ( Slice * pSlice , INTER_PRED_SEARCH_PATTERN searchPattern );
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Decode
    
void interMBDec ( Macroblock * pMB );
void interSliceDec ( Slice * pSlice );