#include "ZGuiPortValidator.h"


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiPortValidator>::m_name("ZGuiPortValidator") ;

ZGuiPortValidator::ZGuiPortValidator()
    : QValidator(Q_NULLPTR),
      Util::InstanceCounter<ZGuiPortValidator>(),
      Util::ClassName<ZGuiPortValidator>()
{
}

ZGuiPortValidator::~ZGuiPortValidator()
{
}

QValidator::State ZGuiPortValidator::validate(QString& data, int &) const
{
    State state = Invalid ;
    bool ok = false ;
    uint32_t value = 0 ;

    if (!data.length())
        state = Intermediate ;
    else if ((value = data.toUInt(&ok)) && ok)
    {
        state = Acceptable ;
    }

    return state ;
}


ZGuiPortValidator* ZGuiPortValidator::newInstance()
{
    ZGuiPortValidator* validator = Q_NULLPTR ;

    try
    {
        validator = new ZGuiPortValidator ;
    }
    catch (...)
    {
        validator = Q_NULLPTR ;
    }

    return validator ;
}

} // namespace GUI

} // namespace ZMap

