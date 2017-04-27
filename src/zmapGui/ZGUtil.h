#ifndef ZGUTIL_H
#define ZGUTIL_H


template<typename T>
T randType(const T & range)
{
    return static_cast<T>((static_cast<double>(rand())/static_cast<double>(RAND_MAX)) * range) ;
}


#endif
