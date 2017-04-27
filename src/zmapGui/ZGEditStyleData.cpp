#include "ZGEditStyleData.h"
#include <stdexcept>


namespace ZMap
{

namespace GUI
{

template <>
const std::string Util::ClassName<ZGEditStyleData>::m_name("ZGEditStyleData") ;

ZGEditStyleData::ZGEditStyleData()
    : Util::InstanceCounter<ZGEditStyleData>(),
      Util::ClassName<ZGEditStyleData>(),
      m_style_data(),
      m_style_child()
{
}

ZGEditStyleData::ZGEditStyleData(const ZGStyleData &data, const ZGStyleData& child)
    : Util::InstanceCounter<ZGEditStyleData>(),
      Util::ClassName<ZGEditStyleData>(),
      m_style_data(data),
      m_style_child(child)
{
}

ZGEditStyleData::ZGEditStyleData(const ZGEditStyleData& data)
    : Util::InstanceCounter<ZGEditStyleData>(),
      Util::ClassName<ZGEditStyleData>(),
      m_style_data(data.m_style_data),
      m_style_child(data.m_style_child)
{
}

ZGEditStyleData& ZGEditStyleData::operator=(const ZGEditStyleData& data)
{
    if (this != &data)
    {
        m_style_data = data.m_style_data ;
        m_style_child = data.m_style_child ;
    }
    return *this ;
}

ZGEditStyleData::~ZGEditStyleData()
{
}

void ZGEditStyleData::setStyleData(const ZGStyleData& data)
{
    m_style_data = data ;
}

void ZGEditStyleData::setStyleDataChild(const ZGStyleData& data)
{
    m_style_child = data ;
}

} // namespace GUI

} // namespace ZMap
