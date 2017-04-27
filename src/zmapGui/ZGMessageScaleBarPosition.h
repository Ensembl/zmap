#ifndef ZGMESSAGESCALEBARPOSITION_H
#define ZGMESSAGESCALEBARPOSITION_H

#include "ZGMessage.h"
#include "ZInternalID.h"
#include "ZGScaleRelativeType.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>

// set the relative position of the scale bar for the specified sequence
// in the specified gui window


namespace ZMap
{

namespace GUI
{


class ZGMessageScaleBarPosition : public ZGMessage,
        public Util::InstanceCounter<ZGMessageScaleBarPosition>,
        public Util::ClassName<ZGMessageScaleBarPosition>
{
public:

    ZGMessageScaleBarPosition();
    ZGMessageScaleBarPosition(ZInternalID message_id,
                              ZInternalID sequence_id,
                              ZInternalID gui_id ,
                              ZGScaleRelativeType position) ;
    ZGMessageScaleBarPosition(const ZGMessageScaleBarPosition& message) ;
    ZGMessageScaleBarPosition& operator=(const ZGMessageScaleBarPosition& message) ;
    ZGMessage* clone() const override ;
    ~ZGMessageScaleBarPosition() ;

    ZInternalID getSequenceID() const {return m_sequence_id ;}
    ZInternalID getGuiID() const {return m_gui_id ; }
    ZGScaleRelativeType getScalePosition() const {return m_scale_position ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_sequence_id,
        m_gui_id ;
    ZGScaleRelativeType m_scale_position ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGMESSAGESCALEBARPOSITION_H
