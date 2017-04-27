#ifndef ZGSCALEPOSITIONTYPE_H
#define ZGSCALEPOSITIONTYPE_H

//
// OK, so this refers to the absolute position
// in the viewport widget.
//
//
#include <ostream>
#include <cstdint>

namespace ZMap
{

namespace GUI
{

enum class ZGScalePositionType : std::uint8_t
{
    Invalid,
    Top,
    Bottom,
    Left,
    Right
} ;

std::ostream& operator<<(std::ostream & os, ZGScalePositionType type) ;

} // namespace GUI

} // namespace ZMap

#endif // ZGSCALEPOSITIONTYPE_H
