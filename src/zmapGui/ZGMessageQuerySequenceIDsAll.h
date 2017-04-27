#ifndef ZGMESSAGEQUERYSEQUENCEIDSALL_H
#define ZGMESSAGEQUERYSEQUENCEIDSALL_H

#include "ZGMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <string>
#include <cstddef>

namespace ZMap
{

namespace GUI
{

class ZGMessageQuerySequenceIDsAll: public ZGMessage,
        public Util::InstanceCounter<ZGMessageQuerySequenceIDsAll>,
        public Util::ClassName<ZGMessageQuerySequenceIDsAll>
{
public:

    ZGMessageQuerySequenceIDsAll();
    ZGMessageQuerySequenceIDsAll(ZInternalID message_id ) ;
    ZGMessageQuerySequenceIDsAll(const ZGMessageQuerySequenceIDsAll& message) ;
    ZGMessageQuerySequenceIDsAll& operator=(const ZGMessageQuerySequenceIDsAll& message) ;
    ZGMessage* clone() const override ;
    ~ZGMessageQuerySequenceIDsAll() ;

private:
    static const ZGMessageType m_specific_type ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGMESSAGEQUERYSEQUENCEIDSALL_H
