#include "ZGuiWidgetInfoPanel.h"
#include "ZGuiWidgetCreator.h"
#include "ZGuiWidgetFramedLabel.h"
#include <QHBoxLayout>
#include <new>
#include <stdexcept>

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiWidgetInfoPanel>::m_name("ZGuiWidgetInfoPanel") ;
const size_t ZGuiWidgetInfoPanel::m_num_entries = 8 ;
const QStringList ZGuiWidgetInfoPanel::m_tooltips =
{
    QString("Feature name"),
    QString("Feature strand"),
    QString("Feature coordinates"),
    QString("Subfeature"),
    QString("Score"),
    QString("Sequence Ontology term"),
    QString("Featureset"),
    QString("Feature source")
} ;

ZGuiWidgetInfoPanel::ZGuiWidgetInfoPanel(QWidget *parent)
    : QWidget(parent),
      Util::InstanceCounter<ZGuiWidgetInfoPanel>(),
      Util::ClassName<ZGuiWidgetInfoPanel>(),
      m_top_layout(Q_NULLPTR),
      m_label_name(Q_NULLPTR),
      m_label_strand(Q_NULLPTR),
      m_label_coords(Q_NULLPTR),
      m_label_subfeature(Q_NULLPTR),
      m_label_score(Q_NULLPTR),
      m_label_soterm(Q_NULLPTR),
      m_label_featureset(Q_NULLPTR),
      m_label_featuresource(Q_NULLPTR)
{
    if (!Util::canCreateQtItem())
        throw std::runtime_error(std::string("ZGuiWidgetInfoPanel::ZGuiWidgetInfoPanel() ; can not create ")) ;

    if (!(m_top_layout = ZGuiWidgetCreator::createHBoxLayout()))
        throw std::runtime_error(std::string("ZGuiWidgetInfoPanel::ZGuiWidgetInfoPanel() ; unable to create top level layout ")) ;
    setLayout(m_top_layout) ;
    m_top_layout->setMargin(0) ;

    if (!(m_label_name = ZGuiWidgetFramedLabel::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetInfoPanel::ZGuiWidgetInfoPanel() ; unable to create label_name ")) ;
    m_top_layout->addWidget(m_label_name) ;

    if (!(m_label_strand = ZGuiWidgetFramedLabel::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetInfoPanel::ZGuiWidgetInfoPanel() ; unable to create label_strand ")) ;
    m_top_layout->addWidget(m_label_strand) ;

    if (!(m_label_coords = ZGuiWidgetFramedLabel::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetInfoPanel::ZGuiWidgetInfoPanel() ; unable to create label_coords ")) ;
    m_top_layout->addWidget(m_label_coords) ;

    if (!(m_label_subfeature = ZGuiWidgetFramedLabel::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetInfoPanel::ZGuiWidgetInfoPanel() ; unable to create label_subfeature")) ;
    m_top_layout->addWidget(m_label_subfeature) ;

    if (!(m_label_score = ZGuiWidgetFramedLabel::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetInfoPanel::ZGuiWidgetInfoPanel() ; unable to create label_score")) ;
    m_top_layout->addWidget(m_label_score ) ;

    if (!(m_label_soterm = ZGuiWidgetFramedLabel::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetInfoPanel::ZGuiWidgetInfoPanel() ; unable to create label_soterm")) ;
    m_top_layout->addWidget(m_label_soterm) ;

    if (!(m_label_featureset = ZGuiWidgetFramedLabel::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetInfoPanel::ZGuiWidgetInfoPanel() ; unable to create label_featureset")) ;
    m_top_layout->addWidget(m_label_featureset) ;

    if (!(m_label_featuresource = ZGuiWidgetFramedLabel::newInstance()))
        throw std::runtime_error(std::string("ZGuiWidgetInfoPanel::ZGuiWidgetInfoPanel() ; unable to create label_featuresource")) ;
    m_top_layout->addWidget(m_label_featuresource) ;

    setToolTips() ;
}

ZGuiWidgetInfoPanel::~ZGuiWidgetInfoPanel()
{
}

ZGuiWidgetInfoPanel* ZGuiWidgetInfoPanel::newInstance(QWidget *parent)
{
    ZGuiWidgetInfoPanel* thing = Q_NULLPTR ;
    try
    {
        thing = new (std::nothrow) ZGuiWidgetInfoPanel(parent) ;
    }
    catch (...)
    {
        thing = Q_NULLPTR ;
    }
    return thing ;
}

void ZGuiWidgetInfoPanel::setName(const QString &str)
{
    if (m_label_name)
    {
        m_label_name->setText(str) ;
    }
}

QString ZGuiWidgetInfoPanel::getName() const
{
    QString str ;
    if (m_label_name)
    {
        str = m_label_name->text() ;
    }
    return str ;
}

void ZGuiWidgetInfoPanel::setStrand(const QString& str)
{
    if (m_label_strand && str.length() == 1)
    {
        m_label_strand->setText(str) ;
    }
}

QString ZGuiWidgetInfoPanel::getStrand() const
{
    QString str ;
    if (m_label_strand)
    {
        str = m_label_strand->text() ;
    }
    return str ;
}

void ZGuiWidgetInfoPanel::setCoords(const QString & str)
{
    if (m_label_coords)
    {
        m_label_coords->setText(str) ;
    }
}

QString ZGuiWidgetInfoPanel::getCoords() const
{
    QString str ;
    if (m_label_coords)
    {
        str = m_label_coords->text() ;
    }
    return str ;
}

void ZGuiWidgetInfoPanel::setSubfeature(const QString& str)
{
    if (m_label_subfeature)
    {
        m_label_subfeature->setText(str) ;
    }
}

QString ZGuiWidgetInfoPanel::getSubfeature() const
{
    QString str ;
    if (m_label_subfeature)
    {
        str = m_label_subfeature->text() ;
    }
    return str ;
}

void ZGuiWidgetInfoPanel::setScore(const QString& str)
{
    if (m_label_score)
    {
        m_label_score->setText(str) ;
    }
}

QString ZGuiWidgetInfoPanel::getScore() const
{
    QString str ;
    if (m_label_score)
    {
        str = m_label_score->text() ;
    }
    return str ;
}

void ZGuiWidgetInfoPanel::setSOTerm(const QString &str)
{
    if (m_label_soterm)
    {
        m_label_soterm->setText(str) ;
    }
}

QString ZGuiWidgetInfoPanel::getSOTerm() const
{
    QString str ;
    if (m_label_soterm)
    {
        str = m_label_soterm->text() ;
    }
    return str ;
}

void ZGuiWidgetInfoPanel::setFeatureset(const QString &str)
{
    if (m_label_featureset)
    {
        m_label_featureset->setText(str) ;
    }
}

QString ZGuiWidgetInfoPanel::getFeatureset() const
{
    QString str ;
    if (m_label_featureset)
    {
        str = m_label_featureset->text() ;
    }
    return str ;
}

void ZGuiWidgetInfoPanel::setFeaturesource(const QString& str)
{
    if (m_label_featuresource)
    {
        m_label_featuresource->setText(str) ;
    }
}

QString ZGuiWidgetInfoPanel::getFeaturesource() const
{
    QString str ;
    if (m_label_featuresource)
    {
        str = m_label_featuresource->text() ;
    }
    return str ;
}

void ZGuiWidgetInfoPanel::setAll(const QStringList& list)
{
    if (list.size() == m_num_entries)
    {
        setName(list.at(0)) ;
        setStrand(list.at(1)) ;
        setCoords(list.at(2)) ;
        setSubfeature(list.at(3)) ;
        setScore(list.at(4)) ;
        setSOTerm(list.at(5)) ;
        setFeatureset(list.at(6)) ;
        setFeaturesource(list.at(7)) ;
    }
}

void ZGuiWidgetInfoPanel::showName(bool flag)
{
    if (m_label_name)
    {
        if (flag)
            m_label_name->show() ;
        else
            m_label_name->hide() ;
    }
}

void ZGuiWidgetInfoPanel::showStrand(bool flag)
{
    if (m_label_strand)
    {
        if (flag)
            m_label_strand->show() ;
        else
            m_label_strand->hide() ;
    }
}

void ZGuiWidgetInfoPanel::showCoords(bool flag)
{
    if (m_label_coords)
    {
        if (flag)
            m_label_coords->show() ;
        else
            m_label_coords->hide() ;
    }
}

void ZGuiWidgetInfoPanel::showSubfeature(bool flag)
{
    if (m_label_subfeature)
    {
        if (flag)
            m_label_subfeature->show() ;
        else
            m_label_subfeature->hide() ;
    }
}

void ZGuiWidgetInfoPanel::showScore(bool flag){
    if (m_label_score)
    {
        if (flag)
            m_label_score->show() ;
        else
            m_label_score->hide() ;
    }
}

void ZGuiWidgetInfoPanel::showSOTerm(bool flag)
{
    if (m_label_soterm)
    {
        if (flag)
            m_label_soterm->show() ;
        else
            m_label_soterm->hide() ;
    }
}

void ZGuiWidgetInfoPanel::showFeatureset(bool flag){
    if (m_label_featureset)
    {
        if (flag)
            m_label_featureset->show() ;
        else
            m_label_featureset->hide() ;
    }
}

void ZGuiWidgetInfoPanel::showFeaturesource(bool flag)
{
    if (m_label_featuresource)
    {
        if (flag)
            m_label_featuresource->show() ;
        else
            m_label_featuresource->hide() ;
    }
}

void ZGuiWidgetInfoPanel::showAll(const QBitArray &array)
{
    if (array.size() == m_num_entries)
    {
        showName(array.at(0)) ;
        showStrand(array.at(1)) ;
        showCoords(array.at(2)) ;
        showSubfeature(array.at(3)) ;
        showScore(array.at(4)) ;
        showSOTerm(array.at(5)) ;
        showFeatureset(array.at(6)) ;
        showFeaturesource(array.at(7)) ;
    }
}

void ZGuiWidgetInfoPanel::showLabel(ZGuiWidgetFramedLabel *label, bool flag)
{
    if (label)
    {
        if (flag)
            label->show() ;
        else
            label->hide() ;
    }
}

void ZGuiWidgetInfoPanel::setToolTips()
{
    if (m_label_name)
        m_label_name->setToolTip(m_tooltips.at(0)) ;
    if (m_label_strand)
        m_label_strand->setToolTip(m_tooltips.at(1)) ;
    if (m_label_coords)
        m_label_coords->setToolTip(m_tooltips.at(2)) ;
    if (m_label_subfeature)
        m_label_subfeature->setToolTip(m_tooltips.at(3)) ;
    if (m_label_score)
        m_label_score->setToolTip(m_tooltips.at(4)) ;
    if (m_label_soterm)
        m_label_soterm->setToolTip(m_tooltips.at(5)) ;
    if (m_label_featureset)
        m_label_featureset->setToolTip(m_tooltips.at(6)) ;
    if (m_label_featuresource)
        m_label_featuresource->setToolTip(m_tooltips.at(7)) ;
}

} // namespace GUI

} // namespace ZMap
