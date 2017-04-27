#ifndef ZGPREFERENCESTYPE_H
#define ZGPREFERENCESTYPE_H

#include <ostream>
#include <cstdint>

namespace ZMap
{

namespace GUI
{

enum class ZGPreferencesType: std::uint8_t
{
    Invalid,

    Display,
    Blixem
} ;

class ZGDisplayPreferencesCompare
{
public:
    bool operator()(ZGPreferencesType p1, ZGPreferencesType p2) const
    {
        return p1 < p2 ;
    }
};

std::ostream & operator<<(std::ostream& os, ZGPreferencesType preferences) ;

} // namespace GUI

} // namespace ZMap

#endif // ZGPREFERENCESTYPE_H
