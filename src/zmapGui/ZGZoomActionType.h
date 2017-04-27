#ifndef ZGZOOMACTIONTYPE_H
#define ZGZOOMACTIONTYPE_H

#include <cstdint>
#include <ostream>

namespace ZMap
{

namespace GUI
{

enum class ZGZoomActionType: uint8_t
{
    Invalid,
    Max,
    Option01,
    Option02,
    Option03,
    AllDNA,
    Min
} ;

std::ostream& operator<<(std::ostream &, ZGZoomActionType type) ;


} // namespace GUI

} // namespace ZMap

#endif // ZGZOOMACTIONTYPE_H
