#ifndef ZGMESSAGESETSEQUENCEBGCOLOR_H
#define ZGMESSAGESETSEQUENCEBGCOLOR_H

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


// set the background color of a sequence in the gui

class ZGMessageSetSequenceBGColor : public ZGMessage,
        public Util::InstanceCounter<ZGMessageSetSequenceBGColor>,
        public Util::ClassName<ZGMessageSetSequenceBGColor>
{
public:

    ZGMessageSetSequenceBGColor() ;
    ZGMessageSetSequenceBGColor(ZInternalID message_id,
                                ZInternalID sequence_id,
                                const ZGColor& color) ;
    ZGMessageSetSequenceBGColor(const ZGMessageSetSequenceBGColor& message) ;
    ZGMessageSetSequenceBGColor& operator=(const ZGMessageSetSequenceBGColor& message) ;
    ~ZGMessageSetSequenceBGColor() ;
    ZGMessage* clone() const override ;

    ZInternalID getSequenceID() const {return m_sequence_id ; }
    ZGColor getColor() const {return m_color ; }

private:
    static const ZGMessageType m_specific_type ;

    ZInternalID m_sequence_id ;
    ZGColor m_color ;
};

} // namespace GUI

} // namespace ZMap

#endif // ZGMESSAGESETSEQUENCEBGCOLOR_H
