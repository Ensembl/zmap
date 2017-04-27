#ifndef ZGMESSAGESCALEBAROPERATION_H
#define ZGMESSAGESCALEBAROPERATION_H

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

// show or hide the scale bar in the current view for the
// specified sequence and gui window

class ZGMessageScaleBarOperation : public ZGMessage,
        public Util::InstanceCounter<ZGMessageScaleBarOperation>,
        public Util::ClassName<ZGMessageScaleBarOperation>
{
public:

    enum class Type: unsigned char
    {
        Invalid,
        Show,
        Hide
    } ;

    ZGMessageScaleBarOperation();
    ZGMessageScaleBarOperation(ZInternalID message_id,
                               ZInternalID sequence_id,
                               ZInternalID gui_id,
                               Type operation_type ) ;
    ZGMessageScaleBarOperation(const ZGMessageScaleBarOperation& message) ;
    ZGMessageScaleBarOperation& operator=(const ZGMessageScaleBarOperation& message) ;
    ZGMessage *clone() const override ;
    ~ZGMessageScaleBarOperation() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZInternalID getGuiID() const {return m_gui_id ; }
    Type getOperationType() const {return m_operation_type ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_sequence_id,
        m_gui_id ;
    Type m_operation_type ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGMESSAGESCALEBAROPERATION_H
