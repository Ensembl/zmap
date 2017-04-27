#ifndef ZGTRACKDATA_H
#define ZGTRACKDATA_H

#include <cstddef>
#include <string>
#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif

//
// Registered for use as a QVariant metatype. This is used in the gui
// as part of the track configuration.
//

namespace ZMap
{

namespace GUI
{

class ZGTrackData: public Util::InstanceCounter<ZGTrackData>,
        public Util::ClassName<ZGTrackData>
{

public:

    enum class ShowType : unsigned char
    {
        Show,
        Auto,
        Hide,
        Invalid
    } ;

    enum class LoadType : unsigned char
    {
        All,
        Marked,
        NoData,
        Invalid
    } ;

    ZGTrackData() ;
    ZGTrackData(ZInternalID id) ;
    ZGTrackData(ZInternalID id,
                const std::string& name,
                ShowType show_forward,
                ShowType show_reverse,
                LoadType load_type,
                const std::string& group,
                const std::string& styles) ;
    ZGTrackData(const ZGTrackData& data) ;
    ZGTrackData& operator=(const ZGTrackData& data) ;
    ~ZGTrackData() ;

    bool setID(ZInternalID id) ;
    ZInternalID getID() const {return m_id ; }
    void setTrackName(const std::string& name) ;
    std::string getTrackName() const {return m_track_name ; }
    void setIsCurrent(bool is) {m_is_current = is; }
    bool getIsCurrent() const {return m_is_current; }
    void setShowForward(ShowType show) ;
    ShowType getShowForward() const {return m_show_forward ; }
    void setShowReverse(ShowType show) ;
    ShowType getShowReverse() const {return m_show_reverse ; }
    void setLoad(LoadType load) ;
    LoadType getLoad() const {return m_load_type ; }
    void setGroup(const std::string& group) ;
    std::string getGroup() const {return m_group ; }
    void setStyles(const std::string& data) ;
    std::string getStyles() const {return m_styles ; }

private:

    ZInternalID m_id ;
    std::string m_track_name ;
    bool m_is_current ;
    ShowType m_show_forward,
        m_show_reverse ;
    LoadType m_load_type ;
    std::string m_group,
       m_styles ;
};

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGTrackData& data) ;
#endif

class ZGTrackDataCompare
{
public:
    bool operator()(const ZGTrackData& t1, const ZGTrackData& t2) const
    {
        return t1.getID() < t2.getID() ;
    }
};

} // namespace GUI

} // namespace ZMap


#endif // ZGTRACKDATA_H
