#ifndef ZGMESSAGESETTRACKBGCOLOR_H
#define ZGMESSAGESETTRACKBGCOLOR_H

#include "ZGMessage.h"
#include "ZInternalID.h"
#include "ZGColor.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <string>
#include <cstddef>

namespace ZMap
{

namespace GUI
{


// set the background color of a track in the gui

class ZGMessageSetTrackBGColor : public ZGMessage,
        public Util::InstanceCounter<ZGMessageSetTrackBGColor>,
        public Util::ClassName<ZGMessageSetTrackBGColor>
{
public:

    ZGMessageSetTrackBGColor();
    ZGMessageSetTrackBGColor(ZInternalID message_id,
                             ZInternalID sequence_id,
                             ZInternalID track_id,
                             const ZGColor & color) ;
    ZGMessageSetTrackBGColor(const ZGMessageSetTrackBGColor& message) ;
    ZGMessageSetTrackBGColor& operator=(const ZGMessageSetTrackBGColor& message) ;
    ~ZGMessageSetTrackBGColor() ;
    ZGMessage* clone() const override ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZInternalID getTrackID() const {return m_track_id ; }
    ZGColor getColor() const {return m_color ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_sequence_id,
        m_track_id ;
    ZGColor m_color ;

};

} // namespace GUI

} // namespace ZMap

#endif // ZGMESSAGESETTRACKBGCOLOR_H
