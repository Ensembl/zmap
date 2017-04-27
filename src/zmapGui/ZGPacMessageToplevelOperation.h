#ifndef ZGPACMESSAGETOPLEVELOPERATION_H
#define ZGPACMESSAGETOPLEVELOPERATION_H

#include "ZGPacMessage.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{

class ZGPacMessageToplevelOperation : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageToplevelOperation>,
        public Util::ClassName<ZGPacMessageToplevelOperation>
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

    ZGPacMessageToplevelOperation() ;
    ZGPacMessageToplevelOperation(ZInternalID message_id,
                                  ZGPacMessageToplevelOperation::Type type ) ;
    ZGPacMessageToplevelOperation(const ZGPacMessageToplevelOperation& data) ;
    ZGPacMessageToplevelOperation& operator=(const ZGPacMessageToplevelOperation& data) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageToplevelOperation() ;

    ZGPacMessageToplevelOperation::Type getOperationType() const {return m_operation_type ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZGPacMessageToplevelOperation::Type m_operation_type ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGPACMESSAGETOPLEVELOPERATION_H
