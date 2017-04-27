#ifndef ZGMESSAGESETVIEWORIENTATION_H
#define ZGMESSAGESETVIEWORIENTATION_H

#include "ZGMessage.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZInternalID.h"
#include "ZGViewOrientationType.h"
#include <string>
#include <cstddef>

namespace ZMap
{

namespace GUI
{

//
// This sets the orientation of whichever view is current for the specified
// sequence and gui.
//

class ZGMessageSetViewOrientation : public ZGMessage,
        public Util::InstanceCounter<ZGMessageSetViewOrientation>,
        public Util::ClassName<ZGMessageSetViewOrientation>
{
public:

    ZGMessageSetViewOrientation();
    ZGMessageSetViewOrientation(ZInternalID message_id,
                                ZInternalID sequence_id,
                                ZInternalID gui_id,
                                ZGViewOrientationType orientation ) ;
    ZGMessageSetViewOrientation(const ZGMessageSetViewOrientation& message) ;
    ZGMessageSetViewOrientation& operator=(const ZGMessageSetViewOrientation& message) ;
    ZGMessage* clone() const override ;
    ~ZGMessageSetViewOrientation() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZInternalID getGuiID() const {return m_gui_id ; }
    ZGViewOrientationType getOrientation() const {return m_view_orientation ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_sequence_id,
        m_gui_id ;
    ZGViewOrientationType m_view_orientation ;
};

} // namespace GUI

} // namespace ZMap


#endif // ZGMESSAGESETVIEWORIENTATION_H
