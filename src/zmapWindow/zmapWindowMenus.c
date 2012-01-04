/*  Last edited: Jul 12 08:18 2011 (edgrif) */
/*  File: zmapWindowMenus.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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


/* some common menu strings, needed because cascading menus need the same string as their parent
 * menu item. */

#define FEATURE_DUMP_STR           "Export "
#define FEATURE_SHOW_STR           "Show "

#define FEATURE_DNA_STR            "Feature DNA"
#define FEATURE_DNA_DUMP_STR       FEATURE_DUMP_STR FEATURE_DNA_STR
#define FEATURE_DNA_SHOW_STR       FEATURE_SHOW_STR FEATURE_DNA_STR

#define FEATURE_PEPTIDE_STR        "Feature peptide"
#define FEATURE_PEPTIDE_DUMP_STR   FEATURE_DUMP_STR FEATURE_PEPTIDE_STR
#define FEATURE_PEPTIDE_SHOW_STR   FEATURE_SHOW_STR FEATURE_PEPTIDE_STR

#define CONTEXT_STR                "Whole View"
#define CONTEXT_EXPORT_STR         FEATURE_DUMP_STR CONTEXT_STR


#define DEVELOPER_STR              "Developer"


/* Strings/enums for invoking blixem. */
#define BLIXEM_MENU_STR            "Blixem "
#define BLIXEM_DNA_STR             "DNA"
#define BLIXEM_DNAS_STR            BLIXEM_DNA_STR "s"
#define BLIXEM_AA_STR              "Protein"
#define BLIXEM_AAS_STR             BLIXEM_AA_STR "s"


enum
  {
    BLIX_INVALID,
    BLIX_NONE,		/* Blixem on column with no aligns. */
    BLIX_SELECTED,	/* Blixem all matches for selected features in this column. */
    BLIX_SET,		/* Blixem all matches for all features in this column. */
    BLIX_MULTI_SETS,	/* Blixem all matches for all features in the list of columns in the blixem config file. */
    BLIX_SEQ_COVERAGE,	/* Blixem a coverage column from the mark: find the real data column */
//    BLIX_SEQ_SET		/* Blixem a paried read featureset */
  } ;

#define BLIX_SEQ		10000       /* Blixem short reads data from the mark base menu index */
#define REQUEST_SELECTED 20000	/* data related to selected featureset in column */
#define REQUEST_ALL_SEQ 20001		/* all data related to coverage featuresets in column */
#define REQUEST_SEQ	20002		/* request SR data from mark from menu */
    /* MH17 NOTE BLIX_SEQ etc is a range of values */
    /* this is a temporary implementation till we can blixem a short reads column */
    /* we assume that we have less than 10k datasets */


/* Choose which way a transcripts dna is dumped... */
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



static void maskToggleMenuCB(int menu_item_id, gpointer callback_data);


static void hideEvidenceMenuCB(int menu_item_id, gpointer callback_data);
static void compressMenuCB(int menu_item_id, gpointer callback_data);
static void configureMenuCB(int menu_item_id, gpointer callback_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void bumpToInitialCB(int menu_item_id, gpointer callback_data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void unbumpAllCB(int menu_item_id, gpointer callback_data);
static void bumpMenuCB(int menu_item_id, gpointer callback_data) ;
static void bumpToggleMenuCB(int menu_item_id, gpointer callback_data) ;
static void dnaMenuCB(int menu_item_id, gpointer callback_data) ;
static void peptideMenuCB(int menu_item_id, gpointer callback_data) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void transcriptNavMenuCB(int menu_item_id, gpointer callback_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static void dumpMenuCB(int menu_item_id, gpointer callback_data) ;
static void developerMenuCB(int menu_item_id, gpointer callback_data) ;
static void requestShortReadsCB(int menu_item_id, gpointer callback_data);
static void blixemMenuCB(int menu_item_id, gpointer callback_data) ;

static FooCanvasGroup *menuDataItemToColumn(FooCanvasItem *item) ;

static void dumpFASTA(ZMapWindow window, ZMapFASTASeqType seq_type, char *seq, char *seq_name, int seq_len,
		      char *molecule_name, char *gene_name) ;
static void dumpFeatures(ZMapWindow window, ZMapSpan span, ZMapFeatureAny feature) ;
static void dumpContext(ZMapWindow window) ;

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

ZMapGUIMenuItem zmapWindowMakeMenuBump(int *start_index_inout,
				       ZMapGUIMenuItemCallbackFunc callback_func,
				       gpointer callback_data, ZMapStyleBumpMode curr_bump)
{
  #define MORE_OPTS "Column Bump More Opts"

  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_TOGGLE, "Column Bump",                            ZMAPBUMP_UNBUMP,  bumpToggleMenuCB, NULL, "B"},
      {ZMAPGUI_MENU_NORMAL, "Column Hide",                            ZMAPWINDOWCOLUMN_HIDE, configureMenuCB,  NULL},

      {ZMAPGUI_MENU_TOGGLE, "Show Masked Features",                   ZMAPWINDOWCOLUMN_MASK,  maskToggleMenuCB, NULL, NULL},

      {ZMAPGUI_MENU_BRANCH, "Column Configure",                       0,                              NULL,            NULL},
      {ZMAPGUI_MENU_NORMAL, "Column Configure/Configure This Column", ZMAPWINDOWCOLUMN_CONFIGURE,     configureMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Column Configure/Configure All Columns", ZMAPWINDOWCOLUMN_CONFIGURE_ALL, configureMenuCB, NULL},
      {ZMAPGUI_MENU_BRANCH, MORE_OPTS,                                0,                                 NULL,       NULL},
      {ZMAPGUI_MENU_RADIO,  NULL,                                     ZMAPBUMP_NAME,                  bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  NULL,                                     ZMAPBUMP_NAME_INTERLEAVE,    bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  NULL,                                     ZMAPBUMP_NAME_COLINEAR,         bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  NULL,                                     ZMAPBUMP_NAME_NO_INTERLEAVE,    bumpMenuCB, NULL},
// may be some logical problems doing this
//      {ZMAPGUI_MENU_RADIO,  NULL,                                     ZMAPBUMP_NAME_INTERLEAVE_COLINEAR, bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  NULL,                                     ZMAPBUMP_NAME_BEST_ENDS,     bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  NULL,                                     ZMAPBUMP_OVERLAP,               bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  NULL,                                     ZMAPBUMP_START_POSITION,         bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  NULL,                                     ZMAPBUMP_ALTERNATING,        bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  NULL,                                     ZMAPBUMP_ALL,                bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  NULL,                                     ZMAPBUMP_UNBUMP,                bumpMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Unbump All Columns",                     0,                              unbumpAllCB, NULL},
//      {ZMAPGUI_MENU_NORMAL, "Bump All Columns to Default",            0,                              bumpToInitialCB, NULL},

      {ZMAPGUI_MENU_NORMAL, "Compress Columns",                       ZMAPWINDOW_COMPRESS_MARK,    compressMenuCB,  NULL, "c"},
      {ZMAPGUI_MENU_NORMAL, "UnCompress Columns",                     ZMAPWINDOW_COMPRESS_VISIBLE, compressMenuCB,  NULL, "<shift>C"},

      {ZMAPGUI_MENU_HIDE, "Hide Evidence",                     ZMAPWINDOW_HIDE_EVIDENCE, hideEvidenceMenuCB,  NULL },

      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}  // menu terminates on id = 0 in one loop below
    } ;
  static gboolean menu_set = FALSE ;
  static int ind_hide_evidence = -1;
  static int ind_mask = -1;
  ZMapGUIMenuItem item ;
  ItemMenuCBData menu_data = (ItemMenuCBData) callback_data;

  /* Dynamically set the bump option menu text. */
  if (!menu_set)
    {
      ZMapGUIMenuItem tmp = menu ;

      while ((tmp->type))
	{
	  if (tmp->type == ZMAPGUI_MENU_RADIO)
	    tmp->name = g_strdup_printf("%s/%s", MORE_OPTS, zmapStyleBumpMode2ShortText(tmp->id)) ;

        if(tmp->id == ZMAPWINDOW_HIDE_EVIDENCE)
          ind_hide_evidence = tmp - menu;
        if(tmp->id == ZMAPWINDOWCOLUMN_MASK)
          ind_mask = tmp - menu;

	  tmp++ ;
	}

      menu_set = TRUE ;
    }


  /* Set the initial toggle button correctly....make sure this stays in step with the array above.
   * NOTE logic, this button is either "no bump" or "Name + No Bump", the latter should be
   * selectable whatever.... */
  item = &(menu[0]) ;
  if (curr_bump != ZMAPBUMP_UNBUMP)
    {
      item->type = ZMAPGUI_MENU_TOGGLEACTIVE ;
      item->id = curr_bump ;

    }
  else
    {
      item->type = ZMAPGUI_MENU_TOGGLE ;
      item->id = ZMAPBUMP_UNBUMP ;
    }


      // set toggle state of masked features for this column
  if(ind_mask >= 0)
    {
      item = &(menu[ind_mask]) ;

      // set toggle state of masked features for this column
      // hide the menu item if it's not maskable
      item->type = ZMAPGUI_MENU_TOGGLE ;

      if(!menu_data->container_set->maskable || !menu_data->window->highlights_set.masked || !zMapWindowFocusCacheGetSelectedColours(WINDOW_FOCUS_GROUP_MASKED, NULL, NULL))
        item->type = ZMAPGUI_MENU_HIDE;
      else if(!menu_data->container_set->masked)
        item->type = ZMAPGUI_MENU_TOGGLEACTIVE ;
    }



  if(ind_hide_evidence >= 0)
    {
      item = &menu[ind_hide_evidence];
      if(zmapWindowFocusHasType(menu_data->window->focus,WINDOW_FOCUS_GROUP_EVIDENCE))
      {
            item->type = ZMAPGUI_MENU_TOGGLEACTIVE;
            item->name = "Highlight Evidence/ Transcript";
      }
      else
      {
            item->type = ZMAPGUI_MENU_HIDE;
      }
    }


  /* Unset any previous active radio button.... */
  item = &(menu[0]) ;
  while (item->type != ZMAPGUI_MENU_NONE)
    {
      if (item->type == ZMAPGUI_MENU_RADIOACTIVE)
	{
	  item->type = ZMAPGUI_MENU_RADIO ;
	  break ;
	}

      item++ ;
    }

  /* Set new one... */
  item = &(menu[0]) ;
  while (item->type != ZMAPGUI_MENU_NONE)
    {
      if (item->type == ZMAPGUI_MENU_RADIO && item->id == curr_bump)
	{
	  item->type = ZMAPGUI_MENU_RADIOACTIVE ;
	  break ;
	}

      item++ ;
    }

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


/* Probably it would be wise to pass in the callback function, the start index for the item
 * identifier and perhaps the callback data...... */
ZMapGUIMenuItem zmapWindowMakeMenuDNAFeatureAny(int *start_index_inout,
						ZMapGUIMenuItemCallbackFunc callback_func,
						gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, FEATURE_DNA_SHOW_STR,               ZMAPUNSPLICED,     dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


ZMapGUIMenuItem zmapWindowMakeMenuDNAFeatureAnyFile(int *start_index_inout,
						    ZMapGUIMenuItemCallbackFunc callback_func,
						    gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, FEATURE_DNA_DUMP_STR,               ZMAPUNSPLICED_FILE,     dnaMenuCB, NULL},
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
      {ZMAPGUI_MENU_BRANCH, "_"FEATURE_DNA_SHOW_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_DNA_SHOW_STR"/CDS",                    ZMAPCDS,           dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_DNA_SHOW_STR"/transcript",             ZMAPTRANSCRIPT,    dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_DNA_SHOW_STR"/unspliced",              ZMAPUNSPLICED,     dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_DNA_SHOW_STR"/with flanking sequence", ZMAPCHOOSERANGE,   dnaMenuCB, NULL},
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
      {ZMAPGUI_MENU_BRANCH, "_"FEATURE_DNA_DUMP_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_DNA_DUMP_STR"/CDS",                    ZMAPCDS_FILE,           dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_DNA_DUMP_STR"/transcript",             ZMAPTRANSCRIPT_FILE,    dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_DNA_DUMP_STR"/unspliced",              ZMAPUNSPLICED_FILE,     dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_DNA_DUMP_STR"/with flanking sequence", ZMAPCHOOSERANGE_FILE,   dnaMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
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
	zMapAssertNotReached() ;				    /* exits... */
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
	  dumpFASTA(menu_data->window, ZMAPFASTA_SEQTYPE_DNA, dna, seq_name, seq_len, molecule_type, gene_name) ;
	}

      g_free(dna) ;
    }

  g_free(menu_data) ;

  return ;
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

      {ZMAPGUI_MENU_NORMAL, FEATURE_PEPTIDE_SHOW_STR,                    ZMAPCDS,           peptideMenuCB, NULL},

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

ZMapGUIMenuItem zmapWindowMakeMenuPeptideFile(int *start_index_inout,
					      ZMapGUIMenuItemCallbackFunc callback_func,
					      gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, "_"FEATURE_PEPTIDE_DUMP_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_PEPTIDE_DUMP_STR"/CDS",             ZMAPCDS_FILE,           peptideMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_PEPTIDE_DUMP_STR"/transcript",      ZMAPTRANSCRIPT_FILE,    peptideMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
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
  zMapAssert(feature->type == ZMAPSTYLE_MODE_TRANSCRIPT) ;

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
      zMapAssertNotReached() ;				    /* exits... */
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
	  dumpFASTA(menu_data->window, ZMAPFASTA_SEQTYPE_AA,
		    zMapPeptideSequence(peptide), seq_name,
		    pep_length, molecule_type, gene_name) ;
	}

      zMapPeptideDestroy(peptide) ;

      g_free(dna) ;
    }

  g_free(menu_data) ;

  return ;
}


static void compressMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapWindowCompressMode compress_mode = (ZMapWindowCompressMode)menu_item_id  ;
  FooCanvasGroup *column_group ;

  if (menu_data->item)
    column_group = FOO_CANVAS_GROUP(menu_data->item) ;

  if(compress_mode != ZMAPWINDOW_COMPRESS_VISIBLE)
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


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void bumpToInitialCB(int menu_item_id, gpointer callback_data) // menu item commented out
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  FooCanvasGroup *column_group ;

  column_group = menuDataItemToColumn(menu_data->item);

  zmapWindowColumnBumpAllInitial(FOO_CANVAS_ITEM(column_group));

  zmapWindowFullReposition(menu_data->window);

  g_free(menu_data) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static void unbumpAllCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  FooCanvasGroup *column_group ;

  column_group = menuDataItemToColumn(menu_data->item);

  zmapWindowColumnUnbumpAll(FOO_CANVAS_ITEM(column_group));

  zmapWindowFullReposition(menu_data->window);

  g_free(menu_data) ;

  return ;
}

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

  zmapWindowFullReposition(menu_data->window) ;

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

      zmapWindowFullReposition(menu_data->window) ;
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
  zmapWindowFullReposition(menu_data->window) ;

  g_free(menu_data) ;

  return ;
}




// unhighlight the evidence features
static void hideEvidenceMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapWindowFocus focus = menu_data->window->focus;

  if(focus)
  {
      zmapWindowFocusResetType(focus,WINDOW_FOCUS_GROUP_EVIDENCE);
  }

  g_free(menu_data) ;

  return ;
}


ZMapGUIMenuItem zmapWindowMakeMenuMarkDumpOps(int *start_index_inout,
					      ZMapGUIMenuItemCallbackFunc callback_func,
					      gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, "Export _Marked",               0, NULL,       NULL},
#ifdef RDS_DONT_INCLUDE
      {ZMAPGUI_MENU_NORMAL, "Export _Marked/DNA"          , 1, dumpMenuCB, NULL},
#endif
      {ZMAPGUI_MENU_NORMAL, "Export _Marked/Features"     , 12, dumpMenuCB, NULL},
#ifdef RDS_DONT_INCLUDE
      {ZMAPGUI_MENU_NORMAL, "Export _Marked/Context"      , 3, dumpMenuCB, NULL},
#endif
      {ZMAPGUI_MENU_NONE, NULL               , 0, NULL, NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}



ZMapGUIMenuItem zmapWindowMakeMenuDumpOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, "_"CONTEXT_EXPORT_STR,                  0, NULL,       NULL},
      {ZMAPGUI_MENU_NORMAL, CONTEXT_EXPORT_STR"/DNA"          , 1, dumpMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, CONTEXT_EXPORT_STR"/Features"     , 2, dumpMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, CONTEXT_EXPORT_STR"/Context"      , 3, dumpMenuCB, NULL},
/*      {ZMAPGUI_MENU_NORMAL, CONTEXT_EXPORT_STR"/Config "      , 4, dumpMenuCB, NULL},*/
      {ZMAPGUI_MENU_NONE, NULL               , 0, NULL, NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


static void dumpMenuCB(int menu_item_id, gpointer callback_data)
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
	  dumpFASTA(menu_data->window, ZMAPFASTA_SEQTYPE_DNA, sequence, seq_name, seq_len, "DNA", NULL) ;
	else
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s", "Context contains no DNA.") ;

	break ;
      }
    case 2:
      {
	dumpFeatures(menu_data->window, NULL, feature) ;

	break ;
      }
    case 3:
      {
	dumpContext(menu_data->window) ;

	break ;
      }
    case 4:
      {
      /*    dumpConfig(); */
      break;
      }
    case 12:
      {
	if(zmapWindowMarkIsSet(menu_data->window->mark))
	  {
	    ZMapSpanStruct mark_region = {0,0};

	    zmapWindowMarkGetSequenceRange(menu_data->window->mark, &(mark_region.x1), &(mark_region.x2));

	    dumpFeatures(menu_data->window, &mark_region, feature);
	  }
      }
      break;
    default:
      zMapAssert("Coding error, unrecognised menu item number.") ; /* exits... */
      break ;
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
      {ZMAPGUI_MENU_BRANCH, "_"DEVELOPER_STR,                  0, NULL,       NULL},
      {ZMAPGUI_MENU_NORMAL, DEVELOPER_STR"/Show Style"         , 1, developerMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, DEVELOPER_STR"/Print Canvas"       , 2, developerMenuCB, NULL},
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

  feature_any = zmapWindowItemGetFeatureAny(menu_data->item);

  switch (menu_item_id)
    {
    case 1:
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
	    container = (ZMapWindowContainerFeatureSet)(menu_data->item->parent->parent) ;
	    if (container)
	      {
		ZMapFeatureTypeStyle style ;
		ZMapFeature feature ;

		feature = (ZMapFeature)feature_any ;
		style = feature->style ;

		zmapWindowShowStyle(style) ;
	      }
	  }

	break ;
      }
    case 2:
      {
	zmapWindowPrintCanvas(menu_data->window->canvas) ;

	break ;
      }
    default:
      zMapAssert("Coding error, unrecognised menu item number.") ; /* exits... */
      break ;
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
      {ZMAPGUI_MENU_NORMAL, BLIXEM_MENU_STR BLIXEM_DNA_STR " - show this column",
       BLIX_NONE, blixemMenuCB, NULL, "<shift>A"},
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
      {ZMAPGUI_MENU_NORMAL, BLIXEM_MENU_STR BLIXEM_DNA_STR " - all matches for selected features",
       BLIX_SELECTED, blixemMenuCB, NULL, "<shift>A"},
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
      {ZMAPGUI_MENU_NORMAL, BLIXEM_MENU_STR BLIXEM_DNAS_STR " - all matches for this column",
       BLIX_SET, blixemMenuCB, NULL, "A"},

      {ZMAPGUI_MENU_NORMAL, BLIXEM_MENU_STR BLIXEM_DNAS_STR " - all matches for associated columns",
       BLIX_MULTI_SETS, blixemMenuCB, NULL, "<Ctrl>A"},

      {ZMAPGUI_MENU_NONE,   NULL,                                             0, NULL,         NULL}
    } ;


  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


ZMapGUIMenuItem zmapWindowMakeMenuProteinHomolFeature(int *start_index_inout,
						      ZMapGUIMenuItemCallbackFunc callback_func,
						      gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, BLIXEM_MENU_STR BLIXEM_AA_STR " - all matches for selected features",
       BLIX_SELECTED, blixemMenuCB, NULL, "<shift>A"},
      {ZMAPGUI_MENU_NONE,   NULL,                             0, NULL,         NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

ZMapGUIMenuItem zmapWindowMakeMenuProteinHomol(int *start_index_inout,
					       ZMapGUIMenuItemCallbackFunc callback_func,
					       gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, BLIXEM_MENU_STR BLIXEM_AAS_STR " - all matches for this column",
       BLIX_SET, blixemMenuCB, NULL, "A"},

      {ZMAPGUI_MENU_NORMAL, BLIXEM_MENU_STR BLIXEM_AAS_STR " - all matches for associated columns",
       BLIX_MULTI_SETS, blixemMenuCB, NULL, "<Ctrl>A"},

      {ZMAPGUI_MENU_NONE,   NULL,                                            0, NULL,         NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


/* return string with / instead of _ */
static char * get_menu_string(GQuark set_quark,char disguise)
{
	char *p;
	char *seq_set;

	seq_set = g_strdup ((char *) g_quark_to_string(GPOINTER_TO_UINT(set_quark)));
	for(p = seq_set;*p;p++)
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
		zMapLogWarning("Can't find src data for %s\n",g_quark_to_string(fset_id));

	return q;
}


/* NOTE this function creates a Blixem BAM menu _and_ a ZMap request BAM menu
  it's called from a few places so adjust all calling code if you split this
 */
ZMapGUIMenuItem zmapWindowMakeMenuSeqData(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data)
{
  /* get a list of featuresets from the window's context_map */
  static ZMapGUIMenuItem menu = NULL;
  ZMapGUIMenuItem m;
  static int n_menu = 0;

  ItemMenuCBData cbdata  = (ItemMenuCBData) callback_data;
  GList *fs_list = cbdata->window->context_map->seq_data_featuresets;
  GList *fsl;
  int n_sets = g_list_length(fs_list);
  int i = 0;
  char * related = NULL;	/* req data from coverage */
  char * blixem_col = NULL;	/* blixem related from coverage or directly from real data */
  GQuark fset_id = 0,req_id = 0;

  if(!n_sets)	// || !zmapWindowMarkIsSet(cbdata->window->mark))
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
  	ZMapFeatureSetDesc f2c = g_hash_table_lookup(cbdata->window->context_map->featureset_2_column,GUINT_TO_POINTER(fset_id));
//zMapLogWarning("menu is_seq: %s -> %p\n",g_quark_to_string(fset_id),f2c);

  	if(f2c)
  	{
  		cbdata->req_id = f2c->column_id;
	  	blixem_col = get_menu_string(f2c->column_ID,'-');
  	}
  	else
  	{
  		zMapLogWarning("cannot find column for featureset %s",g_quark_to_string(fset_id));
  		/* rather than asserting */
  	}
  }

  if(blixem_col)
  {
	m->type = ZMAPGUI_MENU_NORMAL;
	m->name = g_strdup_printf("Blixem %s paired reads from mark", blixem_col);
	m->id = BLIX_SEQ_COVERAGE;
	m->callback_func = blixemMenuCB;
	m++;
  }

  if(related)
  {
	m->type = ZMAPGUI_MENU_NORMAL;
	m->name = g_strdup_printf("Request %s paired reads from mark", related);
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
			m->name = g_strdup("Request all paired reads from mark");
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
  m->name = g_strdup("Blixem paired reads data from mark");
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
			m->name = g_strdup_printf("Blixem paired reads data from mark/%s", fset);
			m->id = BLIX_SEQ + i;
			m->callback_func = blixemMenuCB;
			m++;
		}
	}
    }

  /* add sub menu */
  m->type = ZMAPGUI_MENU_BRANCH;
  m->name = g_strdup("Request paired reads data from mark");
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
			m->name = g_strdup_printf("Request paired reads data from mark/%s", fset);
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

	if (!zmapWindowMarkIsSet(menu_data->window->mark))
	{
		zMapMessage("You must set the mark first to select this option","");
	}
	else if(menu_item_id == REQUEST_SELECTED)
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
		block = zmapWindowItemGetFeatureBlock(container);

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
    case BLIX_SET:
      requested_homol_set = ZMAPWINDOW_ALIGNCMD_SET ;
      break;
    case BLIX_MULTI_SETS:
      requested_homol_set = ZMAPWINDOW_ALIGNCMD_MULTISET ;
      break;

    case BLIX_SEQ_COVERAGE:		/* blixem from a selected item in a coverage featureset */
	if (!zmapWindowMarkIsSet(menu_data->window->mark))
	  {
	    zMapMessage("You must set the mark first to select this option","");
	  }
	else
	  {
#warning if we ever have paired reads data in a virtual featureset we need to expand that here
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


	if (!zmapWindowMarkIsSet(menu_data->window->mark))
	  {
	    zMapMessage("You must set the mark first to select this option","");
	  }
	else
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



static void dumpFASTA(ZMapWindow window, ZMapFASTASeqType seq_type, char *sequence, char *seq_name, int seq_len,
		      char *molecule_name, char *gene_name)
{
  char *filepath = NULL ;
  GIOChannel *file = NULL ;
  GError *error = NULL ;
  char *error_prefix = "FASTA DNA dump failed:" ;
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


static void dumpFeatures(ZMapWindow window, ZMapSpan region_span, ZMapFeatureAny feature)
{
  char *filepath = NULL ;
  GIOChannel *file = NULL ;
  GError *error = NULL ;
  char *error_prefix = "Features dump failed:" ;
  ZMapFeatureBlock feature_block ;

  /* Should extend this any type.... */
  zMapAssert(feature->struct_type == ZMAPFEATURE_STRUCT_FEATURESET
	     || feature->struct_type == ZMAPFEATURE_STRUCT_FEATURE) ;

  /* Find the block from whatever pointer we are sent...  */
  feature_block = (ZMapFeatureBlock)zMapFeatureGetParentGroup(feature, ZMAPFEATURE_STRUCT_BLOCK);

  if (!(filepath = zmapGUIFileChooser(gtk_widget_get_toplevel(window->toplevel),
				      "Feature Dump filename ?", NULL, "gff"))
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

static void dumpContext(ZMapWindow window)
{
  char *filepath = NULL ;
  GIOChannel *file = NULL ;
  GError *error = NULL ;
  char *error_prefix = "Feature context dump failed:" ;

  if (!(filepath = zmapGUIFileChooser(window->toplevel, "Context Dump filename ?", NULL, "zmap"))
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
  ZMapFeatureStructType feature_type = ZMAPFEATURE_STRUCT_INVALID;
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
      zMapAssertNotReached();
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
  zMapAssert(window);
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
	  zMapAssertNotReached() ;
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

