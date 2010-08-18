/*  File: zmapWindowSequenceFeature.c
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:  Function/methods for object representing either a
 *               DNA or a Peptide sequence. Handles display, coords,
 *               highlighting.
 *
 * Exported functions: See zmapWindowSequenceFeature.h
 * HISTORY:
 * Last edited: Aug 18 10:05 2010 (edgrif)
 * Created: Fri Jun 12 10:01:17 2009 (rds)
 * CVS info:   $Id: zmapWindowSequenceFeature.c,v 1.14 2010-08-18 09:24:51 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>
#include <ZMap/zmapPeptide.h>
#include <ZMap/zmapSequence.h>
#include <zmapWindowSequenceFeature_I.h>
#include <zmapWindowSequenceFeatureCMarshal.h>
#include <zmapWindowTextItem.h>



enum
  {
    PROP_0,

    /* For the floating character */
    PROP_FLOAT_MIN_X,
    PROP_FLOAT_MAX_X,
    PROP_FLOAT_MIN_Y,
    PROP_FLOAT_MAX_Y,
    PROP_FLOAT_AXIS,

    PROP_LAST,
  };



typedef struct
{
  ZMapFeature feature;
  FooCanvasItem *item;
  int translation_start;
  int translation_end;
  char *translation;
  int cds_coord_counter;
  int phase_length;
  GList **full_exons;
  gboolean result;
} ItemShowTranslationTextDataStruct, *ItemShowTranslationTextData;


typedef struct
{
  ZMapSpanStruct exon_span;
  ZMapSpanStruct cds_span;
  ZMapSpanStruct pep_span;
  char *peptide;
  unsigned int phase     ;
  unsigned int end_phase ;
}ZMapFullExonStruct, *ZMapFullExon;







static void zmap_window_sequence_feature_class_init  (ZMapWindowSequenceFeatureClass sequence_class);
static void zmap_window_sequence_feature_init        (ZMapWindowSequenceFeature      sequence);
static void zmap_window_sequence_feature_set_property(GObject               *object,
						      guint                  param_id,
						      const GValue          *value,
						      GParamSpec            *pspec);
static void zmap_window_sequence_feature_get_property(GObject               *object,
						      guint                  param_id,
						      GValue                *value,
						      GParamSpec            *pspec);


static void zmap_window_sequence_feature_update  (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);
static void zmap_window_sequence_feature_realize (FooCanvasItem *item);
static void zmap_window_sequence_feature_draw    (FooCanvasItem  *item,
						  GdkDrawable    *drawable,
						  GdkEventExpose *expose);
static double zmap_window_sequence_feature_point      (FooCanvasItem  *item,
						       double            x,
						       double            y,
						       int               cx,
						       int               cy,
						       FooCanvasItem **actual_item);
#ifdef RDS_DONT_INCLUDE
static void zmap_window_sequence_feature_destroy     (GtkObject *gtkobject);
#endif /* RDS_DONT_INCLUDE */
static gboolean zmap_window_sequence_feature_selected_signal(ZMapWindowSequenceFeature sequence_feature,
							     int text_first_char, int text_final_char);

static void save_scroll_region(FooCanvasItem *item);
static GType float_group_axis_get_type (void);

static gboolean sequence_feature_emit_signal(ZMapWindowSequenceFeature sequence_feature,
					     guint                     signal_id,
					     int first_index, int final_index);


static gboolean feature_exons_world2canvas_text(ZMapFeature    feature,
						gboolean       include_protein,
						FooCanvasItem *item,
						GList        **list_out) ;
static void get_detailed_exons(gpointer exon_data, gpointer user_data) ;
static ZMapFullExon zmapExonCreate(void) ;
static void exonListFree(gpointer data, gpointer user_data_unused) ;
static void zmapExonDestroy(ZMapFullExon exon) ;

static char *zMapFeatureTranscriptTranslation(ZMapFeature feature, int *length) ;
static char *zMapFeatureTranslation(ZMapFeature feature, int *length) ;

static void coordsDNA2Pep(ZMapFeature feature, int *start_inout, int *end_inout) ;



/* globals. */

static FooCanvasItemClass *canvas_parent_class_G ;
static FooCanvasItemClass *group_parent_class_G ;




/* 
 *                        External interface.
 */


GType zMapWindowSequenceFeatureGetType(void)
{
  static GType group_type = 0;

  if (!group_type)
    {
      static const GTypeInfo group_info =
	{
	  sizeof (zmapWindowSequenceFeatureClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_sequence_feature_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowSequenceFeature),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_sequence_feature_init
	};

      group_type = g_type_register_static (ZMAP_TYPE_CANVAS_ITEM,
					   ZMAP_WINDOW_SEQUENCE_FEATURE_NAME,
					   &group_info,
					   0);
  }

  return group_type;
}


/*
 *   highlight/select the DNA/Peptide
 */

/* Highlight sequence corresponding to supplied feature, frame is needed for  */
gboolean zMapWindowSequenceFeatureSelectByFeature(ZMapWindowSequenceFeature sequence_feature,
						  FooCanvasItem *item, ZMapFeature seed_feature)
{
  FooCanvasGroup *sequence_group;
  GList *list;
  gboolean result = TRUE;
  ZMapWindowCanvasItem canvas_item ;
  ZMapFeature feature ;
  gboolean sub_part ;

  sequence_group = FOO_CANVAS_GROUP(sequence_feature) ;
  canvas_item = ZMAP_CANVAS_ITEM(sequence_feature) ;
  feature = canvas_item->feature ;

  /* Is the given item a sub-part of a feature or the whole feature ? */
  sub_part = zMapWindowCanvasItemIsSubPart(item) ;


  if ((list = sequence_group->item_list))
    {
      do
	{
	  ZMapWindowTextItem text_item = (ZMapWindowTextItem)(list->data) ;

	  /* There should only ever be one iteration here! UMMMM, SO WHY THIS COMMENT ?? */
	  switch(seed_feature->type)
	    {
	    case ZMAPSTYLE_MODE_TRANSCRIPT:
	      {
		GList *exon_list = NULL, *exon_list_member ;
		ZMapFullExon current_exon ;
		gboolean deselect, event ;

		deselect = TRUE ;			    /* 1st time deselect any existing highlight. */
		event = FALSE ;

		feature_exons_world2canvas_text(seed_feature, TRUE, NULL, &exon_list) ;


		if (sub_part)
		  {
		    ZMapFeatureSubPartSpan span ;
		    int start, end ;

		    span = zMapWindowCanvasItemIntervalGetData(item) ;

		    current_exon = (ZMapFullExon)(g_list_nth_data(exon_list, span->index - 1)) ;

		    start = current_exon->exon_span.x1 ;
		    end = current_exon->exon_span.x2 ;

		    if (feature->type == ZMAPSTYLE_MODE_PEP_SEQUENCE)
		      coordsDNA2Pep(feature, &start, &end) ;

		    zMapWindowTextItemSelect(text_item,
					     start, end,
					     deselect, deselect) ;
		  }
		else
		  {
		    exon_list_member = g_list_first(exon_list);

		    do
		      {
			int start, end ;

			current_exon = (ZMapFullExon)(exon_list_member->data) ;

			start = current_exon->exon_span.x1 ;
			end = current_exon->exon_span.x2 ;

			if (feature->type == ZMAPSTYLE_MODE_PEP_SEQUENCE)
			  coordsDNA2Pep(feature, &start, &end) ;

			zMapWindowTextItemSelect(text_item,
						 start, end,
						 deselect, deselect) ;

			deselect = FALSE ;
		      }
		    while((exon_list_member = g_list_next(exon_list_member)));

		  }


		/* Tidy up. */
		g_list_foreach(exon_list, exonListFree, NULL) ;
		g_list_free(exon_list) ;

		break ;
	      }

	    default:
	      {
		if (ZMAP_IS_WINDOW_TEXT_ITEM(list->data))
		  {
		    int start, end ;

		    start = seed_feature->x1 ;
		    end = seed_feature->x2 ;

		    if (feature->type == ZMAPSTYLE_MODE_PEP_SEQUENCE)
		      coordsDNA2Pep(feature, &start, &end) ;

		    zMapWindowTextItemSelect(text_item,
					     start, end,
					     TRUE, TRUE) ;
		  }
		break;
	      }
	    }
	}
      while ((list = list->next));
    }

  return result;
}


/* Highlight a region in the sequence object, coord_type specifies whether region_start/end are
 * sequence or peptide coords. */
gboolean zMapWindowSequenceFeatureSelectByRegion(ZMapWindowSequenceFeature sequence_feature,
						 ZMapSequenceType coord_type, int region_start, int region_end)
{
  gboolean result = TRUE ;
  FooCanvasGroup *sequence_group ;
  GList *list ;


  sequence_group = FOO_CANVAS_GROUP(sequence_feature);

  if((list = sequence_group->item_list))
    {
      do
	{
	  /* There should only ever be one iteration here!  IS THIS BECAUSE THERE SHOULD ONLY BE
	     ONE TEXT ITEM ???? */
	  if (ZMAP_IS_WINDOW_TEXT_ITEM(list->data))
	    {
	      ZMapWindowTextItem text_item = (ZMapWindowTextItem)(list->data);

	      zMapWindowTextItemSelect(text_item,
				       region_start, region_end,
				       TRUE, FALSE);
	    }
	}
      while((list = list->next));
    }

  return result ;
}


gboolean zMapWindowSequenceDeSelect(ZMapWindowSequenceFeature sequence_feature)
{
  gboolean result = FALSE;
  FooCanvasGroup *sequence_group ;
  GList *list ;

  sequence_group = FOO_CANVAS_GROUP(sequence_feature);

  if((list = sequence_group->item_list))
    {
      do
	{
	  /* There should only ever be one iteration here!  IS THIS BECAUSE THERE SHOULD ONLY BE
	     ONE TEXT ITEM ???? */
	  if (ZMAP_IS_WINDOW_TEXT_ITEM(list->data))
	    {
	      ZMapWindowTextItem text_item = (ZMapWindowTextItem)(list->data);

	      zMapWindowTextItemDeselect(text_item, TRUE) ;
	    }
	}
      while((list = list->next));
    }

  return result;
}


static gboolean feature_exons_world2canvas_text(ZMapFeature    feature,
						gboolean       include_protein,
						FooCanvasItem *item,
						GList        **list_out)
{
  gboolean result = FALSE;

  if(ZMAPFEATURE_IS_TRANSCRIPT(feature))
    {
      ItemShowTranslationTextDataStruct full_data = { NULL };
      int translation_start, translation_end;

      /* This lot should defo be in a function
       * zMapFeatureTranslationStartCoord(feature);
       */
      if(ZMAPFEATURE_HAS_CDS(feature))
	translation_start = feature->feature.transcript.cds_start;
      else
	translation_start = feature->x1;

      if(feature->feature.transcript.flags.start_not_found)
	translation_start += (feature->feature.transcript.start_phase - 1);

      /* This lot should defo be in a function
       * zMapFeatureTranslationEndCoord(feature);
       */
      if(ZMAPFEATURE_HAS_CDS(feature))
	translation_end = feature->feature.transcript.cds_end;
      else
	translation_end = feature->x2;

      full_data.feature      = feature;
      full_data.item         = item;
      full_data.full_exons   = list_out;
      full_data.result       = result;
      full_data.translation_start = translation_start;
      full_data.translation_end   = translation_end;

      if(include_protein)
	{
	  int real_length;
	  full_data.translation = zMapFeatureTranslation(feature, &real_length);
	}

      zMapFeatureTranscriptExonForeach(feature, get_detailed_exons,
				       &full_data);

      if(include_protein && full_data.translation)
	g_free(full_data.translation);

      /* In case there are no exons */
      result = full_data.result;

      if (result && full_data.phase_length < full_data.cds_coord_counter)
	{
	  ZMapFullExon last_exon;
	  GList *last;

	  if ((last = g_list_last(*(full_data.full_exons))))
	    {
	      char *updated_pep;
	      last_exon = (ZMapFullExon)(last->data);
	      updated_pep = g_strdup_printf("%sX", last_exon->peptide);
	      g_free(last_exon->peptide);
	      last_exon->peptide = updated_pep;
	      last_exon->pep_span.x2 += 1;
	    }
	}
    }

  return result;
}


static void get_detailed_exons(gpointer exon_data, gpointer user_data)
{
  ItemShowTranslationTextData full_data = (ItemShowTranslationTextData)user_data;
  ZMapSpan exon_span = (ZMapSpan)exon_data;
  ZMapFullExon full_exon = NULL;
  FooCanvasItem *item;
  ZMapFeature feature;
  int tr_start, tr_end;

  full_exon = zmapExonCreate();

  full_exon->exon_span = *exon_span; /* struct copy */

  feature  = full_data->feature;
  item     = full_data->item;
  tr_start = full_data->translation_start;
  tr_end   = full_data->translation_end;

  /* chop out the correct bit of sequence */

  if (!(tr_start > exon_span->x2 || tr_end   < exon_span->x1))
    {
      /* translate able exon i.e. not whole UTR exon. */
      int pep_start, pep_length, phase_length;
      int ex1, ex2, coord_length, start, end;
      char *peptide;

      ex1 = exon_span->x1;
      ex2 = exon_span->x2;

      if (ex1 < tr_start)
	{
	  /* exon is part utr */
	  ex1 = tr_start;
	  if(feature->feature.transcript.flags.start_not_found)
	    full_exon->phase = (feature->feature.transcript.start_phase - 1);
	}
      else if (ex1 == tr_start)
	{
	  if(feature->feature.transcript.flags.start_not_found)
	    full_exon->phase = (feature->feature.transcript.start_phase - 1);
	}
      else
	{
	  full_exon->phase = full_data->cds_coord_counter % 3;
	}

      coord_length = (ex2 - ex1 + 1);

      if (ex2 > tr_end)
	{
	  /* exon is part utr */
	  ex2 = tr_end;
	  full_exon->end_phase = -1;
	  coord_length = (ex2 - ex1 + 1);
	}
      else
	{
	  full_exon->end_phase = (full_exon->phase + coord_length) % 3 ;
	}

      start = full_data->cds_coord_counter + 1;
      full_exon->cds_span.x1 = start;

      full_data->cds_coord_counter += coord_length;

      end = full_data->cds_coord_counter;
      full_exon->cds_span.x2 = end;

      /* Keep track of the phase length and save this part of protein */
      phase_length  = coord_length;
      phase_length -= full_exon->end_phase;
      phase_length += full_exon->phase;

      pep_length = (phase_length / 3);
      pep_start  = ((full_data->phase_length) / 3);

      /* _only_ count _full_ codons in this length. */
      full_exon->pep_span.x1   = pep_start + 1;
      full_data->phase_length += (pep_length * 3);
      full_exon->pep_span.x2   = (int)(full_data->phase_length / 3);

      if(full_data->translation)
	{
	  peptide = full_data->translation + pep_start;

	  full_exon->peptide = g_strndup(peptide, pep_length);
	}
    }
  else
    {
      full_exon->peptide = NULL;
    }

  *(full_data->full_exons) = g_list_append(*(full_data->full_exons), full_exon) ;

  /* what more can we do? */
  full_data->result = TRUE;

  return ;
}


static char *zMapFeatureTranscriptTranslation(ZMapFeature feature, int *length)
{
  char *pep_str = NULL ;
  ZMapFeatureContext context ;
  ZMapPeptide peptide ;
  char *dna_str, *name, *free_me ;

  context = (ZMapFeatureContext)(zMapFeatureGetParentGroup((ZMapFeatureAny)feature,
							   ZMAPFEATURE_STRUCT_CONTEXT));

  if ((dna_str = zMapFeatureGetTranscriptDNA(feature, TRUE, feature->feature.transcript.flags.cds)))
    {
      free_me = dna_str;					    /* as we potentially move ptr. */
      name    = (char *)g_quark_to_string(feature->original_id);

      if(feature->feature.transcript.flags.start_not_found)
	dna_str += (feature->feature.transcript.start_phase - 1);

      peptide = zMapPeptideCreate(name, NULL, dna_str, NULL, TRUE);

      if(length)
	{
	  *length = zMapPeptideLength(peptide);
	  if (zMapPeptideHasStopCodon(peptide))
	    *length = *length - 1;
	}

      pep_str = zMapPeptideSequence(peptide);
      pep_str = g_strdup(pep_str);

      zMapPeptideDestroy(peptide);
      g_free(free_me);
    }

  return pep_str ;
}

static char *zMapFeatureTranslation(ZMapFeature feature, int *length)
{
  char *seq;

  if(feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      seq = zMapFeatureTranscriptTranslation(feature, length);
    }
  else
    {
      GArray *rubbish;
      int i, l;
      char c = '.';

      l = feature->x2 - feature->x1 + 1;

      rubbish = g_array_sized_new(TRUE, TRUE, sizeof(char), l);

      for(i = 0; i < l; i++)
	{
	  g_array_append_val(rubbish, c);
	}

      seq = rubbish->data;

      if(length)
	*length = l;

      g_array_free(rubbish, FALSE);
    }

  return seq;
}


static ZMapFullExon zmapExonCreate(void)
{
  ZMapFullExon exon = NULL ;

  exon = g_new0(ZMapFullExonStruct, 1) ;
  exon->peptide = NULL ;

  return exon ;
}


/* A GFunc() callback to free each exon element of the exons list. */
static void exonListFree(gpointer data, gpointer user_data_unused)
{
  ZMapFullExon exon = (ZMapFullExon)data ;

  zmapExonDestroy(exon) ;

  return ;
}




static void zmapExonDestroy(ZMapFullExon exon)
{
  zMapAssert(exon);

  if (exon->peptide)
    g_free(exon->peptide) ;

  g_free(exon) ;

  return ;
}



static gboolean sequence_feature_selection_proxy_cb(ZMapWindowTextItem text,
						    double cellx1, double celly1,
						    double cellx2, double celly2,
						    int index1, int index2,
						    gpointer user_data)
{
  ZMapWindowSequenceFeature sequence_feature;
  ZMapWindowCanvasItem canvas_item;
  ZMapFeature feature;
  FooCanvasItem *item;
  double x1, y1, x2, y2;
  double wx1, wy1, wx2, wy2;
  int first_char_index = 0;
  int origin_index, current_index;
  guint signal_id;

  sequence_feature = ZMAP_WINDOW_SEQUENCE_FEATURE(user_data);
  canvas_item = ZMAP_CANVAS_ITEM(sequence_feature);
  item = FOO_CANVAS_ITEM(text);

  feature = canvas_item->feature;

  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
  wx1 = x1; wx1 = x2; wy1 = y1; wy2 = y2;
  foo_canvas_item_i2w(item, &wx1, &wy1);
  foo_canvas_item_i2w(item, &wx2, &wy2);

  if (feature->type == ZMAPSTYLE_MODE_PEP_SEQUENCE)
    {
      first_char_index = (int)(wy1 - feature->x1);
      first_char_index = (int)(first_char_index / 3) + 1;
    }
  else
    {
      first_char_index = (int)(wy1 - feature->x1 + 1);
    }

  origin_index  = index1 + first_char_index;
  current_index = index2 + first_char_index;

  /* this is needed to fix up what is probably unavoidable from the text item... [-ve coords] */
  if (origin_index < 1)
    origin_index = 1;

  /* this is needed to fix up what is probably unavoidable from the text item... [too long coords] */
  if(current_index > feature->feature.sequence.length)
    current_index = feature->feature.sequence.length;

  signal_id = ZMAP_WINDOW_SEQUENCE_FEATURE_GET_CLASS(sequence_feature)->signals[SEQUENCE_SELECTED_SIGNAL];

  sequence_feature_emit_signal(sequence_feature, signal_id,
			       origin_index, current_index);

  return TRUE;
}

static FooCanvasItem *zmap_window_sequence_feature_add_interval(ZMapWindowCanvasItem   sequence,
								ZMapFeatureSubPartSpan unused,
								double top,  double bottom,
								double left, double right)
{
  FooCanvasGroup *group;
  FooCanvasItem *item = NULL;
  ZMapFeatureTypeStyle style;
  ZMapFeature feature;
  char *font_name = "monospace" ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("In %s %s()\n", __FILE__, __PRETTY_FUNCTION__) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  group = FOO_CANVAS_GROUP(sequence);

  if (!(group->item_list))
    {
      GdkColor *draw = NULL, *fill = NULL, *border = NULL ;


      feature = sequence->feature ;
      style = (ZMAP_CANVAS_ITEM_GET_CLASS(sequence)->get_style)(sequence) ;

      item = foo_canvas_item_new(group, ZMAP_TYPE_WINDOW_TEXT_ITEM,
				 "x",          left,
				 "y",          top,
				 "anchor",     GTK_ANCHOR_NW,
				 "font"    ,   font_name,
				 "full-width", 30.0,
				 "wrap-mode",  PANGO_WRAP_CHAR,
				 NULL);

      /* Selection highlighting takes its colour from the style "select" fill colour. */
      zMapStyleGetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_SELECTED,
			  &fill, &draw, &border) ;

      if (fill)
	g_object_set(G_OBJECT(item),
		     "select-color-gdk", fill,
		     NULL) ;

      g_signal_connect(G_OBJECT(item), "text-selected",
		       G_CALLBACK(sequence_feature_selection_proxy_cb),
		       sequence);
    }

  return item;
}

static void zmap_window_sequence_feature_bounds (FooCanvasItem *item,
						 double *x1, double *y1,
						 double *x2, double *y2)
{

  if (canvas_parent_class_G->bounds)
    (canvas_parent_class_G->bounds)(item, x1, y1, x2, y2);

  return ;
}


static void zmap_window_sequence_feature_class_init  (ZMapWindowSequenceFeatureClass sequence_class)
{
  ZMapWindowCanvasItemClass canvas_class;
  FooCanvasItemClass *item_class;
  GObjectClass *gobject_class;
  GtkObjectClass *gtk_object_class;

  gobject_class = (GObjectClass *) sequence_class;
  gtk_object_class = (GtkObjectClass *) sequence_class;
  canvas_class  = (ZMapWindowCanvasItemClass)sequence_class;
  item_class    = (FooCanvasItemClass *)sequence_class;

  canvas_parent_class_G = (FooCanvasItemClass *)g_type_class_peek_parent(sequence_class);

  gobject_class->set_property = zmap_window_sequence_feature_set_property;
  gobject_class->get_property = zmap_window_sequence_feature_get_property;

  group_parent_class_G = g_type_class_peek (FOO_TYPE_CANVAS_GROUP);

  /* Properties for the floating character */
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_MIN_X,
				  g_param_spec_double("min-x",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						       G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_MAX_X,
				  g_param_spec_double("max-x",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						      G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_MIN_Y,
				  g_param_spec_double("min-y",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						      G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_MAX_Y,
				  g_param_spec_double("max-y",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						      G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_AXIS,
				  g_param_spec_enum ("float-axis", NULL, NULL,
						     float_group_axis_get_type(),
						     ZMAP_FLOAT_AXIS_Y,
						     G_PARAM_READWRITE));

  /* floating... */
  item_class->draw    = zmap_window_sequence_feature_draw;
  item_class->update  = zmap_window_sequence_feature_update;
  item_class->realize = zmap_window_sequence_feature_realize;
  item_class->point   = zmap_window_sequence_feature_point;

  canvas_class->add_interval = zmap_window_sequence_feature_add_interval;

  item_class->bounds  = zmap_window_sequence_feature_bounds;

#ifdef RDS_DONT_INCLUDE
  canvas_class->set_colour   = zmap_window_sequence_feature_set_colour;
#endif


  sequence_class->signals[SEQUENCE_SELECTED_SIGNAL] =
    g_signal_new("sequence-selected",
		 G_TYPE_FROM_CLASS(sequence_class),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(zmapWindowSequenceFeatureClass, selected_signal),
		 NULL, NULL,
		 zmapWindowSequenceFeatureCMarshal_BOOL__INT_INT,
		 G_TYPE_BOOLEAN, 2,
		 G_TYPE_INT, G_TYPE_INT);

  sequence_class->selected_signal = zmap_window_sequence_feature_selected_signal;

#ifdef RDS_DONT_INCLUDE
  gtk_object_class->destroy = zmap_window_sequence_feature_destroy;
#endif  /* RDS_DONT_INCLUDE */

  return ;
}

static void zmap_window_sequence_feature_init(ZMapWindowSequenceFeature  sequence)
{
  ZMapWindowCanvasItem canvas_item;
  FooCanvasItem *item;

  canvas_item = ZMAP_CANVAS_ITEM(sequence);
  canvas_item->auto_resize_background = 1;

  /* initialise the floating stuff */
  sequence->float_settings.zoom_x = 0.0;
  sequence->float_settings.zoom_y = 0.0;
  sequence->float_settings.scr_x1 = 0.0;
  sequence->float_settings.scr_y1 = 0.0;
  sequence->float_settings.scr_x2 = 0.0;
  sequence->float_settings.scr_y2 = 0.0;

  item = FOO_CANVAS_ITEM(sequence);
  if(item->canvas)
    {
      sequence->float_settings.zoom_x = item->canvas->pixels_per_unit_x;
      sequence->float_settings.zoom_y = item->canvas->pixels_per_unit_y;
    }

  /* y axis only! */
  sequence->float_flags.float_axis = ZMAP_FLOAT_AXIS_Y;

  return ;
}

static void zmap_window_sequence_feature_set_property(GObject               *gobject,
						      guint                  param_id,
						      const GValue          *value,
						      GParamSpec            *pspec)
{
  ZMapWindowSequenceFeature seq_feature;
  double d = 0.0;		/* dummy value for i2w() */

  seq_feature = ZMAP_WINDOW_SEQUENCE_FEATURE(gobject);

  switch (param_id)
    {
    case PROP_FLOAT_MIN_X:
      seq_feature->float_settings.scr_x1 = g_value_get_double (value);
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(gobject), &(seq_feature->float_settings.scr_x1), &d);
      seq_feature->float_flags.min_x_set = TRUE;
      break;
    case PROP_FLOAT_MAX_X:
      seq_feature->float_settings.scr_x2 = g_value_get_double (value);
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(gobject), &(seq_feature->float_settings.scr_x2), &d);
      seq_feature->float_flags.max_x_set = TRUE;
      break;
    case PROP_FLOAT_MIN_Y:
      seq_feature->float_settings.scr_y1 = g_value_get_double (value);
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(gobject), &d, &(seq_feature->float_settings.scr_y1));
      seq_feature->float_flags.min_y_set = TRUE;
      break;
    case PROP_FLOAT_MAX_Y:
      seq_feature->float_settings.scr_y2 = g_value_get_double (value);
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(gobject), &d, &(seq_feature->float_settings.scr_y2));
      seq_feature->float_flags.max_y_set = TRUE;
      break;
    case PROP_FLOAT_AXIS:
      {
	int axis = g_value_get_enum(value);
	seq_feature->float_flags.float_axis = axis;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
      break;
    }


  return ;
}

static void zmap_window_sequence_feature_get_property(GObject               *object,
						      guint                  param_id,
						      GValue                *value,
						      GParamSpec            *pspec)
{
  switch(param_id)
    {
    case PROP_FLOAT_MIN_X:
      break;
    case PROP_FLOAT_MAX_X:
      break;
    case PROP_FLOAT_MIN_Y:
      break;
    case PROP_FLOAT_MAX_Y:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}


static void invoke_zeroing_update(gpointer item_data, gpointer unused_data)
{
  FooCanvasItem *item = (FooCanvasItem *)item_data;
  FooCanvasGroup *group;

  if(FOO_IS_CANVAS_GROUP(item))
    {
      group = (FooCanvasGroup *)item;
      g_list_foreach(group->item_list, invoke_zeroing_update, unused_data);
    }

  item->x1 = item->y1 = 0.0;
  item->x2 = item->y2 = 0.0;

  return ;
}

static void zmap_window_sequence_feature_update(FooCanvasItem *item,
						double i2w_dx, double i2w_dy,
						int flags)
{
  ZMapWindowSequenceFeature seq_feature;
  FooCanvasItem *parent;
  FooCanvasGroup *group;
  ZMapWindowCanvasItem canvas_item ;
  ZMapFeature feature ;
  double x1, x2, y1, y2;
  double xpos, ypos;
  double scr_x1, scr_y1, scr_x2, scr_y2;
  gboolean force_intersect = FALSE;

  seq_feature = ZMAP_WINDOW_SEQUENCE_FEATURE(item);
  group = FOO_CANVAS_GROUP (item);
  canvas_item = ZMAP_CANVAS_ITEM(seq_feature) ;
  feature = canvas_item->feature ;

  xpos  = group->xpos;
  ypos  = group->ypos;

  /* First we move the group to the correct position. */

  /* run a "update" loop to zero all the items in this group */
  invoke_zeroing_update(item, NULL);

  /* Now we can safely get the bounds of our parent group */
  /* actually we want the ContainerGroup because they don't
   * work in the same way as canvasitems (mistake) */
  if(ZMAP_IS_CONTAINER_FEATURES(item->parent))
    {
      parent = (FooCanvasItem *)zmapWindowContainerChildGetParent(item->parent);
    }
  else
    parent = item->parent;

  foo_canvas_item_get_bounds(parent, &x1, &y1, &x2, &y2);

  /* If the group x,y is outside the scroll region, move it back in! */
  foo_canvas_get_scroll_region(item->canvas, &scr_x1, &scr_y1, &scr_x2, &scr_y2);

  foo_canvas_item_w2i(item, &scr_x1, &scr_y1);
  foo_canvas_item_w2i(item, &scr_x2, &scr_y2);

  if (seq_feature->float_flags.float_axis & ZMAP_FLOAT_AXIS_X)
    {
      xpos = x1;
      if (scr_x1 >= x1 && scr_x1 < x2)
	xpos = scr_x1;
    }

  if (seq_feature->float_flags.float_axis & ZMAP_FLOAT_AXIS_Y)
    {
      ypos = y1;

      if(scr_y1 >= y1 && scr_y1 < y2)
	ypos = scr_y1;
    }

  /* round down to whole bases... We need to do this in a few places! */
  xpos = (double)((int)xpos);
  ypos = (double)((int)ypos);

  group->xpos = xpos;
  group->ypos = ypos;


  if(force_intersect && (item->object.flags & FOO_CANVAS_ITEM_VISIBLE))
    {
      int cx1, cx2, cy1, cy2;

      foo_canvas_w2c(item->canvas,
		     seq_feature->float_settings.scr_x1,
		     seq_feature->float_settings.scr_y1,
		     &cx1, &cy1);
      foo_canvas_w2c(item->canvas,
		     seq_feature->float_settings.scr_x2,
		     seq_feature->float_settings.scr_y2,
		     &cx2, &cy2);

      /* These must be set in order to make the seq_feature intersect with any
       * rectangle within the whole of the scroll region */
      if(seq_feature->float_flags.float_axis & ZMAP_FLOAT_AXIS_X)
	{
	  item->x1 = cx1; //seq_feature->scr_x1;
	  item->x2 = cx2; //seq_feature->scr_x2;
	}
      if(seq_feature->float_flags.float_axis & ZMAP_FLOAT_AXIS_Y)
	{
	  item->y1 = cy1; //seq_feature->scr_y1;
	  item->y2 = cy2; //seq_feature->scr_y2;
	}
    }

  if(canvas_parent_class_G->update)
    (* canvas_parent_class_G->update)(item, i2w_dx, i2w_dy, flags);


  return ;
}

static double zmap_window_sequence_feature_point(FooCanvasItem  *item,
						 double            x,
						 double            y,
						 int               cx,
						 int               cy,
						 FooCanvasItem **actual_item)
{
  double dist = 1.0;

  if(group_parent_class_G->point)
    dist = (group_parent_class_G->point)(item, x, y, cx, cy, actual_item);

  return dist;
}

static void zmap_window_sequence_feature_realize (FooCanvasItem *item)
{
  ZMapWindowSequenceFeature seq_feature;

  seq_feature = ZMAP_WINDOW_SEQUENCE_FEATURE(item);

  if(item->canvas)
    {
      seq_feature->float_settings.zoom_x = item->canvas->pixels_per_unit_x;
      seq_feature->float_settings.zoom_y = item->canvas->pixels_per_unit_y;
      save_scroll_region(item);
    }

  if(canvas_parent_class_G->realize)
    (* canvas_parent_class_G->realize)(item);
}


static void zmap_window_sequence_feature_draw(FooCanvasItem  *item,
					      GdkDrawable    *drawable,
					      GdkEventExpose *expose)
{
  ZMapWindowSequenceFeature seq_feature;
  FooCanvasGroup *group;

  group = FOO_CANVAS_GROUP (item);
  seq_feature = ZMAP_WINDOW_SEQUENCE_FEATURE(item);

  /* parent->draw? */
  if(canvas_parent_class_G->draw)
    (* canvas_parent_class_G->draw)(item, drawable, expose);

  return ;
}


#ifdef RDS_DONT_INCLUDE
static void zmap_window_sequence_feature_destroy     (GtkObject *gtkobject)
{
  if(GTK_OBJECT_CLASS(canvas_parent_class_G)->destroy)
    (* GTK_OBJECT_CLASS(canvas_parent_class_G)->destroy)(gtkobject);

  return ;
}
#endif /* RDS_DONT_INCLUDE */


static gboolean zmap_window_sequence_feature_selected_signal(ZMapWindowSequenceFeature sequence_feature,
							     int text_first_char, int text_final_char)
{
#ifdef RDS_DONT_INCLUDE
  g_warning("%s sequence indices %d to %d", __PRETTY_FUNCTION__, text_first_char, text_final_char);
#endif /* RDS_DONT_INCLUDE */

  return FALSE;
}


static void save_scroll_region(FooCanvasItem *item)
{
  ZMapWindowSequenceFeature seq_feature;
  double x1, y1, x2, y2;
  double *x1_ptr, *y1_ptr, *x2_ptr, *y2_ptr;

  seq_feature = ZMAP_WINDOW_SEQUENCE_FEATURE(item);

  foo_canvas_get_scroll_region(item->canvas, &x1, &y1, &x2, &y2);

  x1_ptr = &(seq_feature->float_settings.scr_x1);
  y1_ptr = &(seq_feature->float_settings.scr_y1);
  x2_ptr = &(seq_feature->float_settings.scr_x2);
  y2_ptr = &(seq_feature->float_settings.scr_y2);

  if(item->canvas->pixels_per_unit_x <= seq_feature->float_settings.zoom_x)
    {
      if(!seq_feature->float_flags.min_x_set)
	seq_feature->float_settings.scr_x1 = x1;
      else
	x1_ptr = &x1;
      if(!seq_feature->float_flags.max_x_set)
	seq_feature->float_settings.scr_x2 = x2;
      else
	x2_ptr = &x2;
    }

  if(item->canvas->pixels_per_unit_y <= seq_feature->float_settings.zoom_y)
    {
      if(!seq_feature->float_flags.min_y_set)
	seq_feature->float_settings.scr_y1 = y1;
      else
	y1_ptr = &y1;
      if(!seq_feature->float_flags.max_y_set)
	seq_feature->float_settings.scr_y2 = y2;
      else
	y2_ptr = &y2;
    }

  foo_canvas_item_w2i(item, x1_ptr, y1_ptr);
  foo_canvas_item_w2i(item, x2_ptr, y2_ptr);

  return ;
}


static GType float_group_axis_get_type (void)
{
  static GType etype = 0 ;

  if (etype == 0)
    {
      static const GEnumValue values[] =
	{
	  { ZMAP_FLOAT_AXIS_X, "ZMAP_FLOAT_AXIS_X", "x-axis" },
	  { ZMAP_FLOAT_AXIS_Y, "ZMAP_FLOAT_AXIS_Y", "y-axis" },
	  { 0, NULL, NULL }
	};

      etype = g_enum_register_static ("ZMapWindowFloatGroupAxis", values);
    }

  return etype ;
}


static gboolean sequence_feature_emit_signal(ZMapWindowSequenceFeature sequence_feature,
					     guint                     signal_id,
					     int first_index, int final_index)
{
#define SIGNAL_N_PARAMS 3
  GValue instance_params[SIGNAL_N_PARAMS] = {{0}}, sig_return = {0};
  GQuark detail = 0;
  int i;
  gboolean result = FALSE;

  g_value_init(instance_params, ZMAP_TYPE_WINDOW_SEQUENCE_FEATURE);
  g_value_set_object(instance_params, sequence_feature);

  g_value_init(instance_params + 1, G_TYPE_INT);
  g_value_set_int(instance_params + 1, first_index);

  g_value_init(instance_params + 2, G_TYPE_INT);
  g_value_set_int(instance_params + 2, final_index);

  g_value_init(&sig_return, G_TYPE_BOOLEAN);
  g_value_set_boolean(&sig_return, result);

  g_object_ref(G_OBJECT(sequence_feature));

  g_signal_emitv(instance_params, signal_id, detail, &sig_return);

  for(i = 0; i < SIGNAL_N_PARAMS; i++)
    {
      g_value_unset(instance_params + i);
    }

  g_object_unref(G_OBJECT(sequence_feature));

#undef SIGNAL_N_PARAMS

  return TRUE;
}


/* Convert coords from dna to pep, conversion is done on complete codons only. */
static void coordsDNA2Pep(ZMapFeature feature, int *start_inout, int *end_inout)
{
  ZMapFrame frame = feature->feature.sequence.frame ;

  /* If no frame we just default to frame 1 which seems sensible. */
  if (!frame)
    frame = ZMAPFRAME_0 ;

  zMapSequenceDNA2Pep(start_inout, end_inout, frame) ;

  return ;
}
