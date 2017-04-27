#include "ZGuiWidgetComboSource.h"
#include <stdexcept>
#include <sstream>

Q_DECLARE_METATYPE(ZMap::GUI::ZGSourceType)

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetComboSource>::m_name("ZGuiWidgetComboSource") ;
const ZGuiWidgetComboSourceRegister ZGuiWidgetComboSource::m_source_register
{
    ZGSourceType::FileURL,
    ZGSourceType::Ensembl
} ;
const int ZGuiWidgetComboSource::m_source_id(qMetaTypeId<ZGSourceType>()) ;

ZGuiWidgetComboSource::ZGuiWidgetComboSource(QWidget *parent)
    : QComboBox(parent),
      Util::InstanceCounter<ZGuiWidgetComboSource>(),
      Util::ClassName<ZGuiWidgetComboSource>(),
      m_source_map()
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetComboSource::ZGuiWidgetComboSource() ; can not create instance without QApplication existing ")) ;

    QVariant var ;
    ZGSourceType type ;
    std::stringstream str ;
    int index = 0 ;
    for (auto it = m_source_register.begin() ; it != m_source_register.end() ; ++it, str.str(""))
    {
        type = *it ;
        var.setValue(type) ;
        str << type ;
        insertItem(index, QString::fromStdString(str.str()), var) ;
        m_source_map.insert(std::pair<ZGSourceType,int>(type, index)) ;
    }
}

ZGuiWidgetComboSource::~ZGuiWidgetComboSource()
{
}

ZGuiWidgetComboSource* ZGuiWidgetComboSource::newInstance(QWidget* parent)
{
    ZGuiWidgetComboSource *thing = Q_NULLPTR ;

    try
    {
        thing = new ZGuiWidgetComboSource(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

bool ZGuiWidgetComboSource::setSource(ZGSourceType type)
{
    bool result = false ;
    auto it = m_source_map.find(type) ;
    if (it != m_source_map.end())
    {
        setCurrentIndex(it->second) ;
        result = true ;
    }
    return result ;
}

ZGSourceType ZGuiWidgetComboSource::getSourceType() const
{
    ZGSourceType type = ZGSourceType::Invalid ;
    QVariant var = currentData() ;
    if (var.canConvert<ZGSourceType>() && var.convert(m_source_id))
        type = var.value<ZGSourceType>() ;
    return type ;
}

} // namespace GUI

} // namespace ZMap
