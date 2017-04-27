#include "ZGFeatureType.h"

namespace ZMap
{

namespace GUI
{

std::ostream& operator<<(std::ostream& str, ZGFeatureType type)
{
    switch (type)
    {
    case ZGFeatureType::Invalid:
    {
        str << "Invalid" ;
        break ;
    }
    case ZGFeatureType::Basic:
    {
        str << "Basic" ;
        break ;
    }
    case ZGFeatureType::Transcript:
    {
        str << "Transcript" ;
        break ;
    }
    case ZGFeatureType::Alignment:
    {
        str << "Alignment" ;
        break ;
    }
    case ZGFeatureType::Graph:
    {
        str << "Graph" ;
        break ;
    }
    default:
        break ;
    }

    return str ;
}

} // namespace GUI

} // namespace ZMap
