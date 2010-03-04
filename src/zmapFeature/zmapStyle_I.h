/*  File: zmapStyle_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * Description: Private header for style.
 *
 * HISTORY:
 * Last edited: Jul 29 09:43 2009 (edgrif)
 * Created: Mon Feb 26 09:13:30 2007 (edgrif)
 * CVS info:   $Id: zmapStyle_I.h,v 1.16 2010-03-04 15:10:33 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_STYLE_I_H__
#define __ZMAP_STYLE_I_H__


/* TEMP....WHILE I MOVE STYLE STUFF IN HERE.... */
#include <ZMap/zmapFeature.h>

#include <ZMap/zmapStyle.h>
#include <zmapBase_I.h>



/* We need to know whether a get/set is part of a copy or a straight get/set (in a copy
 * the get method is called for the original style and the set method for the new style. */
#define ZMAPSTYLE_OBJ_COPY "ZMap_Style_Copy"

/* We need out get/set routines to signal whether they succeeded, this must be done via setting
 * user data on the style itself because there is nothing in the GObject interface that allows
 * us to signal this. */
#define ZMAPSTYLE_OBJ_RC "ZMap_Style_RC"





typedef struct _zmapFeatureTypeStyleClassStruct
{
  zmapBaseClass __parent__;

} zmapFeatureTypeStyleClassStruct;



/*! @addtogroup zmapstyles
 * @{
 *  */


/*! @struct ZMapStyleColour zmapStyle_P.h
 *  @brief ZMap object colours
 *
 * All ZMap objects can potentially have a border colour, a fill colour and a draw colour
 * which can be used to "draw" over the fill colour. */

typedef struct
{
  struct
  {
    unsigned int draw : 1 ;
    unsigned int fill : 1 ;
    unsigned int border : 1 ;
  } fields_set ;

  GdkColor  draw ;					    /* Overlaid on fill colour. */
  GdkColor  fill ;					    /* Fill/background colour. */
  GdkColor  border ;					    /* Surround/line colour. */
} ZMapStyleColourStruct, *ZMapStyleColour ;


/*! @struct ZMapStyleFullColour zmapStyle_P.h
 *  @brief ZMap feature colours
 *
 * All features in ZMap can be selected and hence must have both "normal" and "selected"
 * colours. */
typedef struct
{
  ZMapStyleColourStruct normal ;
  ZMapStyleColourStruct selected ;
} ZMapStyleFullColourStruct, *ZMapStyleFullColour ;

/* At most 6 colour specs can be given for any one field (i.e. all the combinations of selected,
 * normal and draw, fill, border. */
enum {ZMAPSTYLE_MAX_COLOUR_SPECS = 6} ;



/*! Styles have different modes, e.g. graph, alignment etc, information specific to a particular
 * style is held in its own struct. */


/*! @struct ZMapBasicGraph zmapStyle_P.h
 *  @brief Basic feature
 *
 * < currently this is empty > */
typedef struct
{
  char *dummy ;

} ZMapStyleBasicStruct, *ZMapStyleBasic ;

/*! @struct ZMapSequenceGraph zmapStyle_P.h
 *  @brief Sequence feature
 *
 * (currently this is empty) */
typedef struct
{
  char *dummy ;

} ZMapStyleSequenceStruct, *ZMapStyleSequence ;

/*! @struct ZMapTextGraph zmapStyle_P.h
 *  @brief Text feature
 *
 * (currently this is empty) */
typedef struct
{
  struct
  {
    unsigned int font : 1;
  } fields_set;

  char *font;

} ZMapStyleTextStruct, *ZMapStyleText ;



/*! @struct ZMapStyleGraph zmapStyle_P.h
 *  @brief Graph feature
 *
 * Draws a feature as a graph, the feature must contain graph points. */
typedef struct
{
  struct
  {
    unsigned int mode : 1 ;
    unsigned int baseline : 1 ;
  } fields_set ;					    /*!< Fields set.  */

  ZMapStyleGraphMode mode ;				    /*!< Graph style. */

  double baseline ;					    /*!< zero level for graph.  */

} ZMapStyleGraphStruct, *ZMapStyleGraph ;


/*! @struct ZMapStyleGlyph zmapStyle_P.h
 *  @brief Glyph feature
 *
 * Draws shapes of various kinds, e.g. splice site indicators etc. */
typedef struct
{
  struct
  {
    unsigned int mode : 1 ;
    unsigned int type : 1 ;
  } fields_set ;					    /*!< Fields set.  */

  ZMapStyleGlyphMode mode ;				    /*!< Glyph mode. eg splice or marker*/
  ZMapStyleGlyphMode type ;                         /*!< Glyph type. eg diamond or circle */

} ZMapStyleGlyphStruct, *ZMapStyleGlyph ;


/*! @struct ZMapStyleAlignment zmapStyle_P.h
 *  @brief Alignment feature
 *
 * Draws an alignment as a series of blocks joined by straight lines. The lines can be coloured
 * to indicate colinearity between adjacent blocks. */
typedef struct
 {
   struct
   {
     unsigned int parse_gaps      : 1 ;
     unsigned int show_gaps       : 1 ;
     unsigned int between_align_error : 1 ;
     unsigned int allow_misalign : 1 ;
     unsigned int pfetchable : 1 ;
     unsigned int blixem : 1 ;
     unsigned int incomplete_glyph : 1;
   } fields_set ;						    /*!< Fields set.  */

  /*! Allowable align errors, used to decide whether a match should be classified as "perfect".
   *  between_align_error   is used to assess several alignments (e.g. for exon matches) if join_homols = TRUE
   *
   * Number is allowable number of missing bases between blocks/alignments, default is 0. */
   unsigned int between_align_error ;

   /* If set then blixem will be run with nucleotide or peptide sequences for the features. */
   ZMapStyleBlixemType blixem_type ;

   /*! Colours for bars joining up intra/inter alignment gaps. */
   ZMapStyleFullColourStruct perfect ;
   ZMapStyleFullColourStruct colinear ;
   ZMapStyleFullColourStruct noncolinear ;

   /*! glyph type and colours for markimng incomplete ends */
   ZMapStyleGlyphType incomplete_glyph_type;
   ZMapStyleFullColourStruct incomplete_glyph_colour ;

   /* State for alignments. */
   struct
   {
     unsigned int pfetchable : 1 ;			    /* TRUE => alignments have pfetch entries. */
     unsigned int parse_gaps : 1 ;
     unsigned int show_gaps : 1 ;			    /*!< TRUE: gaps within alignment are displayed,
							      FALSE: alignment is displayed as a single block. */
     unsigned int allow_misalign : 1 ;			    /* TRUE => ref and match sequences
							       don't have to be exactly same
							       length, ref coords dominate. */
   } state ;

} ZMapStyleAlignmentStruct, *ZMapStyleAlignment ;


/*! @struct ZMapStyleTranscript zmapStyle_P.h
 *  @brief Transcript feature
 *
 * Draws a transcript as a series of boxes joined by angled lines. */
typedef struct
{
  struct
  {
    unsigned int unused : 1 ;
  } fields_set ;					    /*!< Fields set.  */


  ZMapStyleFullColourStruct CDS_colours ;		    /*!< Colour for CDS part of feature. */

} ZMapStyleTranscriptStruct, *ZMapStyleTranscript ;


/*! @struct ZMapStyleAssemblyPath zmapStyle_P.h
 *  @brief AssemblyPath feature
 *
 * Draws an assembly path as a series of boxes placed alternately to form a tiling path. */
typedef struct
{
  struct
  {
    unsigned int unused : 1 ;
  } fields_set ;					    /*!< Fields set.  */


  ZMapStyleFullColourStruct non_path_colours ;		    /*!< Colour for non-assembly part of feature. */

} ZMapStyleAssemblyPathStruct, *ZMapStyleAssemblyPath ;


/* THIS STRUCT NEEDS A MAGIC PTR, ONCE IT HAS ONE THEN ADD A TEST TO zmapStyleIsValid() FOR IT.... */

/*! @struct ZMapFeatureTypeStyle zmapStyle_P.h
 *  @brief ZMap Style
 *
 * ZMap Style definition, the style must have a mode which specifies what sort of
 * of feature the style represents. */
typedef struct _zmapFeatureTypeStyleStruct
{
  zmapBase __parent__;

  /*! _All_ styles must have these fields set, no other fields are compulsory. */
  GQuark original_id ;					    /*!< Original name. */
  GQuark unique_id ;					    /*!< Name normalised to be unique. */


  /*! Since all these fields are optional we need flags for all of them to show whether they were
   * set. N.B. these fields should _not_ be used for checking the state of the style, _only_
   * to see if a field has been set or not. (note also that sub structs, e.g. colours, have
   * their own flags.) */
  struct
  {
    unsigned int parent_id : 1 ;

    unsigned int description : 1 ;

    unsigned int mode : 1 ;

    /* Colours flags are in the colour structs. */

    unsigned int col_display_state : 1 ;

    unsigned int default_bump_mode : 1 ;
    unsigned int curr_bump_mode : 1 ;
    unsigned int bump_fixed : 1 ;
    unsigned int bump_spacing : 1 ;

    unsigned int min_mag : 1 ;
    unsigned int max_mag : 1 ;

    unsigned int width : 1 ;

    unsigned int score_mode : 1 ;
    unsigned int min_score : 1 ;
    unsigned int max_score : 1 ;

    unsigned int gff_source : 1 ;
    unsigned int gff_feature : 1 ;

    unsigned int displayable     : 1 ;

    unsigned int show_when_empty : 1 ;

    unsigned int showText        : 1 ;





    unsigned int strand_specific : 1 ;
    unsigned int show_rev_strand : 1 ;
    unsigned int frame_mode : 1 ;

    unsigned int show_only_in_separator : 1;

    unsigned int directional_end : 1 ;

    unsigned int deferred : 1 ;
    unsigned int loaded : 1 ;

  } fields_set ;



  /*! Data fields for the style. */


  GQuark parent_id ;					    /*!< Styles can inherit from other
							       styles, the parent style _must_ be
							       identified by its unique id. */

  char *description ;					    /*!< Description of what this style
							       represents. */

  ZMapStyleMode mode ;					    /*!< Specifies how features that
							       reference this style will be processed. */

  ZMapStyleMode implied_mode;	/* This is necessary for the
				 * inheritance and correct access of
				 * the mode_data union. See set_implied_mode() */

  ZMapStyleFullColourStruct colours ;			    /*!< Main feature colours. */

  /*! Colours for when feature is shown in frames. */
  ZMapStyleFullColourStruct frame0_colours ;
  ZMapStyleFullColourStruct frame1_colours ;
  ZMapStyleFullColourStruct frame2_colours ;

  /*! Colours for when feature is shown stranded by colour  */
  ZMapStyleFullColourStruct strand_rev_colours;


  ZMapStyleColumnDisplayState col_display_state ;	    /* Controls how/when col is displayed. */

  ZMapStyleBumpMode default_bump_mode ;		    /*!< Allows return to original bump mode. */
  ZMapStyleBumpMode curr_bump_mode ;		    /*!< Controls how features are grouped
							       into sub columns within a column. */
  double bump_spacing ;					    /*!< gap between bumped features. */

  ZMapStyle3FrameMode frame_mode ;			    /*!< Controls how frame sensitive
							      features are displayed. */

  double min_mag ;					    /*!< Don't display if fewer bases/line */
  double max_mag ;					    /*!< Don't display if more bases/line */

  double width ;					    /*!< column width */

  ZMapStyleScoreMode score_mode ;			    /*!< Controls width of features that
							       have scores. */
  double min_score, max_score ;				    /*!< Min/max for score width calc. */


  /*! GFF feature dumping, allows specifying of source/feature types independently of feature
   * attributes. */
  GQuark gff_source ;
  GQuark gff_feature ;


  /*! State information for the style. */
  struct
  {
    unsigned int displayable     : 1 ;			    /* FALSE means never, ever display,
							       for TRUE see col_display_state. */


    unsigned int show_when_empty : 1 ;			    /*!< If FALSE, features' column is
							       displayed even if there are no features. */
    unsigned int bump_fixed      : 1 ;			    /*!< If TRUE then bump mode cannot be changed.  */

    unsigned int showText        : 1 ;			    /*!< Should feature text be displayed. */

    /*! Strand, show reverse and frame are all linked: something that is frame specific must be
     * strand specific as well.... */
    unsigned int strand_specific : 1 ;			    /*!< Feature that is on one strand of the dna. */
    unsigned int show_rev_strand : 1 ;			    /*!< Only display the feature on the
							       reverse strand if this is set. */
    unsigned int show_only_in_separator : 1;

    unsigned int directional_end : 1 ;			    /*!< Display pointy ends on exons etc. */

    unsigned int deferred : 1; 	/* flag for to say if this style is deferred loaded */

    unsigned int loaded : 1;	/* flag to say if we're loaded */
  } opts ;

  /*! Mode specific fields, see docs for individual structs. */
  union
  {
    ZMapStyleBasicStruct basic ;
    ZMapStyleSequenceStruct sequence ;
    ZMapStyleTextStruct text ;
    ZMapStyleTranscriptStruct transcript ;
    ZMapStyleAssemblyPathStruct assembly_path ;
    ZMapStyleAlignmentStruct alignment ;
    ZMapStyleGraphStruct graph ;
    ZMapStyleGlyphStruct glyph ;
  } mode_data ;


} zmapFeatureTypeStyleStruct ;





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Enum -> String Decs */
/* const char *zMapStyleMode2Str(ZMapStyleMode mode); */
ZMAP_ENUM_AS_STRING_DEC(zmapStyleMode2Str,            ZMapStyleMode);
ZMAP_ENUM_AS_STRING_DEC(zmapStyleColDisplayState2Str, ZMapStyleColumnDisplayState);
ZMAP_ENUM_AS_STRING_DEC(zmapStyle3FrameMode2Str, ZMapStyle3FrameMode) ;
ZMAP_ENUM_AS_STRING_DEC(zmapStyleGraphMode2Str,       ZMapStyleGraphMode);
ZMAP_ENUM_AS_STRING_DEC(zmapStyleGlyphMode2Str,       ZMapStyleGlyphMode);
ZMAP_ENUM_AS_STRING_DEC(zmapStyleDrawContext2Str,     ZMapStyleDrawContext);
ZMAP_ENUM_AS_STRING_DEC(zmapStyleColourType2Str,      ZMapStyleColourType);
ZMAP_ENUM_AS_STRING_DEC(zmapStyleColourTarget2Str,    ZMapStyleColourTarget);
ZMAP_ENUM_AS_STRING_DEC(zmapStyleScoreMode2Str,       ZMapStyleScoreMode);
ZMAP_ENUM_AS_STRING_DEC(zmapStyleBumpMode2Str,     ZMapStyleBumpMode);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/*! @} end of zmapstyles docs. */


gboolean zmapStyleIsValid(ZMapFeatureTypeStyle style) ;

gboolean zmapStyleBumpIsFixed(ZMapFeatureTypeStyle style) ;





#endif /* !ZMAP_STYLE_P_H */
