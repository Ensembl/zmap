#include "ZGuiWidgetMenuZoom.h"
#include "ZGuiWidgetCreator.h"
#include "Utilities.h"
#include <QAction>
#include <QVariant>
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif
#include <stdexcept>
#include <new>

Q_DECLARE_METATYPE(ZMap::GUI::ZGZoomActionType)

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetMenuZoom>::m_name("ZGuiWidgetMenuZoom") ;
const QString ZGuiWidgetMenuZoom::m_menu_title("Zoom menu"),
    ZGuiWidgetMenuZoom::m_label_max("Max zoom"),
    ZGuiWidgetMenuZoom::m_label_alldna("All DNA"),
    ZGuiWidgetMenuZoom::m_label_min("Min zoom") ;
const int ZGuiWidgetMenuZoom::m_action_id(qMetaTypeId<ZGZoomActionType>()) ;

ZGuiWidgetMenuZoom* ZGuiWidgetMenuZoom::newInstance(QWidget* parent)
{
    ZGuiWidgetMenuZoom *thing = Q_NULLPTR ;

    try
    {
        thing = new (std::nothrow) ZGuiWidgetMenuZoom(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

ZGuiWidgetMenuZoom::ZGuiWidgetMenuZoom(QWidget *parent)
    : QMenu(parent),
      Util::InstanceCounter<ZGuiWidgetMenuZoom>(),
      Util::ClassName<ZGuiWidgetMenuZoom>(),
      m_action_title(Q_NULLPTR),
      m_action_max(Q_NULLPTR),
      m_action_alldna(Q_NULLPTR),
      m_action_min(Q_NULLPTR),
      m_action_type(ZGZoomActionType::Invalid)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetMenuZoom::ZGuiWidgetMenuZoom() ; unable to create instance ")) ;

    if (!(m_action_title = ZGuiWidgetCreator::createAction(m_menu_title, this)))
        throw std::runtime_error(std::string("ZGuiWidgetMenuZoom::ZGuiWidgetMenuZoom() ; unable to create action for menu title ")) ;
    m_action_title->setEnabled(false) ;
    if (!(m_action_max = ZGuiWidgetCreator::createAction(m_label_max, this)))
        throw std::runtime_error(std::string("ZGuiWidgetMenuZoom::ZGuiWidgetMenuZoom() ; unable to create action for max zoom item ")) ;
    if (!(m_action_alldna = ZGuiWidgetCreator::createAction(m_label_alldna)))
        throw std::runtime_error(std::string("ZGuiWidgetMenuZoom::ZGuiWidgetMenuZoom() ; unable to create action for all dna item")) ;
    if (!(m_action_min = ZGuiWidgetCreator::createAction(m_label_min, this)))
        throw std::runtime_error(std::string("ZGuiWidgetMenuZoom::ZGuiWidgetMenuZoom() ; unable to create action for min zoom item ")) ;

    addAction(m_action_title) ;
    addSeparator() ;

    QVariant variant ;

    addAction(m_action_max) ;
    variant.setValue(ZGZoomActionType::Max) ;
    m_action_max->setData(variant) ;
    if (!connect(m_action_max, SIGNAL(triggered(bool)), this, SLOT(slot_action_triggered())))
        throw std::runtime_error(std::string("ZGuiWidgetMenuZoom::ZGuiWidgetMenuZoom() ; unable to make connection with action_max")) ;

    addAction(m_action_alldna) ;
    variant.setValue(ZGZoomActionType::AllDNA) ;
    m_action_alldna->setData(variant) ;
    if (!connect(m_action_alldna, SIGNAL(triggered(bool)), this, SLOT(slot_action_triggered())))
        throw std::runtime_error(std::string("ZGuiWidgetMenuZoom::ZGuiWidgetMenuZoom() ; unable to make connection with action_alldna")) ;

    addAction(m_action_min) ;
    variant.setValue(ZGZoomActionType::Min) ;
    m_action_min->setData(variant) ;
    if (!connect(m_action_min, SIGNAL(triggered(bool)), this, SLOT(slot_action_triggered())))
        throw std::runtime_error(std::string("ZGuiWidgetMenuZoom::ZGuiWidgetMenuZoom() ; unable to make connection with action_min")) ;
}

ZGuiWidgetMenuZoom::~ZGuiWidgetMenuZoom()
{
}

void ZGuiWidgetMenuZoom::slot_action_triggered()
{
    QAction *action = dynamic_cast<QAction*>(QObject::sender()) ;
    m_action_type = ZGZoomActionType::Invalid ;
    if (action)
    {
        QVariant variant = action->data() ;
        if (variant.canConvert<ZGZoomActionType>() && variant.convert(m_action_id))
        {
            m_action_type = variant.value<ZGZoomActionType>() ;
        }
    }

}

} // namespace GUI

} // namespace ZMap
