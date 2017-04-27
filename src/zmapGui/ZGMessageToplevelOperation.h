#ifndef ZGMESSAGETOPLEVELOPERATION_H
#define ZGMESSAGETOPLEVELOPERATION_H

#include "ZGMessage.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <string>
#include <cstddef>

namespace ZMap
{

namespace GUI
{

class ZGMessageToplevelOperation: public ZGMessage,
        public Util::InstanceCounter<ZGMessageToplevelOperation>,
        public Util::ClassName<ZGMessageToplevelOperation>
{
public:

    enum class Type: unsigned char
    {
        Invalid,
        Create,
        Delete,
        Show,
        Hide
    } ;

    ZGMessageToplevelOperation();
    ZGMessageToplevelOperation(ZInternalID message_id,
                               ZGMessageToplevelOperation::Type type ) ;
    ZGMessageToplevelOperation(const ZGMessageToplevelOperation& data) ;
    ZGMessageToplevelOperation& operator=(const ZGMessageToplevelOperation& data) ;
    ZGMessage *clone() const override ;
    ~ZGMessageToplevelOperation() ;

    ZGMessageToplevelOperation::Type getOperationType() const {return m_operation_type ;}

private:
    static const ZGMessageType m_specific_type ;

    ZGMessageToplevelOperation::Type m_operation_type ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGMESSAGETOPLEVELOPERATION_H
