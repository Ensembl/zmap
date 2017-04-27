#ifndef ZGSOURCETYPE_H
#define ZGSOURCETYPE_H

#include <ostream>
#include <cstdint>

namespace ZMap
{

namespace GUI
{

enum class ZGSourceType: std::uint8_t
{
    Invalid,

    FileURL,
    Ensembl
};

class ZGSourceTypeCompare
{
public:
    bool operator()(ZGSourceType t1, ZGSourceType t2) const
    {
        return t1 < t2 ;
    }
};

std::ostream & operator<<(std::ostream & stream, ZGSourceType type) ;

} // namespace GUI

} // namespace ZMap

#endif // ZGSOURCETYPE_H
