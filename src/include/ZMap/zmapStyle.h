/*  File: zmapStyle.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Style and Style set handling functions.
 *
 * HISTORY:
 * Last edited: Oct 28 15:32 2008 (edgrif)
 * Created: Mon Feb 26 09:28:26 2007 (edgrif)
 * CVS info:   $Id: zmapStyle.h,v 1.25 2008-10-29 16:22:58 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_STYLE_H
#define ZMAP_STYLE_H

#include <ZMap/zmapEnum.h>


#define ZMAP_STYLE_MODE_LIST(_)                                         \
_(ZMAPSTYLE_MODE_INVALID,)	/**< invalid mode */                    \
_(ZMAPSTYLE_MODE_BASIC,)        /**< Basic box features */              \
_(ZMAPSTYLE_MODE_ALIGNMENT,)    /**< Usual homology structure */        \
_(ZMAPSTYLE_MODE_TRANSCRIPT,)   /**< Usual transcript like structure */ \
_(ZMAPSTYLE_MODE_RAW_SEQUENCE,) /**< DNA Sequence */                    \
_(ZMAPSTYLE_MODE_PEP_SEQUENCE,) /**< Peptide Sequence */                \
_(ZMAPSTYLE_MODE_TEXT,)         /**< Text only display */               \
_(ZMAPSTYLE_MODE_GRAPH,)        /**< Graphs of various types */         \
_(ZMAPSTYLE_MODE_GLYPH,)        /**< Meta object controlling other features */ \
_(ZMAPSTYLE_MODE_META,)

ZMAP_DEFINE_ENUM(ZMapStyleMode, ZMAP_STYLE_MODE_LIST);


#define ZMAP_STYLE_COLUMN_DISPLAY_LIST(_)                                         \
_(ZMAPSTYLE_COLDISPLAY_INVALID,)   /**< invalid mode  */                          \
_(ZMAPSTYLE_COLDISPLAY_HIDE,)	   /**< Never show. */                            \
_(ZMAPSTYLE_COLDISPLAY_SHOW_HIDE,) /**< Show according to zoom/mag, mark etc. */  \
_(ZMAPSTYLE_COLDISPLAY_SHOW,)	   /**< Always show. */

ZMAP_DEFINE_ENUM(ZMapStyleColumnDisplayState, ZMAP_STYLE_COLUMN_DISPLAY_LIST);


#define ZMAP_STYLE_3_FRAME_LIST(_)                                                                        \
_(ZMAPSTYLE_3_FRAME_INVALID,)			  /**< invalid mode  */	                                  \
_(ZMAPSTYLE_3_FRAME_NEVER,)			  /**< Not frame sensitive.  */                           \
_(ZMAPSTYLE_3_FRAME_ALWAYS,)      		  /**< Display normally and as 3 cols in 3 frame mode. */ \
_(ZMAPSTYLE_3_FRAME_ONLY_3,)		          /**< Only dislay in 3 frame mode as 3 cols. */          \
_(ZMAPSTYLE_3_FRAME_ONLY_1,)		          /**< Only display in 3 frame mode as 1 col. */

ZMAP_DEFINE_ENUM(ZMapStyle3FrameMode, ZMAP_STYLE_3_FRAME_LIST);



/* Specifies the style of graph. */
#define ZMAP_STYLE_GRAPH_MODE_LIST(_)                                 \
_(ZMAPSTYLE_GRAPH_INVALID,)	/**< Initial setting. */              \
_(ZMAPSTYLE_GRAPH_LINE,)	/**< Just points joining a line. */   \
_(ZMAPSTYLE_GRAPH_HISTOGRAM,)	/**< Usual blocky like graph. */

ZMAP_DEFINE_ENUM(ZMapStyleGraphMode, ZMAP_STYLE_GRAPH_MODE_LIST);


/* Specifies the style of glyph. */
#define ZMAP_STYLE_GLYPH_MODE_LIST(_)                       \
_(ZMAPSTYLE_GLYPH_INVALID,)	/**< Initial setting. */    \
_(ZMAPSTYLE_GLYPH_SPLICE,)

ZMAP_DEFINE_ENUM(ZMapStyleGlyphMode, ZMAP_STYLE_GLYPH_MODE_LIST);


/* For drawing/colouring the various parts of a feature. */
#define ZMAP_STYLE_DRAW_CONTEXT_LIST(_)                           \
_(ZMAPSTYLE_DRAW_INVALID,)	/**< invalid, initial setting */  \
_(ZMAPSTYLE_DRAW_FILL,)		/**<  */                          \
_(ZMAPSTYLE_DRAW_DRAW,)		/**<  */                          \
_(ZMAPSTYLE_DRAW_BORDER,)

ZMAP_DEFINE_ENUM(ZMapStyleDrawContext, ZMAP_STYLE_DRAW_CONTEXT_LIST) ;


/* Specifies type of colour, e.g. normal or selected. */
#define ZMAP_STYLE_COLOUR_TYPE_LIST(_)        \
_(ZMAPSTYLE_COLOURTYPE_NORMAL,)	/**<  */      \
_(ZMAPSTYLE_COLOURTYPE_SELECTED,) /**<  */

ZMAP_DEFINE_ENUM(ZMapStyleColourType, ZMAP_STYLE_COLOUR_TYPE_LIST) ;


/* Specifies the target type of the colour. */
#define ZMAP_STYLE_COLOUR_TARGET_LIST(_)                        \
_(ZMAPSTYLE_COLOURTARGET_NORMAL,) /* Normal colour */           \
_(ZMAPSTYLE_COLOURTARGET_FRAME0,) /* Frame 1 colour */          \
_(ZMAPSTYLE_COLOURTARGET_FRAME1,) /* Frame 2 colour */          \
_(ZMAPSTYLE_COLOURTARGET_FRAME2,) /* Frame 3 colour */          \
_(ZMAPSTYLE_COLOURTARGET_CDS,)	  /* Colour to apply to CDS */  \
_(ZMAPSTYLE_COLOURTARGET_STRAND,) /* Colour to apply to Strand */

ZMAP_DEFINE_ENUM(ZMapStyleColourTarget, ZMAP_STYLE_COLOUR_TARGET_LIST) ;


/* Specifies how wide features should be in relation to their score. */
#define ZMAP_STYLE_SCORE_MODE_LIST(_)                                     \
_(ZMAPSCORE_WIDTH,)		/* Use column width only - default. */    \
_(ZMAPSCORE_OFFSET,)                                                      \
_(ZMAPSCORE_HISTOGRAM,)                                                   \
_(ZMAPSCORE_PERCENT,)

ZMAP_DEFINE_ENUM(ZMapStyleScoreMode, ZMAP_STYLE_SCORE_MODE_LIST) ;


/* Specifies how features in columns should be overlapped for compact display. */
#define ZMAP_STYLE_OVERLAP_MODE_LIST(_)                                              \
_(ZMAPOVERLAP_INVALID,)                                                              \
_(ZMAPOVERLAP_COMPLETE,)	/* draw on top - default */                          \
_(ZMAPOVERLAP_OVERLAP,)		/* bump if feature coords overlap. */                \
_(ZMAPOVERLAP_POSITION,)        /* bump if features start at same coord. */          \
_(ZMAPOVERLAP_NAME,)		/* one column per homol target */                    \
_(ZMAPOVERLAP_OSCILLATE,)       /* Oscillate between one column and another...       \ 
				   Useful for displaying tile paths. */              \
_(ZMAPOVERLAP_ITEM_OVERLAP,)    /* bump if item coords overlap in canvas space... */ \
_(ZMAPOVERLAP_SIMPLE,)	        /* one column per feature, for testing... */         \
_(ZMAPOVERLAP_ENDS_RANGE,)	/* Sort by 5' and 3' best/biggest                    \
				   matches, one match per column, very               \
				   fmap like but better... */                        \
_(ZMAPOVERLAP_COMPLEX_INTERLEAVE,)/* all features with same name in a                  \
				   single sub-column, several names in one               \
				   column but no 2 features overlap. */              \
_(ZMAPOVERLAP_COMPLEX_NO_INTERLEAVE,)	/* as ZMAPOVERLAP_COMPLEX but no interleaving of \
					   features with different names. */		\
_(ZMAPOVERLAP_COMPLEX_RANGE,)	/* All features with same name in a                  \
				   single column if they overlap the                 \
				   'mark' range, all other features in                \
				   a single column.  */                              \
  _(ZMAPOVERLAP_COMPLEX_LIMIT,)	/* As ZMAPOVERLAP_COMPLEX_RANGE but constrain matches \
				   to be colinear within range or are truncated. */ 


ZMAP_DEFINE_ENUM(ZMapStyleOverlapMode, ZMAP_STYLE_OVERLAP_MODE_LIST) ;

#define ZMAPOVERLAP_START ZMAPOVERLAP_COMPLETE
#define ZMAPOVERLAP_END ZMAPOVERLAP_COMPLEX_LIMIT


/* Note the naming here in the macros. ZMAP_TYPE_FEATURE_TYPE_STYLE seemed confusing... */

#define ZMAP_TYPE_FEATURE_STYLE           (zMapFeatureTypeStyleGetType())
#define ZMAP_FEATURE_STYLE(obj)	          (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_FEATURE_STYLE, zmapFeatureTypeStyle))
#define ZMAP_FEATURE_STYLE_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_FEATURE_STYLE, zmapFeatureTypeStyle const))
#define ZMAP_FEATURE_STYLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_FEATURE_STYLE, zmapFeatureTypeStyleClass))
#define ZMAP_IS_FEATURE_STYLE(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_FEATURE_STYLE))
#define ZMAP_FEATURE_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_FEATURE_STYLE, zmapFeatureTypeStyleClass))

/* Instance */
typedef struct _zmapFeatureTypeStyleStruct *ZMapFeatureTypeStyle;

typedef struct _zmapFeatureTypeStyleStruct  zmapFeatureTypeStyle;

/* Class */
typedef struct _zmapFeatureTypeStyleClassStruct *ZMapFeatureTypeStyleClass;

typedef struct _zmapFeatureTypeStyleClassStruct  zmapFeatureTypeStyleClass;

typedef struct 
{
  char *fill;
  char *draw;
  char *border;
} ZMapStyleColourStringsStruct, *ZMapStyleColourStrings;

/* Public funcs */
GType zMapFeatureTypeStyleGetType(void);




/* Enum -> String function decs: const char *zMapStyleXXXXMode2Str(ZMapStyleXXXXXMode mode);  */
ZMAP_ENUM_AS_STRING_DEC(zMapStyleMode2Str,            ZMapStyleMode) ;
ZMAP_ENUM_AS_STRING_DEC(zmapStyleColDisplayState2Str, ZMapStyleColumnDisplayState) ;
ZMAP_ENUM_AS_STRING_DEC(zmapStyle3FrameMode2Str, ZMapStyle3FrameMode) ;
ZMAP_ENUM_AS_STRING_DEC(zmapStyleGraphMode2Str,       ZMapStyleGraphMode) ;
ZMAP_ENUM_AS_STRING_DEC(zmapStyleGlyphMode2Str,       ZMapStyleGlyphMode) ;
ZMAP_ENUM_AS_STRING_DEC(zmapStyleDrawContext2Str,     ZMapStyleDrawContext) ;
ZMAP_ENUM_AS_STRING_DEC(zmapStyleColourType2Str,      ZMapStyleColourType) ;
ZMAP_ENUM_AS_STRING_DEC(zmapStyleColourTarget2Str,    ZMapStyleColourTarget) ;
ZMAP_ENUM_AS_STRING_DEC(zmapStyleScoreMode2Str,       ZMapStyleScoreMode) ;
ZMAP_ENUM_AS_STRING_DEC(zmapStyleOverlapMode2Str,     ZMapStyleOverlapMode) ;


ZMapFeatureTypeStyle zMapStyleCreate(char *name, char *description) ;
ZMapFeatureTypeStyle zMapFeatureStyleCopy(ZMapFeatureTypeStyle src);
gboolean zMapStyleCCopy(ZMapFeatureTypeStyle src, ZMapFeatureTypeStyle *dest_out);
void zMapStyleDestroy(ZMapFeatureTypeStyle style);


gboolean zMapStyleNameCompare(ZMapFeatureTypeStyle style, char *name) ;
gboolean zMapStyleIsTrueFeature(ZMapFeatureTypeStyle style) ;
GQuark zMapStyleGetID(ZMapFeatureTypeStyle style) ;
GQuark zMapStyleGetUniqueID(ZMapFeatureTypeStyle style) ;
char *zMapStyleGetDescription(ZMapFeatureTypeStyle style) ;

gboolean zMapStyleIsDrawable(ZMapFeatureTypeStyle style, GError **error) ;
gboolean zMapStyleMakeDrawable(ZMapFeatureTypeStyle style) ;

gboolean zMapStyleGetColoursCDSDefault(ZMapFeatureTypeStyle style, 
				       GdkColor **background, GdkColor **foreground, GdkColor **outline) ;
gboolean zMapStyleIsColour(ZMapFeatureTypeStyle style, ZMapStyleDrawContext colour_context) ;
gboolean zMapStyleIsBackgroundColour(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsForegroundColour(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsOutlineColour(ZMapFeatureTypeStyle style) ;

gboolean zMapStyleSetColours(ZMapFeatureTypeStyle style, ZMapStyleColourTarget target, ZMapStyleColourType type,
			     char *fill, char *draw, char *border) ;
gboolean zMapStyleGetColours(ZMapFeatureTypeStyle style, ZMapStyleColourTarget target, ZMapStyleColourType type,
			     GdkColor **fill, GdkColor **draw, GdkColor **border) ;
gboolean zMapStyleColourByStrand(ZMapFeatureTypeStyle style);

ZMapStyleMode zMapStyleGetMode(ZMapFeatureTypeStyle style) ;



void zMapStyleSetGFF(ZMapFeatureTypeStyle style, char *gff_source, char *gff_feature) ;
char *zMapStyleGetGFFSource(ZMapFeatureTypeStyle style) ;
char *zMapStyleGetGFFFeature(ZMapFeatureTypeStyle style) ;

gboolean zMapStyleIsDirectionalEnd(ZMapFeatureTypeStyle style) ;

void zMapStyleSetDisplayable(ZMapFeatureTypeStyle style, gboolean displayable) ;
gboolean zMapStyleIsDisplayable(ZMapFeatureTypeStyle style) ;

void zMapStyleSetDeferred(ZMapFeatureTypeStyle style, gboolean displayable) ;
gboolean zMapStyleIsDeferred(ZMapFeatureTypeStyle style) ;
void zMapStyleSetLoaded(ZMapFeatureTypeStyle style, gboolean deferred) ;
gboolean zMapStyleIsLoaded(ZMapFeatureTypeStyle style) ;

void zMapStyleSetDisplay(ZMapFeatureTypeStyle style, ZMapStyleColumnDisplayState col_show) ;
ZMapStyleColumnDisplayState zMapStyleGetDisplay(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsHidden(ZMapFeatureTypeStyle style) ;

void zMapStyleSetShowWhenEmpty(ZMapFeatureTypeStyle style, gboolean show_when_empty) ;
gboolean zMapStyleGetShowWhenEmpty(ZMapFeatureTypeStyle style);

void zMapStyleSetStrandSpecific(ZMapFeatureTypeStyle type, gboolean strand_specific) ;
void zMapStyleSetStrandShowReverse(ZMapFeatureTypeStyle type, gboolean show_reverse) ;
void zMapStyleSetFrameMode(ZMapFeatureTypeStyle type, ZMapStyle3FrameMode frame_mode) ;
void zMapStyleGetStrandAttrs(ZMapFeatureTypeStyle type,
			     gboolean *strand_specific, gboolean *show_rev_strand, ZMapStyle3FrameMode *frame_mode) ;
gboolean zMapStyleIsStrandSpecific(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsShowReverseStrand(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsFrameSpecific(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsFrameOneColumn(ZMapFeatureTypeStyle style) ;

double zMapStyleGetWidth(ZMapFeatureTypeStyle style) ;
double zMapStyleGetMaxScore(ZMapFeatureTypeStyle style) ;
double zMapStyleGetMinScore(ZMapFeatureTypeStyle style) ;

double zMapStyleBaseline(ZMapFeatureTypeStyle style) ;

gboolean zMapStyleIsMinMag(ZMapFeatureTypeStyle style, double *min_mag) ;
gboolean zMapStyleIsMaxMag(ZMapFeatureTypeStyle style, double *max_mag) ;
void zMapStyleSetMag(ZMapFeatureTypeStyle style, double min_mag, double max_mag) ;
double zMapStyleGetMinMag(ZMapFeatureTypeStyle style) ;
double zMapStyleGetMaxMag(ZMapFeatureTypeStyle style) ;


void zMapStyleSetPfetch(ZMapFeatureTypeStyle style, gboolean pfetchable) ;
gboolean zMapStyleGetPfetch(ZMapFeatureTypeStyle style) ;


/* Lets change all these names to just be zmapStyle, i.e. lose the featuretype bit..... */

ZMapFeatureTypeStyle zMapFeatureTypeCreate(char *name, char *description) ;

void zMapStyleSetDescription(ZMapFeatureTypeStyle style, char *description) ;
void zMapStyleSetParent(ZMapFeatureTypeStyle style, char *parent_name) ;
void zMapStyleSetMode(ZMapFeatureTypeStyle style, ZMapStyleMode mode) ;
gboolean zMapStyleHasMode(ZMapFeatureTypeStyle style);
ZMapStyleMode zMapStyleGetMode(ZMapFeatureTypeStyle style) ;
void zMapStyleSetWidth(ZMapFeatureTypeStyle style, double width) ;
double zMapStyleGetWidth(ZMapFeatureTypeStyle style) ;
void zMapStyleSetBumpWidth(ZMapFeatureTypeStyle style, double bump_width) ;
double zMapStyleGetBumpWidth(ZMapFeatureTypeStyle style) ;

gboolean zMapStyleFormatMode(char *mode_str, ZMapStyleMode *mode_out) ;



void zMapStyleSetScore(ZMapFeatureTypeStyle style, double min_score, double max_score) ;
void zMapStyleSetGraph(ZMapFeatureTypeStyle style, ZMapStyleGraphMode mode, double min, double max, double baseline) ;
void zMapStyleSetEndStyle(ZMapFeatureTypeStyle style, gboolean directional) ;

void zMapStyleSetGappedAligns(ZMapFeatureTypeStyle style, gboolean parse_gaps, unsigned int within_align_error) ;
gboolean zMapStyleGetGappedAligns(ZMapFeatureTypeStyle style, unsigned int *within_align_error) ;
void zMapStyleSetJoinAligns(ZMapFeatureTypeStyle style, unsigned int between_align_error) ;
gboolean zMapStyleGetJoinAligns(ZMapFeatureTypeStyle style, unsigned int *between_align_error) ;
gboolean zMapStyleGetParseGaps(ZMapFeatureTypeStyle style) ;
unsigned int zmapStyleGetWithinAlignError(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsParseGaps(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsAlignGaps(ZMapFeatureTypeStyle style) ;
void zMapStyleSetAlignGaps(ZMapFeatureTypeStyle style, gboolean show_gaps) ;

void zMapStyleSetGlyphMode(ZMapFeatureTypeStyle style, ZMapStyleGlyphMode glyph_mode) ;

char *zMapStyleCreateName(char *style_name) ;
GQuark zMapStyleCreateID(char *style_name) ;
char *zMapStyleGetName(ZMapFeatureTypeStyle style) ;
GQuark zMapStyleGetID(ZMapFeatureTypeStyle style) ;

void zMapFeatureTypeDestroy(ZMapFeatureTypeStyle type) ;
ZMapFeatureTypeStyle zMapStyleGetPredefined(char *style_name) ;
gboolean zMapFeatureTypeSetAugment(GData **current, GData **new) ;

void zMapStyleInitOverlapMode(ZMapFeatureTypeStyle style, 
			      ZMapStyleOverlapMode default_overlap_mode, ZMapStyleOverlapMode curr_overlap_mode) ;
void zMapStyleSetBumpSpace(ZMapFeatureTypeStyle style, double bump_spacing) ;
void zMapStyleSetBump(ZMapFeatureTypeStyle type, char *bump) ;
double zMapStyleGetBumpSpace(ZMapFeatureTypeStyle style) ;
ZMapStyleOverlapMode zMapStyleGetOverlapMode(ZMapFeatureTypeStyle style) ;
void zMapStyleSetOverlapMode(ZMapFeatureTypeStyle style, ZMapStyleOverlapMode overlap_mode) ;
ZMapStyleOverlapMode zMapStyleResetOverlapMode(ZMapFeatureTypeStyle style) ;
ZMapStyleOverlapMode zMapStyleGetDefaultOverlapMode(ZMapFeatureTypeStyle style);

ZMapFeatureTypeStyle zMapFeatureStyleCopy(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleMerge(ZMapFeatureTypeStyle curr_style, ZMapFeatureTypeStyle new_style) ;

unsigned int zmapStyleGetWithinAlignError(ZMapFeatureTypeStyle style) ;


GData *zMapFeatureTypeGetFromFile(char *styles_list, char *styles_file) ;

gboolean zMapStyleDisplayInSeparator(ZMapFeatureTypeStyle style);

/* Style set functions... */

gboolean zMapStyleSetAdd(GData **style_set, ZMapFeatureTypeStyle style) ;
gboolean zMapStyleInheritAllStyles(GData **style_set) ;
gboolean zMapStyleNameExists(GList *style_name_list, char *style_name) ;
ZMapFeatureTypeStyle zMapFindStyle(GData *styles, GQuark style_id) ;
GList *zMapStylesGetNames(GData *styles) ;
GData *zMapStyleGetAllPredefined(void) ;
GData *zMapStyleMergeStyles(GData *curr_styles, GData *new_styles) ;
void zMapStyleDestroyStyles(GData **styles) ;

/* Debug functions. */

void zMapStylePrint(ZMapFeatureTypeStyle style, char *prefix, gboolean full) ;
void zMapFeatureTypePrintAll(GData *type_set, char *user_string) ;
void zMapFeatureStylePrintAll(GList *styles, char *user_string) ;




#endif /* ZMAP_STYLE_H */
