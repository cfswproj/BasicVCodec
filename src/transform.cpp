///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        transform.cpp
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <memory.h>
#include <stdexcept>

#include "transform.h"
#include "macroblock.h"
#include "slice.h"

#define CF4_TRANSFORM_MAT_WIDTH                     TRANSFORM4x4_WIDTH
#define CF4_TRANSFORM_MAT_HEIGHT                    TRANSFORM4x4_HEIGHT
#define CF4_TRANSFORM_MAT_NUM_EL                    ( CF4_TRANSFORM_MAT_WIDTH * CF4_TRANSFORM_MAT_HEIGHT )

#define QPV4_QUANT_MAT_WIDTH                        3
#define QPV4_QUANT_MAT_HEIGHT                       6
#define QPV4_QUANT_MAT_NUM_EL                       ( QPV4_QUANT_MAT_WIDTH * QPV4_QUANT_MAT_HEIGHT )

#define CF8_TRANSFORM_MAT_WIDTH                     TRANSFORM8x8_WIDTH
#define CF8_TRANSFORM_MAT_HEIGHT                    TRANSFORM8x8_HEIGHT
#define CF8_TRANSFORM_MAT_NUM_EL                    ( CF8_TRANSFORM_MAT_WIDTH * CF8_TRANSFORM_MAT_HEIGHT )

#define QPV8_QUANT_MAT_WIDTH                        6
#define QPV8_QUANT_MAT_HEIGHT                       6
#define QPV8_QUANT_MAT_NUM_EL                       ( QPV8_QUANT_MAT_WIDTH * QPV8_QUANT_MAT_HEIGHT )


#define DCT4_TRANSFORM_MAT_WIDTH                    TRANSFORM4x4_WIDTH
#define DCT4_TRANSFORM_MAT_HEIGHT                   TRANSFORM4x4_HEIGHT
#define DCT4_TRANSFORM_MAT_NUM_EL                   ( DCT4_TRANSFORM_MAT_WIDTH * DCT4_TRANSFORM_MAT_HEIGHT )

#define DCT8_TRANSFORM_MAT_WIDTH                    TRANSFORM8x8_WIDTH
#define DCT8_TRANSFORM_MAT_HEIGHT                   TRANSFORM8x8_HEIGHT
#define DCT8_TRANSFORM_MAT_NUM_EL                   ( DCT8_TRANSFORM_MAT_WIDTH * DCT8_TRANSFORM_MAT_HEIGHT )

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The 4x4 Transforms

static const int cf4[CF4_TRANSFORM_MAT_NUM_EL]      = { 1,  1,  1,  1,
                                                        2,  1, -1, -2,
                                                        1, -1, -1,  1,
                                                        1, -2,  2, -1
                                                      };

static const int cf4T[CF4_TRANSFORM_MAT_NUM_EL]     = { 1,  2,  1,  1,
                                                        1,  1, -1, -2,
                                                        1, -1, -1,  2,
                                                        1, -2,  1, -1
                                                      };

static const float ci4[CF4_TRANSFORM_MAT_NUM_EL]    = { 1,     1,     1,     1,
                                                        1,  0.5f, -0.5f,    -1,
                                                        1,    -1,    -1,     1,
                                                        0.5f, -1,     1, -0.5f
                                                      };

static const float ci4T[CF4_TRANSFORM_MAT_NUM_EL]   = { 1,     1,     1,  0.5f,
                                                        1,  0.5f,    -1,    -1,
                                                        1, -0.5f,    -1,     1,
                                                        1,    -1,     1, -0.5f
                                                      };

#define INV_TRANSFORM_4x4_SCALE                     64 // ( 1 << 6 )
#define INV_TRANSFORM_4x4_OFFSET                    32 // ( 1 << 5 )

static const int qpV4[QPV4_QUANT_MAT_NUM_EL]        = { 13107, 5243, 8066,
                                                        11916, 4660, 7490,
                                                        10082, 4194, 7490,
                                                        9362,  3647, 5825,
                                                        8192,  3355, 5243,
                                                        7182,  2893, 4559
                                                      };

#define FWD_QUANTIZER_V4_SCALE                      32768 // ( 1 << 15 )

static const int qpV4Inv[QPV4_QUANT_MAT_NUM_EL]     = { 10, 16, 13,
                                                        11, 18, 14,
                                                        13, 20, 16,
                                                        14, 23, 18,
                                                        16, 25, 20,
                                                        18, 29, 23
                                                      };
#define SCALE4_0IDX_NUM_EL                            4
#define SCALE4_1IDX_NUM_EL                            4
#define SCALE4_2IDX_NUM_EL                            8

static const int scale4_0Idx[SCALE4_0IDX_NUM_EL]    = { 0, 8, 2, 10 };
static const int scale4_1Idx[SCALE4_1IDX_NUM_EL]    = { 5, 13, 7, 15 };
static const int scale4_2Idx[SCALE4_2IDX_NUM_EL]    = { 1, 3, 4, 6, 9, 11, 12, 14 };

static int zz4x4SO[NUM_TRANSFORM4x4_COEFFS]         = { 0,  1,  4,  8,
                                                        5,  2,  3,  6, 
                                                        9, 12, 13, 10,
                                                        7, 11, 14, 15
                                                      };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The 8x8 Transforms

static const int cf8[CF8_TRANSFORM_MAT_NUM_EL]      = {  8,   8,   8,   8,   8,   8,   8,   8,
                                                        12,  10,   6,   3,  -3,  -6, -10, -12,
                                                         8,   4,  -4,  -8,  -8,  -4,   4,   8,
                                                        10,  -3, -12,  -6,   6,  12,   3, -10,
                                                         8,  -8,  -8,   8,   8,  -8,  -8,   8,
                                                         6, -12,   3,  10, -10,  -3,  12,  -6,
                                                         4,  -8,   8,  -4,  -4,   8,  -8,   4,
                                                         3,  -6,  10, -12,  12, -10,   6,  -3
                                                      };

static const int cf8T[CF8_TRANSFORM_MAT_NUM_EL]     = { 8,   12,   8,  10,   8,   6,   4,   3,
                                                        8,   10,   4,  -3,  -8, -12,  -8,  -6,
                                                        8,    6,  -4, -12,  -8,   3,   8,  10,
                                                        8,    3,  -8,  -6,   8,  10,  -4, -12,
                                                        8,   -3,  -8,   6,   8, -10,  -4,  12,
                                                        8,   -6,  -4,  12,  -8,  -3,   8, -10,
                                                        8,  -10,   4,   3,  -8,  12,  -8,   6,
                                                        8,  -12,   8, -10,   8,  -6,   4,  -3
                                                      };

static const float ci8[CF8_TRANSFORM_MAT_NUM_EL]    = {      1,	      1,      1,      1,       1,       1,      1,       1,
                                                          1.5f,	  1.25f,  0.75f, 0.375f, -0.375f,  -0.75f, -1.25f,   -1.5f,
                                                             1,	   0.5f,  -0.5f,     -1,      -1,   -0.5f,   0.5f,       1,
                                                         1.25f,	-0.375f,  -1.5f, -0.75f,   0.75f,    1.5f, 0.375f,  -1.25f,
                                                             1,	     -1,     -1,      1,       1,      -1,     -1,       1,
                                                         0.75f,	  -1.5f, 0.375f,  1.25f,  -1.25f, -0.375f,   1.5f,  -0.75f,
                                                          0.5f,	     -1,      1,  -0.5f,   -0.5f,       1,     -1,    0.5f,
                                                        0.375f,	 -0.75f,  1.25f,  -1.5f,    1.5f,  -1.25f,  0.75f, -0.375f
                                                      };

static const float ci8T[CF8_TRANSFORM_MAT_NUM_EL]   = { 1,    1.5f,     1,   1.25f,  1,	  0.75f,  0.5f,	 0.375f,
                                                        1,   1.25f,  0.5f, -0.375f, -1,	  -1.5f,    -1,	 -0.75f,
                                                        1,   0.75f, -0.5f,   -1.5f, -1,	 0.375f,     1,	  1.25f,
                                                        1,  0.375f,    -1,  -0.75f,  1,	  1.25f, -0.5f,	  -1.5f,
                                                        1, -0.375f,    -1,   0.75f,  1,	 -1.25f, -0.5f,	   1.5f,
                                                        1,  -0.75f, -0.5f,    1.5f, -1,	-0.375f,     1,	 -1.25f,
                                                        1,  -1.25f,  0.5f,  0.375f, -1,	   1.5f,    -1,	  0.75f,
                                                        1,   -1.5f,     1,  -1.25f,  1,	 -0.75f,  0.5f,	-0.375f
                                                      };

#define INV_TRANSFORM_8x8_SCALE                     64 // ( 1 << 6 )
#define INV_TRANSFORM_8x8_OFFSET                    32 // ( 1 << 5 )
static const int qpV8[QPV8_QUANT_MAT_NUM_EL]        = {  7282,  6428, 11288,  6830, 41683,  8640,
                                                         8192,  7346, 12838,  7740, 47935,  9777,
                                                         9362,  8228, 14549,  8931, 54783, 11259,
                                                        10082,  8943, 15589,  9675, 58103, 11985,
                                                        11916, 10826, 18706, 11058, 68478, 14290,
                                                        13107, 11428, 20460, 12222, 76696, 15481
                                                      };

#define FWD_QUANTIZER_V8_SCALE                      32768 // ( 1 << 15 )

static const int qpV8Inv[QPV8_QUANT_MAT_NUM_EL]     = { 20, 18, 32, 19, 25, 24,
                                                        22, 19, 35, 21, 28, 26,
                                                        26, 23, 42, 24, 33, 31,
                                                        28, 25, 45, 26, 35, 33,
                                                        32, 28, 51, 30, 40, 38,
                                                        36, 32, 58, 34, 46, 43
                                                      };

#define SCALE8_0IDX_NUM_EL                            4
#define SCALE8_1IDX_NUM_EL                            16
#define SCALE8_2IDX_NUM_EL                            4
#define SCALE8_3IDX_NUM_EL                            16
#define SCALE8_4IDX_NUM_EL                            8
#define SCALE8_5IDX_NUM_EL                            16

static const int scale8_0Idx[SCALE8_0IDX_NUM_EL]    = { 0, 4, 32, 36 };
static const int scale8_1Idx[SCALE8_1IDX_NUM_EL]    = { 9, 11, 13, 15, 25, 27, 29, 31, 41, 43, 45, 47, 57, 59, 61, 63 };
static const int scale8_2Idx[SCALE8_2IDX_NUM_EL]    = { 18, 22, 50, 54 };
static const int scale8_3Idx[SCALE8_3IDX_NUM_EL]    = { 1, 3, 5, 7, 8, 12, 24, 28, 33, 35, 37, 39, 40, 44, 56, 60 };
static const int scale8_4Idx[SCALE8_4IDX_NUM_EL]    = { 2, 6, 16, 20, 34, 38, 48, 52};
static const int scale8_5Idx[SCALE8_5IDX_NUM_EL]    = { 10, 14, 17, 19, 21, 23, 26, 30, 42, 46, 49, 51, 53, 55, 58, 62 };

static int zz8x8SO[NUM_TRANSFORM8x8_COEFFS]         = {  0,  1,  8, 16,  9,  2,  3, 10,
                                                        17, 24, 32, 25, 18, 11,  4,  5, 
                                                        12, 19, 26, 33, 40, 48, 41, 34,
                                                        27, 20, 13,  6,  7, 14, 21, 28,
                                                        35, 42, 49, 56, 57, 50, 43, 36,
                                                        29, 22, 15, 23, 30, 37, 44, 51,
                                                        58, 59, 52, 45, 38, 31, 39, 46,
                                                        53, 60, 61, 54, 47, 55, 62, 63
                                                      };


int     roundf              ( float x );
// Compute C = A * B
template<class T>
void    matMult             ( const T * pA , int rA , int cA , const T * pB , int rB , int cB , T * pC );

void    runLevelEnc         ( const vector<int> &zzSO ,  vector<RunLevel> &runLvlSeq );

void    runLevelDec         ( const vector<RunLevel> &runLvlSeq , vector<int> &zzSO , int transSz );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int roundf ( float x )
{
    return (int)_copysign(( fabs(x) + 0.5f ), x);
}

template<class T>
void matMult ( const T * pA , int rA , int cA , const T * pB , int rB , int cB , T * pC )
{
    _ASSERT(NULL != pA);
    _ASSERT(NULL != pB);
    _ASSERT(NULL != pC);

    _ASSERT(cA == rB);
    for (int i = 0; i < rA; i++)
    {
        for (int j = 0; j < cB; j++)
        {
            int rowi        = i * rB;
            int pos         = rowi + j;
            T tmpSum        = 0;

            for (int m = 0; m < cA; m++)
                tmpSum      += pA[rowi+m] * pB[m*cB+j];

            pC[pos]         = tmpSum;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The 4x4 Transforms

void quantize4x4 ( const int * pSrc , const int qp , int * pDst )
{
    _ASSERT(NULL != pSrc);
    _ASSERT(NULL != pDst);

    float m[CF4_TRANSFORM_MAT_NUM_EL];
    int mscale[QPV4_QUANT_MAT_WIDTH];

    int qs                  = qp % QPV4_QUANT_MAT_HEIGHT;
    float d                 = (float) ( 1 << ( qp / QPV4_QUANT_MAT_HEIGHT ) );

    if (( 0 <= qs ) && ( QPV4_QUANT_MAT_HEIGHT > qs ))
        memcpy(mscale, &qpV4[qs*QPV4_QUANT_MAT_WIDTH], sizeof(int) * QPV4_QUANT_MAT_WIDTH);
    else
        memcpy(mscale, qpV4, sizeof(int) * QPV4_QUANT_MAT_WIDTH);
                  
    for (int i = 0; i < SCALE4_0IDX_NUM_EL; i++)
        m[scale4_0Idx[i]]     = ( mscale[0] / d ) / FWD_QUANTIZER_V4_SCALE;
    for (int i = 0; i < SCALE4_1IDX_NUM_EL; i++)
        m[scale4_1Idx[i]]     = ( mscale[1] / d ) / FWD_QUANTIZER_V4_SCALE;
    for (int i = 0; i < SCALE4_2IDX_NUM_EL; i++)
        m[scale4_2Idx[i]]     = ( mscale[2] / d ) / FWD_QUANTIZER_V4_SCALE;

    for (int i = 0; i < CF4_TRANSFORM_MAT_NUM_EL; i++)
        pDst[i]             = roundf(pSrc[i] * m[i]);
}

void invQuantize4x4 ( const int * pSrc , const int qp , int * pDst )
{
    _ASSERT(NULL != pSrc);
    _ASSERT(NULL != pDst);

    float m[CF4_TRANSFORM_MAT_NUM_EL];
    int mscale[QPV4_QUANT_MAT_WIDTH];

    int qs                  = qp % QPV4_QUANT_MAT_HEIGHT;
    float d                 = (float) ( 1 << ( qp / QPV4_QUANT_MAT_HEIGHT ) );

    if (( 0 <= qs ) && ( QPV4_QUANT_MAT_HEIGHT > qs ))
        memcpy(mscale, &qpV4Inv[qs*QPV4_QUANT_MAT_WIDTH], sizeof(int) * QPV4_QUANT_MAT_WIDTH);
    else
        memcpy(mscale, qpV4Inv, sizeof(int) * QPV4_QUANT_MAT_WIDTH);
                  
    for (int i = 0; i < SCALE4_0IDX_NUM_EL; i++)
        m[scale4_0Idx[i]]     = mscale[0] * d;
    for (int i = 0; i < SCALE4_1IDX_NUM_EL; i++)
        m[scale4_1Idx[i]]     = mscale[1] * d;
    for (int i = 0; i < SCALE4_2IDX_NUM_EL; i++)
        m[scale4_2Idx[i]]     = mscale[2] * d;

    for (int i = 0; i < CF4_TRANSFORM_MAT_NUM_EL; i++)
        pDst[i]             = roundf(pSrc[i] * m[i]);
}

// Not optimized
void intTrans4x4 ( const int * pSrc , int * pDst )
{
    _ASSERT(NULL != pSrc);
    _ASSERT(NULL != pDst);

    int tmp[CF4_TRANSFORM_MAT_NUM_EL];

    // Compute cf4 * pSrc
    matMult<int>(   cf4, CF4_TRANSFORM_MAT_HEIGHT, CF4_TRANSFORM_MAT_WIDTH,
                    pSrc, CF4_TRANSFORM_MAT_HEIGHT, CF4_TRANSFORM_MAT_WIDTH,
                    tmp
                );

    
    matMult<int>(   tmp, CF4_TRANSFORM_MAT_HEIGHT, CF4_TRANSFORM_MAT_WIDTH,
                    cf4T, CF4_TRANSFORM_MAT_HEIGHT, CF4_TRANSFORM_MAT_WIDTH,
                    pDst
                );
}

void invIntTrans4x4 ( const int * pSrc , int * pDst )
{
    _ASSERT(NULL != pSrc);
    _ASSERT(NULL != pDst);
    
    int e[CF4_TRANSFORM_MAT_NUM_EL];
    int f[CF4_TRANSFORM_MAT_NUM_EL];
    int g[CF4_TRANSFORM_MAT_NUM_EL];
    int h[CF4_TRANSFORM_MAT_NUM_EL];

    // 1D Row Transform
    for (int i  = 0; i < TRANSFORM4x4_HEIGHT; i++)
    {
        int rowi        = i*TRANSFORM4x4_WIDTH;

        e[rowi+0]       = pSrc[rowi+0] + pSrc[rowi+2];
        e[rowi+1]       = pSrc[rowi+0] - pSrc[rowi+2];
        e[rowi+2]       = ( pSrc[rowi+1] >> 1 ) - pSrc[rowi+3];
        e[rowi+3]       = pSrc[rowi+1] + ( pSrc[rowi+3] >> 1 );

        f[rowi+0]       = e[rowi+0] + e[rowi+3];
        f[rowi+1]       = e[rowi+1] + e[rowi+2];
        f[rowi+2]       = e[rowi+1] - e[rowi+2];
        f[rowi+3]       = e[rowi+0] - e[rowi+3];
    }

    // 1D Col Transform
    int row0            = 0;
    int row1            = TRANSFORM4x4_WIDTH;
    int row2            = 2 * TRANSFORM4x4_WIDTH;
    int row3            = 3 * TRANSFORM4x4_WIDTH;
    for (int j = 0; j < TRANSFORM4x4_WIDTH; j++)
    {
        g[row0+j]       = f[row0+j] + f[row2+j];
        g[row1+j]       = f[row0+j] - f[row2+j];
        g[row2+j]       = ( f[row1+j] >> 1 ) - f[row3+j];
        g[row3+j]       = f[row1+j] + ( f[row3+j] >> 1 );

        h[row0+j]       = g[row0+j] + g[row3+j];
        h[row1+j]       = g[row1+j] + g[row2+j];
        h[row2+j]       = g[row1+j] - g[row2+j];
        h[row3+j]       = g[row0+j] - g[row3+j];
    }

    for (int i = 0; i < CF4_TRANSFORM_MAT_NUM_EL; i++)
        pDst[i]         = roundf((float)( h[i] + INV_TRANSFORM_4x4_OFFSET ) / INV_TRANSFORM_4x4_SCALE);
}

// Not optimized
void transQuant4x4 ( const int * pSrc , const int qp , int * pDst )
{
    _ASSERT(NULL != pSrc);
    _ASSERT(NULL != pDst);

    int tmp[CF4_TRANSFORM_MAT_NUM_EL];
    intTrans4x4(pSrc, tmp);
    quantize4x4(tmp, qp, pDst);
}

// Not optimized
void invTransQuant4x4 ( const int * pSrc , const int qp , int * pDst )
{
    _ASSERT(NULL != pSrc);
    _ASSERT(NULL != pDst);

    int tmp[CF4_TRANSFORM_MAT_NUM_EL];
    invQuantize4x4(pSrc, qp, tmp);
    invIntTrans4x4(tmp, pDst);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The 8x8 Transforms

// Not optimized
void quantize8x8 ( const int * pSrc , const int qp , int * pDst )
{
    _ASSERT(NULL != pSrc);
    _ASSERT(NULL != pDst);

    float m[CF8_TRANSFORM_MAT_NUM_EL];
    int mscale[QPV8_QUANT_MAT_WIDTH];

    int qs                  = qp % QPV8_QUANT_MAT_HEIGHT;
    float d                 = (float) ( 1 << ( qp / QPV8_QUANT_MAT_HEIGHT ) );

    if (( 0 <= qs ) && ( QPV8_QUANT_MAT_HEIGHT > qs ))
        memcpy(mscale, &qpV8[qs*QPV8_QUANT_MAT_WIDTH], sizeof(int) * QPV8_QUANT_MAT_WIDTH);
    else
        memcpy(mscale, qpV8, sizeof(int) * QPV8_QUANT_MAT_WIDTH);
                  
    for (int i = 0; i < SCALE8_0IDX_NUM_EL; i++)
        m[scale8_0Idx[i]]     = ( mscale[0] / d ) / FWD_QUANTIZER_V8_SCALE;
    for (int i = 0; i < SCALE8_1IDX_NUM_EL; i++)
        m[scale8_1Idx[i]]     = ( mscale[1] / d ) / FWD_QUANTIZER_V8_SCALE;
    for (int i = 0; i < SCALE8_2IDX_NUM_EL; i++)
        m[scale8_2Idx[i]]     = ( mscale[2] / d ) / FWD_QUANTIZER_V8_SCALE;
    for (int i = 0; i < SCALE8_3IDX_NUM_EL; i++)
        m[scale8_3Idx[i]]     = ( mscale[3] / d ) / FWD_QUANTIZER_V8_SCALE;
    for (int i = 0; i < SCALE8_4IDX_NUM_EL; i++)
        m[scale8_4Idx[i]]     = ( mscale[4] / d ) / FWD_QUANTIZER_V8_SCALE;
    for (int i = 0; i < SCALE8_5IDX_NUM_EL; i++)
        m[scale8_5Idx[i]]     = ( mscale[5] / d ) / FWD_QUANTIZER_V8_SCALE;

    for (int i = 0; i < CF8_TRANSFORM_MAT_NUM_EL; i++)
        pDst[i]             = roundf(pSrc[i] * m[i]);
}

// Not optimized
void invQuantize8x8 ( const int * pSrc , const int qp , int * pDst )
{
    _ASSERT(NULL != pSrc);
    _ASSERT(NULL != pDst);

    float m[CF8_TRANSFORM_MAT_NUM_EL];
    int mscale[QPV8_QUANT_MAT_WIDTH];

    int qs                  = qp % QPV8_QUANT_MAT_HEIGHT;
    float d                 = (float) ( 1 << ( qp / QPV8_QUANT_MAT_HEIGHT ) );

    if (( 0 <= qs ) && ( QPV8_QUANT_MAT_HEIGHT > qs ))
        memcpy(mscale, &qpV8Inv[qs*QPV8_QUANT_MAT_WIDTH], sizeof(int) * QPV8_QUANT_MAT_WIDTH);
    else
        memcpy(mscale, qpV8Inv, sizeof(int) * QPV8_QUANT_MAT_WIDTH);
                  
    for (int i = 0; i < SCALE8_0IDX_NUM_EL; i++)
        m[scale8_0Idx[i]]     = mscale[0] * d;
    for (int i = 0; i < SCALE8_1IDX_NUM_EL; i++)
        m[scale8_1Idx[i]]     = mscale[1] * d;
    for (int i = 0; i < SCALE8_2IDX_NUM_EL; i++)
        m[scale8_2Idx[i]]     = mscale[2] * d;
    for (int i = 0; i < SCALE8_3IDX_NUM_EL; i++)
        m[scale8_3Idx[i]]     = mscale[3] * d;
    for (int i = 0; i < SCALE8_4IDX_NUM_EL; i++)
        m[scale8_4Idx[i]]     = mscale[4] * d;
    for (int i = 0; i < SCALE8_5IDX_NUM_EL; i++)
        m[scale8_5Idx[i]]     = mscale[5] * d;

    for (int i = 0; i < CF8_TRANSFORM_MAT_NUM_EL; i++)
        pDst[i]             = roundf(pSrc[i] * m[i]);
}

// Not optimized
void intTrans8x8 ( const int * pSrc , int * pDst )
{
    
    _ASSERT(NULL != pSrc);
    _ASSERT(NULL != pDst);

    int tmp[CF8_TRANSFORM_MAT_NUM_EL];

    // Compute cf8 * pSrc
    matMult<int>(   cf8, CF8_TRANSFORM_MAT_HEIGHT, CF8_TRANSFORM_MAT_WIDTH,
                    pSrc, CF8_TRANSFORM_MAT_HEIGHT, CF8_TRANSFORM_MAT_WIDTH,
                    tmp
                );

    
    matMult<int>(   tmp, CF8_TRANSFORM_MAT_HEIGHT, CF8_TRANSFORM_MAT_WIDTH,
                    cf8T, CF8_TRANSFORM_MAT_HEIGHT, CF8_TRANSFORM_MAT_WIDTH,
                    pDst
                );
}

// Not optimized
void invIntTrans8x8 ( const int * pSrc , int * pDst )
{
    _ASSERT(NULL != pSrc);
    _ASSERT(NULL != pDst);

    int e[CF8_TRANSFORM_MAT_NUM_EL];
    int f[CF8_TRANSFORM_MAT_NUM_EL];
    int g[CF8_TRANSFORM_MAT_NUM_EL];
    int h[CF8_TRANSFORM_MAT_NUM_EL];
    int k[CF8_TRANSFORM_MAT_NUM_EL];
    int m[CF8_TRANSFORM_MAT_NUM_EL];

    // 1D Row Transform
    for (int i  = 0; i < TRANSFORM8x8_HEIGHT; i++)
    {
        int rowi        = i*TRANSFORM8x8_WIDTH;

        e[rowi+0]       = pSrc[rowi+0] + pSrc[rowi+4];
        e[rowi+1]       = -pSrc[rowi+3] + pSrc[rowi+5] - pSrc[rowi+7] - ( pSrc[rowi+7] >> 1 );
        e[rowi+2]       = pSrc[rowi+0] - pSrc[rowi+4];
        e[rowi+3]       = pSrc[rowi+1] + pSrc[rowi+7] - pSrc[rowi+3] - ( pSrc[rowi+3] >> 1 );
        e[rowi+4]       = ( pSrc[rowi+2] >> 1 ) - pSrc[rowi+6];
        e[rowi+5]       = -pSrc[rowi+1] + pSrc[rowi+7] + pSrc[rowi+5] + ( pSrc[rowi+5] >> 1 );
        e[rowi+6]       = pSrc[rowi+2] + ( pSrc[rowi+6] >> 1 );
        e[rowi+7]       = pSrc[rowi+3] + pSrc[rowi+5] + pSrc[rowi+1] + ( pSrc[rowi+1] >> 1 );

        f[rowi+0]       = e[rowi+0] + e[rowi+6];
        f[rowi+1]       = e[rowi+1] + ( e[rowi+7] >> 2 );
        f[rowi+2]       = e[rowi+2] + e[rowi+4];
        f[rowi+3]       = e[rowi+3] + ( e[rowi+5] >> 2 );
        f[rowi+4]       = e[rowi+2] - e[rowi+4];
        f[rowi+5]       = ( e[rowi+3] >> 2 ) - e[rowi+5];
        f[rowi+6]       = e[rowi+0] - e[rowi+6];
        f[rowi+7]       = e[rowi+7] - ( e[rowi+1] >> 2 );

        g[rowi+0]       = f[rowi+0] + f[rowi+7];
        g[rowi+1]       = f[rowi+2] + f[rowi+5];
        g[rowi+2]       = f[rowi+4] + f[rowi+3];
        g[rowi+3]       = f[rowi+6] + f[rowi+1];
        g[rowi+4]       = f[rowi+6] - f[rowi+1];
        g[rowi+5]       = f[rowi+4] - f[rowi+3];
        g[rowi+6]       = f[rowi+2] - f[rowi+5];
        g[rowi+7]       = f[rowi+0] - f[rowi+7];
    }

    // 1D Col Transform
    int row0            = 0;
    int row1            = TRANSFORM8x8_WIDTH;
    int row2            = 2 * TRANSFORM8x8_WIDTH;
    int row3            = 3 * TRANSFORM8x8_WIDTH;
    int row4            = 4 * TRANSFORM8x8_WIDTH;
    int row5            = 5 * TRANSFORM8x8_WIDTH;
    int row6            = 6 * TRANSFORM8x8_WIDTH;
    int row7            = 7 * TRANSFORM8x8_WIDTH;
    for (int j = 0; j < TRANSFORM8x8_WIDTH; j++)
    {
        h[row0+j]       = g[row0+j] + g[row4+j];
        h[row1+j]       = -g[row3+j] + g[row5+j] - g[row7+j] - ( g[row7+j] >> 1 );
        h[row2+j]       = g[row0+j] - g[row4+j];
        h[row3+j]       = g[row1+j] + g[row7+j] - g[row3+j] - ( g[row3+j] >> 1 );
        h[row4+j]       = ( g[row2+j] >> 1 ) - g[row6+j];
        h[row5+j]       = -g[row1+j] + g[row7+j] + g[row5+j] + ( g[row5+j] >> 1 );
        h[row6+j]       = g[row2+j] + ( g[row6+j] >> 1 );
        h[row7+j]       = g[row3+j] + g[row5+j] + g[row1+j] + ( g[row1+j] >> 1 );

        k[row0+j]       = h[row0+j] + h[row6+j];
        k[row1+j]       = h[row1+j] + ( h[row7+j] >> 2 );
        k[row2+j]       = h[row2+j] + h[row4+j];
        k[row3+j]       = h[row3+j] + ( h[row5+j] >> 2 );
        k[row4+j]       = h[row2+j] - h[row4+j];
        k[row5+j]       = ( h[row3+j] >> 2 ) - h[row5+j];
        k[row6+j]       = h[row0+j] - h[row6+j];
        k[row7+j]       = h[row7+j] - ( h[row1+j] >> 2 );

        m[row0+j]       = k[row0+j] + k[row7+j];
        m[row1+j]       = k[row2+j] + k[row5+j];
        m[row2+j]       = k[row4+j] + k[row3+j];
        m[row3+j]       = k[row6+j] + k[row1+j];
        m[row4+j]       = k[row6+j] - k[row1+j];
        m[row5+j]       = k[row4+j] - k[row3+j];
        m[row6+j]       = k[row2+j] - k[row5+j];
        m[row7+j]       = k[row0+j] - k[row7+j];
    }

    for (int i = 0; i < CF4_TRANSFORM_MAT_NUM_EL; i++)
        pDst[i]         = roundf((float)( m[i] + INV_TRANSFORM_8x8_OFFSET ) / INV_TRANSFORM_8x8_SCALE);
}

// Not optimized
void transQuant8x8 ( const int * pSrc , const int qp , int * pDst )
{
    _ASSERT(NULL != pSrc);
    _ASSERT(NULL != pDst);

    int tmp[CF8_TRANSFORM_MAT_NUM_EL];
    intTrans8x8(pSrc, tmp);
    quantize8x8(tmp, qp, pDst);
}

// Not optimized
void invTransQuant8x8 ( const int * pSrc , const int qp , int * pDst )
{
    _ASSERT(NULL != pSrc);
    _ASSERT(NULL != pDst);

    int tmp[CF8_TRANSFORM_MAT_NUM_EL];
    invQuantize8x8(pSrc, qp, tmp);
    invIntTrans8x8(tmp, pDst);
}

void runLevelEnc ( const vector<int> &zzSO ,  vector<RunLevel> &runLvlSeq )
{
    runLvlSeq.clear();
    // For each qtcoeff
    int qi                  = 0;
    while (qi < zzSO.size())
    {
        // For each qtcoeff
        RunLevel runLvl         = { 0, 0, 0};

        while (( 0 == runLvl.Level ) && ( qi < zzSO.size() ))
        {
            if (0 == zzSO[qi] && ( zzSO.size() - 1 ) != qi)
                runLvl.Run++;
            else
                runLvl.Level    = (int16_t)zzSO[qi];

            qi++;
        }
        if (0 == runLvl.Level)
            if (!runLvlSeq.empty())
                runLvlSeq.back().bLast  = 1;
            else
            {
                runLvl.bLast            = 1;
                runLvlSeq.push_back(runLvl);
            }
        else
            runLvlSeq.push_back(runLvl);
    }
}

void runLevelDec ( const vector<RunLevel> &runLvlSeq , vector<int> &zzSO )
{
    // For each run level
    int qi                      = 0;
    memset(zzSO.data(), 0, sizeof(int) * zzSO.size());
    for (int ri = 0; ri < runLvlSeq.size(); ri++)
    {
        qi                      += runLvlSeq[ri].Run;
        zzSO[qi]                = runLvlSeq[ri].Level;
        if (runLvlSeq[ri].bLast)
            break;
        qi++;
    }
}

void transQuantMB ( Macroblock * pMB )
{
    _ASSERT(NULL != pMB);

    int mbSubBlkStride      = pMB->SubBlkWidth; 
    int qp                  = pMB->QP;
    int numTCoeffs, transSz, numTransBlks;

    void (* transFunc) ( const int * pSrc , int * pDst );
    void (* quantFunc) ( const int * pSrc , const int qp , int * pDst );
    int * pResid            = NULL;
    int * pZZSO             = NULL;

    if (pMB->bTrans8x8)
    {
        numTCoeffs          = NUM_TRANSFORM8x8_COEFFS;
        transSz             = TRANSFORM8x8_WIDTH;
        numTransBlks        = ( pMB->MBWidth * pMB->MBHeight ) / numTCoeffs;
        transFunc           = &intTrans8x8;
        quantFunc           = &quantize8x8;
        pZZSO               = zz8x8SO;
    }
    else
    {
        numTCoeffs          = NUM_TRANSFORM4x4_COEFFS;
        transSz             = TRANSFORM4x4_WIDTH;
        numTransBlks        = ( pMB->MBWidth * pMB->MBHeight ) / numTCoeffs;
        transFunc           = &intTrans4x4;
        quantFunc           = &quantize4x4;
        pZZSO               = zz4x4SO;
    }

    vector<int> resid(numTCoeffs);
    pMB->TCoeff.resize(numTransBlks);
    pMB->QTCoeff.resize(numTransBlks);
    pMB->QTCoeffZZSO.resize(numTransBlks);
    pMB->RunLvlSeq.resize(numTransBlks);

    int ti              = 0;
    for (int si = 0; si < pMB->NumSubBlks; si++)
    {            
        for (int i = 0; i < mbSubBlkStride; i += transSz)
        {
            for (int j = 0; j < mbSubBlkStride; j += transSz)
            {
                // Fill the residual block in the dimensions 
                // of the transform.
                for (int ii = 0; ii < transSz; ii++)
                {
                    for (int jj = 0; jj < transSz; jj++)
                    {
                        int transPos            = ii * transSz + jj;
                        resid[transPos]         = pMB->MBResid[si].Pel[i+ii][j+jj];
                    }
                }

                // Allocate space for the transform and quantized data.
                pMB->TCoeff[ti].resize(numTCoeffs);
                pMB->QTCoeff[ti].resize(numTCoeffs);
                pMB->QTCoeffZZSO[ti].resize(numTCoeffs);

                transFunc(resid.data(), pMB->TCoeff[ti].data());
                quantFunc(pMB->TCoeff[ti].data(), qp, pMB->QTCoeff[ti].data());

                // Get Ziggy
                for (int zi = 0; zi < numTCoeffs; zi++)
                    pMB->QTCoeffZZSO[ti][zi]      = pMB->QTCoeff[ti][pZZSO[zi]];
 
                // Create the run level encoded qtcoeffs
                runLevelEnc(pMB->QTCoeffZZSO[ti], pMB->RunLvlSeq[ti]);
                ti++;
            }
        }
    }
}

void invTransQuantMB ( Macroblock * pMB )
{
    _ASSERT(NULL != pMB);

    int mbSubBlkStride      = pMB->SubBlkWidth;
    int qp                  = pMB->QP;
    int numTCoeffs, transSz, numTransBlks;

    void (* invTransFunc) ( const int * pSrc , int * pDst );
    void (* invQuantFunc) ( const int * pSrc , const int qp , int * pDst );
    int * pZZSO             = NULL;

    if (pMB->bTrans8x8)
    {
        numTCoeffs          = NUM_TRANSFORM8x8_COEFFS;
        transSz             = TRANSFORM8x8_WIDTH;
        numTransBlks        = pMB->MBWidth * pMB->MBHeight / numTCoeffs;
        invTransFunc        = &invIntTrans8x8;
        invQuantFunc        = &invQuantize8x8;
        pZZSO               = zz8x8SO;
    }
    else
    {
        numTCoeffs          = NUM_TRANSFORM4x4_COEFFS;
        transSz             = TRANSFORM4x4_WIDTH;
        numTransBlks        = pMB->MBWidth * pMB->MBHeight / numTCoeffs;
        invTransFunc        = &invIntTrans4x4;
        invQuantFunc        = &invQuantize4x4;
        pZZSO               = zz4x4SO;
    }

    vector<int> resid(numTCoeffs);
    pMB->QTCoeffZZSO.resize(numTransBlks);
    pMB->TCoeff.resize(numTransBlks);
    pMB->QTCoeff.resize(numTransBlks);
    pMB->MBResid.resize(pMB->NumSubBlks);

    // Allocate all tcoeff space now...
    int ti              = 0;
    for (int si = 0; si < pMB->NumSubBlks; si++)
    {
        pMB->MBResid[si].Pel.resize(mbSubBlkStride);
        for (int i = 0; i < pMB->MBResid[si].Pel.size(); i++)
            pMB->MBResid[si].Pel[i].resize(mbSubBlkStride);

        for (int i = 0; i < mbSubBlkStride; i += transSz)
        {
            for (int j = 0; j < mbSubBlkStride; j += transSz)
            {
                pMB->QTCoeffZZSO[ti].resize(numTCoeffs);
                pMB->QTCoeff[ti].resize(numTCoeffs);
                pMB->TCoeff[ti].resize(numTCoeffs);

                // Get the ZZSO qtcoeffs from the run level decoder
                runLevelDec(pMB->RunLvlSeq[ti], pMB->QTCoeffZZSO[ti]);
                // Un Ziggy
                for (int zi = 0; zi < numTCoeffs; zi++)
                    pMB->QTCoeff[ti][pZZSO[zi]]       = pMB->QTCoeffZZSO[ti][zi];

                invQuantFunc(pMB->QTCoeff[ti].data(), qp, pMB->TCoeff[ti].data());
                invTransFunc(pMB->TCoeff[ti].data(), resid.data());
                ti++;

                // Fill the residual block in the dimensions 
                // of the transform.
                for (int ii = 0; ii < transSz; ii++)
                {
                    for (int jj = 0; jj < transSz; jj++)
                    {
                        int transPos                        = ii * transSz + jj;
                        pMB->MBResid[si].Pel[i+ii][j+jj]    = resid[transPos];
                    }
                }
            }
        }
    }
}


void transQuantMBResid ( const vector<vector<int>> &resid , vector<vector<int>> &qtcoef , bool bTrans8x8 , int qp )
{
    _ASSERT(0 != resid.size());

    int mbSubBlkStride      = (int)resid.size();
    int numTCoeffs, transSz, numTransBlks;

    void (* transFunc) ( const int * pSrc , int * pDst );
    void (* quantFunc) ( const int * pSrc , const int qp , int * pDst );
    

    if (bTrans8x8)
    {
        numTCoeffs          = NUM_TRANSFORM8x8_COEFFS;
        transSz             = TRANSFORM8x8_WIDTH;
        numTransBlks        = ( mbSubBlkStride * mbSubBlkStride ) / numTCoeffs;
        transFunc           = &intTrans8x8;
        quantFunc           = &quantize8x8;
    }
    else
    {
        numTCoeffs          = NUM_TRANSFORM4x4_COEFFS;
        transSz             = TRANSFORM4x4_WIDTH;
        numTransBlks        = ( mbSubBlkStride * mbSubBlkStride ) / numTCoeffs;
        transFunc           = &intTrans4x4;
        quantFunc           = &quantize4x4;
    }
    
    // Allocate space for the transform and quantized data.
    qtcoef.resize(mbSubBlkStride);
    for (int i = 0; i < mbSubBlkStride; i++)
        qtcoef[i].resize(mbSubBlkStride);

    vector<int> tmpResid(numTCoeffs);
    vector<int> tcoef(numTCoeffs);
    vector<int> tmpQTCoef(numTCoeffs);

    int ti              = 0;
           
    for (int i = 0; i < mbSubBlkStride; i += transSz)
    {
        for (int j = 0; j < mbSubBlkStride; j += transSz)
        {
            // Fill the residual block in the dimensions 
            // of the transform.
            for (int ii = 0; ii < transSz; ii++)
            {
                for (int jj = 0; jj < transSz; jj++)
                {
                    int transPos            = ii * transSz + jj;
                    tmpResid[transPos]      = resid[i+ii][j+jj];
                }
            }

            transFunc(tmpResid.data(), tcoef.data());
            quantFunc(tcoef.data(), qp, tmpQTCoef.data());

            // Fill the residual block in the dimensions 
            // of the transform.
            for (int ii = 0; ii < transSz; ii++)
            {
                for (int jj = 0; jj < transSz; jj++)
                {
                    int transPos            = ii * transSz + jj;
                    qtcoef[i+ii][j+jj]      = tmpQTCoef[transPos];
                }
            }

            ti++;
        }
    }
}

void invTransQuantMBResid ( const vector<vector<int>> &qtcoef , vector<vector<int>> &reconResid , bool bTrans8x8 , int qp )
{
    _ASSERT(0 != qtcoef.size());

    int mbSubBlkStride      = (int)qtcoef.size();
    int numTCoeffs, transSz, numTransBlks;

    void (* invTransFunc) ( const int * pSrc , int * pDst );
    void (* invQuantFunc) ( const int * pSrc , const int qp , int * pDst );
    

    if (bTrans8x8)
    {
        numTCoeffs          = NUM_TRANSFORM8x8_COEFFS;
        transSz             = TRANSFORM8x8_WIDTH;
        numTransBlks        = ( mbSubBlkStride * mbSubBlkStride ) / numTCoeffs;
        invTransFunc        = &invIntTrans8x8;
        invQuantFunc        = &invQuantize8x8;
    }
    else
    {
        numTCoeffs          = NUM_TRANSFORM4x4_COEFFS;
        transSz             = TRANSFORM4x4_WIDTH;
        numTransBlks        = ( mbSubBlkStride * mbSubBlkStride ) / numTCoeffs;
        invTransFunc        = &invIntTrans4x4;
        invQuantFunc        = &invQuantize4x4;
    }

    
    // Allocate space for the transform and quantized data.
    reconResid.resize(mbSubBlkStride);
    for (int i = 0; i < mbSubBlkStride; i++)
        reconResid[i].resize(mbSubBlkStride);

    vector<int> tmpQTCoef(numTCoeffs);
    vector<int> tcoef(numTCoeffs);
    vector<int> tmpResid(numTCoeffs);
    int ti              = 0;
           
    for (int i = 0; i < mbSubBlkStride; i += transSz)
    {
        for (int j = 0; j < mbSubBlkStride; j += transSz)
        {
            // Fill the residual block in the dimensions 
            // of the transform.
            for (int ii = 0; ii < transSz; ii++)
            {
                for (int jj = 0; jj < transSz; jj++)
                {
                    int transPos            = ii * transSz + jj;
                    tmpQTCoef[transPos]     = qtcoef[i+ii][j+jj];
                }
            }

            invQuantFunc(tmpQTCoef.data(), qp, tcoef.data());
            invTransFunc(tcoef.data(), tmpResid.data());

            // Fill the residual block in the dimensions 
            // of the transform.
            for (int ii = 0; ii < transSz; ii++)
            {
                for (int jj = 0; jj < transSz; jj++)
                {
                    int transPos            = ii * transSz + jj;
                    reconResid[i+ii][j+jj]  = tmpResid[transPos];
                }
            }

            ti++;
        }
    }
}

void transQuantSlice ( Slice * pSlice )
{
    _ASSERT(NULL != pSlice);

    for (int i = 0; i < pSlice->NumMB; i++)
        transQuantMB(&pSlice->MB[i]);
}

void invTransQuantSlice ( Slice * pSlice )
{
    _ASSERT(NULL != pSlice);

    for (int i = 0; i < pSlice->NumMB; i++)
        invTransQuantMB(&pSlice->MB[i]);
}