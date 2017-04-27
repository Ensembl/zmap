#ifndef ZGSEARCHDNADATA_H
#define ZGSEARCHDNADATA_H

#include "ZInternalID.h"
#include "ZGStrandType.h"
#include "InstanceCounter.h"
#include "ClassName.h"
#include <cstddef>
#include <string>
#include <cstdint>

#ifndef QT_NO_DEBUG
#include <QDebug>
#endif
#include <QColor>

//
// Used to transfer data in and out of the SearchDNA dialogue.
//
// Things to do on this class:
// (1) Should we attempt to police values that are set? e.g. strand = [*,+,-],
//     start <= end, characters in the sequence string, etc? Some of this is
//     done in the gui objects... but not all.
//


namespace ZMap
{

namespace GUI
{

class ZGSearchDNAData: public Util::InstanceCounter<ZGSearchDNAData>,
        public Util::ClassName<ZGSearchDNAData>
{
public:

    ZGSearchDNAData() ;
    ZGSearchDNAData(const std::string & sequence,
                    ZGStrandType strand,
                    uint32_t start,
                    uint32_t end,
                    uint32_t mis,
                    uint32_t bas,
                    const QColor& col_forward,
                    const QColor& col_reverse,
                    bool preserve) ;
    ZGSearchDNAData(const ZGSearchDNAData& data) ;
    ZGSearchDNAData& operator=(const ZGSearchDNAData& data) ;
    ~ZGSearchDNAData() ;

    void setSequence(const std::string & str) ;
    std::string getSequence() const {return m_sequence ; }

    void setColorForward(const QColor& col) ;
    QColor getColorForward() const {return m_col_forward ; }

    void setColorReverse(const QColor& col) ;
    QColor getColorReverse() const {return m_col_reverse ; }

    void setStrand(ZGStrandType strand) ;
    ZGStrandType getStrand() const {return m_strand ; }

    void setStart(uint32_t start) ;
    uint32_t getStart() const {return m_start ; }

    void setEnd(uint32_t end) ;
    uint32_t getEnd() const {return m_end ; }

    void setMismatches(uint32_t mis) ;
    uint32_t getMismatches() const {return m_mis ; }

    void setBases(uint32_t bas) ;
    uint32_t getBases() const {return m_bas ; }

    void setPreserve(bool set) ;
    bool getPreserve() const {return m_preserve ; }

private:
    static const size_t m_max_sequence_length ;

    std::string m_sequence ;
    ZGStrandType m_strand ;
    uint32_t m_start, m_end, m_mis, m_bas ;
    QColor m_col_forward,
        m_col_reverse ;
    bool m_preserve ;
};


#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const ZGSearchDNAData& data) ;
#endif


} // namespace GUI

} // namespace ZMap


#endif // ZGSEARCHDNADATA_H
