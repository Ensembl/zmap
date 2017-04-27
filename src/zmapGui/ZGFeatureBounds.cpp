#include "ZGFeatureBounds.h"

namespace ZMap
{

namespace GUI
{

std::ostream & operator<<(std::ostream &os, const ZGFeatureBounds &bounds)
{
    os << "(" << bounds.start << "," << bounds.end << ")" ;

    return os ;
}

} // namespace GUI

} // namespace ZMap

