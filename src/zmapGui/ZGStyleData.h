#ifndef ZGSTYLEDATA_H
#define ZGSTYLEDATA_H

#include "ZInternalID.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <QColor>
#ifndef QT_NO_DEBUG
#include <QDebug>
#endif
#include <cstddef>
#include <string>
#include <set>

namespace ZMap
{

namespace GUI
{

//
// This is the style that can also represent the track style, so has a list
// of featureset names associated with it.
//

class ZGStyleData: public Util::InstanceCounter<ZGStyleData>,
        public Util::ClassName<ZGStyleData>
{
public:

    ZGStyleData() ;
    ZGStyleData(ZInternalID id ) ;
    ZGStyleData(ZInternalID id,
                ZInternalID id_parent,
                const std::string & style_name,
                const QColor & color_fill,
                const QColor & color_border,
                bool stranded) ;
    ZGStyleData(const ZGStyleData& data) ;
    ZGStyleData& operator=(const ZGStyleData& data) ;
    ~ZGStyleData() ;

    bool setID(ZInternalID id) ;
    ZInternalID getID() const {return m_id ; }

    bool setIDParent(ZInternalID id) ;
    ZInternalID getIDParent() const {return m_id_parent ; }

    void setStyleName(const std::string& data) ;
    std::string getStyleName() const {return m_style_name ; }

    void setColorFill(const QColor& col) ;
    QColor getColorFill() const {return m_color_fill ; }

    void setColorBorder(const QColor& col) ;
    QColor getColorBorder() const {return m_color_border ; }

    void setIsStranded(bool is) {m_is_stranded = is ; }
    bool getIsStranded() const {return m_is_stranded ; }

    size_t getFeaturesetNumber() const {return m_featureset_names.size() ; }
    std::set<std::string> getFeaturesetNames() const {return m_featureset_names; }
    void setFeaturesetNames(const std::set<std::string>& featureset_names ) ;

private:

    ZInternalID m_id, m_id_parent ;
    std::string m_style_name ;
    QColor m_color_fill,
        m_color_border ;
    bool m_is_stranded ;
    std::set<std::string> m_featureset_names ;
};


#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGStyleData& data) ;
#endif

class ZGStyleDataCompare
{
  public:
    bool operator()(const ZGStyleData& s1, const ZGStyleData& s2) const
    {
        return s1.getID() < s2.getID() ;
    }
} ;

} // namespace GUI

} // namespace ZMap


#endif // ZGSTYLEDATA_H
