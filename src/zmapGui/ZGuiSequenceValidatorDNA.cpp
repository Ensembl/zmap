#include "ZGuiSequenceValidatorDNA.h"

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

namespace ZMap
{

namespace GUI
{

template <> const std::string Util::ClassName<ZGuiSequenceValidatorDNA>::m_name("ZGuiSequenceValidatorDNA") ;
const QString ZGuiSequenceValidatorDNA::m_accept_chars("acgt") ;

ZGuiSequenceValidatorDNA::ZGuiSequenceValidatorDNA()
    : QValidator(Q_NULLPTR),
      Util::InstanceCounter<ZGuiSequenceValidatorDNA>(),
      Util::ClassName<ZGuiSequenceValidatorDNA>()
{
}

ZGuiSequenceValidatorDNA::~ZGuiSequenceValidatorDNA()
{
}

QValidator::State ZGuiSequenceValidatorDNA::validate(QString& str, int &) const
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


ZGuiSequenceValidatorDNA* ZGuiSequenceValidatorDNA::newInstance()
{
    ZGuiSequenceValidatorDNA * validator = Q_NULLPTR ;

    try
    {
        validator = new ZGuiSequenceValidatorDNA ;
    }
    catch (...)
    {
        validator = Q_NULLPTR ;
    }

    return validator ;
}

} // namespace GUI

} // namespace ZMap

