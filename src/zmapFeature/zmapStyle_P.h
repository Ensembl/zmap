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
 * Last edited: Feb 26 09:27 2007 (edgrif)
 * Created: Mon Feb 26 09:13:30 2007 (edgrif)
 * CVS info:   $Id: zmapStyle_P.h,v 1.1 2007-02-28 18:15:51 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_STYLE_P_H
#define ZMAP_STYLE_P_H



/* TEMP....WHILE I MOVE STYLE STUFF IN HERE.... */
#include <ZMap/zmapFeature.h>


#include <ZMap/zmapStyle.h>





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

    unsigned int graph_mode : 1 ;
    unsigned int baseline : 1 ;
    unsigned int min_score : 1 ;
    unsigned int max_score : 1 ;

    unsigned int foreground_set : 1 ;
    unsigned int background_set : 1 ;
    unsigned int outline_set    : 1 ;

    unsigned int overlap_mode : 1 ;

    unsigned int min_mag : 1 ;
    unsigned int max_mag : 1 ;

    unsigned int width : 1 ;
    unsigned int bump_width : 1 ;

    unsigned int score_mode : 1 ;

    unsigned int within_align_error : 1 ;
    unsigned int between_align_error : 1 ;

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

  /* graph parameters. */
  ZMapStyleGraphMode graph_mode ;			    /* Says how to draw a graph. */

  double baseline ;


  struct
  {
    GdkColor  foreground ;      /* Overlaid on background. */
    GdkColor  background ;      /* Fill colour. */
    GdkColor  outline ;         /* Surround/line colour. */
  } normal_colours ;


  ZMapStyleOverlapMode overlap_mode ;			    /* Controls how features are grouped
							       into sub columns within a column. */

  double min_mag ;					    /* Don't display if fewer bases/line */
  double max_mag ;					    /* Don't display if more bases/line */

  double   width ;					    /* column width */
  double bump_width;					    /* gap between bumped columns. */

  ZMapStyleScoreMode   score_mode ;			    /* Controls width of features that
							       have scores. */
  double    min_score, max_score ;			    /* Min/max for score width calc. */


  /* Allowable align errors, used to decide whether a match should be classified as "perfect".
   *   within_align_error   is used to assess the blocks in a single gapped alignment if align_gaps = TRUE
   *  between_align_error   is used to assess several alignments (e.g. for exon matches) if join_homols = TRUE
   * 
   * Number is allowable number of missing bases between blocks/alignments, default is 0. */
  unsigned int within_align_error ;
  unsigned int between_align_error ;

  /* GFF feature dumping, allows specifying of source/feature types independently of feature
   * attributes. */
  GQuark gff_source ;
  GQuark gff_feature ;


  /* State information for the style. */
  struct
  {
    /* I don't want a general show/hide flag here because we should
     * get that dynamically from the state of the column canvas
     * item.                     ummm, doesn't fit with the hidden_now flag ??????  */
    unsigned int hidden_always   : 1 ;			    /* Column always hidden. */

    unsigned int hidden_now      : 1 ;			    /* Column hidden now ? */

    unsigned int show_when_empty : 1 ;			    /* If TRUE, features' column is
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

} ZMapFeatureTypeStyleStruct ;







#endif /* !ZMAP_STYLE_P_H */
