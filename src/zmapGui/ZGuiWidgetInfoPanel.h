#ifndef ZGUIWIDGETINFOPANEL_H
#define ZGUIWIDGETINFOPANEL_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include <cstddef>
#include <string>
#include <QtGlobal>
#include <QApplication>
#include <QWidget>
#include <QString>
#include <QStringList>
#include <QBitArray>

QT_BEGIN_NAMESPACE
class QHBoxLayout ;
QT_END_NAMESPACE

namespace ZMap
{

namespace GUI
{

class ZGuiWidgetFramedLabel ;

class ZGuiWidgetInfoPanel : public QWidget,
        public Util::InstanceCounter<ZGuiWidgetInfoPanel>,
        public Util::ClassName<ZGuiWidgetInfoPanel>
{

    Q_OBJECT

public:
    static ZGuiWidgetInfoPanel* newInstance(QWidget* parent = Q_NULLPTR) ;

    ~ZGuiWidgetInfoPanel() ;

    void setName(const QString& str) ;
    QString getName() const ;

    void setStrand(const QString& str) ;
    QString getStrand() const ;

    void setCoords(const QString& str) ;
    QString getCoords() const ;

    void setSubfeature(const QString& str) ;
    QString getSubfeature() const ;

    void setScore(const QString& str) ;
    QString getScore() const ;

    void setSOTerm(const QString& str) ;
    QString getSOTerm() const ;

    void setFeatureset(const QString& str) ;
    QString getFeatureset() const ;

    void setFeaturesource(const QString& str) ;
    QString getFeaturesource() const ;

    void setAll(const QStringList& list) ;
    QStringList getAll() const ;

    void showName(bool flag) ;
    void showStrand(bool flag) ;
    void showCoords(bool flag) ;
    void showSubfeature(bool flag) ;
    void showScore(bool flag) ;
    void showSOTerm(bool flag) ;
    void showFeatureset(bool flag) ;
    void showFeaturesource(bool flag) ;

    void showAll(const QBitArray& array) ;

signals:

public slots:

private:
    static const size_t m_num_entries ;
    static const QStringList m_tooltips ;

    ZGuiWidgetInfoPanel(QWidget *parent = Q_NULLPTR) ;
    ZGuiWidgetInfoPanel(const ZGuiWidgetInfoPanel&) = delete ;
    ZGuiWidgetInfoPanel& operator=(const ZGuiWidgetInfoPanel&) = delete ;

    void showLabel(ZGuiWidgetFramedLabel* label, bool flag) ;
    void setToolTips() ;

    QHBoxLayout *m_top_layout ;
    ZGuiWidgetFramedLabel * m_label_name,
        *m_label_strand,
        *m_label_coords,
        *m_label_subfeature,
        *m_label_score,
        *m_label_soterm,
        *m_label_featureset,
        *m_label_featuresource ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIWIDGETINFOPANEL_H
