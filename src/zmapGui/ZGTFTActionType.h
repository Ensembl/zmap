#ifndef ZGTFTACTIONTYPE_H
#define ZGTFTACTIONTYPE_H

#include <cstdint>
#include <ostream>

//
// Describes the actions possible with the three frame translation
// context menu.
//

namespace ZMap
{

namespace GUI
{

enum class ZGTFTActionType: uint8_t
{
    Invalid,
    None,
    Features,
    TFT,
    FeaturesTFT
};

std::ostream& operator<<(std::ostream& str, ZGTFTActionType type) ;

} // namespace GUI

} // namespace ZMap

#endif // ZGTFTACTIONTYPE_H
