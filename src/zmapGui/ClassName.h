#ifndef CLASSNAME_H
#define CLASSNAME_H

#include <string>

namespace ZMap
{

namespace Util
{

template <typename ClientClass>
class ClassName
{
public:
    static std::string className() {return m_name ; }
    std::string name() const {return m_name ; }
private:
    static const std::string m_name ;
};


} // namespace Util

} // namespace ZMap


#endif
