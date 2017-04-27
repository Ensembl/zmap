#ifndef ZGMESSAGESETSEPARATORCOLOR_H
#define ZGMESSAGESETSEPARATORCOLOR_H


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


// set the color of the strand separator

class ZGMessageSetSeparatorColor : public ZGMessage,
        public Util::InstanceCounter<ZGMessageSetSeparatorColor>,
        public Util::ClassName<ZGMessageSetSeparatorColor>
{
public:

    ZGMessageSetSeparatorColor() ;
    ZGMessageSetSeparatorColor(ZInternalID message_id,
                               ZInternalID sequence_id,
                               const ZGColor& color) ;
    ZGMessageSetSeparatorColor(const ZGMessageSetSeparatorColor& message) ;
    ZGMessageSetSeparatorColor& operator=(const ZGMessageSetSeparatorColor& message) ;
    ~ZGMessageSetSeparatorColor() ;
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

#endif // ZGMESSAGESETSEPARATORCOLOR_H
