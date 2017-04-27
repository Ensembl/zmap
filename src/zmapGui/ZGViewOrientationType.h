#ifndef ZGVIEWORIENTATIONTYPE_H
#define ZGVIEWORIENTATIONTYPE_H

//
// This describes the orientation in the GraphicsView viewport.
// Meaning is "direction of forward strand", i.e. "Right" means
// that the forward strand is on the top and running to the right.
//

#include <ostream>
#include <cstdint>

namespace ZMap
{

namespace GUI
{

enum class ZGViewOrientationType : std::uint8_t
{
    Invalid,

    Right,
    Left,
    Up,
    Down
} ;

std::ostream & operator<<(std::ostream &os, ZGViewOrientationType) ;

} // namespace GUI

} // namespace ZMap

#endif // ZGVIEWORIENTATIONTYPE_H
