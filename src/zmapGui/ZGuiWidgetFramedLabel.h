#ifndef ZGUIWIDGETFRAMEDLABEL_H
#define ZGUIWIDGETFRAMEDLABEL_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include "Utilities.h"
#include <cstddef>
#include <string>
#include <QApplication>
#include <QLabel>

namespace ZMap
{

namespace GUI
{

class ZGuiWidgetFramedLabel : public QLabel,
        public Util::InstanceCounter<ZGuiWidgetFramedLabel>,
        public Util::ClassName<ZGuiWidgetFramedLabel>
{

    Q_OBJECT

public:
    static ZGuiWidgetFramedLabel* newInstance(QWidget* parent = Q_NULLPTR) ;

    ~ZGuiWidgetFramedLabel() ;

private:

    ZGuiWidgetFramedLabel(QWidget* parent = Q_NULLPTR) ;
    ZGuiWidgetFramedLabel(const ZGuiWidgetFramedLabel&) = delete ;
    ZGuiWidgetFramedLabel& operator=(const ZGuiWidgetFramedLabel& ) = delete ;
};


} // namespace GUI
} // namespace ZMap

#endif // ZGUIWIDGETFRAMEDLABEL_H
