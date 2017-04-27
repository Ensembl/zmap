#include "ZGuiWidgetMenuTFT.h"
#include "ZGuiWidgetCreator.h"
#include "ZGTFTActionType.h"
#include "Utilities.h"
#include <QAction>
#include <QVariant>
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif
#include <stdexcept>
#include <new>

Q_DECLARE_METATYPE(ZMap::GUI::ZGTFTActionType)

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetMenuTFT>::m_name("ZGuiWidgetMenuTFT") ;
const QString ZGuiWidgetMenuTFT::m_menu_title("3 Frame Menu"),
    ZGuiWidgetMenuTFT::m_label_none("None"),
    ZGuiWidgetMenuTFT::m_label_features("Features"),
    ZGuiWidgetMenuTFT::m_label_tft("3 Frame Translation"),
    ZGuiWidgetMenuTFT::m_label_featurestft("Features + 3FT") ;
const int ZGuiWidgetMenuTFT::m_action_id(qMetaTypeId<ZGTFTActionType>()) ;

ZGuiWidgetMenuTFT* ZGuiWidgetMenuTFT::newInstance(QWidget *parent)
{
    ZGuiWidgetMenuTFT* thing = Q_NULLPTR ;
    try
    {
        thing = new (std::nothrow) ZGuiWidgetMenuTFT(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }

    return thing ;
}

ZGuiWidgetMenuTFT::ZGuiWidgetMenuTFT(QWidget *parent)
    : QMenu(parent),
      Util::InstanceCounter<ZGuiWidgetMenuTFT>(),
      Util::ClassName<ZGuiWidgetMenuTFT>(),
      m_action_title(Q_NULLPTR),
      m_action_none(Q_NULLPTR),
      m_action_features(Q_NULLPTR),
      m_action_tft(Q_NULLPTR),
      m_action_featurestft(Q_NULLPTR),
      m_action_type(ZGTFTActionType::Invalid)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetMenuTFT::ZGuiWidgetMenuTFT() ; unable to create instance ")) ;

    if (!(m_action_title = ZGuiWidgetCreator::createAction(m_menu_title, this)))
        throw std::runtime_error(std::string("ZGuiWidgetMenuTFT::ZGuiWidgetMenuTFT() ; unable to create action_title")) ;
    m_action_title->setEnabled(false) ;
    if (!(m_action_none = ZGuiWidgetCreator::createAction(m_label_none, this)))
        throw std::runtime_error(std::string("ZGuiWidgetMenuTFT::ZGuiWidgetMenuTFT() ; unable to create action_none")) ;
    if (!(m_action_features = ZGuiWidgetCreator::createAction(m_label_features, this)))
        throw std::runtime_error(std::string("ZGuiWidgetMenuTFT::ZGuiWidgetMenuTFT() ; unable to create action_features ")) ;
    if (!(m_action_tft = ZGuiWidgetCreator::createAction(m_label_tft, this)))
        throw std::runtime_error(std::string("ZGuiWidgetMenuTFT::ZGuiWidgetMenuTFT() ; unable to create action_tft")) ;
    if (!(m_action_featurestft = ZGuiWidgetCreator::createAction(m_label_featurestft, this )))
        throw std::runtime_error(std::string("ZGuiWidgetMenuTFT::ZGuiWidgetMenuTFT() ; unable to create action_featurestft")) ;

    addAction(m_action_title) ;
    addSeparator() ;

    QVariant variant ;

    addAction(m_action_none) ;
    variant.setValue(ZGTFTActionType::None) ;
    m_action_none->setData(variant) ;
    if (!connect(m_action_none, SIGNAL(triggered(bool)), this, SLOT(slot_action_triggered())))
        throw std::runtime_error(std::string("ZGuiWidgetMenuTFT::ZGuiWidgetMenuTFT() ; unable to make connection with action_none")) ;

    addAction(m_action_features) ;
    variant.setValue(ZGTFTActionType::Features) ;
    m_action_features->setData(variant) ;
    if (!connect(m_action_features, SIGNAL(triggered(bool)), this, SLOT(slot_action_triggered())))
        throw std::runtime_error(std::string("ZGuiWidgetMenuTFT::ZGuiWidgetMenuTFT() ; unable to make connection for action_features")) ;

    addAction(m_action_tft) ;
    variant.setValue(ZGTFTActionType::TFT) ;
    m_action_tft->setData(variant) ;
    if (!connect(m_action_tft, SIGNAL(triggered(bool)), this, SLOT(slot_action_triggered())))
        throw std::runtime_error(std::string("ZGuiWidgetMenuTFT::ZGuiWidgetMenuTFT() ; unable to make connection with action_tft")) ;

    addAction(m_action_featurestft) ;
    variant.setValue(ZGTFTActionType::FeaturesTFT) ;
    m_action_featurestft->setData(variant) ;
    if (!connect(m_action_featurestft, SIGNAL(triggered(bool)), this, SLOT(slot_action_triggered())))
        throw std::runtime_error(std::string("ZGuiWidgetMenuTFT::ZGuiWidgetMenuTFT() ; unable to make connection with action_featurestft")) ;
}

ZGuiWidgetMenuTFT::~ZGuiWidgetMenuTFT()
{
}

void ZGuiWidgetMenuTFT::slot_action_triggered()
{
    QAction *action = dynamic_cast<QAction*>(QObject::sender()) ;
    m_action_type = ZGTFTActionType::Invalid ;
    if (action)
    {
        QVariant variant = action->data() ;
        if (variant.canConvert<ZGTFTActionType>() && variant.convert(m_action_id))
        {
            m_action_type = variant.value<ZGTFTActionType>() ;
        }
    }
}

} // namespace GUI

} // namespace ZMap
