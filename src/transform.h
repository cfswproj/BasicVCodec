///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        transform.h
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <vector>

using namespace std;
#define TRANSFORM4x4_WIDTH              4
#define TRANSFORM4x4_HEIGHT             4
#define NUM_TRANSFORM4x4_COEFFS         ( TRANSFORM4x4_WIDTH * TRANSFORM4x4_HEIGHT )

#define TRANSFORM8x8_WIDTH              8
#define TRANSFORM8x8_HEIGHT             8
#define NUM_TRANSFORM8x8_COEFFS         ( TRANSFORM8x8_WIDTH * TRANSFORM8x8_HEIGHT )

struct MacroBlock;
struct Slice;
enum COLOR_PLANE_YUV;

void    intTrans4x4             ( const int * pSrc , int * pDst );
void    invIntTrans4x4          ( const int * pSrc , int * pDst );

void    quantize4x4             ( const int * pSrc , const int qp , int * pDst );
void    invQuantize4x4          ( const int * pSrc , const int qp , int * pDst );

void    transQuant4x4           ( const int * pSrc , const int qp , int * pDst );
void    invTransQuant4x4        ( const int * pSrc , const int qp , int * pDst );


void    intTrans8x8             ( const int * pSrc , int * pDst );
void    invIntTrans8x8          ( const int * pSrc , int * pDst );

void    quantize8x8             ( const int * pSrc , const int qp , int * pDst );
void    invQuantize8x8          ( const int * pSrc , const int qp , int * pDst );

void    transQuant8x8           ( const int * pSrc , const int qp , int * pDst );
void    invTransQuant8x8        ( const int * pSrc , const int qp , int * pDst );


void    transQuantMB            ( MacroBlock * pMB );
void    invTransQuantMB         ( MacroBlock * pMB );

void    transQuantSlice         ( Slice * pSlice );
void    invTransQuantSlice      ( Slice * pSlice );

// To be used for computing DPB residual data for reconstruction.
void    transQuantMBResid       ( const vector<vector<int>> &resid , vector<vector<int>> &qtcoef , bool bTrans8x8 , int qp );
void    invTransQuantMBResid    ( const vector<vector<int>> &qtcoef , vector<vector<int>> &reconResid , bool bTrans8x8 , int qp );
