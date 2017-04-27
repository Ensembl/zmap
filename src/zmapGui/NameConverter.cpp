#include "NameConverter.h"
#include <cstddef>
#include <mutex>
#include <sstream>

namespace ZMap
{

namespace Util
{

std::once_flag name_converter_flag ;
NameConverter* NameConverter::m_name_converter_instance (nullptr) ;

NameConverter::NameConverter()
{

}

NameConverter& NameConverter::NameConverterInstance()
{
    std::call_once(name_converter_flag, create) ;
    return *m_name_converter_instance ;
}

void NameConverter::create()
{
    m_name_converter_instance = new NameConverter ;
}

std::string NameConverter::id2string(GUI::ZInternalID id)
{
    std::string result ;

    // this is temporary --- in case that's not obvious
    std::stringstream str ;
    str << "NameConvertedFrom_" << id ;
    result = str.str() ;

    return result ;
}

GUI::ZInternalID NameConverter::string2id(const std::string& str)
{
    GUI::ZInternalID result = 0 ;

    // this is also temporary --- just to give a return value
    // that's non-zero
    for (auto it = str.begin() ; it != str.end() ; ++it)
        result += static_cast<uint32_t>(*it) ;

    return result ;
}


} // namespace GUI

} // namespace ZMap

