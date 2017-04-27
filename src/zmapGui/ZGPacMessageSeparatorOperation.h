#ifndef ZGPACMESSAGESEPARATOROPERATION_H
#define ZGPACMESSAGESEPARATOROPERATION_H

#include "ZGPacMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{

class ZGPacMessageSeparatorOperation : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageSeparatorOperation>,
        public Util::ClassName<ZGPacMessageSeparatorOperation>
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

    ZGPacMessageSeparatorOperation();
    ZGPacMessageSeparatorOperation(ZInternalID message_id,
                                   ZInternalID sequence_id,
                                   Type operation_type) ;
    ZGPacMessageSeparatorOperation(const ZGPacMessageSeparatorOperation& message) ;
    ZGPacMessageSeparatorOperation& operator=(const ZGPacMessageSeparatorOperation& message) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageSeparatorOperation() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    Type getOperationType() const {return m_operation_type ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_sequence_id ;
    Type m_operation_type ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGPACMESSAGESEPARATOROPERATION_H
