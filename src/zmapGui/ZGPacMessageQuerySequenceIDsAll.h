#ifndef ZGPACMESSAGEQUERYSEQUENCEIDSALL_H
#define ZGPACMESSAGEQUERYSEQUENCEIDSALL_H


//
// Query the sequecne ids stored in the whole system.
//

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

class ZGPacMessageQuerySequenceIDsAll : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageQuerySequenceIDsAll>,
        public Util::ClassName<ZGPacMessageQuerySequenceIDsAll>
{
public:

    ZGPacMessageQuerySequenceIDsAll() ;
    ZGPacMessageQuerySequenceIDsAll(ZInternalID message_id) ;
    ZGPacMessageQuerySequenceIDsAll(const ZGPacMessageQuerySequenceIDsAll& message) ;
    ZGPacMessageQuerySequenceIDsAll& operator=(const ZGPacMessageQuerySequenceIDsAll& message) ;
    ZGPacMessage * clone() const override ;
    ~ZGPacMessageQuerySequenceIDsAll() ;

private:
    static const ZGPacMessageType m_specific_type ;

};

} // namspace GUI

} // namespace ZMap

#endif // ZGPACMESSAGEQUERYSEQUENCEIDSALL_H
