#include "ZGTFTActionType.h"

namespace ZMap
{

namespace GUI
{

std::ostream& operator<<(std::ostream& str, ZGTFTActionType type)
{
    switch (type)
    {
    case ZGTFTActionType::Invalid:
    {
        str << "Invalid" ;
        break ;
    }
    case ZGTFTActionType::None:
    {
        str << "None" ;
        break ;
    }
    case ZGTFTActionType::Features:
    {
        str << "Features" ;
        break ;
    }
    case ZGTFTActionType::TFT:
    {
        str << "TFT" ;
        break ;
    }
    case ZGTFTActionType::FeaturesTFT:
    {
        str << "FeaturesTFT" ;
        break ;
    }
    default:
        break ;
    }

    return str ;
}

} // namespace GUI

} // namespace ZMap
