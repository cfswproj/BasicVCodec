///////////////////////////////////////////////////////////////////////////////////////////////
//
// File:        qualityMetrics.h
// Abstract:    F
//
// Author:      Corey Fernando              (2016)
//
///////////////////////////////////////////////////////////////////////////////////////////////


#pragma once



template<class T>
T       clip            ( const T x , const T lo , const T hi )
{
    T val               = x < lo ? lo : x;
    val                 = val > hi ? hi : val;

    return val;
}

template<class T>
double       sad        ( const T * pA , const T * pB , int size )
{
    _ASSERT(( NULL != pA ) || ( NULL != pB ));

    double sadVal            = 0;

    for (int i = 0; i < size; i++)
        sadVal          += fabs((double)(pA[i] - pB[i]));

    return sadVal;
}

template<class T>
double       ssd        ( const T * pA , const T * pB , int size )
{
    _ASSERT(( NULL != pA ) || ( NULL != pB ));

    double ssdVal            = 0;

    for (int i = 0; i < size; i++)
        ssdVal          += ( pA[i] - pB[i] ) * ( pA[i] - pB[i] );

    return ssdVal;
}