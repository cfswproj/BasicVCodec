///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        entropy.h
// Abstract:    
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////



#pragma once

#include <cstdint>
#include <vector>

using namespace std;
struct MacroBlock;
struct Slice;

int         entropyEncMB            ( MacroBlock * pMB , vector<uint8_t> &bitstream );

int         entropyEncSlice         ( Slice * pSlice , vector<uint8_t> &bitstream );

void        entropyDecMB            ( MacroBlock * pMB , vector<uint8_t> &bitstream , int &decPos );

void        entropyDecSlice         ( Slice * pSlice , vector<uint8_t> &bitstream , int &decPos );