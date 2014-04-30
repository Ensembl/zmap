/*  File: zmapWindowCanvasUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2014: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Utility functions for canvas objects.
 *
 * Exported functions: See zmapWindowCanvasUtils.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <zmapWindowCanvasUtils.h>
#include <zmapWindowCanvasFeatureset_I.h>




/* 
 *                   External routines.
 */


/* Malcolm's comments about being handed NULLs surely implies that his code handling
 * the lists is faulty....it's also annoying that he has inserted return statements
 * everywhere when it's completely unnecessary.....deep sigh.... */


/* sort by genomic coordinate for display purposes */
/* start coord then end coord reversed, mainly for summarise function */
/* also used by collapse code and locus de-overlap  */
gint zMapFeatureCmp(gconstpointer a, gconstpointer b)
{
  ZMapWindowCanvasFeature feata = (ZMapWindowCanvasFeature) a;
  ZMapWindowCanvasFeature featb = (ZMapWindowCanvasFeature) b;

  /* we can get NULLs due to GLib being silly */
  /* this code is pedantic, but I prefer stable sorting */
  if(!featb)
    {
      if(!feata)
	return(0);
      return(1);
    }
  if(!feata)
    return(-1);

  if(feata->y1 < featb->y1)
    return(-1);
  if(feata->y1 > featb->y1)
    return(1);

  if(feata->y2 > featb->y2)
    return(-1);

  if(feata->y2 < featb->y2)
    return(1);

  return(0);
}


/* Fuller version of zMapFeatureCmp() which handles special glyph code where
 * positions to be compared can be greater than the feature coords.
 *
 * NOTE that featb is a 'dummy' just used for coords.
 *  */
gint zMapFeatureFullCmp(gconstpointer a, gconstpointer b)
{
  ZMapWindowCanvasFeature feata = (ZMapWindowCanvasFeature) a ;
  ZMapWindowCanvasFeature featb = (ZMapWindowCanvasFeature) b ;

  /* we can get NULLs due to GLib being silly */
  /* this code is pedantic, but I prefer stable sorting */
  if(!featb)
    {
      if(!feata)
	return(0);
      return(1);
    }
  else if(!feata)
    {
      return(-1);
    }
  else
    {
      int real_start, real_end ;

      real_start = feata->y1 ;
      real_end = feata->y2 ;

      if (feata->type == FEATURE_GLYPH && feata->feature->flags.has_boundary)
	{
	  if (feata->feature->boundary_type == ZMAPBOUNDARY_5_SPLICE)
	    {
	      real_start = feata->y1 + 0.5 ;
	      real_end = feata->y2 + 2 ;
	    }
	  else if (feata->feature->boundary_type == ZMAPBOUNDARY_3_SPLICE)
	    {
	      real_start = feata->y1 - 2 ;
	      real_end = feata->y2 - 0.5 ;
	    }


	  /* Look for dummy to be completey inside.... */
	  if (real_start <= featb->y1 && real_end >= featb->y2)
	    return(0);

	  if(real_start < featb->y1)
	    return(-1);
	  if(real_start > featb->y1)
	    return(1);
	  if(real_end > featb->y2)
	    return(-1);
	  if(real_end < featb->y2)
	    return(1);
	}
      else
	{
	  if(real_start < featb->y1)
	    return(-1);
	  if(real_start > featb->y1)
	    return(1);

	  if(real_end > featb->y2)
	    return(-1);

	  if(real_end < featb->y2)
	    return(1);
	}
    }

  return(0);
}


/* sort by name and the start coord to link same name features,
 * NOTE that we _must_ use the original_id here because the unique_id
 * is different as it incorporates position information.
 * 
 * note that ultimately we are interested in query coords in a zmapHomol struct
 * but only after getting alignments in order on the genomic sequence
 */
gint zMapFeatureNameCmp(gconstpointer a, gconstpointer b)
{
  ZMapWindowCanvasFeature feata = (ZMapWindowCanvasFeature) a;
  ZMapWindowCanvasFeature featb = (ZMapWindowCanvasFeature) b;

  if(!zmapWindowCanvasFeatureValid(featb))
    {
      if(!zmapWindowCanvasFeatureValid(feata))
	return(0);
      return(1);
    }
  if(!zmapWindowCanvasFeatureValid(feata))
    return(-1);

  if(feata->feature->strand < featb->feature->strand)
    return(-1);
  if(feata->feature->strand > featb->feature->strand)
    return(1);

  if(feata->feature->original_id < featb->feature->original_id)
    return(-1);
  if(feata->feature->original_id > featb->feature->original_id)
    return(1);

  return(zMapFeatureCmp(a,b));
}


/* need a flag to say whether to sort by name too ??? */
/* Sort by featureset name and the start coord to link same featureset features */
gint zMapFeatureSetNameCmp(gconstpointer a, gconstpointer b)
{
  gint result = 0 ;
  ZMapWindowCanvasFeature feata = (ZMapWindowCanvasFeature) a ;
  ZMapWindowCanvasFeature featb = (ZMapWindowCanvasFeature) b ;
  ZMapFeature feature_a = feata->feature ;
  ZMapFeature feature_b = featb->feature ;
  ZMapFeatureSet featureset_a = (ZMapFeatureSet)(feature_a->parent) ;
  ZMapFeatureSet featureset_b = (ZMapFeatureSet)(feature_b->parent) ;


  /* Can this really happen ????? */
  if(!zmapWindowCanvasFeatureValid(featb))
    {
      if(!zmapWindowCanvasFeatureValid(feata))
	return(-1) ;

      return(1) ;
    }

  /* Sort on unique (== canonicalised) name. */
  if (featureset_a->unique_id < featureset_b->unique_id)
    return -1 ;
  else if (featureset_a->unique_id > featureset_b->unique_id)
    return 1 ;

  if(feata->feature->strand < featb->feature->strand)
    return(-1);
  if(feata->feature->strand > featb->feature->strand)
    return(1);



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* ok...I think we don't want to do this as graph things have weird names */

  /* is this correct...sorting on original name.....surely not ???? */
  if(feata->feature->unique_id < featb->feature->unique_id)
    return(-1);
  if(feata->feature->unique_id > featb->feature->unique_id)
    return(1);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  result = zMapFeatureCmp(a,b) ;

  return result ;
}

