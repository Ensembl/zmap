/*  File: zmapWindowFeature.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Description: Functions that manipulate displayed features, they
 *              encapsulate handling of the feature context, the
 *              displayed canvas items and the feature->item hash.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Oct 25 15:36 2007 (edgrif)
 * Created: Mon Jan  9 10:25:40 2006 (edgrif)
 * CVS info:   $Id: zmapWindowFeature.c,v 1.117 2007-11-01 15:04:21 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <math.h>
#include <ZMap/zmapFASTA.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapPeptide.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>
#include <zmapWindowItemFactory.h>
#include <zmapWindowItemTextFillColumn.h>

#define PFETCH_READ_SIZE 80	/* about a line */
#define PFETCH_FAILED_PREFIX "PFetch failed:"

typedef struct
{
  GtkWidget *dialog;
  GtkTextBuffer *text_buffer;
  gulong widget_destroy_handler_id;
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

typedef struct
{
  FooCanvasItem   *dna_item;
  GdkEvent        *event;
  int              origin_index;
  gboolean         selected;
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
static ZMapGUIMenuItem makeMenuPfetchOps(int *start_index_inout,
					 ZMapGUIMenuItemCallbackFunc callback_func,
					 gpointer callback_data) ;
static ZMapGUIMenuItem makeMenuGeneralOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data) ;
static void itemMenuCB(int menu_item_id, gpointer callback_data) ;

static gboolean dnaItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean canvasItemDestroyCB(FooCanvasItem *item, gpointer data) ;

static gboolean event_to_char_cell_coords(FooCanvasPoints **points_out, 
                                          FooCanvasItem    *subject, 
                                          gpointer          user_data);

static void pfetchEntry(ZMapWindow window, char *sequence_name) ;
static gboolean pfetch_stdout_io_func(GIOChannel *source, GIOCondition cond, gpointer user_data);
static gboolean pfetch_stderr_io_func(GIOChannel *source, GIOCondition cond, gpointer user_data);
static void handle_dialog_close(GtkWidget *dialog, gpointer user_data);
static void free_pfetch_data(PFetchData pfetch_data);


static ZMapFeatureContextExecuteStatus oneBlockHasDNA(GQuark key, 
                                                      gpointer data, 
                                                      gpointer user_data,
                                                      char **error_out);

static gboolean factoryTopItemCreated(FooCanvasItem *top_item,
                                      ZMapFeatureContext context,
                                      ZMapFeatureAlignment align,
                                      ZMapFeatureBlock block,
                                      ZMapFeatureSet set,
                                      ZMapFeature feature,
                                      gpointer handler_data);
static void factoryItemCreated(FooCanvasItem            *new_item,
                               ZMapWindowItemFeatureType new_item_type,
                               ZMapFeature               full_feature,
                               ZMapWindowItemFeature     sub_feature,
                               double                    new_item_y1,
                               double                    new_item_y2,
                               gpointer                  handler_data);
static gboolean factoryFeatureSizeReq(ZMapFeature feature, 
                                      double *limits_array, 
                                      double *points_array_inout, 
                                      gpointer handler_data);

static void cleanUpFeatureCB(gpointer data, gpointer user_data) ;



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




GData *zMapWindowFeatureAllStyles(ZMapWindow window)
{
  zMapAssert(window && window->feature_context);

  return window->feature_context->styles ;
}





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


  feature_set = zmapWindowContainerGetData(feature_group, ITEM_FEATURE_DATA) ;
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
  FooCanvasGroup *forward_strand;
  FooCanvasGroup *reverse_strand;
  GQuark style_id,feature_set_id;

  feature_block = zmapWindowContainerGetData(block_group, ITEM_FEATURE_DATA);
  zMapAssert(feature_block);
  
  context = (ZMapFeatureContext)(zMapFeatureGetParentGroup((ZMapFeatureAny)feature_block, ZMAPFEATURE_STRUCT_CONTEXT));
  zMapAssert(context);

  feature_set_id = zMapFeatureSetCreateID(feature_set_name);
  style_id = zMapStyleCreateID(feature_set_name);

  /* Make sure it's somewhere in our list of feature set names.... columns to draw. */
  if(g_list_find(window->feature_set_names, GUINT_TO_POINTER(feature_set_id)) &&
     (style = zMapFindStyle(context->styles, style_id)))
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
      forward_strand = zmapWindowContainerGetStrandGroup(block_group, ZMAPSTRAND_FORWARD);
      reverse_strand = zmapWindowContainerGetStrandGroup(block_group, ZMAPSTRAND_REVERSE);
      
      /* Create the columns */
      zmapWindowCreateSetColumns(window,
                                 forward_strand, 
                                 reverse_strand,
                                 feature_block, 
                                 feature_set,
                                 ZMAPFRAME_NONE, 
                                 &new_forward_set, 
                                 &new_reverse_set);
      
      zmapWindowColOrderColumns(window);

    }

  if (new_forward_set != NULL)
    new_canvas_item = FOO_CANVAS_ITEM(new_forward_set) ;

  return new_canvas_item ;
}

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

  curr_feature = g_object_get_data(G_OBJECT(curr_feature_item), ITEM_FEATURE_DATA) ;
  zMapAssert(curr_feature && zMapFeatureIsValid((ZMapFeatureAny)curr_feature)) ;

  feature_set = (ZMapFeatureSet)(curr_feature->parent) ;

  /* Feature must exist in current set to be replaced...belt and braces....??? */
  if (zMapFeatureSetFindFeature(feature_set, curr_feature))
    {
      FooCanvasGroup *set_group ;

      set_group = zmapWindowContainerGetParentContainerFromItem(curr_feature_item) ;
      zMapAssert(set_group) ;

      /* Remove it completely. */
      if (zMapWindowFeatureRemove(zmap_window, curr_feature_item, destroy_orig_feature))
	{
	  replaced_feature = addNewCanvasItem(zmap_window, set_group, new_feature, FALSE) ;
	}
    }

  return replaced_feature ;
}


/* Remove an existing feature from the displayed feature context.
 * 
 * Returns FALSE if the feature does not exist. */
gboolean zMapWindowFeatureRemove(ZMapWindow zmap_window, FooCanvasItem *feature_item, gboolean destroy_feature)
{
  gboolean result = FALSE ;
  ZMapFeature feature ;
  ZMapWindowItemFeatureSetData set_data ;
  ZMapFeatureSet feature_set ;
  FooCanvasGroup *set_group ;


  feature = g_object_get_data(G_OBJECT(feature_item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature && zMapFeatureIsValid((ZMapFeatureAny)feature)) ;
  feature_set = (ZMapFeatureSet)(feature->parent) ;

  set_group = zmapWindowContainerGetParentContainerFromItem(feature_item) ;
  zMapAssert(set_group) ;
  set_data = g_object_get_data(G_OBJECT(set_group), ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;    


  /* Need to delete the feature from the feature set and from the hash and destroy the
   * canvas item....NOTE this is very order dependent. */

  /* I'm still not sure this is all correct.  
   * canvasItemDestroyCB has a FToIRemove! 
   */

  /* Firstly remove from the FToI hash... */
  if (zmapWindowFToIRemoveFeature(zmap_window->context_to_item,
				  set_data->strand, set_data->frame, feature))
    {
      /* check the feature is in featureset. */
      if(zMapFeatureSetFindFeature(feature_set, feature))
        {
          double x1, x2, y1, y2;
          /* remove the item from the focus items list. */
          zmapWindowFocusRemoveFocusItem(zmap_window->focus, feature_item);

          if(zmapWindowMarkIsSet(zmap_window->mark) && 
             feature_item == zmapWindowMarkGetItem(zmap_window->mark) &&
             zmapWindowMarkGetWorldRange(zmap_window->mark, &x1, &y1, &x2, &y2))
            {
              zmapWindowMarkSetWorldRange(zmap_window->mark, x1, y1, x2, y2);
            }

          /* destroy the canvas item...this will invoke canvasItemDestroyCB() */
          gtk_object_destroy(GTK_OBJECT(feature_item)) ;

          if(destroy_feature)
            /* destroy the feature... deletes record in the featureset. */
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
 *  */
ZMapStrand zmapWindowFeatureStrand(ZMapFeature feature)
{
  ZMapStrand strand = ZMAPSTRAND_FORWARD ;
  ZMapFeatureTypeStyle style = feature->style ;


  if (!(zMapStyleIsStrandSpecific(style))
      || (feature->strand == ZMAPSTRAND_FORWARD || feature->strand == ZMAPSTRAND_NONE))
    strand = ZMAPSTRAND_FORWARD ;
  else
    strand = ZMAPSTRAND_REVERSE ;

    return strand ;
}

void zmapWindowFeatureFactoryInit(ZMapWindow window)
{
  ZMapWindowFToIFactoryProductionTeamStruct factory_helpers = {NULL};

  factory_helpers.feature_size_request = factoryFeatureSizeReq;
  factory_helpers.item_created         = factoryItemCreated;
  factory_helpers.top_item_created     = factoryTopItemCreated;

  zmapWindowFToIFactorySetup(window->item_factory, window->config.feature_line_width,
                             &factory_helpers, (gpointer)window);

  return ;
}


/* Called to draw each individual feature. */
FooCanvasItem *zmapWindowFeatureDraw(ZMapWindow window, FooCanvasGroup *set_group, ZMapFeature feature)
{
  FooCanvasItem *new_feature = NULL ;
  ZMapWindowItemFeatureSetData set_data ;
  ZMapFeatureTypeStyle style ;
  ZMapFeatureContext context ;
  ZMapFeatureAlignment alignment ;
  ZMapFeatureBlock block ;
  ZMapFeatureSet set ;
  FooCanvasGroup *column_group ;


  set_data = g_object_get_data(G_OBJECT(set_group), ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;
  

  /* Get the styles table from the column and look for the features style.... */
  if (!(style = zmapWindowStyleTableFind(set_data->style_table, zMapStyleGetUniqueID(feature->style))))
    {
      style = zMapFeatureStyleCopy(feature->style) ;  
      zmapWindowStyleTableAdd(set_data->style_table, style) ;
    }



  /* Users will often not want to see what is on the reverse strand, style specifies what should
   * be shown. */
  if (zMapStyleIsStrandSpecific(style)
      && (feature->strand == ZMAPSTRAND_REVERSE && !zMapStyleIsShowReverseStrand(style)))
    {
      return NULL ;
    }

  /* These should be parameters, rather than continually fetch them, caller will almost certainly know these! */
  set = (ZMapFeatureSet)zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_FEATURESET) ;
  block = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)set, ZMAPFEATURE_STRUCT_BLOCK) ;
  alignment = (ZMapFeatureAlignment)zMapFeatureGetParentGroup((ZMapFeatureAny)block, ZMAPFEATURE_STRUCT_ALIGN) ;
  context = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)alignment, ZMAPFEATURE_STRUCT_CONTEXT) ;

 /* Retrieve the parent col. group/id. */
  column_group = zmapWindowContainerGetFeatures(set_group) ;

  new_feature = zmapWindowFToIFactoryRunSingle(window->item_factory,
                                               set_group, 
                                               context, 
                                               alignment, 
                                               block, 
                                               set, 
                                               feature);
  return new_feature;
}


/* This is not a great function...a bit dumb really...but hard to make smarter as caller
 * must make a number of decisions which we can't.... */
char *zmapWindowFeatureSetDescription(GQuark feature_set_id, ZMapFeatureTypeStyle style)
{
  char *description = NULL ;

  zMapAssert(feature_set_id && style) ;

  description = zMapStyleGetGFFSource(style) ;

  description = g_strdup_printf("%s  :  %s%s%s", (char *)g_quark_to_string(feature_set_id),
				description ? "\"" : "",
				description ? description : "<no description available>",
				description ? "\"" : "") ;

  return description ;
}

gboolean zMapWindowGetDNAStatus(ZMapWindow window)
{
  gboolean drawable = FALSE;
  BlockHasDNAStruct dna = {0};

  /* We just need one of the blocks to have DNA.
   * This enables us to turn on this button as we
   * can't have half sensitivity.  Any block which
   * doesn't have DNA creates a warning for the user
   * to complain about.
   */

  /* check for style too. */
  /* sometimes we don't have a featrue_context ... ODD! */
  if(window->feature_context &&
     zMapFindStyle(window->feature_context->styles, 
                   zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME)))
    {
      zMapFeatureContextExecute((ZMapFeatureAny)(window->feature_context), 
                                ZMAPFEATURE_STRUCT_BLOCK, 
                                oneBlockHasDNA, 
                                &dna);

      drawable = dna.exists;
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







/* 
 *                       Internal functions.
 */

/* Callback for destroy of feature items... */
static gboolean canvasItemDestroyCB(FooCanvasItem *feature_item, gpointer data)
{
  gboolean event_handled = FALSE ;			    /* Make sure any other callbacks also get run. */
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapWindow window = (ZMapWindowStruct*)data ;
  gboolean status ;
  FooCanvasGroup *set_group ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(feature_item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;

  /* We may not have a parent group if we are being called as a result of a
   * parent/superparent being destroyed. In this case our parent pointer is
   * set to NULL before we are called. */
  if ((set_group = zmapWindowContainerGetParentContainerFromItem(feature_item)))
    {
      ZMapFeature feature ;
      ZMapWindowItemFeatureSetData set_data ;

      /* These should go in container some time.... */
      set_data = g_object_get_data(G_OBJECT(set_group), ITEM_FEATURE_SET_DATA) ;
      zMapAssert(set_data) ;

      feature = g_object_get_data(G_OBJECT(feature_item), ITEM_FEATURE_DATA) ;
      zMapAssert(feature && zMapFeatureIsValid((ZMapFeatureAny)feature)) ;

      /* Remove this feature item from the hash. */
      status = zmapWindowFToIRemoveFeature(window->context_to_item,
					   set_data->strand, set_data->frame, feature) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Can't do this at the moment because the hash may have been invalidated....
       * this is an order bug in the code that revcomps and will be for the 3 frame
       * stuff.... */

      zMapAssert(status) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    }


  if (item_feature_type == ITEM_FEATURE_SIMPLE)
    {
      /* Check to see if there is an entry in long items for this feature.... */
      zmapWindowLongItemRemove(window->long_items, feature_item) ;  /* Ignore boolean result. */

      zmapWindowFocusRemoveFocusItem(window->focus, feature_item);
    }
  else /* ITEM_FEATURE_PARENT */
    {
      FooCanvasGroup *group = FOO_CANVAS_GROUP(feature_item) ;

      /* Now do children of compound object.... */
      g_list_foreach(group->item_list, cleanUpFeatureCB, window) ;
    }

  return event_handled ;
}



static void cleanUpFeatureCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *feature_item = FOO_CANVAS_ITEM(data) ;
  ZMapWindow window = (ZMapWindow)user_data ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapWindowItemFeature item_data ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(feature_item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_BOUNDING_BOX || item_feature_type == ITEM_FEATURE_CHILD) ;

  item_data = g_object_get_data(G_OBJECT(feature_item), ITEM_SUBFEATURE_DATA) ;
  g_free(item_data) ;

  /* Check to see if there is an entry in long items for this feature.... */
  zmapWindowLongItemRemove(window->long_items, feature_item) ;  /* Ignore boolean result. */

  zmapWindowFocusRemoveFocusItem(window->focus, feature_item);

  return ;
}

static void featureCopySelectedItem(ZMapFeature feature_in, 
                                    ZMapFeature feature_out, 
                                    FooCanvasItem *selected)
{
  ZMapWindowItemFeature item_feature_data;
  ZMapSpanStruct span = {0};
  ZMapAlignBlockStruct alignBlock = {0};

  if(feature_in && feature_out)
    memcpy(feature_out, feature_in, sizeof(ZMapFeatureStruct));
  else
    zMapAssertNotReached();

  if((item_feature_data = g_object_get_data(G_OBJECT(selected), ITEM_SUBFEATURE_DATA)))
    {
      if(feature_out->type == ZMAPFEATURE_TRANSCRIPT)
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
      else if(feature_out->type == ZMAPFEATURE_ALIGNMENT && 
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


/* Callback for any events that happen on individual canvas items.
 * 
 * NOTE that we can use a static for the double click detection because the
 * GUI runs in a single thread and because the user cannot double click on
 * more than one window at a time....;-)
 *  */
static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow window = (ZMapWindowStruct*)data ;
  ZMapFeature feature ;
  static guint32 last_but_press = 0 ;			    /* Used for double clicks... */

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
      {
	GdkEventButton *but_event = (GdkEventButton *)event ;
	ZMapWindowItemFeatureType item_feature_type ;
	FooCanvasItem *real_item ;
	FooCanvasItem *highlight_item ;
        GList *focus_items;

        /* Get the feature attached to the item, checking that its type is valid */
	feature = (ZMapFeature)g_object_get_data(G_OBJECT(item), 
                                                 ITEM_FEATURE_DATA);  
	item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							      ITEM_FEATURE_TYPE)) ;
	zMapAssert(feature) ;
	zMapAssert(item_feature_type    == ITEM_FEATURE_SIMPLE
		   || item_feature_type == ITEM_FEATURE_CHILD
		   || item_feature_type == ITEM_FEATURE_BOUNDING_BOX) ;

	/* If its type is bounding box then we don't want the that to influence
         * highlighting... so we look up the real item, or just take the current */
	if (item_feature_type == ITEM_FEATURE_BOUNDING_BOX)
	  real_item = zMapWindowFindFeatureItemByItem(window, item) ;
	else
	  real_item = item ;

	if (but_event->type == GDK_BUTTON_PRESS)
	  {
	    /* Double clicks occur within 250 milliseconds so we ignore the second button
	     * press generated by clicks but catch the button2 press (see below). */
	    if (but_event->time - last_but_press > 250)
	      {
                GdkModifierType shift_mask = GDK_SHIFT_MASK, 
                  control_mask             = GDK_CONTROL_MASK, 
                  shift_control_mask       = GDK_SHIFT_MASK | GDK_CONTROL_MASK,
                  unwanted_masks           = GDK_LOCK_MASK | GDK_MOD2_MASK | GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK,
                  locks_mask;

                /* In order to make the modifier only checks work we
                 * need to OR in the unwanted masks that might be on.
                 * This includes the shift lock and num lock.
                 * Depending on the setup of X these might be mapped
                 * to other things which is why MODs 2-5 are included
                 * This in theory should include the new (since 2.10)
                 * GDK_SUPER_MASK, GDK_HYPER_MASK and GDK_META_MASK */
                if((locks_mask = (but_event->state & unwanted_masks)))
                  {
                    shift_mask         |= locks_mask;
                    control_mask       |= locks_mask;
                    shift_control_mask |= locks_mask;
                  }

		/* Button 1 and 3 are handled, 2 is left for a general handler which could be
		 * the root handler. */
		if (but_event->button == 1 || but_event->button == 3)
		  {
		    gboolean replace_highlight = TRUE, highlight_same_names = TRUE, externally_handled = FALSE;
		    FooCanvasItem *highlight_item;

		    if (zMapGUITestModifiersOnly(but_event, shift_mask))
		      {
			/* multiple selections */
			highlight_item = FOO_CANVAS_ITEM(zmapWindowItemGetTrueItem(real_item)) ;

			if (zmapWindowFocusIsItemInHotColumn(window->focus, highlight_item))
                          {
                            replace_highlight = FALSE ;
                            externally_handled = zmapWindowUpdateXRemoteData(window, (ZMapFeatureAny)feature, "multiple_select", highlight_item);
                          }
                        else
                          externally_handled = zmapWindowUpdateXRemoteData(window, (ZMapFeatureAny)feature, "single_select", highlight_item);
		      }
		    else if (zMapGUITestModifiersOnly(but_event, control_mask))
		      {
                        ZMapFeatureStruct feature_copy = {};

			/* sub selections */
			highlight_item = real_item ;
			highlight_same_names = FALSE ;

                        /* monkey around to get feature_copy to be the right correct data */
                        featureCopySelectedItem(feature, &feature_copy,
                                                highlight_item);
                        externally_handled = zmapWindowUpdateXRemoteData(window, (ZMapFeatureAny)(&feature_copy), "single_select", highlight_item);
		      }
		    else if (zMapGUITestModifiersOnly(but_event, shift_control_mask))
		      {
                        ZMapFeatureStruct feature_copy = {};

			/* sub selections + multiple selections */
			highlight_item = real_item ;
			highlight_same_names = FALSE ;

                        /* monkey around to get feature_copy to be the right correct data */
                        featureCopySelectedItem(feature, &feature_copy,
                                                highlight_item);

			if (zmapWindowFocusIsItemInHotColumn(window->focus, highlight_item))
                          {
                            replace_highlight = FALSE ;
                            externally_handled = zmapWindowUpdateXRemoteData(window, (ZMapFeatureAny)(&feature_copy), "multiple_select", highlight_item);
                          }
                        else
                          externally_handled = zmapWindowUpdateXRemoteData(window, (ZMapFeatureAny)(&feature_copy), "single_select", highlight_item);
		      }
		    else
		      {
                        /* single select */
			highlight_item = FOO_CANVAS_ITEM(zmapWindowItemGetTrueItem(real_item)) ;
                        externally_handled = zmapWindowUpdateXRemoteData(window, (ZMapFeatureAny)feature, "single_select", highlight_item);
		      }

		    /* Pass information about the object clicked on back to the application. */
		    zMapWindowUpdateInfoPanel(window, feature, real_item, highlight_item,
					      replace_highlight, highlight_same_names) ;

		    if (but_event->button == 3)
		      {
			/* Pop up an item menu. */
			zmapMakeItemMenu(but_event, window, highlight_item) ;
		      }
		  }
	      }

	    last_but_press = but_event->time ;

	    event_handled = TRUE ;
	  }
	else
	  {
	    /* Handle second click of a double click. */
	    if (but_event->button == 1)
	      {
		gboolean externally_handled = FALSE ;

		highlight_item = FOO_CANVAS_ITEM(zmapWindowItemGetTrueItem(real_item)) ;
		
                if(!(externally_handled = zmapWindowUpdateXRemoteData(window, (ZMapFeatureAny)feature, "edit", highlight_item)))
                  {
		    if (feature->type == ZMAPFEATURE_ALIGNMENT)
                      {
                        if((focus_items = zmapWindowFocusGetFocusItems(window->focus)))
                          {
                            zmapWindowListWindowCreate(window, focus_items, 
                                                       (char *)g_quark_to_string(feature->parent->original_id), 
                                                       zmapWindowFocusGetHotItem(window->focus), FALSE) ;
                            g_list_free(focus_items);
                            focus_items = NULL;
                          }
                      }
		    else
		      {
			zmapWindowFeatureShow(window, highlight_item) ;
		      }
                  }

		event_handled = TRUE ;
	      }
	  }
	break ;
      }
    default:
      {
	/* By default we _don't_ handle events. */
	event_handled = FALSE ;

	break ;
      }
    }

  return event_handled ;
}

static gboolean dnaItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  static DNAItemEventStruct 
    dna_item_event       = {NULL};
  ZMapWindow window      = (ZMapWindowStruct*)data;
  gboolean event_handled = FALSE;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *button = (GdkEventButton *)event;

        if(event->type == GDK_BUTTON_RELEASE)
          {
            if(dna_item_event.selected)
              {
                ZMapWindowItemTextContext context;
                FooCanvasItem *context_owner;
                if((context = g_object_get_data(G_OBJECT(dna_item_event.dna_item), 
                                                ITEM_FEATURE_TEXT_DATA)))
                  context_owner = FOO_CANVAS_ITEM( dna_item_event.dna_item );
                else if((context = g_object_get_data(G_OBJECT(dna_item_event.dna_item->parent), 
                                                     ITEM_FEATURE_TEXT_DATA)))
                  context_owner = FOO_CANVAS_ITEM( dna_item_event.dna_item->parent );

                if(context)
                  {
                    ZMapWindowSelectStruct select = {0};
                    ZMapWindowItemFeatureType item_feature_type;
                    ZMapFeature feature;
                    int start, end, tmp;
                    int dna_start, dna_end;
		    int display_start, display_end;
                    /* Copy/Paste from zMapWindowUpdateInfoPanel is quite high, 
                     * but */
                    select.type = ZMAPWINDOW_SELECT_SINGLE;
                    
                    start = dna_item_event.origin_index;
                    zmapWindowItemTextWorldToIndex(context, context_owner, button->x, button->y, &end);
                    
                    if(start > end){ tmp = start; start = end; end = tmp; }
                    
                    feature = (ZMapFeature)g_object_get_data(G_OBJECT(context_owner), 
                                                             ITEM_FEATURE_DATA);  
                    item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(context_owner),
                                                                          ITEM_FEATURE_TYPE)) ;
                    zMapAssert(feature) ;
                    /* zMapAssert(item_feature_type == SOMETHING_GOOD); */
                    
                    if(feature->type == ZMAPFEATURE_PEP_SEQUENCE)
                      {
			int window_origin;
                        int frame = zmapWindowFeatureFrame(feature);
                        dna_start = start;
                        dna_end   = end;
                        /* correct these for the frame */
                        start    -= 3 - frame;
                        frame--;
                        end      += frame;

			display_start = zmapWindowCoordToDisplay(window, start);
			display_end   = zmapWindowCoordToDisplay(window, end)  ;

                        select.feature_desc.sub_feature_start  = g_strdup_printf("%d", display_start);
                        select.feature_desc.sub_feature_end    = g_strdup_printf("%d", display_end);
                        select.feature_desc.sub_feature_length = g_strdup_printf("%d", end - start + 1);

                        start = dna_start / 3;
                        end   = dna_end   / 3;

			/* zmapWindowCoordToDisplay() doesn't work for protein coord space, whether this is useful though.... */
			window_origin = (window->origin == window->min_coord ? window->min_coord : window->origin / 3);
			display_start = start - (window_origin - 1);
			display_end   = end   - (window_origin - 1);
                      }
		    else
		      {
			display_start = zmapWindowCoordToDisplay(window, start);
			display_end   = zmapWindowCoordToDisplay(window, end);
		      }

                    select.feature_desc.feature_start  = g_strdup_printf("%d", display_start);
                    select.feature_desc.feature_end    = g_strdup_printf("%d", display_end);
                    select.feature_desc.feature_length = g_strdup_printf("%d", end - start + 1);

                    /* What does fMap do for the info?  */
                    /* Sequence:"Em:BC043419.2"    166314 167858 (1545)  vertebrate_mRNA 96.9 (1 - 1547) Em:BC043419.2 */
                    select.feature_desc.feature_name   = (char *)g_quark_to_string(feature->original_id);
                    
                    /* update the clipboard and the info panel */
                    (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;
                    
                    /* We wait until here to do this so we are only setting the
                     * clipboard text once. i.e. for this window. And so that we have
                     * updated the focus object correctly. */
                    if(feature->type == ZMAPFEATURE_RAW_SEQUENCE)
                      {
                        char *dna_string, *seq_name;
                        dna_string = zMapFeatureGetDNA((ZMapFeatureAny)feature, start, end, FALSE);
                        seq_name = g_strdup_printf("%d-%d", display_start, display_end);
                        select.secondary_text = zMapFASTAString(ZMAPFASTA_SEQTYPE_DNA, 
                                                                seq_name, "DNA", NULL, 
                                                                end - start + 1,
                                                                dna_string);
                        g_free(seq_name);
                        zMapWindowUtilsSetClipboard(window, select.secondary_text);
                      }
                    else if(feature->type == ZMAPFEATURE_PEP_SEQUENCE)
                      {
                        ZMapPeptide translation;
                        char *dna_string, *seq_name;
                        int frame = zmapWindowFeatureFrame(feature);
                        /* highlight the corresponding DNA
                         * Can we do this? */
                        /* Find the dna feature? */
                        /* From feature->frame get the dna indices */
                        start = dna_start - (3 - frame); /* aa -> dna (first base of codon) */
                        frame--;
                        end   = dna_end + frame; /* aa -> dna (last base of codon)  */
                        
                        /* From those highlight! */
                        zmapWindowItemHighlightDNARegion(window, item, start, end);

                        dna_string  = zMapFeatureGetDNA((ZMapFeatureAny)feature, start, end, FALSE);
                        seq_name    = g_strdup_printf("%d-%d (%d-%d)", dna_start / 3, dna_end / 3, start, end);
                        translation = zMapPeptideCreate(seq_name, NULL, dna_string, NULL, TRUE);
                        select.secondary_text = zMapFASTAString(ZMAPFASTA_SEQTYPE_AA, 
                                                                seq_name, "Protein", NULL, 
                                                                zMapPeptideLength(translation),
                                                                zMapPeptideSequence(translation));
                        g_free(seq_name);
                        zMapPeptideDestroy(translation);
                        zMapWindowUtilsSetClipboard(window, select.secondary_text);
                        g_free(select.feature_desc.sub_feature_start);
                        g_free(select.feature_desc.sub_feature_end);
                        g_free(select.feature_desc.sub_feature_length);
                      }
                    
                    g_free(select.feature_desc.feature_start);
                    g_free(select.feature_desc.feature_end);
                    g_free(select.feature_desc.feature_length);
                    
                    dna_item_event.selected = FALSE;
                    dna_item_event.origin_index = 0;
                    event_handled = TRUE;
                  }
              }
          }
        else if(button->button == 1)
          {
            FooCanvasGroup *container_parent;
            ZMapWindowOverlay overlay;
            if((container_parent = zmapWindowContainerGetParentContainerFromItem(item)) &&
               (overlay          = zmapWindowContainerGetData(container_parent, "OVERLAY_MANAGER")))
              {
                FooCanvasGroup *container_underlay;
                
                if((container_underlay = zmapWindowContainerGetUnderlays(container_parent)))
                  {
                    ZMapFeature feature;
                    GdkColor* background;

                    feature = (ZMapFeature)g_object_get_data(G_OBJECT(item), 
                                                             ITEM_FEATURE_DATA);  
                    zMapAssert(feature);

                    zmapWindowOverlayUnmaskAll(overlay);
                    
                    if(window->highlights_set.item)
                      zmapWindowOverlaySetGdkColorFromGdkColor(overlay, &(window->colour_item_highlight));
                    else if(zMapStyleGetColours(feature->style, ZMAPSTYLE_COLOURTARGET_NORMAL, 
                                                ZMAPSTYLE_COLOURTYPE_SELECTED,
                                                &background, NULL, NULL))
                      zmapWindowOverlaySetGdkColorFromGdkColor(overlay, background);

                    zmapWindowOverlaySetLimitItem(overlay, item->parent);
                    zmapWindowOverlaySetSubject(overlay, item);
                    
                    dna_item_event.dna_item     = item;
                    dna_item_event.event        = event;
                    dna_item_event.origin_index = 0;
                    dna_item_event.selected     = TRUE;

                    zmapWindowOverlayMaskFull(overlay, event_to_char_cell_coords, &dna_item_event);

                    event_handled = TRUE;
                  }
              }
          }
        else if(button->button == 3)
          {
            
          }
      }
      break;
    case GDK_MOTION_NOTIFY:
      {
        if(dna_item_event.selected)
          {
            FooCanvasGroup *container_parent;
            ZMapWindowOverlay overlay;
            FooCanvasGroup *container_underlay;

            if((container_parent   = zmapWindowContainerGetParentContainerFromItem(item)) &&
               (overlay            = zmapWindowContainerGetData(container_parent, "OVERLAY_MANAGER")) &&
               (container_underlay = zmapWindowContainerGetUnderlays(container_parent)))
              {
                zmapWindowOverlayUnmaskAll(overlay);
                
                zmapWindowOverlaySetLimitItem(overlay, item->parent);
                zmapWindowOverlaySetSubject(overlay, item);

                dna_item_event.dna_item = item;
                dna_item_event.event    = event;
                
                zmapWindowOverlayMaskFull(overlay, event_to_char_cell_coords, &dna_item_event);
              }
            event_handled = TRUE;
          }
      }
      break;
    case GDK_LEAVE_NOTIFY:
      {
	event_handled = FALSE ;
        break;
      }
    case GDK_ENTER_NOTIFY:
      {
	event_handled = FALSE ;
        break;
      }
    default:
      {
	/* By default we _don't_handle events. */
	event_handled = FALSE ;
	break ;
      }
    }

  return event_handled ;
}

static gboolean event_to_char_cell_coords(FooCanvasPoints **points_out, 
                                          FooCanvasItem    *subject, 
                                          gpointer          user_data)
{
  DNAItemEvent full_data  = (DNAItemEvent)user_data;
  ZMapWindowItemTextContext context;
  GdkEventButton *button  = (GdkEventButton *)full_data->event;
  GdkEventMotion *motion  = (GdkEventMotion *)full_data->event;
  FooCanvasPoints *points = NULL;
  FooCanvasItem *context_owner = NULL;
  double first[ITEMTEXT_CHAR_BOUND_COUNT], last[ITEMTEXT_CHAR_BOUND_COUNT];
  int index;
  gboolean set = FALSE;

  points = foo_canvas_points_new(8);

  /* event x and y should be world coords... */

  /* We can get here either from the text item or from the group.
   * Either way we need the context, but we also need to know which
   * item owns it so that the w2i call in ItemTextWorldToIndex 
   * returns sane coords. */
  if((context = g_object_get_data(G_OBJECT(full_data->dna_item), ITEM_FEATURE_TEXT_DATA)))
    context_owner = FOO_CANVAS_ITEM( full_data->dna_item );
  else if((context = g_object_get_data(G_OBJECT(full_data->dna_item->parent), ITEM_FEATURE_TEXT_DATA)))
    context_owner = FOO_CANVAS_ITEM( full_data->dna_item->parent );

  if(context)
    {
      int index1, index2;
      /* From the x,y of event, get the text index */
      switch(full_data->event->type)
        {
        case GDK_BUTTON_RELEASE:
        case GDK_BUTTON_PRESS:
          zmapWindowItemTextWorldToIndex(context, context_owner, button->x, button->y, &index);
          break;
        case GDK_MOTION_NOTIFY:
          zmapWindowItemTextWorldToIndex(context, context_owner, motion->x, motion->y, &index);
          break;
        default:
          zMapAssertNotReached();
          break;
        }

      if(full_data->origin_index == 0)
        full_data->origin_index = index;

      if(full_data->origin_index < index)
        {
          index1 = full_data->origin_index;
          index2 = index;
        }
      else
        {
          index2 = full_data->origin_index;
          index1 = index;
        }

      /* From the text indices, get the bounds of that char */
      zmapWindowItemTextIndexGetBounds(context, index1, &first[0]);
      zmapWindowItemTextIndexGetBounds(context, index2, &last[0]);
      /* From the bounds, calculate the area of the overlay */
      zmapWindowItemTextCharBounds2OverlayPoints(context, &first[0],
                                                 &last[0], points);
      /* set the points */
      if(points_out)
        {
          *points_out = points;
          /* record such */
          set = TRUE;
        }
    }

  return set;
}


/* Build the menu for a feature item. */
void zmapMakeItemMenu(GdkEventButton *button_event, ZMapWindow window, FooCanvasItem *item)
{
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


  /* Some parts of the menu are feature type specific so retrieve the feature item info
   * from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;

  {
    GHashTable *style_table = NULL;
    FooCanvasGroup *column_group =  NULL ;
    ZMapWindowItemFeatureSetData set_data = NULL;

    
    column_group = zmapWindowContainerGetParentContainerFromItem(item) ;
    set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA);
    zMapAssert(set_data);
    
    style_table = set_data->style_table;
    /* Get the styles table from the column and look for the features style.... */
    style = zmapWindowStyleTableFind(style_table, zMapStyleGetUniqueID(feature->style)) ;
  }


  menu_title = zMapFeatureName((ZMapFeatureAny)feature) ;

  /* Call back stuff.... */
  menu_data = g_new0(ItemMenuCBDataStruct, 1) ;
  menu_data->item_cb = TRUE ;
  menu_data->window = window ;
  menu_data->item = item ;

  /* Make up the menu. */

  /* Feature ops. */
  menu_sets = g_list_append(menu_sets, makeMenuFeatureOps(NULL, NULL, menu_data)) ;

  if (feature->url)
    menu_sets = g_list_append(menu_sets, makeMenuURL(NULL, NULL, menu_data)) ;

  if (feature->type == ZMAPFEATURE_ALIGNMENT)
    {
      menu_sets = g_list_append(menu_sets, makeMenuPfetchOps(NULL, NULL, menu_data)) ;

      if (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
	menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuProteinHomol(NULL, NULL, menu_data)) ;
      else
	menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNAHomol(NULL, NULL, menu_data)) ;
    }

  /* DNA/Peptide ops. */
  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
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
						   zMapStyleGetOverlapMode(style))) ;

  menu_sets = g_list_append(menu_sets, separator) ;

  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDumpOps(NULL, NULL, menu_data)) ;

  menu_sets = g_list_append(menu_sets, separator) ;

  menu_sets = g_list_append(menu_sets, makeMenuGeneralOps(NULL, NULL, menu_data)) ;

  zMapGUIMakeMenu(menu_title, menu_sets, button_event) ;

  return ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void makeTextItemMenu(GdkEventButton *button_event, ZMapWindow window, FooCanvasItem *item)
{
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


  /* Some parts of the menu are feature type specific so retrieve the feature item info
   * from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;

  style = zmapWindowItemGetStyle(item) ;
  zMapAssert(style) ;

  menu_title = zMapFeatureName((ZMapFeatureAny)feature) ;

  /* Call back stuff.... */
  menu_data = g_new0(ItemMenuCBDataStruct, 1) ;
  menu_data->item_cb = TRUE ;
  menu_data->window = window ;
  menu_data->item = item ;

  /* Make up the menu. */

  /* The select all, None, Copy... */
  menu_sets = g_list_append(menu_sets, makeMenuTextSelectOps(NULL, NULL, menu_data)) ;

  menu_sets = g_list_append(menu_sets, separator) ;


  /* General */
  menu_sets = g_list_append(menu_sets, makeMenuGeneralOps(NULL, NULL, menu_data)) ;

  /* DNA Search ????????????????? */

  zMapGUIMakeMenu(menu_title, menu_sets, button_event) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static void itemMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeature feature ;

  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(menu_data->item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;

  switch (menu_item_id)
    {
    case 1:
      {
        GList *list = NULL;
	ZMapStrand set_strand ;
	ZMapFrame set_frame ;
	gboolean result ;

	result = zmapWindowItemGetStrandFrame(menu_data->item, &set_strand, &set_frame) ;
	zMapAssert(result) ;

        list = zmapWindowFToIFindItemSetFull(menu_data->window->context_to_item, 
					     feature->parent->parent->parent->unique_id,
					     feature->parent->parent->unique_id,
					     feature->parent->unique_id,
					     zMapFeatureStrand2Str(set_strand),
					     zMapFeatureFrame2Str(set_frame),
					     g_quark_from_string("*"), NULL, NULL) ;

        zmapWindowListWindowCreate(menu_data->window, list, 
                                   (char *)g_quark_to_string(feature->parent->original_id), 
                                   menu_data->item, TRUE) ;
	break ;
      }
    case 9:
      {
        GList *list = NULL;
	ZMapStrand set_strand ;
	ZMapFrame set_frame ;
	gboolean result ;

	result = zmapWindowItemGetStrandFrame(menu_data->item, &set_strand, &set_frame) ;
	zMapAssert(result) ;

	list = zmapWindowFToIFindSameNameItems(menu_data->window->context_to_item,
					       zMapFeatureStrand2Str(set_strand), zMapFeatureFrame2Str(set_frame),
					       feature) ;

        zmapWindowListWindowCreate(menu_data->window, list, 
                                   (char *)g_quark_to_string(feature->parent->original_id), 
                                   menu_data->item, FALSE) ;
	break ;
      }
    case 2:
      {
	zmapWindowFeatureShow(menu_data->window, menu_data->item) ;

	break ;
      }
    case 7:
      zmapWindowMarkSetItem(menu_data->window->mark, menu_data->item) ;

      break ;
    case 3:
      zmapWindowCreateSearchWindow(menu_data->window, menu_data->item) ;

      break ;
    case 4:
      pfetchEntry(menu_data->window, (char *)g_quark_to_string(feature->original_id)) ;
      break ;

    case 5:
      zmapWindowCreateSequenceSearchWindow(menu_data->window, menu_data->item, ZMAPSEQUENCE_DNA) ;

      break ;

    case 11:
      zmapWindowCreateSequenceSearchWindow(menu_data->window, menu_data->item, ZMAPSEQUENCE_PEPTIDE) ;

      break ;

    case 6:
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
      {ZMAPGUI_MENU_NORMAL, "Show Feature Details",   2, itemMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Set Feature for Bump",   7, itemMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     0, NULL,       NULL}
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
      {ZMAPGUI_MENU_NORMAL, "Pfetch this feature",    4, itemMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     0, NULL,       NULL}
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
      {ZMAPGUI_MENU_NORMAL, "List All Column Features",       1, itemMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "List This Name Column Features", 9, itemMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Feature Search Window",          3, itemMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "DNA Search Window",              5, itemMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Peptide Search Window",          11, itemMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                               0, NULL,       NULL}
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
      {ZMAPGUI_MENU_NORMAL, "URL",                    6, itemMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     0, NULL,       NULL}
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


static void pfetchEntry(ZMapWindow window, char *sequence_name)
{
  char *argv[4] = {NULL};
  char *title = NULL;
  PFetchData pfetch_data = g_new0(PFetchDataStruct, 1);
  GError *error = NULL;
  gboolean result = FALSE;

  argv[0] = "pfetch";
  argv[1] = "-F";
  argv[2] = g_strdup_printf("%s", sequence_name);


  /* create the dialog with an initial value... This should get overwritten later. */
  title = g_strdup_printf("pfetch: \"%s\"", sequence_name) ;
  pfetch_data->dialog = zMapGUIShowTextFull(title, "pfetching...\n", FALSE, &(pfetch_data->text_buffer));

  pfetch_data->widget_destroy_handler_id = g_signal_connect(pfetch_data->dialog, 
							    "destroy", 
							    G_CALLBACK(handle_dialog_close), 
							    pfetch_data);

  if(!(result = zMapUtilsSpawnAsyncWithPipes(&argv[0], NULL, pfetch_stdout_io_func, 
					     pfetch_stderr_io_func, pfetch_data, NULL, 
					     &error, NULL, NULL)))
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "%s %s", PFETCH_FAILED_PREFIX, error->message);      
    }

  g_free(title);
  g_free(argv[2]);

  return ;
}

static void free_pfetch_data(PFetchData pfetch_data)
{
  if (pfetch_data->widget_destroy_handler_id != 0)
    g_signal_handler_disconnect(pfetch_data->dialog, 
				pfetch_data->widget_destroy_handler_id);

  if (!(pfetch_data->got_response))
    {
      /* some versions of pfetch erroneously return nothing if they fail to find an entry. */
      char *no_response = "No output returned !";
      if(pfetch_data->text_buffer != NULL)
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(pfetch_data->text_buffer), no_response, strlen(no_response));
      else
	zMapShowMsg(ZMAP_MSG_WARNING, "%s %s", PFETCH_FAILED_PREFIX, no_response);
    }
    
  pfetch_data->dialog = NULL ;
  pfetch_data->text_buffer = NULL ;

  g_free(pfetch_data);

  return ;
}

/* A GIOFunc */
static gboolean pfetch_stdout_io_func(GIOChannel *source, GIOCondition cond, gpointer user_data)
{
  PFetchData pfetch_data = (PFetchData)user_data;
  gboolean call_again = FALSE;

  if(cond & G_IO_IN)		/* there is data to be read */
    {
      GIOStatus status;
      gchar text[PFETCH_READ_SIZE] = {0};
      gsize actual_read = 0;
      GError *error = NULL;

      if((status = g_io_channel_read_chars(source, &text[0], PFETCH_READ_SIZE, 
					   &actual_read, &error)) == G_IO_STATUS_NORMAL)
	{
	  if(pfetch_data->text_buffer == NULL)
	    {
	      free_pfetch_data(pfetch_data);
	      call_again = FALSE;
	    }
	  else
	    {
	      if(actual_read > 0)
		{
		  if(!pfetch_data->got_response) /* clear the buffer the first time... */
		    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(pfetch_data->text_buffer), "", 0);
		  gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(pfetch_data->text_buffer), text, actual_read);
		  pfetch_data->got_response = TRUE;
		}
		
	      call_again = TRUE;
	    }
	}	  
      else
	{
	  switch(error->code)
	    {
	      /* Derived from errno */
	    case G_IO_CHANNEL_ERROR_FBIG:
	    case G_IO_CHANNEL_ERROR_INVAL:
	    case G_IO_CHANNEL_ERROR_IO:
	    case G_IO_CHANNEL_ERROR_ISDIR:
	    case G_IO_CHANNEL_ERROR_NOSPC:
	    case G_IO_CHANNEL_ERROR_NXIO:
	    case G_IO_CHANNEL_ERROR_OVERFLOW:
	    case G_IO_CHANNEL_ERROR_PIPE:
	      break;
	      /* Other */
	    case G_IO_CHANNEL_ERROR_FAILED:
	    default:
	      break;
	    }

	  zMapShowMsg(ZMAP_MSG_WARNING, "Error reading from pfetch pipe: %s", ((error && error->message) ? error->message : "UNKNOWN ERROR"));
	}

    }
  else if(cond & G_IO_HUP)	/* hung up, connection broken */
    {
      free_pfetch_data(pfetch_data);
      //zMapShowMsg(ZMAP_MSG_INFORMATION, "%s", "G_IO_HUP");
    }
  else if(cond & G_IO_ERR)	/* error condition */
    {
      zMapShowMsg(ZMAP_MSG_INFORMATION, "%s", "G_IO_ERR");
     
    }
  else if(cond & G_IO_NVAL)	/* invalid request, file descriptor not open */
    {
      zMapShowMsg(ZMAP_MSG_INFORMATION, "%s", "G_IO_NVAL");      
    }

  return call_again;
}

static gboolean pfetch_stderr_io_func(GIOChannel *source, GIOCondition cond, gpointer user_data)
{
  PFetchData pfetch_data = (PFetchData)user_data;
  gboolean call_again = FALSE;

  if(cond & G_IO_IN)
    {
      GIOStatus status;
      gchar text[PFETCH_READ_SIZE] = {0};
      gsize actual_read = 0;
      GError *error = NULL;

      if((status = g_io_channel_read_chars(source, &text[0], PFETCH_READ_SIZE, 
					   &actual_read, &error)) == G_IO_STATUS_NORMAL)
	{
	  if(pfetch_data->text_buffer == NULL)
	    call_again = FALSE;
	  else
	    {
	      if(actual_read > 0)
		{
		  if(!pfetch_data->got_response) /* clear the buffer the first time... */
		    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(pfetch_data->text_buffer), "PFetch error:\n", 14);
		  gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(pfetch_data->text_buffer), text, actual_read);
		  pfetch_data->got_response = TRUE;
		}
	      call_again = TRUE;
	    }
	}	  
    }

  return call_again;
}

/* GtkObject destroy signal handler */
static void handle_dialog_close(GtkWidget *dialog, gpointer user_data)
{
  PFetchData pfetch_data = (PFetchData)user_data;
  pfetch_data->text_buffer = NULL;
  pfetch_data->widget_destroy_handler_id = 0; /* can we get this more than once? */
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
  return TRUE;
}

static void factoryItemCreated(FooCanvasItem            *new_item,
                               ZMapWindowItemFeatureType new_item_type,
                               ZMapFeature               full_feature,
                               ZMapWindowItemFeature     sub_feature,
                               double                    new_item_y1,
                               double                    new_item_y2,
                               gpointer                  handler_data)
{
  /* some items, e.g. dna, have their event handling messed up if we include the general handler. */
  if((full_feature->type == ZMAPFEATURE_RAW_SEQUENCE) ||
     (full_feature->type == ZMAPFEATURE_PEP_SEQUENCE))
    g_signal_connect(GTK_OBJECT(new_item), "event",
                     GTK_SIGNAL_FUNC(dnaItemEventCB), handler_data) ;
  else
    g_signal_connect(GTK_OBJECT(new_item), "event",
                     GTK_SIGNAL_FUNC(canvasItemEventCB), handler_data) ;
    
  return ;
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

  if(feature->type == ZMAPFEATURE_RAW_SEQUENCE ||
     feature->type == ZMAPFEATURE_PEP_SEQUENCE)
    {
      ZMapWindow window = (ZMapWindow)handler_data;
      double x1, x2;
      
      points_array_inout[1] = points_array_inout[3] = x1 = x2 = 0.0;

      zmapWindowScrollRegionTool(window, 
                                 &x1, &(points_array_inout[1]), 
                                 &x2, &(points_array_inout[3]));
    }
  else
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
  ZMapWindowItemFeatureSetData set_data;
  ZMapFeatureSet feature_set ;
  gboolean column_is_empty = FALSE;
  FooCanvasGroup *container_features;
  ZMapStyleOverlapMode bump_mode;

  feature_set = zmapWindowContainerGetData(feature_group, ITEM_FEATURE_DATA) ;
  zMapAssert(feature_set) ;

          
  container_features = zmapWindowContainerGetFeatures(feature_group);
          
  column_is_empty = !(container_features->item_list);
            
  /* This function will add the new feature to the hash. */
  new_feature = zmapWindowFeatureDraw(window, FOO_CANVAS_GROUP(feature_group), feature) ;

  if (bump_col)
    {
      if ((set_data = g_object_get_data(G_OBJECT(feature_group), ITEM_FEATURE_SET_DATA)))
	{
	  if((bump_mode = zMapStyleGetOverlapMode(set_data->style)) != ZMAPOVERLAP_COMPLETE)
	    {
	      zmapWindowColumnBump(FOO_CANVAS_ITEM(feature_group), bump_mode);
	    }
	}

      if (column_is_empty)
	zmapWindowColOrderPositionColumns(window);
    }


  return new_feature ;
}





