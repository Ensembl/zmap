/*  File: zmapWindowColBump.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Column bumping functions.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <glib.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <zmapWindow_P.hpp>


typedef struct
{
  ZMapWindow                    window;
  ZMapWindowContainerFeatureSet container;
  GQuark                        style_id;
  ZMapStyleBumpMode             overlap_mode;
  ZMapStyleColumnDisplayState   display_state;
  unsigned int                  match_threshold;
  gboolean                      bump_all;
} BumpPropertiesStruct, *BumpProperties;

/* For straight forward bumping. */
typedef struct
{
  GHashTable          *pos_hash ;
  GList               *pos_list ;

  double               offset;
  double               incr ;
  double spacing ;

  int                  start;
  int                  end ;

  BumpPropertiesStruct bump_prop_data;
} BumpColStruct, *BumpCol ;


typedef struct
{
  double y1, y2 ;
  double offset ;
  double incr ;
  int column;
} BumpColRangeStruct, *BumpColRange ;

typedef struct
{
  double x, width;
}OffsetWidthStruct, *OffsetWidth;


/* THERE IS A BUG IN THE CODE THAT USES THIS...IT MOVES FEATURES OUTSIDE OF THE BACKGROUND SO
 * IT ALL LOOKS NAFF, PROBABLY WE GET AWAY WITH IT BECAUSE COLS ARE SO WIDELY SPACED OTHERWISE
 * THEY MIGHT OVERLAP. I THINK THE MOVING CODE MAY BE AT FAULT. */
/* For complex bump users seem to want columns to overlap a bit, if this is set to 1.0 there is
 * no overlap, if you set it to 0.5 they will overlap by a half and so on. */
#define COMPLEX_BUMP_COMPRESS 1.0

typedef struct
{
  ZMapWindow window ;
  ZMapWindowContainerFeatureSet container;
} AddGapsDataStruct, *AddGapsData ;

typedef gboolean (*OverLapListFunc)(GList *curr_features, GList *new_features) ;

typedef GList* (*GListTraverseFunc)(GList *glist) ;

typedef struct
{
  BumpProperties  bump_properties;

  GHashTable     *name_hash ;
  GList          *bumpcol_list ;
  double          curr_offset ;
  double          incr ;
  OverLapListFunc overlap_func ;
  gboolean        protein ;
  GString        *temp_buffer;

  ZMapWindowContainerFeatureSet feature_set ;

} ComplexBumpStruct, *ComplexBump ;


typedef struct
{
  BumpProperties bump_properties;
  double         incr ;
  double         offset ;
  double         width; /* this will be removed! */
  GList          *feature_list ;
  ZMapWindowContainerFeatureSet feature_set ;
} ComplexColStruct, *ComplexCol ;


typedef struct
{
  GList **name_list ;
  OverLapListFunc overlap_func ;
} FeatureDataStruct, *FeatureData ;

typedef struct
{
  gboolean overlap;
  int start, end ;
  int feature_diff ;
  ZMapFeature feature ;
  double width ;
} RangeDataStruct, *RangeData ;



typedef struct
{
  ZMapWindow window ;
  double zoom ;

} ZoomDataStruct, *ZoomData ;


typedef gboolean (*MakeNameListNamePredicateFunc)(FooCanvasItem *item, ComplexBump complex_bump);
typedef GQuark   (*MakeNameListNameKeyFunc)      (ZMapFeature feature, ComplexBump complex_bump);

typedef struct
{
  MakeNameListNamePredicateFunc predicate_func;
  MakeNameListNameKeyFunc       quark_gen_func;
} MakeNameListsHashDataStruct, *MakeNameListsHashData;



static void invoke_bump_to_initial(ZMapWindowContainerGroup container, FooCanvasPoints *points,
   ZMapContainerLevelType level, gpointer user_data);
static void invoke_bump_to_unbump(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                           ZMapContainerLevelType level, gpointer user_data);


static gboolean containerBumpStyle(ZMapWindow window, ZMapWindowContainerFeatureSet container, gboolean bump) ;





/* Merely a cover function for the real bumping code function zmapWindowColumnBumpRange(). */
void zmapWindowColumnBump(FooCanvasItem *column_item, ZMapStyleBumpMode bump_mode)
{
  ZMapWindowCompressMode compress_mode ;
  ZMapWindow window ;

  g_return_if_fail(ZMAP_IS_CONTAINER_FEATURESET(column_item));

  window = (ZMapWindow)g_object_get_data(G_OBJECT(column_item), ZMAP_WINDOW_POINTER) ;
  if (!window)
    return ;

  if (zmapWindowMarkIsSet(window->mark))
    compress_mode = ZMAPWINDOW_COMPRESS_MARK ;
  else
    compress_mode = ZMAPWINDOW_COMPRESS_ALL ;

  zmapWindowColumnBumpRange(column_item, bump_mode, compress_mode) ;

  return ;
}



/* Bumps either the whole column represented by column_item, or if the item is a feature item
 * in the column then bumps just features in that column that share the same style. This
 * allows the user control over whether to bump all features in a column or just some features
 * in a column.
 *
 * NOTE: this function bumps an individual column but it DOES NOT move any other columns,
 * to do that you need to use zmapWindowColumnReposition(). The split is made because
 * we don't always want to reposition all the columns following a bump, e.g. when we
 * are creating a zmap window.
 *  */
void zmapWindowColumnBumpRange(FooCanvasItem *bump_item, ZMapStyleBumpMode bump_mode,
                               ZMapWindowCompressMode compress_mode)
{
  ZMapWindowContainerFeatureSet container = NULL;
  ZMapStyleBumpMode historic_bump_mode;
  ZMapWindow window = NULL;
  gboolean ok = TRUE;
  gboolean mark_set;
  int start, end ;
//double time;


  /* Decide if the column_item is a column group or a feature within that group. */
  if (ZMAP_IS_CANVAS_ITEM(bump_item))
    {
      container = (ZMapWindowContainerFeatureSet)zmapWindowContainerCanvasItemGetContainer(bump_item);
    }
  else if (ZMAP_IS_CONTAINER_FEATURESET(bump_item))
    {
      container = (ZMapWindowContainerFeatureSet)(bump_item);
    }
  else
    {
      zMapWarnIfReached();
    }

  if (container)
    {
      window = (ZMapWindow)g_object_get_data(G_OBJECT(container), ZMAP_WINDOW_POINTER) ;
    }

  if (window)
    {
      historic_bump_mode = zmapWindowContainerFeatureSetGetBumpMode(container) ;

      if(historic_bump_mode > ZMAPBUMP_UNBUMP && historic_bump_mode != bump_mode && bump_mode != ZMAPBUMP_UNBUMP)
        zmapWindowColumnBumpRange(bump_item,ZMAPBUMP_UNBUMP,compress_mode);

      if (bump_mode == ZMAPBUMP_INVALID)      // this is set to 'rebump' the columns
        bump_mode = historic_bump_mode ;

      if(bump_mode == ZMAPBUMP_UNBUMP && bump_mode == historic_bump_mode)
        return;

      /* Need to know if mark is set for limiting feature display for several modes/feature types. */
      mark_set = zmapWindowMarkIsSet(window->mark) ;

      /* If range set explicitly or a mark is set on the window, then only bump within the range of mark
       * or the visible section of the window. */
      if (compress_mode == ZMAPWINDOW_COMPRESS_INVALID)
        {
          if (mark_set)
            zmapWindowMarkGetSequenceRange(window->mark, &start, &end) ;
          else
            {
              start = window->min_coord ;
              end   = window->max_coord ;
            }
        }
      else
        {
          if (compress_mode == ZMAPWINDOW_COMPRESS_VISIBLE)
            {
              double wx1, wy1, wx2, wy2 ;

              zmapWindowItemGetVisibleWorld(window, &wx1, &wy1, &wx2, &wy2);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
              printf("Visible %f, %f  -> %f, %f\n", wx1, wy1, wx2, wy2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

              /* should really clamp to seq. start/end..... */
              start = (int)wy1 ;
              end = (int)wy2 ;
            }
          else if (compress_mode == ZMAPWINDOW_COMPRESS_MARK)
            {
              /* we know mark is set so no need to check result of range check. But should check
               * that col to be bumped and mark are in same block ! */
              zmapWindowMarkGetSequenceRange(window->mark, &start, &end) ;
            }
          else
            {
              start = window->min_coord ;
              end   = window->max_coord ;
            }
        }


//time = zMapElapsedSeconds;


      if(bump_mode == ZMAPBUMP_STYLE || historic_bump_mode == ZMAPBUMP_STYLE)
        {
          /* this does many featuresets, we expect heatmap sub-columns */
          if (!containerBumpStyle(window, container, bump_mode == ZMAPBUMP_STYLE))
            {
              ok = FALSE;
              zMapWarning("bump style not configured","");
            }
        }
      else
        {
          /* bump features within each featureset item, we normally only expect one */
          /* except for coverage data where we have a few heatmaps.*/

          GList *l;
          FooCanvasGroup *column_features;
          BumpFeaturesetStruct bump_feature_data = { 0 };


          column_features = (FooCanvasGroup *)zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)container) ;

          if(!column_features)
            return;

          bump_feature_data.start = start ;/* NOTE: different struct from previous one */
          bump_feature_data.end = end ;
          bump_feature_data.mark_set = mark_set;
          bump_feature_data.spacing = zmapWindowContainerFeatureGetBumpSpacing(container) ;

          for (l = column_features->item_list ; l ; l = l->next)
            {
              ZMapWindowFeaturesetItem featureset = (ZMapWindowFeaturesetItem)(l->data) ;

              if (!zMapWindowCanvasFeaturesetBump(featureset, bump_mode, (int) compress_mode, &bump_feature_data))
                {
                  zMapWindowCanvasFeaturesetBump(featureset, ZMAPBUMP_UNBUMP, compress_mode, &bump_feature_data) ;
                  ok = FALSE ;
                }


            }
        }

      /* this is a bit poor: we could have a column half bumped if there are > 1 CanvasFeatureset
       * in practice w/ heatmaps there will always be room and w/ other features only 1 CanvasFeatureset
       */
      if(ok)
        zMapWindowContainerFeatureSetSetBumpMode(container,bump_mode);


//time = zMapElapsedSeconds - time;
//printf("featureset bump in %.3f seconds\n", time);
    }

  return ;
}


void zmapWindowColumnBumpAllInitial(FooCanvasItem *column_item)
{
  ZMapWindowContainerGroup container_strand;

  if((container_strand = zmapWindowContainerUtilsItemGetParentLevel(column_item, ZMAPCONTAINER_LEVEL_BLOCK)))
    {
      /* container execute */
      zmapWindowContainerUtilsExecute(container_strand,
                                      ZMAPCONTAINER_LEVEL_FEATURESET,
                                      invoke_bump_to_initial, NULL);
      /* happy days */

    }

  return ;
}

void zmapWindowColumnUnbumpAll(FooCanvasItem *column_item)
{
  ZMapWindowContainerGroup container_strand;

  if((container_strand = zmapWindowContainerUtilsItemGetParentLevel(column_item, ZMAPCONTAINER_LEVEL_BLOCK)))
    {
     /* container execute */
      zmapWindowContainerUtilsExecute(container_strand,
                                      ZMAPCONTAINER_LEVEL_FEATURESET,
                                      invoke_bump_to_unbump, NULL) ;
      /* happy days */
    }

  return ;
}





// (menu item commented out)
static void invoke_bump_to_initial(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                                   ZMapContainerLevelType level, gpointer user_data)
{

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
        ZMapWindowContainerFeatureSet container_set;
        ZMapStyleBumpMode default_mode, initial_mode;

        container_set = (ZMapWindowContainerFeatureSet)container;
        default_mode  = zmapWindowContainerFeatureSetGetDefaultBumpMode(container_set);

        initial_mode  = default_mode;

        zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(container), initial_mode, ZMAPWINDOW_COMPRESS_ALL);

        break;
      }

    default:
      {
        break;
      }
    }

  return ;
}



static void invoke_bump_to_unbump(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                           ZMapContainerLevelType level, gpointer user_data)
{

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
        zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(container), ZMAPBUMP_UNBUMP, ZMAPWINDOW_COMPRESS_ALL);

        break;
      }
    default:
      {
        break;
      }
    }

  return ;
}


/* (expecting column wide features (eg heatmaps) to bump to wiggles but let's not impose this) */
/* change all features in the column to use the bump style or the normal one */
/* hmmm.... we have to assume density style features as displaying normal foo/ zmap_canvas items
 * with an alternate style could involve hacking a huge amount of code which assumes the feature
 * decides what style it should be displayed with. it's that view and model tangle again.
 *
 * we use the column bump_style as a default but allow diff features to specify another,
 * so they could be colour coded for example
 * col style wiil be set if the feature's style is.
 */
static gboolean containerBumpStyle(ZMapWindow window,
                                   ZMapWindowContainerFeatureSet container, gboolean bump)
{
  ZMapFeatureTypeStyle col_style = NULL ;
  GList *l;
  FooCanvasGroup *column_features;

  zMapReturnValIfFail((container && (col_style = zMapWindowContainerFeatureSetGetStyle(container))), FALSE) ;

  if (col_style->bump_style)
    {
      //printf("col style: %d %s -> %s\n", bump, g_quark_to_string(col_style->unique_id), g_quark_to_string(col_style->bump_style));

      if(bump)
        col_style = window->context_map->styles.find_style(zMapStyleCreateID( (char *)g_quark_to_string(col_style->bump_style)));


      /* Umm....hateful coding, truly hateful....Ed */
      if(!col_style)
        return FALSE;


      /* get all the features and try it using canvas_item->set_style() */
      column_features = (FooCanvasGroup *)zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)container) ;

      for (l = column_features->item_list;l;l = l->next)
        {
          ZMapWindowCanvasItem item = (ZMapWindowCanvasItem)(l->data) ;
          ZMapFeature feature ;
          ZMapFeatureTypeStyle style ;
          ZMapFeatureTypeStyle bump_style ;

          if (!(feature = zMapWindowCanvasItemGetFeature((FooCanvasItem *)item)))
            continue ;

          style = *(feature->style) ;
          /* NOTE item contains many features but they must all have the same style */

          bump_style = style;
          //printf("item style: %d %s -> %s\n", bump, g_quark_to_string(col_style->unique_id), g_quark_to_string(style->bump_style));
          if(bump)
            {
              bump_style = window->context_map->styles.find_style(zMapStyleCreateID((char *)g_quark_to_string(style->bump_style)));
              if(!bump_style)
                bump_style = col_style;
            }
          /*! \todo #warning need to recode simple features alignements & trancripts to handle bump_style */
          // best to remove style from feature and use the one in featureset
          // avoid getting confused with featureset in the feature context and canvasfeatureset = column
          // this all relates to GFF based ZMap and the need to set styles OTF

          // NOTE this assumes that all features in the column have the same style
          // orignally this was ok for graph features
          // extending to basic features only (late aug 2012)

          // NOTE must set the featureset style not the feature
          zMapWindowCanvasItemSetStyle(item,bump_style);
          zMapWindowRequestReposition((FooCanvasItem *) item);
        }
    }

  return TRUE;
}

