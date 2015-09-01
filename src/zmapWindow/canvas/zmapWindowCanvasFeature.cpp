/*  File: zmapWindowCanvasFeature.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2014-2015: Genome Research Ltd.
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
 *         Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Provides the interface to individual features within
 *              columns.
 *
 * Exported functions: See zmapWindowCanvasFeature.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>
#include <glib.h>

#include <ZMap/zmapUtils.hpp>
#include <zmapWindowCanvasFeatureset_I.hpp>
#include <zmapWindowCanvasFeature_I.hpp>


static void freeSplicePosCB(gpointer data, gpointer user_data_unused) ;



/*
 *                Globals
 */

static ZMapWindowCanvasFeatureClass feature_class_G = NULL ;

static gpointer feature_extent_G[FEATURE_N_TYPE] = { 0 } ;
static gpointer feature_subpart_G[FEATURE_N_TYPE] = { 0 } ;

static long n_block_alloc = 0;
static long n_feature_alloc = 0;
static long n_feature_free = 0;




/* Some important notes:
 *
 * ZMapWindowCanvasFeature's are not implemented as a GObject, this
 * is a deliberate decision because we found that GObjects are not
 * well suited to situations where you have very large numbers of
 * small objects. For instance the casting and type checking
 * mechanisms for GObjects are far too slow to be of use as are
 * functions like copy constructors and the like.
 *
 * Therefore while initialisation and so on are similar,
 * ZMapWindowCanvasFeature's are handled by straightforward function
 * calls and do not go through the GObject type system and are not
 * part of the Foocanvas hierachy. They have a parent, the
 * ZMapWindowFeaturesetItem, that is part of Foocanvas but that's it.
 *
 */



/*
 *                          External interface routines
 */


void zMapWindowCanvasFeatureInit(void)
{
  ZMapWindowCanvasFeatureClass feature_class ;

  feature_class = g_new0(ZMapWindowCanvasFeatureClassStruct, 1) ;

  /* set size of unspecified features structs to just the base */
  feature_class->struct_size[FEATURE_INVALID] = sizeof(zmapWindowCanvasFeatureStruct);

  feature_class_G = feature_class ;

  return ;
}

void zMapWindowCanvasFeatureSetSize(int featuretype, gpointer *feature_funcs, size_t feature_struct_size)
{
  feature_class_G->struct_size[featuretype] = feature_struct_size ;

  feature_extent_G[featuretype] = feature_funcs[CANVAS_FEATURE_FUNC_EXTENT] ;
  feature_subpart_G[featuretype] = feature_funcs[CANVAS_FEATURE_FUNC_SUBPART] ;

  return ;
}



/* allocate a free list for an unknown structure */
ZMapWindowCanvasFeature zMapWindowCanvasFeatureAlloc(zmapWindowCanvasFeatureType type)
{
  ZMapWindowCanvasFeature feat = NULL ;

  /* WARNING...THIS CODE IS QUITE HARD TO FATHOM/FOLLOW... */

  if (type > FEATURE_INVALID && type < FEATURE_N_TYPE)
    {
      GList *l = NULL ;
      size_t size = feature_class_G->struct_size[type] ;

      if (!feature_class_G->feature_free_list[type])
        {
          int i ;
          ZMapWindowCanvasFeature mem = NULL ;
          mem = (ZMapWindowCanvasFeature) g_malloc0(size * N_FEAT_ALLOC) ;

          if (mem)
            {
              for (i = 0 ; i < N_FEAT_ALLOC ; ++i)
                {
                  feat = (ZMapWindowCanvasFeature)mem ;
                  feat->type = type ;
                  feat->feature = NULL ;
                  zmapWindowCanvasFeatureFree((gpointer)mem) ;
                  mem = (ZMapWindowCanvasFeature) ((size_t)mem + size ) ;
                }
              ++n_block_alloc ;
            }
        }

      l = feature_class_G->feature_free_list[type] ;

      if (l)
        {
          feat = (ZMapWindowCanvasFeature)l->data ;

          feature_class_G->feature_free_list[type]
            = g_list_delete_link(feature_class_G->feature_free_list[type], l) ;

          /* GOODNESS ME....THIS SEEMS TO REZERO SOMETHING WE INIT'D ABOVE.... */
          /* these can get re-allocated so must zero */
          memset((void*)feat, 0, size) ;

          /* AND NOW WE RESET THE TYPE....STREUTH..... */
          feat->type = type ;
          feat->feature = NULL ;

          ++n_feature_alloc ;
        }
    }

  return feat ;
}


/* get the total sequence extent of a simple or complex feature */
/* used by bumping; we allow force to simple for optional but silly bump modes */
gboolean zMapWindowCanvasFeatureGetFeatureExtent(ZMapWindowCanvasFeature feature,
                                                 gboolean is_complex, ZMapSpan span, double *width)
{
  gboolean result = TRUE ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  void (*func) (ZMapWindowCanvasFeature feature, ZMapSpan span, double *width) = NULL;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  ZMapWindowCanvasGetExtentFunc func = NULL ;


  zMapReturnValIfFail(zmapWindowCanvasFeatureValid(feature), FALSE) ;


  /* oh dear....perhaps we can move this wholly into there from featureset.c....not a standard
     func so should be ok.... */
  func = (ZMapWindowCanvasGetExtentFunc)feature_extent_G[feature->type];

  if(!is_complex)                /* reset any compund feature extents */
    feature->y2 = feature->feature->x2;

  if(!func || !is_complex)
    {
      /* all features have an extent, if it's simple get it here */
      span->x1 = feature->feature->x1;        /* use real coord from the feature context */
      span->x2 = feature->feature->x2;
      *width = feature->width;
    }
  else
    {
      func(feature, span, width);
    }

  return result ;
}

/* Add/remove splice positions to a feature, these can be displayed/highlighted
 * to the user.
 *
 * These positions probably indicate where the feature splices match some other
 * feature selected by the user. */
void zMapWindowCanvasFeatureAddSplicePos(ZMapWindowCanvasFeature feature_item, int feature_pos,
                                         gboolean match, ZMapBoundaryType boundary_type)
{
  ZMapSplicePosition splice_pos ;
  int splice_width = 10 ;

  splice_pos = g_new0(ZMapSplicePositionStruct, 1) ;
  splice_pos->match = match ;
  splice_pos->boundary_type = boundary_type ;

  if (boundary_type == ZMAPBOUNDARY_5_SPLICE)
    {
      splice_pos->start = feature_pos - splice_width ;
      splice_pos->end = feature_pos - 1 ;
    }
  else
    {
      splice_pos->start = feature_pos + 1 ;
      splice_pos->end = feature_pos + splice_width ;
    }

  feature_item->splice_positions = g_list_append(feature_item->splice_positions, splice_pos) ;

  return ;
}


void zMapWindowCanvasFeatureRemoveSplicePos(ZMapWindowCanvasFeature feature_item)
{
  g_list_foreach(feature_item->splice_positions, freeSplicePosCB, NULL) ;

  g_list_free(feature_item->splice_positions) ;

  feature_item->splice_positions = NULL ;

  return ;
}


/* Output the canvas feature as a descriptive string. */
GString *zMapWindowCanvasFeature2Txt(ZMapWindowCanvasFeature canvas_feature)
{
  GString *canvas_feature_text = NULL ;
  const char *indent = "" ;

  zMapReturnValIfFail(canvas_feature, NULL) ;

  canvas_feature_text = g_string_sized_new(2048) ;

  /* Title */
  g_string_append_printf(canvas_feature_text, "%sZMapWindowCanvasFeature %p, type = %s)\n",
                         indent, canvas_feature, zMapWindowCanvasFeatureType2ExactStr(canvas_feature->type)) ;

  indent = "  " ;

  g_string_append_printf(canvas_feature_text, "%sStart, End:  %g, %g\n",
                         indent, canvas_feature->y1, canvas_feature->y2) ;

  if (canvas_feature->feature)
    g_string_append_printf(canvas_feature_text, "%sContained ZMapFeature %p \"%s\" (unique id = \"%s\")\n",
                           indent,
                           canvas_feature->feature,
                           (char *)g_quark_to_string(canvas_feature->feature->original_id),
                           (char *)g_quark_to_string(canvas_feature->feature->unique_id)) ;

  g_string_append_printf(canvas_feature_text, "%sScore:  %g\n",
                         indent, canvas_feature->score) ;

  g_string_append_printf(canvas_feature_text, "%swidth:  %g\n",
                         indent, canvas_feature->width) ;

  g_string_append_printf(canvas_feature_text, "%sbump_offset, bump subcol:  %g, %d\n",
                         indent, canvas_feature->bump_offset, canvas_feature->bump_col) ;

  if (canvas_feature->splice_positions)
    g_string_append_printf(canvas_feature_text, "%sHas splice highlighting\n", indent) ;

  return canvas_feature_text ;
}


/* return a span struct for the feature */
ZMapFeatureSubPart zMapWindowCanvasFeaturesetGetSubPart(FooCanvasItem *foo, ZMapFeature feature,
                                                        double x, double y)
{
  ZMapFeatureSubPart sub_part = NULL ;
  ZMapWindowCanvasGetSubPartFunc func = NULL ;

  zMapReturnValIfFail((foo && feature && x && y), NULL) ;

  /* oh dear...another func to move into here from featureset.c I think..... */

  if (feature->mode > 0 && (zmapWindowCanvasFeatureType)(feature->mode) < FEATURE_N_TYPE
      && (func = (ZMapWindowCanvasGetSubPartFunc)feature_subpart_G[feature->mode]))
    sub_part = func(foo, feature, x, y) ;

  return sub_part ;
}


ZMapFeature zMapWindowCanvasFeatureGetFeature(ZMapWindowCanvasFeature feature_item)
{
  ZMapFeature feature = NULL ;

  if (feature_item->feature)
    feature = feature_item->feature ;

  return feature ;
}



/* Malcolm's comments about being handed NULLs surely implies that his code handling
 * the lists is faulty....it's also annoying that he has inserted return statements
 * everywhere when it's completely unnecessary.....deep sigh.... */

/* sort by genomic coordinate for display purposes */
/* start coord then end coord reversed, mainly for summarise function */
/* also used by collapse code and locus de-overlap  */
gint zMapWindowFeatureCmp(gconstpointer a, gconstpointer b)
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


/* Fuller version of zMapWindowFeatureCmp() which handles special glyph code where
 * positions to be compared can be greater than the feature coords.
 *
 * NOTE that featb is a 'dummy' just used for coords.
 *  */
gint zMapWindowFeatureFullCmp(gconstpointer a, gconstpointer b)
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

  return(zMapWindowFeatureCmp(a,b));
}


/* need a flag to say whether to sort by name too ??? */
/* Sort by featureset name and the start coord to link same featureset features */
gint zMapFeatureSetNameCmp(gconstpointer a, gconstpointer b)
{
  gint result = 0 ;
  ZMapWindowCanvasFeature feata = (ZMapWindowCanvasFeature) a ;
  ZMapWindowCanvasFeature featb = (ZMapWindowCanvasFeature) b ;
  ZMapFeature feature_a = NULL ;
  ZMapFeature feature_b = NULL ;
  ZMapFeatureSet featureset_a = NULL ;
  ZMapFeatureSet featureset_b = NULL ;

  zMapReturnValIfFail(feata && featb && feata->feature && featb->feature, result ) ;

  feature_a = feata->feature ;
  feature_b = featb->feature ;
  featureset_a = (ZMapFeatureSet)(feature_a->parent) ;
  featureset_b = (ZMapFeatureSet)(feature_b->parent) ;


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

  result = zMapWindowFeatureCmp(a,b) ;

  return result ;
}




/*
 *                          Package routines
 */


/* need to be a ZMapSkipListFreeFunc for use as a callback */
void zmapWindowCanvasFeatureFree(gpointer thing)
{
  ZMapWindowCanvasFeature feat = NULL ;
  zmapWindowCanvasFeatureType type ;

  zMapReturnIfFail(thing && feature_class_G) ;

  feat = (ZMapWindowCanvasFeature) thing ;
  type = feat->type ;

  if(!feature_class_G->struct_size[type])
    type = FEATURE_INVALID ;                /* catch all for simple features */

  feature_class_G->feature_free_list[type] =
    g_list_prepend(feature_class_G->feature_free_list[type], thing) ;

  n_feature_free++ ;
}



/*
 *                    Internal routines
 */


static void freeSplicePosCB(gpointer data, gpointer user_data_unused)
{
  ZMapSplicePosition splice_pos = (ZMapSplicePosition)data ; /* for debugging. */

  g_free(splice_pos) ;

  return ;
}

