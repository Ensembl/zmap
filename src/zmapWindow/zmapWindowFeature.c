/*  File: zmapWindowFeature.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Functions that manipulate displayed features, they
 *              encapsulate handling of the feature context, the
 *              displayed canvas items and the feature->item hash.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Feb 15 08:12 2011 (edgrif)
 * Created: Mon Jan  9 10:25:40 2006 (edgrif)
 * CVS info:   $Id: zmapWindowFeature.c,v 1.205 2011-02-15 11:49:40 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <math.h>

#include <ZMap/zmap.h>
#include <ZMap/zmapFASTA.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapPeptide.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowItemFactory.h>
#include <zmapWindowItemTextFillColumn.h>
#include <zmapWindowFeatures.h>
#include <libpfetch/libpfetch.h>
#include <zmapWindowContainerFeatureSet_I.h>
#include <zmapWindowFeatures.h>


#include <ZMap/zmapThreads.h> // for ForkLock functions

#define PFETCH_READ_SIZE 80	/* about a line */
#define PFETCH_FAILED_PREFIX "PFetch failed:"
#define PFETCH_TITLE_FORMAT "ZMap - pfetch \"%s\""



enum
  {
    ITEM_MENU_INVALID,
    ITEM_MENU_LIST_NAMED_FEATURES,
    ITEM_MENU_LIST_ALL_FEATURES,
    ITEM_MENU_MARK_ITEM,
    ITEM_MENU_SEARCH,
    ITEM_MENU_FEATURE_DETAILS,
    ITEM_MENU_PFETCH,
    ITEM_MENU_SEQUENCE_SEARCH_DNA,
    ITEM_MENU_SEQUENCE_SEARCH_PEPTIDE,
    ITEM_MENU_SHOW_URL_IN_BROWSER,
    ITEM_MENU_SHOW_TRANSLATION,
    ITEM_MENU_TOGGLE_MARK,

    ITEM_MENU_SHOW_EVIDENCE,
    ITEM_MENU_ADD_EVIDENCE,
    ITEM_MENU_SHOW_TRANSCRIPT,

    ITEM_MENU_ITEMS
  };

typedef struct
{
  GtkWidget *dialog;
  GtkTextBuffer *text_buffer;
  char *title;
  gulong widget_destroy_handler_id;
  PFetchHandle pfetch;
  gboolean got_response;
}PFetchDataStruct, *PFetchData;


typedef struct
{
  ZMapWindow window;
  FooCanvasGroup *column;
}processFeatureDataStruct, *processFeatureData;

typedef struct
{
  gboolean exists;
}BlockHasDNAStruct, *BlockHasDNA;


typedef struct
{
  FooCanvasGroup *new_parent ;
  double x_origin, y_origin ;
} ReparentDataStruct, *ReparentData ;

/* This struct needs rationalising. */
typedef struct
{
  FooCanvasItem   *dna_item;
  PangoLayoutIter *iterator;
  GdkEvent        *event;
  int              origin_index;
  double           origin_x, origin_y;
  gboolean         selected;
  gboolean         result;
  FooCanvasPoints *points;
  double           index_bounds[ITEMTEXT_CHAR_BOUND_COUNT];
}DNAItemEventStruct, *DNAItemEvent;


FooCanvasItem *addNewCanvasItem(ZMapWindow window, FooCanvasGroup *feature_group, ZMapFeature feature,
				gboolean bump_col) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void makeTextItemMenu(GdkEventButton *button_event, ZMapWindow window, FooCanvasItem *item);
static ZMapGUIMenuItem makeMenuTextSelectOps(int *start_index_inout,
                                             ZMapGUIMenuItemCallbackFunc callback_func,
                                             gpointer callback_data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static ZMapGUIMenuItem makeMenuURL(int *start_index_inout,
				   ZMapGUIMenuItemCallbackFunc callback_func,
				   gpointer callback_data) ;
static ZMapGUIMenuItem makeMenuFeatureOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data) ;
static ZMapGUIMenuItem makeMenuShowTranslation(int *start_index_inout,
					       ZMapGUIMenuItemCallbackFunc callback_func,
					       gpointer callback_data);
static ZMapGUIMenuItem makeMenuPfetchOps(int *start_index_inout,
					 ZMapGUIMenuItemCallbackFunc callback_func,
					 gpointer callback_data) ;
static ZMapGUIMenuItem makeMenuGeneralOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data) ;
static void itemMenuCB(int menu_item_id, gpointer callback_data) ;

static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean handleButton(GdkEventButton *but_event, ZMapWindow window, FooCanvasItem *item, ZMapFeature feature) ;

static gboolean canvasItemDestroyCB(FooCanvasItem *item, gpointer data) ;

static void pfetchEntry(ZMapWindow window, char *sequence_name) ;
static void handle_dialog_close(GtkWidget *dialog, gpointer user_data);



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static ZMapFeatureContextExecuteStatus oneBlockHasDNA(GQuark key,
                                                      gpointer data,
                                                      gpointer user_data,
                                                      char **error_out);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static gboolean factoryTopItemCreated(FooCanvasItem *top_item,
                                      ZMapFeatureContext context,
                                      ZMapFeatureAlignment align,
                                      ZMapFeatureBlock block,
                                      ZMapFeatureSet set,
                                      ZMapFeature feature,
                                      gpointer handler_data);
static gboolean factoryFeatureSizeReq(ZMapFeature feature,
                                      double *limits_array,
                                      double *points_array_inout,
                                      gpointer handler_data);

static gboolean sequenceSelectionCB(FooCanvasItem *item, int start, int end, gpointer user_data) ;




/* Local globals for debugging. */
static gboolean mouse_debug_G = FALSE ;



/* NOTE that we make some assumptions in this code including:
 *
 * - the caller has found the correct context, alignment, block and set
 *   to put the feature in. Probably we will have to write some helper
 *   helper functions to do this if they have not already been written.
 *   There will need to map stuff like  "block spec" -> ZMapFeatureBlock etc.
 *
 *
 *  */

/*
 * NOTE TO ROY: I've written the functions so that when you get the xml from James, you can
 * get the align/block/set information and then
 * use the hash calls to get the foocanvasgroup that is the column you want to add/modify the feature
 * in. The you can just call these routines with the group and the feature.
 *
 * This way if there are errors in the xml and you can't find the right align/block/set then
 * you can easily report back to lace what the error was....
 *  */





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
GHashTable *zMapWindowFeatureAllStyles(ZMapWindow window)
{
  zMapAssert(window && window->feature_context);

  return window->feature_context->styles ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





#if MH17_NOT_CALLED
/* Add a new feature to the feature_set given by the set canvas item.
 *
 * Returns the new canvas feature item or NULL if there is some problem, e.g. the feature already
 * exists in the feature_set.
 *  */
FooCanvasItem *zMapWindowFeatureAdd(ZMapWindow window,
				    FooCanvasGroup *feature_group, ZMapFeature feature)
{
  FooCanvasItem *new_feature = NULL ;
  ZMapFeatureSet feature_set ;

  zMapAssert(window && feature_group && feature && zMapFeatureIsValid((ZMapFeatureAny)feature)) ;


  feature_set = zmapWindowContainerGetFeatureSet((ZMapWindowContainerGroup)feature_group);
  zMapAssert(feature_set) ;


  /* Check the feature does not already exist in the feature_set. */
  if (!zMapFeatureSetFindFeature(feature_set, feature))
    {
      /* Add it to the feature set. */
      if (zMapFeatureSetAddFeature(feature_set, feature))
	{
	  new_feature = addNewCanvasItem(window, feature_group, feature, TRUE) ;
	}
    }

  return new_feature ;
}

#endif

#if MH17_NOT_CALLED
/* N.B. This function creates TWO columns.  One Forward (returned) and One Reverse.
 * To get hold of the reverse one you'll need to use a FToI call.
 * Also to note. If the feature_set_name is NOT in the window feature_set_names list
 * the columns will NOT be created.
 */
FooCanvasItem *zMapWindowFeatureSetAdd(ZMapWindow window,
                                       FooCanvasGroup *block_group,
                                       char *feature_set_name)
{
  FooCanvasItem *new_canvas_item = NULL ;
  FooCanvasGroup *new_forward_set = NULL;
  FooCanvasGroup *new_reverse_set;
  ZMapFeatureSet     feature_set;
  ZMapFeatureBlock feature_block;
  ZMapFeatureContext     context;
  ZMapFeatureTypeStyle     style;
  ZMapWindowContainerStrand forward_strand, reverse_strand;
  ZMapWindowContainerFeatures forward_features, reverse_features;
  GQuark style_id,feature_set_id;

  feature_block = zmapWindowContainerGetFeatureBlock(ZMAP_CONTAINER_GROUP( block_group ));
  zMapAssert(feature_block);

  context = (ZMapFeatureContext)(zMapFeatureGetParentGroup((ZMapFeatureAny)feature_block, ZMAPFEATURE_STRUCT_CONTEXT));
  zMapAssert(context);

  feature_set_id = zMapFeatureSetCreateID(feature_set_name);
  style_id = zMapStyleCreateID(feature_set_name);

  /* Make sure it's somewhere in our list of feature set names.... columns to draw. */
  if(g_list_find(window->feature_set_names, GUINT_TO_POINTER(feature_set_id)) &&
     (style = zMapFindStyle(window->context_map->styles, style_id)))
    {
      /* Check feature set does not already exist. */
      if(!(feature_set = zMapFeatureBlockGetSetByID(feature_block, feature_set_id)))
        {
          /* Create the new feature set... */
          if((feature_set = zMapFeatureSetCreate(feature_set_name, NULL)))
            {
              /* Setup the style */
              zMapFeatureSetStyle(feature_set, style);
              /* Add it to the block */
              zMapFeatureBlockAddFeatureSet(feature_block, feature_set);
            }
          else
            zMapAssertNotReached();
        }

      /* Get the strand groups */
      forward_strand   = zmapWindowContainerBlockGetContainerStrand(ZMAP_CONTAINER_BLOCK( block_group ), ZMAPSTRAND_FORWARD);
      forward_features = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)forward_strand);
      reverse_strand   = zmapWindowContainerBlockGetContainerStrand(ZMAP_CONTAINER_BLOCK( block_group ), ZMAPSTRAND_REVERSE);
      reverse_features = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)reverse_strand);

      /* Create the columns */
      zmapWindowCreateSetColumns(window,
                                 forward_features,
                                 reverse_features,
                                 feature_block,
                                 feature_set,
				 window->context_map->styles,
                                 ZMAPFRAME_NONE,
                                 &new_forward_set,
                                 &new_reverse_set,
				 NULL);

      zmapWindowColOrderColumns(window);

    }

  if (new_forward_set != NULL)
    new_canvas_item = FOO_CANVAS_ITEM(new_forward_set) ;

  return new_canvas_item ;
}
#endif


/* THERE IS A PROBLEM HERE IN THAT WE REMOVE THE EXISTING FOOCANVAS ITEM FOR THE FEATURE
 * AND DRAW A NEW ONE, THIS WILL INVALIDATE ANY CODE THAT IS CACHING THE ITEM.
 *
 * THE FIX IS FOR ALL OUR CODE TO USE THE HASH TO GET TO FEATURE ITEMS AND NOT CACHE THEM....*/

/* Replace an existing feature in the feature context, this is the only way to modify
 * a feature currently.
 *
 * Returns FALSE if the feature does not exist. */
FooCanvasItem *zMapWindowFeatureReplace(ZMapWindow zmap_window,
					FooCanvasItem *curr_feature_item,
					ZMapFeature new_feature,
					gboolean destroy_orig_feature)
{
  FooCanvasItem *replaced_feature = NULL ;
  ZMapFeatureSet feature_set ;
  ZMapFeature curr_feature ;

  zMapAssert(zmap_window && curr_feature_item
	     && new_feature && zMapFeatureIsValid((ZMapFeatureAny)new_feature)) ;

  curr_feature = zmapWindowItemGetFeature(curr_feature_item);
  zMapAssert(curr_feature && zMapFeatureIsValid((ZMapFeatureAny)curr_feature)) ;

  feature_set = (ZMapFeatureSet)(curr_feature->parent) ;

  /* Feature must exist in current set to be replaced...belt and braces....??? */
  if (zMapFeatureSetFindFeature(feature_set, curr_feature))
    {
      FooCanvasGroup *set_group ;

      set_group = (FooCanvasGroup *)zmapWindowContainerCanvasItemGetContainer(curr_feature_item) ;
      zMapAssert(set_group) ;

      /* Remove it completely. */
      if (zMapWindowFeatureRemove(zmap_window, curr_feature_item, destroy_orig_feature))
	{
	  replaced_feature = addNewCanvasItem(zmap_window, set_group, new_feature, FALSE) ;
	}
      else
      {
            printf("Could not remove feature\n");
      }
    }
    else
    {
            printf("Could not find feature\n");
    }

  return replaced_feature ;
}


/* Remove an existing feature from the displayed feature context.
 *
 * Returns FALSE if the feature does not exist. */
gboolean zMapWindowFeatureRemove(ZMapWindow zmap_window, FooCanvasItem *feature_item, gboolean destroy_feature)
{
  ZMapWindowContainerFeatureSet container_set;
  gboolean result = FALSE ;
  ZMapFeature feature ;
  ZMapFeatureSet feature_set ;

  feature = zmapWindowItemGetFeature(feature_item);
  zMapAssert(feature && zMapFeatureIsValid((ZMapFeatureAny)feature)) ;
  feature_set = (ZMapFeatureSet)(feature->parent) ;

  container_set = (ZMapWindowContainerFeatureSet)zmapWindowContainerCanvasItemGetContainer(feature_item) ;


  /* Need to delete the feature from the feature set and from the hash and destroy the
   * canvas item....NOTE this is very order dependent. */

  /* I'm still not sure this is all correct.
   * canvasItemDestroyCB has a FToIRemove!
   */

  /* Firstly remove from the FToI hash... */
  if (zmapWindowFToIRemoveFeature(zmap_window->context_to_item,
				  container_set->strand,
				  container_set->frame, feature))
    {
      /* check the feature is in featureset. */
      if(zMapFeatureSetFindFeature(feature_set, feature))
        {
          double x1, x2, y1, y2;

          if (zmapWindowMarkIsSet(zmap_window->mark)
	      && feature_item == zmapWindowMarkGetItem(zmap_window->mark)
	      && zmapWindowMarkGetWorldRange(zmap_window->mark, &x1, &y1, &x2, &y2))
            {
              zmapWindowMarkSetWorldRange(zmap_window->mark, x1, y1, x2, y2);
            }

#ifdef RT_63281
	  /* I thought this call would be needed, but it turns out that the bump code
	   * needed to not removeGapsCB and addGapsCB when not changing bump mode. */
	  zmapWindowItemFeatureSetFeatureRemove(set_data, feature);
#endif /* RT_63281 */

          /* destroy the canvas item...this will invoke canvasItemDestroyCB() */
          gtk_object_destroy(GTK_OBJECT(feature_item)) ;

	  /* I think we shouldn't need to do this probably....on the other hand showing
	   * empty cols is configurable.... */
	  if (!(zmapWindowContainerHasFeatures((ZMapWindowContainerGroup)container_set)))
	    zmapWindowContainerSetVisibility(FOO_CANVAS_GROUP(container_set), FALSE) ;

	  /* destroy the feature... deletes record in the featureset. */
          if (destroy_feature)
            zMapFeatureDestroy(feature);

          result = TRUE ;
        }
    }



  return result ;
}

ZMapFrame zmapWindowFeatureFrame(ZMapFeature feature)
{
  /* do we need to consider the reverse strand.... or just return ZMAPFRAME_NONE */

  return zMapFeatureFrame(feature);
}

/* Encapulates the rules about which strand a feature will be drawn on.
 *
 * For ZMap this amounts to:
 *                            strand specific     not strand specific
 *       features strand
 *
 *       ZMAPSTRAND_NONE            +                     +
 *       ZMAPSTRAND_FORWARD         +                     +
 *       ZMAPSTRAND_REVERSE         -                     +
 *
 * Some features (e.g. repeats) are derived from alignments that match to the
 * forward or reverse strand and hence the feature can be said to have strand
 * in the alignment sense but for the actual feature (the repeat) the strand
 * is irrelevant.
 *
 * Points to a data problem really, if features are not strand specific then
 * their strand should be ZMAPSTRAND_NONE !
 *
 *  */
ZMapStrand zmapWindowFeatureStrand(ZMapWindow window, ZMapFeature feature)
{
  ZMapFeatureTypeStyle style = NULL;
  ZMapStrand strand = ZMAPSTRAND_FORWARD ;

//  style = zMapFindStyle(window->context_map.styles, feature->style_id) ;
  style = feature->style;     // safe failure...

  g_return_val_if_fail(style != NULL, strand);

  if ((!(zMapStyleIsStrandSpecific(style))) ||
      ((feature->strand == ZMAPSTRAND_FORWARD) ||
       (feature->strand == ZMAPSTRAND_NONE)))
    strand = ZMAPSTRAND_FORWARD ;
  else
    strand = ZMAPSTRAND_REVERSE ;

  if(zMapStyleDisplayInSeparator(style))
    strand = ZMAPSTRAND_NONE;

  return strand ;
}

void zmapWindowFeatureFactoryInit(ZMapWindow window)
{
  ZMapWindowFToIFactoryProductionTeamStruct factory_helpers = {NULL};

  factory_helpers.feature_size_request = factoryFeatureSizeReq;
  factory_helpers.top_item_created     = factoryTopItemCreated;

  zmapWindowFToIFactorySetup(window->item_factory, window->config.feature_line_width,
                             &factory_helpers, (gpointer)window);

  return ;
}


/* Called to draw each individual feature. */
FooCanvasItem *zmapWindowFeatureDraw(ZMapWindow      window,
				     ZMapFeatureTypeStyle style,
				     FooCanvasGroup *set_group,
				     ZMapFeature     feature)
{
  FooCanvasItem *new_feature = NULL ;
  ZMapFeatureContext context ;
  ZMapFeatureAlignment alignment ;
  ZMapFeatureBlock block ;
  ZMapFeatureSet set ;
  ZMapWindowContainerFeatureSet container = (ZMapWindowContainerFeatureSet) set_group;
  gboolean masked;

  /* Users will often not want to see what is on the reverse strand, style specifies what should
   * be shown. */
  if ((zMapStyleIsStrandSpecific(style)) &&
      ((feature->strand == ZMAPSTRAND_REVERSE) && (!zMapStyleIsShowReverseStrand(style))))
    {
      return NULL ;
    }
  if ((zMapStyleIsStrandSpecific(style)) && window->revcomped_features &&
      ((feature->strand == ZMAPSTRAND_FORWARD) && (zMapStyleIsHideForwardStrand(style))))
    {
      return NULL ;
    }

#ifdef MH17_REVCOMP_DEBUG
      printf("FeatureDraw %d-%d\n",feature->x1,feature->x2);
#endif


  /* These should be parameters, rather than continually fetch them, caller will almost certainly know these! */
  set       = (ZMapFeatureSet)zMapFeatureGetParentGroup((ZMapFeatureAny)feature,
							ZMAPFEATURE_STRUCT_FEATURESET) ;

  masked = (zMapStyleGetMode(style) == ZMAPSTYLE_MODE_ALIGNMENT &&  feature->feature.homol.flags.masked);
  if(masked)
    {
      if(container->masked && !window->highlights_set.masked)
      {
        return NULL;
      }
    }

  block     = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)set,
							  ZMAPFEATURE_STRUCT_BLOCK) ;
  alignment = (ZMapFeatureAlignment)zMapFeatureGetParentGroup((ZMapFeatureAny)block,
							      ZMAPFEATURE_STRUCT_ALIGN) ;
  context   = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)alignment,
							    ZMAPFEATURE_STRUCT_CONTEXT) ;

  new_feature = zmapWindowFToIFactoryRunSingle(window->item_factory,
					       NULL,
                                               set_group,
                                               context,
                                               alignment,
                                               block,
                                               set,
                                               feature);
  if(masked && container->masked && new_feature)
      foo_canvas_item_hide(new_feature);

  return new_feature;
}


/* Look for any description in the feature set. */
char *zmapWindowFeatureSetDescription(ZMapFeatureSet feature_set)
{
  char *description = NULL ;

  description = g_strdup_printf("%s  :  %s%s%s", (char *)g_quark_to_string(feature_set->original_id),
				feature_set->description ? "\"" : "",
				feature_set->description ? feature_set->description : "<no description available>",
				feature_set->description ? "\"" : "") ;

  return description ;
}


#if MH17_NOT_USED
// see zmapWindow/zMapWindowUpdateInfoPanel()
void zmapWindowFeatureGetSetTxt(ZMapFeature feature, char **set_name_out, char **set_description_out)
{
  if (feature->parent)
    {
      ZMapFeatureSet feature_set = (ZMapFeatureSet)feature->parent ;

      *set_name_out = g_strdup(g_quark_to_string(feature_set->original_id)) ;

      if (feature_set->description)
	*set_description_out = g_strdup(feature_set->description) ;
    }

  return ;
}
#endif




/* Get various aspects of features source... */
void zmapWindowFeatureGetSourceTxt(ZMapFeature feature, char **source_name_out, char **source_description_out)
{
  if (feature->source_id)
    *source_name_out = g_strdup(g_quark_to_string(feature->source_id)) ;

  if (feature->source_text)
    *source_description_out = g_strdup(g_quark_to_string(feature->source_text)) ;

  return ;
}





char *zmapWindowFeatureDescription(ZMapFeature feature)
{
  char *description = NULL ;

  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature)) ;


  if (feature->description)
    {
      description = g_strdup(feature->description) ;
    }

  return description ;
}






gboolean zMapWindowGetDNAStatus(ZMapWindow window)
{
  gboolean drawable = FALSE;

  /* We just need one of the blocks to have DNA.
   * This enables us to turn on this button as we
   * can't have half sensitivity.  Any block which
   * doesn't have DNA creates a warning for the user
   * to complain about.
   */

  /* check for style too. */
  /* sometimes we don't have a featrue_context ... ODD! */
  if(window->feature_context
     && zMapFindStyle(window->context_map->styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME)))
    {
      drawable = zMapFeatureContextGetDNAStatus(window->feature_context);
    }

  return drawable;
}

FooCanvasGroup *zmapWindowFeatureItemsMakeGroup(ZMapWindow window, GList *feature_items)
{
  FooCanvasGroup *new_group = NULL ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Sort the list of items for position...this looks like a candidate for a general function. */
  feature_items = zmapWindowItemSortByPostion(feature_items) ;

  /* Now get the position of the top item and use that as the position of the new group. */
  item = (FooCanvasItem *)(g_list_first(feature_items)->data) ;
  reparent_data.x_origin = item->x1 ;
  reparent_data.y_origin = item->x2 ;

  /* Make the new group....remember to add an underlay/background....which is hidden by
   * default..... */
  reparent_data.new_parent = new_group = foo_canvas_item_new(FOO_CANVAS_GROUP(item->parent),
							     foo_canvas_group_get_type(),
							     "x", reparent_data.x_origin,
							     "y", reparent_data.y_origin,
							     NULL) ;

  /* Add all the items to the new group correcting their coords. */
  g_list_foreach(feature_items, reparentItemCB, &reparent_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return new_group ;
}


/* Get the peptide as a fasta string.  All contained in a function so
 * there's no falling over phase values and stop codons... */
char *zmapWindowFeatureTranscriptFASTA(ZMapFeature feature, gboolean spliced, gboolean cds_only)
{
  char *peptide_fasta = NULL ;
  ZMapFeatureContext context ;


  if((feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
     && ((context = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)feature,
								  ZMAPFEATURE_STRUCT_CONTEXT))))
    {
      ZMapPeptide peptide;
      char *dna, *seq_name = NULL, *gene_name = NULL;
      int pep_length, start_incr = 0;

      seq_name  = (char *)g_quark_to_string(context->original_id);
      gene_name = (char *)g_quark_to_string(feature->original_id);

      if ((dna = zMapFeatureGetTranscriptDNA(feature, spliced, cds_only)))
	{
	  /* Adjust for when its known that the start exon is incomplete.... */
	  if (feature->feature.transcript.flags.start_not_found)
	    start_incr = feature->feature.transcript.start_phase - 1 ; /* Phase values are 1 <= phase <= 3 */

	  peptide = zMapPeptideCreate(seq_name, gene_name, (dna + start_incr), NULL, TRUE) ;

	  /* Note that we do not include the "Stop" in the peptide length, is this the norm ? */
	  pep_length = zMapPeptideLength(peptide) ;
	  if (zMapPeptideHasStopCodon(peptide))
	    pep_length-- ;

	  peptide_fasta = zMapFASTAString(ZMAPFASTA_SEQTYPE_AA,
					  seq_name, "Protein",
					  gene_name, pep_length,
					  zMapPeptideSequence(peptide));

	  g_free(dna) ;
	}
    }

  return peptide_fasta;
}




/*
 *                       Internal functions.
 */

/* Callback for destroy of feature items... */
static gboolean canvasItemDestroyCB(FooCanvasItem *feature_item, gpointer data)
{
  gboolean event_handled = FALSE ;			    /* Make sure any other callbacks also get run. */
  ZMapWindow window = (ZMapWindowStruct*)data ;

  /* Check to see if there is an entry in long items for this feature.... */
  zmapWindowLongItemRemove(window->long_items, feature_item) ;  /* Ignore boolean result. */

  if (window->focus)
    zmapWindowFocusRemoveOnlyFocusItem(window->focus, feature_item) ;

  return event_handled ;
}




static void featureCopySelectedItem(ZMapFeature feature_in,
                                    ZMapFeature feature_out,
                                    FooCanvasItem *selected)
{
  ZMapFeatureSubPartSpan item_feature_data;
  ZMapSpanStruct span = {0};
  ZMapAlignBlockStruct alignBlock = {0};

  if (feature_in && feature_out)
    memcpy(feature_out, feature_in, sizeof(ZMapFeatureStruct));
  else
    zMapAssertNotReached();

  if ((item_feature_data = g_object_get_data(G_OBJECT(selected), ITEM_SUBFEATURE_DATA)))
    {
      if (feature_out->type == ZMAPSTYLE_MODE_TRANSCRIPT)
        {
          feature_out->feature.transcript.exons   = NULL;
          feature_out->feature.transcript.introns = NULL;
          /* copy the selected intron/exon */
          span.x1 = item_feature_data->start;
          span.x2 = item_feature_data->end;
          if(item_feature_data->subpart == ZMAPFEATURE_SUBPART_EXON ||
             item_feature_data->subpart == ZMAPFEATURE_SUBPART_EXON_CDS)
            zMapFeatureAddTranscriptExonIntron(feature_out, &span, NULL);
          else
            zMapFeatureAddTranscriptExonIntron(feature_out, NULL, &span);
        }
      else if (feature_out->type == ZMAPSTYLE_MODE_ALIGNMENT &&
	       item_feature_data->subpart == ZMAPFEATURE_SUBPART_MATCH)
        {
          feature_out->feature.homol.align = NULL;
          /* copy the selected align */
          alignBlock.q1 = item_feature_data->start;
          alignBlock.q2 = item_feature_data->end;
          feature_out->feature.homol.align = g_array_sized_new(FALSE, TRUE, sizeof(ZMapAlignBlockStruct), 1);
        }
    }

  return ;
}



/* Handle events on items, note that events for text items are passed through without processing
 * so the text item code can do highlighting etc. */
static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;			    /* By default we _don't_ handle events. */
  ZMapWindow window = (ZMapWindowStruct*)data ;
  ZMapFeature feature ;
  static guint32 last_but_release = 0 ;			    /* Used for double clicks... */
  static gboolean second_press = FALSE ;		    /* Used for double clicks... */


  if (event->type == GDK_BUTTON_PRESS || event->type == GDK_2BUTTON_PRESS  || event->type == GDK_BUTTON_RELEASE)
    {
      GdkEventButton *but_event = (GdkEventButton *)event ;
      FooCanvasItem *highlight_item = NULL ;

      zMapDebugPrint(mouse_debug_G, "Start: %s %d",
		     (event->type == GDK_BUTTON_PRESS ? "button_press"
		      : event->type == GDK_2BUTTON_PRESS ? "button_2press" : "button_release"),
		     but_event->button) ;

      if (!ZMAP_IS_CANVAS_ITEM(item))
	{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  g_warning("Not a ZMapWindowCanvasItem.");
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  zMapDebugPrint(mouse_debug_G, "Leave (Not canvas item): %s %d - return FALSE",
			 (event->type == GDK_BUTTON_PRESS ? "button_press"
			  : event->type == GDK_2BUTTON_PRESS ? "button_2press" : "button_release"),
			 but_event->button) ;

	  return FALSE;
	}

      /* Get the feature attached to the item, checking that its type is valid */
      feature  = zMapWindowCanvasItemGetFeature(item);

      if (but_event->type == GDK_BUTTON_PRESS)
	{
	  if (but_event->button == 3)
	    {
	      /* Pop up an item menu. */
	      zmapMakeItemMenu(but_event, window, item) ;
	    }

	  event_handled = TRUE ;
	}
      else if (but_event->type == GDK_2BUTTON_PRESS)
	{
	  second_press = TRUE ;

	  event_handled = TRUE ;
	}
      else						    /* button release */
	{
	  /* Gdk defines double clicks as occuring within 250 milliseconds of each other
	   * but unfortunately if on the first click we do a lot of processing,
	   * STUPID Gdk no longer delivers the GDK_2BUTTON_PRESS so we have to do this
	   * hack looking for the difference in time. This can happen if user clicks on
	   * a very large feature causing us to paste a lot of text to the selection
	   * buffer. */
	  guint but_threshold = 500 ;			    /* Separation of clicks in milliseconds. */

	  if (second_press || but_event->time - last_but_release < but_threshold)
	    {
	      /* Second click of a double click means show feature details. */
	      if (but_event->button == 1)
		{
		  gboolean externally_handled = FALSE ;

		  highlight_item = item;

		  /* If no external handling then show what we can. */
		  if (!(externally_handled = zmapWindowUpdateXRemoteData(window, (ZMapFeatureAny)feature,
									 "edit", highlight_item)))
		    {
		      zmapWindowFeatureShow(window, highlight_item) ;
		    }
		}

	      second_press = FALSE ;
	    }
	  else
	    {
	      event_handled = handleButton(but_event, window, item, feature) ;
	    }

	  last_but_release = but_event->time ;

	  event_handled = TRUE ;
	}

      zMapDebugPrint(mouse_debug_G, "Leave: %s %d - return %s",
		     (event->type == GDK_BUTTON_PRESS ? "button_press"
		      : event->type == GDK_2BUTTON_PRESS ? "button_2press" : "button_release"),
		     but_event->button,
		     event_handled ? "TRUE" : "FALSE") ;
    }

  if (mouse_debug_G)
    fflush(stdout) ;

  return event_handled ;
}


/* Handle button single click to highlight and double click to show feature details. */
static gboolean handleButton(GdkEventButton *but_event, ZMapWindow window, FooCanvasItem *item, ZMapFeature feature)
{
  gboolean event_handled = FALSE ;
  GdkModifierType shift_mask = GDK_SHIFT_MASK,
    control_mask = GDK_CONTROL_MASK,
    shift_control_mask = (GDK_SHIFT_MASK | GDK_CONTROL_MASK),
    unwanted_masks = (GDK_LOCK_MASK | GDK_MOD2_MASK | GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK
		      | GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK
		      | GDK_BUTTON4_MASK | GDK_BUTTON5_MASK),
    locks_mask ;

  /* In order to make the modifier only checks work we need to OR in the unwanted masks that might be on.
   * This includes the shift lock and num lock. Depending on the setup of X these might be mapped
   * to other things which is why MODs 2-5 are included This in theory should include the new (since 2.10)
   * GDK_SUPER_MASK, GDK_HYPER_MASK and GDK_META_MASK */
  if ((locks_mask = (but_event->state & unwanted_masks)))
    {
      shift_mask         |= locks_mask;
      control_mask       |= locks_mask;
      shift_control_mask |= locks_mask;
    }

  /* Button 1 and 3 are handled, 2 is left for a general handler which could be the root handler. */
  if (but_event->button == 1 || but_event->button == 3)
    {
      FooCanvasItem *sub_item = NULL, *highlight_item = NULL ;
      gboolean replace_highlight = TRUE, highlight_same_names = TRUE, externally_handled = FALSE;
      ZMapFeatureSubPartSpan sub_feature ;
      ZMapWindowCanvasItem canvas_item ;
      ZMapFeatureStruct feature_copy = {};
      ZMapFeatureAny my_feature = (ZMapFeatureAny) feature;

      canvas_item = ZMAP_CANVAS_ITEM(item);
      highlight_item = item;

      sub_item = zMapWindowCanvasItemGetInterval(canvas_item, but_event->x, but_event->y, &sub_feature);


      if (zMapGUITestModifiers(but_event, control_mask))
	{
	  /* Only highlight the single item user clicked on. */
	  highlight_same_names = FALSE ;

	  /* Annotators say they don't want subparts sub selections + multiple
	   * selections for alignments. */
	  if (feature->type != ZMAPSTYLE_MODE_ALIGNMENT)
	    {
	      highlight_item = sub_item ;

            /* monkey around to get feature_copy to be the right correct data */
            featureCopySelectedItem(feature, &feature_copy, highlight_item);
            my_feature = (ZMapFeatureAny) &feature_copy;
	    }

      }

      if (zMapGUITestModifiers(but_event, shift_mask))
	{
	  /* multiple selections */

	  if (zmapWindowFocusIsItemInHotColumn(window->focus, item))      //      && window->multi_select)
	    {
	      replace_highlight = FALSE ;
	      externally_handled = zmapWindowUpdateXRemoteData(window, my_feature, "multiple_select", highlight_item);
	    }
	  else
	    {
	      externally_handled = zmapWindowUpdateXRemoteData(window, my_feature, "single_select", highlight_item);
	      window->multi_select = TRUE ;
	    }
	}

      else
	{
	  /* single select */
	  externally_handled = zmapWindowUpdateXRemoteData(window, my_feature, "single_select", highlight_item);
	  window->multi_select = FALSE ;
	}

      /* Pass information about the object clicked on back to the application. */
      zmapWindowUpdateInfoPanel(window, feature, sub_item, highlight_item, 0, 0,
				NULL, replace_highlight, highlight_same_names) ;
    }


  return event_handled ;
}




/* Build the menu for a feature item. */
void zmapMakeItemMenu(GdkEventButton *button_event, ZMapWindow window, FooCanvasItem *item)
{
  ZMapWindowContainerGroup column_group =  NULL ;
  static ZMapGUIMenuItemStruct separator[] =
    {
      {ZMAPGUI_MENU_SEPARATOR, NULL, 0, NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;
  char *menu_title ;
  GList *menu_sets = NULL ;
  ItemMenuCBData menu_data ;
  ZMapFeature feature ;
  ZMapFeatureTypeStyle style ;
  ZMapFeatureSet feature_set;
  ZMapWindowContainerFeatureSet container_set;


  /* Some parts of the menu are feature type specific so retrieve the feature item info
   * from the canvas item. */

      /* MH17:
       * if we get here thay clicked on a feature not the column
       * if the click on the column background the it gets handled in
       * zmapWindowDrawFeatures.c/columnMenuCB()
       */

  feature = zMapWindowCanvasItemGetFeature(item);
  zMapAssert(feature);


  style = feature->style;

  feature_set = (ZMapFeatureSet)(feature->parent);
  menu_title = g_strdup_printf("%s (%s)", zMapFeatureName((ZMapFeatureAny)feature),
                         zMapFeatureSetGetName(feature_set)) ;

  column_group  = zmapWindowContainerCanvasItemGetContainer(item) ;
  container_set = (ZMapWindowContainerFeatureSet) column_group;

  /* Call back stuff.... */
  menu_data = g_new0(ItemMenuCBDataStruct, 1) ;

  menu_data->x = button_event->x ;
  menu_data->y = button_event->y ;
  menu_data->item_cb = TRUE ;
  menu_data->window = window ;
  menu_data->item = item ;
  menu_data->feature = feature;

  /* get the featureset and container for the feature, needed for Show Masked Features */
  menu_data->feature_set = feature_set;
  menu_data->container_set = container_set;

  /* Make up the menu. */

  /* optional developer-only functions. */
  if (zMapUtilsUserIsDeveloper())
    {
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDeveloperOps(NULL, NULL, menu_data)) ;

      menu_sets = g_list_append(menu_sets, separator) ;
    }

  /* Feature ops. */
  menu_sets = g_list_append(menu_sets, makeMenuFeatureOps(NULL, NULL, menu_data)) ;

  if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT && feature->strand == ZMAPSTRAND_FORWARD)
    menu_sets = g_list_append(menu_sets, makeMenuShowTranslation(NULL, NULL, menu_data));

  if (feature->url)
    menu_sets = g_list_append(menu_sets, makeMenuURL(NULL, NULL, menu_data)) ;

  if (feature->type == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      menu_sets = g_list_append(menu_sets, makeMenuPfetchOps(NULL, NULL, menu_data)) ;

      if (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
	{
	  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuProteinHomolFeature(NULL, NULL, menu_data)) ;
	  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuProteinHomol(NULL, NULL, menu_data)) ;
	}
      else
	{
	  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNAHomolFeature(NULL, NULL, menu_data)) ;
	  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNAHomol(NULL, NULL, menu_data)) ;
	}
    }

  /* DNA/Peptide ops. */
  if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNATranscript(NULL, NULL, menu_data)) ;
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNATranscriptFile(NULL, NULL, menu_data)) ;
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuPeptide(NULL, NULL, menu_data)) ;
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuPeptideFile(NULL, NULL, menu_data)) ;
    }
  else
    {
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNAFeatureAny(NULL, NULL, menu_data)) ;
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNAFeatureAnyFile(NULL, NULL, menu_data)) ;
    }

  menu_sets = g_list_append(menu_sets, separator) ;

  menu_sets = g_list_append(menu_sets,
			    zmapWindowMakeMenuBump(NULL, NULL, menu_data,
						   zmapWindowContainerFeatureSetGetBumpMode(container_set))) ;

  menu_sets = g_list_append(menu_sets, separator) ;

  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDumpOps(NULL, NULL, menu_data)) ;

  menu_sets = g_list_append(menu_sets, separator) ;

  if(zmapWindowMarkIsSet(window->mark))
    {
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuMarkDumpOps(NULL, NULL, menu_data));

      menu_sets = g_list_append(menu_sets, separator) ;
    }

  menu_sets = g_list_append(menu_sets, makeMenuGeneralOps(NULL, NULL, menu_data)) ;

  zMapGUIMakeMenu(menu_title, menu_sets, button_event) ;

  g_free(menu_title) ;

  return ;
}





static void itemMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeature feature ;
  gboolean zoom_to_item = TRUE;

#ifndef REQUEST_TO_STOP_ZOOMING_IN_ON_SELECTION
  zoom_to_item = FALSE;
#endif /* REQUEST_TO_STOP_ZOOMING_IN_ON_SELECTION */

  /* Retrieve the feature item info from the canvas item. */
  feature = zmapWindowItemGetFeature(menu_data->item);
  zMapAssert(feature) ;

  switch (menu_item_id)
    {
    case ITEM_MENU_LIST_ALL_FEATURES:
      {
	ZMapWindowFToISetSearchData search_data = NULL;
	ZMapStrand set_strand ;
	ZMapFrame set_frame ;
	gboolean result ;

	result = zmapWindowItemGetStrandFrame(menu_data->item, &set_strand, &set_frame) ;
	zMapAssert(result) ;

	search_data = zmapWindowFToISetSearchCreate(zmapWindowFToIFindItemSetFull, NULL,
						    feature->parent->parent->parent->unique_id,
						    feature->parent->parent->unique_id,
						    menu_data->container_set->unique_id,
                                        0,
						    g_quark_from_string("*"),
						    zMapFeatureStrand2Str(set_strand),
						    zMapFeatureFrame2Str(set_frame));

	zmapWindowListWindow(menu_data->window,
			     menu_data->item,
			     (char *)g_quark_to_string(feature->parent->original_id),
			     NULL, NULL,
			     (ZMapWindowListSearchHashFunc)zmapWindowFToISetSearchPerform, search_data,
			     (GDestroyNotify)zmapWindowFToISetSearchDestroy, zoom_to_item) ;
	break ;
      }
    case ITEM_MENU_LIST_NAMED_FEATURES:
      {
	ZMapWindowFToISetSearchData search_data = NULL;
	ZMapStrand set_strand ;
	ZMapFrame set_frame ;
	gboolean result ;

	result = zmapWindowItemGetStrandFrame(menu_data->item, &set_strand, &set_frame) ;
	zMapAssert(result) ;

	search_data = zmapWindowFToISetSearchCreate(zmapWindowFToIFindSameNameItems, feature,
						    0, 0, menu_data->container_set->unique_id, 0, 0, zMapFeatureStrand2Str(set_strand),
						    zMapFeatureFrame2Str(set_frame));
	zmapWindowListWindow(menu_data->window,
			     menu_data->item,
			     (char *)g_quark_to_string(feature->parent->original_id),
			     NULL, NULL,
			     (ZMapWindowListSearchHashFunc)zmapWindowFToISetSearchPerform, search_data,
			     (GDestroyNotify)zmapWindowFToISetSearchDestroy, zoom_to_item) ;
	break ;
      }
    case ITEM_MENU_FEATURE_DETAILS:
      {
	zmapWindowFeatureShow(menu_data->window, menu_data->item) ;

	break ;
      }
    case ITEM_MENU_MARK_ITEM:
      zmapWindowMarkSetItem(menu_data->window->mark, menu_data->item) ;

      break ;
    case ITEM_MENU_SEARCH:
      zmapWindowCreateSearchWindow(menu_data->window,
				   NULL, NULL,
				   menu_data->item) ;

      break ;
    case ITEM_MENU_PFETCH:
      pfetchEntry(menu_data->window, (char *)g_quark_to_string(feature->original_id)) ;
      break ;

    case ITEM_MENU_SEQUENCE_SEARCH_DNA:
      zmapWindowCreateSequenceSearchWindow(menu_data->window, menu_data->item, ZMAPSEQUENCE_DNA) ;

      break ;

    case ITEM_MENU_SEQUENCE_SEARCH_PEPTIDE:
      zmapWindowCreateSequenceSearchWindow(menu_data->window, menu_data->item, ZMAPSEQUENCE_PEPTIDE) ;

      break ;

    case ITEM_MENU_SHOW_URL_IN_BROWSER:
      {
	gboolean result ;
	GError *error = NULL ;

	if (!(result = zMapLaunchWebBrowser(feature->url, &error)))
	  {
	    zMapWarning("Error: %s\n", error->message) ;

	    g_error_free(error) ;
	  }
	break ;
      }
    case ITEM_MENU_SHOW_TRANSLATION:
      {
	zmapWindowItemShowTranslation(menu_data->window,
				      menu_data->item);
      }
      break;
    case ITEM_MENU_TOGGLE_MARK:
      {
	zmapWindowToggleMark(menu_data->window, 0);
      }
      break;

    case ITEM_MENU_SHOW_EVIDENCE:
    case ITEM_MENU_ADD_EVIDENCE:

      {
            // show evidence for a transcript
        ZMapWindowFocus focus = menu_data->window->focus;

        if(focus)
          {
            GList *evidence;
            GList *evidence_items = NULL;
            ZMapFeatureAny any = (ZMapFeatureAny) menu_data->feature;   /* our transcript */

            /* clear any existing highlight */
            if(menu_item_id != ITEM_MENU_ADD_EVIDENCE)
                  zmapWindowFocusResetType(focus,WINDOW_FOCUS_GROUP_EVIDENCE);

            /* add the transcript to the evidence group */
            zmapWindowFocusAddItemType(menu_data->window->focus, menu_data->item, WINDOW_FOCUS_GROUP_EVIDENCE);

            /* request the list of features from otterlace */
            evidence = zmapWindowFeatureGetEvidence(menu_data->window,menu_data->feature);

            /* search for the features named in the list and find thier canvas items */

            for(;evidence;evidence = evidence->next)
            {
                  GList *items,*items_free;
                  GQuark wildcard = g_quark_from_string("*");
                  char *feature_name;
                  GQuark feature_search_id;

                  /* need to add a * to the end to match strand and frame name mangling */

                  feature_name = g_strdup_printf("%s*",g_quark_to_string(GPOINTER_TO_UINT(evidence->data)));
                  feature_name = zMapFeatureCanonName(feature_name);    /* done in situ */
                  feature_search_id = g_quark_from_string(feature_name);

                  items_free = zmapWindowFToIFindItemSetFull(menu_data->window,
                             menu_data->window->context_to_item,
                             any->parent->parent->parent->unique_id,
                             any->parent->parent->unique_id,
                             wildcard,0,"*","*",
                             feature_search_id,
                             NULL,NULL);

                  /* zMapLogWarning("evidence %s returns %d features\n", feature_name, g_list_length(items_free));*/
                  g_free(feature_name);

                  for(items = items_free; items; items = items->next)
                  {
                        /* NOTE: need to filter by transcript start and end coords in case of repeat alignments */
                        /* Not so - annotators want to see duplicated features' data */

                        evidence_items = g_list_prepend(evidence_items,items->data);

                  }
                  g_list_free(items_free);
            }

            /* menu_data->item is the transcript and would be the
             * focus hot item if this was a focus highlight
             * in this call it's irrelevant
             */
            zmapWindowFocusAddItemsType(menu_data->window->focus, evidence_items,
                  menu_data->item, WINDOW_FOCUS_GROUP_EVIDENCE);


            g_list_free(evidence);
            g_list_free(evidence_items);
          }
      }
      break;

    case ITEM_MENU_SHOW_TRANSCRIPT:
      {
      }
      break;

#ifdef RDS_DONT_INCLUDE
    case 101:
      zmapWindowContextExplorerCreate(menu_data->window, (ZMapFeatureAny)feature);
      break;
#endif
    default:
      zMapAssertNotReached() ;				    /* exits... */
      break ;
    }

  g_free(menu_data) ;

  return ;
}




/* This is in the general menu and needs to be handled separately perhaps as the index is a global
 * one shared amongst all general menu functions...
 * NOTE HOW THE MENUS ARE DECLARED STATIC IN THE VARIOUS ROUTINES TO MAKE SURE THEY STAY
 * AROUND...OTHERWISE WE WILL HAVE TO KEEP ALLOCATING/DEALLOCATING THEM.....
 */
static ZMapGUIMenuItem makeMenuFeatureOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, "Show Feature Details", ITEM_MENU_FEATURE_DETAILS, itemMenuCB, NULL, "Return"},
      {ZMAPGUI_MENU_NORMAL, "Set Feature for Bump", ITEM_MENU_MARK_ITEM,       itemMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         itemMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         itemMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         NULL,       NULL}
    } ;

  ItemMenuCBData md = (ItemMenuCBData) callback_data;
  int i = 2;

  menu[i].type = ZMAPGUI_MENU_NONE;

            // add in evidence/ transcript items
            // option to remove existing is in column menu
      if(md->feature && md->feature->style)
      {
            if(md->feature->style->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
            {
                  menu[i].type = ZMAPGUI_MENU_NORMAL;
                  menu[i].name = "Highlight Evidence";
                  menu[i].id = ITEM_MENU_SHOW_EVIDENCE;
                  i++;
                  menu[i].type = ZMAPGUI_MENU_NORMAL;
                  menu[i].name = "Highlight Evidence (add more)";
                  menu[i].id = ITEM_MENU_ADD_EVIDENCE;
            }
#if MH17_NOT_IMPLEMENTED
            else
            {
                  menu[i].type = ZMAPGUI_MENU_NORMAL;
                  menu[i].name = "Highlight Transcript";
                  menu[i].id = ITEM_MENU_SHOW_TRANSCRIPT;
            }
#endif

      }
      else
      {
            // style should be attached to the feature, but if not don't fall over
            // new features should also have styles attached
            zMapLogWarning("Feature menu item does not have style","");
      }

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

static ZMapGUIMenuItem makeMenuShowTranslation(int *start_index_inout,
					       ZMapGUIMenuItemCallbackFunc callback_func,
					       gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, "Show Translation in ZMap", ITEM_MENU_SHOW_TRANSLATION, itemMenuCB, NULL, "T"},
      {ZMAPGUI_MENU_NONE, NULL,                         ITEM_MENU_INVALID,          NULL,       NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}



static ZMapGUIMenuItem makeMenuPfetchOps(int *start_index_inout,
					 ZMapGUIMenuItemCallbackFunc callback_func,
					 gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, "Pfetch this feature", ITEM_MENU_PFETCH,  itemMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                    ITEM_MENU_INVALID, NULL,       NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}



static ZMapGUIMenuItem makeMenuGeneralOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
/* this is identical or very nearly with columnMenuCB in zmapWindowDrawFeatures.c and should be combined */

      {ZMAPGUI_MENU_NORMAL, "List All Column Features",       ITEM_MENU_LIST_ALL_FEATURES,   itemMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "List This Name Column Features", ITEM_MENU_LIST_NAMED_FEATURES, itemMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Feature Search Window",          ITEM_MENU_SEARCH,              itemMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "DNA Search Window",              ITEM_MENU_SEQUENCE_SEARCH_DNA, itemMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Peptide Search Window",          ITEM_MENU_SEQUENCE_SEARCH_PEPTIDE, itemMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Toggle Mark",                    ITEM_MENU_TOGGLE_MARK,             itemMenuCB, NULL, "M"},
      {ZMAPGUI_MENU_NONE, NULL,                               ITEM_MENU_INVALID,                 NULL,       NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}



static ZMapGUIMenuItem makeMenuURL(int *start_index_inout,
				   ZMapGUIMenuItemCallbackFunc callback_func,
				   gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, "URL", ITEM_MENU_SHOW_URL_IN_BROWSER, itemMenuCB, NULL},
      {ZMAPGUI_MENU_NONE,   NULL,  ITEM_MENU_INVALID,             NULL,       NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void textSelectCB(int menu_item_id, gpointer callback_data)
{
#ifdef RDS_BREAKING_STUFF
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapWindowItemHighlighter hlght = NULL;

  switch(menu_item_id){
  case 3:
    zMapWarning("%s", "Manually selecting the text will copy it to the clip buffer automatically");
    break;
  case 2:
    if((hlght = zmapWindowItemTextHighlightRetrieve(FOO_CANVAS_GROUP(menu_data->item))))
      zmapWindowItemTextHighlightReset(hlght);
    break;
  default:
    zMapWarning("%s", "Unimplemented feature");
    break;
  }
#endif
  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static ZMapGUIMenuItem makeMenuTextSelectOps(int *start_index_inout,
                                             ZMapGUIMenuItemCallbackFunc callback_func,
                                             gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, "Select All",  1, textSelectCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Select None", 2, textSelectCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Copy",        3, textSelectCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     0, NULL,       NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static PFetchStatus pfetch_reader_func(PFetchHandle *handle,
				       char         *text,
				       guint        *actual_read,
				       GError       *error,
				       gpointer      user_data)
{
  PFetchData pfetch_data = (PFetchData)user_data;
  PFetchStatus status    = PFETCH_STATUS_OK;

  if(actual_read && *actual_read > 0 && pfetch_data)
    {
      GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(pfetch_data->text_buffer);

      /* clear the buffer the first time... */
      if(pfetch_data->got_response == FALSE)
	gtk_text_buffer_set_text(text_buffer, "", 0);

      gtk_text_buffer_insert_at_cursor(text_buffer, text, *actual_read);

      pfetch_data->got_response = TRUE;
    }

  return status;
}

static PFetchStatus pfetch_closed_func(gpointer user_data)
{
  PFetchStatus status = PFETCH_STATUS_OK;
#ifdef DEBUG_ONLY
  printf("pfetch closed\n");
#endif	/* DEBUG_ONLY */
  return status;
}

static void pfetchEntry(ZMapWindow window, char *sequence_name)
{
  PFetchData pfetch_data = g_new0(PFetchDataStruct, 1);
  PFetchHandle    pfetch = NULL;
  PFetchUserPrefsStruct prefs = {NULL};
  gboolean  debug_pfetch = FALSE;

  if((zmapWindowGetPFetchUserPrefs(&prefs)) &&
     (prefs.location   != NULL))
    {
      GType pfetch_type = PFETCH_TYPE_HTTP_HANDLE;

      if(prefs.mode && strncmp(prefs.mode, "pipe", 4) == 0)
	pfetch_type = PFETCH_TYPE_PIPE_HANDLE;

      pfetch_data->pfetch = pfetch = PFetchHandleNew(pfetch_type);

      if((pfetch_data->title = g_strdup_printf(PFETCH_TITLE_FORMAT, sequence_name)))
	{
	  pfetch_data->dialog = zMapGUIShowTextFull(pfetch_data->title, "pfetching...\n",
						    FALSE, &(pfetch_data->text_buffer));

	  pfetch_data->widget_destroy_handler_id =
	    g_signal_connect(G_OBJECT(pfetch_data->dialog), "destroy",
			     G_CALLBACK(handle_dialog_close), pfetch_data);
	}

      PFetchHandleSettings(pfetch,
			   "debug",       debug_pfetch,
			   "full",        prefs.full_record,
			   "pfetch",      prefs.location,
			   "isoform-seq", TRUE,
			   NULL);

      if(PFETCH_IS_HTTP_HANDLE(pfetch))
	PFetchHandleSettings(pfetch,
			     "port",       prefs.port,
			     "cookie-jar", prefs.cookie_jar,
			     NULL);

      g_free(prefs.location);
      g_free(prefs.cookie_jar);

      g_signal_connect(G_OBJECT(pfetch), "reader", G_CALLBACK(pfetch_reader_func), pfetch_data);

      g_signal_connect(G_OBJECT(pfetch), "error",  G_CALLBACK(pfetch_reader_func), pfetch_data);

      g_signal_connect(G_OBJECT(pfetch), "closed", G_CALLBACK(pfetch_closed_func), pfetch_data);

      zMapThreadForkLock();   // see zmapThreads.c

      if(PFetchHandleFetch(pfetch, sequence_name) == PFETCH_STATUS_FAILED)
      	zMapWarning("Error fetching sequence '%s'", sequence_name);

      zMapThreadForkUnlock();
    }
  else
    zMapWarning("%s", "Failed to obtain preferences for pfetch.\n"
		"ZMap's config file needs at least pfetch "
		"entry in the ZMap stanza.");

  return ;
}

/* GtkObject destroy signal handler */
static void handle_dialog_close(GtkWidget *dialog, gpointer user_data)
{
  PFetchData pfetch_data   = (PFetchData)user_data;
  pfetch_data->text_buffer = NULL;
  pfetch_data->widget_destroy_handler_id = 0; /* can we get this more than once? */

  if(pfetch_data->pfetch)
    pfetch_data->pfetch = PFetchHandleDestroy(pfetch_data->pfetch);

  return ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* We may wish to make this public some time but as the rest of the long item calls for features
 * are in this file then there is no need for it to be exposed.
 *
 * Deals with removing all the items contained in a compound object from the longitem list.
 *
 *  */
static void removeFeatureLongItems(ZMapWindowLongItems long_items, FooCanvasItem *feature_item)
{
  ZMapWindowItemFeatureType type ;

  zMapAssert(long_items && feature_item) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(feature_item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(type == ITEM_FEATURE_SIMPLE
	     || type == ITEM_FEATURE_PARENT
	     || type == ITEM_FEATURE_CHILD || type == ITEM_FEATURE_BOUNDING_BOX) ;

  /* If item is part of a compound feature then get the parent. */
  if (type == ITEM_FEATURE_CHILD || type == ITEM_FEATURE_BOUNDING_BOX)
    feature_item = feature_item->parent ;

  /* Test for long items according to whether feature is simple or compound. */
  if (type == ITEM_FEATURE_SIMPLE)
    {
      zmapWindowLongItemRemove(long_items, feature_item) ;  /* Ignore boolean result. */
    }
  else
    {
      FooCanvasGroup *feature_group = FOO_CANVAS_GROUP(feature_item) ;

      g_list_foreach(feature_group->item_list, removeLongItemCB, long_items) ;
    }

  return ;
}


/* GFunc callback for removing long items. */
static void removeLongItemCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ZMapWindowLongItems long_items = (ZMapWindowLongItems)user_data ;

  zmapWindowLongItemRemove(long_items, item) ;		    /* Ignore boolean result. */

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Function to check whether any of the blocks has dna */
static ZMapFeatureContextExecuteStatus oneBlockHasDNA(GQuark key,
                                                      gpointer data,
                                                      gpointer user_data,
                                                      char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureBlock     feature_block = NULL;
  ZMapFeatureStructType feature_type = ZMAPFEATURE_STRUCT_INVALID;

  BlockHasDNA dna = (BlockHasDNA)user_data;

  feature_type = feature_any->struct_type;

  switch(feature_type)
    {
    case ZMAPFEATURE_STRUCT_BLOCK:
      feature_block = (ZMapFeatureBlock)feature_any;
      if(!dna->exists)
        dna->exists = (gboolean)(feature_block->sequence.length ? TRUE : FALSE);
      break;
    case ZMAPFEATURE_STRUCT_CONTEXT:
    case ZMAPFEATURE_STRUCT_FEATURESET:
    case ZMAPFEATURE_STRUCT_FEATURE:
    case ZMAPFEATURE_STRUCT_ALIGN:
      break;
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      zMapAssertNotReached();
      break;

    }

  return ZMAP_CONTEXT_EXEC_STATUS_OK;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* THIS ALL LOOKS LIKE IT NEEDS WRITING SOMETIME.... */

/* Move a canvas item into a new group, redoing its coords as they must be relative to
 * the new parent. */
static void reparentItemCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *feature_item = (FooCanvasItem *)data ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  foo_canvas_item_reparent (FooCanvasItem *item, FooCanvasGroup *new_group);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static double getWidthFromScore(ZMapFeatureTypeStyle style, double score)
{
  double tmp, width = 0.0 ;
  double fac ;

  fac = style->width / (style->max_score - style->min_score) ;

  if (score <= style->min_score)
    tmp = 0 ;
  else if (score >= style->max_score)
    tmp = style->width ;
  else
    tmp = fac * (score - style->min_score) ;

  width = tmp ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if (seg->data.f <= bc->meth->minScore)
	x = 0 ;
      else if (seg->data.f >= bc->meth->maxScore)
	x = bc->width ;
      else
	x = fac * (seg->data.f - bc->meth->minScore) ;

      box = graphBoxStart() ;

      if (x > origin + 0.5 || x < origin - 0.5)
	graphLine (bc->offset+origin, y, bc->offset+x, y) ;
      else if (x > origin)
	graphLine (bc->offset+origin-0.5, y, bc->offset+x, y) ;
      else
	graphLine (bc->offset+origin+0.5, y, bc->offset+x, y) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return width ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Here's how acedb does all this..... */

void fMapShowSplices (LOOK genericLook, float *offset, MENU *menu)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  float x, y1, y2, origin ;
  BoxCol *bc ;

  bc = bcFromName (look, look->map->activeColName, menu) ;
	/* NB old default diff was 5 - now using bc system it is 1 */
  if (!bcTestMag (bc, look->map->mag))
    return ;
  bc->offset = *offset ;

  y1 = MAP2GRAPH(look->map,look->min) ;
  y2 = MAP2GRAPH(look->map,look->max) ;

  graphColor(LIGHTGRAY) ;
  x = *offset + 0.5 * bc->width ;  graphLine (x, y1, x, y2) ;
  x = *offset + 0.75 * bc->width ;  graphLine (x, y1, x, y2) ;

  graphColor(DARKGRAY) ;
  if (bc->meth->minScore < 0 && 0 < bc->meth->maxScore)
    origin = bc->width *
      (-bc->meth->minScore / (bc->meth->maxScore - bc->meth->minScore)) ;
  else
    origin = 0 ;
  graphLine (*offset + origin, y1, *offset + origin, y2) ;

  graphColor (BLACK) ;
  showSplices (look, SPLICE5, bc, origin) ;
  showSplices (look, SPLICE3, bc, origin) ;

  *offset += bc->width + 1 ;

  return;
} /* fMapShowSplices */


static void showSplices (FeatureMap look, SegType type, BoxCol *bc, float origin)
{
  char  *v ;
  int i, box, background ;
  SEG *seg ;
  float y=0, delta=0, x=0 ; /* delta: mieg: shows frame by altering the drawing of the arrow */
  float fac;

  fac = bc->width / (bc->meth->maxScore - bc->meth->minScore);

  for (i = 0 ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs, i, SEG) ;
      if (seg->x1 > look->max
	  || seg->x2 < look->min
	  || (lexAliasOf(seg->key) != bc->meth->key)
	  || seg->type != type)
	continue ;
      y = MAP2GRAPH(look->map, seg->x2) ;
      if (seg->data.f <= bc->meth->minScore)
	x = 0 ;
      else if (seg->data.f >= bc->meth->maxScore)
	x = bc->width ;
      else
	x = fac * (seg->data.f - bc->meth->minScore) ;
      box = graphBoxStart() ;
      if (x > origin + 0.5 || x < origin - 0.5)
	graphLine (bc->offset+origin, y, bc->offset+x, y) ;
      else if (x > origin)
	graphLine (bc->offset+origin-0.5, y, bc->offset+x, y) ;
      else
	graphLine (bc->offset+origin+0.5, y, bc->offset+x, y) ;
      switch (type)
	{
	case SPLICE5:
	  delta = (look->flag & FLAG_REVERSE) ? -0.5 : 0.5 ;
	  break ;
	case SPLICE3:
	  delta = (look->flag & FLAG_REVERSE) ? 0.5 : -0.5 ;
	  break ;
        default:
	  messcrash ("Bad type %d in showSplices", type) ;
	}
      graphLine (bc->offset+x, y, bc->offset+x, y+delta) ;
      graphBoxEnd() ;
      v = SEG_HASH (seg) ;
      if (assFind (look->chosen, v, 0))
	background = GREEN ;
      else if (assFind (look->antiChosen, v, 0))
	background = LIGHTGREEN ;
      else
	background = TRANSPARENT ;
      switch (seg->x2 % 3)
	{
	case 0:
	  graphBoxDraw (box, RED, background) ; break ;
	case 1:
	  graphBoxDraw (box, BLUE, background) ; break ;
	case 2:
	  graphBoxDraw (box, DARKGREEN, background) ; break ;
	}
      array(look->boxIndex, box, int) = i ;
      fMapBoxInfo (look, box, seg) ;
      graphBoxFreeMenu (box, fMapChooseMenuFunc, fMapChooseMenu) ;
    }

  return;
} /* showSplices */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static gboolean sequenceSelectionCB(FooCanvasItem *item, int start, int end, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data ;
  ZMapWindowSequenceFeature sequence_feature ;
  ZMapFeature feature;

  sequence_feature = ZMAP_WINDOW_SEQUENCE_FEATURE(item);

  feature = zMapWindowCanvasItemGetFeature(FOO_CANVAS_ITEM(sequence_feature)) ;

  if (feature->feature.sequence.type == ZMAPSEQUENCE_DNA)
    {
      zmapWindowItemHighlightTranslationRegions(window, FALSE, item,
						feature->feature.sequence.type, start, end) ;
    }
  else
    {
      zmapWindowItemHighlightDNARegion(window, FALSE, item, feature->feature.sequence.frame,
				       feature->feature.sequence.type, start, end) ;

      /* We want to highlight the peptides correctly but really we need to take the frame of
       * the selected peptide column, get the corresponding dna, then for the remaining frames
       * clip their translations to this section of dna..... */
      zmapWindowItemHighlightTranslationRegions(window, FALSE, item,
						feature->feature.sequence.type, start, end) ;
    }

  /* Pass information about the object clicked on back to the application. */
  zmapWindowUpdateInfoPanel(window, feature, item, item, start, end, NULL, FALSE, FALSE) ;

  return FALSE ;
}



static gboolean factoryTopItemCreated(FooCanvasItem *top_item,
                                      ZMapFeatureContext context,
                                      ZMapFeatureAlignment align,
                                      ZMapFeatureBlock block,
                                      ZMapFeatureSet set,
                                      ZMapFeature feature,
                                      gpointer handler_data)
{
  g_signal_connect(GTK_OBJECT(top_item), "destroy",
		   GTK_SIGNAL_FUNC(canvasItemDestroyCB), handler_data) ;


  /* the problem with doing this kind of thing is that we also need the canvasItemEventCB to be
   *  attached as there are general things we need to do as well...
   *
   * REVISIT THE WHOLE EVENT DELIVERY ORDER STUFF.....
   *
   *  */

  /* ummmmm....I don't like this....suggests that all is not fully implemented in the new
   * feature item stuff..... */
  if (ZMAP_IS_WINDOW_SEQUENCE_FEATURE(top_item))
    g_signal_connect(G_OBJECT(top_item), "sequence-selected",
		     G_CALLBACK(sequenceSelectionCB), handler_data) ;


  switch(feature->type)
    {
    case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
    case ZMAPSTYLE_MODE_TRANSCRIPT:
    case ZMAPSTYLE_MODE_ALIGNMENT:
    case ZMAPSTYLE_MODE_BASIC:
    case ZMAPSTYLE_MODE_TEXT:
    case ZMAPSTYLE_MODE_GLYPH:
    case ZMAPSTYLE_MODE_GRAPH:
    case ZMAPSTYLE_MODE_RAW_SEQUENCE:
    case ZMAPSTYLE_MODE_PEP_SEQUENCE:
      g_signal_connect(G_OBJECT(top_item), "event", G_CALLBACK(canvasItemEventCB), handler_data);
      break;

    default:
      break;
    }


  /* the problem with doing this kind of thing is that we also need the canvasItemEventCB to be
   *  attached as there are general things we need to do as well... */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* ummmmm....I don't like this....suggests that all is not fully implemented in the new
   * feature item stuff..... */
  if (ZMAP_IS_WINDOW_SEQUENCE_FEATURE(top_item))
    g_signal_connect(G_OBJECT(top_item), "sequence-selected",
		     G_CALLBACK(sequenceSelectionCB), handler_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return TRUE ;
}




static gboolean factoryFeatureSizeReq(ZMapFeature feature,
                                      double *limits_array,
                                      double *points_array_inout,
                                      gpointer handler_data)
{
  gboolean outside = FALSE;
  double x1_in = points_array_inout[1];
  double x2_in = points_array_inout[3];
  int start_end_crossing = 0;
  double block_start, block_end;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* There is a least one problem with this...early on the canvas is only 100 pixels square
   * so guess what size we end up using....sigh...and in addition it's the block size
   * we should be using anyway.... */

  if(feature->type == ZMAPSTYLE_MODE_RAW_SEQUENCE ||
     feature->type == ZMAPSTYLE_MODE_PEP_SEQUENCE)
    {
      ZMapWindow window = (ZMapWindow)handler_data;
      double x1, x2;

      points_array_inout[1] = points_array_inout[3] = x1 = x2 = 0.0;
      /* Get scrolled region (clamped to sequence coords)  */
      zmapWindowGetScrollRegion(window,
                                 &x1, &(points_array_inout[1]),
                                 &x2, &(points_array_inout[3]));
    }
  else
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    {
      block_start = limits_array[1];
      block_end   = limits_array[3];

      /* shift according to how we cross, like this for ease of debugging, not speed */
      start_end_crossing |= ((x1_in < block_start) << 1);
      start_end_crossing |= ((x2_in > block_end)   << 2);
      start_end_crossing |= ((x1_in > block_end)   << 3);
      start_end_crossing |= ((x2_in < block_start) << 4);

      /* Now check whether we cross! */
      if(start_end_crossing & 8 ||
         start_end_crossing & 16) /* everything is out of range don't display! */
        outside = TRUE;

      if(start_end_crossing & 2)
        points_array_inout[1] = block_start;
      if(start_end_crossing & 4)
        points_array_inout[3] = block_end;
    }

 return outside;
}


FooCanvasItem *addNewCanvasItem(ZMapWindow window, FooCanvasGroup *feature_group,
				ZMapFeature feature, gboolean bump_col)
{
  FooCanvasItem *new_feature = NULL ;
  ZMapFeatureTypeStyle style ;
  gboolean column_is_empty = FALSE;
  FooCanvasGroup *container_features;
  ZMapStyleBumpMode bump_mode;
  ZMapWindowContainerFeatureSet container_set = (ZMapWindowContainerFeatureSet) feature_group;

  style = feature->style;

  container_features = FOO_CANVAS_GROUP(zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)feature_group));
  column_is_empty = !(container_features->item_list);

  /* This function will add the new feature to the hash. */
  new_feature = zmapWindowFeatureDraw(window, style, FOO_CANVAS_GROUP(feature_group), feature) ;

  if (bump_col)
    {
      if((bump_mode = zmapWindowContainerFeatureSetGetBumpMode(container_set)) != ZMAPBUMP_UNBUMP)
	{
	  zmapWindowColumnBump(FOO_CANVAS_ITEM(feature_group), bump_mode);
	}

      if (column_is_empty)
	zmapWindowColOrderPositionColumns(window);
    }


  return new_feature ;
}

