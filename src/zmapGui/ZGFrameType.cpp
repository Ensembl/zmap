#include "ZGFrameType.h"

namespace ZMap
{

namespace GUI
{

std::ostream & operator<<(std::ostream &os, ZGFrameType frame)
{
    switch (frame)
    {
    case ZGFrameType::Invalid:
        {
            os << "Invalid" ;
            break ;
        }
    case ZGFrameType::Dot:
        {
            os << "." ;
            break ;
        }
    case ZGFrameType::Zero:
        {
            os << "0" ;
            break ;
        }
    case ZGFrameType::One:
        {
            os << "1" ;
            break ;
        }
    case ZGFrameType::Two:
        {
            os << "2" ;
            break ;
        }
    case ZGFrameType::Star:
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
