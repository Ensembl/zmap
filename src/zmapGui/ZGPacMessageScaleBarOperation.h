#ifndef ZGPACMESSAGESCALEBAROPERATION_H
#define ZGPACMESSAGESCALEBAROPERATION_H

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

// show or hide scale bar

class ZGPacMessageScaleBarOperation : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageScaleBarOperation>,
        public Util::ClassName<ZGPacMessageScaleBarOperation>
{
public:

    enum class Type: unsigned char
    {
        Invalid,
        Show,
        Hide
    } ;

    ZGPacMessageScaleBarOperation();
    ZGPacMessageScaleBarOperation(ZInternalID message_id,
                                  ZInternalID sequence_id,
                                  ZInternalID gui_id,
                                  Type operation_type ) ;
    ZGPacMessageScaleBarOperation(const ZGPacMessageScaleBarOperation& message) ;
    ZGPacMessageScaleBarOperation& operator=(const ZGPacMessageScaleBarOperation& message) ;
    ZGPacMessage *clone() const override ;
    ~ZGPacMessageScaleBarOperation() ;

    ZInternalID getSequenceID() const {return  m_sequence_id ; }
    ZInternalID getGuiID() const {return m_gui_id ; }
    Type getOperationType() const {return m_operation_type ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_sequence_id,
        m_gui_id ;
    Type m_operation_type ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGPACMESSAGESCALEBAROPERATION_H
