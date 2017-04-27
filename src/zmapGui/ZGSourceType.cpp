#include "ZGSourceType.h"


namespace ZMap
{

namespace GUI
{

std::ostream & operator<<(std::ostream & stream, ZGSourceType type)
{
    switch (type)
    {
    case ZGSourceType::Invalid:
        {
            stream << "Invalid" ;
            break ;
        }
    case ZGSourceType::FileURL:
        {
            stream << "FileURL" ;
            break ;
        }
    case ZGSourceType::Ensembl:
        {
            stream << "Ensembl" ;
            break ;
        }
    default:
        break ;
    }

    return stream ;
}


} // namespace GUI

} // namespace ZMap

