/*  File: zmapWindowCanvasFeatureset.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * NOTE
 * this module implements the basic handling of featuresets as foo canvas items
 *
 * for basic features (the first type implemented) it runs about 3-5x faster than the foo canvas
 * but note that that does not include expose/ GDK
 * previous code can be used with 'foo=true' in the style
 *
 *  _new code_
 *  deskpro17848[mh17]32: zmap --conf_file=ZMap_bins
 *  # /nfs/users/nfs_m/mh17/.ZMap/ZMap_bins        27/09/2011
 *  Draw featureset basic_1000: 999 features in 0.060 seconds
 *  Draw featureset scored: 9999 features in 0.347 seconds
 *  Draw featureset basic_10000: 9999 features in 0.324 seconds
 *  Draw featureset basic_100000: 99985 features in 1.729 seconds
 *
 *  _old code_
 *  deskpro17848[mh17]33: zmap --conf_file=ZMap_bins
 *  # /nfs/users/nfs_m/mh17/.ZMap/ZMap_bins        27/09/2011
 *  Draw featureset basic_1000: 999 features in 0.165 seconds
 *  Draw featureset scored: 9999 features in 0.894 seconds
 *  Draw featureset basic_10000: 9999 features in 1.499 seconds
 *  Draw featureset basic_100000: 99985 features in 8.968 seconds
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <string.h>

#include <ZMap/zmapUtilsLog.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtilsLog.h>
#include <zmapWindowCanvasBasic.h>
#include <zmapWindowCanvasAlignment.h>
#include <zmapWindowCanvasGraphItem.h>
#include <zmapWindowCanvasTranscript.h>
#include <zmapWindowCanvasAssembly.h>
#include <zmapWindowCanvasSequence.h>
#include <zmapWindowCanvasLocus.h>
#include <zmapWindowCanvasGraphics.h>
#include <zmapWindowCanvasGlyph.h>
#include <zmapWindowCanvasDraw.h>
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasFeature_I.h>


/* format string for printing double positions in a reasonable format... */
#define CANVAS_FORMAT_DOUBLE "%.2f"



/* Used in zMapWindowCanvasFeaturesetGetSetIDAtPos() */
typedef struct FindIDAtPosDataStructType
{
  double position ;
  GQuark id_at_position ;
} FindIDAtPosDataStruct, *FindIDAtPosData ;


static void zmap_window_featureset_item_item_class_init(ZMapWindowFeaturesetItemClass featureset_class) ;
static void zmap_window_featureset_item_item_init(ZMapWindowFeaturesetItem item) ;
static void zmap_window_featureset_item_item_update(FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags) ;
static double zmap_window_featureset_item_foo_point(FooCanvasItem *item,
                                                     double x, double y, int cx, int cy, FooCanvasItem **actual_item) ;
static void zmap_window_featureset_item_item_bounds(FooCanvasItem *item,
                                                     double *x1, double *y1, double *x2, double *y2) ;
static void zmap_window_featureset_item_item_draw(FooCanvasItem *item, GdkDrawable *drawable, GdkEventExpose *expose) ;
static gboolean zmap_window_featureset_item_set_style(FooCanvasItem *item, ZMapFeatureTypeStyle style) ;
static void zmap_window_featureset_item_set_colour(ZMapWindowCanvasItem   thing,
						   FooCanvasItem         *interval,
						   ZMapFeature			feature,
						   ZMapFeatureSubPart sub_feature,
						   ZMapStyleColourType    colour_type,
						   int colour_flags,
						   GdkColor              *default_fill,
						   GdkColor              *border);
static gboolean zmap_window_featureset_item_set_feature(FooCanvasItem *item, double x, double y);
static gboolean zmap_window_featureset_item_show_hide(FooCanvasItem *item, gboolean show);

static void zmap_window_featureset_item_item_destroy(GtkObject *object);

static void zMapWindowCanvasFeaturesetPaintSet(ZMapWindowFeaturesetItem featureset,
                                               GdkDrawable *drawable, GdkEventExpose *expose) ;

static double featurePoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
                           double item_x, double item_y, int cx, int cy,
                           double local_x, double local_y, double x_off) ;
static double graphicsPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
                            double item_x, double item_y, int cx, int cy,
                            double local_x, double local_y, double x_off) ;

static void setFeaturesetColours(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature);

static void featuresetAddToIndex(ZMapWindowFeaturesetItem featureset_item, ZMapWindowCanvasFeature feat) ;

static ZMapSkipList zmap_window_canvas_featureset_find_feature_index(ZMapWindowFeaturesetItem fi,ZMapFeature feature);
static ZMapWindowCanvasFeature zmap_window_canvas_featureset_find_feature(ZMapWindowFeaturesetItem fi,
                                                                          ZMapFeature feature);
static ZMapSkipList zmap_window_canvas_featureset_find_feature_coords(FeatureCmpFunc compare_func,
                                                                      ZMapWindowFeaturesetItem fi,
                                                                      double y1, double y2) ;
static ZMapWindowCanvasFeature findFeatureSubPart(ZMapWindowFeaturesetItem fi,
                                                  ZMapFeature feature,
                                                  ZMapFeatureSubPart sub_feature) ;





void zmap_window_canvas_featureset_expose_feature(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs);
static guint32 gdk_color_to_rgba(GdkColor *color) ;

static void itemLinkSideways(ZMapWindowFeaturesetItem fi) ;
#if NOT_USED
static gint setNameCmp(gconstpointer a, gconstpointer b) ;
#endif

static void printCanvasFeature(void *data, void *user_data_unused) ;

static void findNameAtPosCB(gpointer data, gpointer user_data) ;

static void groupListFreeCB(gpointer data, gpointer user_data_unused) ;



/*
 *                Globals
 */

static FooCanvasItemClass *item_class_G = NULL;
static ZMapWindowCanvasItemClass parent_class_G = NULL;
static ZMapWindowFeaturesetItemClass featureset_class_G = NULL;


/* this is in parallel with the ZMapStyleMode enum */
/* beware, traditionally glyphs ended up as basic features */
/* This is a retro-fit ... i noticed that i'd defined a struct/feature type but not set it as
   originally there was only one  */
static zmapWindowCanvasFeatureType feature_types[N_STYLE_MODE + 1] =
  {
    FEATURE_INVALID,                /* ZMAPSTYLE_MODE_INVALID */

        FEATURE_BASIC,                 /* ZMAPSTYLE_MODE_BASIC */
        FEATURE_ALIGN,                /* ZMAPSTYLE_MODE_ALIGNMENT */
        FEATURE_TRANSCRIPT,        /* ZMAPSTYLE_MODE_TRANSCRIPT */
        FEATURE_SEQUENCE,                /* ZMAPSTYLE_MODE_SEQUENCE */
        FEATURE_ASSEMBLY,                /* ZMAPSTYLE_MODE_ASSEMBLY_PATH */
        FEATURE_LOCUS,                /* ZMAPSTYLE_MODE_TEXT */
        FEATURE_GRAPH,                /* ZMAPSTYLE_MODE_GRAPH */
        FEATURE_GLYPH,                /* ZMAPSTYLE_MODE_GLYPH */

        FEATURE_GRAPHICS,                /* ZMAPSTYLE_MODE_PLAIN */        /* plain graphics, no features eg scale bar */

        FEATURE_INVALID                /* ZMAPSTYLE_MODE_META */
};

/* Sanity check or what ????? */
#if N_STYLE_MODE != FEATURE_N_TYPE
#error N_STYLE_MODE and FEATURE_N_TYPE differ
#endif


/* Tables of function pointers for individual feature types, only required ones have to be provided. */
static gpointer _featureset_set_init_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_prepare_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_set_paint_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_paint_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_flush_G[FEATURE_N_TYPE] = { 0 };

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gpointer _featureset_extent_G[FEATURE_N_TYPE] = { 0 };
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static gpointer _featureset_free_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_pre_zoom_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_zoom_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_point_G[FEATURE_N_TYPE] = { 0 };
static gpointer _featureset_add_G[FEATURE_N_TYPE] = { 0 };

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gpointer _featureset_subpart_G[FEATURE_N_TYPE] = { 0 };
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static gpointer _featureset_colour_G[FEATURE_N_TYPE] = { 0 };



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static long n_block_alloc = 0;
static long n_feature_alloc = 0;
static long n_feature_free = 0;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */







/*
 *                         External routines
 */



/* fetch all the funcs we know about
 * there's no clean way to do this without everlasting overhead per feature
 * NB: canvas featuresets could get more than one type of feature
 * we know how many there are due to the zmapWindowCanvasFeatureType enum
 * so we just do them all, once, from the featureset class init
 * rather tediously that means we have to include headers for each type
 * In terms of OO 'classiness' we regard CanvasFeatureSet as the big daddy
 * that has a few dependant children they dont get to evolve unknown to thier parent.
 *
 * NOTE these functions could be called from the application,
 * before starting the window/ camvas, which would allow extendablilty
 */
void featureset_init_funcs(void)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* set size of unspecified features structs to just the base */
  featureset_class_G->struct_size[FEATURE_INVALID] = sizeof(zmapWindowCanvasFeatureStruct);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  zMapWindowCanvasFeatureInit() ;

  zMapWindowCanvasBasicInit();                /* the order of these may be important */
  zMapWindowCanvasGlyphInit();
  zMapWindowCanvasAlignmentInit();
  zMapWindowCanvasGraphInit();
  zMapWindowCanvasTranscriptInit();
  zMapWindowCanvasAssemblyInit();
  zMapWindowCanvasSequenceInit();
  zMapWindowCanvasLocusInit();
  zMapWindowCanvasGraphicsInit();

  /* if you add a new one then update feature_types[N_STYLE_MODE] above */

  /* if not then graphics modules have to set thier size */
  /* this wastes a bit of memory but uses only one free list and may be safer */

  return ;
}



/* THIS, IT TURNS OUT, IS THE "CREATE" FUNCTION FOR THESE ITEMS....
 *
 * start, end should be doubles....
 *
 * Oh crikey this is over-complicated...why not have an explicit call to create a
 * ZMapWindowCanvasItem and then an explicit call to add ZMapFeatureSets to that
 * item...that would do nicely....
 *
 * return a singleton column wide canvas item
 * just in case we wanted to overlay two or more line graphs we need to allow for more than one
 * graph per column, so we specify these by col_id (which includes strand) and featureset_id
 * we allow for stranded bins, which get fed in via the add function below
 * we also have to include the window to allow more than one -> not accessable so we use the canvas instead
 *
 * we also allow several featuresets to map into the same canvas item via a virtual featureset (quark)
 *
 * NOTE after adding an item we sort the group's item list according to layer.
 * **** This assumes that we only have a few items in the group ****
 * which is ok as it's typically 1, occasionally 2 (focus)  and sometimes serveral eg 4 or 12
 */
ZMapWindowFeaturesetItem zMapWindowCanvasItemFeaturesetGetFeaturesetItem(FooCanvasGroup *parent, GQuark id,
                                                                         GtkAdjustment *v_adjust,
                                                                         int start, int end,
                                                                         ZMapFeatureTypeStyle style,
                                                                         ZMapStrand strand, ZMapFrame frame,
                                                                         int index, guint layer)
{
  ZMapWindowFeaturesetItem featureset_item = NULL;
  zmapWindowCanvasFeatureType type;
  int stagger;
  ZMapWindowFeatureItemSetInitFunc func ;
  FooCanvasItem *foo  = NULL;
  gboolean initialise = FALSE ;

  /* class not initialised till we make an item in foo_canvas_item_new() below */
  if(featureset_class_G && featureset_class_G->featureset_items)
    foo = (FooCanvasItem *)g_hash_table_lookup( featureset_class_G->featureset_items, GUINT_TO_POINTER(id));

  if (foo)
    {
      featureset_item = (ZMapWindowFeaturesetItem)foo ;
      initialise = FALSE ;
    }
  else
    {
      foo = foo_canvas_item_new(parent, ZMAP_TYPE_WINDOW_FEATURESET_ITEM, NULL);
      g_hash_table_insert(featureset_class_G->featureset_items, GUINT_TO_POINTER(id), (gpointer) foo);
      featureset_item = (ZMapWindowFeaturesetItem)foo ;
      featureset_item->type = FEATURE_INVALID ;
      featureset_item->opt = NULL ; /* this one should be treated with some care .... */
      featureset_item->id = id ;
      initialise = TRUE ;
    }

  if (    featureset_item->type == FEATURE_BASIC
       && featureset_item->style
       && featureset_item->style->mode == ZMAPSTYLE_MODE_BASIC
       && style
       && style->mode == ZMAPSTYLE_MODE_ALIGNMENT )
    initialise = TRUE ;


  /*
   * Everything after this point is initialisation of the object...
   * there appears to be only one other memory allocation
   */
  if (initialise)
    {


      /* we record strand and frame for display colours
       * the features get added to the appropriate column depending on strand, frame
       * so we get the right featureset item foo item implicitly
       */
      featureset_item->strand = strand;
      featureset_item->frame = frame;
      featureset_item->style = style;
      featureset_item->draw_truncation_glyphs = FALSE ;

      featureset_item->overlap = TRUE;

      /* column type, style is a combination of all context featureset styles in the column */
      /* main use is for graph density items */
      featureset_item->type = type = feature_types[zMapStyleGetMode(featureset_item->style)];

      if (featureset_item->opt)
        g_free(featureset_item->opt) ;
      if (featureset_class_G->set_struct_size[type])
        featureset_item->opt = g_malloc0(featureset_class_G->set_struct_size[type]);


      /* Maybe these should be subsumed into the feature_set_init_G mechanism..... */
      /* mh17: via the set_init_G function, already moved code from here for link_sideways */

      if (zMapStyleGetDefaultBumpMode(style) == ZMAPBUMP_FEATURESET_NAME)
        {
          featureset_item->link_sideways = TRUE;
        }
      else if(type == FEATURE_ALIGN)
        {
          featureset_item->link_sideways = TRUE;
          /*
             featureset_item->style->default_bump_mode = ZMAPBUMP_ALL ; this should be used for alignments, but is not
             being propagated through to where it is used...
           */
        }
      else if(type == FEATURE_TRANSCRIPT)
        {
          featureset_item->highlight_sideways = TRUE;
        }
      else if(type == FEATURE_GRAPH && zMapStyleDensity(style))
        {
          featureset_item->overlap = FALSE;
          featureset_item->re_bin = TRUE;

          /* this was originally invented for heatmaps & it would be better as generic, but that's another job */
          stagger = zMapStyleDensityStagger(style);
          featureset_item->set_index = index;
          featureset_item->x_off = stagger * featureset_item->set_index;
        }

      featureset_item->x_off += zMapStyleOffset(style);

      featureset_item->width = zMapStyleGetWidth(featureset_item->style);

      /* width is in characters, need to get the sequence code to adjust this */
      if(zMapStyleGetMode(featureset_item->style) == ZMAPSTYLE_MODE_SEQUENCE)
        featureset_item->width *= 10;
      //  if(zMapStyleGetMode(featureset_item->style) == ZMAPSTYLE_MODE_TEXT)
      //    featureset_item->width *= 10;

      featureset_item->start = start;
      featureset_item->end = end;

      /* We need the scrolled window to get at the vertical adjuster to make sure we redraw
       * the entire window after scrolling somewhere because we may have altered the window
       * under the adjusters feet. */
      featureset_item->v_adjuster = v_adjust ;


      /* initialise zoom to prevent double index create on first draw (coverage graphs) */
      featureset_item->recalculate_zoom = FALSE ;
      featureset_item->zoom = foo->canvas->pixels_per_unit_y;
      featureset_item->bases_per_pixel = 1.0 / featureset_item->zoom;

      /* feature type specific code. */
      if ((featureset_item->type > 0 && featureset_item->type < FEATURE_N_TYPE)
          && (func = _featureset_set_init_G[featureset_item->type]))
        func(featureset_item) ;

      featureset_item->layer = layer;

      zMapWindowContainerGroupSortByLayer(parent);

      /* set our bounding box in canvas coordinates to be the whole column */
      foo_canvas_item_request_update (foo);
    }


  return featureset_item ;
}


GString *zMapWindowCanvasFeatureset2Txt(ZMapWindowFeaturesetItem featureset_item)
{
  GString *canvas_featureset_text = NULL ;
  char *indent = "" ;

  zMapReturnValIfFail(featureset_item, NULL) ;

  canvas_featureset_text = g_string_sized_new(2048) ;

  /* Title */
  g_string_append_printf(canvas_featureset_text, "%sZMapWindowFeaturesetItem %p, id = %s)\n",
                         indent, featureset_item, g_quark_to_string(featureset_item->id)) ;

  indent = "  " ;

  g_string_append_printf(canvas_featureset_text, "%sCanvasFeature type: \"%s\"\n",
                         indent, zMapWindowCanvasFeatureType2ExactStr(featureset_item->type)) ;

  g_string_append_printf(canvas_featureset_text, "%sColumn style: \"%s\"\n",
                         indent, zMapStyleGetName(featureset_item->style)) ;

  g_string_append_printf(canvas_featureset_text, "%sCached featureset style: \"%s\"\n",
                         indent, zMapStyleGetName(featureset_item->featurestyle)) ;

  g_string_append_printf(canvas_featureset_text, "%sColumn strand/frame: \"%s\"  \"%s\"\n",
                         indent,
                         zMapFeatureStrand2Str(featureset_item->strand),
                         zMapFeatureFrame2Str(featureset_item->frame)) ;

  if (featureset_item->point_feature)
    g_string_append_printf(canvas_featureset_text, "%spoint_feature ZMapFeature %p \"%s\" (unique id = \"%s\")\n",
                           indent,
                           featureset_item->point_feature,
                           (char *)g_quark_to_string(featureset_item->point_feature->original_id),
                           (char *)g_quark_to_string(featureset_item->point_feature->unique_id)) ;
  else
    g_string_append_printf(canvas_featureset_text, "%spoint_feature ZMapFeature NULL\n", indent) ;

  if (featureset_item->point_canvas_feature)
    g_string_append_printf(canvas_featureset_text, "%spoint_canvas_feature ZMapWindowCanvasFeature %p, type = %s)\n",
                           indent, featureset_item->point_canvas_feature,
                           zMapWindowCanvasFeatureType2ExactStr(featureset_item->point_canvas_feature->type)) ;
  else
    g_string_append_printf(canvas_featureset_text, "%spoint_canvas_feature ZMapWindowCanvasFeature NULL\n", indent) ;

  g_string_append_printf(canvas_featureset_text, "%sPer column data ?: %s\n",
                         indent, ((featureset_item->per_column_data) ? "\"yes\"" : "\"no\"")) ;

  g_string_append_printf(canvas_featureset_text, "%sGlyph per column data ?: %s\n",
                         indent, ((featureset_item->glyph_per_column_data) ? "\"yes\"" : "\"no\"")) ;

  g_string_append_printf(canvas_featureset_text, "%sOpt data ?: %s\n",
                         indent, ((featureset_item->opt) ? "\"yes\"" : "\"no\"")) ;

  g_string_append_printf(canvas_featureset_text, "%sZoom: recalculate = %s, factor/bases per pixel: %g, %g\n",
                         indent,
                         (featureset_item->recalculate_zoom ? "yes" : "no"),
                         featureset_item->zoom, featureset_item->bases_per_pixel) ;

  g_string_append_printf(canvas_featureset_text, "%sStart, End: " CANVAS_FORMAT_DOUBLE ", " CANVAS_FORMAT_DOUBLE "\n",
                         indent, featureset_item->start, featureset_item->end) ;

  g_string_append_printf(canvas_featureset_text, "%sLongest Feature: " CANVAS_FORMAT_DOUBLE "\n",
                         indent, featureset_item->longest) ;

  g_string_append_printf(canvas_featureset_text, "%sx_offset = %g, width = %g, bump_width = %g\n",
                         indent, featureset_item->x_off, featureset_item->width, featureset_item->bump_width) ;

  g_string_append_printf(canvas_featureset_text, "%soverlap ?, bump_overlap: %s " CANVAS_FORMAT_DOUBLE "\n",
                         indent,
                         (featureset_item->overlap ? "\"yes\"" : "\"no\""),
                         featureset_item->bump_overlap) ;

  g_string_append_printf(canvas_featureset_text, "%sBump = %s, mode = %s\n",
                         indent,
                         (featureset_item->bumped ? "yes" : "no"),
                         zmapStyleBumpMode2ExactStr(featureset_item->bump_mode)) ;







  return canvas_featureset_text ;
}


/* Enum -> String functions, these functions convert the enums _directly_ to strings.
 *
 * The functions all have the form
 *  const char *zMapStyleXXXXMode2ExactStr(ZMapStyleXXXXXMode mode)
 *  {
 *    code...
 *  }
 *
 *  */
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zMapWindowCanvasFeatureType2ExactStr, zmapWindowCanvasFeatureType, ZMAP_CANVASFEATURE_TYPE_LIST) ;



/* If there is more than one feature set we should probably return NULL ? Otherwise it's confusing ? */
GQuark zMapWindowCanvasFeaturesetGetSetIDAtPos(ZMapWindowFeaturesetItem fi, double event_x)
{
  GQuark set_id = 0 ;

  /* Get offset in col of y.....perhaps it is that already...check...... */
  if ((fi->sub_col_list))
    {
      FindIDAtPosDataStruct pos_data = {0, 0} ;

      pos_data.position = event_x - fi->dx ;                /* Adjust event x to featureset. */

      g_list_foreach(fi->sub_col_list, findNameAtPosCB, &pos_data) ;

      set_id = pos_data.id_at_position ;
    }

  return set_id ;
}






/* Moved from items/zmapWindowContainerGroup.c */
/* NOTE don-t _EVER_ let this get called by a Foo Canvas callback....YES BUT WHY ??????? */
/* let's use a filter sort here, stability is a good thing and volumes are small */
void zMapWindowContainerGroupSortByLayer(FooCanvasGroup * group)
{
  GList *old;
  ZMapWindowFeaturesetItem item;
  guint layer;
  /* we only implement 3 layers */
  GList *back = NULL, *features = NULL, *overlay = NULL;

  zMapReturnIfFail(group) ;
  if(!ZMAP_IS_CONTAINER_GROUP(group))
    return;

  for(old = group->item_list; old; old = old->next)
    {
      if(!ZMAP_IS_WINDOW_FEATURESET_ITEM(old->data))
        {
          layer = 0;                /* is another group eg a column */
        }
      else
        {
          item = (ZMapWindowFeaturesetItem) old->data;
          layer = zMapWindowCanvasFeaturesetGetLayer(item);
        }

      if(!(layer & ZMAP_CANVAS_LAYER_DECORATION))        /* normal features */
        features = g_list_append(features, old->data);
      else if((layer & ZMAP_CANVAS_LAYER_OVERLAY))
        overlay = g_list_append(overlay, old->data);
      else
        back = g_list_append(back, old->data);
    }

  features = g_list_concat(features, overlay);
  back = g_list_concat(back, features);

  g_list_free(group->item_list);

  group->item_list = back;
  group->item_list_end = g_list_last(back);

  return ;
}






/* each feature type defines its own functions */
/* if they inherit from another type then they must include that type's headers and call code directly */

void zMapWindowCanvasFeatureSetSetFuncs(int featuretype,
                                        gpointer *set_funcs, int set_struct_size)
{

  _featureset_set_init_G[featuretype] = set_funcs[FUNC_SET_INIT];
  _featureset_prepare_G[featuretype] = set_funcs[FUNC_PREPARE];
  _featureset_set_paint_G[featuretype] = set_funcs[FUNC_SET_PAINT];
  _featureset_paint_G[featuretype] = set_funcs[FUNC_PAINT];
  _featureset_flush_G[featuretype] = set_funcs[FUNC_FLUSH];

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  _featureset_extent_G[featuretype] = set_funcs[FUNC_EXTENT];
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  _featureset_colour_G[featuretype] = set_funcs[FUNC_COLOUR];
  _featureset_pre_zoom_G[featuretype] = set_funcs[FUNC_PRE_ZOOM];
  _featureset_zoom_G[featuretype] = set_funcs[FUNC_ZOOM];
  _featureset_free_G[featuretype] = set_funcs[FUNC_FREE];
  _featureset_point_G[featuretype] = set_funcs[FUNC_POINT];
  _featureset_add_G[featuretype] = set_funcs[FUNC_ADD];
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  _featureset_subpart_G[featuretype] = set_funcs[FUNC_SUBPART];
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  featureset_class_G->set_struct_size[featuretype] = set_struct_size;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  featureset_class_G->struct_size[featuretype] = feature_struct_size;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}


gboolean zMapWindowCanvasIsFeatureSet(ZMapWindowFeaturesetItem feature_list)
{
  gboolean is_featureset_item = FALSE ;

  zMapReturnValIfFail(feature_list, is_featureset_item) ;

  if (feature_list->layer == 0)
    is_featureset_item = TRUE ;

  return is_featureset_item ;
}



/* get all the pango stuff we need for a font on a drawable */
void zmapWindowCanvasFeaturesetInitPango(GdkDrawable *drawable,
                                         ZMapWindowFeaturesetItem featureset,
                                         ZMapWindowCanvasPango pango, char *family, int size, GdkColor *draw)
{
  GdkScreen *screen = gdk_drawable_get_screen (drawable);
  PangoFontDescription *desc;
  int width, height;

  zMapReturnIfFail(pango && featureset && featureset->gc) ;

  if(pango->drawable && pango->drawable != drawable)
    zmapWindowCanvasFeaturesetFreePango(pango);

  if(!pango->renderer)
    {
      pango->renderer = gdk_pango_renderer_new (screen);
      pango->drawable = drawable;

      gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (pango->renderer), drawable);

      gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (pango->renderer), featureset->gc);

      /* Create a PangoLayout, set the font and text */
      pango->context = gdk_pango_context_get_for_screen (screen);
      pango->layout = pango_layout_new (pango->context);

      desc = pango_font_description_from_string (family);
      /*! \todo #warning mod this function and call from both places */
#if 0
      /* this must be identical to the one get by the ZoomControl */
      if(zMapGUIGetFixedWidthFont(view,
                                  fixed_font_list, ZMAP_ZOOM_FONT_SIZE, PANGO_WEIGHT_NORMAL,
                                  NULL,desc))
        {
        }
      else
        {
          /* if this fails then we get a proportional font and someone will notice */
          zmapLogWarning("Paint sequence cannot get fixed font","");
        }
#endif

      pango_font_description_set_size (desc,size * PANGO_SCALE);
      pango_layout_set_font_description (pango->layout, desc);
      pango_font_description_free (desc);

      pango_layout_set_text (pango->layout, "a", 1);                /* we need to get the size of one character */
      pango_layout_get_size (pango->layout, &width, &height);
      pango->text_height = height / PANGO_SCALE;
      pango->text_width = width / PANGO_SCALE;

      gdk_pango_renderer_set_override_color (GDK_PANGO_RENDERER (pango->renderer), PANGO_RENDER_PART_FOREGROUND, draw );
    }
}


void zmapWindowCanvasFeaturesetFreePango(ZMapWindowCanvasPango pango)
{
  zMapReturnIfFail(pango) ;

  if(pango->renderer)                /* free the pango renderer if allocated */
    {
      /* Clean up renderer, possiby this is not necessary */
      gdk_pango_renderer_set_override_color (GDK_PANGO_RENDERER (pango->renderer),
                                             PANGO_RENDER_PART_FOREGROUND, NULL);
      gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (pango->renderer), NULL);
      gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (pango->renderer), NULL);

      /* free other objects we created */
      if(pango->layout)
        {
          g_object_unref(pango->layout);
          pango->layout = NULL;
        }

      if(pango->context)
        {
          g_object_unref (pango->context);
          pango->context = NULL;
        }

      g_object_unref(pango->renderer);
      pango->renderer = NULL;
      //                pango->drawable = NULL;
    }
}



/* WHY DO ALL THESE FUNCTIONS HAVE THESE PROTOTYPES....SHOULD BE IN A HEADER SOMEWHERE.... */


/* define feature specific functions here */
/* only access via wrapper functions to allow type checking */

/* handle upstream edge effects converse of flush */
/* feature may be NULL to signify start of data */
void zMapWindowCanvasFeaturesetPaintPrepare(ZMapWindowFeaturesetItem featureset,
                                            ZMapWindowCanvasFeature feature,
                                            GdkDrawable *drawable, GdkEventExpose *expose)
{
  void (*func) (ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                GdkDrawable *drawable, GdkEventExpose *expose) = NULL;
  zMapReturnIfFail(featureset ) ;

  if ((featureset->type > 0 && featureset->type < FEATURE_N_TYPE)
      && (func = _featureset_prepare_G[featureset->type]))
    func(featureset, feature, drawable, expose);

  return ;
}



/* paint one feature, all context needed is in the FeaturesetItem */
/* we need the expose region to clip at high zoom esp with peptide alignments */
void zMapWindowCanvasFeaturesetPaintFeature(ZMapWindowFeaturesetItem featureset,
                                            ZMapWindowCanvasFeature feature,
                                            GdkDrawable *drawable, GdkEventExpose *expose)
{
  void (*func) (ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                GdkDrawable *drawable, GdkEventExpose *expose) = NULL;

  zMapReturnIfFail(feature) ;


  /* NOTE we can have diff types of features in a column eg alignments and basic features in Repeats
   * if we use featureset->style then we get to call the wrong paint function and crash
   * NOTE that if we use PaintPrepare and PaintFlush we require one type of feature only in the column
   * (more complex behavious has not been programmed; alignemnts and basic features don't use this)
   */
  if ((feature->type > 0 && feature->type < FEATURE_N_TYPE)
      && (func = _featureset_paint_G[feature->type]))
    func(featureset, feature, drawable, expose) ;

  return ;
}


/* output any buffered paints: useful eg for poly-line */
/* paint function and flush must access data via FeaturesetItem or globally in thier module */
/* feature is the last feature painted */
void zMapWindowCanvasFeaturesetPaintFlush(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                          GdkDrawable *drawable, GdkEventExpose *expose)
{
  void (*func) (ZMapWindowFeaturesetItem featureset,ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose) = NULL;

  zMapReturnIfFail(featureset) ;

  /* NOTE each feature type has it's own buffer if implemented
   * but  we expect only one type of feature and to handle mull features
   * (which we do to join up lines in gaps between features)
   * we need to use the featureset type instead.
   * we could code this as featureset iff feature is null
   * x-ref with PaintPrepare above
   */

#if 0
  if(feature &&
     (feature->mode > 0 && feature->mode < FEATURE_N_TYPE)
     && (func = _featureset_flush_G[feature->mode]))
#else
    if ((featureset->type > 0 && featureset->type < FEATURE_N_TYPE)
        && (func = _featureset_flush_G[featureset->type]))
#endif
      func(featureset, feature, drawable, expose);

  return;
}


/* Dump to screen information about all the features  */
void zmapWindowCanvasFeaturesetDumpFeatures(ZMapWindowFeaturesetItem featureset)
{
  g_list_foreach(featureset->features, printCanvasFeature, NULL) ;

  return ;
}


void zmapWindowFeaturesetSetSubPartHighlight(ZMapWindowFeaturesetItem featureset, gboolean subpart_highlight)
{
  featureset->highlight_sideways = subpart_highlight ;

  return ;
}




/* interface design driven by exsiting (ZMapCanvasItem/Foo) application code
 *
 * for normal faatures we only set the colour flags
 * for sequence  features we actually supply colours
 */
void zmapWindowFeaturesetItemSetColour(FooCanvasItem *interval,
				       ZMapFeature feature, ZMapFeatureSubPart sub_feature,
				       ZMapStyleColourType colour_type, int colour_flags,
				       GdkColor *default_fill, GdkColor *default_border)
{
  ZMapWindowFeaturesetItem fi = NULL ;
  ZMapWindowCanvasFeature gs ;

  void (*func) (FooCanvasItem         *interval,
		ZMapFeature			feature,
		ZMapFeatureSubPart sub_feature,
		ZMapStyleColourType    colour_type,
		int colour_flags,
		GdkColor              *default_fill,
		GdkColor              *default_border);

  zMapReturnIfFail(interval) ;

  fi = (ZMapWindowFeaturesetItem)interval ;

  func = _featureset_colour_G[fi->type] ;

  gs = findFeatureSubPart(fi, feature, sub_feature) ;

  if (!gs)
    return ;

  if(func)
    {
      zmapWindowCanvasFeatureStruct dummy;

      func(interval, feature, sub_feature, colour_type, colour_flags, default_fill, default_border);

      dummy.width = gs->width;
      dummy.y1 = gs->y1;
      dummy.y2 = gs->y2;
      if(sub_feature && sub_feature->subpart != ZMAPFEATURE_SUBPART_INVALID)
        {
          dummy.y1 = sub_feature->start;
          dummy.y2 = sub_feature->end;
        }
      dummy.bump_offset = gs->bump_offset;

      zmap_window_canvas_featureset_expose_feature(fi, &dummy);
    }
  else
    {
      /* TMP HACK.... */
      if (sub_feature)
        fi->highlight_sideways = FALSE ;


      if(fi->highlight_sideways)        /* ie transcripts as composite features */
        {
          while(gs->left)
            gs = gs->left;
        }

      while(gs)
        {
          /* set the focus flags (on or off) */
          /* zmapWindowFocus.c maintains these and we set/clear then all at once */
          /* NOTE some way up the call stack in zmapWindowFocus.c add_unique()
           * colour flags has a window id set into it
           */

          /* these flags are very fiddly: handle with care */
          gs->flags = ((gs->flags & ~FEATURE_FOCUS_MASK) | (colour_flags & FEATURE_FOCUS_MASK)) |
            ((gs->flags & FEATURE_FOCUS_ID) | (colour_flags & FEATURE_FOCUS_ID)) |
            (gs->flags & FEATURE_FOCUS_BLURRED);

          zmap_window_canvas_featureset_expose_feature(fi, gs);

          /* HACK..... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          if(!fi->highlight_sideways)                /* only doing the selected one */
            return;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
          if(!fi->highlight_sideways)                /* only doing the selected one */
            {
              if (sub_feature)
                fi->highlight_sideways = TRUE ;

              return;
            }


          gs = gs->right;
        }
    }
}




gboolean zMapWindowCanvasFeaturesetAddFeature(ZMapWindowFeaturesetItem featureset,
                                              ZMapFeature feature, double y1, double y2)
{
  gboolean rc = FALSE ;
  ZMapWindowCanvasFeature (*func)(ZMapWindowFeaturesetItem featureset, ZMapFeature feature, double y1, double y2) ;
  ZMapWindowCanvasFeature canvas_feature ;

  zMapReturnValIfFail(featureset, FALSE) ;

  if (feature)
    {
      if ((func = _featureset_add_G[featureset->type]))
        {
          canvas_feature = func(featureset, feature, y1, y2) ;
        }
      else
        {
          if ((canvas_feature = zMapWindowFeaturesetAddFeature(featureset, feature, y1, y2)))
            zMapWindowFeaturesetSetFeatureWidth(featureset, canvas_feature) ;
        }

      if (canvas_feature)
        {
          featureset->last_added = canvas_feature ;
          rc = TRUE ;
        }
    }

  return rc ;
}


/* if a featureset has any allocated data free it */
void zMapWindowCanvasFeaturesetFree(ZMapWindowFeaturesetItem featureset)
{
  ZMapWindowFeatureFreeFunc func ;

  zMapReturnIfFail(featureset) ;

  if ((featureset->type > 0 && featureset->type < FEATURE_N_TYPE)
      && (func = _featureset_free_G[featureset->type]))
    func(featureset) ;

  return;
}

gboolean zMapWindowCanvasFeaturesetHasPointFeature(FooCanvasItem *item)
{
  gboolean result = FALSE ;
  ZMapWindowFeaturesetItem featureset = (ZMapWindowFeaturesetItem)item ;

  zMapReturnValIfFail(ZMAP_IS_WINDOW_FEATURESET_ITEM(item), FALSE) ;

  /* Not sure how much checking we should do here...should we check the actual feature ptr...? */
  if (featureset->point_feature)
    {
      result = TRUE ;
    }

  return result ;
}


ZMapFeature zMapWindowCanvasFeaturesetGetPointFeature(ZMapWindowFeaturesetItem featureset_item)
{
  ZMapFeature feature = NULL ;

  zMapReturnValIfFail(ZMAP_IS_WINDOW_FEATURESET_ITEM(featureset_item), FALSE) ;

  /* Not sure how much checking we should do here...should we check the actual feature ptr...? */
  if (featureset_item->point_feature)
    {
      feature = featureset_item->point_feature ;
    }

  return feature ;
}



/* For normal clicking by user the point_feature is reset automatically but
 * features can be removed programmatically (i.e. xremote) requiring the
 * point_feature to be unset as the feature it points to is no longer valid. */
gboolean zMapWindowCanvasFeaturesetUnsetPointFeature(FooCanvasItem *item)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail(item, FALSE) ;

  if (ZMAP_IS_WINDOW_FEATURESET_ITEM(item))
    {
      ZMapWindowFeaturesetItem featureset = (ZMapWindowFeaturesetItem) item ;

      if (featureset->point_feature)
        {
          featureset->point_canvas_feature = NULL ;
          featureset->point_feature = NULL ;
          result = TRUE ;
        }
    }

  return result ;
}



ZMapWindowCanvasFeature zMapWindowCanvasFeaturesetGetPointFeatureItem(ZMapWindowFeaturesetItem featureset_item)
{
  ZMapWindowCanvasFeature feature_item = NULL ;

  zMapReturnValIfFail(ZMAP_IS_WINDOW_FEATURESET_ITEM(featureset_item), FALSE) ;

  /* Not sure how much checking we should do here...should we check the actual feature ptr...? */
  if (featureset_item->point_canvas_feature)
    {
      feature_item = featureset_item->point_canvas_feature ;
    }

  return feature_item ;
}


zmapWindowCanvasFeatureType zMapWindowCanvasFeaturesetGetFeatureType(ZMapWindowFeaturesetItem featureset_item)
{
  zmapWindowCanvasFeatureType canvas_feature_type ;

  canvas_feature_type = featureset_item->type ;


  return canvas_feature_type ;
}




/* Do anythings we need to _before_ the zooming of features. */
void zMapWindowCanvasFeaturesetPreZoom(ZMapWindowFeaturesetItem featureset)
{

  if (featureset)
    {
      ZMapWindowFeatureItemPreZoomFunc func ;

      if ((featureset->type > 0 && featureset->type < FEATURE_N_TYPE)
          && (func = _featureset_pre_zoom_G[featureset->type]))
        {
          func(featureset) ;
        }
    }

  return ;
}


/*
 * do anything that needs doing on zoom
 * we have column specific things like are features visible
 * and feature specific things like recalculate gapped alignment display
 * NOTE this is also called on the first paint to create the index
 * it must create the index if it's not there and this may be done by class Zoom functions
 */
void zMapWindowCanvasFeaturesetZoom(ZMapWindowFeaturesetItem featureset, GdkDrawable *drawable)
{
  ZMapSkipList sl;
  long trigger = 0 ;
  PixRect pix = NULL;

  zMapReturnIfFail(featureset ) ;

  trigger = (long) zMapStyleGetSummarise(featureset->style);

  if (featureset)
    {
      ZMapWindowFeatureItemZoomFunc func ;

      if(!featureset->display_index)
        zMapWindowCanvasFeaturesetIndex(featureset);

      if ((featureset->type > 0 && featureset->type < FEATURE_N_TYPE)
          && (func = _featureset_zoom_G[featureset->type]))
        {
          /* zoom can require actions like (re)create the index eg if graphs density stuff gets re-binned */
          func(featureset, drawable) ;
        }


      /* column summarise: after creating the index work out which features are visible and hide the rest */
      /* set for basic features and alignments */
      /* this could be moved to the class functions but as it's used more generally not worth it */

      if (!featureset->bumped && !featureset->re_bin && trigger && featureset->n_features >= trigger)
        {
          /*
            on min zoom we have nominally 1000 pixels so if we have 1000 features we should get some overlap
            depends on the sequence length, longer sequence means more overlap (and likely more features
            but simple no of features is easy to work with.
            if we used 100 features out of sheer exuberance
            then the chance of wasted CPU is small as the data is small
            at high zoom we still do it as overlap is still overlap (eg w/ high coverage BAM regions)
          */

          /* NOTE: for faster code just process features overlapping the visible scroll region */
          /* however on current volumes (< 200k normally) it makes little difference */
          for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
            {
              ZMapWindowCanvasFeature feature = (ZMapWindowCanvasFeature) sl->data;

              pix = zmapWindowCanvasFeaturesetSummarise(pix,featureset,feature);
            }

          /* clear up */
          zmapWindowCanvasFeaturesetSummariseFree(featureset, pix);
        }
    }

  return;
}



/* This has become a non-function...what on earth is going on here.... */
void  zmap_window_featureset_item_item_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
  double minx,miny,maxx,maxy;

  minx = item->x1;
  miny = item->y1;
  maxx = item->x2;
  maxy = item->y2;

#if 0
  /* Make the bounds be relative to our parent's coordinate system */
  if (item->parent)
    {
      FooCanvasGroup *group = FOO_CANVAS_GROUP (item->parent);

      minx += group->xpos;
      miny += group->ypos;
      maxx += group->xpos;
      maxy += group->ypos;
    }
#endif

  *x1 = minx;
  *y1 = miny;
  *x2 = maxx;
  *y2 = maxy;
}




void zMapWindowCanvasFeaturesetIndex(ZMapWindowFeaturesetItem fi)
{
  GList *features;

  zMapReturnIfFail(fi) ;

  /*
   * this call has to be here as zMapWindowCanvasFeaturesetIndex() is called from bump,
   * which can happen before we get a paint i tried to move it into alignments
   * (it's a bodge to cope with the data being shredded before we get it)
   */
  if (fi->link_sideways && !fi->linked_sideways)
    itemLinkSideways(fi) ;

  features = fi->display;                /* NOTE: is always sorted */

  /* the link_sideways call above sets features_sorted to FALSE I guess to trigger this
   * but why !!!! */
  if (!fi->features_sorted)
    {
      //printf("sort index\n");
      fi->features = g_list_sort(fi->features, zMapWindowFeatureCmp) ;
      fi->features_sorted = TRUE;
    }

  if (!features)                                /* was not pre-processed */
    features = fi->features;

  fi->display_index = zMapSkipListCreate(features, NULL) ;

  return ;
}


/* A disturbing element of this routine are the references to graph....this shouldn't be exposed
 * at this level.... */
static void zmap_window_featureset_item_item_draw(FooCanvasItem *item, GdkDrawable *drawable, GdkEventExpose *expose)
{
  ZMapSkipList sl;
  ZMapWindowCanvasFeature feat = NULL;
  double y1,y2;
  double width;
  GList *highlight = NULL;        /* must paint selected on top ie last */
  gboolean is_line = FALSE, is_graphic = FALSE ;
  ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem)item;
  //gboolean debug = FALSE;
  GdkRegion *region;
  GdkRectangle rect;
  GtkAdjustment *v_adjust ;


  /* get visible scroll region in gdk coordinates to clip features that
   * overlap and possibly extend beyond actual scroll
   * this avoids artifacts due to wrap round
   * NOTE we cannot calc this post zoom as we get scroll afterwards
   * except possibly if we combine the zoom and scroll operation
   * but this code cannot assume that
   *
   * ACTUALLY SOMETHING GOES WRONG HERE...STUFF GET'S CLIPPED WHEN IT SHOULDN'T IN THE CASE WHERE
   * WE'VE SCROLLED OFF THE CURRENT POSITION OF THE FOO CANVAS WINDOW......
   *
   *
   */

  region = gdk_drawable_get_visible_region(drawable);
  gdk_region_get_clipbox ((const GdkRegion *) region, &rect);
  gdk_region_destroy(region);


  v_adjust = fi->v_adjuster ;

  if (rect.height < v_adjust->page_size)
    rect.height = v_adjust->page_size + 1000 ;                    /* hack...try it.... */


  fi->clip_x1 = rect.x - 1;
  fi->clip_y1 = rect.y - 1;

  fi->clip_x2 = rect.x + rect.width + 1;
  fi->clip_y2 = rect.y + rect.height + 1;


  /* UM....WHY NOT DO THIS AT THE BEGINNING...DUH..... */
  if(!fi->gc && (item->object.flags & FOO_CANVAS_ITEM_REALIZED))
    fi->gc = gdk_gc_new (item->canvas->layout.bin_window);


  /* check zoom level and recalculate */
  /* NOTE this also creates the index if needed */
  if(!fi->display_index || fi->recalculate_zoom)
    {
      fi->recalculate_zoom = FALSE;
      fi->bases_per_pixel = 1.0 / fi->zoom;
      zMapWindowCanvasFeaturesetZoom(fi, drawable) ;
    }

  /* paint all the data in the exposed area */

  //  width = zMapStyleGetWidth(fi->style) - 1;                /* off by 1 error! width = #pixels not end-start */
  width = fi->width;

  /*
   *        get the exposed area
   *        find the top (and bottom?) items
   *        clip the extremes and paint all
   */

  fi->dx = fi->dy = 0.0;                /* this gets the offset of the parent of this item */
  foo_canvas_item_i2w (item, &fi->dx, &fi->dy);
  /* ref to zMapCanvasFeaturesetDrawBoxMacro to see how seq coords map to world coords and then canvas coords */

  /*
   * (sm23) The correction here extends the region that will be used to signal a redraw of
   * the feature in canvas coordinates. It is necessary because the glyphs are drawn
   * as part of the feature paint functions, but are not within the features' coordinates.
   * The choice of value here is a little arbitrary as there is not hard limit on
   * glyph sizes and these quantities are not easily queried.
   */
  int pixel_correction = 10 ;
  foo_canvas_c2w(item->canvas,0,floor(expose->area.y - 1 - pixel_correction),NULL,&y1);
  foo_canvas_c2w(item->canvas,0,ceil(expose->area.y + expose->area.height + 1 + pixel_correction),NULL,&y2);

#if 0

  if(fi->type >= FEATURE_GRAPHICS)
    //if(!fi->features)
    {
      ZMapWindowContainerFeatureSet column;
      GdkRectangle *area = &expose->area;

      printf("expose %p %s %.1f,%.1f (%d %d, %d %d) %ld features\n", item->canvas, g_quark_to_string(fi->id),
             y1, y2, area->x, area->y, area->width, area->height, fi->n_features);

      column =  (ZMapWindowContainerFeatureSet) ((ZMapWindowCanvasItem) item)->__parent__.parent;
      //        if(ZMAP_IS_CONTAINER_FEATURESET(column))
      //                printf("painting column %s\n", g_quark_to_string(zmapWindowContainerFeatureSetGetColumnId(column)));
    }
#endif




  /* Do featureset specific painting.....like the mark !! */
  zMapWindowCanvasFeaturesetPaintSet(fi, drawable, expose) ;


  /* THESE RETURNS MAKE ME NERVOUS...SUGGESTS THAT THE PRECONDITIONS FOR THIS
   * ROUTINE HAVE NOT BEEN ANALYSED SUFFICIENTLY.... */

  /* could be an empty column or a mistake */
  if(!fi->display_index)
    return ;

  //if(zMapStyleDisplayInSeparator(fi->style)) debug = TRUE;

  sl = zmap_window_canvas_featureset_find_feature_coords(NULL, fi, y1, y2);
  //if(debug) printf("draw %s        %f,%f: %p\n",g_quark_to_string(fi->id),y1,y2,sl);

  if(!sl)
    return;


  /* Handle graphics prepaint, line drawing etc. */
  /* we have already found the first matching or previous item */
  /* get the previous one to handle wiggle plots that must go off screen */
  if (zMapStyleGetMode(fi->style) == ZMAPSTYLE_MODE_GRAPH)
    {
      is_graphic = TRUE ;

      if (fi->style->mode_data.graph.mode == ZMAPSTYLE_GRAPH_LINE)
        is_line = TRUE ;
    }


  if(is_line)
    {
      feat = sl->prev ? (ZMapWindowCanvasFeature) sl->prev->data : NULL;
    }

  if (is_graphic)
    {
      zMapWindowCanvasFeaturesetPaintPrepare(fi, feat, drawable, expose) ;
    }


  for (fi->featurestyle = NULL ; sl ; sl = sl->next)
    {
      feat = (ZMapWindowCanvasFeature) sl->data;

      /*
        if(debug && feat->feature)
        printf("feat: %s %lx %f %f\n",g_quark_to_string(feat->feature->unique_id), feat->flags,
        feat->y1,feat->y2);
      */

      if(!is_line && (feat->y1-y2 > 1.0)) //feat->y1 > y2)                /* for lines we have to do one more */
        break;        /* finished */

      /*
       * This test is really to see if the coordinates differ by more than one base, BUT
       * this is only true if y2 - y1 > 1.0 since they are both double values.
       */
      if ((y1-feat->y2) > 1.0 )
        {
          /* if bumped and complex then the first feature does the join up lines */
          if(!fi->bumped || feat->left)
            continue;
        }

      if (feat->type < FEATURE_GRAPHICS && (feat->flags & FEATURE_HIDDEN))
        continue;

      /* when bumped we can have a sequence wide 'bump_overlap
       * which means we could try to paint all the features
       * which would be slow
       * so we need to test again for the expose region and not call gdk
       * for alignments the first feature in a set has the colinear lines and we clip in that paint function too
       */
      /* erm... already did that */

      /*
        NOTE need to sort out container positioning to make this work
        di covers its container exactly, but is it offset??
        by analogy w/ old style ZMapWindowCanvasItems we should display
        'intervals' as item relative
      */

      /* we don't display focus on lines */
      if (feat->type < FEATURE_GRAPHICS && !is_line && (feat->flags & FEATURE_FOCUS_MASK))
        {
          highlight = g_list_prepend(highlight, feat) ;
          continue ;
        }

      /* clip this one (GDK does that? or is it X?) and paint */
      //if(debug) printf("paint %d-%d\n",(int) feat->y1,(int) feat->y2);

      /* set style colours if they changed */
      if(feat->type < FEATURE_GRAPHICS)
        setFeaturesetColours(fi, feat) ;

      // call the paint function for the feature

      /* gb10: hack to redraw the entire screen area for the show translation column. I don't understand
       * why but it is blanking out some areas on scrolling (where the expose area is just the new
       * bit of the view that has been scrolled into view. */
      static GQuark show_translation_id = 0 ;
      if (!show_translation_id)
        show_translation_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME) ;

      if (fi && fi->featurestyle && fi->featurestyle->unique_id == show_translation_id && item && item->canvas)
        {
          int diff = item->canvas->layout.container.widget.allocation.height - expose->area.height ;

          if (diff > 0)
            {
              if (diff > expose->area.y - 1)
                diff = expose->area.y - 1 ;

              expose->area.height += diff ;
              expose->area.y -= diff ;
            }
        }

      zMapWindowCanvasFeaturesetPaintFeature(fi,feat,drawable,expose) ;

      if(feat->y1 > y2)                                     /* for lines we have to do one more */
        break ;                                             /* finished */
    }

  /* flush out any stored data (eg if we are drawing polylines) */
  zMapWindowCanvasFeaturesetPaintFlush(fi, sl ? feat : NULL, drawable, expose);

  if (!is_line && highlight)
    {
      // NOTE type will be < FEATURE_GRAPHICS for all items in the list

      highlight = g_list_reverse(highlight);        /* preserve normal display ordering */

      /* now paint the focus features on top, clear style to force colours lookup */
      for(fi->featurestyle = NULL;highlight;highlight = highlight->next)
        {
          feat = (ZMapWindowCanvasFeature) highlight->data;

          setFeaturesetColours(fi,feat);
          zMapWindowCanvasFeaturesetPaintFeature(fi,feat,drawable,expose);
        }

      zMapWindowCanvasFeaturesetPaintFlush(fi, feat ,drawable, expose);
    }

#if MOUSE_DEBUG
  zMapLogWarning("expose completes","");
#endif


  return ;
}



/* called by item drawing code, we cache style colours hoping it will run faster */
/* see also zmap_window_canvas_alignment_get_colours() */
int zMapWindowCanvasFeaturesetGetColours(ZMapWindowFeaturesetItem featureset,
                                         ZMapWindowCanvasFeature feature,
                                         gulong *fill_pixel, gulong *outline_pixel)
{
  int ret = 0;

  zMapReturnValIfFail(featureset && feature && fill_pixel && outline_pixel, ret ) ;

  if (zmapWindowCanvasFeatureValid(feature))
    {
      ret = zMapWindowFocusCacheGetSelectedColours(feature->flags, fill_pixel, outline_pixel);

      if(!(ret & WINDOW_FOCUS_CACHE_FILL))
        {
          if (featureset->fill_set)
            {
              *fill_pixel = featureset->fill_pixel;
              ret |= WINDOW_FOCUS_CACHE_FILL;
            }
        }

      if(!(ret & WINDOW_FOCUS_CACHE_OUTLINE))
        {
          if (featureset->outline_set)
            {
              *outline_pixel = featureset->outline_pixel;
              ret |= WINDOW_FOCUS_CACHE_OUTLINE;
            }
        }
    }

  return ret;
}


/* HACK WHILE I GET SPLICE STUFF WORKING....NEED TO ADD SPLICE IN PROPERLY...TO STYLE AND EVERYTHING... */
gboolean zMapWindowCanvasFeaturesetGetSpliceColour(ZMapWindowFeaturesetItem featureset, gboolean match,
                                                   gulong *border_pixel, gulong *fill_pixel)
{
  gboolean result = FALSE ;
  static gboolean splice_allocated = FALSE ;
  static GdkColor border_colour ;
  static GdkColor match_fill_colour ;
  static GdkColor non_match_fill_colour ;

  if (!splice_allocated)
    {
      if (((result = gdk_color_parse("black", &border_colour))
           && (result = gdk_colormap_alloc_color(gdk_colormap_get_system(), &border_colour, FALSE, TRUE)))
          && ((result = gdk_color_parse("green", &match_fill_colour))
              && (result = gdk_colormap_alloc_color(gdk_colormap_get_system(), &match_fill_colour, FALSE, TRUE)))
          && ((result = gdk_color_parse("red", &non_match_fill_colour))
              && (result = gdk_colormap_alloc_color(gdk_colormap_get_system(), &non_match_fill_colour, FALSE, TRUE))))
        {
          splice_allocated = TRUE ;
        }
    }

  if (splice_allocated)
    {
      *border_pixel = border_colour.pixel ;

      if (match)
        *fill_pixel = match_fill_colour.pixel ;
      else
        *fill_pixel = non_match_fill_colour.pixel ;
      result = TRUE ;
    }

  return result ;
}







/* show / hide all masked features in the CanvasFeatureset
 * this just means setting a flag
 *
 * unless of course we are re-masking and have to delete items if no colour is defined
 * in which case we destroy the index to force a rebuild: slow but not run very often
 * original foo code actually deleted the features, we will just flag them away.
 * feature homol flags displayed means 'is in foo'
 * so we could have to post process the featureset->features list
 * to delete then and then rebuild the index (skip list)
 * if the colour is not defined we should not have the show/hide menu item
 */
void zMapWindowCanvasFeaturesetShowHideMasked(FooCanvasItem *foo, gboolean show, gboolean set_colour)
{
  ZMapWindowFeaturesetItem featureset = (ZMapWindowFeaturesetItem) foo;
  ZMapSkipList sl;
  gboolean delete = FALSE;
  int has_colours;

  zMapReturnIfFail(foo) ;

  has_colours = zMapWindowFocusCacheGetSelectedColours(WINDOW_FOCUS_GROUP_MASKED, NULL, NULL);
  delete = !has_colours;

  for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
    {
      ZMapWindowCanvasFeature feature = (ZMapWindowCanvasFeature) sl->data;        /* base struct of all features */

      if(feature->type == FEATURE_ALIGN && feature->feature->feature.homol.flags.masked)
        {
          if(set_colour)      /* called on masking by another featureset */
            {
              feature->flags |= focus_group_mask[WINDOW_FOCUS_GROUP_MASKED];
            }

          if(set_colour && delete)
            {
              feature->feature->feature.homol.flags.displayed = FALSE;
              feature->flags |= FEATURE_MASK_HIDE | FEATURE_HIDDEN;
            }
          else if(show)
            {
              feature->flags &= ~FEATURE_MASK_HIDE;
              /* this could get complicated combined with summarise */
              if(!(feature->flags & FEATURE_HIDE_REASON))
                {
                  feature->flags &= ~FEATURE_HIDDEN;
                  //                                        feature->feature.homol.flags.displayed = TRUE;        /* legacy, should net be needed */
                }
            }
          else
            {
              feature->flags |= FEATURE_MASK_HIDE | FEATURE_HIDDEN;
              //                                feature->feature.homol.flags.displayed = FALSE;        /* legacy, should net be needed */
            }
        }
    }
#if MH17_NOT_IMPLEMENTED
  if(delete)
    {
      scan featureset->features, delete masked features with homol.flags/displayed == FALSE
        destroy the index to force a rebuild
        }
#endif
}




int get_heat_rgb(int a,int b,double score)
{
  int val = b - a;

  val = a + (int) (val * score);
  if(val < 0)
    val = 0;
  if(val > 0xff)
    val = 0xff;
  return(val);
}

/* find an RGBA pixel value between a and b */
/* NOTE foo canvas and GDK have got in a tangle with pixel values and we go round in circle to do this
 * but i acted dumb and followed the procedures (approx) used elsewhere
 */
gulong zMapWindowCanvasFeatureGetHeatColour(gulong a, gulong b, double score)
{
  int ar,ag,ab;
  int br,bg,bb;
  gulong colour;

  a >>= 8;                /* discard alpha */
  ab = a & 0xff; a >>= 8;
  ag = a & 0xff; a >>= 8;
  ar = a & 0xff; a >>= 8;

  b >>= 8;
  bb = b & 0xff; b >>= 8;
  bg = b & 0xff; b >>= 8;
  br = b & 0xff; b >>= 8;

  colour = 0xff;
  colour |= get_heat_rgb(ab,bb,score) << 8;
  colour |= get_heat_rgb(ag,bg,score) << 16;
  colour |= get_heat_rgb(ar,br,score) << 24;

  return(colour);
}



/* paint set-level features, e.g. graph base lines etc. */
static void zMapWindowCanvasFeaturesetPaintSet(ZMapWindowFeaturesetItem fi,
                                               GdkDrawable *drawable, GdkEventExpose *expose)
{
  ZMapWindowFeatureItemSetPaintFunc func ;
  FooCanvasItem * foo = (FooCanvasItem *) fi;
  gint x1, x2, y1, y2;
  GdkColor c;

  zMapReturnIfFail(fi) ;

  x1 = (gint) foo->x1;
  x2 = (gint) foo->x2;
  y1 = (gint) foo->y1;
  y2 = (gint) foo->y2;

  /* NB: clip region is 1 bigger than the expose to avoid drawing spurious lines */

  if(x1 < fi->clip_x1)
    x1 = fi->clip_x1;
  if(x2 > fi->clip_x2)
    x2 = fi->clip_x2;
  if(y1 < fi->clip_y1)
    y1 = fi->clip_y1;
  if(y2 > fi->clip_y2)
    y2 = fi->clip_y2;


  /* You can't use zMapCanvasFeaturesetDrawBoxMacro() here because it converts from world to
   * canvas and these coords are already canvas. */
  if(fi->background_set)
    {

      c.pixel = fi->background;
      gdk_gc_set_foreground (fi->gc, &c);

      if(fi->stipple)
        {
          gdk_gc_set_stipple (fi->gc, fi->stipple);
          gdk_gc_set_fill (fi->gc, GDK_STIPPLED);
        }
      else
        {
          gdk_gc_set_fill (fi->gc, GDK_SOLID);
        }


      zMap_draw_rect(drawable, fi, x1, y1, x2, y2, TRUE);
    }

  if (fi->border_set)
    {
      /* Is this EVER called...????? */

      c.pixel = fi->border;
      gdk_gc_set_foreground (fi->gc, &c);
      gdk_gc_set_fill (fi->gc, GDK_SOLID);

      zMap_draw_rect(drawable, fi, x1, y1, x2, y2, FALSE);
    }



  if ((fi->type > 0 && fi->type < FEATURE_N_TYPE) &&(func = _featureset_set_paint_G[fi->type]))
    func(fi, drawable, expose) ;

  return ;
}





/* UM...THIS IS ALREADY IN ANOTHER PLACE....WHY THE REPLICATION..... */

/* Converts a sequence extent into a canvas extent.
 *
 *
 * sequence coords:           1  2  3  4  5  6  7  8
 *
 *                           |__|__|__|__|__|__|__|__|
 *
 *                           |                       |
 * canvas coords:           1.0                     9.0
 *
 * i.e. when we actually come to draw it we need to go one _past_ the sequence end
 * coord because our drawing needs to draw in the whole of the last base.
 *
 */
void zmapWindowFeaturesetS2Ccoords(double *start_inout, double *end_inout)
{
  if (!start_inout || !end_inout || !(*start_inout <= *end_inout))
    return ;

  *end_inout += 1 ;

  return ;
}



GType zMapWindowFeaturesetItemGetType(void)
{
  static GType group_type = 0 ;

  if (!group_type)
    {
      static const GTypeInfo group_info = {
        sizeof(ZMapWindowFeaturesetItemClassStruct),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) zmap_window_featureset_item_item_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (ZMapWindowFeaturesetItemStruct),
        0,              /* n_preallocs */
        (GInstanceInitFunc) zmap_window_featureset_item_item_init,
        NULL
      };

      group_type = g_type_register_static(zMapWindowCanvasItemGetType(),
                                          ZMAP_WINDOW_FEATURESET_ITEM_NAME,
                                          &group_info,
                                          0) ;
    }

  return group_type;
}


void zMapWindowCanvasItemFeaturesetSetVAdjust(ZMapWindowFeaturesetItem featureset, GtkAdjustment *v_adjust)
{
  featureset->v_adjuster = v_adjust ;

  return ;
}


guint zMapWindowCanvasFeaturesetGetId(ZMapWindowFeaturesetItem featureset)
{
  return(featureset->id);
}


void zMapWindowCanvasFeaturesetSetZoomRecalc(ZMapWindowFeaturesetItem featureset, gboolean recalc)
{
  featureset->recalculate_zoom = recalc ;

  return ;
}

void zMapWindowCanvasFeaturesetSetZoomY(ZMapWindowFeaturesetItem fi, double zoom_y)
{
  /* If the zoom has changed, we need to set a flag so that we recalculate
   * featureset summary data after the zoom has been completed. */
  zMapReturnIfFail(fi) ;

  if (fi->zoom != zoom_y)
    {
      fi->recalculate_zoom = TRUE ;
      fi->zoom = zoom_y ;
    }

  return ;
}





void zMapWindowCanvasFeaturesetSetStipple(ZMapWindowFeaturesetItem featureset, GdkBitmap *stipple)
{
  zMapReturnIfFail(featureset) ;
  featureset->stipple = stipple;
  foo_canvas_item_request_redraw((FooCanvasItem *) featureset);
}

/* scope issues.... */
void zMapWindowCanvasFeaturesetSetWidth(ZMapWindowFeaturesetItem featureset, double width)
{
  zMapReturnIfFail(featureset) ;
  featureset->width = width;
}

double zMapWindowCanvasFeaturesetGetWidth(ZMapWindowFeaturesetItem featureset)
{
  zMapReturnValIfFail(featureset, 0.0) ;
  if(featureset->bumped)
    return featureset->bump_width;
  else
    return featureset->width;

}

double zMapWindowCanvasFeaturesetGetOffset(ZMapWindowFeaturesetItem featureset)
{
  return featureset->x_off;
}


void zMapWindowCanvasFeaturesetSetSequence(ZMapWindowFeaturesetItem featureset, double y1, double y2)
{
  zMapReturnIfFail(featureset) ;
  featureset->start = y1;
  featureset->end = y2;
}




void zMapWindowCanvasFeaturesetSetBackground(FooCanvasItem *foo, GdkColor *fill, GdkColor * outline)
{
  ZMapWindowFeaturesetItem featureset = (ZMapWindowFeaturesetItem) foo;
  gulong pixel;

  zMapReturnIfFail(foo) ;

  featureset->background_set = featureset->border_set = FALSE;

  if(fill)
    {
      pixel = zMap_gdk_color_to_rgba(fill);
      featureset->background = foo_canvas_get_color_pixel(foo->canvas, pixel);
      featureset->background_set = TRUE;
    }
  if(outline)
    {
      pixel = zMap_gdk_color_to_rgba(outline);
      featureset->border = foo_canvas_get_color_pixel(foo->canvas, pixel);
      featureset->border_set = TRUE;
    }

  // call from caller: this gives exposes between drawing featuresets
  //        zMapWindowCanvasFeaturesetRedraw(featureset, featureset->zoom);
}




guint zMapWindowCanvasFeaturesetGetLayer(ZMapWindowFeaturesetItem featureset)
{
  zMapReturnValIfFail(featureset, (gint)0) ;
  return(featureset->layer);
}



#if EVER_CALLED
void zMapWindowCanvasFeaturesetSetLayer(ZMapWindowFeaturesetItem featureset, guint layer)
{
  featureset->layer = layer;
  zMapWindowContainerGroupSortByLayer(((FooCanvasItem *) featureset)->parent);
}
#endif



/* this is a nest of incestuous functions that all do the same thing
 * but at least this way we keep the search criteria in one place not 20
 */
static ZMapSkipList zmap_window_canvas_featureset_find_feature_coords(FeatureCmpFunc compare_func,
                                                                      ZMapWindowFeaturesetItem fi,
                                                                      double y1, double y2)
{
  ZMapSkipList sl = NULL;
  zmapWindowCanvasFeatureStruct search;
  double extra = 0.0;

  zMapReturnValIfFail(fi, NULL) ;

  extra = fi->longest;

  if (!compare_func)
    compare_func = zMapWindowFeatureCmp;

  search.y1 = y1;
  search.y2 = y2;

  if(fi->overlap)
    {
      if(fi->bumped)
        extra =  fi->bump_overlap;

      /* glyphs are fixed size so expand/ contract according to zoom, fi->longest is in canvas pixel coordinates  */
      if(fi->style->mode == ZMAPSTYLE_MODE_GLYPH)
        foo_canvas_c2w(((FooCanvasItem *) fi)->canvas, 0, ceil(extra), NULL, &extra);

      search.y1 -= extra;

      // this is harmelss and otherwise prevents features overlapping the featureset being found
      //      if(search.y1 < fi->start)
      //                search.y1 = fi->start;
    }

  sl =  zMapSkipListFind(fi->display_index, compare_func, &search) ;
  //        if(sl->prev)
  //                sl = sl->prev;        /* in case of not exact match when rebinned... done by SkipListFind */

  return sl;
}


static ZMapSkipList zmap_window_canvas_featureset_find_feature_index(ZMapWindowFeaturesetItem fi, ZMapFeature feature)
{
  ZMapWindowCanvasFeature gs;
  ZMapSkipList sl = NULL ;

  zMapReturnValIfFail(feature, sl) ;

  sl = zmap_window_canvas_featureset_find_feature_coords(NULL, fi, feature->x1, feature->x2);

  /* find the feature's feature struct but return the skip list */

  while(sl)
    {
      gs = sl->data;

      /* if we got rebinned then we need to find the bin surrounding the feature
         if the feature is split bewteeen bins just choose one
      */
      if(gs->y1 > feature->x2)
        return NULL;

      /* don't we know we have a feature and item that both exist?
         There must have been a reason for this w/ DensityItems */
#if 1 // NEVER_REBINNED
      if(gs->feature == feature)
#else
        /*
         *        if we re-bin variable sized features then we could have features extending beyond bins
         *        in which case a simple containment test will fail
         *        so we have to test for overlap, which will also handle the simpler case.
         *        bins have a real feature attached and this is fed in from the cursor code (point function)
         *        this argument also applies to fixed size bins: we pro-rate overlaps when we calculate bins
         *        according to pixel coordinates.
         *        so we can have bin contains feature, feature contains bin, bin and feature overlap left or right. Yuk
         */
        /* NOTE: lookup exact feature may fail if rebinned */

        if(!((gs->y1 > feature->x2) || (gs->y2 < feature->x1)))
#endif
          {
            return sl;
          }

      sl = sl->next;
    }
  return NULL;
}


/* Check that the given feature is a valid ZMapWindowCanvasFeature. I think that
 * other structs are cast to this type when they shouldn't really be because
 * zmapWindowCanvasBaseStruct isn't implemented. Therefore this struct can
 * contain corrupt values if its type is not correct: this function returns
 * true if we can trust all of its values (otherwise we can only trust the
 * first three, that would have been in zmapWindowCanvasBaseStruct). */
gboolean zmapWindowCanvasFeatureValid(ZMapWindowCanvasFeature feature)
{
  gboolean result = FALSE ;

  if (feature && feature->type > FEATURE_INVALID && feature->type < FEATURE_GRAPHICS)
    result = TRUE ;

  /*! \todo Add an assert if this fails because it would be useful to
   * debug when this happens - but only once we are able to disable asserts
   * in release code! */

  return result ;
}


static ZMapWindowCanvasFeature zmap_window_canvas_featureset_find_feature(ZMapWindowFeaturesetItem fi,
                                                                          ZMapFeature feature)
{
  ZMapWindowCanvasFeature gs = NULL;
  ZMapSkipList sl ;

  zMapReturnValIfFail(fi, NULL) ;

  if(fi->last_added && zmapWindowCanvasFeatureValid(fi->last_added) && fi->last_added->feature == feature)
    {
      gs = fi->last_added;
    }
  else
    {
      sl = zmap_window_canvas_featureset_find_feature_index(fi, feature);

      if (sl)
        {
          gs = (ZMapWindowCanvasFeature) sl->data;

          if (!zmapWindowCanvasFeatureValid(gs))
            gs = NULL ;
        }
    }

  return gs;
}


/* This function currently finds the gs for a feature and then if it's a transcript feature
 * looks for the gs that represents an exon.
 *
 * Note the subfeature passed in may be just the coding or non-coding part of an exon and not the
 * whole exon. */
static ZMapWindowCanvasFeature findFeatureSubPart(ZMapWindowFeaturesetItem fi,
                                                  ZMapFeature feature, ZMapFeatureSubPart sub_feature)
{
  ZMapWindowCanvasFeature gs = NULL ;

  gs = zmap_window_canvas_featureset_find_feature(fi, feature) ;

  /* Only find subparts for transcripts, we don't have any other features where this is useful
   * currently. */
  if (gs && ZMAPFEATURE_IS_TRANSCRIPT(feature) && sub_feature)
    {
      ZMapTranscript transcript = &feature->feature.transcript ;
      gboolean cds = transcript->flags.cds ;

      while(gs->left)
        gs = gs->left ;

      while (gs)
        {
          int start, end ;

          start = sub_feature->start ;
          end = sub_feature->end ;

          if (cds
              && (start >= gs->y1 && end <= gs->y2)
              && ((transcript->cds_start >= gs->y1 && transcript->cds_start <= gs->y2)
                  || (transcript->cds_end >= gs->y1 && transcript->cds_end <= gs->y2)))
            {
              start = gs->y1 ;
              end = gs->y2 ;
            }

          if (gs->y1 == start && gs->y2 == end)
            break ;

          gs = gs->right ;
        }
    }

  return gs ;
}







/* NOTE idea: allow NULL feature for whole foo item */
void zmap_window_canvas_featureset_expose_feature(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs)
{
  double i2w_dx,i2w_dy;
  int cx1,cx2,cy1,cy2;
  FooCanvasItem *foo = (FooCanvasItem *) fi;
  double x1;

  zMapReturnIfFail(fi) ;

  x1 = fi->x_off;
  if(fi->bumped)
    x1 += gs->bump_offset;

  /* trigger a repaint */
  /* get canvas coords */
  i2w_dx = i2w_dy = 0.0;
  foo_canvas_item_i2w (foo, &i2w_dx, &i2w_dy);

  /* get item canvas coords, following example from FOO_CANVAS_RE (used by graph items) */
  foo_canvas_w2c (foo->canvas, x1 + i2w_dx, gs->y1 - fi->start + i2w_dy, &cx1, &cy1);
  foo_canvas_w2c (foo->canvas, x1 + gs->width + i2w_dx, (gs->y2 - fi->start + i2w_dy + 1), &cx2, &cy2);

  /* need to expose + 1, plus for glyphs add on a bit: bodged to 8 pixels
   * really ought to work out max glyph size or rather have true feature extent
   * NOTE this is only currently used via OTF remove exisitng features
   */
  foo_canvas_request_redraw (foo->canvas, cx1, cy1-8, cx2 + 1, cy2 + 8);
}


void zMapWindowCanvasFeaturesetExpose(ZMapWindowFeaturesetItem fi)
{
  zMapReturnIfFail(fi) ;
  zMapWindowCanvasFeaturesetRedraw(fi, fi->zoom);
}


void zMapWindowCanvasFeaturesetRedraw(ZMapWindowFeaturesetItem fi, double zoom)
{
  double i2w_dx,i2w_dy;
  int cx1,cx2,cy1,cy2;
  FooCanvasItem *foo = (FooCanvasItem *) fi;
  double x1;
  double width = 0.0 ;

  zMapReturnIfFail(fi);

  foo = (FooCanvasItem *) fi ;
  width = fi->width ;

  /*fi->recalculate_zoom;*/ /* can set to TRUE to trigger a recalc of zoom data */
  fi->zoom = zoom;

#if 1

  foo_canvas_item_request_update (foo);
  //  foo_canvas_item_request_redraw (foo); /* won't run if the item is not mapped yet */

#endif

  // this code fails if we just added the item as it's not been updated yet
  x1 = fi->x_off;
  /* x_off is for staggered columns - we can't just add it to our foo position as it's columns that get moved about */
  /* well maybe that would be possible but the rest of the code works this way */

  if(fi->bumped)
    width = fi->bump_width;

  /* trigger a repaint */
  /* get canvas coords */
  i2w_dx = i2w_dy = 0.0;
  foo_canvas_item_i2w (foo, &i2w_dx, &i2w_dy);

  /* get item canvas coords, following example from FOO_CANVAS_RE (used by graph items) */
  //  foo_canvas_w2c (foo->canvas, x1 + i2w_dx, i2w_dy, &cx1, &cy1);
  //  foo_canvas_w2c (foo->canvas, x1 + width + i2w_dx, fi->end - fi->start + i2w_dy, &cx2, &cy2);
  /* NOTE: these coords should be relative to the block but that requires fixing update and .... draw, point??? */
  foo_canvas_w2c (foo->canvas, x1 + i2w_dx, fi->start, &cx1, &cy1);
  foo_canvas_w2c (foo->canvas, x1 + width + i2w_dx, fi->end, &cx2, &cy2);

  /* need to expose + 1, plus for glyphs add on a bit: bodged to 8 pixels
   * really ought to work out max glyph size or rather have true feature extent
   * NOTE this is only currently used via OTF remove exisitng features
   */
  foo_canvas_request_redraw (foo->canvas, cx1, cy1, cx2 + 1, cy2 + 1);        /* hits column next door? (NO, is needed) */
}


/* NOTE this function interfaces to user show/hide via zmapWindow/unhideItemsCB()
 * and zmapWindowFocus/hideFocusItemsCB()
 * and could do other things if appropriate if we include some other flags
 * current best guess is that this is not best practice: eg summarise is
 * an internal function and does not need an external interface
 */
void zmapWindowFeaturesetItemShowHide(FooCanvasItem *foo,
                                      ZMapFeature feature, gboolean show, ZMapWindowCanvasFeaturesetHideType how)
{
  ZMapWindowFeaturesetItem fi = NULL ;
  ZMapWindowCanvasFeature gs;

  zMapReturnIfFail(foo) ;
  fi = (ZMapWindowFeaturesetItem) foo ;

  //printf("show hide %s %d %d\n",g_quark_to_string(feature->original_id),show,how);

  gs = zmap_window_canvas_featureset_find_feature(fi,feature);
  if(!gs)
    return;

  if(fi->highlight_sideways)        /* ie transcripts as composite features */
    {
      while(gs->left)
        gs = gs->left;
    }

  while(gs)
    {
      if(show)
        {
          /* due to calling code these flgs are not operated correctly, so always show */
          gs->flags &= ~FEATURE_HIDDEN & ~FEATURE_HIDE_REASON;
        }
      else
	{
	  switch(how)
	    {
	    case ZMWCF_HIDE_USER:
	      gs->flags |= FEATURE_HIDDEN | FEATURE_USER_HIDE;
	      break;

	    case ZMWCF_HIDE_EXPAND:
	      /* NOTE as expanded features get deleted if unbumped we can be
               * fairly slack not testing for other flags */
	      gs->flags |= FEATURE_HIDDEN | FEATURE_HIDE_EXPAND;
	      break;
	    default:
	      break;
	    }
	}
      //printf("gs->flags: %lx\n", gs->flags);
      zmap_window_canvas_featureset_expose_feature(fi, gs);

      if(!fi->highlight_sideways)                /* only doing the selected one */
        return;

      gs = gs->right;
    }
}

/* THIS WAS MALCOLM'S COMMENTARY.....FOR THE ORIGINAL zmapWindowFeaturesetItemShowHide() */
/* NOTE this function interfaces to user show/hide via zmapWindow/unhideItemsCB()
 * and zmapWindowFocus/hideFocusItemsCB()
 * and could do other things if appropriate if we include some other flags
 * current best guess is that this is not best practice: eg summarise is
 * an internal function and does not need an external interface
 */
/* HERE'S MINE.... */
/* Function to hide a feature..... */
void zmapWindowFeaturesetItemCanvasFeatureShowHide(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature feature_item,
                                                   gboolean show, ZMapWindowCanvasFeaturesetHideType how)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapWindowCanvasFeature gs ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if(fi->highlight_sideways)	/* ie transcripts as composite features */
    {
      while(feature_item->left)
	feature_item = feature_item->left ;
    }

  while(feature_item)
    {
      if(show)
	{
	  /* due to calling code these flgs are not operated correctly, so always show */
	  feature_item->flags &= ~FEATURE_HIDDEN & ~FEATURE_HIDE_REASON ;
	}
      else
	{
	  switch(how)
	    {
	    case ZMWCF_HIDE_USER:
	      feature_item->flags |= FEATURE_HIDDEN | FEATURE_USER_HIDE ;
	      break;

	    case ZMWCF_HIDE_EXPAND:
	      /* NOTE as expanded features get deleted if unbumped we can be
               * fairly slack not testing for other flags */
	      feature_item->flags |= FEATURE_HIDDEN | FEATURE_HIDE_EXPAND ;
	      break ;

	    default:
	      break;
	    }
	}

      //printf("feature_item->flags: %lx\n", feature_item->flags);
      zmap_window_canvas_featureset_expose_feature(fi, feature_item) ;

      if(!fi->highlight_sideways)                           /* only doing the selected one */
	break ;

      feature_item = feature_item->right ;
    }

  return ;
}







/* return the foo canvas item under the lassoo, and a list of more features if included
 * we restrict the features to one column as previously multi-select was restricted to one column
 * the column is defined by the centre of the lassoo and we include features inside not overlapping
 * NOTE we implement this for CanvasFeatureset only, not old style foo
 * NOTE due to happenstance (sic) transcripts are selected if they overlap
 *
 * coordinates are canvas not world, we have to compare with foo bounds for each canvasfeatureset
 */
GList *zMapWindowFeaturesetFindItemAndFeatures(FooCanvasItem **item, double y1, double y2, double x1, double x2)
{
  GList *feature_list = NULL ;
  ZMapWindowFeaturesetItem fset ;
  ZMapSkipList sl ;
  FooCanvasItem *foo = NULL ;
  double mid_x = (x1 + x2) / 2 ;
  GList *l, *lx ;

  zMapReturnValIfFail(item && *item, NULL) ;

  zMap_g_hash_table_get_data(&lx, featureset_class_G->featureset_items) ;

  for (l = lx ; l ; l = l->next)
    {
      foo = (FooCanvasItem *)(l->data) ;

      if (foo->canvas != (*item)->canvas)        /* on another window ? */
        continue;

      if (!(foo->object.flags & FOO_CANVAS_ITEM_VISIBLE))
        continue;

      fset = (ZMapWindowFeaturesetItem)foo ;

      /* feature set must surround the given coords and must be features and not
       * some graphics. */
      if ((foo->x1 < mid_x && foo->x2 > mid_x)
          && (fset->start < y1 && fset->end > y2)
          && (fset->type == FEATURE_BASIC || fset->type == FEATURE_ALIGN || fset->type == FEATURE_TRANSCRIPT))
        {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          /* Keeping this in for debugging.....original bug was that we picked up a feature set
           * that was graphics or the wrong type or..... */

          char *fset_name ;

          fset_name = g_quark_to_string(fset->id) ;

          printf("%s\n", fset_name) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          break;
        }
    }

  if (lx)
    g_list_free(lx) ;

  if (!l || !foo)
    return(NULL) ;


  {
    char *fset_name ;

    fset_name = (char *)g_quark_to_string(fset->id) ;

    printf("%s\n", fset_name) ;

  }




  sl = zmap_window_canvas_featureset_find_feature_coords(NULL, fset, y1, y2);

  for( ; sl ; sl = sl->next)
    {
      ZMapWindowCanvasFeature gs;
      gs = sl->data;

      if(gs->flags & FEATURE_HIDDEN)        /* we are setting focus on visible features ! */
        continue;

      if(gs->y1 > y2)
        break;

      /* reject overlaps as we can add to the selection but not subtract from it */
      if(gs->y1 < y1)
        continue;
      if(gs->y2 > y2)
        continue;

      if(fset->bumped)
        {
          double x = fset->dx + gs->bump_offset;
          if(x < x1)
            continue;
          x += gs->width;

          if(x > x2)
            continue;
        }
      /* else just match */

      if(!feature_list)
        {
          *item = (FooCanvasItem *) fset;

          zMapWindowCanvasItemSetFeaturePointer((ZMapWindowCanvasItem)*item, gs->feature) ;

          /* rather boringly these could get revived later and overwrite the canvas item feature ?? */
          /* NOTE probably not, the bug was a missing * in the line above */
          fset->point_feature = gs->feature;
          fset->point_canvas_feature = gs;
        }
      //     else        // why? item has the first one and feature list is the others if present
      // mh17: always include the first in the list to filter duplicates eg transcript exons
      {
        feature_list = zMap_g_list_append_unique(feature_list, gs->feature);
      }
    }


  return feature_list ;
}


/* THIS NEEDS TO BE COMBINED WITH THE ROUTINE ABOVE...BUT FOR NOW LET'S JUST GET ON.... */

/* Returns a list of ZMapWindowFeaturesetItem that overlap the range y1, y2. */
GList *zMapWindowFeaturesetFindFeatures(ZMapWindowFeaturesetItem featureset_item, double y1, double y2)
{
  GList *feature_list = NULL ;
  ZMapSkipList sl ;

  zMapReturnValIfFail((featureset_item || (featureset_item->start < y1 || featureset_item->end > y2)), NULL) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMap_g_hash_table_get_data(&lx, featureset_class_G->featureset_items) ;

  for (l = lx ; l ; l = l->next)
    {
      foo = (FooCanvasItem *)(l->data) ;

      if (foo->canvas != (*item)->canvas)        /* on another window ? */
        continue;

      if (!(foo->object.flags & FOO_CANVAS_ITEM_VISIBLE))
        continue;

      fset = (ZMapWindowFeaturesetItem)foo ;

      /* feature set must surround the given coords and must be features and not
       * some graphics. */
      if ((foo->x1 < mid_x && foo->x2 > mid_x)
          && (fset->start < y1 && fset->end > y2)
          && (fset->type == FEATURE_BASIC || fset->type == FEATURE_ALIGN || fset->type == FEATURE_TRANSCRIPT))
        {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          /* Keeping this in for debugging.....original bug was that we picked up a feature set
           * that was graphics or the wrong type or..... */

          char *fset_name ;

          fset_name = g_quark_to_string(fset->id) ;

          printf("%s\n", fset_name) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          break;
        }
    }

  if (lx)
    g_list_free(lx) ;

  if (!l || !foo)
    return(NULL) ;


  {
    char *fset_name ;

    fset_name = (char *)g_quark_to_string(fset->id) ;

    printf("%s\n", fset_name) ;

  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





  if ((featureset_item->type == FEATURE_BASIC || featureset_item->type == FEATURE_ALIGN
       || featureset_item->type == FEATURE_TRANSCRIPT))
    {
      sl = zmap_window_canvas_featureset_find_feature_coords(NULL, featureset_item, y1, y2) ;

      for ( ; sl ; sl = sl->next)
        {
          ZMapWindowCanvasFeature gs;

          gs = sl->data;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          char *name ;

          name = g_quark_to_string(gs->feature->original_id) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          if(gs->flags & FEATURE_HIDDEN)        /* we are setting focus on visible features ! */
            continue;

          if (gs->y1 > y2)
            break;

          /* ADDING A TEST FOR NON-OVERLAPPING AT THE START, REMOVING OVERLAPS AS WE NEED THEM,
           * MAKE THE LATTER INTO A FUNCTION PARAMETER ?? */
          if (gs->y2 < y1)
            continue ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          /* reject overlaps as we can add to the selection but not subtract from it */
          if(gs->y1 < y1)
            continue;
          if(gs->y2 > y2)
            continue;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


          if (!feature_list)
            {
              zMapWindowCanvasItemSetFeaturePointer((ZMapWindowCanvasItem)featureset_item, gs->feature) ;

              /* rather boringly these could get revived later and overwrite the canvas item feature ?? */
              /* NOTE probably not, the bug was a missing * in the line above */
              featureset_item->point_feature = gs->feature;
              featureset_item->point_canvas_feature = gs;
            }


          /* BUGGER...HERE'S MY PROBLEM...MALCOLM WAS ADDING THE _FEATURE_ TO THE LIST WHICH IS UNIQUE..BUT EACH
             FEATURE WHEN IT'S A TRANSCRIPT HAS MULTIPLE gs POINTERS AND SO IS ADDED MULTIPLE TIMES....BUGGER....

             NOTE, I NEED ALL THE gs POINTERS TO MAKE SURE I HIDE THEM ALL BUT I NEED NOT TO DO MATCHING ON THEM
             ALL....BOTHER........*/


          //     else	// why? item has the first one and feature list is the others if present
          // mh17: always include the first in the list to filter duplicates eg transcript exons
          {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
            feature_list = zMap_g_list_append_unique(feature_list, gs->feature);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
            /* let's append some debugging...... */
            if (zMapFeatureIsValid((ZMapFeatureAny)(gs->feature)))
              zMapDebugPrintf("Feature Item:  %p,/tfeature:  %s", gs, zMapFeatureName((ZMapFeatureAny)(gs->feature))) ;
            else
              zMapDebugPrintf("Feature Item:  %p,/thas invalid feature !!", gs) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


            /* ok...let's return the canvasfeature instead..... */
            feature_list = zMap_g_list_append_unique(feature_list, gs) ;

          }
        }
    }


  return feature_list ;
}





/* My new super-duper call..... */
/* Returns a list of 'lists of ZMapWindowFeaturesetItem' that overlap y1, y2. Each sublist
 * contains a list of ZMapWindowFeaturesetItem's that all originate from the same
 * genomic feature, e.g. an EST.
 *
 * If canonical_only is TRUE then any non-canonical features in the span will be excluded.
 *
 *
 * Notes
 *  - only supported for FEATURE_BASIC, FEATURE_ALIGN, FEATURE_TRANSCRIPT currently.
 *
 *  */
GList *zMapWindowFeaturesetFindGroupedFeatures(ZMapWindowFeaturesetItem featureset_item,
                                               double y1, double y2, gboolean canonical_only)
{
  GList *feature_list = NULL ;
  ZMapSkipList sl ;

  zMapReturnValIfFail((featureset_item || (featureset_item->start < y1 || featureset_item->end > y2)), NULL) ;


  if ((featureset_item->type == FEATURE_BASIC || featureset_item->type == FEATURE_ALIGN
       || featureset_item->type == FEATURE_TRANSCRIPT))
    {
      GHashTable *feature_items_found ;

      feature_items_found = g_hash_table_new(NULL, NULL) ;


      sl = zmap_window_canvas_featureset_find_feature_coords(NULL, featureset_item, y1, y2) ;

      for ( ; sl ; sl = sl->next)
        {
          ZMapWindowCanvasFeature gs ;

          gs = sl->data ;

          if (gs->flags & FEATURE_HIDDEN)	/* we are setting focus on visible features ! */
            continue;

          if (gs->y1 > y2)
            break;

          if (gs->y2 < y1)
            continue ;


          /* I DO NOT UNDERSTAND MALCOLM'S CODE HERE..... */
          if (!feature_list)
            {
              zMapWindowCanvasItemSetFeaturePointer((ZMapWindowCanvasItem)featureset_item, gs->feature) ;


              /* rather boringly these could get revived later and overwrite the canvas item feature ?? */
              /* NOTE probably not, the bug was a missing * in the line above */
              featureset_item->point_feature = gs->feature;
              featureset_item->point_canvas_feature = gs;
            }




          //     else	// why? item has the first one and feature list is the others if present
          // mh17: always include the first in the list to filter duplicates eg transcript exons
          {
            GList *grouped_features = NULL ;

            do
              {
                /* oh dear repeating tests from above for first time round loop....rationalise... */
                if (gs->flags & FEATURE_HIDDEN)	/* we are setting focus on visible features ! */
                  continue;

                if (gs->y1 > y2)
                  break;

                if (gs->y2 < y1)
                  continue ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                /* Not correct at the moment... */

                if (canonical_only)
                  {
                    if (gs->y1 < prev_y2)
                      continue ;

                    prev_y1 = gs->y1 ;
                    prev_y2 = gs->y2 ;
                  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


                /* If feature is already there stop, otherwise insert. */
                if ((g_hash_table_lookup(feature_items_found, gs)))
                  {
                    break ;
                  }
                else
                  {
                    grouped_features = g_list_append(grouped_features, gs) ;

                    g_hash_table_insert(feature_items_found, gs, gs) ;
                  }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                gs = gs->right ;

              } while (gs) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
          } while ((gs = gs->right)) ;




            if (grouped_features)
              feature_list = g_list_append(feature_list, grouped_features) ;

          }
        }

      g_hash_table_destroy(feature_items_found) ;
    }


  return feature_list ;
}


void zMapWindowFeaturesetFreeGroupedFeatures(GList *grouped_features)
{
  g_list_foreach(grouped_features, groupListFreeCB, NULL) ;

  g_list_free(grouped_features) ;

  return ;
}







/* I'm not sure how closely ->display and ->display_index are linked, if the first exists does
 * the second ? or vice versa ?......would affect how this function is coded. */
gboolean zmapWindowCanvasFeaturesetFreeDisplayLists(ZMapWindowFeaturesetItem featureset_item_inout)
{
  gboolean result = FALSE ;

  if (featureset_item_inout->display_index)
    {
      zMapSkipListDestroy(featureset_item_inout->display_index, NULL) ;
      featureset_item_inout->display_index = NULL ;

      if (featureset_item_inout->display)
        {
          GList  *features ;

          for (features = featureset_item_inout->display ; features ;
               features = g_list_delete_link(features, features))
            {
              ZMapWindowCanvasFeature feat = (ZMapWindowCanvasFeature)features->data ;

              zmapWindowCanvasFeatureFree(feat) ;
            }
          featureset_item_inout->display = NULL ;
        }

      result = TRUE ;
    }

  return result ;
}







/* for composite items (eg ZMapWindowGraphDensity)
 * set the pointed at feature to be the one nearest to cursor
 * We need this to stabilise the feature while processing a mouse click (item/feature select)
 * point used to set this but the cursor is still moving while we trundle thro the code
 * causing the feature to change with unpredictable results
 * write up in density.html when i get round to correcting it.
 */
gboolean zMapWindowCanvasItemSetFeature(ZMapWindowCanvasItem item, double x, double y)
{
  ZMapWindowCanvasItemClass class = ZMAP_CANVAS_ITEM_GET_CLASS(item);
  gboolean ret = FALSE;

  zMapReturnValIfFail(class, FALSE) ;

  if(class->set_feature)
    ret = class->set_feature((FooCanvasItem *) item,x,y);

  return ret;
}



/* cut and paste from former graph density code */
gboolean zMapWindowFeaturesetItemSetStyle(ZMapWindowFeaturesetItem featureset_item, ZMapFeatureTypeStyle style)
{
  GdkColor *draw = NULL, *fill = NULL, *outline = NULL;
  FooCanvasItem *foo = (FooCanvasItem *) featureset_item;
  gboolean re_index = FALSE;

  zMapReturnValIfFail(featureset_item, FALSE) ;

  //  featureset_item->recalculate_zoom = TRUE;                // trigger recalc


  if (zMapStyleGetMode(featureset_item->style) == ZMAPSTYLE_MODE_GRAPH
      && featureset_item->re_bin && zMapStyleDensityMinBin(featureset_item->style) != zMapStyleDensityMinBin(style))
    re_index = TRUE;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (featureset_item->display_index && re_index)
    {
      zMapSkipListDestroy(featureset_item->display_index, NULL);
      featureset_item->display_index = NULL;

      if (featureset_item->display)        /* was re-binned */
        {
          for(features = featureset_item->display; features; features = g_list_delete_link(features,features))
            {
              ZMapWindowCanvasFeature feat = (ZMapWindowCanvasFeature) features->data;

              zmapWindowCanvasFeatureFree(feat);
            }
          featureset_item->display = NULL;
        }
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  if (re_index && featureset_item->display_index)
    zmapWindowCanvasFeaturesetFreeDisplayLists(featureset_item) ;


  featureset_item->style = style;                /* includes col width */
  featureset_item->width = style->width;
  featureset_item->x_off = zMapStyleDensityStagger(style) * featureset_item->set_index;
  featureset_item->x_off += zMapStyleOffset(style);

  /* need to set colours */
  zmapWindowCanvasItemGetColours(style, featureset_item->strand, featureset_item->frame,
                                 ZMAPSTYLE_COLOURTYPE_NORMAL, &fill, &draw, &outline, NULL, NULL);

  if(fill)
    {
      featureset_item->fill_set = TRUE;
      featureset_item->fill_colour = gdk_color_to_rgba(fill);
      featureset_item->fill_pixel = foo_canvas_get_color_pixel(foo->canvas, featureset_item->fill_colour);
    }
  if(outline)
    {
      featureset_item->outline_set = TRUE;
      featureset_item->outline_colour = gdk_color_to_rgba(outline);
      featureset_item->outline_pixel = foo_canvas_get_color_pixel(foo->canvas, featureset_item->outline_colour);
    }

  foo_canvas_item_request_update ((FooCanvasItem *)featureset_item);

  return TRUE;
}

/* Standard way to allocate featureset colours, parameters are:
 *
 *          FALSE => we don't need the colour to be writeable
 *           TRUE => we will accept the best available match  */
gboolean zmapWindowFeaturesetAllocColour(ZMapWindowFeaturesetItemClass featureset_class,
                                         GdkColor *colour)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail(featureset_class, FALSE) ;

  result = gdk_colormap_alloc_color(featureset_class->colour_map, colour, FALSE, TRUE) ;

  return result ;
}



/* probably need pixel and gdkcolour versions of these...... */
/* Return pointers to the GdkColor structs for the default colours, you should not write
 * to or attempt to free these structs. */
gboolean zmapWindowFeaturesetGetDefaultColours(ZMapWindowFeaturesetItem feature_set_item,
                                               GdkColor **fill, GdkColor **draw, GdkColor **border)
{
  gboolean result = FALSE ;
  ZMapWindowFeaturesetItemClass featureset_class ;

  zMapReturnValIfFail(feature_set_item, FALSE) ;

  featureset_class = ZMAP_WINDOW_FEATURESET_ITEM_GET_CLASS(feature_set_item) ;

  /* It's possible our colour allocation failed in which case we return FALSE. */
  if (featureset_class->colour_alloc)
    {
      if (fill)
        *fill = &(featureset_class->fill) ;
      if (draw)
        *draw = &(featureset_class->draw) ;
      if (border)
        *fill = &(featureset_class->border) ;

      result = FALSE ;
    }

  return result ;
}


/* Return the pixel values for the default colours, these can be used directly to in
 * setting the foreground/background colours in a graphics context. */
gboolean zmapWindowFeaturesetGetDefaultPixels(ZMapWindowFeaturesetItem feature_set_item,
                                              guint32 *fill, guint32 *draw, guint32 *border)
{
  gboolean result = FALSE ;
  ZMapWindowFeaturesetItemClass featureset_class ;

  zMapReturnValIfFail(feature_set_item, FALSE) ;

  featureset_class = ZMAP_WINDOW_FEATURESET_ITEM_GET_CLASS(feature_set_item) ;

  /* It's possible our colour allocation failed in which case we return FALSE. */
  if (featureset_class->colour_alloc)
    {
      if (fill)
        *fill = featureset_class->fill.pixel ;
      if (draw)
        *draw = featureset_class->draw.pixel ;
      if (fill)
        *fill = featureset_class->border.pixel ;

      result = FALSE ;
    }

  return result ;
}





/* NOTE this extends FooCanvasItem via ZMapWindowCanvasItem */
static void zmap_window_featureset_item_item_class_init(ZMapWindowFeaturesetItemClass featureset_class)
{
  GtkObjectClass *gtkobject_class ;
  FooCanvasItemClass *item_class;
  ZMapWindowCanvasItemClass canvas_class;

  featureset_class_G = featureset_class;
  featureset_class_G->featureset_items = g_hash_table_new(NULL,NULL);

  parent_class_G = g_type_class_peek_parent (featureset_class);

  gtkobject_class = (GtkObjectClass *) featureset_class;
  item_class = (FooCanvasItemClass *) featureset_class;
  item_class_G = gtk_type_class(FOO_TYPE_CANVAS_ITEM);
  canvas_class = (ZMapWindowCanvasItemClass) featureset_class;

  gtkobject_class->destroy = zmap_window_featureset_item_item_destroy;

  item_class->update = zmap_window_featureset_item_item_update;
  item_class->bounds = zmap_window_featureset_item_item_bounds;
  item_class->point  = zmap_window_featureset_item_foo_point;
  item_class->draw   = zmap_window_featureset_item_item_draw;

  canvas_class->set_colour = zmap_window_featureset_item_set_colour;
  canvas_class->set_feature = zmap_window_featureset_item_set_feature;
  canvas_class->set_style = zmap_window_featureset_item_set_style;
  canvas_class->showhide = zmap_window_featureset_item_show_hide;

  //  zmapWindowItemStatsInit(&(canvas_class->stats), ZMAP_TYPE_WINDOW_FEATURESET_ITEM) ;

  /* Set up default colours stuff.... */
  featureset_class->colour_map = gdk_colormap_get_system() ;    /* Not screen sensitive.... */

  featureset_class->colour_alloc = TRUE ;
  if (!gdk_color_parse(CANVAS_DEFAULT_COLOUR_FILL, &(featureset_class->fill))
      || !zmapWindowFeaturesetAllocColour(featureset_class, &(featureset_class->fill)))
    {
      zMapLogCritical("Allocation of colour %d as default %d failed.",
                      CANVAS_DEFAULT_COLOUR_FILL, "fill") ;
      featureset_class->colour_alloc = FALSE ;
    }
  if (!gdk_color_parse(CANVAS_DEFAULT_COLOUR_DRAW, &(featureset_class->draw))
      || !zmapWindowFeaturesetAllocColour(featureset_class, &(featureset_class->draw)))
    {
      zMapLogCritical("Allocation of colour %d as default %d failed.",
                      CANVAS_DEFAULT_COLOUR_DRAW, "draw") ;
      featureset_class->colour_alloc = FALSE ;
    }
  if (!gdk_color_parse(CANVAS_DEFAULT_COLOUR_BORDER, &(featureset_class->border))
      || !zmapWindowFeaturesetAllocColour(featureset_class, &(featureset_class->border)))
    {
      zMapLogCritical("Allocation of colour %d as default %d failed.",
                      CANVAS_DEFAULT_COLOUR_BORDER, "border") ;
      featureset_class->colour_alloc = FALSE ;
    }

  featureset_init_funcs() ;

  return ;
}


/* record the current feature found by cursor movement which continues moving as we run more code using the feature */
static gboolean zmap_window_featureset_item_set_feature(FooCanvasItem *item, double x, double y)
{
  gboolean result = FALSE ;

  if (g_type_is_a(G_OBJECT_TYPE(item), ZMAP_TYPE_WINDOW_FEATURESET_ITEM))
    {
      ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) item ;
#if MOUSE_DEBUG
      zMapLogWarning("set feature %p",fi->point_feature);
#endif

      if (fi->point_feature)
        {
          fi->__parent__.feature = fi->point_feature ;
          result = TRUE ;
        }
    }

  return result ;
}



/* ERRRR....these functions always return FALSE !!!! CHECK THEIR USAGE..... */

/* redisplay the column using an alternate style */
static gboolean zmap_window_featureset_item_set_style(FooCanvasItem *item, ZMapFeatureTypeStyle style)
{
  if (g_type_is_a(G_OBJECT_TYPE(item), ZMAP_TYPE_WINDOW_FEATURESET_ITEM))
    {
      ZMapWindowFeaturesetItem di = (ZMapWindowFeaturesetItem) item;

      zMapWindowFeaturesetItemSetStyle(di,style);
    }

  return FALSE;
}



static gboolean zmap_window_featureset_item_show_hide(FooCanvasItem *item, gboolean show)
{
  if (g_type_is_a(G_OBJECT_TYPE(item), ZMAP_TYPE_WINDOW_FEATURESET_ITEM))
    {
      ZMapWindowFeaturesetItem di = (ZMapWindowFeaturesetItem) item;
      ZMapWindowCanvasItem canvas_item = (ZMapWindowCanvasItem) &(di->__parent__);
      /* find the feature struct and set a flag */

      zmapWindowFeaturesetItemShowHide(item,canvas_item->feature,show, ZMWCF_HIDE_USER);
    }

  return FALSE;
}


static void zmap_window_featureset_item_set_colour(ZMapWindowCanvasItem   item,
						   FooCanvasItem         *interval,
						   ZMapFeature			feature,
						   ZMapFeatureSubPart sub_feature,
						   ZMapStyleColourType    colour_type,
						   int colour_flags,
						   GdkColor              *fill,
						   GdkColor              *border)
{
  if (g_type_is_a(G_OBJECT_TYPE(interval), ZMAP_TYPE_WINDOW_FEATURESET_ITEM))
    {
      zmapWindowFeaturesetItemSetColour(interval,feature,sub_feature,colour_type,colour_flags,fill,border);
    }

  return ;
}



/* Called for each new featureset (== column ??), gosh what happens here.... */
static void zmap_window_featureset_item_item_init(ZMapWindowFeaturesetItem featureset)
{
  char *featureset_id ;


  featureset_id = (char *)g_quark_to_string(featureset->id) ;



  return ;
}




static void zmap_window_featureset_item_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{

  ZMapWindowFeaturesetItem di = (ZMapWindowFeaturesetItem) item;
  int cx1,cy1,cx2,cy2;
  double x1,x2,y1,y2;
  double width;

  if(item_class_G->update)
    item_class_G->update(item, i2w_dx, i2w_dy, flags);                /* just sets flags */

  // cribbed from FooCanvasRE; this sets the canvas coords in the foo item
  /* x_off is needed for staggered graphs, is currently 0 for all other types */
  /* x1 is relative to the groups (column), but would be better to implement the foo 'x' property */
  x1 = di->dx = i2w_dx + di->x + di->x_off;
  width = (di->bumped? di->bump_width : di->width);
  //printf("update %s width = %.1f\n",g_quark_to_string(di->id),di->width);

  if((di->layer & ZMAP_CANVAS_LAYER_STRETCH_X))
    width = 1;          /* will be set afterwards by caller */

  x2 = x1 + width;

  di->dy = i2w_dy;
  y1 = di->dy = di->start;
  y2 = (y1 + di->end - di->start) + 1 ;                     /* + 1 to cover last base. */

  if((di->layer & ZMAP_CANVAS_LAYER_STRETCH_Y))
    y2 = y1;          /* will be set afterwards by caller */

  //printf("update %s y1,y2 = %f, %f\n",g_quark_to_string(di->id), y1, y2);

  foo_canvas_w2c (item->canvas, x1, y1, &cx1, &cy1);
  foo_canvas_w2c (item->canvas, x2, y2, &cx2, &cy2);

  item->x1 = cx1;
  item->x2 = cx2;

  item->y1 = cy1;
  item->y2 = cy2;

  return ;
}



/* are we within the bounding box if not on a feature ? */
/* when we call this we already know, but we have to set actual_item */
double featureset_background_point(FooCanvasItem *item,int cx, int cy, FooCanvasItem **actual_item)
{
  double best = 1.0e36;

  zMapReturnValIfFail(item && actual_item, best ) ;

  if(item->y1 <= cy && item->y2 >= cy)
    {
      if(item->x1 <= cx && item->x2 >= cx)
        {
          best = 0.0;
          *actual_item = item;
        }
    }

  return(best);
}



/* how far are we from the cursor? */
/* can't return foo canvas item for the feature as they are not in the canvas,
 * so return the featureset foo item adjusted to point at the nearest feature */

/* by a process of guesswork x,y are world coordinates and cx,cy are canvas (i think) */
/* No: x,y are parent item local coordinates ie offset within the group
 * we have a ZMapCanvasItem group with no offset, so we need to adjust by the x,ypos of that group
 */
double  zmap_window_featureset_item_foo_point(FooCanvasItem *item,
                                              double item_x, double item_y, int cx, int cy,
                                              FooCanvasItem **actual_item)
{
  double best = 1.0e36 ;                                    /* Default value from foocanvas code. */
  ZMapWindowFeatureItemPointFunc point_func = NULL;
  ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem)item;
  ZMapWindowCanvasFeature gs;
  ZMapSkipList sl;
  double local_x, local_y ;
  double y1,y2;
  //      FooCanvasGroup *group;
  double x_off;
  static double save_best = 1.0e36, save_x = 0.0, save_y = 0.0 ;
  //int debug = fi->type >= FEATURE_GRAPHICS ;
  int n = 0;


  /*
   * need to scan internal list and apply close enough rules
   */

  /* zmapSkipListFind();                 gets exact match to start coord or item before
     if any feature overlaps choose that
     (assuming non overlapping features)
     else choose nearest of next and previous
  */
  *actual_item = NULL;                                      /* UM, SHOULDN'T DO THIS.... */

#if 0
  {
    ZMapWindowContainerFeatureSet x = (ZMapWindowContainerFeatureSet) item->parent;

    printf("CFS point %s(%p)/%s %x %ld\n",
           g_quark_to_string(zmapWindowContainerFeatureSetGetColumnId(x)),
           x, g_quark_to_string(fi->id),fi->layer, fi->n_features);
  }
#endif

  /* YES BUT WHAT ARE THEY !!!!!!!!!" */
  if ((fi->layer & ZMAP_CANVAS_LAYER_DECORATION))        /* we don-t want to click on these ! */
    return(best);


  /* optimise repeat calls: the foo canvas does 6 calls for a click event (3 down 3 up)
   * and if we are zoomed into a bumped peptide alignment column that means looking at a lot of features
   * each one goes through about 12 layers of canvas containers first but that's another issue
   * then if we move the lassoo that gets silly (button down: calls point())
   */
  //if(debug)
  //        zMapLogWarning("point: %.1f,%.1f %.1f %.1f", item_x, item_y, fi->start, fi->dy);

  if (fi->point_canvas_feature && item_x == save_x && item_y == save_y)
    {
      fi->point_feature = fi->point_canvas_feature->feature;
      *actual_item = item;
      best = save_best ;
    }
  else
    {
      save_x = item_x;
      save_y = item_y;
      fi->point_canvas_feature = NULL;
      fi->point_feature = NULL;

      best = fi->end - fi->start + 1;

      /*! \todo #warning need to check these coordinate calculations */
      local_x = item_x + fi->dx;
      local_y = item_y + fi->dy;
      /*! \todo #warning this code is in the wrong place, review when containers rationalised */

      y1 = local_y - item->canvas->close_enough;
      y2 = local_y + item->canvas->close_enough;


      /* This all seems a bit hokey...who says the glyphs are in the middle of the column ? */

      /* NOTE histgrams are hooked onto the LHS, but we can click on the row and still get the feature */

      /*! \todo #warning change this to use featurex1 and x2 coords */
      /* NOTE warning even better if we express point() function in pixel coordinates only */

      x_off = fi->dx + fi->x_off;

      /* NOTE there is a flake in world coords at low zoom */
      /* NOTE close_enough is zero */
      sl = zmap_window_canvas_featureset_find_feature_coords(zMapWindowFeatureFullCmp, fi, y1, y2) ;


      /* AGH....HATEFUL....STOP RETURNING FROM THE MIDDLE OF STUFF..... */
      //printf("point %s        %f,%f %d,%d: %p\n",g_quark_to_string(fi->id),x,y,cx,cy,sl);
      if (!sl)
        return featureset_background_point(item, cx, cy, actual_item) ;


      for (; sl ; sl = sl->next)
        {
          double this_one;
          double left;

          gs = (ZMapWindowCanvasFeature) sl->data;

          // printf("y1,2: %.1f %.1f,   gs: %s %lx %f %f\n",y1,y2, g_quark_to_string(gs->feature->unique_id), gs->flags, gs->y1,gs->y2);

          n++;
          if (gs->flags & FEATURE_HIDDEN)
            continue;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          /* Perhaps this works for normal features BUT it's completely broken for glyphs....if
             it's done at all it should be in the specific feature point routines. */

          // mh17: if best is 1e36 this is silly:
          //          if (gs->y1 > y2  + best)
          if (gs->y1 > y2)                /* y2 has close_enough factored in */
            break;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


          /* check for feature type specific point code, otherwise default to standard point func. */
          point_func = NULL;

            if (gs->type > 0 && gs->type < FEATURE_N_TYPE)
            point_func = _featureset_point_G[gs->type] ;

          if (!point_func)
            point_func = gs->type < FEATURE_GRAPHICS ? featurePoint : graphicsPoint;

          left = x_off;

          if(zMapStyleGetMode(fi->style) != ZMAPSTYLE_MODE_GRAPH)
            left += fi->width / 2 - gs->width / 2;

          if ((this_one = point_func(fi, gs, item_x, item_y, cx, cy, local_x, local_y, left)) < best)
            {
              fi->point_feature = gs->feature;
              *actual_item = item;
              //printf("overlaps x\n");

              /*
               * NOTE: this could concievably cause a memory fault if we freed point_canvas_feature
               * but that seems unlikely if we don-t nove the cursor
               */
              fi->point_canvas_feature = gs;
              best = this_one;


              if(!best)        /* can't get better */
                {
                  /* and if we don't quit we will look at every other feature,
                   * pointlessly, although that makes no difference to the user
                   */
                  break;
                }
            }
        }
    }


  /* experiment: this prevents the delay:         *actual_item = NULL;  best = 1e36; */

#if MOUSE_DEBUG

  { char *x = "";
    extern int n_item_pick; // foo canvas debug

    if(fi->point_feature) x = (char *) g_quark_to_string(fi->point_feature->unique_id);

    zMapLogWarning("point tried %d/ %d features (%.1f,%.1f) @ %s (picked = %d)",
                   n,fi->n_features, item_x, item_y, x, n_item_pick);
  }
#endif

  /* return found if cursor is in the coloumn, then we don't need invisble background rectangles */
  if (!fi->point_feature)
    {
      best = featureset_background_point(item, cx, cy, actual_item) ;
    }

  return best ;
}



/* Convert given sequence coords into world coords, the sequence coords must lie
 * within the parent block coords, i.e. the sequence start/end, of the featureset. */
gboolean zMapCanvasFeaturesetSeq2World(ZMapWindowFeaturesetItem featureset,
                                       int seq_start, int seq_end, double *world_start_out, double *world_end_out)
{
  gboolean result = FALSE ;
  double world_start, world_end ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* THIS IS FROM zMapCanvasFeaturesetDrawBoxMacro(), the conversion should be a macro, it's done
   * all over the place in the code..... */

  /* Note the calc to get to world....from sequence.... */

  /* get item canvas coords, following example from FOO_CANVAS_RE (used by graph items) */
  /* NOTE CanvasFeature coords are the extent including decorations so we get coords from the feature */
  foo_canvas_w2c (item->canvas, x1, y1 - featureset->start + featureset->dy, &cx1, &cy1);
  foo_canvas_w2c (item->canvas, x2, y2 - featureset->start + featureset->dy + 1, &cx2, &cy2);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (featureset && world_start_out && world_end_out
      && (seq_start < seq_end)
      && (seq_start >= featureset->start && seq_start <= featureset->end)
      && (seq_end >= featureset->start && seq_end <= featureset->end))
    {
      world_start = seq_start - featureset->start + featureset->dy ;
      world_end = seq_end - featureset->start + featureset->dy ;

      *world_start_out = world_start ;
      *world_end_out = world_end ;

      result = TRUE ;
    }

  return result ;
}




/* cribbed from zmapWindowGetPosFromScore(() which is called 3x from Item Factory and will eventaully get removed */
double zMapWindowCanvasFeatureGetWidthFromScore(ZMapFeatureTypeStyle style, double width, double score)
{
  double dx = 0.0 ;
  double numerator, denominator ;
  double max_score, min_score ;

  zMapReturnValIfFail(style, width) ;

  min_score = zMapStyleGetMinScore(style) ;
  max_score = zMapStyleGetMaxScore(style) ;

  /* We only do stuff if there are both min and max scores to work with.... */
  if (max_score && min_score)
    {
      numerator = score - min_score ;
      denominator = max_score - min_score ;

      if (denominator == 0)                                    /* catch div by zero */
        {
          if (numerator <= 0)
            dx = 0.25 ;
          else if (numerator > 0)
            dx = 1 ;
        }
      else
        {
          dx = 0.25 + (0.75 * (numerator / denominator)) ;
        }

      if (dx < 0.25)
        dx = 0.25 ;
      else if (dx > 1)
        dx = 1 ;

      width *= dx;
    }

  return width;
}



/* used by graph data ... which recalulates bins */
/* normal features just have width set from feature score */
double zMapWindowCanvasFeatureGetNormalisedScore(ZMapFeatureTypeStyle style, double score)
{
  double numerator, denominator, dx = 0 ;
  double max_score, min_score ;

  min_score = zMapStyleGetMinScore(style) ;
  max_score = zMapStyleGetMaxScore(style) ;

  numerator = score - min_score ;
  denominator = max_score - min_score ;


  if(numerator < 0)                        /* coverage and histgrams do not have -ve values */
    numerator = 0;
  if(denominator < 0)                /* dumb but wise, could conceivably be mis-configured and not checked */
    denominator = 0;

  if (zMapStyleIsPropertySetId(style, STYLE_PROP_SCORE_SCALE)
      && (zMapStyleGetScoreScale(style) == ZMAPSTYLE_SCALE_LOG))
    {
      numerator++;        /* as log(1) is zero we need to bodge values of 1 to distingish from zero */
      /* and as log(0) is big -ve number bias zero to come out as zero */

      numerator = log(numerator);
      denominator = log(denominator) ;
    }

  if (denominator == 0)                         /* catch div by zero */
    {
      if (numerator < 0)
        dx = 0 ;
      else if (numerator > 0)
        dx = 1 ;
    }
  else
    {
      dx = numerator / denominator ;
      if (dx < 0)
        dx = 0 ;
      if (dx > 1)
        dx = 1 ;
    }


  return(dx) ;
}


double zMapWindowCanvasFeaturesetGetFilterValue(FooCanvasItem *foo)
{
  ZMapWindowFeaturesetItem featureset_item;

  featureset_item = (ZMapWindowFeaturesetItem) foo;
  return featureset_item->filter_value ;
}

int zMapWindowCanvasFeaturesetGetFilterCount(FooCanvasItem *foo)
{
  ZMapWindowFeaturesetItem featureset_item;

  featureset_item = (ZMapWindowFeaturesetItem) foo;
  return featureset_item->n_filtered ;
}


int zMapWindowCanvasFeaturesetFilter(gpointer gfilter, double value, gboolean highlight_filtered_columns)
{
  ZMapWindowFilter filter        = (ZMapWindowFilter) gfilter;
  ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) filter->featureset;
  ZMapSkipList sl;
  int was = fi->n_filtered;
  double score;

  fi->filter_value = value;
  fi->n_filtered = 0;

  for(sl = zMapSkipListFirst(fi->display_index); sl; sl = sl->next)
    {
      ZMapWindowCanvasFeature feature = (ZMapWindowCanvasFeature) sl->data;        /* base struct of all features */
      ZMapWindowCanvasFeature f;

      if (!zmapWindowCanvasFeatureValid(feature))
        continue;

      if(feature->left)                /* we do joined up alignments */
        continue;

      if( !feature->feature->flags.has_score && !feature->feature->population)
        continue;


      /* get score for whole series of alignments */
      for(f = feature, score = 0.0; f; f = f->right)
        {
          double feature_score = feature->feature->population;

          if(!feature_score)
            {
              feature_score = feature->feature->score;
              /* NOTE feature->score is normalised, feature->feature->score is what we filter by */

              if(zMapStyleGetScoreMode(*f->feature->style) == ZMAPSCORE_PERCENT)
                feature_score = f->feature->feature.homol.percent_id;
            }
          if(feature_score > score)
            score = feature_score;
        }

      /* set flags for whole series based on max score: filter is all below value */
      for(f = feature; f; f = f->right)
        {
          if(score < value)
            {
              f->flags |= FEATURE_HIDE_FILTER | FEATURE_HIDDEN;
              fi->n_filtered++;

              /* NOTE many of these may be FEATURE_SUMMARISED which is not operative during bump
               * so setting HIDDEN here must be done for the filtered features only
               * Hmmm... not quite sure of the mechanism, but putting this if
               * outside the brackets give a glitch on set column focus when bumped (features not painted)
               * .... summarised features (FEATURE_SUMMARISED set but not HIDDEN bue to bumping) set to hidden when bumped
               */
            }
          else
            {
              /* reset in case score went down */

              f->flags &= ~FEATURE_HIDE_FILTER;
              if(!(f->flags & FEATURE_HIDE_REASON))
                f->flags &= ~FEATURE_HIDDEN;
              else if(fi->bumped && (f->flags & FEATURE_HIDE_REASON) == FEATURE_SUMMARISED)
                f->flags &= ~FEATURE_HIDDEN;
            }
        }
    }

  if(fi->n_filtered != was)
    {
      /* trigger a re-calc if summarised to ensure the picture is pixel perfect
       * NOTE if bumped we don-t calculate so no creeping inefficiency here
       */
      fi->recalculate_zoom = TRUE ;
      //fi->zoom = 0; /* gb10: now we have the recalculate_zoom flag this shouldn't be necessary */

#if HIGHLIGHT_FILTERED_COLUMNS
      /*!> \todo This code highlights columns that are filtered.
       * It is requested functionality but it needs to be optional
       * so that the user can turn it off. */
      ZMapWindowContainerGroup column = (ZMapWindowContainerGroup)((FooCanvasItem *)fi)->parent;
      GdkColor white = { 0xffffffff, 0xffff, 0xffff, 0xffff } ;                /* is there a column background config colour? */
      GdkColor *fill = &white;

      if(fi->n_filtered && filter->window)
        {
          zMapWindowGetFilteredColour(filter->window,&fill);

          column->flags.filtered = 1;

          // NO:        zMapWindowCanvasFeaturesetSetBackground((FooCanvasItem *) fi, fill, NULL);
          // must do the column not the featureset
          zmapWindowDrawSetGroupBackground(column, 0, 1, 1.0, ZMAP_CANVAS_LAYER_COL_BACKGROUND, fill, NULL);

          foo_canvas_item_request_redraw(((FooCanvasItem *)fi)->parent);
        }
      else
        {
          column->flags.filtered = 0;
          /*
            this col is selected or else we could not operate its filter button
            so we revert to select not normal background
          */
          zmapWindowFocusHighlightHotColumn(filter->window->focus);
        }
#endif

      if(fi->bumped)
        {
          ZMapWindowCompressMode compress_mode;

          if (zMapWindowMarkIsSet(filter->window))
            compress_mode = ZMAPWINDOW_COMPRESS_MARK ;
          else
            compress_mode = ZMAPWINDOW_COMPRESS_ALL ;

          zmapWindowColumnBumpRange(filter->column, ZMAPBUMP_INVALID, ZMAPWINDOW_COMPRESS_INVALID);

          /* dissapointing: we only need to reposition columns to the right of this one */

          zmapWindowFullReposition(filter->window->feature_root_group,TRUE, "filter") ;
        }
      else
        {
          zMapWindowCanvasFeaturesetRedraw(fi, fi->zoom);
        }
    }

  return(fi->n_filtered);
}


void zMapWindowFeaturesetSetFeatureWidth(ZMapWindowFeaturesetItem featureset_item, ZMapWindowCanvasFeature feat)
{
  ZMapFeature feature = feat->feature;
  ZMapFeatureTypeStyle style = *feature->style;

  feat->width = featureset_item->width;

  if(feature->population)        /* collapsed duplicated features, takes precedence over score */
    {
      double score = (double) feature->population;

      feat->score = zMapWindowCanvasFeatureGetNormalisedScore(style, score);

      if ((zMapStyleGetScoreMode(style) == ZMAPSCORE_WIDTH) || (zMapStyleGetScoreMode(style) == ZMAPSCORE_HEAT_WIDTH))
        feat->width = zMapWindowCanvasFeatureGetWidthFromScore(style, featureset_item->width, score);
    }
  else if(feature->flags.has_score)
    {
      if(featureset_item->style->mode == ZMAPSTYLE_MODE_GRAPH)
        {
          feat->score = zMapWindowCanvasFeatureGetNormalisedScore(style, feature->score);
          if(featureset_item->style->mode_data.graph.mode != ZMAPSTYLE_GRAPH_HEATMAP)
            feat->width = featureset_item->width * feat->score;
        }
      else
        {
          if ((zMapStyleGetScoreMode(style) == ZMAPSCORE_WIDTH && feature->flags.has_score))
            feat->width = zMapWindowCanvasFeatureGetWidthFromScore(style, featureset_item->width, feature->score);
          else if(zMapStyleGetScoreMode(style) == ZMAPSCORE_PERCENT)
            feat->width = zMapWindowCanvasFeatureGetWidthFromScore(style, featureset_item->width, feature->feature.homol.percent_id);
        }
    }
}


ZMapWindowCanvasFeature zMapWindowFeaturesetAddFeature(ZMapWindowFeaturesetItem featureset_item,
                                                       ZMapFeature feature, double y1, double y2)
{
  ZMapWindowCanvasFeature feat = NULL ;
  ZMapFeatureTypeStyle style = *feature->style;
  zmapWindowCanvasFeatureType type = FEATURE_INVALID;

  if (zMapFeatureIsValid((ZMapFeatureAny) feature))
    {
      if(style)
        type = feature_types[zMapStyleGetMode(style)];
      if(type == FEATURE_INVALID)                /* no style or feature type not implemented */
        return NULL;

      feat = zMapWindowCanvasFeatureAlloc(type);

      feat->feature = feature;
      feat->type = type;

      feat->y1 = y1;
      feat->y2 = y2;

      if(y2 - y1 > featureset_item->longest)
        featureset_item->longest = y2 - y1;

      featuresetAddToIndex(featureset_item, feat);
    }

  return feat;
}


/*
  delete feature from the features list and trash the index
  returns no of features left

  rather tediously we can't get the feature via the index as that will give us the feature not the list node pointing to it.
  so to delete a whole featureset we could have a quadratic search time unless we delete in order
  but from OTF if we delete old ones we do this via a small hash table
  we don't delete elsewhere, execpt for legacy gapped alignments, so this works ok by fluke
  NOTE contract expended BAM features will delete 1000 times, so may be slow

  this function's a bit horrid: when we find the feature to delete we have to look it up in the index to repaint
  we really need a column refresh

  we really need to rethink this: deleting was not considered
*/
/*! \todo #warning need to revist this code and make it more efficient */
// ideas:
// use a skip list exclusively ??
// use features list for loading, convert to skip list and remove features
// can add new features via list and add to skip list (extract skip list, add to features , sort and re-create skip list)

/* NOTE Here we improve the efficiency of deleting a feature with some rubbish code
 * we add a pointer to a feature's list mode in the fi->features list
 * which allows us to delete that feature from the list without searching for it
 * this is building rubbish on top of rubbish: really we should loose the features list
 * and _only_ store references to features in the skip list
 * there's reasons why not:
 * - haven't implemented add/delete in zmapSkipList.c, which requires fiddly code to prevent degeneracy.
 *   (Initially not implemented as not used, but now needed).
 * - graph density items recalculate bins and have a seperate list of features that get indexed -> we should do this using another skip list
 * - sometimes we may wish to sort differently, having a diff list to sort helps... we can use glib functions.
 * So to implement this quickly i added an extra pointer in the CanvasFeature struct.
 *
 * unfortunately glib sorts by creating new list nodes (i infer) so that the from pointer is invalid
 *
 */
/* NOTE it turns out that g_list_sort invalidates ->from pointers so we can't use them
 * however to delete all features we can just destroy the featureset,
 * it will be created again when we add a new feature
 */
int zMapWindowFeaturesetItemRemoveFeature(FooCanvasItem *foo, ZMapFeature feature)
{
  ZMapWindowFeaturesetItem fi = NULL ;

  zMapReturnValIfFail(foo && feature, 0) ;
  fi = (ZMapWindowFeaturesetItem) foo ;

#if 1 // ORIGINAL_SLOW_VERSION
  GList *l;
  ZMapWindowCanvasFeature feat;

  for(l = fi->features;l;)
    {
      GList *del;

      feat = (ZMapWindowCanvasFeature) l->data;

      if(zmapWindowCanvasFeatureValid(feat) && feat->feature == feature)
        {
          /* NOTE the features list and display index both point to the same structs */

          zmap_window_canvas_featureset_expose_feature(fi, feat);

          zmapWindowCanvasFeatureFree(feat);
          del = l;
          l = l->next;
          fi->features = g_list_delete_link(fi->features,del);
          fi->n_features--;

          /*! \todo #warning review this (feature remove) */
          // not sure what this is here for: we-d have to process the sideways list??
          // and that does not give us the features list instsead the canvasfeature structs so no workee
          // perhaps the ultimate caller calls several times??
          if(fi->link_sideways)        /* we'll get calls for each sub-feature */
            break;
          /* else have to go through the whole list; fortunately transcripts are low volume */
        }
      else
        {
          l = l->next;
        }
    }

  /* NOTE we may not have an index so this flag must be unset seperately */
  fi->linked_sideways = FALSE;  /* See code below: this was slack */


#else

  ZMapWindowCanvasFeature gs = zmap_window_canvas_featureset_find_feature(fi,feature);

  if(gs)
    {
      GList *link = gs->from;
      /* NOTE search for ->from if you revive this */
      /* glib  g_list_sort() invalidates these adresses -> preserves the data but not the list elements */

      zmap_window_canvas_featureset_expose_feature(fi, gs);


      //      if(fi->linked_sideways)
      {
        if(gs->left)
          gs->left->right = gs->right;
        if(gs->right)
          gs->right->left = gs->left;
      }

      zmapWindowCanvasFeatureFree(gs);
      fi->features = g_list_delete_link(fi->features,link);
      fi->n_features--;
    }
#endif

  /* not strictly necessary to re-sort as the order is the same
   * but we avoid the index becoming degenerate by doing this
   * better to implement zmapSkipListRemove() properly
   */
  if(fi->display_index)
    {
      /* need to recalc bins */
      /* quick fix FTM, de-calc which requires a re-calc on display */
      zMapSkipListDestroy(fi->display_index, NULL);
      fi->display_index = NULL;
      /* is still sorted if it was before */
    }

  return fi->n_features;
}

int zMapWindowFeaturesetGetNumFeatures(ZMapWindowFeaturesetItem featureset_item)
{
  return featureset_item->n_features ;
}

/*
 *
 */
void zMapWindowFeaturesetRemoveAllGraphics(ZMapWindowFeaturesetItem featureset_item )
{

  GList *l;
  ZMapWindowCanvasFeature feat;

  if (featureset_item->features)
    {

  for(l = featureset_item->features; l ; )
    {
      //GList *del;

      feat = (ZMapWindowCanvasFeature) l->data;

      if(zmapWindowCanvasFeatureValid(feat))
        {
          /* NOTE the features list and display index both point to the same structs */

          //zmap_window_canvas_featureset_expose_feature(fi, feat);

          zmapWindowCanvasFeatureFree(feat);
          //del = l;
          l = l->next;
          //fi->features = g_list_delete_link(fi->features,del);
          //fi->n_features--;
        }
      else
        {
          l = l->next;
        }
    }

  g_list_free(featureset_item->features) ;
  featureset_item->features = NULL ;

    }

   if(featureset_item->display_index)
    {
      zMapSkipListDestroy(featureset_item->display_index, NULL);
      featureset_item->display_index = NULL;
    }

  featureset_item->n_features = 0 ;
}

/*
 * Adds a graphics item to the featureset_item.
 */
ZMapWindowCanvasGraphics zMapWindowFeaturesetAddGraphics(ZMapWindowFeaturesetItem featureset_item,
                                                         zmapWindowCanvasFeatureType type,
                                                         double x1, double y1, double x2, double y2,
                                                         GdkColor *fill, GdkColor *outline, char *text)
{
  ZMapWindowCanvasGraphics feat;
  gulong fill_pixel= 0, outline_pixel = 0;
  FooCanvasItem *foo = (FooCanvasItem *) featureset_item;

  if (type == FEATURE_INVALID || type < FEATURE_GRAPHICS)
    return NULL;

  feat = (ZMapWindowCanvasGraphics)zMapWindowCanvasFeatureAlloc(type);

  feat->type = type;

  if(x1 > x2) {  double x = x1; x1 = x2; x2 = x; }        /* boring.... */
  if(y1 > y2) {  double x = y1; y1 = y2; y2 = x; }

  feat->y1 = y1;
  feat->y2 = y2;

  feat->x1 = x1;
  feat->x2 = x2;

  feat->text = text;

  if(fill)
    {
      fill_pixel = zMap_gdk_color_to_rgba(fill);
      feat->fill = foo_canvas_get_color_pixel(foo->canvas, fill_pixel);
      feat->flags |= WCG_FILL_SET;
    }
  if(outline)
    {
      outline_pixel = zMap_gdk_color_to_rgba(outline);
      feat->outline = foo_canvas_get_color_pixel(foo->canvas, outline_pixel);
      feat->flags |= WCG_OUTLINE_SET;
    }

  if(y2 - y1 > featureset_item->longest)
    featureset_item->longest = y2 - y1 + 1;

  featuresetAddToIndex(featureset_item, (ZMapWindowCanvasFeature) feat);

  return feat;
}



int zMapWindowFeaturesetRemoveGraphics(ZMapWindowFeaturesetItem featureset_item, ZMapWindowCanvasGraphics feat)
{

  /*! \todo #warning zMapWindowFeaturesetRemoveGraphics not implemented */
  /* is this needed? yes: diff struct, yes: gets called on revcomp */

#if 0
  /* copy from remove feature */


  /* not strictly necessary to re-sort as the order is the same
   * but we avoid the index becoming degenerate by doing this
   * better to implement zmapSkipListRemove() properly
   */
  if(fi->display_index)
    {
      /* need to recalc bins */
      /* quick fix FTM, de-calc which requires a re-calc on display */
      zMapSkipListDestroy(fi->display_index, NULL);
      fi->display_index = NULL;
      /* is still sorted if it was before */
    }

  return fi->n_features;
#else
  return 0;
#endif
}




/*
 * remove all the features in the given set from the canvas item
 * we may have several mapped into one featuresetItem
 * we could just remove each feature individually , but this is quicker and easier
 */

int zMapWindowFeaturesetItemRemoveSet(FooCanvasItem *foo, ZMapFeatureSet featureset, gboolean destroy)
{
  ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) foo;
  int n_feat = fi->n_features;

  if(!ZMAP_IS_WINDOW_FEATURESET_ITEM(foo))
    return 0;
#if 1
  GList *l;
  ZMapWindowCanvasFeature feat;
  ZMapFeatureSet set;

  for (l = fi->features;l;)
    {
      GList *del;

      feat = (ZMapWindowCanvasFeature) l->data;

      if (zmapWindowCanvasFeatureValid(feat))
        {
          set = (ZMapFeatureSet) feat->feature->parent;

          if (set == featureset)
            {
              /* NOTE the features list and display index both point to the same structs */

              zmap_window_canvas_featureset_expose_feature(fi, feat);

              zmapWindowCanvasFeatureFree(feat);
              del = l;
              l = l->next;
              fi->features = g_list_delete_link(fi->features,del);
              fi->n_features--;
            }
          else
            {
              l = l->next;
            }
        }
      else
        {
          l = l->next;
        }
    }

  /* NOTE we may not have an index so this flag must be unset seperately */
  fi->linked_sideways = FALSE;  /* See code below: this was slack */


#else
#if CODE_COPIED_FROM_REMOVE_FEATURE
  ZMapWindowCanvasFeature gs = zmap_window_canvas_featureset_find_feature(fi,feature);

  if(gs)
    {
      GList *link = gs->from;
      /* NOTE search for ->from if you revive this */
      /* glib  g_list_sort() invalidates these adresses -> preserves the data but not the list elements */

      zmap_window_canvas_featureset_expose_feature(fi, gs);


      //      if(fi->linked_sideways)
      {
        if(gs->left)
          gs->left->right = gs->right;
        if(gs->right)
          gs->right->left = gs->left;
      }

      zmapWindowCanvasFeatureFree(gs);
      fi->features = g_list_delete_link(fi->features,link);
      fi->n_features--;
    }
#endif
#endif

  /* not strictly necessary to re-sort as the order is the same
   * but we avoid the index becoming degenerate by doing this
   * better to implement zmapSkipListRemove() properly
   */
  if(fi->display_index)
    {
      /* need to recalc bins */
      /* quick fix FTM, de-calc which requires a re-calc on display */
      zMapSkipListDestroy(fi->display_index, NULL);
      fi->display_index = NULL;
      /* is still sorted if it was before */
    }

  n_feat = fi->n_features;


  //printf("canvas remove set %p %s %s: %d features\n", fi, g_quark_to_string(fi->id), g_quark_to_string(featureset->unique_id), n_feat);

  if(!fi->n_features && destroy)        /* if the canvasfeatureset is used only as a background we may not want to do this */
    {
      // don-t do this we get glib **** errors
      //          zmap_window_featureset_item_item_destroy((GtkObject *) fi);
      gtk_object_destroy(GTK_OBJECT(fi));
    }


  return n_feat;
}


/*!
 * \brief Return the number of filtered-out features in a featureset item
 */
int zMapWindowFeaturesetItemGetNFiltered(FooCanvasItem *item)
{
  int result = 0;

  if (item)
    {
      ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem)item;

      if (fi)
        result = fi->n_filtered;
    }

  return result;
}


/*! \todo #warning make this into a foo canvas item class func */

/* get the bounds of the current feature which has been set by the caller */
void zMapWindowCanvasFeaturesetGetFeatureBounds(FooCanvasItem *foo,
                                                double *rootx1, double *rooty1, double *rootx2, double *rooty2)
{
  ZMapWindowCanvasItem item = (ZMapWindowCanvasItem) foo;
  ZMapWindowFeaturesetItem fi;

  zMapReturnIfFail(foo) ;

  fi = (ZMapWindowFeaturesetItem) foo;

  if(rootx1)
    *rootx1 = fi->dx;
  if(rootx2)
    *rootx2 = fi->dx + fi->width;
  if(rooty1)
    *rooty1 = item->feature->x1;
  if(rooty2)
    *rooty2 = item->feature->x2;

  return ;
}





/*
 *                      Internal routines.
 */


static void featuresetAddToIndex(ZMapWindowFeaturesetItem featureset_item, ZMapWindowCanvasFeature feat)
{
  /* even if they come in order we still have to sort them to be sure so just add to the front */
  /* NOTE we asign the from pointer here: not just more efficient if we have eg 60k features but essential to prepend */
  //  feat->from =
  // only for feature not graphics, from does not work anyway

  featureset_item->features = g_list_prepend(featureset_item->features,feat);
  featureset_item->n_features++;

#if STYLE_DEBUG
  if(feat->type < FEATURE_GRAPHICS)
    {
      ZMapFeatureSet featureset = (ZMapFeatureSet) feat->feature->parent;
      ZMapFeature feature = feat->feature;

      printf("add item %s %s @%p %p: %ld/%d style %p/%p %s\n",
             g_quark_to_string(featureset_item->id),g_quark_to_string(feature->unique_id),
             featureset, feature,
             featureset_item->n_features, g_list_length(featureset_item->features),
             featureset->style, *feature->style, g_quark_to_string(featureset->style->unique_id));
    }
#endif

  /* add to the display bins if index already created */
  if(featureset_item->display_index)
    {
      /* have to re-sort... NB SkipListAdd() not exactly well tested, so be dumb */
      /* it's very rare that we add features anyway */
      /* although we could have several featuresets being loaded into one column */
      /* whereby this may be more efficient ? */
#if 1
      {
        /* need to recalc bins */
        /* quick fix FTM, de-calc which requires a re-calc on display */
        zMapSkipListDestroy(featureset_item->display_index, NULL) ;
        featureset_item->display_index = NULL ;
      }
    }
  /* must set this independantly as empty columns with no index get flagged as sorted */
  featureset_item->features_sorted = FALSE;
  //  featureset_item->recalculate_zoom = TRUE;        /* trigger a recalc */
#else
  // untested code
  {
    /* NOTE when this is implemented properly then it might be best to add to a linked list if there's no index created
     * as that might be more effecient,  or maybe it won't be.
     * i'd guess that creating a skip list from a sorted list is faster
     * Worth considering and also timing this,
     */
    featureset_item->display_index =
      zMapSkipListAdd(featureset_item->display_index, zMapWindowFeatureCmp, feat);
    /* NOTE need to fix linked_sideways */

  }
}
#endif
}





static void setFeaturesetColours(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature feat)
{

  if (zmapWindowCanvasFeatureValid(feat) && zMapFeatureIsValid((ZMapFeatureAny)(feat->feature)))
    {
      FooCanvasItem *item = (FooCanvasItem *) fi;
      ZMapFeature feature = feat->feature ;
      ZMapStyleColourType ct;
      static gboolean tmp_debug = FALSE ;


      /* NOTE carefully balanced code:
       * we do all blurred features then all focussed ones
       * if the style changes then we look up the colours
       * so we reset the style before doing focus
       * other kinds of focus eg evidence do not have colours set in the style
       * so mixed focus types are not a problem
       */

      if (tmp_debug)
        zMapUtilsDebugPrintf(stderr, "Feature: \"%s\", \"%s\"\n",
                             g_quark_to_string(feature->original_id), g_quark_to_string(feature->unique_id)) ;

      /* diff style: set colour from style */
      /* cache style for a single featureset
       * if we have one source featureset in the column then this works fastest (eg good for trembl/ swissprot)
       * if we have several (eg BAM rep1, rep2, etc,  Repeatmasker then we get to switch around frequently
       * but hopefully it's faster than not caching regardless
       * beware of enum ZMapWindowFocusType not being a bit-mask
       */
      if ((fi->featurestyle != *(feat->feature->style))
          || !(fi->frame && zMapStyleIsFrameSpecific(*feat->feature->style)))
        {
          GdkColor *fill = NULL,*draw = NULL, *outline = NULL;
          ZMapFrame frame;
          ZMapStrand strand;

          /* This is the only place this is set.....seems to cause problems in the code elsewhere.... */
          fi->featurestyle = *(feat->feature->style) ;

          /* eg for glyphs these get mixed up in one column so have to set for the feature not featureset */
          frame = zMapFeatureFrame(feat->feature);
          strand = feat->feature->strand;

          ct = feat->flags & WINDOW_FOCUS_GROUP_FOCUSSED ? ZMAPSTYLE_COLOURTYPE_SELECTED : ZMAPSTYLE_COLOURTYPE_NORMAL;

          zmapWindowCanvasItemGetColours(fi->featurestyle, strand, frame, ct , &fill, &draw, &outline, NULL, NULL);

          /* can cache these in the feature? or style?*/

          fi->fill_set = FALSE;

          if(fill)
            {
              fi->fill_set = TRUE;
              fi->fill_colour = zMap_gdk_color_to_rgba(fill);
              fi->fill_pixel = foo_canvas_get_color_pixel(item->canvas, fi->fill_colour);
            }

          fi->outline_set = FALSE;

          if(outline)
            {
              fi->outline_set = TRUE;
              fi->outline_colour = zMap_gdk_color_to_rgba(outline);
              fi->outline_pixel = foo_canvas_get_color_pixel(item->canvas, fi->outline_colour);
            }

        }
    }

  return ;
}



/* a slightly ad-hoc function
 * really the feature context should specify complex features
 * but for historical reasons alignments come disconnected
 * and we have to join them up by inference (same name)
 * we may have several context featuresets but by convention all these have features with different names
 * do we have to do the same w/ transcripts? watch this space:
 *
 */
/* revisit this when VULGAR alignments are implemented: call from zmapView code */
static void itemLinkSideways(ZMapWindowFeaturesetItem fi)
{
  GList *l ;
  ZMapWindowCanvasFeature left, right ;                /* feat -ures */
  GQuark name ;
  ZMapFeatureTypeStyle style = fi->style ;
  zmapWindowCanvasFeatureType type ;
  gboolean sort_by_featureset = FALSE ;

  zMapReturnIfFail(fi) ;

  type = feature_types[zMapStyleGetMode(style)] ;

  /* we use the featureset features list which sits there in parallel with the skip list (display index) */
  /* sort by name and start coord */
  /* link same name features with ascending query start coord */

  /* if we ever need to link by something other than same name
   * then we can define a feature type specific sort function
   * and revive the zMapWindowCanvasFeaturesetLinkFeature() function
   */

  if (zMapStyleGetDefaultBumpMode(style) == ZMAPBUMP_FEATURESET_NAME)
    sort_by_featureset = TRUE ;

  /*
     ok...this is fine for alignments but we need to sort differently for histogram, by
     featuresetname....so need a parameter to this function so we can choose how to link.....
     just copy code below but do feature set names instead......we should supply a sort func
     and a func to get the name...below it's just the feature id but we need the featureset
     id.....
  */
  if (sort_by_featureset)
    fi->features = g_list_sort(fi->features, zMapFeatureSetNameCmp) ;
  else
    fi->features = g_list_sort(fi->features, zMapFeatureNameCmp) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (sort_by_featureset)
    zmapWindowCanvasFeaturesetDumpFeatures(fi) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* this is really odd...don't understand... */
  fi->features_sorted = FALSE;                              /* WHY IS THIS NOW SET FALSE !!!!! */



  /* THIS ALSO NEEDS RECODING.... */

  for (l = fi->features, name = 0 ; l ; l = l->next)
    {
      GQuark feat_name ;

      right = (ZMapWindowCanvasFeature)(l->data) ;

      /* HOW CAN THIS HAPPEN....WHAT CAUSES SOMETHING NOT TO BE VALID... */
      if (!zmapWindowCanvasFeatureValid(right))
        continue ;

      right->left = right->right = NULL;                /* we can re-calculate so must zero */


      if (sort_by_featureset)
        feat_name = right->feature->parent->unique_id ;
      else
        feat_name = right->feature->original_id ;



      if (name == feat_name)
        {
          right->left = left;
          left->right = right;
        }


      if (sort_by_featureset)
        {
          char *name_str, *feat_name_str ;

          name_str = (char *)g_quark_to_string(name) ;
          feat_name_str = (char *)g_quark_to_string(feat_name) ;

          /* SORTING SEEMS TO BE OK...... */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          if (name != feat_name)
            zMapDebugPrintf("featuresets: \"%s\"\t\"%s\"\n", name_str, feat_name_str) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
        }

      left = right ;

      if (sort_by_featureset)
        name = left->feature->parent->unique_id ;
      else
        name = left->feature->original_id ;

    }

  fi->linked_sideways = TRUE;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (sort_by_featureset)
    zmapWindowCanvasFeaturesetDumpFeatures(fi) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return ;
}





/* Default function to check if the given x,y coord is within a feature, this
 * function assumes the feature is box-like. */
static double featurePoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
                           double item_x, double item_y, int cx, int cy,
                           double local_x, double local_y, double x_off)
{
  double best = 1.0e36 ;
  double can_start, can_end ;

  zMapReturnValIfFail(gs, best) ;

  /* Get feature extent on display. */
  /* NOTE cannot use feature coords as transcript exons all point to the same feature */
  /* alignments have to implement a special function to handle bumped features - the first exon gets expanded to cover the whole */
  /* when we get upgraded to vulgar strings these can be like transcripts... except that there's a performance problem due to volume */
  /* perhaps better to add  extra display/ search coords to ZMapWindowCanvasFeature ?? */
  can_start = gs->y1;         //feature->x1 ;
  can_end = gs->y2;        //feature->x2 ;
  zmapWindowFeaturesetS2Ccoords(&can_start, &can_end) ;


  if (can_start <= local_y && can_end >= local_y)                            /* overlaps cursor */
    {
      double wx ;
      double left, right ;

      wx = x_off; // - (gs->width / 2) ;

      if (fi->bumped)
        wx += gs->bump_offset ;

      /* get coords within one pixel */
      left = wx - 1 ;                                            /* X coords are on fixed zoom, allow one pixel grace */
      right = wx + gs->width + 1 ;

      if (local_x > left && local_x < right)                            /* item contains cursor */
        {
          best = 0.0;
        }
    }

  return best ;
}

/* Default function to check if the given x,y coord is within a feature, this
 * function assumes the feature is box-like. */
static double graphicsPoint(ZMapWindowFeaturesetItem fi, ZMapWindowCanvasFeature gs,
                            double item_x, double item_y, int cx, int cy,
                            double local_x, double local_y, double x_off)
{
  double best = 1.0e36 ;
  double can_start, can_end ;
  ZMapWindowCanvasGraphics gfx = (ZMapWindowCanvasGraphics) gs;

  zMapReturnValIfFail(gs, best) ;




  /* Get feature extent on display. */
  can_start = gs->y1;         //feature->x1 ;
  can_end = gs->y2;        //feature->x2 ;
  zmapWindowFeaturesetS2Ccoords(&can_start, &can_end) ;


  if (can_start <= local_y && can_end >= local_y)                            /* overlaps cursor */
    {
      double left, right ;

      /* get coords within one pixel */
      left = gfx->x1 - 1 ;                                            /* X coords are on fixed zoom, allow one pixel grace */
      right = gfx->x2 + 1 ;

      if (local_x > left && local_x < right)                            /* item contains cursor */
        {
          best = 0.0;
        }
    }

  return best ;
}





static void zmap_window_featureset_item_item_destroy (GtkObject *object)
{
  ZMapWindowFeaturesetItem featureset_item;
  GList *features;
  ZMapWindowCanvasFeature feat;

  /* mh17 NOTE: this function gets called twice for every object via a very very tall stack */
  /* no idea why, but this is all harmless here if we make sure to test if pointers are valid */
  /* what's more interesting is why an object has to be killed twice */

  /* having changed it from GtkObject->destroy to GObject->destroy
   * it seems that FootCanvasitem destroy eventually calls remove from group
   * which calls dispose 'just to be sure'
   * so we have to ignore 2nd time round
   */
  /* didn-t change anything so i put it back to destroy
   * foo_canvas_re uses destroy not dispose so there must be some reaosn behind it
   * ...and that fixed it: the key is not to chain up to the parent 2nd time round
   */


  //printf("zmap_window_featureset_item_item_destroy %p\n",object);

  g_return_if_fail(ZMAP_IS_WINDOW_FEATURESET_ITEM(object));



  featureset_item = ZMAP_WINDOW_FEATURESET_ITEM(object);




  if(g_hash_table_remove(featureset_class_G->featureset_items,GUINT_TO_POINTER(featureset_item->id)))
    {

      //printf("destroy featureset %s %ld features\n",g_quark_to_string(featureset_item->id), featureset_item->n_features);

      if(featureset_item->display_index)
        {
          zMapSkipListDestroy(featureset_item->display_index, NULL);
          featureset_item->display_index = NULL;
          featureset_item->features_sorted = FALSE;
        }
      if(featureset_item->display)        /* was re-binned */
        {
          for(features = featureset_item->display; features; features = g_list_delete_link(features,features))
            {
              feat = (ZMapWindowCanvasFeature) features->data;
              zmapWindowCanvasFeatureFree(feat);
            }
          featureset_item->display = NULL;
        }

      if(featureset_item->features)
        {
          /* free items separately from the index as conceivably we may not have an index */
          for(features = featureset_item->features; features; features = g_list_delete_link(features,features))
            {
              feat = (ZMapWindowCanvasFeature) features->data;
              zmapWindowCanvasFeatureFree(feat);
            }
          featureset_item->features = NULL;
          featureset_item->n_features = 0;
        }

      // printf("featureset %s: %ld %ld %ld,\n",g_quark_to_string(featureset_item->id), n_block_alloc, n_feature_alloc, n_feature_free);

      zMapWindowCanvasFeaturesetFree(featureset_item);        /* must tidy optional set data*/

      if(featureset_item->opt)
        {
          g_free(featureset_item->opt);
          featureset_item->opt = NULL;
        }


      if(featureset_item->gc)
        {
          g_object_unref(featureset_item->gc);
          featureset_item->gc = NULL;
        }

      //printf("chaining to parent... \n");
      if(GTK_OBJECT_CLASS (parent_class_G)->destroy)
        GTK_OBJECT_CLASS (parent_class_G)->destroy (object);
    }

  return ;
}




static guint32 gdk_color_to_rgba(GdkColor *color)
{
  guint32 rgba = 0;

  rgba = ((color->red & 0xff00) << 16  |
          (color->green & 0xff00) << 8 |
          (color->blue & 0xff00)       |
          0xff);

  return rgba;
}



/* Sort on unique (== canonicalised) name. */
#if NOT_USED
static gint setNameCmp(gconstpointer a, gconstpointer b)
{
  int result ;
  ZMapFeatureSet featureset_a = (ZMapFeatureSet)a ;
  ZMapFeatureSet featureset_b = (ZMapFeatureSet)b ;

  if (featureset_a->unique_id < featureset_b->unique_id)
    result = -1 ;
  else if (featureset_a->unique_id > featureset_b->unique_id)
    result = 1 ;
  else
    result = 0 ;

  return result ;
}
#endif



static void printCanvasFeature(void *data, void *user_data_unused)
{
  ZMapWindowCanvasFeature canvas_feature = (ZMapWindowCanvasFeature)data ;

  zMapDebugPrintf("\"%s\"(\"%s\") - \"%s\"(\"%s\")\t" CANVAS_FORMAT_DOUBLE ", " CANVAS_FORMAT_DOUBLE "\t"
                  "width: " CANVAS_FORMAT_DOUBLE "\tbump_offset: " CANVAS_FORMAT_DOUBLE
                  "\t bump_col: %d\tleft: %s\tright: %s\n",
                  zMapFeatureName((ZMapFeatureAny)(canvas_feature->feature->parent)),
                  zMapFeatureUniqueName((ZMapFeatureAny)(canvas_feature->feature->parent)),
                  zMapFeatureName((ZMapFeatureAny)(canvas_feature->feature)),
                  zMapFeatureUniqueName((ZMapFeatureAny)(canvas_feature->feature)),
                  canvas_feature->y1, canvas_feature->y2,
                  canvas_feature->width, canvas_feature->bump_offset, canvas_feature->bump_col,
                  ((canvas_feature->left)
                   ? zMapFeatureUniqueName((ZMapFeatureAny)(canvas_feature->left->feature)) : "NULL"),
                  ((canvas_feature->right)
                   ? zMapFeatureUniqueName((ZMapFeatureAny)(canvas_feature->right->feature)) : "NULL")) ;

  return ;
}



/* A GFunc() called from zMapWindowCanvasFeaturesetGetSetIDAtPos() to find the subcol
 * containing position, the first subcol found is the one reported. */
static void findNameAtPosCB(gpointer data, gpointer user_data)
{
  ZMapWindowCanvasSubCol sub_col = (ZMapWindowCanvasSubCol)data ;
  FindIDAtPosData pos_data = (FindIDAtPosData)user_data ;

  if (!(pos_data->id_at_position))                     /* once we've found it that's it. */
    {
      if (pos_data->position >= sub_col->offset && pos_data->position < (sub_col->offset + sub_col->width))
        {
          pos_data->id_at_position = sub_col->subcol_id ;
        }
    }

  return ;
}


/* A GFunc() to free a list which is an element of another list. */
static void groupListFreeCB(gpointer data, gpointer user_data_unused)
{
  GList *grouped_features = (GList *)data ;

  g_list_free(grouped_features) ;

  return ;
}


