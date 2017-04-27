#ifndef INSTANCECOUNTER_H
#define INSTANCECOUNTER_H

#include <cstddef>

namespace ZMap
{

namespace Util
{

template <typename ClientClass>
class InstanceCounter
{
public:
    static size_t instances() {return m_instances ; }
    InstanceCounter()
    {
        ++m_instances ;
    }
protected:
    ~InstanceCounter()
    {
        --m_instances ;
    }

private:
    static size_t m_instances ;
};

template <typename ClientClass>
size_t InstanceCounter<ClientClass>::m_instances(0) ;

} // namespace Util

} // namespace ZMap

#endif // INSTANCECOUNTER_H
