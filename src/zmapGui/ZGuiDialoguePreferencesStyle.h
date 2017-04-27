#ifndef ZGUIDIALOGUEPREFERENCESSTYLE_H
#define ZGUIDIALOGUEPREFERENCESSTYLE_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include <QProxyStyle>
#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{


class ZGuiDialoguePreferencesStyle : public QProxyStyle,
        public Util::InstanceCounter<ZGuiDialoguePreferencesStyle>,
        public Util::ClassName<ZGuiDialoguePreferencesStyle>
{
public:
    static ZGuiDialoguePreferencesStyle* newInstance() ;

    ~ZGuiDialoguePreferencesStyle() ;

    int styleHint(StyleHint hint,
                  const QStyleOption *option=Q_NULLPTR,
                  const QWidget *widget=Q_NULLPTR,
                  QStyleHintReturn *returnData=Q_NULLPTR) const override ;

private:
    ZGuiDialoguePreferencesStyle();
    ZGuiDialoguePreferencesStyle(const ZGuiDialoguePreferencesStyle& ) = delete ;
    ZGuiDialoguePreferencesStyle& operator=(const ZGuiDialoguePreferencesStyle& ) = delete ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIDIALOGUEPREFERENCESSTYLE_H
