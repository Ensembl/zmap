/*  File: zmapStyle_P.h
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
 * Description: Private header for style.
 *
 * HISTORY:
 * Last edited: Mar 21 13:32 2007 (edgrif)
 * Created: Mon Feb 26 09:13:30 2007 (edgrif)
 * CVS info:   $Id: zmapStyle_P.h,v 1.3 2007-03-28 16:34:38 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_STYLE_P_H
#define ZMAP_STYLE_P_H



/* TEMP....WHILE I MOVE STYLE STUFF IN HERE.... */
#include <ZMap/zmapFeature.h>


#include <ZMap/zmapStyle.h>


/* Substructs within a style. */

typedef struct
{
  struct
  {
    unsigned int draw : 1 ;
    unsigned int fill : 1 ;
    unsigned int border : 1 ;
  } fields_set ;

  GdkColor  draw ;					    /* Overlaid on background. */
  GdkColor  fill ;					    /* Fill colour. */
  GdkColor  border ;					    /* Surround/line colour. */
} ZMapStyleColourStruct, *ZMapStyleColour ;


typedef struct
{
  ZMapStyleColourStruct normal ;
  ZMapStyleColourStruct selected ;
} ZMapStyleFullColourStruct, *ZMapStyleFullColour ;


/* Styles have different modes, e.g. graph, alignment etc, information specific to a particular
 * style is held in its own struct. */

typedef struct
{
  struct
  {
    unsigned int mode : 1 ;
    unsigned int baseline : 1 ;
  } fields_set ;

  ZMapStyleGraphMode mode ;				    /* Says how to draw a graph. */

  double baseline ;

} ZMapStyleGraphStruct, *ZMapStyleGraph ;


typedef struct
{
  struct
  {
    unsigned int unused : 1 ;
  } fields_set ;

  ZMapStyleGlyphMode mode ;				    /* Says how to draw a graph. */

} ZMapStyleGlyphStruct, *ZMapStyleGlyph ;


typedef struct
{
  struct
  {
    unsigned int within_align_error : 1 ;
    unsigned int between_align_error : 1 ;
  } fields_set ;

  /* Allowable align errors, used to decide whether a match should be classified as "perfect".
   *   within_align_error   is used to assess the blocks in a single gapped alignment if align_gaps = TRUE
   *  between_align_error   is used to assess several alignments (e.g. for exon matches) if join_homols = TRUE
   * 
   * Number is allowable number of missing bases between blocks/alignments, default is 0. */
  unsigned int within_align_error ;
  unsigned int between_align_error ;

  /* Colours for bars joining up intra/inter alignment gaps. */
  ZMapStyleFullColourStruct perfect ;
  ZMapStyleFullColourStruct colinear ;
  ZMapStyleFullColourStruct noncolinear ;

} ZMapStyleAlignmentStruct, *ZMapStyleAlignment ;


typedef struct
{
  struct
  {
    unsigned int unused : 1 ;
  } fields_set ;


  ZMapStyleFullColourStruct CDS_colours ;		    /* Colour for CDS part of feature. */


} ZMapStyleTranscriptStruct, *ZMapStyleTranscript ;





typedef struct ZMapFeatureTypeStyleStruct_
{
  /* _All_ styles must have these fields set, no other fields are compulsory. */
  GQuark original_id ;					    /* Original name. */
  GQuark unique_id ;					    /* Name normalised to be unique. */


  /* Since all these fields are optional we need flags for all of them to show whether they were
   *  set.
   * 
   * IN FACT THIS MAY NOT BE TRUE AS SOME OF THESE FLAGS ARE NOT INDEPENDENT...WE'LL SEE....
   *  */
  struct
  {
    unsigned int parent_style : 1 ;

    unsigned int description : 1 ;

    unsigned int mode : 1 ;

    unsigned int min_score : 1 ;
    unsigned int max_score : 1 ;

    unsigned int overlap_mode : 1 ;
    unsigned int bump_spacing : 1 ;

    unsigned int min_mag : 1 ;
    unsigned int max_mag : 1 ;

    unsigned int width : 1 ;

    unsigned int score_mode : 1 ;

    unsigned int gff_source : 1 ;
    unsigned int gff_feature : 1 ;

  } fields_set ;



  /*
   * Data fields for the style.
   */

  /* Styles can inherit from other styles, the parent style _must_ be identified by its unique id. */
  GQuark parent_id ;

  char *description ;					    /* Description of what this style
							       represents. */

  ZMapStyleMode mode ;					    /* Specifies how features that
							       reference this style will be processed. */

  ZMapStyleFullColourStruct colours ;			    /* Main feature colours. */

  /* Colours for when feature is shown in frames. */
  ZMapStyleFullColourStruct frame0_colours ;
  ZMapStyleFullColourStruct frame1_colours ;
  ZMapStyleFullColourStruct frame2_colours ;


  ZMapStyleOverlapMode overlap_mode ;			    /* Controls how features are grouped
							       into sub columns within a column. */
  double bump_spacing ;					    /* gap between bumped features. */

  double min_mag ;					    /* Don't display if fewer bases/line */
  double max_mag ;					    /* Don't display if more bases/line */

  double width ;					    /* column width */

  ZMapStyleScoreMode score_mode ;			    /* Controls width of features that
							       have scores. */
  double min_score, max_score ;				    /* Min/max for score width calc. */


  /* GFF feature dumping, allows specifying of source/feature types independently of feature
   * attributes. */
  GQuark gff_source ;
  GQuark gff_feature ;


  /* State information for the style. */
  struct
  {
    unsigned int hidden_always   : 1 ;		    /* Column always hidden. */
    unsigned int hidden_init     : 1 ;		    /* Column hidden initially */
    unsigned int show_when_empty : 1 ;		    /* If FALSE, features' column is
							       displayed even if there are no features. */

    unsigned int showText        : 1 ;			    /* Should feature text be displayed. */

    unsigned int parse_gaps      : 1 ;
    unsigned int align_gaps      : 1 ;			    /* TRUE: gaps within alignment are displayed,
							       FALSE: alignment is displayed as a single block. */
    unsigned int join_aligns     : 1 ;			    /* TRUE: joins aligns if between_align_error not exceeded,
							       FALSE: does not joing aligns. */



    /* These are all linked, if strand_specific is FALSE, then so are
     * frame_specific and show_rev_strand. */
    unsigned int strand_specific : 1 ;			    /* Feature that is on one strand of the dna. */
    unsigned int show_rev_strand : 1 ;			    /* Only display the feature on the
							       reverse strand if this is set. */
    unsigned int frame_specific  : 1 ;			    /* Feature that is in some way linked
							       to the reading frame of the dna. */
    unsigned int show_only_as_3_frame : 1 ;		    /* frame specific feature that should
							       only be displayed when all 3 frames
							       are shown. */

    unsigned int directional_end : 1 ;			    /* Display pointy ends on exons etc. */

  } opts ;

  
  union
  {
    ZMapStyleTranscriptStruct transcript ;
    ZMapStyleAlignmentStruct alignment ;
    ZMapStyleGraphStruct graph ;
    ZMapStyleGlyphStruct glyph ;
  } mode_data ;



} ZMapFeatureTypeStyleStruct ;




#define IS_COLOURSET(COLOUR_PTR) \
  ((COLOUR_PTR)->colours_set.draw || (COLOUR_PTR)->colours_set.fill || (COLOUR_PTR)->colours_set.border)





#endif /* !ZMAP_STYLE_P_H */
