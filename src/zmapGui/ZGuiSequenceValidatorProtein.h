#ifndef ZGUISEQUENCEVALIDATORPROTEIN_H
#define ZGUISEQUENCEVALIDATORPROTEIN_H

//
// Input validator for protein sequence.
//

#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>
#include <QValidator>
#include <QString>

namespace ZMap
{

namespace GUI
{

class ZGuiSequenceValidatorProtein : public QValidator,
        public Util::InstanceCounter<ZGuiSequenceValidatorProtein>,
        public Util::ClassName<ZGuiSequenceValidatorProtein>
{

    Q_OBJECT

public:
    static ZGuiSequenceValidatorProtein* newInstance() ;

    ~ZGuiSequenceValidatorProtein() ;

    State validate(QString &, int &) const Q_DECL_OVERRIDE ;

private:

    ZGuiSequenceValidatorProtein() ;
    static const QString m_accept_chars ;

};

} // namespace GUI

} // namespace ZMap


#endif // ZGUISEQUENCEVALIDATORPROTEIN_H
