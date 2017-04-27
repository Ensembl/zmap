#include "ZGZoomActionType.h"


namespace ZMap
{

namespace GUI
{

std::ostream& operator<<(std::ostream& str, ZGZoomActionType type)
{
    switch (type)
    {
    case ZGZoomActionType::Invalid:
    {
        str << "Invalid" ;
        break ;
    }
    case ZGZoomActionType::Max:
    {
        str << "Max" ;
        break ;
    }
    case ZGZoomActionType::Option01:
    {
        str << "Option01" ;
        break ;
    }
    case ZGZoomActionType::Option02:
    {
        str << "Option02" ;
        break ;
    }
    case ZGZoomActionType::Option03:
    {
        str << "Option03" ;
        break ;
    }
    case ZGZoomActionType::AllDNA:
    {
        str << "AllDNA" ;
        break ;
    }
    case ZGZoomActionType::Min:
    {
        str << "Min" ;
        break ;
    }

    default:
        break ;
    }

    return str ;
}

} // namespace GUI

} // namespace ZMap
