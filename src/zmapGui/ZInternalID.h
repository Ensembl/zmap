#ifndef ZINTERNALID_H
#define ZINTERNALID_H

#include <cstdint>

// This is used to map into the ZMap internal id (i.e. the
// unique_id and original_id data members of various entities)
// These are QQuark, which is defined in
//   'https://developer.gnome.org/glib/stable/glib-Quarks.html#GQuark'
// to be typedef guint32 GQuark. This is safe on the systems we
// currently support, but obviously not universal.


namespace ZMap
{

namespace GUI
{

typedef uint32_t ZInternalID ;

// This is a bit redundant since this operation would be the default
// for this built in type; it's more of a placeholder since at some point
// the ID type itself is going to be revised when we get rid of glib.
// If we ever, do that is. It's on the list...
class ZInternalIDCompare
{
public:
    bool operator() (ZInternalID id1, ZInternalID id2) const
    {
        return id1 < id2 ;
    }
};

} // namespace GUI

} // namespace ZMap

#endif // ZINTERNALID_H
