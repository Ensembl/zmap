/*  File: zmapWindowTranscriptFeature.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Feb 22 07:44 2011 (edgrif)
 * Created: Wed Dec  3 10:02:22 2008 (rds)
 * CVS info:   $Id: zmapWindowTranscriptFeature.c,v 1.14 2011-02-24 13:59:04 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>




#include <include/ZMap/zmapEnum.h>
#include <include/ZMap/zmapStyle.h>

#include <zmapWindowTranscriptFeature_I.h>

enum
  {
    TRANSCRIPT_INTERVAL_0,		/* invalid */
  };

#define DEFAULT_LINE_WIDTH 1





static void zmap_window_transcript_feature_class_init  (ZMapWindowTranscriptFeatureClass transcript_class);
static void zmap_window_transcript_feature_init        (ZMapWindowTranscriptFeature      transcript);
static void zmap_window_transcript_feature_post_create(ZMapWindowCanvasItem canvas_item) ;
static void zmap_window_transcript_feature_set_property(GObject               *object,
							guint                  param_id,
							const GValue          *value,
							GParamSpec            *pspec);
static void zmap_window_transcript_feature_get_property(GObject               *object,
							guint                  param_id,
							GValue                *value,
							GParamSpec            *pspec);
static void zmap_window_transcript_feature_destroy     (GObject *object);


static void zmap_window_transcript_feature_set_colour(ZMapWindowCanvasItem   transcript,
						      FooCanvasItem         *interval,
						      ZMapFeatureSubPartSpan sub_feature,
						      ZMapStyleColourType    colour_type,
						      GdkColor              *default_fill,
                                          GdkColor              *border);
static FooCanvasItem *zmap_window_transcript_feature_add_interval(ZMapWindowCanvasItem   transcript,
								  ZMapFeatureSubPartSpan sub_feature,
								  double top,  double bottom,
								  double left, double right);



/* Local globals. */
static ZMapWindowCanvasItemClass canvas_item_class_G ;
static gboolean create_locus_text_G = FALSE ;



/*
 *                     External routines.
 */


GType zMapWindowTranscriptFeatureGetType(void)
{
  static GType group_type = 0;

  if (!group_type)
    {
      static const GTypeInfo group_info =
	{
	  sizeof (zmapWindowTranscriptFeatureClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_transcript_feature_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowTranscriptFeature),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_transcript_feature_init
	};

      group_type = g_type_register_static (zMapWindowCanvasItemGetType(),
					   ZMAP_WINDOW_TRANSCRIPT_FEATURE_NAME,
					   &group_info,
					   0);
    }

  return group_type;
}



/*
 *                  Internal routines.
 */


/* object impl */
static void zmap_window_transcript_feature_class_init(ZMapWindowTranscriptFeatureClass transcript_class)
{
  ZMapWindowCanvasItemClass canvas_class;
  GObjectClass *gobject_class;
  GType canvas_item_type, parent_type;

  gobject_class = (GObjectClass *) transcript_class ;
  canvas_class  = (ZMapWindowCanvasItemClass)transcript_class ;

  gobject_class->set_property = zmap_window_transcript_feature_set_property;
  gobject_class->get_property = zmap_window_transcript_feature_get_property;

  canvas_item_type = g_type_from_name(ZMAP_WINDOW_TRANSCRIPT_FEATURE_NAME);
  parent_type      = g_type_parent(canvas_item_type);

  canvas_item_class_G = gtk_type_class(parent_type);

  gobject_class->dispose     = zmap_window_transcript_feature_destroy;

  canvas_class->post_create  = zmap_window_transcript_feature_post_create;
  canvas_class->add_interval = zmap_window_transcript_feature_add_interval;
  canvas_class->set_colour   = zmap_window_transcript_feature_set_colour;

  zmapWindowItemStatsInit(&(canvas_class->stats), ZMAP_TYPE_WINDOW_TRANSCRIPT_FEATURE) ;

  return ;
}



static void zmap_window_transcript_feature_post_create(ZMapWindowCanvasItem canvas_item)
{
  ZMapFeature feature;

  (* canvas_item_class_G->post_create)(canvas_item);

  if (create_locus_text_G && (feature = canvas_item->feature) && feature->feature.transcript.locus_id)
    {
      FooCanvasGroup *parent;
      FooCanvasItem *item;
      char *text;
      text = (char *)g_quark_to_string(feature->feature.transcript.locus_id);

      parent = FOO_CANVAS_GROUP(canvas_item->items[WINDOW_ITEM_OVERLAY]);

      item = foo_canvas_item_new(parent,
				 foo_canvas_text_get_type(),
				 "font",   "Lucida Console",
				 "anchor", GTK_ANCHOR_NW,
				 "x",      0.0,
				 "y",      0.0,
				 "text",   text,
				 NULL);
      foo_canvas_item_hide(item);
    }
}


static void zmap_window_transcript_feature_init(ZMapWindowTranscriptFeature transcript)
{
  ZMapWindowTranscriptFeatureClass transcript_class ;

  transcript_class = ZMAP_WINDOW_TRANSCRIPT_FEATURE_GET_CLASS(transcript) ;

  return ;
}




static void zmap_window_transcript_feature_set_property(GObject               *object,
							guint                  param_id,
							const GValue          *value,
							GParamSpec            *pspec)

{
  ZMapWindowCanvasItem canvas_item;
  ZMapWindowTranscriptFeature transcript;

  g_return_if_fail(ZMAP_IS_WINDOW_TRANSCRIPT_FEATURE(object));

  transcript = ZMAP_WINDOW_TRANSCRIPT_FEATURE(object);
  canvas_item = ZMAP_CANVAS_ITEM(object);

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_transcript_feature_get_property(GObject               *object,
							guint                  param_id,
							GValue                *value,
							GParamSpec            *pspec)
{
  return ;
}

static void zmap_window_transcript_feature_destroy(GObject *object)
{
  ZMapWindowTranscriptFeature transcript ;
  ZMapWindowTranscriptFeatureClass transcript_class ;
  GList *list ;

  transcript = ZMAP_WINDOW_TRANSCRIPT_FEATURE(object);
  transcript_class = ZMAP_WINDOW_TRANSCRIPT_FEATURE_GET_CLASS(transcript) ;

  list = transcript->overlay_reparented;

  while(list)
    {
      gtk_object_destroy(GTK_OBJECT(list->data));

      list = list->next;
    }

  g_list_free(transcript->overlay_reparented);
  transcript->overlay_reparented = NULL;

  if(G_OBJECT_CLASS(canvas_item_class_G)->dispose)
    (*G_OBJECT_CLASS(canvas_item_class_G)->dispose)(object);

  return ;
}


static void zmap_window_transcript_feature_set_colour(ZMapWindowCanvasItem   transcript,
						      FooCanvasItem         *interval,
						      ZMapFeatureSubPartSpan sub_feature,
						      ZMapStyleColourType    colour_type,
						      GdkColor              *default_fill,
                                          GdkColor              *border)
{
  ZMapFeatureTypeStyle style;
  gboolean make_clickable_req = FALSE;

  g_return_if_fail(transcript  != NULL);
  g_return_if_fail(sub_feature != NULL);
  g_return_if_fail(interval    != NULL);

  style = (ZMAP_CANVAS_ITEM_GET_CLASS(transcript)->get_style)(transcript);

  switch(sub_feature->subpart)
    {
    case ZMAPFEATURE_SUBPART_EXON:
    case ZMAPFEATURE_SUBPART_EXON_CDS:
      {
	GdkColor *xon_border = NULL, *xon_draw = NULL, *xon_fill = NULL ;

	/* fill = background, draw = foreground, border = outline */
	if((sub_feature->subpart == ZMAPFEATURE_SUBPART_EXON) &&
	   !(zMapStyleGetColours(style, STYLE_PROP_COLOURS, colour_type,
				 &xon_fill, &xon_draw, &xon_border)))
	  {
	    zMapLogWarning("Feature \"%s\" of feature set \"%s\" has no colours set.",
			   g_quark_to_string(transcript->feature->original_id),
			   g_quark_to_string(transcript->feature->parent->original_id)) ;
	  }

	if ((sub_feature->subpart == ZMAPFEATURE_SUBPART_EXON_CDS) &&
	    !(zMapStyleGetColours(style, STYLE_PROP_TRANSCRIPT_CDS_COLOURS, colour_type,
				  &xon_fill, &xon_draw, &xon_border)))
	  {
	    zMapLogWarning("Feature \"%s\" of feature set \"%s\" has a CDS but it's style, \"%s\","
			   "has no CDS colours set.",
			   g_quark_to_string(transcript->feature->original_id),
			   g_quark_to_string(transcript->feature->parent->original_id),
			   zMapStyleGetName(style)) ;

	    /* cds will default to normal colours if its own colours are not set. */
	    zMapStyleGetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
				&xon_fill, &xon_draw, &xon_border);
	  }

	if(colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED)
	  {
	    if(default_fill)
	      xon_fill = default_fill;
          if(border)
            xon_border = border;

#ifdef POSSIBLY_CORRECT
	    if(!xon_border)
#endif
	      g_object_get(G_OBJECT(interval),
			   "outline_color_gdk", &xon_border,
			   NULL);
	  }

	foo_canvas_item_set(interval,
			    "fill_color_gdk",    xon_fill,
			    "outline_color_gdk", xon_border,
			    NULL);

	if(make_clickable_req && !FOO_CANVAS_RE(interval)->fill_set)
	  {
	    foo_canvas_item_set(interval,
				"fill_color_gdk", xon_border,
				"fill_stipple",   ZMAP_CANVAS_ITEM_GET_CLASS(transcript)->fill_stipple,
				NULL);
	  }
      }
      break;
    case ZMAPFEATURE_SUBPART_INTRON_CDS:
    case ZMAPFEATURE_SUBPART_INTRON:
      {
	GdkColor *xon_border = NULL;
	gboolean intron_follow_cds = TRUE; /* needs to come from style. */

	if((sub_feature->subpart == ZMAPFEATURE_SUBPART_INTRON) &&
	   !(zMapStyleGetColours(style, STYLE_PROP_COLOURS, colour_type,
				 NULL, NULL, &xon_border)))
	  {
	    zMapLogWarning("Feature \"%s\" of feature set \"%s\" has no %s colour set.",
			   g_quark_to_string(transcript->feature->original_id),
			   g_quark_to_string(transcript->feature->parent->original_id),
                     zmapStyleColourType2ExactStr(colour_type));
	  }

	if((intron_follow_cds == TRUE)
	   && (sub_feature->subpart == ZMAPFEATURE_SUBPART_INTRON_CDS)
	   && !(zMapStyleGetColours(style, STYLE_PROP_TRANSCRIPT_CDS_COLOURS, colour_type,
				    NULL, NULL, &xon_border)))
	  {
	    zMapLogWarning("Feature \"%s\" of feature set \"%s\" has a CDS but it's style, \"%s\","
			   "has no CDS %s colour set.",
			   g_quark_to_string(transcript->feature->original_id),
			   g_quark_to_string(transcript->feature->parent->original_id),
			   zMapStyleGetName(style), zmapStyleColourType2ExactStr(colour_type));
	  }


	if(colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED && default_fill)
	  xon_border = default_fill;

	foo_canvas_item_set(interval,
			    "fill_color_gdk", xon_border,
			    NULL);
      }
      break;

    default:
      g_warning("Invalid ZMapWindowItemFeature->subpart for ZMapWindowTranscriptFeature object.");
      break;
    }

  if(create_locus_text_G && colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED)
    {
      FooCanvasGroup *overlay;
      if((overlay = FOO_CANVAS_GROUP(transcript->items[WINDOW_ITEM_OVERLAY])))
	{
	  ZMapWindowTranscriptFeature transcript_feature;
	  GList *list;

	  list = overlay->item_list;
	  transcript_feature = ZMAP_WINDOW_TRANSCRIPT_FEATURE(transcript);

	  while(list)
	    {
	      FooCanvasItem *o_interval;

	      o_interval = FOO_CANVAS_ITEM(list->data);

	      if(o_interval == transcript->mark_item)
		{
		  list = list->next;
		  continue;
		}

	      foo_canvas_item_show(o_interval);

	      transcript_feature->overlay_reparented = g_list_prepend(transcript_feature->overlay_reparented,
								      list->data);

	      zMapWindowCanvasItemReparent(o_interval, foo_canvas_root(o_interval->canvas));

	      foo_canvas_item_raise_to_top(o_interval);

	      list = list->next;
	    }

	}
    }
  else if(create_locus_text_G)
    {
      FooCanvasGroup *overlay;
      if((overlay = FOO_CANVAS_GROUP(transcript->items[WINDOW_ITEM_OVERLAY])))
	{
	  ZMapWindowTranscriptFeature transcript_feature;
	  GList *list;

	  transcript_feature = ZMAP_WINDOW_TRANSCRIPT_FEATURE(transcript);

	  list = transcript_feature->overlay_reparented;

	  while(list)
	    {
	      foo_canvas_item_hide(FOO_CANVAS_ITEM(list->data));
	      zMapWindowCanvasItemReparent(list->data, overlay);
	      list = list->next;
	    }
	  g_list_free(transcript_feature->overlay_reparented);
	  transcript_feature->overlay_reparented = NULL;
	}
    }

  return ;
}

static FooCanvasItem *zmap_window_transcript_feature_add_interval(ZMapWindowCanvasItem   transcript,
								  ZMapFeatureSubPartSpan sub_feature,
								  double point_a, double point_b,
								  double width_a, double width_b)
{
  FooCanvasItem *item = NULL;
  ZMapFeatureTypeStyle style;

  g_return_val_if_fail(transcript  != NULL, NULL);
  g_return_val_if_fail(sub_feature != NULL, NULL);

  style = (ZMAP_CANVAS_ITEM_GET_CLASS(transcript)->get_style)(transcript);

  switch(sub_feature->subpart)
    {
    case ZMAPFEATURE_SUBPART_EXON:
    case ZMAPFEATURE_SUBPART_EXON_CDS:
      {
	item = foo_canvas_item_new(FOO_CANVAS_GROUP(transcript),
				   FOO_TYPE_CANVAS_RECT,
				   "x1", width_a, "y1", point_a,
				   "x2", width_b, "y2", point_b,
				   NULL);

	break;
      }

    case ZMAPFEATURE_SUBPART_INTRON:
    case ZMAPFEATURE_SUBPART_INTRON_CDS:
      {
	/* Draw the intron lines, they go from the middle of the exons out to
	 * nearly the right hand edge of the exons. */
	FooCanvasPoints points;
	double coords[6];
	double mid_short, mid_long;
	int line_width = DEFAULT_LINE_WIDTH;

	mid_short = width_a + ((width_b - width_a) / 2.0);

	mid_long  = point_a + ((point_b - point_a) / 2.0);

	coords[0] = mid_short;
	coords[1] = point_a;

	coords[2] = width_b - line_width ;		    /* Make sure line does not extend past
							       edge of exon. */
	coords[3] = mid_long ;

	coords[4] = mid_short;
	coords[5] = point_b;

	points.coords     = &coords[0];
	points.num_points = 3;
	points.ref_count  = 1;	/* make sure noone tries to destroy */

	item = foo_canvas_item_new(FOO_CANVAS_GROUP(transcript),
				   FOO_TYPE_CANVAS_LINE,
				   "width_pixels",   line_width,
				   "points",         &points,
				   "join_style",     GDK_JOIN_BEVEL,
				   "cap_style",      GDK_CAP_BUTT,
				   NULL);

	break;
      }
    default:
      {
	break;
      }
    }

  return item;
}



