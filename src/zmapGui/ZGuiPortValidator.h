#ifndef ZGUIPORTVALIDATOR_H
#define ZGUIPORTVALIDATOR_H

#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>
#include <cstdint>
#include <QValidator>

//
// Validator to be used with various dialogues for port number
//
//


namespace ZMap
{

namespace GUI
{

class ZGuiPortValidator : public QValidator,
        public Util::InstanceCounter<ZGuiPortValidator>,
        public Util::ClassName<ZGuiPortValidator>
{

    Q_OBJECT

public:

    static ZGuiPortValidator* newInstance() ;

    ~ZGuiPortValidator() ;

    State validate(QString&, int &) const Q_DECL_OVERRIDE ;

private:

    explicit ZGuiPortValidator() ;
    ZGuiPortValidator(const ZGuiPortValidator&) = delete ;
    ZGuiPortValidator& operator=(const ZGuiPortValidator&) = delete ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGUIPORTVALIDATOR_H
