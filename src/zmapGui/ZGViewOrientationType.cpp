#include "ZGViewOrientationType.h"


namespace ZMap
{

namespace GUI
{

std::ostream& operator<<(std::ostream& os, ZGViewOrientationType type)
{
    switch (type)
    {
    case ZGViewOrientationType::Invalid:
    {
        os << "Invalid" ;
        break ;
    }
    case ZGViewOrientationType::Right:
    {
        os << "Right" ;
        break ;
    }
    case ZGViewOrientationType::Left:
    {
        os << "Left" ;
        break ;
    }
    case ZGViewOrientationType::Up:
    {
        os << "Up" ;
        break ;
    }
    case ZGViewOrientationType::Down:
    {
        os << "Down" ;
        break ;
    }
    default:
        break ;
    }

    return os ;
}

} // namespace GUI

} // namespace ZMap

