///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        slice.h
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include "slice.h"
#include "encoder.hpp"

void getSliceHeader ( const Slice * pSlice , SliceHeader & sliceHeader )
{
    sliceHeader.ColorPlane      = pSlice->ColorPlane;
    sliceHeader.SliceType       = pSlice->SliceType;
    sliceHeader.QP              = pSlice->QP;
    sliceHeader.NumMB           = pSlice->NumMB;
}

void setSliceHeader ( Slice * pSlice , const SliceHeader & sliceHeader )
{
    pSlice->ColorPlane      = sliceHeader.ColorPlane;
    pSlice->SliceType       = sliceHeader.SliceType;
    pSlice->QP              = sliceHeader.QP;
    pSlice->NumMB           = sliceHeader.NumMB;
}

void frameSlicePartition ( Encoder * pEncoder ,  Frame * pFrame ,  Frame * pDPB , Slice &slice , COLOR_PLANE_YUV plane , int sliceID )
{
    _ASSERT(NULL != pEncoder);

    slice.pEncoder              = pEncoder;
    slice.QP                    = pEncoder->getDesc().RDDesc.QP;
    slice.pFrame                = pFrame;
    slice.pDPBFrame             = pDPB;
    slice.SliceID               = sliceID;
    slice.ColorPlane            = plane;
    FrameDesc frameDesc         = pFrame->getFrameDesc();
    slice.FrameWidth            = frameDesc.FrameWidth;
    slice.FrameHeight           = frameDesc.FrameHeight;

    slice.List0                 = (*pEncoder->getRefPicListPtr());

    
    // Let's keep it simple and assign one slice per frame.
    // WIthout further constraints like:
    //      - fixed packet sizes
    //      - knowledge about image motion via video analysis...
    // we may not benefit from multiple slices per frame.

    if (IMAGE_COLOR_FORMAT_YUV420P == frameDesc.FrameFmt)
    {
        int numLumaPel                      = ( frameDesc.FrameWidth * frameDesc.FrameHeight );
        int numChromaPel                    = ( numLumaPel / 4 );
        MB_BLOCK_SIZE mbBlockSize           = MB_BLOCK_SIZE_NUM_SIZES;
        int mbWidth                         = 0;
        int mbHeight                        = 0;

        int pelPlaneOffset                  = 0;

        if (COLOR_PLANE_YUV_LUMA == plane)
        {
            slice.NumMB                     = ( numLumaPel / MACROBLOCK_SIZE_16x16_NUM_PEL );
            slice.FrameWidthInMB            = slice.FrameWidth / MACROBLOCK_SIZE_16x16_WIDTH;
            slice.FrameHeightInMB           = slice.FrameHeight / MACROBLOCK_SIZE_16x16_HEIGHT;
            slice.pInterpRefPic             = pEncoder->getInterpRefPic(COLOR_PLANE_YUV_LUMA);
            slice.pInterpPic                = pEncoder->getInterpPic(COLOR_PLANE_YUV_LUMA);

            slice.MB.resize(slice.NumMB);

            mbWidth                         = MACROBLOCK_SIZE_16x16_WIDTH;
            mbHeight                        = MACROBLOCK_SIZE_16x16_HEIGHT; 

            mbBlockSize                     = MB_BLOCK_SIZE_16x16;
        }
        else if (COLOR_PLANE_YUV_CHROMA_B == plane || COLOR_PLANE_YUV_CHROMA_R == plane)
        {
            slice.FrameWidth                /= 2;
            slice.FrameHeight               /= 2;
            slice.NumMB                     = ( numChromaPel / MACROBLOCK_SIZE_8x8_NUM_PEL );
            slice.FrameWidthInMB            = slice.FrameWidth / MACROBLOCK_SIZE_8x8_WIDTH;
            slice.FrameHeightInMB           = slice.FrameHeight / MACROBLOCK_SIZE_8x8_HEIGHT;
            slice.pInterpRefPic             = pEncoder->getInterpRefPic(COLOR_PLANE_YUV_CHROMA_B);
            slice.pInterpPic                = pEncoder->getInterpPic(COLOR_PLANE_YUV_CHROMA_B);

            slice.MB.resize(slice.NumMB);
            
            mbWidth                         = MACROBLOCK_SIZE_8x8_WIDTH;
            mbHeight                        = MACROBLOCK_SIZE_8x8_HEIGHT;

            mbBlockSize                     = MB_BLOCK_SIZE_8x8;

            pelPlaneOffset                  += numLumaPel;

            if (COLOR_PLANE_YUV_CHROMA_R == plane)
            {
                pelPlaneOffset              += numChromaPel;
                slice.pInterpRefPic         = pEncoder->getInterpRefPic(COLOR_PLANE_YUV_CHROMA_R);
                slice.pInterpPic            = pEncoder->getInterpPic(COLOR_PLANE_YUV_CHROMA_R);
            }
        }
        else
            runtime_error("In function frameSlicePartition(): Invalid COLOR_PLANE specified\n");

        int mbID                        = 0;        
        int y                           = 0;
        for (int i = 0; i < slice.FrameHeight; i += mbHeight)
        {
            int x                       = 0;
            for (int j = 0; j < slice.FrameWidth; j += mbWidth)
            {
                Macroblock * pMB        = &slice.MB[mbID];

                setMBSliceInfo(&slice, pMB);
                pMB->SliceMBID          = mbID;
                pMB->XPos               = x;
                pMB->YPos               = y;
                pMB->MBWidth            = mbWidth;
                pMB->MBHeight           = mbHeight;
                pMB->MBBlockSize        = mbBlockSize;

                mbID++;
                x++;
            }
            y++;
        }
    }
}

void frameSlicePartitionN ( Encoder * pEncoder ,  Frame * pFrame ,  Frame * pDPB , vector<Slice> &sliceVec , COLOR_PLANE_YUV plane , int numSlices )
{
    _ASSERT(NULL != pEncoder);

    for (int i = 0; i < numSlices; i++)
        frameSlicePartition(pEncoder, pFrame, pDPB, sliceVec[i], plane, i);
}