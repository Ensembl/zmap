#ifndef ZGUISEQUENCEVALIDATORDNA_H
#define ZGUISEQUENCEVALIDATORDNA_H

//
// Input validator for DNA sequence; the input string is tolowered, and
// we test against the contents of m_accept_chars (see implementation file).
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

class ZGuiSequenceValidatorDNA : public QValidator,
        public Util::InstanceCounter<ZGuiSequenceValidatorDNA>,
        public Util::ClassName<ZGuiSequenceValidatorDNA>
{

    Q_OBJECT

public:
    static ZGuiSequenceValidatorDNA * newInstance() ;

    ~ZGuiSequenceValidatorDNA() ;

    State validate(QString& , int &) const Q_DECL_OVERRIDE ;

private:

    ZGuiSequenceValidatorDNA() ;
    static const QString m_accept_chars ;

};

} // namespace GUI

} // namespace ZMap


#endif // ZGUISEQUENCEVALIDATORDNA_H
