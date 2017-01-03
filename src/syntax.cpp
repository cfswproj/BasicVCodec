///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        syntax.cpp
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include "syntax.h"
#include "slice.h"
#include "macroblock.h"

#define SLICE_TYPE_P_BS_VAL                 0x00
#define SLICE_TYPE_B_BS_VAL                 0x01
#define SLICE_TYPE_I_BS_VAL                 0x02
#define SLICE_TYPE_SP_BS_VAL                0x03
#define SLICE_TYPE_SI_BS_VAL                0x04

// ITU-T H.264 Table 7-11
// IF (SLICE_TYPE == I)
// Intra mode signaling
#define MB_TYPE_I_4x4                       0x00 // transform_size_8x8_flag = 0;
#define MB_TYPE_I_8x8                       0x00 // transform_size_8x8_flag = 1;
// 16x16Intra Mode | CBP Chroma | CBP Luma
#define MB_TYPE_I_16x16_0_0_0               0x01
#define MB_TYPE_I_16x16_1_0_0               0x02
#define MB_TYPE_I_16x16_2_0_0               0x03
#define MB_TYPE_I_16x16_3_0_0               0x04
#define MB_TYPE_I_16x16_0_1_0               0x05
#define MB_TYPE_I_16x16_1_1_0               0x06
#define MB_TYPE_I_16x16_2_1_0               0x07
#define MB_TYPE_I_16x16_3_1_0               0x08
#define MB_TYPE_I_16x16_0_2_0               0x09
#define MB_TYPE_I_16x16_1_2_0               0x0A
#define MB_TYPE_I_16x16_2_2_0               0x0B
#define MB_TYPE_I_16x16_3_2_0               0x0C
#define MB_TYPE_I_16x16_0_0_1               0x0D
#define MB_TYPE_I_16x16_1_0_1               0x0E
#define MB_TYPE_I_16x16_2_0_1               0x0F
#define MB_TYPE_I_16x16_3_0_1               0x10
#define MB_TYPE_I_16x16_0_1_1               0x11
#define MB_TYPE_I_16x16_1_1_1               0x12
#define MB_TYPE_I_16x16_2_1_1               0x13
#define MB_TYPE_I_16x16_3_1_1               0x14
#define MB_TYPE_I_16x16_0_2_1               0x15
#define MB_TYPE_I_16x16_1_2_1               0x16
#define MB_TYPE_I_16x16_2_2_1               0x17
#define MB_TYPE_I_16x16_3_2_1               0x18
#define MB_TYPE_IPCM                        0x19

uint8_t g_mbTypeIntra16x16LUT[2][3][4]      = { {
                                                    { 
                                                        MB_TYPE_I_16x16_0_0_0,
                                                        MB_TYPE_I_16x16_1_0_0,
                                                        MB_TYPE_I_16x16_2_0_0,
                                                        MB_TYPE_I_16x16_3_0_0
                                                    },
                                                    { 
                                                        MB_TYPE_I_16x16_0_1_0,
                                                        MB_TYPE_I_16x16_1_1_0,
                                                        MB_TYPE_I_16x16_2_1_0,
                                                        MB_TYPE_I_16x16_3_1_0 
                                                    },
                                                    { 
                                                        MB_TYPE_I_16x16_0_2_0,
                                                        MB_TYPE_I_16x16_1_2_0,
                                                        MB_TYPE_I_16x16_2_2_0,
                                                        MB_TYPE_I_16x16_3_2_0
                                                    }
                                                },
                                                { 
                                                    { 
                                                        MB_TYPE_I_16x16_0_0_1,
                                                        MB_TYPE_I_16x16_1_0_1,
                                                        MB_TYPE_I_16x16_2_0_1,
                                                        MB_TYPE_I_16x16_3_0_1
                                                    },
                                                    { 
                                                        MB_TYPE_I_16x16_0_1_1,
                                                        MB_TYPE_I_16x16_1_1_1,
                                                        MB_TYPE_I_16x16_2_1_1,
                                                        MB_TYPE_I_16x16_3_1_1 
                                                    },
                                                    { 
                                                        MB_TYPE_I_16x16_0_2_1,
                                                        MB_TYPE_I_16x16_1_2_1,
                                                        MB_TYPE_I_16x16_2_2_1,
                                                        MB_TYPE_I_16x16_3_2_1
                                                    }
                                                }
                                              };
// ITU-T H.264 Table 7-12
// IF (SLICE_TYPE == SI)
#define MB_SI_4x4                           0x00

// ITU-T H.264 Table 7-13
// IF (SLICE_TYPE == ( P || SP ))
#define MB_TYPE_P_L0_16x16                  0x00
#define MB_TYPE_P_L0_L0_16x8                0x01
#define MB_TYPE_P_L0_L0_8x16                0x02
#define MB_TYPE_P_8x8                       0x03
#define MB_TYPE_P_8x8_REF0                  0x04
// P_SKIP is inferred. The size is always 16x16

// Let's skip B-Slice implementation for now.


// The CBP DOES NOT HOLD TRUE to ITU-REC H.264...
// We do not handle DC transform coefficients.
uint8_t mbCBP ( const Macroblock * pMBY, const Macroblock * pMBCb , const Macroblock * pMBCr )
{
    _ASSERT(NULL != pMBY);
    _ASSERT(NULL != pMBCb);
    _ASSERT(NULL != pMBCr);

    uint8_t cbp             = 0;
    // 6-bit code
    // b_5 b_4 b_3 b_2 b_1 b_0
    // b_3:b_0 are reserved for Luma 8x8 tcoeffs 
    /*
               Luma
         _______ _______
        |       |       |
        |   0   |   1   |       Cb           Cr
        |_______|_______|     _______     _______
        |       |       |    |       |   |       |
        |   2   |   3   |    |   4   |   |   5   |
        |_______|_______|    |_______|   |_______|
    */
    
    const int numSubBlks8x8         = 4;
    
    if (pMBY->bTrans8x8)
    {
        for (int i = 0; i < numSubBlks8x8; i++)
        {
            int sum         = 0;
            for (int ti = 0; ti < pMBY->TCoeff[i].size(); ti++)
            {
                if ( 0 != pMBY->TCoeff[i][ti])
                {
                    cbp     |= 0x01 << i;
                    break;
                }
            }
        }
    }
    else
    {
        const int subBlkIdxLUT[16]     = {0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15};
        for (int i = 0; i < numSubBlks8x8; i++)
        {
            int sum       = 0;
            for (int li = i * 4; li < 16; li++)
            {
                for (int si = subBlkIdxLUT[li]; si < 16; si++)
                {
                    for (int ti = 0; ti < pMBY->TCoeff[si].size(); ti++)
                    {
                        if ( 0 != pMBY->TCoeff[si][ti])
                        {
                            cbp     |= 0x01 << i;
                            si      = 15;
                            li      = 15;
                            break;
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < pMBCb->TCoeff.size(); i++)
    {
        for (int j = 0; j < pMBCb->TCoeff[i].size(); j++)
        {
            if (0 != pMBCb->TCoeff[i][j])
            {
                cbp                 |= 0x10;
                j                   = pMBCb->TCoeff[i].size();
                i                   = pMBCb->TCoeff.size()-1;
            }
        }
    }

    for (int i = 0; i < pMBCr->TCoeff.size(); i++)
    {
        for (int j = 0; j < pMBCr->TCoeff[i].size(); j++)
        {
            if (0 != pMBCr->TCoeff[i][j])
            {
                cbp                 |= 0x20;
                j                   = pMBCr->TCoeff[i].size();
                i                   = pMBCr->TCoeff.size()-1;
            }
        }
    }

    return cbp;
}

void sliceHeaderEnc ( const Slice * pSlice , vector<uint8_t> &bitstream )
{
    _ASSERT(NULL != pSlice);
}

void mbHeaderEnc ( const Macroblock * pMBY, const Macroblock * pMBCb , const Macroblock * pMBCr , vector<uint8_t> &bitstream )
{
    /*
        | Type | Prediction | CBP | 
    */
    _ASSERT(NULL != pMBY);
    _ASSERT(NULL != pMBCb);
    _ASSERT(NULL != pMBCr);

    uint8_t mb_type;
    uint8_t mb_cbp          = mbCBP(pMBY, pMBCb, pMBCr);

    // Are we INTRA or INTER?
    if (MB_PRED_MODE_I4x4 == pMBY->PredMode)
    {
        mb_type             = MB_TYPE_I_4x4;
    }

    else if (MB_PRED_MODE_I8x8 == pMBY->PredMode)
    {
        mb_type             = MB_TYPE_I_8x8;
    }

    else if (MB_PRED_MODE_I16x16 == pMBY->PredMode)
    {
        // DO NOT SEND THE CBP...
        uint8_t chromaFlag  = ( mb_cbp & 0xF0 ) >> 4;
        uint8_t mbIntraMode = pMBY->IntraModes[0];

        if (0x0F == ( 0x0F & mb_cbp ))
            mb_type         = g_mbTypeIntra16x16LUT[1][chromaFlag][mbIntraMode];
        else
            mb_type         = g_mbTypeIntra16x16LUT[0][chromaFlag][mbIntraMode];

    }

    else if (MB_PRED_MODE_IPCM == pMBY->PredMode)
    {
        mb_type             = MB_TYPE_IPCM;
    }

    else if (MB_PRED_MODE_P4x4 == pMBY->PredMode)
    {
    }

    else if (MB_PRED_MODE_P8x8 == pMBY->PredMode)
    {
    }

    else if (MB_PRED_MODE_P16x16 == pMBY->PredMode)
    {
        if (MB_PARTITION_16x16 == pMBY->PartitionType)
        {
            mb_type         = MB_TYPE_P_L0_16x16;
        }
        else if (MB_PARTITION_8x16 == pMBY->PartitionType)
        {
            mb_type         = MB_TYPE_P_L0_L0_8x16;
        }
        else if (MB_PARTITION_16x8 == pMBY->PartitionType)
        {
            mb_type         = MB_TYPE_P_L0_L0_16x8;
        }
        else if (MB_PARTITION_8x8 == pMBY->PartitionType)
        {
            mb_type         = MB_TYPE_P_8x8;
        }
    }

    else if (MB_PRED_MODE_PSKIP == pMBY->PredMode)
    {
    }





}