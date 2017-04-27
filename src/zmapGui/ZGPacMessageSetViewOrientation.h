#ifndef ZGPACMESSAGESETVIEWORIENTATION_H
#define ZGPACMESSAGESETVIEWORIENTATION_H

#include "ZGPacMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGViewOrientationType.h"
#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{

//
// Set the orientation of the current view for the specified sequence and gui.
//

class ZGPacMessageSetViewOrientation : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageSetViewOrientation>,
        public Util::ClassName<ZGPacMessageSetViewOrientation>
{
public:

    ZGPacMessageSetViewOrientation();
    ZGPacMessageSetViewOrientation(ZInternalID message_id,
                                   ZInternalID sequence_id,
                                   ZInternalID gui_id,
                                   ZGViewOrientationType orientation) ;
    ZGPacMessageSetViewOrientation(const ZGPacMessageSetViewOrientation& message) ;
    ZGPacMessageSetViewOrientation& operator=(const ZGPacMessageSetViewOrientation& message) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageSetViewOrientation() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZInternalID getGuiID() const {return m_gui_id ; }
    ZGViewOrientationType getOrientation() const {return m_view_orientation ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_sequence_id,
        m_gui_id ;
    ZGViewOrientationType m_view_orientation ;


};

} // namespace GUI

} // namespace ZMap

#endif // ZGPACMESSAGESETVIEWORIENTATION_H
