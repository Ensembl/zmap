#include "ZGScalePositionType.h"

namespace ZMap
{

namespace GUI
{


std::ostream& operator<<(std::ostream& os, ZGScalePositionType type)
{
    switch (type)
    {
    case ZGScalePositionType::Invalid:
    {
        os << "Invalid" ;
        break ;
    }
    case ZGScalePositionType::Top:
    {
        os << "Top" ;
        break ;
    }
    case ZGScalePositionType::Bottom:
    {
        os << "Bottom" ;
        break ;
    }
    case ZGScalePositionType::Left:
    {
        os << "Left" ;
        break ;
    }
    case ZGScalePositionType::Right:
    {
        os << "Right" ;
        break ;
    }
    default:
        break ;
    }

    return os ;
}

} // namespace ZMap

} // namespace GUI
