/*  File: zmapConfigStyleDefaults.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2006
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Defines various strings etc. that are required by the
 *              configuration routines.
 *
 * HISTORY:
 * Last edited: Oct  3 14:39 2006 (edgrif)
 * Created: Sun May 28 09:16:38 2006 (rds)
 * CVS info:   $Id: zmapConfigStyleDefaults.h,v 1.6 2006-10-03 15:05:42 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_CONFIG_STYLE_DEFAULTS_H
#define ZMAP_CONFIG_STYLE_DEFAULTS_H


/*! 
 * Here are a list of predefined/fixed styles which zmap uses. Styles with these names
 * _MUST_ be available from the server with the given name (case insensitive),
 * if they are not then various types of display (dna, 3 frame) are not possible.
 * 
 * ZMap will fill in the style with some reasonable defaults, which
 * users may overload/override, so long as they use these names.
 * 
 * - 3 Frame                  controls 3 frame display
 * - 3 Frame Translation      controls 3 frame protein translation
 * 
 * - DNA                      controls dna sequence display
 * 
 * - Locus                    controls display of a column of locus names
 * 
 * - GeneFinderFeatures       controls fetching/display of gene finder features
 * 
 */

#define ZMAP_FIXED_STYLE_3FRAME   "3 Frame"
#define ZMAP_FIXED_STYLE_3FT_NAME "3 Frame Translation"

#define ZMAP_FIXED_STYLE_DNA_NAME "DNA"

#define ZMAP_FIXED_STYLE_LOCUS_NAME "Locus"

#define ZMAP_FIXED_STYLE_GFF_NAME "GeneFinderFeatures"



/* The opts struct */
#define ZMAP_STYLE_DEFAULT_HIDE_INITIALLY  FALSE
#define ZMAP_STYLE_DEFAULT_SHOW_WHEN_EMPTY FALSE
#define ZMAP_STYLE_DEFAULT_SHOW_TEXT       FALSE
#define ZMAP_STYLE_DEFAULT_PARSE_GAPS      TRUE
#define ZMAP_STYLE_DEFAULT_ALIGN_GAPS      TRUE
#define ZMAP_STYLE_DEFAULT_STRAND_SPECIFIC TRUE
#define ZMAP_STYLE_DEFAULT_FRAME_SPECIFIC  FALSE
#define ZMAP_STYLE_DEFAULT_SHOW_REV_STRAND TRUE
#define ZMAP_STYLE_DEFAULT_DIRECTIONAL_END FALSE

/* The colours struct */
#define ZMAP_STYLE_DEFAULT_OUTLINE    "black"
#ifdef RDS_THESE_DEFAULT_TO_TRANSPARENT
#define ZMAP_STYLE_DEFAULT_FOREGROUND "white"
#define ZMAP_STYLE_DEFAULT_BACKGROUND "white"
#endif  /* RDS_THESE_DEFAULT_TO_TRANSPARENT */

/* Overlap */
#define ZMAP_STYLE_DEFAULT_OVERLAP_MODE 1

/* Magnification */
#define ZMAP_STYLE_DEFAULT_MIN_MAG 10.0
#define ZMAP_STYLE_DEFAULT_MAX_MAG 10.0

/* Widths */
#define ZMAP_STYLE_DEFAULT_WIDTH      10.0
#define ZMAP_STYLE_DEFAULT_BUMP_WIDTH 10.0

/* Scores */
#define ZMAP_STYLE_DEFAULT_SCORE_MODE 1
#define ZMAP_STYLE_DEFAULT_MIN_SCORE 10.0
#define ZMAP_STYLE_DEFAULT_MAX_SCORE 10.0

#endif  /* ZMAP_CONFIG_STYLE_DEFAULTS_H */
