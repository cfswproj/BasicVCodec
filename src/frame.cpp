///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        frame.h
// Abstract:    Function definitions for encoding I-Frames
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////


#include <fstream>
#include "frame.hpp"

Frame::Frame ( )
{
    memset(&m_frameDesc_t, 0, sizeof(FrameDesc));
}

Frame::Frame ( Frame &frame )
{
    memset(&m_frameDesc_t, 0, sizeof(FrameDesc));

    copy(frame);
}

Frame::Frame ( const uint8_t * pData , FrameDesc desc )
{
    setFrame(pData, desc);
}

Frame::~Frame ( )
{

}

void Frame::setFrame (const uint8_t * pData , FrameDesc desc )
{
    m_frameDesc_t   = desc;

    int numChnls    = 0;
    vector<int> numPelInChnl;
    vector<int> widthInChnl;
    vector<int> heightInChnl;
    switch (desc.FrameFmt)
    {
    case IMAGE_COLOR_FORMAT_RGB:
        for (int i = 0; i < COLOR_PLANE_RGB_NUM_PLANES; i++)
        {
            numPelInChnl.push_back(desc.FrameWidth * desc.FrameHeight);
            widthInChnl.push_back(desc.FrameWidth);
            heightInChnl.push_back(desc.FrameHeight);
        }
        numChnls    = 3;
        break;
    case IMAGE_COLOR_FORMAT_BGR:
        for (int i = 0; i < COLOR_PLANE_BGR_NUM_PLANES; i++)
        {
            numPelInChnl.push_back(desc.FrameWidth * desc.FrameHeight);
            widthInChnl.push_back(desc.FrameWidth);
            heightInChnl.push_back(desc.FrameHeight);
        }
        numChnls    = 3;
        break;
    case IMAGE_COLOR_FORMAT_YUV420P:
        numPelInChnl.push_back(desc.FrameWidth * desc.FrameHeight);
        widthInChnl.push_back(desc.FrameWidth);
        heightInChnl.push_back(desc.FrameHeight);

        widthInChnl.push_back(desc.FrameWidth / 2);
        heightInChnl.push_back(desc.FrameHeight / 2);
        numPelInChnl.push_back(( desc.FrameWidth * desc.FrameHeight ) / 4);
        
        widthInChnl.push_back(desc.FrameWidth / 2);
        heightInChnl.push_back(desc.FrameHeight / 2);
        numPelInChnl.push_back(( desc.FrameWidth * desc.FrameHeight ) / 4);
        numChnls    = 3;
        break;
    default:
        break;
    }

    m_data.resize(numChnls);

    for (int ci = 0; ci < numChnls; ci++)
    {
        m_data[ci].resize(heightInChnl[ci]);
        for (int i = 0; i < heightInChnl[ci]; i++)
        {
            m_data[ci][i].resize(widthInChnl[ci]);
        }
    }

    if (NULL != pData)
    {
        if (IMAGE_COLOR_FORMAT_RGB == desc.FrameFmt || IMAGE_COLOR_FORMAT_BGR == desc.FrameFmt)
        {
            for (int i = 0; i < desc.FrameHeight; i++)
            {
                for (int j = 0; j < desc.FrameHeight; j++)
                {
                    for (int ci = 0; ci < numChnls; ci++)
                    {
                        int pos             = ( i * numChnls * desc.FrameWidth ) + ( j * numChnls ) + ci;
                        m_data[ci][i][j]    = pData[pos];
                    }
                }
            }
        }
        else if (IMAGE_COLOR_FORMAT_YUV420P == desc.FrameFmt)
        {
            int pos                         = 0;
            for (int ci = 0; ci < numChnls; ci++)
            {
                for (int i = 0; i < heightInChnl[ci]; i++)
                {
                    memcpy(m_data[ci][i].data(), &pData[pos], widthInChnl[ci]);
                    pos                     += widthInChnl[ci];
                }
            }
        }
    }
}

uint8_t Frame::getPel ( int i , int j , int chnl ) const
{
    _ASSERT(chnl < m_data.size());
    _ASSERT(i < m_data[chnl].size());
    _ASSERT(j < m_data[chnl][i].size());

    return m_data[chnl][i][j];
}

uint8_t &Frame::getPelRef ( int i , int j , int chnl )
{
    return *getPelPtr(i, j, chnl);
}

uint8_t * Frame::getPelPtr ( int i , int j , int chnl )
{
    _ASSERT(chnl < m_data.size());
    _ASSERT(i < m_data[chnl].size());
    _ASSERT(j < m_data[chnl][i].size());

    return &m_data[chnl][i][j];
}

int Frame::height ( int chnl ) const
{
    if (0 != m_data.size())
        return m_data[chnl].size();

    return 0;
}

int Frame::width ( int chnl ) const
{
    if (0 != m_data.size())
        if (0 != m_data[chnl].size())
            return m_data[chnl][0].size();

    return 0;
}

FrameDesc Frame::getFrameDesc ( ) const
{
    return m_frameDesc_t;
}

void Frame::copy (Frame &frame )
{
    m_data.clear();

    setFrame(NULL, frame.m_frameDesc_t);

    for (int ci = 0; ci < frame.m_data.size(); ci++)
        for (int i = 0; i < frame.m_data[ci].size(); i++)
            memcpy(m_data[ci][i].data(), frame.m_data[ci][i].data(), frame.m_data[ci][i].size());
}

void Frame::fprintf ( string fname )
{

    ofstream test(fname);
    if (test.is_open())
    {
        if (IMAGE_COLOR_FORMAT_RGB == m_frameDesc_t.FrameFmt || IMAGE_COLOR_FORMAT_BGR == m_frameDesc_t.FrameFmt)
        {
            for (int i = 0; i < m_frameDesc_t.FrameHeight; i++)
            {
                for (int j = 0; j < m_frameDesc_t.FrameHeight; j++)
                {
                    for (int ci = 0; ci < m_data.size(); ci++)
                    {
                        m_data[ci][i][j];
                    }
                }
            }
        }
        else if (IMAGE_COLOR_FORMAT_YUV420P == m_frameDesc_t.FrameFmt)
        {
            int pos                         = 0;
            for (int ci = 0; ci < m_data.size(); ci++)
            {
                for (int i = 0; i < m_data[ci].size(); i++)
                {

                }
            }
        }
        test.close();
    }

}