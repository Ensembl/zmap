#include "ZGuiWidgetComboStrand.h"
#include <stdexcept>
#include <sstream>

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

Q_DECLARE_METATYPE(ZMap::GUI::ZGStrandType)

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetComboStrand>::m_name ("ZGuiWidgetComboStrand") ;
const std::vector<ZGStrandType> ZGuiWidgetComboStrand::m_strand_register
{
    ZGStrandType::Star,
    ZGStrandType::Plus,
    ZGStrandType::Minus,
    ZGStrandType::Dot
} ;
const int ZGuiWidgetComboStrand::m_strand_id(qMetaTypeId<ZGStrandType>()) ;

ZGuiWidgetComboStrand::ZGuiWidgetComboStrand(QWidget *parent )
    : QComboBox(parent),
      Util::InstanceCounter<ZGuiWidgetComboStrand>(),
      Util::ClassName<ZGuiWidgetComboStrand>(),
      m_strand_map()
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetComboStrand::ZGuiWidgetComboStrand() ; can not instantiate without QApplication existing ")) ;

    QVariant var ;
    ZGStrandType type ;
    std::stringstream str ;

    int index = 0 ;
    for (auto it = m_strand_register.begin() ; it != m_strand_register.end() ; ++it, ++index, str.str(""))
    {
        type = *it ;
        var.setValue(type) ;
        str << type ;
        insertItem(index, QString::fromStdString(str.str()), var) ;
        m_strand_map.insert(std::pair<ZGStrandType,int>(type, index)) ;
    }
}

ZGuiWidgetComboStrand::~ZGuiWidgetComboStrand()
{
}

bool ZGuiWidgetComboStrand::setStrand(ZGStrandType strand)
{
    bool result = false ;
    auto it = m_strand_map.find(strand) ;
    if (it != m_strand_map.end())
    {
        setCurrentIndex(it->second) ;
        result = true ;
    }
    return result ;
}

ZGStrandType ZGuiWidgetComboStrand::getStrand() const
{
    ZGStrandType strand = ZGStrandType::Invalid ;
    QVariant var = currentData() ;
    if (var.canConvert<ZGStrandType>() && var.convert(m_strand_id))
        strand = var.value<ZGStrandType>() ;
    return strand ;
}

ZGuiWidgetComboStrand* ZGuiWidgetComboStrand::newInstance(QWidget* parent)
{
    ZGuiWidgetComboStrand* thing = Q_NULLPTR ;

    try
    {
        thing = new ZGuiWidgetComboStrand(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

} // namespace GUI

} // namespace ZMap

