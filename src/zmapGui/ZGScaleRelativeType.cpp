#include "ZGScaleRelativeType.h"

namespace ZMap
{

namespace GUI
{

std::ostream& operator<<(std::ostream& os, ZGScaleRelativeType type)
{
    switch (type)
    {
    case ZGScaleRelativeType::Invalid:
    {
        os << "Invalid" ;
        break ;
    }
    case ZGScaleRelativeType::Positive:
    {
        os << "Positive" ;
        break ;
    }
    case ZGScaleRelativeType::Negative:
    {
        os << "Negative" ;
        break ;
    }
    default:
        break ;
    }

    return os ;
}

} // namespace GUI

} // namespace ZMap

