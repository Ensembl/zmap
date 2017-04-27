#include "ZGuiSequenceValidatorProtein.h"

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif


namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiSequenceValidatorProtein>::m_name("ZGuiSequenceValidatorProtein") ;
const QString ZGuiSequenceValidatorProtein::m_accept_chars("arndceqghilkmfpstwyvuo") ;

ZGuiSequenceValidatorProtein::ZGuiSequenceValidatorProtein()
    : QValidator(Q_NULLPTR),
      Util::InstanceCounter<ZGuiSequenceValidatorProtein>(),
      Util::ClassName<ZGuiSequenceValidatorProtein>()
{
}

ZGuiSequenceValidatorProtein::~ZGuiSequenceValidatorProtein()
{
}

QValidator::State ZGuiSequenceValidatorProtein::validate(QString &str, int & ) const
{
    State state = Invalid ;

    if (!str.length())
        state = Intermediate ;
    else
    {
        state = Acceptable ;
        const QString str_lower = str.toLower() ;
        QString::const_iterator it = str_lower.begin() ;
        for ( ; it != str_lower.end() ; ++it)
        {
            if (!m_accept_chars.contains(*it))
            {
                state = Invalid ;
                break ;
            }
        }
    }

    return state ;
}




ZGuiSequenceValidatorProtein* ZGuiSequenceValidatorProtein::newInstance()
{
    ZGuiSequenceValidatorProtein* validator = Q_NULLPTR ;

    try
    {
        validator = new ZGuiSequenceValidatorProtein ;
    }
    catch (...)
    {
        validator = Q_NULLPTR ;
    }

    return validator ;
}

} // namespace GUI

} // namespace ZMap

