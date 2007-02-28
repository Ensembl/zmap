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
 * Last edited: Feb 26 16:06 2007 (edgrif)
 * Created: Mon Feb 26 09:28:26 2007 (edgrif)
 * CVS info:   $Id: zmapStyle.h,v 1.1 2007-02-28 18:14:56 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_STYLE_H
#define ZMAP_STYLE_H


/* The opaque struct representing a style. */
typedef struct ZMapFeatureTypeStyleStruct_ *ZMapFeatureTypeStyle ;




/* Specifies how features that reference this style will be processed. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
typedef enum
  {
    ZMAPSTYLE_MODE_INVALID,				    /* invalid this is bad. */
    ZMAPSTYLE_MODE_NONE,
    ZMAPSTYLE_MODE_META,                                    /* Feature to be processed as meta */
    ZMAPSTYLE_MODE_BASIC,				    /* Basic box features. */
    ZMAPSTYLE_MODE_TRANSCRIPT,				    /* Usual transcript like structure. */
    ZMAPSTYLE_MODE_ALIGNMENT,				    /* Usual homology structure. */
    ZMAPSTYLE_MODE_TEXT,				    /* Text only display. */
    ZMAPSTYLE_MODE_GRAPH				    /* Graphs of various types. */
  } ZMapStyleMode ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#define ZMAP_MODE_METADEFS(DEF_MACRO) \
  DEF_MACRO(ZMAPSTYLE_MODE_INVALID)\
  DEF_MACRO(ZMAPSTYLE_MODE_NONE)\
  DEF_MACRO(ZMAPSTYLE_MODE_META)\
  DEF_MACRO(ZMAPSTYLE_MODE_BASIC)\
  DEF_MACRO(ZMAPSTYLE_MODE_TRANSCRIPT)\
  DEF_MACRO(ZMAPSTYLE_MODE_ALIGNMENT)\
  DEF_MACRO(ZMAPSTYLE_MODE_TEXT)\
  DEF_MACRO(ZMAPSTYLE_MODE_GRAPH)



#define ZMAP_MODE_ENUM( Name ) Name,

typedef enum
{
  ZMAP_MODE_METADEFS( ZMAP_MODE_ENUM )
} ZMapStyleMode ;



/* For drawing/colouring the various parts of a feature. */
typedef enum
  {
    ZMAPSTYLE_DRAW_INVALID,
    ZMAPSTYLE_DRAW_BACKGROUND,
    ZMAPSTYLE_DRAW_FOREGROUND,
    ZMAPSTYLE_DRAW_OUTLINE
  } ZMapStyleDrawContext ;


GQuark zMapStyleGetID(ZMapFeatureTypeStyle style) ;
GQuark zMapStyleGetUniqueID(ZMapFeatureTypeStyle style) ;
char *zMapStyleGetDescription(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsColour(ZMapFeatureTypeStyle style, ZMapStyleDrawContext colour_context) ;
GdkColor *zMapStyleGetColour(ZMapFeatureTypeStyle style, ZMapStyleDrawContext colour_context) ;
ZMapStyleMode zMapStyleGetMode(ZMapFeatureTypeStyle style) ;
const char *zMapStyleMode2Str(ZMapStyleMode mode) ;
char *zMapStyleGetGFFSource(ZMapFeatureTypeStyle style) ;
char *zMapStyleGetGFFFeature(ZMapFeatureTypeStyle style) ;
unsigned int zmapStyleGetWithinAlignError(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsParseGaps(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsDirectionalEnd(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsAlignGaps(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsFrameSpecific(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsHiddenAlways(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsHiddenNow(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsStrandSpecific(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsShowReverseStrand(ZMapFeatureTypeStyle style) ;
double zMapStyleGetWidth(ZMapFeatureTypeStyle style) ;
double zMapStyleGetMaxScore(ZMapFeatureTypeStyle style) ;
double zMapStyleGetMinScore(ZMapFeatureTypeStyle style) ;
double zMapStyleBaseline(ZMapFeatureTypeStyle style) ;
double zMapStyleGetMinMag(ZMapFeatureTypeStyle style) ;
  double zMapStyleGetMaxMag(ZMapFeatureTypeStyle style) ;

#endif /* ZMAP_STYLE_H */
