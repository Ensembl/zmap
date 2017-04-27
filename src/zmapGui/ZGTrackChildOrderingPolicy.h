#ifndef ZGTRACKCHILDORDERINGPOLICY_H
#define ZGTRACKCHILDORDERINGPOLICY_H

#include<QtGlobal>
#include "ZInternalID.h"

namespace ZMap
{

namespace GUI
{

class ZGTrackChildOrderingPolicy
{
public:

    ZGTrackChildOrderingPolicy() ;
    virtual ~ZGTrackChildOrderingPolicy() {}

    virtual qreal coord() const = 0 ;

private:

};

} // namespace GUI

} // namespace ZMap

#endif // ZGTRACKCHILDORDERINGPOLICY_H
