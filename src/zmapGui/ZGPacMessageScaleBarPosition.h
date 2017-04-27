#ifndef ZGPACMESSAGESCALEBARPOSITION_H
#define ZGPACMESSAGESCALEBARPOSITION_H

#include "ZGPacMessage.h"
#include "ZInternalID.h"
#include "ZGScaleRelativeType.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{

// set scale bar position for the specified sequence and gui ids

class ZGPacMessageScaleBarPosition : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageScaleBarPosition>,
        public Util::ClassName<ZGPacMessageScaleBarPosition>
{
public:

    ZGPacMessageScaleBarPosition();
    ZGPacMessageScaleBarPosition(ZInternalID message_id,
                                 ZInternalID sequence_id,
                                 ZInternalID gui_id,
                                 ZGScaleRelativeType position) ;
    ZGPacMessageScaleBarPosition(const ZGPacMessageScaleBarPosition& message) ;
    ZGPacMessageScaleBarPosition& operator=(const ZGPacMessageScaleBarPosition& message) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageScaleBarPosition() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZInternalID getGuiID() const {return m_gui_id ; }
    ZGScaleRelativeType getScalePosition() const {return m_scale_position ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_sequence_id,
        m_gui_id ;
    ZGScaleRelativeType m_scale_position ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGPACMESSAGESCALEBARPOSITION_H
