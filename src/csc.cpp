///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        csc.pp
// Abstract:    Function definitions for performing various color space conversions.
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////


#include <stdlib.h>
#include <assert.h>

#include "csc.h"
#include "frame.hpp"

#define NUM_CHANNELS_XXX            3
#define NUM_PLANES_YUV              3
using namespace std;

template<class T>
inline T clampVal ( T val , T minVal , T maxVal )
{
    T retVal        = val < minVal ? minVal : val;

    retVal          = retVal > maxVal ? maxVal : retVal;

    return retVal;
}

template<class T>
static void cscXXX2YUV420P ( const T * pSrc , T * pDst , int width , int height , const float cscWeights[NUM_PLANES_YUV][NUM_CHANNELS_XXX] )
{
    const uint8_t planeOffset[NUM_PLANES_YUV]                   = {16, 128, 128};

    const int numChnl               = NUM_CHANNELS_XXX;

    int widthUV                     = width / 2;
    int heightUV                    = height / 2;

    T * pY                          = pDst;
    T * pU                          = pDst + ( width * height );
    T * pV                          = pU + ( widthUV * heightUV );

    int rgbPitch                    = width * numChnl;

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int ypos                = i * width + j;
            int srcPos              = i * rgbPitch + j * numChnl;

            T tmpY                  = 0;
            for (int c = 0; c < numChnl; c++)
            {
                tmpY                += (T)( pSrc[srcPos+c] * cscWeights[0][c] );
            }

            pY[ypos]                = clampVal<T>((tmpY + planeOffset[0]), 0, 255);

            if (0 == ( j %  2) && 0 == ( i % 2 ))
            {
                int uvpos           = (i / 2) * widthUV + (j / 2);
                T tmpU              = 0;
                T tmpV              = 0;
                for (int c = 0; c < numChnl; c++)
                {
                    tmpU            += (T)( pSrc[srcPos+c] * cscWeights[1][c] );
                    tmpV            += (T)( pSrc[srcPos+c] * cscWeights[2][c] );
                }

                pU[uvpos]           = clampVal<T>((tmpU + planeOffset[1]), 0, 255);
                pV[uvpos]           = clampVal<T>((tmpV + planeOffset[2]), 0, 255);
            }
        }
    }
}

template<class T>
static void cscYUV420P2XXX ( const T * pSrc , T * pDst , int width , int height , const float cscWeights[NUM_CHANNELS_XXX][NUM_PLANES_YUV] )
{
    const uint8_t planeOffset[NUM_PLANES_YUV]                   = {16, 128, 128};

    const int numChnl               = NUM_CHANNELS_XXX;


    int widthUV                     = width / 2;
    int heightUV                    = height / 2;

    const T * pY                    = pSrc;
    const T * pU                    = pSrc + ( width * height );
    const T * pV                    = pU + ( widthUV * heightUV );

    int rgbPitch                    = width * numChnl;
    int uVal                        = 0;
    int vVal                        = 0;

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {           
            int ypos                = i * width + j;
            int dstPos              = i * rgbPitch + j * numChnl;

            int uvpos           = (i / 2) * widthUV + (j / 2);
            uVal                = pU[uvpos];
            vVal                = pV[uvpos];
            
            for (int c = 0; c < numChnl; c++)
            {
                 T tmp               = (T)( ( cscWeights[c][0] * ( pY[ypos] - planeOffset[0] ) ) + 
                                            ( cscWeights[c][1] * ( uVal - planeOffset[1] ) ) + 
                                            ( cscWeights[c][2] * ( vVal - planeOffset[2] ) )
                                          );

                 pDst[dstPos+c]      = clampVal<T>(tmp, 0, 255);
            }
        }
    }
}

void colorSpaceCvt ( const void * pSrc , void * pDst , int width , int height , CSC_TYPE type )
{
    assert(NULL != pSrc);
    assert(NULL != pDst);

    assert(0 != width || 0 != height);


    if (CSC_TYPE_BGR_TO_YUV420P_UINT8 == type)
    {
        const float cscWeights[NUM_PLANES_YUV][NUM_CHANNELS_XXX]    = { {0.0697f, 0.7175f, 0.2128f },
                                                                        {0.4204f, -0.3243f, -0.0962f},
                                                                        {-0.0543f, -0.5591f, 0.6134f}                                         
                                                                      };
        cscXXX2YUV420P<uint8_t>((uint8_t *)pSrc, (uint8_t *)pDst, width, height, cscWeights);

        return;
    }

    if (CSC_TYPE_RGB_TO_YUV420P_UINT8 == type)
    {
        const float cscWeights[NUM_PLANES_YUV][NUM_CHANNELS_XXX]    = { {0.2128f, 0.7175f, 0.0697f},
                                                                        {-0.0962f, -0.3243f, 0.4204f},
                                                                        {0.6134f, -0.5591f, -0.0543f}
                                                                      };

        cscXXX2YUV420P<uint8_t>((uint8_t *)pSrc, (uint8_t *)pDst, width, height, cscWeights);

        return;
    }

    if (CSC_TYPE_YUV420P_TO_BGR_UINT8 == type)
    {
        const float cscWeights[NUM_CHANNELS_XXX][NUM_PLANES_YUV]    = { {1.0f, 2.212798f, 0.0f}, 
                                                                        {1.0f, -0.21482f, -0.38059f},
                                                                        {1.0f, 0.0f, 1.2833f}
                                                                      };
        cscYUV420P2XXX<uint8_t>((uint8_t *)pSrc, (uint8_t *)pDst, width, height, cscWeights);

        return;
    }

    if (CSC_TYPE_YUV420P_TO_RGB_UINT8 == type) 
    {
        const float cscWeights[NUM_CHANNELS_XXX][NUM_PLANES_YUV]    = { {1.0f, 0.0f, 1.2833f}, 
                                                                        {1.0f, -0.21482f, -0.38059f},
                                                                        {1.0f, 2.212798f, 0.0f}
                                                                      };
        cscYUV420P2XXX<uint8_t>((uint8_t *)pSrc, (uint8_t *)pDst, width, height, cscWeights);
        return;
    }
}


static void cscXXX2YUV420P ( const Frame * pSrc , Frame * pDst , const float cscWeights[NUM_PLANES_YUV][NUM_CHANNELS_XXX] )
{    
    const uint8_t planeOffset[NUM_PLANES_YUV]                   = {16, 128, 128};

    const int numChnl               = NUM_CHANNELS_XXX;

    int width                       = pSrc->getFrameDesc().FrameWidth;
    int height                      = pSrc->getFrameDesc().FrameHeight;

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int tmpY                  = 0;
            for (int c = 0; c < numChnl; c++)
            {
                tmpY                += (int)( pSrc->getPel(i,j,c) * cscWeights[COLOR_PLANE_YUV_LUMA][c] );
            }

            pDst->getPelRef(i,j,COLOR_PLANE_YUV_LUMA)   = (uint8_t)clampVal<int>((tmpY + planeOffset[COLOR_PLANE_YUV_LUMA]), 0, 255);

            if (0 == ( j %  2) && 0 == ( i % 2 ))
            {
                int uvi             = i / 2;
                int uvj             = j / 2;
                int tmpU            = 0;
                int tmpV            = 0;
                for (int c = 0; c < numChnl; c++)
                {
                    tmpU            += (int)( pSrc->getPel(i,j,c) * cscWeights[1][c] );
                    tmpV            += (int)( pSrc->getPel(i,j,c) * cscWeights[2][c] );
                }                       

                pDst->getPelRef(uvi,uvj,COLOR_PLANE_YUV_CHROMA_B)   = (uint8_t)clampVal<int>((tmpU + planeOffset[COLOR_PLANE_YUV_CHROMA_B]), 0, 255);
                pDst->getPelRef(uvi,uvj,COLOR_PLANE_YUV_CHROMA_R)   = (uint8_t)clampVal<int>((tmpV + planeOffset[COLOR_PLANE_YUV_CHROMA_R]), 0, 255);
            }
        }
    }

}

static void cscYUV420P2XXX ( const Frame * pSrc , Frame * pDst , const float cscWeights[NUM_CHANNELS_XXX][NUM_PLANES_YUV] )
{
    const uint8_t planeOffset[NUM_PLANES_YUV]                   = {16, 128, 128};

    const int numChnl               = NUM_CHANNELS_XXX;

    int width                       = pSrc->getFrameDesc().FrameWidth;
    int height                      = pSrc->getFrameDesc().FrameHeight;

    int yVal                        = 0;
    int uVal                        = 0;
    int vVal                        = 0;

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {           
            int uvi             = i / 2;
            int uvj             = j / 2;
            yVal                = pSrc->getPel(i, j, COLOR_PLANE_YUV_LUMA);
            uVal                = pSrc->getPel(uvi, uvj, COLOR_PLANE_YUV_CHROMA_B);
            vVal                = pSrc->getPel(uvi, uvj, COLOR_PLANE_YUV_CHROMA_R);
            
            for (int c = 0; c < numChnl; c++)
            {
                 int tmp                    = (int)( ( cscWeights[c][0] * ( yVal - planeOffset[0] ) ) + 
                                                     ( cscWeights[c][1] * ( uVal - planeOffset[1] ) ) + 
                                                     ( cscWeights[c][2] * ( vVal - planeOffset[2] ) )
                                                   );

                pDst->getPelRef(i, j, c)      = (uint8_t)clampVal<int>(tmp, 0, 255);
            }
        }
    }
}

void colorSpaceCvt ( const Frame * pSrc , Frame * pDst , CSC_TYPE type )
{
    assert(NULL != pSrc);
    assert(NULL != pDst);


    if (CSC_TYPE_BGR_TO_YUV420P_UINT8 == type)
    {
        const float cscWeights[NUM_PLANES_YUV][NUM_CHANNELS_XXX]    = { {0.0697f, 0.7175f, 0.2128f },
                                                                        {0.4204f, -0.3243f, -0.0962f},
                                                                        {-0.0543f, -0.5591f, 0.6134f}                                         
                                                                      };
        cscXXX2YUV420P(pSrc,pDst, cscWeights);
        return;
    }

    if (CSC_TYPE_RGB_TO_YUV420P_UINT8 == type)
    {
        const float cscWeights[NUM_PLANES_YUV][NUM_CHANNELS_XXX]    = { {0.2128f, 0.7175f, 0.0697f},
                                                                        {-0.0962f, -0.3243f, 0.4204f},
                                                                        {0.6134f, -0.5591f, -0.0543f}
                                                                      };

        cscXXX2YUV420P(pSrc, pDst, cscWeights);

        return;
    }

    if (CSC_TYPE_YUV420P_TO_BGR_UINT8 == type)
    {
        const float cscWeights[NUM_CHANNELS_XXX][NUM_PLANES_YUV]    = { {1.0f, 2.212798f, 0.0f}, 
                                                                        {1.0f, -0.21482f, -0.38059f},
                                                                        {1.0f, 0.0f, 1.2833f}
                                                                      };
        cscYUV420P2XXX(pSrc, pDst, cscWeights);

        return;
    }

    if (CSC_TYPE_YUV420P_TO_RGB_UINT8 == type) 
    {
        const float cscWeights[NUM_CHANNELS_XXX][NUM_PLANES_YUV]    = { {1.0f, 0.0f, 1.2833f}, 
                                                                        {1.0f, -0.21482f, -0.38059f},
                                                                        {1.0f, 2.212798f, 0.0f}
                                                                      };
        cscYUV420P2XXX(pSrc, pDst, cscWeights);
        return;
    }
}