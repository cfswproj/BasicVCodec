///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        macroblock.h
// Abstract:    Function definitions for encoding I-Frames
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <vector>

#include "common.h"

#define MB_INTERP_FACTOR_LUMA               4
#define MB_INTERP_FACTOR_CHROMA             8

using namespace std;


enum MB_PRED_MODE           // MB Type
{
    MB_PRED_MODE_I4x4,
    MB_PRED_MODE_I8x8,
    MB_PRED_MODE_I16x16,
    MB_PRED_MODE_IPCM,
    MB_PRED_MODE_P4x4,
    MB_PRED_MODE_P8x8,
    MB_PRED_MODE_P16x16,
    MB_PRED_MODE_PSKIP,
    MB_PRED_MODE_B4x4,
    MB_PRED_MODE_B8x8,
    MB_PRED_MODE_B16x16,
    MB_PRED_MODE_BSKIP,
    MB_PRED_MODE_NUM_MODES,
};

enum MB_PARTITION
{
    MB_PARTITION_4x4,
    MB_PARTITION_4x8,
    MB_PARTITION_8x4,
    MB_PARTITION_8x8,
    MB_PARTITION_16x8,
    MB_PARTITION_8x16,
    MB_PARTITION_16x16,
    MB_PARTITION_NUM_PARTITIONS,
};

enum MB_BLOCK_SIZE
{
    MB_BLOCK_SIZE_4x4,
    MB_BLOCK_SIZE_8x8,
    MB_BLOCK_SIZE_16x16,
    MB_BLOCK_SIZE_NUM_SIZES,
};

struct MotionVector
{
    int             X;
    int             Y;
};

enum INTRA_PRED_MODE_LUMA;

struct EncMBInfo
{
    int                     SliceID;
    int                     SliceMBID;
    COLOR_PLANE_YUV         ColorPlane;
    MB_PRED_MODE            PredMode;           // MB Type
    INTRA_PRED_MODE_LUMA    IntraMode;
    MB_PARTITION            Partition;

    int                     XPos;
    int                     YPos;

    int                     MBGlbAddr;         // Global mb addr = YPos * FrameWidthInMB + XPos
    int                     Width;
    int                     Height;

    MB_BLOCK_SIZE           MBBlockSize;
    // For 
    int                     PartitionIdx;       // {0, 1, 2, 3}

};

#define MACROBLOCK_SIZE_4x4_WIDTH           4
#define MACROBLOCK_SIZE_4x4_HEIGHT          4
#define MACROBLOCK_SIZE_4x4_NUM_PEL         ( MACROBLOCK_SIZE_4x4_WIDTH * MACROBLOCK_SIZE_4x4_HEIGHT )

#define MACROBLOCK_SIZE_8x8_WIDTH           8
#define MACROBLOCK_SIZE_8x8_HEIGHT          8
#define MACROBLOCK_SIZE_8x8_NUM_PEL         ( MACROBLOCK_SIZE_8x8_WIDTH * MACROBLOCK_SIZE_8x8_HEIGHT )

#define MACROBLOCK_SIZE_16x16_WIDTH         16
#define MACROBLOCK_SIZE_16x16_HEIGHT        16
#define MACROBLOCK_SIZE_16x16_NUM_PEL       ( MACROBLOCK_SIZE_16x16_WIDTH * MACROBLOCK_SIZE_16x16_HEIGHT )  



struct Slice;

struct RunLevel
{
    uint8_t                 Run;
    int16_t                 Level;
    uint8_t                 bLast;
};

struct MacroBlockHeader
{
    MB_PRED_MODE            PredMode;                   // Tells us the # of subblocks
    vector<int>             IntraModes;
    vector<MotionVector>    MV;
    // For simplicity we will force all
    // MB in a slice to have the same QP.
};

template <class T>
struct MBSubBlk
{
    vector<vector<T>> Pel;
};

struct Macroblock
{
    Slice *                         pSlice;
    int                             SliceID;
    int                             SliceMBID;                  // The MB idx within its slice.

    COLOR_PLANE_YUV                 ColorPlane;

    int                             XPos;
    int                             YPos;

    int                             MBWidth;
    int                             MBHeight;
    MB_BLOCK_SIZE                   MBBlockSize;

    // Prediction Data Members
    vector<bool>                    bIsAvailableLeft;             // Filled by getNeighbors
    vector<bool>                    bIsAvailableUp;               // Filled by getNeighbors
                
    MB_PRED_MODE                    PredMode;
    int                             SubBlkWidth;
    int                             SubBlkHeight;
    int                             NumSubBlks;
    vector<int>                     IntraModes;
    int                             NumMBPart;                      // Number of MB partitions (1 for all Intra, {1:4} for Inter)
    vector<vector<uint8_t>>         MBIntraPred;                    // #subBlks x subBlkSz Pel Values
    int                             RefPicIdx;

    MB_PARTITION                    PartitionType;
    vector<vector<MotionVector>>    MV;

    vector<MBSubBlk<int>>           MBResid;                        //
    vector<vector<int>>             TCoeff;               // #transform blks x transSz Organized in raster scan order
    vector<vector<int>>             QTCoeff;              // #transform blks x transSz Organized in raster scan order
    vector<vector<int>>             QTCoeffZZSO;          // #transform blks x transSz
    bool                            bTrans8x8;

    vector<vector<RunLevel>>        RunLvlSeq;                     // #transform blks x N
    // Quantization
    int                             QP;
    int                             PrevQP;
   
    uint8_t                         CBP;
    // Decoded picture buffer data
    vector<MBSubBlk<uint8_t>>       MBDPB;                          // Instead of writing directly into a frame,
                                                            // let's store the DPB data in it's respective MB.
    int                             ObjQualScore;
};


void        setMBSliceInfo              ( Slice * pSlice , Macroblock * pMB );

uint8_t *   getMBDPBPelPtr              ( Macroblock * pMB , int i , int j );
uint8_t     &getMBDPBPelRef             ( Macroblock * pMB , int i , int j );
uint8_t     getMBDPBPel                 ( const Macroblock * pMB , int i , int j );

uint8_t     getMBRefPicPel              ( const Macroblock * pMB , int i , int j );
uint8_t     getMBInterpRefPicPel        ( const Macroblock * pMB , int i , int j );

uint8_t     getMBInterpPicPel           ( const Macroblock * pMB , int i , int j );


void        copyRefPicMB                ( const Macroblock * pMB , vector<vector<uint8_t>> &mbPel );
void        copyDPBMB                   ( const Macroblock * pMB , vector<vector<uint8_t>> &mbPel );

void        copyRefPicMB                ( const Macroblock * pMB , vector<vector<int>> &mbPel );
void        copyDPBMB                   ( const Macroblock * pMB , vector<vector<int>> &mbPel );

// Sub-block operations
void        copyRefPicMBSubBlk          ( const Macroblock * pMB , int si , vector<vector<uint8_t>> &mbPel );
void        copyDPBMBSubBlk             ( const Macroblock * pMB , int si , vector<vector<uint8_t>> &mbPel );

void        copyRefPicMBSubBlk          ( const Macroblock * pMB , int si , vector<uint8_t> &mbPel );
void        copyDPBMBSubBlk             ( const Macroblock * pMB , int si , vector<uint8_t> &mbPel );

void        copyRefPicMBSubBlk          ( const Macroblock * pMB , int si , vector<vector<int>> &mbPel );
void        copyDPBMBSubBlk             ( const Macroblock * pMB , int si , vector<vector<int>> &mbPel );

void        copyRefPicMBSubBlk          ( const Macroblock * pMB , int si , vector<int> &mbPel );
void        copyDPBMBSubBlk             ( const Macroblock * pMB , int si , vector<int> &mbPel );

// 1-D operations
void        copyRefPicMBSubBlk          ( const Macroblock * pMB , int si , vector<int> &mbPel );
void        copyDPBMBSubBlk             ( const Macroblock * pMB , int si , vector<int> &mbPel );

// Sub-block operations
uint8_t *   getMBDPBSubBlkPelPtr        ( Macroblock * pMB , int si , int i , int j );
uint8_t     &getMBDPBSubBlkPelRef       ( Macroblock * pMB , int si , int i , int j );
uint8_t     getMBDPBSubBlkPel           ( const Macroblock * pMB , int si , int i , int j );
uint8_t     getMBRefPicSubBlkPel        ( const Macroblock * pMB , int si , int i , int j );


uint8_t     getMBList0SubBlkPel         ( const Macroblock * pMB , int li , int si , int i , int j );

// Copy to Frame
void        copyMBSubBlkToDPB           ( Macroblock * pMB , int si , const vector<vector<uint8_t>> &mbPel );
void        copyMBSubBlkToDPB           ( Macroblock * pMB , int si , const vector<vector<int>> &mbPel );
// 1-D operations
void        copyMBSubBlkToDPB           ( Macroblock * pMB , int si , const vector<uint8_t> &mbPel );
void        copyMBSubBlkToDPB           ( Macroblock * pMB , int si , const vector<int> &mbPel );