#ifndef ZGMESSAGESEPARATOROPERATION_H
#define ZGMESSAGESEPARATOROPERATION_H

#include "ZGMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <string>
#include <cstddef>

// create, delete, show or hide the strand separator
// within a particular sequence


namespace ZMap
{

namespace GUI
{


class ZGMessageSeparatorOperation : public ZGMessage,
        public Util::InstanceCounter<ZGMessageSeparatorOperation>,
        public Util::ClassName<ZGMessageSeparatorOperation>
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

    ZGMessageSeparatorOperation();
    ZGMessageSeparatorOperation(ZInternalID message_id,
                                ZInternalID sequence_id,
                                Type operation_type ) ;
    ZGMessageSeparatorOperation(const ZGMessageSeparatorOperation& message) ;
    ZGMessageSeparatorOperation& operator=(const ZGMessageSeparatorOperation& message) ;
    ZGMessage * clone() const override ;
    ~ZGMessageSeparatorOperation() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    Type getOperationType() const {return m_operation_type ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_sequence_id ;
    Type m_operation_type ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGMESSAGESEPARATOROPERATION_H
