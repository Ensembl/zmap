/*  File: zmapFeatureContext.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Functions that operate on the feature context such as
 *              for reverse complementing.
 *              
 * Exported functions: See ZMap/zmapFeature.h
 * HISTORY:
 * Last edited: Jan 23 14:00 2006 (edgrif)
 * Created: Tue Jan 17 16:13:12 2006 (edgrif)
 * CVS info:   $Id: zmapFeatureContext.c,v 1.1 2006-01-23 14:10:55 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <zmapFeature_P.h>


typedef struct
{
  int end ;
} RevCompDataStruct, *RevCompData ;



static void revcompDNA(ZMapSequence sequence) ;
static void revcompFeatures(ZMapFeatureContext context) ;
static void doFeatureAnyCB(ZMapFeatureAny any_feature, gpointer user_data) ;
static void datasetCB(GQuark key_id, gpointer data, gpointer user_data) ;
static void listCB(gpointer data, gpointer user_data) ;
static void revcompSpan(GArray *spans, int seq_end) ;




/* Reverse complement a feature context.
 * 
 * Efficiency does not allow us to simply throw everything away and reconstruct the context.
 * Therefore we have to go through and carefully reverse everything.
 *  */
void zMapFeatureReverseComplement(ZMapFeatureContext context)
{

  /*	features need to have their positions reversed and also their strand.
    what about blocks ??? they also need doing... and in fact there are the alignment
    * mappings etc.....needs some thought and effort.... */

  revcompFeatures(context) ;

  return ;
}









/* 
 *                        Internal functions. 
 */



/* Reverse complement the DNA. This function is fast enough for now, if it proves too slow
 * then rewrite it !
 * 
 * It works by starting at each end and swopping the bases and then complementing the bases
 * so that the whole thing is done in place.
 *  */
static void revcompDNA(ZMapSequence sequence)
{
  char *s, *e ;
  int i ;

  for (s = sequence->sequence, e = (sequence->sequence + sequence->length - 1), i = 0 ;
       i < sequence->length / 2 ;
       s++, e--, i++)
    {
      char tmp ;

      tmp = *s ;
      *s = *e ;
      *e = tmp ;

      switch (*s)
	{
	case 'a':
	  *s = 't' ;
	  break ;
	case 't':
	  *s = 'a' ;
	  break ;
	case 'c':
	  *s = 'g' ;
	  break ;
	case 'g':
	  *s = 'c' ;
	  break ;
	}

      switch (*e)
	{
	case 'a':
	  *e = 't' ;
	  break ;
	case 't':
	  *e = 'a' ;
	  break ;
	case 'c':
	  *e = 'g' ;
	  break ;
	case 'g':
	  *e = 'c' ;
	  break ;
	}
    }

  return ;
}




/* The next three functions could be condensed to a call simply to one if I had had
 * the sense to make all of the context contain g_datalists and not have the blocks as
 * a g_list...sigh...I could go back and change it but there are surprisingly many places
 * where it occurs... */

static void revcompFeatures(ZMapFeatureContext context)
{
  RevCompDataStruct cb_data ;

  cb_data.end = context->sequence_to_parent.c2 ;

  doFeatureAnyCB((ZMapFeatureAny)context, &cb_data) ;

  return ;
}


static void datasetCB(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureAny any_feature = (ZMapFeatureAny)data ;

  doFeatureAnyCB(any_feature, user_data) ;

  return ;
}


static void listCB(gpointer data, gpointer user_data)
{
  ZMapFeatureAny any_feature = (ZMapFeatureAny)data ;

  doFeatureAnyCB(any_feature, user_data) ;

  return ;
}


/* The guts of the code, this is a recursive function in that it calls g_datalist or g_list
 * which then calls it for each member of the list. */
static void doFeatureAnyCB(ZMapFeatureAny any_feature, gpointer user_data)
{
  RevCompData cb_data = (RevCompData)user_data ;
  GData **dataset = NULL ;
  GList *list = NULL ;

  zMapAssert(any_feature && zMapFeatureIsValid(any_feature)) ;

  switch (any_feature->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      {
	ZMapFeatureContext context = (ZMapFeatureContext)any_feature ;

	zmapFeatureSwop(Coord, context->sequence_to_parent.p1, context->sequence_to_parent.p2) ;

	/* Revcomp the DNA if there is any. */
	if (context->sequence)
	  revcompDNA(context->sequence) ;

	dataset = &(context->alignments) ;
	break ;
      }
    case ZMAPFEATURE_STRUCT_ALIGN:
      /* Nothing to swop. */
      list = ((ZMapFeatureAlignment)any_feature)->blocks ;
      break ;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
	ZMapFeatureBlock block = (ZMapFeatureBlock)any_feature ;

	/* DNA revcomp should happen here, but what if the block is the same as the context ?
	 * SORT this out.... */
	if (block->sequence.sequence)
	  revcompDNA(&(block->sequence)) ;


	zmapFeatureRevComp(Coord, cb_data->end,
			 block->block_to_sequence.t1, block->block_to_sequence.t2) ;

	dataset = &(block->feature_sets) ;
	break ;
      }
    case ZMAPFEATURE_STRUCT_FEATURESET:
      /* Nothing to swop here, I think..... */

      dataset = &(((ZMapFeatureSet)any_feature)->features) ;
      break ;
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
	ZMapFeature feature = (ZMapFeature)any_feature ;

	zmapFeatureRevComp(Coord, cb_data->end, feature->x1, feature->x2) ;

	if (feature->strand == ZMAPSTRAND_FORWARD)
	  feature->strand = ZMAPSTRAND_REVERSE ;
	else if (feature->strand == ZMAPSTRAND_REVERSE)
	  feature->strand = ZMAPSTRAND_FORWARD ;

	if (feature->type == ZMAPFEATURE_TRANSCRIPT
	    && (feature->feature.transcript.exons || feature->feature.transcript.introns))
	  {
	    if (feature->feature.transcript.exons)
	      revcompSpan(feature->feature.transcript.exons, cb_data->end) ;

	    if (feature->feature.transcript.introns)
	      revcompSpan(feature->feature.transcript.introns, cb_data->end) ;
	  }
	else if (feature->type == ZMAPFEATURE_ALIGNMENT
		 && feature->feature.homol.align)
	  {
	    int i ;
	    for (i = 0; i < feature->feature.homol.align->len; i++)
	      {
		ZMapAlignBlock align ;
		
		align = &g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, i) ;
		
		zmapFeatureRevComp(Coord, cb_data->end, align->t1, align->t2) ;
	      }
	  }
	break ;
      }
    default:
      {
	zMapAssertNotReached() ;
      }
    }


  if (dataset)
    g_datalist_foreach(dataset, datasetCB, user_data) ;
  else if (list)
    g_list_foreach(list, listCB, user_data) ;
  


  return ;
}



static void revcompSpan(GArray *spans, int seq_end)
{
  int i ;

  for (i = 0; i < spans->len; i++)
    {
      ZMapSpan span ;
      
      span = &g_array_index(spans, ZMapSpanStruct, i) ;

      zmapFeatureRevComp(Coord, seq_end, span->x1, span->x2) ;
    }
  

  return ;
}
