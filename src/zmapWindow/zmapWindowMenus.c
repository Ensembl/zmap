/*  File: zmapWindowMenus.c
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
 * Description: Code implementing the menus for sequence display.
 *
 * Exported functions: ZMap/zmapWindows.h
 *              
 * HISTORY:
 * Last edited: Apr  4 17:38 2007 (edgrif)
 * Created: Thu Mar 10 07:56:27 2005 (edgrif)
 * CVS info:   $Id: zmapWindowMenus.c,v 1.30 2007-04-05 14:20:45 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFASTA.h>
#include <ZMap/zmapGFF.h>
#include <ZMap/zmapPeptide.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>


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


static void configureMenuCB(int menu_item_id, gpointer callback_data) ;
static void bumpMenuCB(int menu_item_id, gpointer callback_data) ;
static void bumpToggleMenuCB(int menu_item_id, gpointer callback_data) ;
static void dnaMenuCB(int menu_item_id, gpointer callback_data) ;
static void peptideMenuCB(int menu_item_id, gpointer callback_data) ;
static void transcriptNavMenuCB(int menu_item_id, gpointer callback_data) ;
static void dumpMenuCB(int menu_item_id, gpointer callback_data) ;
static void blixemMenuCB(int menu_item_id, gpointer callback_data) ;

static FooCanvasGroup *getItemsColGroup(FooCanvasItem *item) ;

static void dumpFASTA(ZMapWindow window, ZMapFASTASeqType seq_type, char *seq, char *seq_name, int seq_len,
		      char *molecule_name, char *gene_name) ;
static void dumpFeatures(ZMapWindow window, ZMapFeatureAny feature) ;
static void dumpContext(ZMapWindow window) ;

static void insertSubMenus(GString *branch_point_string,
                           ZMapGUIMenuItem sub_menus,
                           ZMapGUIMenuItem item,
                           GArray **items_array);
static ZMapFeatureContextExecuteStatus alignBlockMenusDataListForeach(GQuark key, 
                                                                      gpointer data, 
                                                                      gpointer user_data,
                                                                      char **error_out);



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
 * There is some mucky stuff for setting buttons etc but its a bit unavoidable.... 
 * 
 *  */
ZMapGUIMenuItem zmapWindowMakeMenuBump(int *start_index_inout,
				       ZMapGUIMenuItemCallbackFunc callback_func,
				       gpointer callback_data, ZMapStyleOverlapMode curr_overlap)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_TOGGLE, "Column Bump",                 ZMAPOVERLAP_COMPLETE, bumpToggleMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Column Hide",                 ZMAPWWINDOWCOLUMN_HIDE,          configureMenuCB, NULL},
      {ZMAPGUI_MENU_BRANCH, "Column Bump Opts", 0, NULL, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/Feature Overlap", ZMAPOVERLAP_COMPLEX_RANGE,  bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/FMap Test Bump", ZMAPOVERLAP_ENDS_RANGE,  bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/Compact + No Interleave", ZMAPOVERLAP_NO_INTERLEAVE,  bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/Compact + Interleave", ZMAPOVERLAP_COMPLEX,  bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/Cluster",    ZMAPOVERLAP_NAME,     bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/No Overlap", ZMAPOVERLAP_OVERLAP,  bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/Bump on Start Position",    ZMAPOVERLAP_POSITION, bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/Bump everything",            ZMAPOVERLAP_SIMPLE,   bumpMenuCB, NULL},
      {ZMAPGUI_MENU_RADIO,  "Column Bump Opts/Unbump",     ZMAPOVERLAP_COMPLETE, bumpMenuCB, NULL},
      {ZMAPGUI_MENU_BRANCH, "Column Configure", 0, NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, "Column Configure/Configure This Column",  ZMAPWWINDOWCOLUMN_CONFIGURE,     configureMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Column Configure/Configure All Columns",  ZMAPWWINDOWCOLUMN_CONFIGURE_ALL, configureMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;
  ZMapGUIMenuItem item ;

  /* Set the initial toggle button correctly....make sure this stays in step with the array above.
   * NOTE logic, this button is either "no bump" or "Name + No Overlap", the latter should be
   * selectable whatever.... */
  item = &(menu[0]) ;
  if (curr_overlap == ZMAPOVERLAP_NO_INTERLEAVE || curr_overlap == ZMAPOVERLAP_COMPLEX_RANGE)
    {
      item->type = ZMAPGUI_MENU_TOGGLEACTIVE ;
      item->id = curr_overlap ;
    }
  else
    {
      item->type = ZMAPGUI_MENU_TOGGLE ;
      item->id = ZMAPOVERLAP_COMPLETE ;
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
      if (item->type == ZMAPGUI_MENU_RADIO && item->id == curr_overlap)
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


  feature = (ZMapFeature)g_object_get_data(G_OBJECT(menu_data->item), ITEM_FEATURE_DATA) ;

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
      zMapAssertNotReached() ;				    /* exits... */
      break ;
    }

  if (feature->type == ZMAPFEATURE_TRANSCRIPT)
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

      dna = zmapWindowDNAChoose(menu_data->window, menu_data->item, dialog_type) ;
    }
  else if (feature->type == ZMAPFEATURE_TRANSCRIPT)
    {
      dna = zMapFeatureGetTranscriptDNA(context, feature, spliced, cds) ;
    }
  else
    {
      dna = zMapFeatureGetFeatureDNA(context, feature) ;
    }

  if (dna)
    {
      /* Would be better to have the dna functions calculate and return this.... */
      seq_len = strlen(dna) ;


      if (menu_item_id == ZMAPCDS || menu_item_id == ZMAPTRANSCRIPT || menu_item_id == ZMAPUNSPLICED
	  || menu_item_id == ZMAPCHOOSERANGE)
	{
	  char *title ;
	  char *dna_fasta ;

	  dna_fasta = zMapFASTAString(ZMAPFASTA_SEQTYPE_DNA, seq_name, molecule_type, gene_name, seq_len, dna) ;

	  title = g_strdup_printf("%s%s%s",
				  seq_name,
				  gene_name ? ":" : "",
				  gene_name ? gene_name : "") ;
	  zMapGUIShowText(title, dna_fasta, FALSE) ;
	  g_free(title) ;

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
      {ZMAPGUI_MENU_BRANCH, "_"FEATURE_PEPTIDE_SHOW_STR, 0, NULL, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_PEPTIDE_SHOW_STR"/CDS",                    ZMAPCDS,           peptideMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, FEATURE_PEPTIDE_SHOW_STR"/transcript",             ZMAPTRANSCRIPT,    peptideMenuCB, NULL},
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
  int pep_length ;


  feature = (ZMapFeature)g_object_get_data(G_OBJECT(menu_data->item), ITEM_FEATURE_DATA) ;

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

  dna = zMapFeatureGetTranscriptDNA(menu_data->window->feature_context, feature, spliced, cds) ;

  peptide = zMapPeptideCreate(seq_name, gene_name, dna, NULL, TRUE) ;

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

  g_free(menu_data) ;

  return ;
}




/* Configure a column, may mean repositioning the other columns. */
static void configureMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapWindowColConfigureMode configure_mode = (ZMapWindowColConfigureMode)menu_item_id  ;
  FooCanvasGroup *column_group ;

  /* did user click on an item or on the column background ? */
  if (menu_data->item_cb)
    column_group = getItemsColGroup(menu_data->item) ;
  else
    column_group = FOO_CANVAS_GROUP(menu_data->item) ;

  zmapWindowColumnConfigure(menu_data->window, column_group, configure_mode) ;

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
  ZMapStyleOverlapMode bump_type = (ZMapStyleOverlapMode)menu_item_id  ;
  FooCanvasGroup *column_group ;
  FooCanvasItem *style_item ;

  if (menu_data->item_cb)
    {
      column_group = getItemsColGroup(menu_data->item) ;
    }
  else
    {
      column_group = FOO_CANVAS_GROUP(menu_data->item) ;
    }

  style_item = menu_data->item ;

  zmapWindowColumnBump(style_item, bump_type) ;

  zmapWindowNewReposition(menu_data->window) ;

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
  ZMapStyleOverlapMode bump_type = (ZMapStyleOverlapMode)menu_item_id  ;
  FooCanvasGroup *column_group ;
  FooCanvasItem *style_item ;

  if (bump_type == ZMAPOVERLAP_NO_INTERLEAVE || bump_type == ZMAPOVERLAP_COMPLEX_RANGE)
    {
      bump_type = ZMAPOVERLAP_COMPLETE ;
    }
  else
    {
      if (zmapWindowMarkIsSet(menu_data->window->mark))
	bump_type = ZMAPOVERLAP_COMPLEX_RANGE ;
      else
	bump_type = ZMAPOVERLAP_NO_INTERLEAVE ;
    }

  if (menu_data->item_cb)
    {
      column_group = getItemsColGroup(menu_data->item) ;
    }
  else
    {
      column_group = FOO_CANVAS_GROUP(menu_data->item) ;
    }


  style_item = menu_data->item ;

  zmapWindowColumnBump(style_item, bump_type) ;

  zmapWindowNewReposition(menu_data->window) ;

  g_free(menu_data) ;

  return ;
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
      {ZMAPGUI_MENU_NONE, NULL               , 0, NULL, NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


static void dumpMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeatureAny feature ;

  feature = (ZMapFeatureAny)g_object_get_data(G_OBJECT(menu_data->item), ITEM_FEATURE_DATA) ;

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
	dumpFeatures(menu_data->window, feature) ;

	break ;
      }
    case 3:
      {
	dumpContext(menu_data->window) ;

	break ;
      }
    default:
      zMapAssert("Coding error, unrecognised menu item number.") ; /* exits... */
      break ;
    }

  g_free(menu_data) ;

  return ;
}



/* JAMES HAS MADE THE POINT THAT ANNOTATORS ONLY EVER USE THE "JUST THIS TYPE" OPTION, SO
 * I'VE COMMENTED OUT THE TWO MENU OPTIONS AND REPLACED THEM WITH ONE.... */
ZMapGUIMenuItem zmapWindowMakeMenuDNAHomol(int *start_index_inout,
					   ZMapGUIMenuItemCallbackFunc callback_func,
					   gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, "Blixem (DNA alignments)",  2, blixemMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                                                          0, NULL,         NULL}
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
      {ZMAPGUI_MENU_NORMAL, "Blixem (Protein alignments)",  2, blixemMenuCB, NULL},
      {ZMAPGUI_MENU_NONE,  NULL, 0, NULL,         NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}


/* call blixem either for a single type of homology or for all homologies. */
static void blixemMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  gboolean status ;
  GPid blixem_pid;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* only one way to call blixem now but this may change... */
  switch (menu_item_id)
    {
    case 1:
      single_homol_type = FALSE ;
      break ;
    case 2:
      single_homol_type = TRUE ;
      break ;
    default:
      zMapAssert("Coding error, unrecognised menu item number.") ; /* exits... */
      break ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* some architectures have pointers for pids, this might need addressing */
  if((status = zmapWindowCallBlixem(menu_data->window, menu_data->item, &blixem_pid)))
    menu_data->window->blixem_windows = g_list_append(menu_data->window->blixem_windows, GINT_TO_POINTER(blixem_pid));

  g_free(menu_data) ;

  return ;
}





/* this needs to be a general function... */
static FooCanvasGroup *getItemsColGroup(FooCanvasItem *item)
{
  FooCanvasGroup *group = NULL ;
  ZMapWindowItemFeatureType item_feature_type ;


  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							ITEM_FEATURE_TYPE)) ;

  switch (item_feature_type)
    {
    case ITEM_FEATURE_SIMPLE:
    case ITEM_FEATURE_PARENT:
      group = zmapWindowContainerGetParent(item->parent) ;
      break ;
    case ITEM_FEATURE_CHILD:
    case ITEM_FEATURE_BOUNDING_BOX:
      group = zmapWindowContainerGetParent(item->parent->parent) ;
      break ;
    default:
      zMapAssert("Coding error, unrecognised menu item number.") ;
      break ;
    }

  return group ;
}





static void dumpFASTA(ZMapWindow window, ZMapFASTASeqType seq_type, char *sequence, char *seq_name, int seq_len,
		      char *molecule_name, char *gene_name)
{
  char *filepath = NULL ;
  GIOChannel *file = NULL ;
  GError *error = NULL ;
  char *error_prefix = "FASTA DNA dump failed:" ;

  if (!(filepath = zmapGUIFileChooser(window->toplevel, "FASTA filename ?", NULL, NULL))
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


  return ;
}


static void dumpFeatures(ZMapWindow window, ZMapFeatureAny feature)
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
  if (feature->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
    feature_block = (ZMapFeatureBlock)(feature->parent) ;
  else
    feature_block = (ZMapFeatureBlock)(feature->parent->parent) ;


  if (!(filepath = zmapGUIFileChooser(window->toplevel, "Feature Dump filename ?", NULL, "gff"))
      || !(file = g_io_channel_new_file(filepath, "w", &error))
      || !zMapGFFDump(file, feature_block, &error))
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




static void dumpContext(ZMapWindow window)
{
  char *filepath = NULL ;
  GIOChannel *file = NULL ;
  GError *error = NULL ;
  char *error_prefix = "Feature context dump failed:" ;

  if (!(filepath = zmapGUIFileChooser(window->toplevel, "Context Dump filename ?", NULL, "zmap"))
      || !(file = g_io_channel_new_file(filepath, "w", &error))
      || !zMapFeatureContextDump(file, window->feature_context, &error))
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
