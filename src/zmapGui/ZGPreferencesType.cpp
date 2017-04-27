#include "ZGPreferencesType.h"

namespace ZMap
{

namespace GUI
{

std::ostream & operator<<(std::ostream &os, ZGPreferencesType preferences)
{
    switch (preferences)
    {

    case ZGPreferencesType::Invalid:
        {
            os << "Invalid" ;
            break ;
        }

    case ZGPreferencesType::Blixem:
    {
        os << "Blixem" ;
        break ;
    }

    case ZGPreferencesType::Display:
    {
        os << "Display" ;
        break ;
    }

    default:
        break ;

    }

    return os ;
}

} // namespace GUI

} // namespace ZMap
