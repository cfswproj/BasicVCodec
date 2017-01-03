///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        syntax.h
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "common.h"

using namespace std;

struct Slice;
struct Macroblock;

uint8_t mbCBP ( const Macroblock * pMBY, const Macroblock * pMBCb , const Macroblock * pMBCr );

void sliceHeaderEnc ( const Slice * pSlice , vector<uint8_t> &bitstream );

void mbHeaderEnc ( const Macroblock * pMBY, const Macroblock * pMBCb , const Macroblock * pMBCr , vector<uint8_t> &bitstream );
