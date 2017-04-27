#include "ZGStrandType.h"


namespace ZMap
{

namespace GUI
{

std::ostream & operator<<(std::ostream &os, ZGStrandType strand)
{
    switch (strand)
    {
    case ZGStrandType::Invalid:
        {
           os << "Invalid" ;
           break ;
        }
    case ZGStrandType::Dot:
        {
            os << "." ;
            break ;
        }
    case ZGStrandType::Plus:
        {
            os << "+" ;
            break ;
        }
    case ZGStrandType::Minus:
        {
            os << "-" ;
            break ;
        }
    case ZGStrandType::Star:
        {
            os << "*" ;
            break ;
        }
    default:
        break ;
    }
    return os ;
}

} // namespace GUI

} // namespace ZMap

