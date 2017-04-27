#ifndef ZGPACMESSAGESETTRACKBGCOLOR_H

#define ZGPACMESSAGESETTRACKBGCOLOR_H

#include "ZGPacMessage.h"
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include "ZGColor.h"
#include <cstddef>
#include <string>

namespace ZMap
{

namespace GUI
{

class ZGPacMessageSetTrackBGColor : public ZGPacMessage,
        public Util::InstanceCounter<ZGPacMessageSetTrackBGColor>,
        public Util::ClassName<ZGPacMessageSetTrackBGColor>
{
public:

    ZGPacMessageSetTrackBGColor();
    ZGPacMessageSetTrackBGColor(ZInternalID message_id,
                                ZInternalID sequence_id,
                                ZInternalID track_id,
                                const ZGColor& color) ;
    ZGPacMessageSetTrackBGColor(const ZGPacMessageSetTrackBGColor& message) ;
    ZGPacMessageSetTrackBGColor& operator=(const ZGPacMessageSetTrackBGColor& message) ;
    ZGPacMessage* clone() const override ;
    ~ZGPacMessageSetTrackBGColor() ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZInternalID getTrackID() const {return m_track_id ; }
    ZGColor getColor() const {return m_color ; }

private:
    static const ZGPacMessageType m_specific_type ;

    ZInternalID m_sequence_id,
        m_track_id ;
    ZGColor m_color ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGPACMESSAGESETTRACKBGCOLOR_H
