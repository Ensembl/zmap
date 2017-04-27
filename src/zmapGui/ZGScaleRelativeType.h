#ifndef ZGSCALERELATIVETYPE_H
#define ZGSCALERELATIVETYPE_H

#include <ostream>
#include <cstdint>

namespace ZMap
{

namespace GUI
{


enum class ZGScaleRelativeType : std::uint8_t
{
    Invalid,
    Positive,
    Negative
} ;

std::ostream& operator<<(std::ostream& os, ZGScaleRelativeType type) ;

} // namespace GUI

} // namespace ZMap


#endif // ZGSCALERELATIVETYPE_H
