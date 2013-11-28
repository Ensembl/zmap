/*  File: zmapWindowMenus.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * Description: Code implementing the menus for sequence display.
 *
 * Exported functions: ZMap/zmapWindows.h
 *
 *-------------------------------------------------------------------
 */


/* PLEASE READ:
 *
 * This file is a unification of code that was scattered and replicated
 * in a number of files. The unification is not complete, itemMenuCB()
 * needs merging with other callbacks to remove all duplication and
 * there is further simplification to be done as there used to be
 * completely separate code to service the feature menu as opposed
 * to the column menu.
 *
 *  */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h> /* zMap_g_hash_table_nth */
#include <ZMap/zmapFASTA.h>
#include <ZMap/zmapGFF.h>
#include <ZMap/zmapPeptide.h>
#include <zmapWindow_P.h>
#include <zmapWindowCanvasItem.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowContainerFeatureSet_I.h>




/* 
 * some common menu strings, needed because cascading menus need the same string as their parent
 * menu item.
 */

/* Show Feature, DNA, Peptide */
#define FEATURE_SHOW_STR           "View or Export Feature, DNA, Peptide"

#define FEATURE_VIEW_STR           "View"
#define FEATURE_EXPORT_STR         "Export"

#define FEATURE_STR                "Feature"

#define FEATURE_DNA_STR            "Feature DNA"
#define FEATURE_DNA_EXPORT_STR     FEATURE_DNA_STR
#define FEATURE_DNA_SHOW_STR       FEATURE_DNA_STR

#define FEATURE_PEPTIDE_STR        "Feature Peptide"
#define FEATURE_PEPTIDE_EXPORT_STR FEATURE_PEPTIDE_STR
#define FEATURE_PEPTIDE_SHOW_STR   FEATURE_PEPTIDE_STR


#define CONTEXT_STR                "All Features"
#define CONTEXT_EXPORT_STR         CONTEXT_STR

#define MARK_STR                   "Marked Features"
#define MARK_EXPORT_STR            MARK_STR

#define DEVELOPER_STR              "Developer"


/* Strings/enums for invoking blixem. */
#define BLIXEM_MENU_STR            "Blixem"
#define BLIXEM_OPS_STR             BLIXEM_MENU_STR " - more options"
#define BLIXEM_READS_STR           BLIXEM_MENU_STR " paired reads data"

#define BLIXEM_DNA_STR             "DNA"
#define BLIXEM_DNAS_STR            BLIXEM_DNA_STR "s"
#define BLIXEM_AA_STR              "Protein"
#define BLIXEM_AAS_STR             BLIXEM_AA_STR "s"


/* Column bumping/state. */



/* Column configuration */
#define COLUMN_CONFIG_STR          "Column Configuration"
#define COLUMN_THIS_ONE            "Configure This Column"
#define COLUMN_ALL                 "Configure All Columns"
#define COLUMN_BUMP_OPTS           "Column Bump More Opts"
#define SCRATCH_CONFIG_STR         "Annotation Column"
#define SCRATCH_COPY_FEATURE       "Copy selected feature"
#define SCRATCH_COPY_SUBFEATURE    "Copy selected subfeature"
#define SCRATCH_UNDO               "Undo"
#define SCRATCH_REDO               "Redo"
#define SCRATCH_CLEAR              "Clear"

#define PAIRED_READS_RELATED       "Request %s paired reads"
#define PAIRED_READS_ALL           "Request all paired reads"
#define PAIRED_READS_DATA          "Request paired reads data"

#define COLUMN_COLOUR			"Edit Style"
#define COLUMN_STYLE_OPTS		"Choose Style"

/* Search/Listing menus. */
#define SEARCH_STR                 "Search or List Features and Sequence"


enum
  {
    BLIX_INVALID,
    BLIX_NONE,		/* Blixem on column with no aligns. */
    BLIX_SELECTED,	/* Blixem all matches for selected features in this column. */
    BLIX_EXPANDED,	/* selected features expanded into hidden underlying data */
    BLIX_SET,		/* Blixem all matches for all features in this column. */
    BLIX_MULTI_SETS,	/* Blixem all matches for all features in the list of columns in the blixem config file. */
    BLIX_SEQ_COVERAGE,	/* Blixem a coverage column: find the real data column */
//    BLIX_SEQ_SET		/* Blixem a paried read featureset */
  } ;

#define BLIX_SEQ		10000       /* Blixem short reads data base menu index */
#define REQUEST_SELECTED 20000	/* data related to selected featureset in column */
#define REQUEST_ALL_SEQ 20001		/* all data related to coverage featuresets in column */
#define REQUEST_SEQ	20002		/* request SR data from mark from menu */
    /* MH17 NOTE BLIX_SEQ etc is a range of values */
    /* this is a temporary implementation till we can blixem a short reads column */
    /* we assume that we have less than 10k datasets */


/* Choose which way a transcripts dna is exported... */
enum
  {
    ZMAPCDS,
    ZMAPTRANSCRIPT,
    ZMAPUNSPLICED,
    ZMAPCHOOSERANGE,
    ZMAPCDS_FILE,
    ZMAPTRANSCRIPT_FILE,
    ZMAPUNSPLICED_FILE,
    ZMAPCHOOSERANGE_FILE
} ;


enum {
  ZMAPNAV_SPLIT_FIRST_LAST_EXON,
  ZMAPNAV_SPLIT_FLE_WITH_OVERVIEW,
  ZMAPNAV_FIRST,
  ZMAPNAV_PREV,
  ZMAPNAV_NEXT,
  ZMAPNAV_LAST
};


enum
  {
    ITEM_MENU_INVALID,

    ITEM_MENU_LIST_NAMED_FEATURES,
    ITEM_MENU_LIST_ALL_FEATURES,
    ITEM_MENU_MARK_ITEM,
    ITEM_MENU_COPY_TO_SCRATCH,
    ITEM_MENU_COPY_SUBPART_TO_SCRATCH,
    ITEM_MENU_CLEAR_SCRATCH,
    ITEM_MENU_UNDO_SCRATCH,
    ITEM_MENU_REDO_SCRATCH,
    ITEM_MENU_SEARCH,
    ITEM_MENU_FEATURE_DETAILS,
    ITEM_MENU_PFETCH,
    ITEM_MENU_SEQUENCE_SEARCH_DNA,
    ITEM_MENU_SEQUENCE_SEARCH_PEPTIDE,
    ITEM_MENU_SHOW_URL_IN_BROWSER,
    ITEM_MENU_SHOW_TRANSLATION,

    ITEM_MENU_SHOW_EVIDENCE,
    ITEM_MENU_ADD_EVIDENCE,
    ITEM_MENU_SHOW_TRANSCRIPT,
    ITEM_MENU_ADD_TRANSCRIPT,

    ITEM_MENU_EXPAND,
    ITEM_MENU_CONTRACT,

    ITEM_MENU_ITEMS
  };





typedef struct
{
  char *stem;
  ZMapGUIMenuItem each_align_items;
  ZMapGUIMenuItem each_block_items;
  GArray **array;
}AlignBlockMenuStruct, *AlignBlockMenu;


typedef struct
{
  ZMapFeature feature ;

  gboolean cds ;
  gboolean spliced ;

  ZMapFeatureTypeStyle sequence_style ;

  GdkColor *non_coding ;
  GdkColor *coding ;
  GdkColor *split_5 ;
  GdkColor *split_3 ;

  GList *text_attrs ;
} MakeTextAttrStruct, *MakeTextAttr ;


/* Used from itemMenuCB to record data needed in highlightEvidenceCB(). */
typedef struct HighlightDataStructType
{
  ZMapWindow window ;
  ZMapFeatureAny feature ;
} HighlightDataStruct, *HighlightData ;





static void maskToggleMenuCB(int menu_item_id, gpointer callback_data);

static void searchListMenuCB(int menu_item_id, gpointer callback_data) ;

static void hideEvidenceMenuCB(int menu_item_id, gpointer callback_data);
static void compressMenuCB(int menu_item_id, gpointer callback_data);
static void configureMenuCB(int menu_item_id, gpointer callback_data) ;

static void colourMenuCB(int menu_item_id, gpointer callback_data);
static void setStyleCB(int menu_item_id, gpointer callback_data);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void bumpToInitialCB(int menu_item_id, gpointer callback_data);
static void unbumpAllCB(int menu_item_id, gpointer callback_data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void bumpMenuCB(int menu_item_id, gpointer callback_data) ;
static void bumpToggleMenuCB(int menu_item_id, gpointer callback_data) ;
static void markMenuCB(int menu_item_id, gpointer callback_data) ;

static void featureMenuCB(int menu_item_id, gpointer callback_data) ;
static void dnaMenuCB(int menu_item_id, gpointer callback_data) ;
static void peptideMenuCB(int menu_item_id, gpointer callback_data) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void transcriptNavMenuCB(int menu_item_id, gpointer callback_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static void exportMenuCB(int menu_item_id, gpointer callback_data) ;
static void developerMenuCB(int menu_item_id, gpointer callback_data) ;
static void requestShortReadsCB(int menu_item_id, gpointer callback_data);
static void blixemMenuCB(int menu_item_id, gpointer callback_data) ;
static char *get_menu_string(GQuark set_quark,char disguise) ;
GQuark related_column(ZMapFeatureContextMap map,GQuark fset_id) ;


/* from zmapWindowFeature.c */
static ZMapGUIMenuItem makeMenuURL(int *start_index_inout,
				   ZMapGUIMenuItemCallbackFunc callback_func,
				   gpointer callback_data) ;
static ZMapGUIMenuItem makeMenuShowTranslation(int *start_index_inout,
					       ZMapGUIMenuItemCallbackFunc callback_func,
					       gpointer callback_data);
static ZMapGUIMenuItem makeMenuPfetchOps(int *start_index_inout,
					 ZMapGUIMenuItemCallbackFunc callback_func,
					 gpointer callback_data) ;
static void itemMenuCB(int menu_item_id, gpointer callback_data) ;

static ZMapGUIMenuItem zmapWindowMakeMenuStyle(int *start_index_inout,
				       ZMapGUIMenuItemCallbackFunc callback_func,
				       gpointer callback_data, ZMapFeatureTypeStyle cur_style, ZMapStyleMode f_type);



static FooCanvasGroup *menuDataItemToColumn(FooCanvasItem *item) ;

static void exportFASTA(ZMapWindow window, ZMapFASTASeqType seq_type, char *seq, char *seq_name, int seq_len,
		      char *molecule_name, char *gene_name) ;
static void exportFeatures(ZMapWindow window, ZMapSpan span, ZMapFeatureAny feature) ;
static void exportContext(ZMapWindow window) ;

static void insertSubMenus(GString *branch_point_string,
                           ZMapGUIMenuItem sub_menus,
                           ZMapGUIMenuItem item,
                           GArray **items_array);

static ZMapFeatureContextExecuteStatus alignBlockMenusDataListForeach(GQuark key,
                                                                      gpointer data,
                                                                      gpointer user_data,
                                                                      char **error_out) ;

static gboolean getSeqColours(ZMapFeatureTypeStyle style,
			      GdkColor **non_coding_out, GdkColor **coding_out,
			      GdkColor **split_5_out, GdkColor **split_3_out) ;
static GList *getTranscriptTextAttrs(ZMapFeature feature, gboolean spliced, gboolean cds,
				     GdkColor *non_coding_out, GdkColor *coding_out,
				     GdkColor *split_5_out, GdkColor *split_3_out) ;
static void createExonTextTag(gpointer data, gpointer user_data) ;
static void offsetTextAttr(gpointer data, gpointer user_data) ;

static void highlightEvidenceCB(GList *evidence, gpointer user_data) ;







/* Set of makeMenuXXXX functions to create common subsections of menus. If you add to this
 * you should make sure you understand how to specify menu paths in the item factory style.
 * If you get it wrong then the menus will be scr*wed up.....
 *
 * The functions are defined in pairs: one to define the menu, one to handle the callback
 * actions, this is to emphasise that their indexes must be kept in step !
 *
 * NOTE HOW THE MENUS ARE DECLARED STATIC IN THE VARIOUS ROUTINES TO MAKE SURE THEY STAY
 * AROUND...OTHERWISE WE WILL HAVE TO KEEP ALLOCATING/DEALLOCATING THEM.....
 *
 * and this is also the weakness of the system, there is duplication in the code because
 * the menus in each function must persist after the function has exitted. This isn't
 * worth fixing now as we will be moving on to the better gtk UI scheme later.
 *
 */



/* Probably it would be wise to pass in the callback function, the start index for the item
 * identifier and perhaps the callback data......
 *
 * There is some mucky stuff for setting buttons etc but it's a bit unavoidable....
 *
 */

/* mh17 somehow we need this number to be unique but many of the id's
 * are enums and will overlap
 * the callbacks are specified per item so that cures one problem (one func per enum)
 * but for menu init here's a bodge up:
 */
#define ZMAPWINDOWCOLUMN_MASK 5678
#define ZMAPWINDOW_HIDE_EVIDENCE 5679
#define ZMAPWINDOW_SHOW_EVIDENCE 5680






/*
 *                          External interface
 *
 * NOTE: restructuring this lot so all menu function is in this file, this will mean
 *       moving functions around and making many static that were external.
 */



/*!
 * \brief Get the ZMapWindowContainerFeatureSet for the scratch column
 */
static ZMapWindowContainerFeatureSet getScratchContainerFeatureset(ZMapWindow window)
{
  ZMapWindowContainerFeatureSet scratch_container = NULL ;
  
  ZMapFeatureSet scratch_featureset = zmapWindowScratchGetFeatureset(window) ;

  if (scratch_featureset)
    {
      FooCanvasItem *scratch_column_item = zmapWindowFToIFindItemFull(window, window->context_to_item, 
                                                                      scratch_featureset->parent->parent->unique_id,
                                                                      scratch_featureset->parent->unique_id,
                                                                      scratch_featureset->unique_id, 
                                                                      ZMAPSTRAND_NONE, ZMAPFRAME_NONE, 0) ;
  
      ZMapWindowContainerGroup scratch_group  = zmapWindowContainerCanvasItemGetContainer(scratch_column_item) ;
      
      if (scratch_group && ZMAP_IS_CONTAINER_FEATURESET(scratch_group))
        {
          scratch_container = ZMAP_CONTAINER_FEATURESET(scratch_group) ;
        }
    }
  
  return scratch_container ;
}


/*
 * In the end this should be merged with column menu code so one function does both...
 */

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
  ZMapGUIMenuItem seq_menus ;


  /* Some parts of the menu are feature type specific so retrieve the feature item info
   * from the canvas item. */
  feature = zMapWindowCanvasItemGetFeature(item);
  if (!feature)
    return ;


  style = *feature->style;

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
  menu_data->context_map = window->context_map;		/* window has it but when we make the menu it's out of scope */

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


  if (feature->type != ZMAPSTYLE_MODE_ALIGNMENT)
    {
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuNonHomolFeature(NULL, NULL, menu_data)) ;
    }
  else if (zMapStyleIsPfetchable(style))
    {
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuBlixCommon(NULL, NULL, menu_data)) ;

      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuBlixTop(NULL, NULL, menu_data)) ;

      if (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
	{
	  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuProteinHomol(NULL, NULL, menu_data)) ;
	}
      else
	{
	  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNAHomol(NULL, NULL, menu_data)) ;
	  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNAHomolFeature(NULL, NULL, menu_data)) ;
	}
    }
  else if (zMapStyleBlixemType(style) != ZMAPSTYLE_BLIXEM_INVALID)
    {
      menu_sets = g_list_append(menu_sets,  zmapWindowMakeMenuDNAHomolFeature(NULL, NULL, menu_data)) ;
    }

  /* list all short reads data, temp access till we get wiggle plots running */
  if ((seq_menus = zmapWindowMakeMenuBlixemBAM(NULL, NULL, menu_data)))
      menu_sets = g_list_append(menu_sets, seq_menus) ;

  if (zMapStyleIsPfetchable(style))
    menu_sets = g_list_append(menu_sets, makeMenuPfetchOps(NULL, NULL, menu_data)) ;

  /* Feature ops. */
  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuFeatureOps(NULL, NULL, menu_data)) ;

  /* Show translation on forward strand features only */
  if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT && feature->strand == ZMAPSTRAND_FORWARD)
    menu_sets = g_list_append(menu_sets, makeMenuShowTranslation(NULL, NULL, menu_data));

  if (feature->url)
    menu_sets = g_list_append(menu_sets, makeMenuURL(NULL, NULL, menu_data)) ;

  menu_sets = g_list_append(menu_sets, separator) ;

  /* Scratch column ops */
  ZMapWindowContainerFeatureSet scratch_container = getScratchContainerFeatureset(window) ;
  
  if (scratch_container &&
      (scratch_container->display_state == ZMAPSTYLE_COLDISPLAY_SHOW ||
       scratch_container->display_state == ZMAPSTYLE_COLDISPLAY_SHOW))
    {
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuScratchOps(NULL, NULL, menu_data)) ;
      menu_sets = g_list_append(menu_sets, separator) ;
    }

    
  /* Big bump menu.... */
  menu_sets = g_list_append(menu_sets,
			    zmapWindowMakeMenuBump(NULL, NULL, menu_data,
						   zmapWindowContainerFeatureSetGetBumpMode(container_set))) ;

  {
	GQuark parent_id = 0;
	ZMapFeatureTypeStyle parent = NULL;

	if(!(style->is_default) && (parent_id = style->parent_id))
	{
		parent = g_hash_table_lookup(window->context_map->styles, GUINT_TO_POINTER(parent_id));
	}

	/*
	 * hacky: otterlace styles will not be defaulted
	 * any styles defined in a style cannot be edited live
	 * NOTE is we define EST_ALIGN as a default style then EST_Human etc can be edited
	 */
	if(window->edit_styles || style->is_default || (parent && parent->is_default))
		menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuStyle(NULL, NULL, menu_data, style, feature->type));
  }

  /* list all short reads data, temp access till we get wiggle plots running */
  if ((seq_menus = zmapWindowMakeMenuRequestBAM(NULL, NULL, menu_data)))
    menu_sets = g_list_append(menu_sets, seq_menus) ;

  menu_sets = g_list_append(menu_sets, separator) ;


  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuSearchListOps(NULL, NULL, menu_data)) ;

  /* Feature/DNA/Peptide view/export ops. */
  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuShowTop(NULL, NULL, menu_data)) ;
  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuShowFeature(NULL, NULL, menu_data)) ;

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

  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuExportOps(NULL, NULL, menu_data)) ;

  if (zmapWindowMarkIsSet(window->mark))
    menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuMarkExportOps(NULL, NULL, menu_data));

  zMapGUIMakeMenu(menu_title, menu_sets, button_event) ;

  g_free(menu_title) ;

  return ;
}


/* Build the background menu for a column. */
void zmapMakeColumnMenu(GdkEventButton *button_event, ZMapWindow window,
			FooCanvasItem *item,
			ZMapWindowContainerFeatureSet container_set,
			ZMapFeatureTypeStyle style_unused)
{
  static ZMapGUIMenuItemStruct separator[] =
    {
      {ZMAPGUI_MENU_SEPARATOR, NULL, 0, NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;
  char *menu_title ;
  GList *menu_sets = NULL ;
  ItemMenuCBData cbdata ;
  ZMapFeature feature;
  ZMapFeatureSet feature_set ;
  ZMapGUIMenuItem seq_menus ;


  menu_title = (char *) g_quark_to_string(container_set->original_id);

  feature_set = zmapWindowContainerFeatureSetRecoverFeatureSet(container_set) ;

  cbdata = g_new0(ItemMenuCBDataStruct, 1) ;
  cbdata->x = button_event->x ;
  cbdata->y = button_event->y ;
  cbdata->item_cb = FALSE ;
  cbdata->window = window ;
  cbdata->item = item ;
  cbdata->feature_set = feature_set ;
  cbdata->container_set = container_set;
  cbdata->context_map = window->context_map ;

  /* Make up the menu. */
  if (zMapUtilsUserIsDeveloper())
    {
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDeveloperOps(NULL, NULL, cbdata)) ;

      menu_sets = g_list_append(menu_sets, separator) ;
    }

  /* Quite a big hack actually, we judge feature type and protein/dna on the first feature
   * we find in the column..... */
  if ((feature = zMap_g_hash_table_nth(feature_set->features, 0)))
    {
      if (feature->type != ZMAPSTYLE_MODE_ALIGNMENT)
	{
	  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuNonHomolFeature(NULL, NULL, cbdata)) ;
	}
      else if (zMapStyleIsPfetchable(*feature->style))
	{
	  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuBlixColCommon(NULL, NULL, cbdata)) ;

	  if (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
	    {
	      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuProteinHomol(NULL, NULL, cbdata)) ;
	    }
	  else
	    {
	      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNAHomolFeature(NULL, NULL, cbdata)) ;
	      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNAHomol(NULL, NULL, cbdata)) ;
	    }
	}
      else if (zMapStyleBlixemType(*feature->style) != ZMAPSTYLE_BLIXEM_INVALID)
	{
	  menu_sets = g_list_append(menu_sets,  zmapWindowMakeMenuDNAHomolFeature(NULL, NULL, cbdata)) ;
	}

      if ((seq_menus = zmapWindowMakeMenuBlixemBAM(NULL, NULL, cbdata)))
	menu_sets = g_list_append(menu_sets, seq_menus) ;
    }

  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuFeatureOps(NULL, NULL, cbdata)) ;
  menu_sets = g_list_append(menu_sets, separator) ;

  /* Scratch column ops */
  ZMapWindowContainerFeatureSet scratch_container = getScratchContainerFeatureset(window) ;
  
  if (scratch_container &&
      (scratch_container->display_state == ZMAPSTYLE_COLDISPLAY_SHOW ||
       scratch_container->display_state == ZMAPSTYLE_COLDISPLAY_SHOW))
    {
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuScratchOps(NULL, NULL, cbdata)) ;
      menu_sets = g_list_append(menu_sets, separator) ;
    }
  
  menu_sets
    = g_list_append(menu_sets,
		    zmapWindowMakeMenuBump(NULL, NULL, cbdata,
					   zmapWindowContainerFeatureSetGetBumpMode((ZMapWindowContainerFeatureSet)item))) ;

  if ((seq_menus = zmapWindowMakeMenuRequestBAM(NULL, NULL, cbdata)))
    menu_sets = g_list_append(menu_sets, seq_menus);

  menu_sets = g_list_append(menu_sets, separator) ;

  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuSearchListOps(NULL, NULL, cbdata)) ;

  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuExportOps(NULL, NULL, cbdata)) ;


  zMapGUIMakeMenu(menu_title, menu_sets, button_event) ;

  return ;
}


/* Utility to add an option to the given menu */
static void addMenuItem(ZMapGUIMenuItem menu,
                        int *i,
                        int max_elements,
                        ZMapGUIMenuType item_type,
                        const char *item_text,
                        int item_id,
                        ZMapGUIMenuItemCallbackFunc callback_func,
                        gpointer callback_data)
{
  /* Check we're not trying to index out of range based on the maximum number
   * of elements we expect in this menu */
  if (*i < max_elements)
    {
      menu[*i].type = item_type;
      menu[*i].name = g_strdup(item_text);
      menu[*i].id = item_id;
      menu[*i].callback_func = callback_func;
      menu[*i].callback_data = callback_data;

      *i = *i + 1;
    }
  else
    {
      zMapWarning("%s", "Program error: tried to add menu item to out-of-range menu position") ;
    }
}


/* This is in the general menu and needs to be handled separately perhaps as the index is a global
 * one shared amongst all general menu functions...
 * NOTE HOW THE MENUS ARE DECLARED STATIC IN THE VARIOUS ROUTINES TO MAKE SURE THEY STAY
 * AROUND...OTHERWISE WE WILL HAVE TO KEEP ALLOCATING/DEALLOCATING THEM.....
 */
ZMapGUIMenuItem zmapWindowMakeMenuFeatureOps(int *start_index_inout,
					     ZMapGUIMenuItemCallbackFunc callback_func,
					     gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         NULL, NULL},

      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         NULL,       NULL}
    } ;
  int i ;
  int max_elements = 5 ;
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;

  i = 0;
  menu[i].type = ZMAPGUI_MENU_NONE;

  if (menu_data->feature)
    {
      /* add in feature options */
      addMenuItem(menu, &i, max_elements, ZMAPGUI_MENU_NORMAL, "Use Feature for Mark", ITEM_MENU_MARK_ITEM, itemMenuCB, NULL);
    }

  /* add in evidence/ transcript items option to remove existing is in column menu */
  if (menu_data->feature)
    {
      ZMapFeatureTypeStyle style = *menu_data->feature->style;

      if (!style)
	{
	  // style should be attached to the feature, but if not don't fall over
	  // new features should also have styles attached
	  zMapLogWarning("Feature menu item does not have style","");
	}
      else
	{
	  if(style->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
	    {
              addMenuItem(menu, &i, max_elements, ZMAPGUI_MENU_NORMAL, "Highlight Evidence", ITEM_MENU_SHOW_EVIDENCE, itemMenuCB, NULL);
              addMenuItem(menu, &i, max_elements, ZMAPGUI_MENU_NORMAL, "Highlight Evidence (add more)", ITEM_MENU_ADD_EVIDENCE, itemMenuCB, NULL);
	    }
	  else if (style->mode == ZMAPSTYLE_MODE_ALIGNMENT)
	    {
              addMenuItem(menu, &i, max_elements, ZMAPGUI_MENU_NORMAL, "Highlight Transcript", ITEM_MENU_SHOW_TRANSCRIPT, itemMenuCB, NULL);
              addMenuItem(menu, &i, max_elements, ZMAPGUI_MENU_NORMAL, "Highlight Transcript (add more)", ITEM_MENU_ADD_TRANSCRIPT, itemMenuCB, NULL);

	      if(menu_data->feature->children)
		{
                  addMenuItem(menu, &i, max_elements, ZMAPGUI_MENU_NORMAL, "Expand Feature", ITEM_MENU_EXPAND, itemMenuCB, NULL);
		}
	      else if(menu_data->feature->composite)
		{
                  addMenuItem(menu, &i, max_elements, ZMAPGUI_MENU_NORMAL, "Contract Features", ITEM_MENU_CONTRACT, itemMenuCB, NULL);
		}
	    }
	}
    }

  if (zmapWindowFocusHasType(menu_data->window->focus, WINDOW_FOCUS_GROUP_EVIDENCE))
    {
      addMenuItem(menu, &i, max_elements, ZMAPGUI_MENU_TOGGLEACTIVE, "Hide Evidence/ Transcript", ZMAPWINDOW_HIDE_EVIDENCE, hideEvidenceMenuCB, NULL);
    }
  else
    {
      menu[i].type = ZMAPGUI_MENU_HIDE ;
      i++ ;
    }

  menu[i].type = ZMAPGUI_MENU_NONE;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


/* Options for the scratch column */
ZMapGUIMenuItem zmapWindowMakeMenuScratchOps(int *start_index_inout,
					     ZMapGUIMenuItemCallbackFunc callback_func,
					     gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, SCRATCH_CONFIG_STR, 0, NULL, NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         NULL, NULL},

      {ZMAPGUI_MENU_NONE, NULL,                     ITEM_MENU_INVALID,         NULL,       NULL}
    } ;
  int i ;
  int max_elements = 6 ;
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;

  i = 1;
  menu[i].type = ZMAPGUI_MENU_NONE;

  if (menu_data->feature)
    {
      /* add in feature options */
      addMenuItem(menu, &i, max_elements, ZMAPGUI_MENU_NORMAL, SCRATCH_CONFIG_STR"/"SCRATCH_COPY_FEATURE, ITEM_MENU_COPY_TO_SCRATCH, itemMenuCB, NULL);
      addMenuItem(menu, &i, max_elements, ZMAPGUI_MENU_NORMAL, SCRATCH_CONFIG_STR"/"SCRATCH_COPY_SUBFEATURE, ITEM_MENU_COPY_SUBPART_TO_SCRATCH, itemMenuCB, NULL);
    }

  /* add in column options */
  addMenuItem(menu, &i, max_elements, ZMAPGUI_MENU_NORMAL, SCRATCH_CONFIG_STR"/"SCRATCH_UNDO, ITEM_MENU_UNDO_SCRATCH, itemMenuCB, NULL);
  addMenuItem(menu, &i, max_elements, ZMAPGUI_MENU_NORMAL, SCRATCH_CONFIG_STR"/"SCRATCH_REDO, ITEM_MENU_REDO_SCRATCH, itemMenuCB, NULL);
  addMenuItem(menu, &i, max_elements, ZMAPGUI_MENU_NORMAL, SCRATCH_CONFIG_STR"/"SCRATCH_CLEAR, ITEM_MENU_CLEAR_SCRATCH, itemMenuCB, NULL);

  menu[i].type = ZMAPGUI_MENU_NONE;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


static void itemMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeature feature ;


  /* Retrieve the feature item info from the canvas item. */
  feature = zmapWindowItemGetFeature(menu_data->item);

  switch (menu_item_id)
    {
    case ITEM_MENU_MARK_ITEM:
      zmapWindowMarkSetItem(menu_data->window->mark, menu_data->item) ;
      break;

    case ITEM_MENU_COPY_TO_SCRATCH:
      zmapWindowScratchCopyFeature(menu_data->window, feature, menu_data->item, menu_data->x, menu_data->y, FALSE);
      break ;

    case ITEM_MENU_COPY_SUBPART_TO_SCRATCH:
      zmapWindowScratchCopyFeature(menu_data->window, feature, menu_data->item, menu_data->x, menu_data->y, TRUE);
      break ;

    case ITEM_MENU_UNDO_SCRATCH:
      zmapWindowScratchUndo(menu_data->window);
      break ;

    case ITEM_MENU_REDO_SCRATCH:
      zmapWindowScratchRedo(menu_data->window);
      break ;

    case ITEM_MENU_CLEAR_SCRATCH:
      zmapWindowScratchClear(menu_data->window);
      break ;

    case ITEM_MENU_SEARCH:
      zmapWindowCreateSearchWindow(menu_data->window,
				   NULL, NULL,
				   menu_data->window->context_map,
				   menu_data->item) ;

      break ;
    case ITEM_MENU_PFETCH:
      zmapWindowPfetchEntry(menu_data->window, (char *)g_quark_to_string(feature->original_id)) ;
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
	    zMapWarning("Error: %s", error->message) ;

	    g_error_free(error) ;
	  }
	break ;
      }
    case ITEM_MENU_SHOW_TRANSLATION:
      {
	zmapWindowItemShowTranslation(menu_data->window, menu_data->item) ;

	break;
      }

    case ITEM_MENU_SHOW_EVIDENCE:
    case ITEM_MENU_ADD_EVIDENCE:
    case ITEM_MENU_SHOW_TRANSCRIPT:       /* XML formats are different for evidence and transcripts */
    case ITEM_MENU_ADD_TRANSCRIPT:        /* but we handle that in zmapWindowFeatureGetEvidence() */
      {
	// show evidence for a transcript
        ZMapWindowFocus focus = menu_data->window->focus ;

        if (focus)
          {
	    HighlightData highlight_data ;

            ZMapFeatureAny any = (ZMapFeatureAny)menu_data->feature ; /* our transcript */

            /* clear any existing highlight */
            if (menu_item_id != ITEM_MENU_ADD_EVIDENCE)
	      zmapWindowFocusResetType(focus,WINDOW_FOCUS_GROUP_EVIDENCE) ;

            /* add the transcript to the evidence group */
            zmapWindowFocusAddItemType(menu_data->window->focus,
				       menu_data->item, menu_data->feature, WINDOW_FOCUS_GROUP_EVIDENCE) ;

	    /* Now call xremote to request the list of evidence features from otterlace. */
	    highlight_data = g_new0(HighlightDataStruct, 1) ;
	    highlight_data->window = menu_data->window ;
	    highlight_data->feature = any ;

            zmapWindowFeatureGetEvidence(menu_data->window, menu_data->feature,
					 highlightEvidenceCB, highlight_data) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
            /* search for the features named in the list and find thier canvas items */

            for ( ; evidence ; evidence = evidence->next)
	      {
		GList *items,*items_free;
		GQuark wildcard = g_quark_from_string("*");
		char *feature_name;
		GQuark feature_search_id;

		/* need to add a * to the end to match strand and frame name mangling */
		feature_name = g_strdup_printf("%s*",g_quark_to_string(GPOINTER_TO_UINT(evidence->data)));
		feature_name = zMapFeatureCanonName(feature_name);    /* done in situ */
		feature_search_id = g_quark_from_string(feature_name);

		/* catch NULL names due to ' getting escaped int &apos; */
		if(feature_search_id == wildcard)
		  continue;

		items_free = zmapWindowFToIFindItemSetFull(menu_data->window,
							   menu_data->window->context_to_item,
							   any->parent->parent->parent->unique_id,
							   any->parent->parent->unique_id,
							   wildcard,0,"*","*",
							   feature_search_id,
							   NULL,NULL);


		/* zMapLogWarning("evidence %s returns %d features", feature_name, g_list_length(items_free));*/
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
             * in this call it's irrelevant so don't set the hot item, pass NULL instead
             */

            zmapWindowFocusAddItemsType(menu_data->window->focus, evidence_items,
                  NULL /* menu_data->item */, WINDOW_FOCUS_GROUP_EVIDENCE);


            g_list_free(evidence);
            g_list_free(evidence_items);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          }

	break;
      }

    case ITEM_MENU_EXPAND:
      {
	zmapWindowFeatureExpand(menu_data->window, menu_data->item, menu_data->feature, menu_data->container_set);

	break;
      }

    case ITEM_MENU_CONTRACT:
      {
	zmapWindowFeatureContract(menu_data->window, menu_data->item, menu_data->feature, menu_data->container_set);

	break;
      }

    default:
      {
        zMapWarning("%s", "Unexpected menu callback action\n") ;
        zMapWarnIfReached() ;
	break ;
      }
    }

  g_free(menu_data) ;

  return ;
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


ZMapGUIMenuItem zmapWindowMakeMenuSearchListOps(int *start_index_inout,
						ZMapGUIMenuItemCallbackFunc callback_func,
						gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, SEARCH_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, SEARCH_STR"/""DNA Search Window",              ITEM_MENU_SEQUENCE_SEARCH_DNA, searchListMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, SEARCH_STR"/""Peptide Search Window",          ITEM_MENU_SEQUENCE_SEARCH_PEPTIDE, searchListMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, SEARCH_STR"/""Feature Search Window",          ITEM_MENU_SEARCH,              searchListMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, SEARCH_STR"/""List All Column Features",       ITEM_MENU_LIST_ALL_FEATURES,   searchListMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, SEARCH_STR"/""List Column Features With This Name", ITEM_MENU_LIST_NAMED_FEATURES, searchListMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL, ITEM_MENU_INVALID, NULL, NULL}
    } ;
  enum {NAMED_FEATURE_INDEX = 5} ;
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeatureAny feature_any ;

  /* If user clicked on a feature set then "List with name" option is not relevant. */
  feature_any = zmapWindowItemGetFeatureAny(menu_data->item);
  if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
    menu[NAMED_FEATURE_INDEX].type = ZMAPGUI_MENU_NONE ;
  else
    menu[NAMED_FEATURE_INDEX].type = ZMAPGUI_MENU_NORMAL ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}




/* Make the Bump, Hide, Masked, Mark and Compress subpart of the menu. */
ZMapGUIMenuItem zmapWindowMakeMenuBump(int *start_index_inout,
				       ZMapGUIMenuItemCallbackFunc callback_func,
				       gpointer callback_data, ZMapStyleBumpMode curr_bump)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_TOGGLE, "Column Bump", ZMAPBUMP_UNBUMP, bumpToggleMenuCB, NULL, "B"},
      {ZMAPGUI_MENU_NORMAL, "Column Hide", ZMAPWINDOWCOLUMN_HIDE, configureMenuCB,  NULL},
      {ZMAPGUI_MENU_TOGGLE, "Show Masked Features", ZMAPWINDOWCOLUMN_MASK,  maskToggleMenuCB, NULL, NULL},
      {ZMAPGUI_MENU_TOGGLE, "Toggle Mark", 0, markMenuCB, NULL, "M"},
      {ZMAPGUI_MENU_TOGGLE, "Compress Columns", ZMAPWINDOW_COMPRESS_MARK, compressMenuCB, NULL, "c"},

      {ZMAPGUI_MENU_SEPARATOR, NULL, 0, NULL, NULL, NULL},

      {ZMAPGUI_MENU_BRANCH, COLUMN_CONFIG_STR, 0, NULL, NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, COLUMN_CONFIG_STR"/"COLUMN_THIS_ONE, ZMAPWINDOWCOLUMN_CONFIGURE, configureMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, COLUMN_CONFIG_STR"/"COLUMN_ALL, ZMAPWINDOWCOLUMN_CONFIGURE_ALL, configureMenuCB, NULL},
      {ZMAPGUI_MENU_BRANCH, COLUMN_CONFIG_STR"/"COLUMN_BUMP_OPTS, 0, NULL, NULL},
      {ZMAPGUI_MENU_RADIO, NULL, ZMAPBUMP_OVERLAP, bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO, NULL, ZMAPBUMP_ALTERNATING, bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO, NULL, ZMAPBUMP_ALL, bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO, NULL, ZMAPBUMP_UNBUMP, bumpMenuCB, NULL},

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Remove when we are sure no one wants it... */
      {ZMAPGUI_MENU_NORMAL, "Unbump All Columns",                     0,                              unbumpAllCB, NULL},
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}  // menu terminates on id = 0 in one loop below
    } ;
  enum {MENU_INDEX_BUMP = 0, MENU_INDEX_MARK = 3,  MENU_INDEX_COMPRESS = 4} ;
							    /* Keep in step with menu[] positions. */
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data;
  static gboolean menu_set = FALSE ;
#if NOT_USED
  static int ind_hide_evidence = -1;
#endif
  static int ind_mask = -1;
  ZMapGUIMenuItem menu_item ;
  static gboolean compress = FALSE ;
  ZMapWindowContainerGroup column_container, block_container;
  FooCanvasGroup *column_group =  NULL;


  /* Set the initial toggle button correctly....make sure this stays in step with the array above.
   * NOTE logic, this button is either "no bump" or "Name + No Bump", the latter should be
   * selectable whatever.... */

  /* Dynamically set various options in the menu text. */
  if (!menu_set)
    {
      ZMapGUIMenuItem tmp = menu ;

      while ((tmp->type))
	{
	  if (tmp->type == ZMAPGUI_MENU_RADIO)
	    tmp->name = g_strdup_printf("%s/%s/%s",
					COLUMN_CONFIG_STR, COLUMN_BUMP_OPTS, zmapStyleBumpMode2ShortText(tmp->id)) ;

	  if (tmp->id == ZMAPWINDOWCOLUMN_MASK)
	    ind_mask = tmp - menu;

	  tmp++ ;
	}

      menu_set = TRUE ;
    }

  /* Set main bump toggle menu item. */
  menu_item = &(menu[MENU_INDEX_BUMP]) ;
  if (curr_bump != ZMAPBUMP_UNBUMP)
    {
      menu_item->type = ZMAPGUI_MENU_TOGGLEACTIVE ;
      menu_item->id = curr_bump ;
    }
  else
    {
      menu_item->type = ZMAPGUI_MENU_TOGGLE ;
      menu_item->id = ZMAPBUMP_UNBUMP ;
    }

  /* Set "Mark" toggle item. */
  menu_item = &(menu[MENU_INDEX_MARK]) ;
  if (zmapWindowMarkIsSet(menu_data->window->mark))
    {
      menu_item->type = ZMAPGUI_MENU_TOGGLEACTIVE ;
    }
  else
    {
      menu_item->type = ZMAPGUI_MENU_TOGGLE ;
    }

  /* Set "Compress" toggle item. */
  menu_item = &(menu[MENU_INDEX_COMPRESS]) ;
  column_container = zmapWindowContainerCanvasItemGetContainer(menu_data->item) ;
  column_group = (FooCanvasGroup *)column_container;
  block_container = zmapWindowContainerUtilsGetParentLevel(column_container, ZMAPCONTAINER_LEVEL_BLOCK) ;
  if (zmapWindowContainerBlockIsCompressedColumn((ZMapWindowContainerBlock)block_container))
    {
      compress = TRUE ;
      menu_item->type = ZMAPGUI_MENU_TOGGLEACTIVE ;
    }
  else
    {
      compress = FALSE ;
      menu_item->type = ZMAPGUI_MENU_TOGGLE ;
    }


  // set toggle state of masked features for this column
  if (ind_mask >= 0)
    {
      menu_item = &(menu[ind_mask]) ;

      // set toggle state of masked features for this column
      // hide the menu item if it's not maskable
      menu_item->type = ZMAPGUI_MENU_TOGGLE ;

      if (!menu_data->container_set->maskable
	  || !menu_data->window->highlights_set.masked
	  || !zMapWindowFocusCacheGetSelectedColours(WINDOW_FOCUS_GROUP_MASKED, NULL, NULL))
        menu_item->type = ZMAPGUI_MENU_HIDE;
      else if (!menu_data->container_set->masked)
        menu_item->type = ZMAPGUI_MENU_TOGGLEACTIVE ;
    }


  /* Unset any previously active bump sub-menu radio button.... */
  menu_item = &(menu[0]) ;
  while (menu_item->type != ZMAPGUI_MENU_NONE)
    {
      if (menu_item->type == ZMAPGUI_MENU_RADIOACTIVE)
	{
	  menu_item->type = ZMAPGUI_MENU_RADIO ;
	  break ;
	}

      menu_item++ ;
    }

  /* Set any new active bump sub-menu radio button... */
  menu_item = &(menu[0]) ;
  while (menu_item->type != ZMAPGUI_MENU_NONE)
    {
      if (menu_item->type == ZMAPGUI_MENU_RADIO && menu_item->id == zMapStylePatchBumpMode(curr_bump))
	{
	  menu_item->type = ZMAPGUI_MENU_RADIOACTIVE ;
	  break ;
	}

      menu_item++ ;
    }

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


/* sort into order of style mode then alpha by name but put current style at the front */
gint style_menu_sort(gconstpointer a, gconstpointer b, gpointer data)
{
	ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle) data;
	ZMapFeatureTypeStyle sa = (ZMapFeatureTypeStyle) a, sb = (ZMapFeatureTypeStyle) b;
	const char *na, *nb;

	if(sa->mode != sb->mode)
	{
		if(sa->mode == style->mode)
			return -1;
		if(sb->mode == style->mode)
			return  1;
		if(sa->mode < sb->mode)
			return -1;
		return 1;
	}

	na = g_quark_to_string(sa->unique_id);
	nb = g_quark_to_string(sb->unique_id);

	return strcmp(na,nb);
}


/* is a style comapatble with a feature type: need to avoid crashes due to wrong data */
gboolean style_is_compatable(ZMapFeatureTypeStyle style, ZMapStyleMode f_type)
{
	if(f_type == style->mode)
		return TRUE;

	switch(f_type)
	{
	case ZMAPSTYLE_MODE_BASIC:
		if(style->mode == ZMAPSTYLE_MODE_GRAPH)
			return TRUE;
		return FALSE;

	case ZMAPSTYLE_MODE_GRAPH:
	case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
	case ZMAPSTYLE_MODE_GLYPH:
		if(style->mode == ZMAPSTYLE_MODE_BASIC)
			return TRUE;
		return FALSE;

	case ZMAPSTYLE_MODE_ALIGNMENT:
	case ZMAPSTYLE_MODE_TRANSCRIPT:
	case ZMAPSTYLE_MODE_SEQUENCE:
	case ZMAPSTYLE_MODE_TEXT:
	default:
		return FALSE;
	}
}


static ZMapGUIMenuItem zmapWindowMakeMenuStyle(int *start_index_inout,
				       ZMapGUIMenuItemCallbackFunc callback_func,
				       gpointer callback_data, ZMapFeatureTypeStyle cur_style, ZMapStyleMode f_type)
{
  /* get a list of featuresets from the window's context_map */
  static ZMapGUIMenuItem menu = NULL;
  static int n_menu = 0;
  ItemMenuCBData cbdata  = (ItemMenuCBData)callback_data;
  ZMapGUIMenuItem m;
  GHashTable *styles = cbdata->window->context_map->styles;
  GList *style_list, *sl;
  int n_styles;
  int i = 0;

  zMap_g_hash_table_get_data(&style_list,styles);
  style_list = g_list_sort_with_data(style_list, style_menu_sort, (gpointer) cur_style);
  n_styles = g_list_length(style_list);		/* max possible, will never be reached */

  if (!n_styles)
    return NULL;


  if(n_menu < n_styles + N_STYLE_MODE + 1)
    {
      /* as this derives from config data read on creating the view
       * it will not change unless we reconfig and open another view
       * alloc enough for all views
       */
      if(menu)
	{
	  for(m = menu; m->type != ZMAPGUI_MENU_NONE ;m++)
	    g_free(m->name);

	  g_free(menu);
	}

	n_menu = n_styles + N_STYLE_MODE + 10;
      menu = g_new0(ZMapGUIMenuItemStruct, n_menu);

    }

  m = menu;

  m->type = ZMAPGUI_MENU_NORMAL;
  m->name = g_strdup(COLUMN_CONFIG_STR"/"COLUMN_COLOUR);
  m->id = 0;
  m->callback_func = colourMenuCB;
  m++;

	/* add sub menu */
  m->type = ZMAPGUI_MENU_BRANCH;
  m->name = g_strdup(COLUMN_CONFIG_STR"/"COLUMN_STYLE_OPTS);
  m->id = 0;
  m->callback_func = NULL;
  m++;

  for( i = 0, sl = style_list;i < n_styles; i++, sl = sl->next)
    {
      char *name;
	char *mode = "";
	ZMapFeatureTypeStyle s = (ZMapFeatureTypeStyle) sl->data;

	if(!style_is_compatable(s,f_type))
		continue;

      m->type = ZMAPGUI_MENU_NORMAL;

	name = get_menu_string(s->original_id, '-');

	if(s->mode != cur_style->mode)
		mode = (char *) zmapStyleMode2ShortText(s->mode);

	m->name = g_strdup_printf(COLUMN_CONFIG_STR"/"COLUMN_STYLE_OPTS"/%s%s%s", mode, *mode ? "/" : "", name);
	g_free(name);

	m->id = s->unique_id;
	m->callback_func = setStyleCB;
	m++;
    }

  if(m <= menu + 3)	/* empty sub_menu or choice of current */
	  m = menu + 1;
  m->type = ZMAPGUI_MENU_NONE;
  m->name = NULL;


  /* this overrides data in the menus as given in the args, but index and func are always NULL */
  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu;
}


/*
 *     Following routines set up the feature, dna, peptide and context view or export functions.
 */


/* Make the top level Show entry. */
ZMapGUIMenuItem zmapWindowMakeMenuShowTop(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, FEATURE_SHOW_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


/* Make top entry of View submenu. */
ZMapGUIMenuItem zmapWindowMakeMenuViewTop(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, FEATURE_SHOW_STR"/"FEATURE_VIEW_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


/* View feature. */
ZMapGUIMenuItem zmapWindowMakeMenuShowFeature(int *start_index_inout,
					      ZMapGUIMenuItemCallbackFunc callback_func,
					      gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_VIEW_STR"/"FEATURE_STR, 0, featureMenuCB, NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

/* View dna for non-transcripts. */
ZMapGUIMenuItem zmapWindowMakeMenuDNAFeatureAny(int *start_index_inout,
						ZMapGUIMenuItemCallbackFunc callback_func,
						gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_VIEW_STR"/"FEATURE_DNA_SHOW_STR, ZMAPUNSPLICED, dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


ZMapGUIMenuItem zmapWindowMakeMenuDNATranscript(int *start_index_inout,
						ZMapGUIMenuItemCallbackFunc callback_func,
						gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, FEATURE_SHOW_STR"/"FEATURE_VIEW_STR"/"FEATURE_DNA_SHOW_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_VIEW_STR"/"FEATURE_DNA_SHOW_STR"/CDS",                    ZMAPCDS,           dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_VIEW_STR"/"FEATURE_DNA_SHOW_STR"/transcript",             ZMAPTRANSCRIPT,    dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_VIEW_STR"/"FEATURE_DNA_SHOW_STR"/unspliced",              ZMAPUNSPLICED,     dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_VIEW_STR"/"FEATURE_DNA_SHOW_STR"/with flanking sequence", ZMAPCHOOSERANGE,   dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


ZMapGUIMenuItem zmapWindowMakeMenuPeptide(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
#ifdef RDS_REMOVED_TICKET_50781
      {ZMAPGUI_MENU_BRANCH, "_"FEATURE_PEPTIDE_SHOW_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_PEPTIDE_SHOW_STR"/CDS",                    ZMAPCDS,           peptideMenuCB, NULL},
#endif /* RDS_REMOVED_TICKET_50781 */

      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_VIEW_STR"/"FEATURE_PEPTIDE_SHOW_STR,                    ZMAPCDS,           peptideMenuCB, NULL},

#ifdef RDS_REMOVED_TICKET_50781
      {ZMAPGUI_MENU_NORMAL, FEATURE_PEPTIDE_SHOW_STR"/transcript",             ZMAPTRANSCRIPT,    peptideMenuCB, NULL},
#endif /* RDS_REMOVED_TICKET_50781 */
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}




/* Make top entry of Export submenu. */
ZMapGUIMenuItem zmapWindowMakeMenuExportTop(int *start_index_inout,
					    ZMapGUIMenuItemCallbackFunc callback_func,
					    gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


ZMapGUIMenuItem zmapWindowMakeMenuDNAFeatureAnyFile(int *start_index_inout,
						    ZMapGUIMenuItemCallbackFunc callback_func,
						    gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"FEATURE_DNA_EXPORT_STR,               ZMAPUNSPLICED_FILE,     dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


ZMapGUIMenuItem zmapWindowMakeMenuDNATranscriptFile(int *start_index_inout,
						    ZMapGUIMenuItemCallbackFunc callback_func,
						    gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"FEATURE_DNA_EXPORT_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"FEATURE_DNA_EXPORT_STR"/CDS",                    ZMAPCDS_FILE,           dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"FEATURE_DNA_EXPORT_STR"/transcript",             ZMAPTRANSCRIPT_FILE,    dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"FEATURE_DNA_EXPORT_STR"/unspliced",              ZMAPUNSPLICED_FILE,     dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"FEATURE_DNA_EXPORT_STR"/with flanking sequence", ZMAPCHOOSERANGE_FILE,   dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


ZMapGUIMenuItem zmapWindowMakeMenuPeptideFile(int *start_index_inout,
					      ZMapGUIMenuItemCallbackFunc callback_func,
					      gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"FEATURE_PEPTIDE_EXPORT_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"FEATURE_PEPTIDE_EXPORT_STR"/CDS",             ZMAPCDS_FILE,           peptideMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"FEATURE_PEPTIDE_EXPORT_STR"/transcript",      ZMAPTRANSCRIPT_FILE,    peptideMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


ZMapGUIMenuItem zmapWindowMakeMenuExportOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"CONTEXT_EXPORT_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"CONTEXT_EXPORT_STR"/DNA", 1, exportMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"CONTEXT_EXPORT_STR"/Features", 2, exportMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"CONTEXT_EXPORT_STR"/Context", 3, exportMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


ZMapGUIMenuItem zmapWindowMakeMenuMarkExportOps(int *start_index_inout,
					      ZMapGUIMenuItemCallbackFunc callback_func,
					      gpointer callback_data)
{
  /* DNA AND CONTEXT SEEM NOT TO BE IMPLEMENTED..... */

  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"MARK_EXPORT_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"MARK_EXPORT_STR"/DNA", 1, exportMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"MARK_EXPORT_STR"/Features", 12, exportMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_SHOW_STR"/"FEATURE_EXPORT_STR"/"MARK_EXPORT_STR"/Context", 3, exportMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL               , 0, NULL, NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}



static void featureMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;

  zmapWindowFeatureShow(menu_data->window, menu_data->item) ;

  return ;
}

static void dnaMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeature feature ;
  gboolean spliced, cds ;
  char *dna ;
  ZMapFeatureContext context ;
  char *seq_name, *molecule_type = NULL, *gene_name = NULL ;
  int seq_len ;
  int user_start = 0, user_end = 0 ;

  /* Retrieve feature selected by user. */
  feature = zMapWindowCanvasItemGetFeature(menu_data->item) ;
  user_start = feature->x1 ;
  user_end = feature->x2 ;

  context = menu_data->window->feature_context ;

  seq_name = (char *)g_quark_to_string(context->original_id) ;

  spliced = cds = FALSE ;
  switch (menu_item_id)
    {
    case ZMAPCDS:
    case ZMAPCDS_FILE:
      {
	spliced = cds = TRUE ;

	break ;
      }
    case ZMAPTRANSCRIPT:
    case ZMAPTRANSCRIPT_FILE:
      {
	spliced = TRUE ;

	break ;
      }
    case ZMAPUNSPLICED:
    case ZMAPUNSPLICED_FILE:
      {
	break ;
      }
    case ZMAPCHOOSERANGE:
    case ZMAPCHOOSERANGE_FILE:
      {
	break ;
      }
    default:
      {
        zMapWarnIfReached() ;
	break ;
      }
    }

  if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      molecule_type = "DNA" ;
      gene_name = (char *)g_quark_to_string(feature->original_id) ;
    }

  if (menu_item_id == ZMAPCHOOSERANGE || menu_item_id == ZMAPCHOOSERANGE_FILE)
    {
      ZMapWindowDialogType dialog_type ;

      if (menu_item_id == ZMAPCHOOSERANGE)
	dialog_type = ZMAP_DIALOG_SHOW ;
      else
	dialog_type = ZMAP_DIALOG_EXPORT ;

      dna = zmapWindowDNAChoose(menu_data->window, menu_data->item, dialog_type, &user_start, &user_end) ;
    }
  else if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      dna = zMapFeatureGetTranscriptDNA(feature, spliced, cds) ;
    }
  else
    {
      dna = zMapFeatureGetFeatureDNA(feature) ;
    }


  if (!dna)
    {
      zMapWarning("%s", "No DNA available") ;
    }
  else
    {
      seq_len = strlen(dna) ;

      if (menu_item_id == ZMAPCDS || menu_item_id == ZMAPTRANSCRIPT || menu_item_id == ZMAPUNSPLICED
	  || menu_item_id == ZMAPCHOOSERANGE)
	{
	  GList *text_attrs = NULL ;
	  char *dna_fasta ;
	  char *window_title ;
	  ZMapFeatureTypeStyle style ;
	  GdkColor *non_coding, *coding, *split_5, *split_3 ;

	  window_title = g_strdup_printf("%s%s%s",
					 seq_name,
					 gene_name ? ":" : "",
					 gene_name ? gene_name : "") ;

	  dna_fasta = zMapFASTAString(ZMAPFASTA_SEQTYPE_DNA, seq_name, molecule_type, gene_name, seq_len, dna) ;

	  /* Transcript annotation only supported if whole transcript shown, add annotation of
	   * partial transcript as an enhancement if/when users ask for it. */
	  if (feature->type == ZMAPSTYLE_MODE_TRANSCRIPT
	      && (user_start <= feature->x1 && user_end >= feature->x2)
	      && ((style = zMapFindStyle(menu_data->window->context_map->styles,
					 zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME)))
		  && getSeqColours(style, &non_coding, &coding, &split_5, &split_3)))
	    {
	      char *dna_fasta_title ;
	      int title_len ;

	      text_attrs = getTranscriptTextAttrs(feature, spliced, cds,
						  non_coding, coding, split_5, split_3) ;

	      /* Find out length of FASTA title so we can offset for it. */
	      dna_fasta_title = zMapFASTATitle(ZMAPFASTA_SEQTYPE_DNA, seq_name, molecule_type, gene_name, seq_len) ;
	      title_len = strlen(dna_fasta_title) - 1 ;	    /* Get rid of newline in length. */

	      /* If user chose a DNA range then offset for that too. */
	      if (user_start)
		user_start = feature->x1 - user_start ;
	      title_len += user_start ;

	      /* Now offset all the exons to take account of title length. */
	      g_list_foreach(text_attrs, offsetTextAttr, GINT_TO_POINTER(title_len)) ;
	    }

	  zMapGUIShowTextFull(window_title, dna_fasta, FALSE, text_attrs, NULL) ;

	  g_free(window_title) ;
	  g_free(dna_fasta) ;
	}
      else
	{
	  exportFASTA(menu_data->window, ZMAPFASTA_SEQTYPE_DNA, dna, seq_name, seq_len, molecule_type, gene_name) ;
	}

      g_free(dna) ;
    }

  g_free(menu_data) ;

  return ;
}

/* If we had a more generalised "sequence" object that could be with dna or peptide we
 * could merge this routine with the dnamenucb...much more elegant.... */
static void peptideMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeature feature ;
  gboolean spliced, cds ;
  char *dna ;
  ZMapPeptide peptide ;
  ZMapFeatureContext context ;
  char *seq_name, *molecule_type = NULL, *gene_name = NULL ;
  int pep_length, start_incr = 0 ;


  feature = zMapWindowCanvasItemGetFeature(menu_data->item) ;
  if (feature->type != ZMAPSTYLE_MODE_TRANSCRIPT) 
    return ;

  context = menu_data->window->feature_context ;

  seq_name = (char *)g_quark_to_string(context->original_id) ;

  spliced = cds = FALSE ;
  switch (menu_item_id)
    {
    case ZMAPCDS:
    case ZMAPCDS_FILE:
      {
	spliced = cds = TRUE ;

	break ;
      }
    case ZMAPTRANSCRIPT:
    case ZMAPTRANSCRIPT_FILE:
      {
	spliced = TRUE ;

	break ;
      }
    default:
      zMapWarnIfReached() ;
      break ;
    }

  molecule_type = "Protein" ;
  gene_name = (char *)g_quark_to_string(feature->original_id) ;

  if ((dna = zMapFeatureGetTranscriptDNA(feature, spliced, cds)))
    {
      /* Adjust for when its known that the start exon is incomplete.... */
      if (feature->feature.transcript.flags.start_not_found)
	start_incr = feature->feature.transcript.start_not_found - 1 ; /* values are 1 <= start_not_found <= 3 */

      peptide = zMapPeptideCreate(seq_name, gene_name, (dna + start_incr), NULL, TRUE) ;

      /* Note that we do not include the "Stop" in the peptide length, is this the norm ? */
      pep_length = zMapPeptideLength(peptide) ;
      if (zMapPeptideHasStopCodon(peptide))
	pep_length-- ;

      if (menu_item_id == ZMAPCDS || menu_item_id == ZMAPTRANSCRIPT || menu_item_id == ZMAPUNSPLICED)
	{
	  char *title ;
	  char *peptide_fasta ;

	  peptide_fasta = zMapFASTAString(ZMAPFASTA_SEQTYPE_AA, seq_name, molecule_type, gene_name,
					  pep_length, zMapPeptideSequence(peptide)) ;

	  title = g_strdup_printf("%s%s%s",
				  seq_name,
				  gene_name ? ":" : "",
				  gene_name ? gene_name : "") ;
	  zMapGUIShowText(title, peptide_fasta, FALSE) ;
	  g_free(title) ;

	  g_free(peptide_fasta) ;
	}
      else
	{
	  exportFASTA(menu_data->window, ZMAPFASTA_SEQTYPE_AA,
		    zMapPeptideSequence(peptide), seq_name,
		    pep_length, molecule_type, gene_name) ;
	}

      zMapPeptideDestroy(peptide) ;

      g_free(dna) ;
    }

  g_free(menu_data) ;

  return ;
}


static void exportMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeatureAny feature ;

  feature = zmapWindowItemGetFeatureAny(menu_data->item);

  switch (menu_item_id)
    {
    case 1:
      {
	char *sequence = NULL ;
	char *seq_name = NULL ;
	int seq_len = 0 ;

	if (zMapFeatureBlockDNA((ZMapFeatureBlock)(feature->parent->parent), &seq_name, &seq_len, &sequence))
	  exportFASTA(menu_data->window, ZMAPFASTA_SEQTYPE_DNA, sequence, seq_name, seq_len, "DNA", NULL) ;
	else
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s", "Context contains no DNA.") ;

	break ;
      }
    case 2:
      {
	exportFeatures(menu_data->window, NULL, feature) ;

	break ;
      }
    case 3:
      {
	exportContext(menu_data->window) ;

	break ;
      }
    case 4:
      {
      /*    exportConfig(); */
      break;
      }
    case 12:
      {
	if(zmapWindowMarkIsSet(menu_data->window->mark))
	  {
	    ZMapSpanStruct mark_region = {0,0};

	    zmapWindowMarkGetSequenceRange(menu_data->window->mark, &(mark_region.x1), &(mark_region.x2));

	    exportFeatures(menu_data->window, &mark_region, feature);
	  }
      }
      break;
    default:
      break ;
    }

  g_free(menu_data) ;

  return ;
}





#if MH17_NOT_CALLED

ZMapGUIMenuItem zmapWindowMakeMenuTranscriptTools(int *start_index_inout,
                                                  ZMapGUIMenuItemCallbackFunc callback_func,
                                                  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, "Transcript Tools",       0,             NULL,               NULL},
      {ZMAPGUI_MENU_NORMAL, "Transcript Tools/Split",   ZMAPNAV_SPLIT_FIRST_LAST_EXON,   transcriptNavMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Transcript Tools/Split 2", ZMAPNAV_SPLIT_FLE_WITH_OVERVIEW, transcriptNavMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Transcript Tools/First", ZMAPNAV_FIRST, transcriptNavMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Transcript Tools/Prev",  ZMAPNAV_PREV,  transcriptNavMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Transcript Tools/Next",  ZMAPNAV_NEXT,  transcriptNavMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Transcript Tools/Last",  ZMAPNAV_LAST,  transcriptNavMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

static void transcriptNavMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data;
  ZMapWindow window = NULL;
  GArray  *patterns = NULL;
  ZMapSplitPatternStruct split_style = {0};

  window   = menu_data->window;
  patterns = g_array_new(FALSE, FALSE, sizeof(ZMapSplitPatternStruct));

  switch(menu_item_id)
    {
    case ZMAPNAV_SPLIT_FLE_WITH_OVERVIEW:
      {
        ZMapWindowSplittingStruct split = {0};
        split.original_window   = window;

        split_style.subject     = ZMAPSPLIT_ORIGINAL;
        split_style.orientation = GTK_ORIENTATION_HORIZONTAL;
        g_array_append_val(patterns, split_style);

        split_style.subject     = ZMAPSPLIT_ORIGINAL;
        split_style.orientation = GTK_ORIENTATION_VERTICAL;
        g_array_append_val(patterns, split_style);

        split_style.subject     = ZMAPSPLIT_NEW;
        split_style.orientation = GTK_ORIENTATION_HORIZONTAL;
        g_array_append_val(patterns, split_style);
        /*
        */
        split.split_patterns    = patterns;

        (*(window->caller_cbs->splitToPattern))(window, window->app_data, &split);
      }
      break;
    case ZMAPNAV_SPLIT_FIRST_LAST_EXON:
      {
        ZMapWindowSplittingStruct split = {0};
        split.original_window   = window;

        split_style.subject     = ZMAPSPLIT_ORIGINAL;
        split_style.orientation = GTK_ORIENTATION_VERTICAL;
        g_array_append_val(patterns, split_style);

        split_style.subject     = ZMAPSPLIT_NEW;
        split_style.orientation = GTK_ORIENTATION_HORIZONTAL;
        g_array_append_val(patterns, split_style);

        split.split_patterns    = patterns;

        (*(window->caller_cbs->splitToPattern))(window, window->app_data, &split);
      }
      break;
    case ZMAPNAV_FIRST:
      break;
    case ZMAPNAV_LAST:
      break;
    case ZMAPNAV_NEXT:
      break;
    case ZMAPNAV_PREV:
      break;
    default:
      break;
    }

  if(patterns)
    g_array_free(patterns, TRUE);

  return ;
}

#endif

static void markMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;

  zmapWindowToggleMark(menu_data->window, 0);

  return ;
}

static void compressMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapWindowCompressMode compress_mode = (ZMapWindowCompressMode)menu_item_id  ;
  FooCanvasGroup *column_group ;

  if (menu_data->item)
    column_group = FOO_CANVAS_GROUP(menu_data->item) ;

  if (compress_mode != ZMAPWINDOW_COMPRESS_VISIBLE)
    {
      if (zmapWindowMarkIsSet(menu_data->window->mark))
	compress_mode = ZMAPWINDOW_COMPRESS_MARK ;
      else
	compress_mode = ZMAPWINDOW_COMPRESS_ALL ;
    }

  zmapWindowColumnsCompress(FOO_CANVAS_ITEM(column_group), menu_data->window, compress_mode) ;

  g_free(menu_data) ;

  return ;

}


/* Configure a column, may mean repositioning the other columns. */
static void configureMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapWindowColConfigureMode configure_mode = (ZMapWindowColConfigureMode)menu_item_id  ;
  FooCanvasGroup *column_group = NULL ;

  /* did user click on an item or on the column background ? */
  if(configure_mode != ZMAPWINDOWCOLUMN_CONFIGURE_ALL)
      column_group = menuDataItemToColumn(menu_data->item);

  zmapWindowColumnConfigure(menu_data->window, column_group, configure_mode) ;

  g_free(menu_data) ;

  return ;
}


static void colourMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;

  zmapWindowShowStyleDialog(menu_data);
	/* don't free the callback data, it's in use */

  return ;
}

static void setStyleCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;

  zmapWindowMenuSetStyleCB(menu_item_id, menu_data);

  g_free(menu_data) ;

  return ;
}




#if ED_G_NEVER_INCLUDE_THIS_CODE

static void unbumpAllCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  FooCanvasGroup *column_group ;

  column_group = menuDataItemToColumn(menu_data->item);

  zmapWindowColumnUnbumpAll(FOO_CANVAS_ITEM(column_group));

  zmapWindowFullReposition(menu_data->window->feature_root_group,TRUE, "unbump all");

  g_free(menu_data) ;

  return ;
}

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* Bump a column and reposition the other columns.
 *
 * NOTE that this function may be called for an individual feature OR a column and
 * needs to deal with both.
 */
static void bumpMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapStyleBumpMode bump_type = (ZMapStyleBumpMode)menu_item_id  ;
  ZMapWindowCompressMode compress_mode ;
  FooCanvasItem *style_item ;

  style_item = menu_data->item ;

  if (zmapWindowMarkIsSet(menu_data->window->mark))
    compress_mode = ZMAPWINDOW_COMPRESS_MARK ;
  else
    compress_mode = ZMAPWINDOW_COMPRESS_ALL ;

  zmapWindowColumnBumpRange(style_item, bump_type, compress_mode) ;

  zmapWindowFullReposition(menu_data->window->feature_root_group,TRUE, "bump menu") ;

  g_free(menu_data) ;

  return ;
}

/* Bump a column and reposition the other columns.
 *
 * NOTE that this function may be called for an individual feature OR a column and
 * needs to deal with both.
 */
static void bumpToggleMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  FooCanvasGroup *column_group = NULL;

  column_group = menuDataItemToColumn(menu_data->item);

  if (column_group)
    {
      ZMapWindowContainerFeatureSet container;
      ZMapStyleBumpMode curr_bump_mode, bump_mode ;
      ZMapWindowCompressMode compress_mode ;

      container = (ZMapWindowContainerFeatureSet)column_group;

      curr_bump_mode = zmapWindowContainerFeatureSetGetBumpMode(container);

      if (curr_bump_mode != ZMAPBUMP_UNBUMP)
      	bump_mode = ZMAPBUMP_UNBUMP ;
      else
	bump_mode = zmapWindowContainerFeatureSetGetDefaultBumpMode(container);

      if (zmapWindowMarkIsSet(menu_data->window->mark))
	compress_mode = ZMAPWINDOW_COMPRESS_MARK ;
      else
	compress_mode = ZMAPWINDOW_COMPRESS_ALL ;

      zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(column_group), bump_mode, compress_mode) ;

      zmapWindowFullReposition(menu_data->window->feature_root_group,TRUE, "bump toggle menu") ;
    }

  g_free(menu_data) ;

  return ;
}



static void maskToggleMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapWindowContainerFeatureSet container = menu_data->container_set;

  zMapWindowContainerFeatureSetShowHideMaskedFeatures(container, container->masked, FALSE);

      /* un/bumped features might be wider */
  zmapWindowFullReposition(menu_data->window->feature_root_group,TRUE, "mask toggle menu") ;

  g_free(menu_data) ;

  return ;
}




// unhighlight the evidence features
static void hideEvidenceMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapWindowFocus focus = menu_data->window->focus;

  if (focus)
    {
      zmapWindowFocusResetType(focus,WINDOW_FOCUS_GROUP_EVIDENCE);
    }

  g_free(menu_data) ;

  return ;
}


ZMapGUIMenuItem zmapWindowMakeMenuDeveloperOps(int *start_index_inout,
					       ZMapGUIMenuItemCallbackFunc callback_func,
					       gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, "_"DEVELOPER_STR,                   0, NULL,       NULL},
      {ZMAPGUI_MENU_NORMAL, DEVELOPER_STR"/Show Feature",       1, developerMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, DEVELOPER_STR"/Show Feature Style", 2, developerMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, DEVELOPER_STR"/Print Canvas",       3, developerMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, DEVELOPER_STR"/Show Window Stats",  4, developerMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL               , 0, NULL, NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

#if 0
static void show_all_styles_cb(ZMapFeatureTypeStyle style, gpointer unused)
{
  zmapWindowShowStyle(style) ;
  return ;
}
#endif

static void developerMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeatureAny feature_any ;

  feature_any = zmapWindowItemGetFeatureAny(menu_data->item) ;

  switch (menu_item_id)
    {
    case 1:
      {
	if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
	  {
	    zMapWarning("%s", "Not on a feature.") ;
	  }
	else if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
	  {
	    char *feature_text ;
	    ZMapFeature feature ;
	    GString *item_text_str ;
	    char *coord_text ;
	    char *msg_text ;

	    feature = (ZMapFeature)feature_any ;

	    feature_text = zMapFeatureAsString(feature) ;

	    item_text_str = g_string_sized_new(2048) ;

	    zmapWindowItemDebugItemToString(item_text_str, menu_data->item) ;

	    coord_text = zmapWindowItemCoordsText(menu_data->item) ;

	    msg_text = g_strdup_printf("%s\n%s\t%s\n", feature_text, item_text_str->str, coord_text) ;

	    zMapGUIShowText((char *)g_quark_to_string(feature->original_id), msg_text, FALSE) ;

	    g_free(msg_text) ;
	    g_free(coord_text) ;
	    g_string_free(item_text_str, TRUE) ;
	    g_free(feature_text) ;
	  }

	break ;
      }
    case 2:
      {
	ZMapWindowContainerFeatureSet container = NULL;

	if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
	  {
	    ZMapFeatureTypeStyle style ;

	    container = (ZMapWindowContainerFeatureSet)(menu_data->item) ;

	    /*zmapWindowStyleTableForEach(container->style_table, show_all_styles_cb, NULL);*/
	    style = container->style ;
	    zmapWindowShowStyle(style) ;
    	  }
	else if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
	  {
	    container = (ZMapWindowContainerFeatureSet)(menu_data->item->parent) ;
	    if (container)
	      {
		ZMapFeatureTypeStyle style ;
		ZMapFeature feature ;

		feature = (ZMapFeature)feature_any ;
		style = *feature->style ;

		zmapWindowShowStyle(style) ;
	      }
	  }

	break ;
      }

    case 3:
      {
	zmapWindowPrintCanvas(menu_data->window->canvas) ;

	break ;
      }

    case 4:
      {
	GString *session_text ;
	char *title ;

	session_text = g_string_new(NULL) ;

	g_string_append(session_text, "General\n") ;
	g_string_append_printf(session_text, "\tProgram: %s\n\n", zMapGetAppTitle()) ;
	g_string_append_printf(session_text, "\tUser: %s (%s)\n\n", g_get_user_name(), g_get_real_name()) ;
	g_string_append_printf(session_text, "\tMachine: %s\n\n", g_get_host_name()) ;
	g_string_append_printf(session_text, "\tSequence: %s\n\n", menu_data->window->sequence->sequence) ;

	g_string_append(session_text, "Window Statistics\n") ;
	zMapWindowStats(menu_data->window, session_text) ;

	title = zMapGUIMakeTitleString(NULL, "Session Statistics") ;
	zMapGUIShowText(title, session_text->str, FALSE) ;
	g_free(title) ;
	g_string_free(session_text, TRUE) ;

	break ;
      }

    default:
      {
	break ;
      }
    }

  g_free(menu_data) ;

  return ;
}


/* Clicked on a non-alignment feature... */
ZMapGUIMenuItem zmapWindowMakeMenuNonHomolFeature(int *start_index_inout,
						  ZMapGUIMenuItemCallbackFunc callback_func,
						  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, BLIXEM_MENU_STR " " BLIXEM_DNA_STR " - show this column",
       BLIX_NONE, blixemMenuCB, NULL, "<shift>A"},
      {ZMAPGUI_MENU_NONE,   NULL,                                        0, NULL,         NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}



/* Common blixem ops that must be in top level of menu. */
ZMapGUIMenuItem zmapWindowMakeMenuBlixCommon(int *start_index_inout,
					     ZMapGUIMenuItemCallbackFunc callback_func,
					     gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, BLIXEM_MENU_STR " - all matches for this column",
       BLIX_SET, blixemMenuCB, NULL, "A"},
      {ZMAPGUI_MENU_NORMAL, BLIXEM_MENU_STR " - all matches for selected features",
       BLIX_SELECTED, blixemMenuCB, NULL, "<shift>A"},
      {ZMAPGUI_MENU_NONE,   NULL,                                        0, NULL,         NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


/* Blixem top level menu branch entry.... */
ZMapGUIMenuItem zmapWindowMakeMenuBlixTop(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, BLIXEM_OPS_STR, 0, NULL,       NULL},
      {ZMAPGUI_MENU_NONE,   NULL,               0, NULL,       NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


/* Common blixem ops that must be at top of column sub-menu for blixem. */
ZMapGUIMenuItem zmapWindowMakeMenuBlixColCommon(int *start_index_inout,
						ZMapGUIMenuItemCallbackFunc callback_func,
						gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, BLIXEM_OPS_STR"/"BLIXEM_MENU_STR " - all matches for this column",
       BLIX_SET, blixemMenuCB, NULL, "A"},
      {ZMAPGUI_MENU_NORMAL, BLIXEM_OPS_STR"/"BLIXEM_MENU_STR " - all matches for selected features",
       BLIX_SELECTED, blixemMenuCB, NULL, "<shift>A"},
      {ZMAPGUI_MENU_NONE,   NULL,                                        0, NULL,         NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


/* Clicked on a dna homol feature... */
ZMapGUIMenuItem zmapWindowMakeMenuDNAHomolFeature(int *start_index_inout,
						  ZMapGUIMenuItemCallbackFunc callback_func,
						  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, BLIXEM_OPS_STR"/"BLIXEM_DNA_STR " - all matches for selected features, expanded",
       BLIX_EXPANDED, blixemMenuCB, NULL, "<shift>X"},
      {ZMAPGUI_MENU_NONE,   NULL,                                        0, NULL,         NULL}
    } ;


  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

/* Clicked on a dna homol column... */
ZMapGUIMenuItem zmapWindowMakeMenuDNAHomol(int *start_index_inout,
					   ZMapGUIMenuItemCallbackFunc callback_func,
					   gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, BLIXEM_OPS_STR"/"BLIXEM_DNAS_STR " - all matches for associated columns",
       BLIX_MULTI_SETS, blixemMenuCB, NULL, "<Ctrl>A"},
      {ZMAPGUI_MENU_NONE,   NULL,                                             0, NULL,         NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* no protein specific operations currently.... */
ZMapGUIMenuItem zmapWindowMakeMenuProteinHomolFeature(int *start_index_inout,
						      ZMapGUIMenuItemCallbackFunc callback_func,
						      gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NONE,   NULL,                             0, NULL,         NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


ZMapGUIMenuItem zmapWindowMakeMenuProteinHomol(int *start_index_inout,
					       ZMapGUIMenuItemCallbackFunc callback_func,
					       gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, BLIXEM_OPS_STR"/"BLIXEM_AAS_STR " - all matches for associated columns",
       BLIX_MULTI_SETS, blixemMenuCB, NULL, "<Ctrl>A"},

      {ZMAPGUI_MENU_NONE,   NULL,                                            0, NULL,         NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


/* This function creates a Blixem BAM menu. */
ZMapGUIMenuItem zmapWindowMakeMenuBlixemBAM(int *start_index_inout,
					    ZMapGUIMenuItemCallbackFunc callback_func,
					    gpointer callback_data)
{
  /* get a list of featuresets from the window's context_map */
  static ZMapGUIMenuItem menu = NULL;
  static int n_menu = 0;
  ItemMenuCBData cbdata  = (ItemMenuCBData)callback_data;
  ZMapGUIMenuItem m;
  GList *fs_list = cbdata->window->context_map->seq_data_featuresets;
  GList *fsl;
  int n_sets = g_list_length(fs_list);
  int i = 0;
  char * related = NULL;	/* req data from coverage */
  char * blixem_col = NULL;	/* blixem related from coverage or directly from real data */
  GQuark fset_id = 0,req_id = 0;

  if (!n_sets)
    return NULL;

  if(n_menu < n_sets)
    {
      /* as this derives from config data read on creating the view
       * it will not change unless we reconfig and open another view
       * alloc enough for all views
       */
      if(menu)
	{
	  for(m = menu; m->type != ZMAPGUI_MENU_NONE ;m++)
	    g_free(m->name);

	  g_free(menu);
	}
      m = menu = g_new0(ZMapGUIMenuItemStruct, n_sets * 2 + 2 + 2 + 1 + 6);
      /* main menu, sub menu, two extra items plus terminator, plus 6 just to be sure */
    }

  /* add request from coverage items and blixem from coverage and real data */
  if(cbdata->feature)
    {
      fset_id = ((ZMapFeatureSet) (cbdata->feature->parent))->unique_id;
      req_id = related_column(cbdata->window->context_map,fset_id);

      if(req_id)
	related = get_menu_string(req_id,'-');
    }

  if(related)
    {
      blixem_col = related;
      cbdata->req_id = req_id;
      /* if we click on a coverage column we blixem the data column which contains featuresets */
    }
  else if(zMapFeatureIsSeqFeatureSet(cbdata->window->context_map,fset_id))
    {
      /* if we click on a data column we blixem that not the featureset as we may have several featuresets in the column */
      /* can get the column from the menu_data->container_set or from the featureset_2_column... */
      /* can't remember how reliable is the container */
      ZMapFeatureSetDesc f2c ;

      if ((f2c = g_hash_table_lookup(cbdata->window->context_map->featureset_2_column,GUINT_TO_POINTER(fset_id))))
  	{
	  //zMapLogWarning("menu is_seq: %s -> %p",g_quark_to_string(fset_id),f2c);
	  cbdata->req_id = f2c->column_id;
	  blixem_col = get_menu_string(f2c->column_ID,'-');
  	}
      else
  	{
	  zMapLogWarning("cannot find column for featureset %s",g_quark_to_string(fset_id));
	  /* rather than asserting */
  	}
    }

  if (blixem_col)
    {
      m->type = ZMAPGUI_MENU_NORMAL;
      m->name = g_strdup_printf("Blixem %s paired reads", blixem_col);
      m->id = BLIX_SEQ_COVERAGE;
      m->callback_func = blixemMenuCB;
      m++;
    }

  /* add sub menu */
  m->type = ZMAPGUI_MENU_BRANCH;
  m->name = g_strdup(BLIXEM_OPS_STR"/"BLIXEM_READS_STR);
  m->id = 0;
  m->callback_func = NULL;
  m++;

  for(i = 0, fsl = fs_list;i < n_sets; i++, fsl = fsl->next)
    {
      const gchar *fset;
      GQuark req_id;
      ZMapFeatureSetDesc f2c;

      m->type = ZMAPGUI_MENU_NORMAL;

      f2c = g_hash_table_lookup(cbdata->window->context_map->featureset_2_column,fsl->data);
      if(f2c)
	{
	  fset = get_menu_string(f2c->feature_src_ID,'/');

	  req_id = related_column(cbdata->window->context_map,GPOINTER_TO_UINT(fsl->data));

	  if(!req_id)		/* don't include coverage data */
	    {
	      m->name = g_strdup_printf(BLIXEM_OPS_STR"/"BLIXEM_READS_STR"/%s", fset);
	      m->id = BLIX_SEQ + i;
	      m->callback_func = blixemMenuCB;
	      m++;
	    }
	}
    }

  m->type = ZMAPGUI_MENU_NONE;
  m->name = NULL;

  /* this overrides data in the menus as given in the args, but index and func are always NULL */
  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu;
}



ZMapGUIMenuItem zmapWindowMakeMenuRequestBAM(int *start_index_inout,
					     ZMapGUIMenuItemCallbackFunc callback_func,
					     gpointer callback_data)
{
  /* get a list of featuresets from the window's context_map */
  static ZMapGUIMenuItem menu = NULL;
  static int n_menu = 0;
  ItemMenuCBData cbdata  = (ItemMenuCBData)callback_data;
  ZMapGUIMenuItem m;
  GList *fs_list = cbdata->window->context_map->seq_data_featuresets;
  GList *fsl;
  int n_sets = g_list_length(fs_list);
  int i = 0;
  char * related = NULL;	/* req data from coverage */
  char * blixem_col = NULL;	/* blixem related from coverage or directly from real data */
  GQuark fset_id = 0,req_id = 0;

  if (!n_sets)
    return NULL;

  if(n_menu < n_sets)
    {
      /* as this derives from config data read on creating the view
       * it will not change unless we reconfig and open another view
       * alloc enough for all views
       */
      if(menu)
	{
	  for(m = menu; m->type != ZMAPGUI_MENU_NONE ;m++)
	    g_free(m->name);

	  g_free(menu);
	}
      m = menu = g_new0(ZMapGUIMenuItemStruct, n_sets * 2 + 2 + 2 + 1 + 6);
      /* main menu, sub menu, two extra items plus terminator, plus 6 just to be sure */
    }

  /* add request from coverage items and blixem from coverage and real data */
  if(cbdata->feature)
    {
      fset_id = ((ZMapFeatureSet) (cbdata->feature->parent))->unique_id;
      req_id = related_column(cbdata->window->context_map,fset_id);

      if(req_id)
	related = get_menu_string(req_id,'-');
    }

  if(related)
    {
      blixem_col = related;
      cbdata->req_id = req_id;
      /* if we click on a coverage column we blixem the data column which contains featuresets */
    }
  else if(zMapFeatureIsSeqFeatureSet(cbdata->window->context_map,fset_id))
    {
      /* if we click on a data column we blixem that not the featureset as we may have several featuresets in the column */
      /* can get the column from the menu_data->container_set or from the featureset_2_column... */
      /* can't remember how reliable is the container */
      ZMapFeatureSetDesc f2c ;

      if ((f2c = g_hash_table_lookup(cbdata->window->context_map->featureset_2_column,GUINT_TO_POINTER(fset_id))))
  	{
	  //zMapLogWarning("menu is_seq: %s -> %p",g_quark_to_string(fset_id),f2c);
	  cbdata->req_id = f2c->column_id;
	  blixem_col = get_menu_string(f2c->column_ID,'-');
  	}
      else
  	{
	  zMapLogWarning("cannot find column for featureset %s",g_quark_to_string(fset_id));
	  /* rather than asserting */
  	}
    }

  if (related)
    {
      m->type = ZMAPGUI_MENU_NORMAL;
      m->name = g_strdup_printf(COLUMN_CONFIG_STR"/"PAIRED_READS_RELATED, related);
      m->id = REQUEST_SELECTED;
      m->callback_func = requestShortReadsCB;
      m++;
    }

  /* add menu item for all data in column id any of them have related featuresets */
  /* can't trigger off selected feature as we don't always have one */
  {
    GQuark fset;
    GList *l;

    l = zmapWindowContainerFeatureSetGetFeatureSets(cbdata->container_set);
    for(;l; l = l->next)
      {
	fset = GPOINTER_TO_UINT(l->data);
	if(related_column(cbdata->window->context_map,fset))
	  {
	    m->type = ZMAPGUI_MENU_NORMAL;
	    m->name = g_strdup(COLUMN_CONFIG_STR"/"PAIRED_READS_ALL);
	    m->id = REQUEST_ALL_SEQ;
	    m->callback_func = requestShortReadsCB;
	    m++;
	    /* found one, so we can request all, don't do it twice */
	    break;
	  }
      }
  }


  /* add sub menu */
  m->type = ZMAPGUI_MENU_BRANCH;
  m->name = g_strdup(COLUMN_CONFIG_STR"/"PAIRED_READS_DATA);
  m->id = 0;
  m->callback_func = NULL;
  m++;

  for(i = 0, fsl = fs_list;i < n_sets; i++, fsl = fsl->next)
    {
      const gchar *fset;
      GQuark req_id;
      ZMapFeatureSetDesc f2c;

      m->type = ZMAPGUI_MENU_NORMAL;

      f2c = g_hash_table_lookup(cbdata->window->context_map->featureset_2_column,fsl->data);
      if(f2c)
	{
	  fset = get_menu_string(f2c->feature_src_ID,'/');

	  req_id = related_column(cbdata->window->context_map,GPOINTER_TO_UINT(fsl->data));

	  if(!req_id)		/* don't include coverage data */
	    {
	      m->name = g_strdup_printf(COLUMN_CONFIG_STR"/"PAIRED_READS_DATA"/%s", fset);
	      m->id = REQUEST_SEQ + i;
	      m->callback_func = requestShortReadsCB;
	      m++;
	    }
	}
    }

  m->type = ZMAPGUI_MENU_NONE;
  m->name = NULL;

  /* this overrides data in the menus as given in the args, but index and func are always NULL */
  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu;
}



/* return string with / instead of _ */
static char *get_menu_string(GQuark set_quark,char disguise)
{
  char *p;
  char *seq_set;

  seq_set = g_strdup ((char *) g_quark_to_string(GPOINTER_TO_UINT(set_quark)));

  for(p = seq_set ; *p ;p++)
    {
      /* _ adds an accelerator and gives us a GTK error */
      /* / is what we'd prefer as that what we use on the menus but that would create a menu structure */
      /* the problem is using the menu text for presentation and also for structure */
      /*  Gthanks GLib */

      if(*p == '_')	/* / is there for menus, we want featuresets without escaping names*/
	*p = disguise;
    }

  return(seq_set);
}


/* does a featureset have a related one?
 * FTM this means fset is coverage and the related is the real data (a one way relation)
 * is so don't include in menus
 */
GQuark related_column(ZMapFeatureContextMap map,GQuark fset_id)
{
  GQuark q = 0;
  ZMapFeatureSource src;

  src = g_hash_table_lookup(map->source_2_sourcedata,GUINT_TO_POINTER(fset_id));
  if(src)
    q = src->related_column;
  else
    zMapLogWarning("Can't find src data for %s",g_quark_to_string(fset_id));

  return q;
}





GList * add_column_featuresets(ZMapFeatureContextMap map, GList *list, GQuark column_id, gboolean unique_id)
{
  ZMapFeatureColumn column = g_hash_table_lookup(map->columns,(GUINT_TO_POINTER(column_id)));
  GList *l = NULL;

  if(column)
    l = zMapFeatureGetColumnFeatureSets(map, column_id, unique_id);

  if(column && l)
    {
      for(;l ; l = l->next)
	{
	  list = g_list_prepend(list,l->data);
	}
    }
  else		/* not configured: assume one featureset of the same name */
    {
      list = g_list_prepend(list,GUINT_TO_POINTER(column_id));
    }
  return list;
}

/* make requests either for a single type of paired read of for all in the current column. */
/* NOTE that 'a single type of paired read' is several featuresets combined into a column
 * and we have to request all the featuresets
 */
static void requestShortReadsCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  GList *l;
  int i;
  GList *req_list = NULL;

#if 0
  if (!zmapWindowMarkIsSet(menu_data->window->mark))
    {
      zMapMessage("You must set the mark first to select this option","");
    }
  else
#endif
  if(menu_item_id == REQUEST_SELECTED)
    {
      /* this is for a column related to a coverage featureset so we get several featuresets */
      req_list = add_column_featuresets(menu_data->context_map,req_list,menu_data->req_id,TRUE);
    }
  else if (menu_item_id == REQUEST_ALL_SEQ)
    {
      GQuark fset_id,req_id;
      ZMapFeatureSource src;

      l = zmapWindowContainerFeatureSetGetFeatureSets(menu_data->container_set);
      for(;l; l = l->next)
	{

	  fset_id = GPOINTER_TO_UINT(l->data);
	  src = g_hash_table_lookup(menu_data->context_map->source_2_sourcedata,GUINT_TO_POINTER(fset_id));
	  req_id = src->related_column;

	  if(req_id)
	    req_list = add_column_featuresets(menu_data->context_map,req_list,req_id,TRUE);
	}
    }
  else
    /* legacy menu */
    {
      for(i = menu_item_id - REQUEST_SEQ,
	    l = menu_data->window->context_map->seq_data_featuresets;
	  i && l; l = l->next, i--)
	{
	  continue;
	}

      if(l)
	{	/* this is a featureset not a column ! */
	  req_list = g_list_prepend(req_list,l->data);
	}
    }

  if(req_list)
    {
      ZMapFeatureBlock block;
      ZMapWindowContainerGroup container;

      /* may not have a feature but must have clicked on a column
       * we need the containing block to fetch the data
       */
      container = zmapWindowContainerUtilsGetParentLevel((ZMapWindowContainerGroup)(menu_data->container_set),
							 ZMAPCONTAINER_LEVEL_BLOCK);
      block = zmapWindowItemGetFeatureBlock((FooCanvasItem *) container);

      /* can't use 'is_column' here as we may be requesting several */
      zmapWindowFetchData(menu_data->window, block, req_list, TRUE,FALSE);
    }
  g_free(menu_data) ;

  return ;
}




/* call blixem either for a single type of homology or for all homologies. */
static void blixemMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapWindowAlignSetType requested_homol_set = BLIX_INVALID ;
  GList *seq_sets = NULL;

  switch(menu_item_id)
    {
    case BLIX_NONE:
      requested_homol_set = ZMAPWINDOW_ALIGNCMD_NONE ;
      break;
    case BLIX_SELECTED:
      requested_homol_set = ZMAPWINDOW_ALIGNCMD_FEATURES ;
      break;
    case BLIX_EXPANDED:
      requested_homol_set = ZMAPWINDOW_ALIGNCMD_EXPANDED ;
      break;
    case BLIX_SET:
      requested_homol_set = ZMAPWINDOW_ALIGNCMD_SET ;
      break;
    case BLIX_MULTI_SETS:
      requested_homol_set = ZMAPWINDOW_ALIGNCMD_MULTISET ;
      break;

    case BLIX_SEQ_COVERAGE:		/* blixem from a selected item in a coverage featureset */
#if RESTRICT_TO_MARK
      if (!zmapWindowMarkIsSet(menu_data->window->mark))
	{
	  zMapMessage("You must set the mark first to select this option","");
	}
      else
#endif
	{
          /*! \todo #warning if we ever have paired reads data in a virtual featureset we need to expand that here */
	  seq_sets = add_column_featuresets(menu_data->window->context_map,seq_sets,menu_data->req_id,FALSE);
	  requested_homol_set = ZMAPWINDOW_ALIGNCMD_SEQ ;
	}
      break;

#if 0
    default:
      break;
#else
    default:
      if(menu_item_id >= BLIX_SEQ)      /* one or more sets starting from BLIX_SEQ */
	{
	  GList *l;
	  int i;

#if RESTRICT_TO_MARK
	  if (!zmapWindowMarkIsSet(menu_data->window->mark))
	    {
	      zMapMessage("You must set the mark first to select this option","");
	    }
	  else
#endif
	    {
	      for (i = menu_item_id - BLIX_SEQ, l = menu_data->window->context_map->seq_data_featuresets ;
		   i && l ;
		   l = l->next, i--)
		{
		  continue ;
		}

	      if (l)
		{
		  ZMapFeatureSetDesc src = g_hash_table_lookup(menu_data->window->context_map->featureset_2_column,l->data);
		  if(src)
		    {
		      seq_sets = g_list_prepend (seq_sets,GUINT_TO_POINTER(src->feature_src_ID));
		      requested_homol_set = ZMAPWINDOW_ALIGNCMD_SEQ ;
		    }
		}
	    }

	  break ;
	}
#endif
    }


  if (requested_homol_set)
    zmapWindowCallBlixem(menu_data->window, menu_data->item, requested_homol_set,
			 menu_data->feature_set, seq_sets, menu_data->x, menu_data->y) ;

  g_free(menu_data) ;

  return ;
}



/* this needs to be a general function... */
static FooCanvasGroup *menuDataItemToColumn(FooCanvasItem *item)
{
  ZMapWindowContainerGroup container;
  FooCanvasGroup *column_group = NULL;

  if((container = zmapWindowContainerCanvasItemGetContainer(item)))
    {
      column_group = (FooCanvasGroup *)container;
    }

  return column_group;
}



static void exportFASTA(ZMapWindow window, ZMapFASTASeqType seq_type, char *sequence, char *seq_name, int seq_len,
			char *molecule_name, char *gene_name)
{
  char *filepath = NULL ;
  GIOChannel *file = NULL ;
  GError *error = NULL ;
  char *error_prefix = "FASTA DNA export failed:" ;
  GtkWidget *toplevel ;

  toplevel = zMapGUIFindTopLevel(window->toplevel) ;


  if (!(filepath = zmapGUIFileChooser(toplevel, "FASTA filename ?", NULL, NULL))
      || !(file = g_io_channel_new_file(filepath, "w", &error))
      || !zMapFASTAFile(file, seq_type, seq_name, seq_len, sequence,
			molecule_name, gene_name, &error))
    {
      char *err_msg = NULL ;

      if (error)
	{
	  err_msg = error->message ;

	  zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, err_msg) ;

	  g_error_free(error) ;
	}
    }


  if (file)
    {
      GIOStatus status ;

      if ((status = g_io_channel_shutdown(file, TRUE, &error)) != G_IO_STATUS_NORMAL)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, error->message) ;

	  g_error_free(error) ;
	}
    }

  if (filepath)
    g_free(filepath) ;


  return ;
}


static void exportFeatures(ZMapWindow window, ZMapSpan region_span, ZMapFeatureAny feature)
{
  char *filepath = NULL ;
  GIOChannel *file = NULL ;
  GError *error = NULL ;
  char *error_prefix = "Features export failed:" ;
  ZMapFeatureBlock feature_block ;

  /* Should extend this any type.... */
  if (feature->struct_type != ZMAPFEATURE_STRUCT_FEATURESET
      && feature->struct_type != ZMAPFEATURE_STRUCT_FEATURE) 
    return ;

  /* Find the block from whatever pointer we are sent...  */
  feature_block = (ZMapFeatureBlock)zMapFeatureGetParentGroup(feature, ZMAPFEATURE_STRUCT_BLOCK);

  if (!(filepath = zmapGUIFileChooser(gtk_widget_get_toplevel(window->toplevel),
				      "Feature Export filename ?", NULL, "gff"))
      || !(file = g_io_channel_new_file(filepath, "w", &error))
      || !zMapGFFDumpRegion((ZMapFeatureAny)feature_block, window->context_map->styles, region_span, file, &error))
    {
      /* N.B. if there is no filepath it means user cancelled so take no action...,
       * otherwise we output the error message. */
      if (error)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, error->message) ;

	  g_error_free(error) ;
	}
    }

  if (file)
    {
      GIOStatus status ;

      if ((status = g_io_channel_shutdown(file, TRUE, &error)) != G_IO_STATUS_NORMAL)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, error->message) ;

	  g_error_free(error) ;
	}
    }


  return ;
}



/* for migration from ACEDB call this via the --dump_config command line arg
 * NB only write the column and featureset config stanzas
 * this really belongs int he view but due to scope issue we cannot access that data here :-(
 * so instead we dump the copy of that data that had to be passed to the window so that we could access it
 *
 * Hmmm.... we have featureset_2_column but not src_2_src
 * so no can do.
 * Will revisit after tidying up the view/window data copying fiasco
 */
#if MH17_DONT_INCLUDE   // can't do this due top scope
void dumpConfig(GHashTable *f2c, GHashTable *f2s)
{
  GList *iter;
  ZMapFeatureSetDesc gff_set;
  ZMapFeatureSource gff_src;
  gpointer key, value;
  GHashTable * cols = zMap_g_hashlist_create();
  GList *l;
  char *filepath = NULL ;
  GIOChannel *file = NULL ;
  GError *error = NULL ;
  char *error_prefix = "Feature config dump failed:" ;

  if (!(filepath = zmapGUIFileChooser(window->toplevel, "Config Dump filename ?", NULL, "zmap"))
      || !(file = g_io_channel_new_file(filepath, "w", &error))
      || !zMapFeatureContextDump(window->feature_context, window->context_map->styles, file, &error))
    {
      /* N.B. if there is no filepath it means user cancelled so take no action...,
       * otherwise we output the error message. */
      if (error)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, error->message) ;

	  g_error_free(error) ;
	}
    }


  if (file)
    {
      GIOStatus status ;

      if ((status = g_io_channel_shutdown(file, TRUE, &error)) != G_IO_STATUS_NORMAL)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, error->message) ;

	  g_error_free(error) ;
	}
    }



  zMap_g_hash_table_iter_init (&iter, f2c);
  while (zMap_g_hash_table_iter_next (&iter, &key, &value))
    {
      gff_set = (ZMapFeatureSetDesc) value;
      zMap_g_hashlist_insert(cols,value,key);
    }

  printf("\n[columns]\n");

  zMap_g_hash_table_iter_init (&iter, cols);
  while (zMap_g_hash_table_iter_next (&iter, &key, &value))
    {
      char *sep = "";
      printf("%s = ");
      for(l = (GList *) value;l;l = l->next)
	{
	  printf(" %s %s",sep,g_quark_to_string(GPOINTER_TO_UINT(l->data)));
	  sep = ";";
	}
      printf("\n");
    }

  zMap_g_hashlist_destroy(cols);

  printf("\nf[featureset-style]\n");
  zMap_g_hash_table_iter_init (&iter, f2s);
  while (zMap_g_hash_table_iter_next (&iter, &key, &value))
    {
      gff_src = (ZMapFeatureSource) value;
      printf("%s = %s\n",g_quark_to_string(GPOINTER_TO_UINT(key)),
	     g_quark_to_string(GPOINTER_TO_UINT(gff_src->style_id)));
    }
  return ;
}

#endif

static void exportContext(ZMapWindow window)
{
  char *filepath = NULL ;
  GIOChannel *file = NULL ;
  GError *error = NULL ;
  char *error_prefix = "Feature context export failed:" ;

  if (!(filepath = zmapGUIFileChooser(window->toplevel, "Context Export filename ?", NULL, "zmap"))
      || !(file = g_io_channel_new_file(filepath, "w", &error))
      || !zMapFeatureContextDump(window->feature_context, window->context_map->styles, file, &error))
    {
      /* N.B. if there is no filepath it means user cancelled so take no action...,
       * otherwise we output the error message. */
      if (error)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, error->message) ;

	  g_error_free(error) ;
	}
    }


  if (file)
    {
      GIOStatus status ;

      if ((status = g_io_channel_shutdown(file, TRUE, &error)) != G_IO_STATUS_NORMAL)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s  %s", error_prefix, error->message) ;

	  g_error_free(error) ;
	}
    }


  return ;
}

/*

* This is an attempt to make dynamic menus... Only really worth it
* for the align & block menus. With others a static struct, as above,
* is much more preferable.

*/

static void insertSubMenus(GString *branch_point_string,
                           ZMapGUIMenuItem sub_menus,
                           ZMapGUIMenuItem item,
                           GArray **items_array)
{
  int branch_length = 0;
  branch_length = branch_point_string->len;

  while(sub_menus && sub_menus->name != NULL)
    {
      if(*items_array)
        {
          g_string_append_printf(branch_point_string, "/%s", sub_menus->name);

          memcpy(item, sub_menus, sizeof(ZMapGUIMenuItemStruct));

          item->name   = g_strdup(branch_point_string->str); /* memory leak */

          *items_array = g_array_append_val(*items_array, *item);
        }

      branch_point_string = g_string_erase(branch_point_string,
                                           branch_length,
                                           branch_point_string->len - branch_length);
      sub_menus++;        /* move on */
    }

  return ;
}


static ZMapFeatureContextExecuteStatus alignBlockMenusDataListForeach(GQuark key,
                                                                      gpointer data,
                                                                      gpointer user_data,
                                                                      char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureLevelType feature_type = ZMAPFEATURE_STRUCT_INVALID;
  GString *item_name = NULL;
  ZMapGUIMenuItem item = NULL, align_items = NULL, block_items = NULL;
  GArray **items_array = NULL;
  char *stem = NULL;
  AlignBlockMenu  all_data = (AlignBlockMenu)user_data;

  items_array = all_data->array;
  stem        = all_data->stem;

  feature_type = feature_any->struct_type;

  item = g_new0(ZMapGUIMenuItemStruct, 1);

  switch(feature_type)
    {
    case ZMAPFEATURE_STRUCT_ALIGN:
      item_name = g_string_sized_new(100);
      g_string_printf(item_name, "%s%sAlign %s",
                      (stem ? stem : ""),
                      (stem ? "/"  : ""),
                      g_quark_to_string( feature_any->original_id ));

      item->name   = g_strdup(item_name->str); /* memory leak */
      item->type   = ZMAPGUI_MENU_BRANCH;
      *items_array = g_array_append_val(*items_array, *item);

      align_items  = all_data->each_align_items;
      while(align_items && align_items->name != NULL)
        {
          ZMapGUIMenuSubMenuData sub_data = g_new0(ZMapGUIMenuSubMenuDataStruct, 1);
          sub_data->align_unique_id  = feature_any->unique_id;
          sub_data->block_unique_id  = 0;
          sub_data->original_data    = align_items->callback_data;
          align_items->callback_data = sub_data;
          align_items++;
        }
      align_items  = all_data->each_align_items;
      insertSubMenus(item_name, align_items, item, items_array);

      g_string_free(item_name, TRUE);
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      item_name = g_string_sized_new(100);
      g_string_printf(item_name, "%s%sAlign %s/Block %s",
                      (stem ? stem : ""),
                      (stem ? "/"  : ""),
                      g_quark_to_string( feature_any->parent->original_id ),
                      g_quark_to_string( feature_any->original_id ));

      item->name   = g_strdup(item_name->str); /* memory leak */
      item->type   = ZMAPGUI_MENU_BRANCH;
      *items_array = g_array_append_val(*items_array, *item);

      block_items  = all_data->each_block_items;
      while(block_items && block_items->name != NULL)
        {
          ZMapGUIMenuSubMenuData sub_data = g_new0(ZMapGUIMenuSubMenuDataStruct, 1);
          sub_data->align_unique_id  = feature_any->parent->unique_id;
          sub_data->block_unique_id  = feature_any->unique_id;
          sub_data->original_data    = block_items->callback_data;
          block_items->callback_data = sub_data;
          block_items++;
        }

      block_items  = all_data->each_block_items;
      insertSubMenus(item_name, block_items, item, items_array);

      g_string_free(item_name, TRUE);
      break;
    case ZMAPFEATURE_STRUCT_CONTEXT:
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
    case ZMAPFEATURE_STRUCT_FEATURE:
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      zMapWarnIfReached();
      break;

    }

  g_free(item);

  return ZMAP_CONTEXT_EXEC_STATUS_OK;
}


 void zMapWindowMenuAlignBlockSubMenus(ZMapWindow window,
				       ZMapGUIMenuItem each_align,
				       ZMapGUIMenuItem each_block,
				       char *root,
				       GArray **items_array_out)
{
  if (!window)
    return ;
  AlignBlockMenuStruct data = {0};

  data.each_align_items = each_align;
  data.each_block_items = each_block;
  data.stem             = root;
  data.array            = items_array_out;

  zMapFeatureContextExecute((ZMapFeatureAny)(window->feature_context),
                            ZMAPFEATURE_STRUCT_BLOCK,
                            alignBlockMenusDataListForeach,
                            &data);

  return ;
}


/* Get colours for coding, non-coding etc of sequence. */
static gboolean getSeqColours(ZMapFeatureTypeStyle style,
			      GdkColor **non_coding_out, GdkColor **coding_out,
			      GdkColor **split_5_out, GdkColor **split_3_out)
{
  gboolean result =  FALSE ;

  /* I'm sure this is not the way to access these...find out from Malcolm where we stand on this... */
  if (style->mode_data.sequence.non_coding.normal.fields_set.fill
      && style->mode_data.sequence.coding.normal.fields_set.fill
      && style->mode_data.sequence.split_codon_5.normal.fields_set.fill
      && style->mode_data.sequence.split_codon_3.normal.fields_set.fill)
    {
      *non_coding_out = (GdkColor *)(&(style->mode_data.sequence.non_coding.normal.fill)) ;
      *coding_out = (GdkColor *)(&(style->mode_data.sequence.coding.normal.fill)) ;
      *split_5_out = (GdkColor *)(&(style->mode_data.sequence.split_codon_5.normal.fill)) ;
      *split_3_out = (GdkColor *)(&(style->mode_data.sequence.split_codon_3.normal.fill)) ;

      result = TRUE ;
    }

  return result ;
}


/* Given a transcript feature get the annotated exons for that feature and create
 * corresponding text attributes for each one, these give position of annotated
 * exon and it's colour. */
static GList *getTranscriptTextAttrs(ZMapFeature feature, gboolean spliced, gboolean cds,
				     GdkColor *non_coding, GdkColor *coding,
				     GdkColor *split_5, GdkColor *split_3)
{
  GList *text_attrs = NULL ;
  GList *exon_list = NULL ;

  if (zMapFeatureAnnotatedExonsCreate(feature, FALSE, &exon_list))
    {
      MakeTextAttrStruct text_data ;

      text_data.feature = feature ;
      text_data.cds = cds ;
      text_data.spliced = spliced ;
      text_data.non_coding = non_coding ;
      text_data.coding = coding ;
      text_data.split_5 = split_5 ;
      text_data.split_3 = split_3 ;
      text_data.text_attrs = text_attrs ;

      g_list_foreach(exon_list, createExonTextTag, &text_data) ;

      text_attrs = text_data.text_attrs ;

      zMapFeatureAnnotatedExonsDestroy(exon_list) ;
    }

  return text_attrs ;
}

/* Create a text attribute for a given annotated exon. */
static void createExonTextTag(gpointer data, gpointer user_data)
{
  ZMapFullExon exon = (ZMapFullExon)data ;
  MakeTextAttr text_data = (MakeTextAttr)user_data ;
  GList *text_attrs_list = text_data->text_attrs ;
  ZMapGuiTextAttr text_attr = NULL ;

  /* Text buffer positions are zero-based so "- 1" for all positions. */
  if (!(text_data->spliced))
    {
      text_attr = g_new0(ZMapGuiTextAttrStruct, 1) ;

      text_attr->start = exon->unspliced_span.x1 - 1 ;
      text_attr->end = exon->unspliced_span.x2 - 1 ;
    }
  else
    {
      if (!(text_data->cds) || exon->region_type != EXON_NON_CODING)
	{
	  text_attr = g_new0(ZMapGuiTextAttrStruct, 1) ;

	  if ((text_data->cds))
	    {
	      text_attr->start = exon->cds_span.x1 - 1 ;
	      text_attr->end = exon->cds_span.x2 - 1 ;
	    }
	  else
	    {
	      text_attr->start = exon->spliced_span.x1 - 1 ;
	      text_attr->end = exon->spliced_span.x2 - 1 ;
	    }
	}
    }


  if (text_attr)
    {
      switch (exon->region_type)
	{
	case EXON_NON_CODING:
	  text_attr->background = text_data->non_coding ;

	  break ;

	case EXON_CODING:
	  {
	    text_attr->background = text_data->coding ;

	    break ;
	  }

	case EXON_START_NOT_FOUND:
	case EXON_SPLIT_CODON_5:
	  text_attr->background = text_data->split_5 ;

	  break ;

	case EXON_SPLIT_CODON_3:
	  text_attr->background = text_data->split_3 ;

	  break ;

	default:
          zMapWarnIfReached() ;
	}

      text_attrs_list = g_list_append(text_attrs_list, text_attr) ;

      text_data->text_attrs = text_attrs_list ;
    }

  return ;
}


/* Offset text attributes by given amount. */
static void offsetTextAttr(gpointer data, gpointer user_data)
{
  ZMapGuiTextAttr text_attr = (ZMapGuiTextAttr)data ;
  int offset = GPOINTER_TO_INT(user_data) ;

  text_attr->start += offset ;
  text_attr->end += offset ;

  return ;
}


/* Services the "Search or List Features and Sequence" sub-menu.  */
static void searchListMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeatureAny feature_any ;
  FooCanvasGroup *featureset_group ;
  ZMapWindowContainerFeatureSet container;
  ZMapFeatureSet featureset ;
  ZMapFeature feature = NULL ;
  gboolean zoom_to_item = TRUE;


  /* Retrieve the feature or featureset info from the canvas item. */
  feature_any = zmapWindowItemGetFeatureAny(menu_data->item);
  if (feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURESET
	     && feature_any->struct_type != ZMAPFEATURE_STRUCT_FEATURE) 
  return ;

  if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
    {
      featureset_group = FOO_CANVAS_GROUP(menu_data->item) ;
      featureset = (ZMapFeatureSet)feature_any ;
    }
  else
    {
      featureset_group = (FooCanvasGroup *)zmapWindowContainerCanvasItemGetContainer(menu_data->item) ;
      feature = zmapWindowItemGetFeature(menu_data->item);
      featureset = (ZMapFeatureSet)feature->parent ;
    }

  container = (ZMapWindowContainerFeatureSet)(featureset_group);


  switch (menu_item_id)
    {
    case ITEM_MENU_LIST_ALL_FEATURES:
    case ITEM_MENU_LIST_NAMED_FEATURES:
      {
	ZMapWindowFToISetSearchData search_data = NULL;
	ZMapStrand set_strand ;
	ZMapFrame set_frame ;
	GQuark align_id = 0, block_id = 0, column_id = 0, featureset_id = 0, feature_id = 0 ;
	FooCanvasItem *current_item = NULL ;
	char *title ;
	gpointer search_func ;

	column_id = menu_data->container_set->unique_id ;
	align_id = featureset->parent->parent->unique_id ;
	block_id = featureset->parent->unique_id ;
	featureset_id = featureset->unique_id ;

	if (feature)
	  current_item = menu_data->item ;

	title = (char *)g_quark_to_string(container->original_id) ;


	if (menu_item_id == ITEM_MENU_LIST_ALL_FEATURES)
	  {
	    /* Set feature name to wild card to ensure we list all of them. */
	    feature_id = g_quark_from_string("*") ;
	    feature = NULL ;				    /* unset feature for "all" search. */
	    search_func = zmapWindowFToIFindItemSetFull ;
	  }
	else
	  {
	    /* Set feature name to original id to ensure we get all features in column with
	     * same name. */
	    feature_id = feature->original_id ;
	    search_func = zmapWindowFToIFindSameNameItems ;
	  }

	set_strand = zmapWindowContainerFeatureSetGetStrand(container) ;
	set_frame = zmapWindowContainerFeatureSetGetFrame(container) ;

	if ((search_data = zmapWindowFToISetSearchCreate(search_func,
							 feature,
							 align_id,
							 block_id,
							 column_id,
							 featureset_id,
							 feature_id,
							 zMapFeatureStrand2Str(set_strand),
							 zMapFeatureFrame2Str(set_frame))))
	  zmapWindowListWindow(menu_data->window,
			       current_item, title,
			       NULL, NULL,
			       menu_data->window->context_map,
			       (ZMapWindowListSearchHashFunc)zmapWindowFToISetSearchPerform, search_data,
			       (GDestroyNotify)zmapWindowFToISetSearchDestroy, zoom_to_item) ;

	break ;
      }

    case ITEM_MENU_SEARCH:
      {
	zmapWindowCreateSearchWindow(menu_data->window,
				     NULL, NULL,
				     menu_data->window->context_map,
				     menu_data->item) ;

	break ;
      }

    case ITEM_MENU_SEQUENCE_SEARCH_DNA:
      {
	zmapWindowCreateSequenceSearchWindow(menu_data->window, menu_data->item, ZMAPSEQUENCE_DNA) ;

	break ;
      }

    case ITEM_MENU_SEQUENCE_SEARCH_PEPTIDE:
      {
	zmapWindowCreateSequenceSearchWindow(menu_data->window, menu_data->item, ZMAPSEQUENCE_PEPTIDE) ;

	break ;
      }

    default:
      {
        zMapWarning("%s", "Unexpected search menu callback action\n") ;
        zMapWarnIfReached() ;
	break ;
      }
    }

  g_free(menu_data) ;

  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
STUFF LEFT TO DO.....

DEFINE HIGHLIGHT_DATA STRUCT
CHANGE FUNC PROTO FOR GETEVIDENCE...AND MAKE IT CALL BACK TO THIS FUNC SUPPLYING
THE EVIDENCE LIST AND WE SHOULD BE DONE.....

BUT MAY NEED A SPECIAL CALLBACK IN FEATURESHOW TO HANDLE THIS CALLBACK 
SEPARATELY FROM THE EXISTING FEATURESHOW CALLBACK......
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* This code is a callback, called from zmapWindowFeatureGetEvidence() which
 * contacts our peer to get evidence lists of features.
 * As far as I can tell this code takes a feature and then tries to highlight all
 * the evidence for that feature by searching columns for named evidence for the
 * feature. */
static void highlightEvidenceCB(GList *evidence, gpointer user_data)
{
  HighlightData highlight_data = (HighlightData)user_data ;
  GList *evidence_items = NULL ;


  /* search for the features named in the list and find thier canvas items */
  for ( ; evidence ; evidence = evidence->next)
    {
      GList *items,*items_free ;
      GQuark wildcard = g_quark_from_string("*");
      char *feature_name;
      GQuark feature_search_id;

      /* need to add a * to the end to match strand and frame name mangling */
      feature_name = g_strdup_printf("%s*",g_quark_to_string(GPOINTER_TO_UINT(evidence->data)));
      feature_name = zMapFeatureCanonName(feature_name);    /* done in situ */
      feature_search_id = g_quark_from_string(feature_name);

      /* catch NULL names due to ' getting escaped int &apos; */
      if(feature_search_id == wildcard)
	continue;

      items_free = zmapWindowFToIFindItemSetFull(highlight_data->window,
						 highlight_data->window->context_to_item,
						 highlight_data->feature->parent->parent->parent->unique_id,
						 highlight_data->feature->parent->parent->unique_id,
						 wildcard,0,"*","*",
						 feature_search_id,
						 NULL,NULL);


      /* zMapLogWarning("evidence %s returns %d features", feature_name, g_list_length(items_free));*/
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
   * in this call it's irrelevant so don't set the hot item, pass NULL instead
   */

  zmapWindowFocusAddItemsType(highlight_data->window->focus, evidence_items,
			      NULL /* menu_data->item */, WINDOW_FOCUS_GROUP_EVIDENCE);


  g_list_free(evidence);
  g_list_free(evidence_items);


  return ;
}

