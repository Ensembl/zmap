/*  File: zmapStyle_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Private header for style.
 *
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_STYLE_I_H__
#define __ZMAP_STYLE_I_H__

#define ZMAPSTYLE_INTERNAL    1

#include <ZMap/zmapStyle.h>
#include <ZMap/zmapBase.h>




// all possible types of zmapStyle parameter
typedef enum
  {
    STYLE_PARAM_TYPE_INVALID,

    STYLE_PARAM_TYPE_FLAGS,               // bitmap of is_set flags (array of uchar)
                                          // NOT TO BE USED FOR STYLE PARAMETERS

    STYLE_PARAM_TYPE_QUARK,               // strings turned into integers by config file code
                                          // (old interface) eg parent_id
                                          // also used for paramters derived from strings eg original_id
    STYLE_PARAM_TYPE_BOOLEAN,
    STYLE_PARAM_TYPE_UINT,                // we don't use INT !!
    STYLE_PARAM_TYPE_DOUBLE,
    STYLE_PARAM_TYPE_STRING,              // gchar *
    STYLE_PARAM_TYPE_COLOUR,              // ZMapStyleFullColourStruct, ext5ernal = string
    STYLE_PARAM_TYPE_MODE,                // ZMapStyleMode
    STYLE_PARAM_TYPE_COLDISP,             // ZMapStyleColumnDisplayState
    STYLE_PARAM_TYPE_BUMP,                // ZMapStyleBumpMode
    STYLE_PARAM_TYPE_SQUARK,              // gchar * stored as a quark eg name
    STYLE_PARAM_TYPE_3FRAME,              // ZMapStyle3FrameMode
    STYLE_PARAM_TYPE_SCORE,               // ZMapStyleScoreMode
    STYLE_PARAM_TYPE_GRAPH_MODE,          // ZMapStyleGraphMode
    STYLE_PARAM_TYPE_GRAPH_SCALE,          // ZMapStyleGraphScale
    STYLE_PARAM_TYPE_BLIXEM,              // ZMapStyleBlixemType
    STYLE_PARAM_TYPE_GLYPH_STRAND,        // ZMapStyleGlyphStrand
    STYLE_PARAM_TYPE_GLYPH_ALIGN,         // ZMapStyleGlyphAlign

    STYLE_PARAM_TYPE_GLYPH_SHAPE,         // ZMapStyleGlyphShapeStruct, external = string
    STYLE_PARAM_TYPE_SUB_FEATURES,        // GQuark[ZMAPSTYLE_SUB_FEATURE_MAX], external = string

    STYLE_PARAM_TYPE_QUARK_LIST_ID        // GList of canonicalised GQuark

    /* If you add a new one then please review the following functions:
     *
     * zMapStyleMerge()
     * zMapStyleCopy()
     * zmap_param_spec_init()
     * zmap_feature_type_style_set_property_full()
     * zmap_feature_type_style_get_property()
     * zmapStyleParamSize()
     */

  } ZMapStyleParamType;


typedef struct
  {
      ZMapStyleParamId id;
      ZMapStyleParamType type;

      gchar *name;
      gchar *nick;            // if NULL or "" defaults to name
      gchar *blurb;

      guint offset;           // of the data in the style struct
      ZMapStyleMode mode;     // non zero if mode dependant

      guint8 flag_ind;        // index of the is_set bit in the array
      guint8 flag_bit;        // which bit to set or test

      guint size;

      GParamSpec *spec;

  } ZMapStyleParamStruct, *ZMapStyleParam;


extern ZMapStyleParamStruct zmapStyleParams_G[_STYLE_PROP_N_ITEMS];


/* We need to know whether a get/set is part of a copy or a straight get/set (in a copy
 * the get method is called for the original style and the set method for the new style. */
//#define ZMAPSTYLE_OBJ_COPY "ZMap_Style_Copy"

/* We need our get/set routines to signal whether they succeeded, this must be done via setting
 * user data on the style itself because there is nothing in the GObject interface that allows
 * us to signal this. */
#define ZMAPSTYLE_OBJ_RC "ZMap_Style_RC"


/* We use GQuarks to give each feature a unique id, the documentation doesn't say, but you
 * can surmise from the code that zero is not a valid quark. */
enum {ZMAPSTYLE_NULLQUARK = 0} ;





typedef struct _zmapFeatureTypeStyleClassStruct
{
  GObjectClass __parent__;

} zmapFeatureTypeStyleClassStruct;





/*! @} end of zmapstyles docs. */


//gboolean zmapStyleIsValid(ZMapFeatureTypeStyle style) ;

//gboolean zmapStyleBumpIsFixed(ZMapFeatureTypeStyle style) ;





#endif /* !ZMAP_STYLE_P_H */
