/*  File: zmapViewCallBlixem.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: prepares input files in FASTA, seqbl and exblx format
 *              and then passes them to blixem for display.
 *              
 * Exported functions: see zmapView_P.h
 *              
 * HISTORY:
 * Last edited: Jan 19 10:41 2010 (edgrif)
 * Created: Thu Jun 28 18:10:08 2007 (edgrif)
 * CVS info:   $Id: zmapViewCallBlixem.c,v 1.27 2010-02-09 08:58:02 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <unistd.h>					    /* for getlogin() */
#include <string.h>					    /* for memcpy */
#include <sys/types.h>					    /* for chmod() */
#include <sys/stat.h>					    /* for chmod() */
#include <glib.h>

#include <zmapView_P.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>



#define ZMAP_BLIXEM_CONFIG "blixem"

#define BLIX_NB_PAGE_GENERAL  "General"
#define BLIX_NB_PAGE_PFETCH   "Pfetch Socket Server"
#define BLIX_NB_PAGE_ADVANCED "Advanced"

/* ARGV positions for building argv to pass to g_spawn_async*/
enum
  {
    BLX_ARGV_PROGRAM,           /* blixem */

    /* We either have a config_file or this host/port */
    BLX_ARGV_NETID_PORT_FLAG,   /* -P */
    BLX_ARGV_NETID_PORT,        /* [hostname:port] */

    BLX_ARGV_CONFIGFILE_FLAG = BLX_ARGV_NETID_PORT_FLAG,   /* -c */
    BLX_ARGV_CONFIGFILE = BLX_ARGV_NETID_PORT,             /* [filepath] */

    BLX_ARGV_RM_TMP_FILES,      /* -r */
    BLX_ARGV_START_FLAG,        /* -S */
    BLX_ARGV_START,             /* [start] */
    BLX_ARGV_OFFSET_FLAG,       /* -O */
    BLX_ARGV_OFFSET,            /* [offset] */
    BLX_ARGV_OPTIONS_FLAG,      /* -o */
    BLX_ARGV_OPTIONS,           /* [options] */
    BLX_ARGV_SEQBL_FLAG,        /* -x */
    BLX_ARGV_SEQBL,             /* [seqbl file] */
    BLX_ARGV_FASTA_FILE,        /* [fasta file] */
    BLX_ARGV_EXBLX_FILE,        /* [exblx file] */
    BLX_ARGV_ARGC               /* argc ;) */
  };


/* Control block for preparing data to call blixem. */
typedef struct BlixemDataStruct 
{
  /* user preferences for blixem */
  gboolean kill_on_exit ;				    /* TRUE => remove this blixem on
							       program exit (default). */

  gchar         *script;                              /* script to call blixem standalone */

  char          *config_file ;				    /* Contains blixem config. information. */

  gchar         *netid;                               /* eg pubseq */
  int            port;                                /* eg 22100  */

  int            scope;                               /* defaults to 40000 */
  int            homol_max;                            /* score cutoff point */

  char          *opts;

  char          *tmpDir ;
  gboolean      keep_tmpfiles ;

  GIOChannel    *curr_channel;

  char          *fastAFile ;
  GIOChannel    *fasta_channel;
  char          *errorMsg;
  char          *exblxFile ;
  GIOChannel    *exblx_channel;
  char          *exblx_errorMsg;
  char          *seqbl_file ;
  GIOChannel    *seqbl_channel ;
  char          *seqbl_errorMsg ;


  ZMapView     view;


  ZMapFeature feature ;
  ZMapFeatureBlock block ;
  GHashTable *known_sequences ;				    /* Used to check if we already have a sequence. */
  GList *local_sequences ;				    /* List of any sequences held in
							       server and not in pfetch. */


  /* Used for processing individual features. */
  ZMapStyleMode required_feature_type ;			    /* specifies what type of feature
							     * needs to be processed. */


  /* User can specify sets of homologies that can be passed to blixem, if the set they selected
   * is in one of these sets then all the features in all the sets are sent to blixem. */
  ZMapHomolType align_type ;				    /* What type of alignment are we doing ? */

  ZMapViewBlixemAlignSet align_set ;			    /* Which set of alignments ? */

  GList *dna_sets ;
  GList *protein_sets ;
  GList *transcript_sets ;


  int offset ;
  int min, max ;
  int qstart, qend ; 
  GString *line ;

  GList *align_list ;

} blixemDataStruct, *blixemData ;


/* Holds just the config data, some of which is user configurable. */
typedef struct
{
  gboolean init ;					    /* TRUE when struct has been initialised. */

  /* User configurable */
  gboolean kill_on_exit ;				    /* TRUE => remove this blixem on
							       program exit (default). */
  gchar         *script ;				    /* script to call blixem standalone */
  gchar         *config_file ;				    /* file containing blixem config. */

  gchar         *netid ;				    /* eg pubseq */
  int           port ;					    /* eg 22100  */

  int           scope ;					    /* defines range over which aligns are
							       collected to send to blixem, defaults to 40000 */
  int           homol_max ;				    /* defines max. number of aligns sent
							       to blixem. */
  gboolean      keep_tmpfiles ;

  /* Not user configurable */
  GList *dna_sets ;
  GList *protein_sets ;
  GList *transcript_sets ;

} BlixemConfigDataStruct, *BlixemConfigData ;




static gboolean initBlixemData(ZMapView view, ZMapFeature feature, blixemData blixem_data, char **err_msg) ;
static gboolean addFeatureDetails(blixemData blixem_data) ;
static gboolean buildParamString (blixemData blixem_data, char **paramString);
static void     freeBlixemData   (blixemData blixem_data);

static void setPrefs(BlixemConfigData curr_prefs, blixemData blixem_data) ;
static gboolean getUserPrefs(BlixemConfigData prefs) ;

static void checkForLocalSequence(gpointer key, gpointer data, gpointer user_data) ;
static gboolean makeTmpfiles(blixemData blixem_data) ;
gboolean makeTmpfile(char *tmp_dir, char *file_prefix, char **tmp_file_name_out) ;
static gboolean setTmpPerms(char *path, gboolean directory) ;

static void processSetList(gpointer data, gpointer user_data) ;

static void writeHashEntry(gpointer key, gpointer data, gpointer user_data) ;
static void writeListEntry(gpointer data, gpointer user_data) ;

static gboolean writeFastAFile(blixemData blixem_data);

static gboolean writeExblxSeqblFiles(blixemData blixem_data);

static void writeExblxSeqblLine(ZMapFeature feature, blixemData  blixem_data) ;

static gboolean printAlignment(ZMapFeature feature, blixemData  blixem_data) ;
static gboolean printTranscript(ZMapFeature feature, blixemData  blixem_data) ;
static gboolean printLine(blixemData blixem_data, char *line) ;
static gboolean processExons(blixemData blixem_data, ZMapFeature feature, gboolean cds_only) ;

static int findFeature(gconstpointer a, gconstpointer b) ;
static void freeSequences(gpointer data, gpointer user_data_unused) ;

static ZMapGuiNotebookChapter makeChapter(ZMapGuiNotebook note_book_parent) ;
static void readChapter(ZMapGuiNotebookChapter chapter) ;
static void saveCB(ZMapGuiNotebookAny any_section, void *user_data_unused);
static void cancelCB(ZMapGuiNotebookAny any_section, void *user_data) ;
static void applyCB(ZMapGuiNotebookAny any_section, void *user_data) ;

static void getSetList(gpointer data, gpointer user_data) ;
static void getFeatureCB(gpointer key, gpointer data, gpointer user_data) ;
static gint scoreOrderCB(gconstpointer a, gconstpointer b) ;



/* Global for hanging on to current configuration. */
static BlixemConfigDataStruct blixem_config_curr_G = {FALSE} ;


static gboolean debug_G = TRUE ;



/* Returns a ZMapGuiNotebookChapter containing all user settable blixem resources. */
ZMapGuiNotebookChapter zMapViewBlixemGetConfigChapter(ZMapGuiNotebook note_book_parent)
{
  ZMapGuiNotebookChapter chapter = NULL ;
  gboolean status = TRUE ;


  /* If the current configuration has not been set yet then read stuff from the config file. */
  if (!blixem_config_curr_G.init)
    {
      status = getUserPrefs(&blixem_config_curr_G);
      if(!status)        
      {
            /* ensure data struct is safe 
             * -> if read fails then options are zero
             * -> if option not configured likewise
             * make the dialog even if options not set so that they can fix it
             */
             
             // null values for strings should be ok, they are programmed by low level cGUINotebook functions

      }
    }

  chapter = makeChapter(note_book_parent) ; // mh17: this uses blixen_config_curr_G
  return chapter ;
}

gboolean zMapViewBlixemGetConfigFunctions(ZMapView view, gpointer *edit_func, 
					  gpointer *apply_func, gpointer save_func, gpointer *data)
{
  
  return TRUE;
}

gboolean zmapViewBlixemLocalSequences(ZMapView view, ZMapFeature feature, GList **local_sequences_out)
{
  gboolean status = TRUE ;
  blixemDataStruct blixem_data = {0} ;
  char *err_msg = "error in zmapViewCallBlixem()" ;
  ZMapFeatureSet feature_set ;


  zMapAssert(view && zMapFeatureIsValid((ZMapFeatureAny)feature)) ;

  status = initBlixemData(view, feature, &blixem_data, &err_msg) ;

  blixem_data.errorMsg = NULL ;

  feature_set = (ZMapFeatureSet)(feature->parent) ;

  /* There is no way to interrupt g_hash_table_foreach(), so instead,
   * if printLine() encounters a problem, we store the error message
   * in blixem_data and skip all further processing, displaying the
   * error when we get back to writeExblxSeqblFile().  */
  
  /* Do requested Homology data first. */
  blixem_data.required_feature_type = ZMAPSTYLE_MODE_ALIGNMENT ;


  blixem_data.align_type = feature->feature.homol.type ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* CURRENTLY I'M NOT SUPPORTING SETS OF STUFF...COME TO THAT LATER.... */

  if (blixem_data->align_type == ZMAPHOMOL_N_HOMOL)
    set_list = blixem_data->dna_sets ;
  else
    set_list = blixem_data->protein_sets ;


  if (set_list && (g_list_find(set_list, GUINT_TO_POINTER(feature_set->unique_id))))
    {
      g_list_foreach(set_list, processSetList, blixem_data) ;
    }
  else
    {
      g_hash_table_foreach(feature_set->features, writeHashEntry, blixem_data) ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* Make a list of all the sequences held locally in the database. */
  blixem_data.known_sequences = g_hash_table_new(NULL, NULL) ;

  g_hash_table_foreach(feature_set->features, checkForLocalSequence, &blixem_data) ;

  g_hash_table_destroy(blixem_data.known_sequences) ;
  blixem_data.known_sequences = NULL ;


  /* Return result if all ok and we found some local sequences.... */
  if (status)
    {
      if (blixem_data.local_sequences)
	{
	  *local_sequences_out = blixem_data.local_sequences ;
	  blixem_data.local_sequences = NULL ;		    /* So its not free'd by freeBlixemData. */
	}
      else
	status = FALSE ;
    }

  freeBlixemData(&blixem_data) ;

  return status ;
}


/* Calls blixem to display alignments to the context block sequence from the feature set given by
 * the parameter, some of the sequences of these alignments are given by the local_sequences
 * parameter, the others blixem will fetch using the pfetch program.
 * 
 * The function returns TRUE if blixem was successfully launched and also returns the pid of the blixem
 * process so that the blixems can be cleared up when the view exits.
 *  */
gboolean zmapViewCallBlixem(ZMapView view, ZMapFeature feature, GList *local_sequences,
			    ZMapViewBlixemAlignSet align_set, GPid *child_pid, gboolean *kill_on_exit)
{
  gboolean status = TRUE ;
  char *argv[BLX_ARGV_ARGC + 1] = {NULL} ;
  blixemDataStruct blixem_data = {0} ;
  char *err_msg = "error in zmapViewCallBlixem()" ;


  zMapAssert(view && zMapFeatureIsValid((ZMapFeatureAny)feature) && child_pid) ;

  status = initBlixemData(view, feature, &blixem_data, &err_msg) ;

  blixem_data.local_sequences = local_sequences ;

  blixem_data.align_set = align_set;

  if (status)
    status = makeTmpfiles(&blixem_data) ;

  if (status)
    {
      status = buildParamString(&blixem_data, &argv[0]);
    }

  if (status)
    status = writeExblxSeqblFiles(&blixem_data);

  if (status)
    status = writeFastAFile(&blixem_data);


  /* Finally launch blixem passing it the temporary files. */
  if (status)
    {
      char *cwd = NULL, **envp = NULL; /* inherit from parent */
      GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
      GSpawnChildSetupFunc pre_exec = NULL;
      gpointer pre_exec_data = NULL;
      GPid spawned_pid;
      GError *error = NULL;
      
      argv[BLX_ARGV_PROGRAM] = g_strdup_printf("%s", blixem_data.script);


      if (debug_G)
	{
	  char **my_argp = argv ;

	  printf("Blix args: ") ;

	  while (*my_argp)
	    {
	      printf("%s ", *my_argp) ;

	      my_argp++ ;
	    }

	  printf("\n") ;
	}



      if(!(g_spawn_async(cwd, &argv[0], envp, flags, pre_exec, pre_exec_data, &spawned_pid, &error)))
        {
          status = FALSE;
          err_msg = error->message;
        }
      else
        zMapLogMessage("Blixem process spawned successfully. PID = '%d'", spawned_pid);

      if (status && child_pid)
        *child_pid = spawned_pid ;

      if (kill_on_exit)
	*kill_on_exit = blixem_data.kill_on_exit ;
    }

  freeBlixemData(&blixem_data) ;

  if (!status)
    zMapShowMsg(ZMAP_MSG_WARNING, err_msg) ;

  return status ;
}




/* 
 *                     Internal routines.
 */



static gboolean initBlixemData(ZMapView view, ZMapFeature feature, blixemData blixem_data, char **err_msg)
{
  gboolean status = TRUE ;
  ZMapFeatureBlock block = NULL;

  blixem_data->view  = view ;
  blixem_data->feature = feature ;

  block = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_BLOCK) ;
  zMapAssert(block) ;
  blixem_data->block = block ;

  if (!(zMapFeatureBlockDNA(block, NULL, NULL, NULL)))
    {
      status = FALSE;
      *err_msg = "No DNA attached to feature's parent so cannot call blixem." ;
    }

  if (status)
    {
      if ((blixem_config_curr_G.init) || (status = getUserPrefs(&blixem_config_curr_G)))
	setPrefs(&blixem_config_curr_G, blixem_data) ;
    }

  if (status)
    status = addFeatureDetails(blixem_data) ;

  return status ;
}



static void freeBlixemData(blixemData blixem_data)
{
  g_free(blixem_data->tmpDir);
  g_free(blixem_data->fastAFile);
  g_free(blixem_data->exblxFile);
  g_free(blixem_data->seqbl_file);

  g_free(blixem_data->netid);
  g_free(blixem_data->script);
  g_free(blixem_data->config_file);


  if (blixem_data->dna_sets)
    g_list_free(blixem_data->dna_sets) ;
  if (blixem_data->protein_sets)
    g_list_free(blixem_data->protein_sets) ;
  if (blixem_data->transcript_sets)
    g_list_free(blixem_data->transcript_sets) ;

  if (blixem_data->local_sequences)
    {
      g_list_foreach(blixem_data->local_sequences, freeSequences, NULL) ;
      g_list_free(blixem_data->local_sequences) ;
    }

  return ;
}

/* Set blixem_data from current user preferences. */
static void setPrefs(BlixemConfigData curr_prefs, blixemData blixem_data)
{
  g_free(blixem_data->netid);
  blixem_data->netid = g_strdup(curr_prefs->netid) ;

  blixem_data->port = curr_prefs->port ;

  g_free(blixem_data->script);
  blixem_data->script = g_strdup(curr_prefs->script) ;

  g_free(blixem_data->config_file);
  blixem_data->config_file = g_strdup(curr_prefs->config_file) ;

  if (curr_prefs->scope > 0)
    blixem_data->scope = curr_prefs->scope ;
  if (curr_prefs->homol_max > 0)
    blixem_data->homol_max = curr_prefs->homol_max ;
  blixem_data->keep_tmpfiles = curr_prefs->keep_tmpfiles ;
  blixem_data->kill_on_exit = curr_prefs->kill_on_exit ;

  if (blixem_data->dna_sets)
    g_list_free(blixem_data->dna_sets) ;
  if (curr_prefs->dna_sets)
    blixem_data->dna_sets = zMapFeatureCopyQuarkList(curr_prefs->dna_sets) ;

  if (blixem_data->protein_sets)
    g_list_free(blixem_data->protein_sets) ;
  if (curr_prefs->protein_sets)
    blixem_data->protein_sets = zMapFeatureCopyQuarkList(curr_prefs->protein_sets) ;

  if (blixem_data->transcript_sets)
    g_list_free(blixem_data->transcript_sets) ;
  if (curr_prefs->transcript_sets)
    blixem_data->transcript_sets = zMapFeatureCopyQuarkList(curr_prefs->transcript_sets) ;

  return ;
}

static gboolean getUserPrefs(BlixemConfigData prefs)
{
  ZMapConfigIniContext context = NULL;
  gboolean status = FALSE;
  
  if((context = zMapConfigIniContextProvide()))
    {
      char *tmp_string = NULL;
      int tmp_int;
      gboolean tmp_bool;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				       ZMAPSTANZA_BLIXEM_NETID, &tmp_string))
	prefs->netid = tmp_string;

      if(zMapConfigIniContextGetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				    ZMAPSTANZA_BLIXEM_PORT, &tmp_int))
	prefs->port = tmp_int;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				       ZMAPSTANZA_BLIXEM_SCRIPT, &tmp_string))
	prefs->script = tmp_string;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				       ZMAPSTANZA_BLIXEM_CONF_FILE, &tmp_string))
	prefs->config_file = tmp_string;

      if(zMapConfigIniContextGetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				    ZMAPSTANZA_BLIXEM_SCOPE, &tmp_int))
	prefs->scope = tmp_int;

      if(zMapConfigIniContextGetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				    ZMAPSTANZA_BLIXEM_MAX, &tmp_int))
	prefs->homol_max = tmp_int;

      if(zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
					ZMAPSTANZA_BLIXEM_KEEP_TEMP, &tmp_bool))
	prefs->keep_tmpfiles = tmp_bool;

      if(zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
					ZMAPSTANZA_BLIXEM_KILL_EXIT, &tmp_bool))
	prefs->kill_on_exit = tmp_bool;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				       ZMAPSTANZA_BLIXEM_DNA_FS, &tmp_string))
	{
	  prefs->dna_sets = zMapFeatureString2QuarkList(tmp_string);
	  g_free(tmp_string);
	}

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				       ZMAPSTANZA_BLIXEM_PROT_FS, &tmp_string))
	{
	  prefs->protein_sets = zMapFeatureString2QuarkList(tmp_string);
	  g_free(tmp_string);
	}

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				       ZMAPSTANZA_BLIXEM_TRANS_FS, &tmp_string))
	{
	  prefs->transcript_sets = zMapFeatureString2QuarkList(tmp_string);
	  g_free(tmp_string);
	}
      zMapConfigIniContextDestroy(context);
    }

  if((prefs->script) && 
     ((prefs->config_file) || 
      (prefs->netid && prefs->port)))
    {
      char *tmp;
      tmp = prefs->script;

      if (!(prefs->script = g_find_program_in_path(tmp)))
  	    zMapShowMsg(ZMAP_MSG_WARNING, 
		    "Either can't locate \"%s\" in your path or it is not executable by you.",
		    tmp) ;
      g_free(tmp) ;

      if (prefs->config_file && !zMapFileAccess(prefs->config_file, "rw"))
	    zMapShowMsg(ZMAP_MSG_WARNING, 
		    "Either can't locate \"%s\" in your path or it is not read/writeable.",
		    prefs->config_file) ;
    }
  else
    zMapShowMsg(ZMAP_MSG_WARNING, "Some or all of the compulsory blixem parameters "
		"(\"netid\", \"port\") or config_file or \"script\" are missing from your config file.");

  if(prefs->script && prefs->config_file)
     status = TRUE;

  prefs->init = TRUE;

  return status;
}

static gboolean addFeatureDetails(blixemData blixem_data)
{
  gboolean status = TRUE;
  ZMapFeature feature;
  int scope = 40000 ;					    /* can be set from user prefs */
  int x1, x2;
  int origin ;
  int length;

  if (blixem_data->scope > 0)
    scope = blixem_data->scope ;

  feature = blixem_data->feature ;

  x1 = feature->x1 ;
  x2 = feature->x2 ;

  /* Set min/max range for blixem, clamp to our sequence. */
  blixem_data->min = x1 - (scope / 2) ;
  blixem_data->max = x2 + (scope / 2) ;
  if (blixem_data->min < 1)
    blixem_data->min = 1 ;
//  if (blixem_data->max > blixem_data->view->features->length)
//    blixem_data->max = blixem_data->view->features->length ;
  length = blixem_data->block->block_to_sequence.t2 - blixem_data->block->block_to_sequence.t1 + 1;
  if (blixem_data->max > length)
    blixem_data->max = length ;


  /* Frame is calculated simply from where the start/end of the scope is on the sequence for forward/reverse,
   * we correct the scope start (blixem_data->min/max) so that it is in the same frame as the
   * sequence start so that objects always have the same frame no matter where the user clicks on
   * zmap. */
  {
    int start, correction ;

    start = blixem_data->min - blixem_data->block->block_to_sequence.q1 ; /* Block coords may not start at 1. */

    if ((correction = 3 - (start % 3)) < 3)
      blixem_data->min += correction ;


    start = blixem_data->block->block_to_sequence.q2 - blixem_data->max + 1 ;

    if ((correction = 3 - (start % 3)) < 3)
      blixem_data->max -= correction ;
  }


  /* Offset is "+ 2" because acedb uses  seq_length + 1  for revcomp for its own obsure reasons and blixem
   * has inherited this. I don't know why but don't want to change anything in blixem. */
  if (blixem_data->view->revcomped_features)
    origin = blixem_data->block->block_to_sequence.q2 + 2 ;
  else
    origin = 1 ;
  blixem_data->offset = blixem_data->min - origin ;


  if (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)  /* protein */
    {
      if (feature->strand == ZMAPSTRAND_REVERSE)
	blixem_data->opts = "X-BR";
      else
	blixem_data->opts = "X+BR";
    }
  else
    blixem_data->opts = "N+BR";                     /* dna */
      

  return status ;
}


/* Make the temporary files which are the sole input to the blixem program to get it to
 * display alignments.
 * 
 * The directory and files are created so the user has write access but all others
 * can read.
 *  */
static gboolean makeTmpfiles(blixemData blixem_data)
{
  gboolean    status = TRUE;
  char       *path;
  char       *login;

  if ((login = (char *)g_get_user_name()))
    {
      path = g_strdup_printf("/tmp/%s_ZMAP_BLIXEM/", login);
    }
  else
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not determine your login.");
      status = FALSE;
    }

  if (status)
    {
      if (!(blixem_data->tmpDir = zMapGetDir(path, FALSE, TRUE)))
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not create temp directory for Blixem.") ;
	  status = FALSE;
	}
      else
	{
	  status = setTmpPerms(blixem_data->tmpDir, TRUE) ;
	}
    }

  if (path)
    g_free(path) ;

  /* Create the file to the hold the DNA in FastA format. */
  if (status)
    {
      if ((status = makeTmpfile(blixem_data->tmpDir, "fasta", &(blixem_data->fastAFile))))
	status = setTmpPerms(blixem_data->fastAFile, FALSE) ;
    }

  /* Create the file to hold the features in exblx format. */
  if (status)
    {
      if ((status = makeTmpfile(blixem_data->tmpDir, "exblx_x", &(blixem_data->exblxFile))))
	status = setTmpPerms(blixem_data->exblxFile, FALSE) ;
    }
     
  /* Create the file to hold the features in seqbl format. */
  if (status)
    {
      if ((status = makeTmpfile(blixem_data->tmpDir, "seqbl_x", &(blixem_data->seqbl_file))))
	status = setTmpPerms(blixem_data->seqbl_file, FALSE) ;
    }
     
  return status ;
}

/* A very noddy routine to set good permissions on our tmp directory/files... */
static gboolean setTmpPerms(char *path, gboolean directory)
{
  gboolean status = TRUE ;
  mode_t mode ;

  if (directory)
    mode = (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) ;
  else
    mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) ;

  if (chmod(path, mode) != 0)
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not set permissions Blixem temp dir/file:  %s.", path) ;
      status = FALSE;
    }

  return status ;
}



static gboolean buildParamString(blixemData blixem_data, char **paramString)
{
  gboolean    status = TRUE;
  int         start;


  /* The acedb code that fires up blixem tells blixem to start its display a little to the
   * left of the start of the target homology so we do the same...a TOTAL HACK should be
   * encapsulated in blixem itself, it should know to position the feature centrally.... */
  start = blixem_data->feature->x1 - blixem_data->min - 30 ;

  {
     int missed = 0;            /* keep track of options we don't specify */

     /* we need to do this as blixem has pretty simple argv processing */

     if (blixem_data->config_file)
       {
	 paramString[BLX_ARGV_CONFIGFILE_FLAG] = g_strdup("-c");
	 paramString[BLX_ARGV_CONFIGFILE]      = g_strdup_printf("%s", blixem_data->config_file) ;
       }
     else if (blixem_data->netid && blixem_data->port)
       {
	 paramString[BLX_ARGV_NETID_PORT_FLAG] = g_strdup("-P");
	 paramString[BLX_ARGV_NETID_PORT]      = g_strdup_printf("%s:%d", blixem_data->netid, blixem_data->port);
       }
     else
       missed += 2;

     /* For testing purposes remove the "-r" flag to leave the temporary files. 
      * (keep_tempfiles = true in blixem stanza of ZMap file) */
     if (!blixem_data->keep_tmpfiles)
       paramString[BLX_ARGV_RM_TMP_FILES - missed] = g_strdup("-r");
     else
       missed += 1;

     if (start)
       {
         paramString[BLX_ARGV_START_FLAG - missed] = g_strdup("-S");
         paramString[BLX_ARGV_START - missed]      = g_strdup_printf("%d", start);
       }
     else
       missed += 2;
     
     if (blixem_data->offset)
       {
         paramString[BLX_ARGV_OFFSET_FLAG - missed] = g_strdup("-O");
         paramString[BLX_ARGV_OFFSET - missed]      = g_strdup_printf("%d", blixem_data->offset);
       }
     else
       missed += 2;

     if (blixem_data->opts)
       {
         paramString[BLX_ARGV_OPTIONS_FLAG - missed] = g_strdup("-o");
         paramString[BLX_ARGV_OPTIONS - missed]      = g_strdup_printf("%s", blixem_data->opts);
       }
     else 
       missed += 2;

     if (blixem_data->seqbl_file)
       {
         paramString[BLX_ARGV_SEQBL_FLAG - missed] = g_strdup("-x");
	 paramString[BLX_ARGV_SEQBL - missed] = g_strdup_printf("%s", blixem_data->seqbl_file);
       }
     else
       missed += 2;

     paramString[BLX_ARGV_FASTA_FILE - missed] = g_strdup_printf("%s", blixem_data->fastAFile);
     paramString[BLX_ARGV_EXBLX_FILE - missed] = g_strdup_printf("%s", blixem_data->exblxFile);
   }

  return status ;
}


static gboolean writeExblxSeqblFiles(blixemData blixem_data)
{
  GError  *channel_error = NULL ;
  gboolean status = TRUE ;
  gboolean seqbl_file = FALSE ;

  if (blixem_data->local_sequences)
    seqbl_file = TRUE ;

  /* We just use the one gstring as a reusable buffer to format all output. */
  blixem_data->line = g_string_new("") ;

  /* Open the exblx file, always needed. */
  if ((blixem_data->exblx_channel = g_io_channel_new_file(blixem_data->exblxFile, "w", &channel_error)))
    {
      g_string_printf(blixem_data->line, "# exblx_x\n# blast%c\n", *blixem_data->opts) ;
      blixem_data->curr_channel = blixem_data->exblx_channel ;
      status = printLine(blixem_data, blixem_data->line->str) ;
      blixem_data->line = g_string_truncate(blixem_data->line, 0) ;
    }
  else
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not open exblx file: %s",
		  channel_error->message) ;
      g_error_free(channel_error) ;
      channel_error = NULL;
      status = FALSE ;
    }

  /* Open the seqbl file, if needed. */
  if (status)
    {
      if (seqbl_file)
	{
	  if ((blixem_data->seqbl_channel = g_io_channel_new_file(blixem_data->seqbl_file, "w", &channel_error)))
	    {
	      g_string_printf(blixem_data->line, "# seqbl_x\n# blast%c\n", *blixem_data->opts) ;
	      blixem_data->curr_channel = blixem_data->seqbl_channel ;
	      status = printLine(blixem_data, blixem_data->line->str) ;
	      blixem_data->line = g_string_truncate(blixem_data->line, 0) ;
	    }
	  else
	    {
	      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not open seqbl file: %s",
			  channel_error->message) ;
	      g_error_free(channel_error) ;
	      channel_error = NULL;
	      status = FALSE ;
	    }
	}
    }

  if (status)
    {
      ZMapFeature feature = blixem_data->feature ;
      ZMapFeatureSet feature_set ;
      GList *set_list = NULL ;
      
      blixem_data->errorMsg = NULL ;

      feature_set = (ZMapFeatureSet)(feature->parent) ;


      /* 
       * Do requested Homology data first.
       */
      blixem_data->required_feature_type = ZMAPSTYLE_MODE_ALIGNMENT ;

      blixem_data->align_type = feature->feature.homol.type ;

      /* Be better even for single feature to have it as a list and put it through some common
       * code...and same for homol_max...just treat it all in one way.... */

      if (blixem_data->homol_max || (blixem_data->align_set != BLIXEM_NO_MATCHES))
	{
	  /* Do a homol max list here.... */
	  int num_homols = 0 ;

	  blixem_data->align_list = NULL ;

	  if (blixem_data->align_set == BLIXEM_FEATURE_SINGLE_MATCH)
	    {
	      blixem_data->align_list = g_list_append(blixem_data->align_list, feature) ;
	    }
	  else if (blixem_data->align_set == BLIXEM_FEATURE_ALL_MATCHES)
	    {
	      blixem_data->align_list = zMapFeatureSetGetNamedFeatures(feature_set, feature->original_id) ;
	    }
	  else if (blixem_data->align_set == BLIXEM_FEATURESET_MATCHES)
	    {
	      g_hash_table_foreach(feature_set->features, getFeatureCB, blixem_data) ;
	    }
	  else if (blixem_data->align_set == BLIXEM_MULTI_FEATURESET_MATCHES)
	    {
	      if (blixem_data->align_type == ZMAPHOMOL_N_HOMOL)
		set_list = blixem_data->dna_sets ;
	      else
		set_list = blixem_data->protein_sets ;

	      g_list_foreach(set_list, getSetList, blixem_data) ;
	    }

	  if (blixem_data->homol_max)
	    {
	      if ((num_homols = g_list_length(blixem_data->align_list)) && blixem_data->homol_max < num_homols)
		{
		  GList *break_point ;
	      
		  blixem_data->align_list = g_list_sort(blixem_data->align_list, scoreOrderCB) ;

		  break_point = g_list_nth(blixem_data->align_list, blixem_data->homol_max + 1) ;
							    /* "+ 1" to go past last homol. */

		  /* Now remove entries.... */
		  if ((break_point = zMap_g_list_split(blixem_data->align_list, break_point)))
		    g_list_free(break_point) ;
		}
	    }

	  g_list_foreach(blixem_data->align_list, writeListEntry, blixem_data) ;
	}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      else
	{
	  /* No homol_max...is this ever called ? */

	  if (set_list && (g_list_find(set_list, GUINT_TO_POINTER(feature_set->unique_id))))
	    {
	      g_list_foreach(set_list, processSetList, blixem_data) ;
	    }
	  else
	    {
	      g_hash_table_foreach(feature_set->features, writeHashEntry, blixem_data) ;
	    }
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      /* 
       * Now do transcripts (may need to filter further...)
       */
      blixem_data->required_feature_type = ZMAPSTYLE_MODE_TRANSCRIPT ;
      if (blixem_data->transcript_sets)
	{
	  g_list_foreach(blixem_data->transcript_sets, processSetList, blixem_data) ;
	}
      else
	{
	  g_hash_table_foreach(blixem_data->block->feature_sets, writeHashEntry, blixem_data) ;
	}
    }


  /* If there was an error writing the data to the file it will
   * be recorded in blixem_data->errorMsg and no further processing will have occurred. */
  if (blixem_data->errorMsg)
    {
      zMapShowMsg(ZMAP_MSG_WARNING, blixem_data->errorMsg);
      status = FALSE;
      g_free(blixem_data->errorMsg);
    }


  /* Shut the two files if they are open. */
  if (blixem_data->exblx_channel)
    {
      g_io_channel_shutdown(blixem_data->exblx_channel, TRUE, &channel_error);
      if (channel_error)
	g_free(channel_error);
    }

  if (blixem_data->seqbl_channel)
    {
      g_io_channel_shutdown(blixem_data->seqbl_channel, TRUE, &channel_error);
      if (channel_error)
	g_free(channel_error);
    }


  /* Free our string buffer. */
  g_string_free(blixem_data->line, TRUE) ;
  blixem_data->line = NULL ;

  return status ;
}


/* A GFunc() to step through the named feature sets and write them out for passing
 * to blixem. */
static void processSetList(gpointer data, gpointer user_data)
{
  GQuark set_id = GPOINTER_TO_UINT(data) ;
  blixemData blixem_data = (blixemData)user_data ;
  ZMapFeatureSet feature_set ;

  if (!(feature_set = g_hash_table_lookup(blixem_data->block->feature_sets, GINT_TO_POINTER(set_id))))
    {
      zMapLogWarning("Could not find %s feature set \"%s\" in context feature sets.",
		     (blixem_data->required_feature_type == ZMAPSTYLE_MODE_ALIGNMENT
		      ? "alignment" : "transcript"),
		     g_quark_to_string(set_id)) ;
    }
  else
    {
      g_hash_table_foreach(feature_set->features, writeHashEntry, blixem_data);
    }

  return ;
}


/* These two functions enable writeExblxSeqblLine() to be called from a g_hash or a g_list
 * foreach function. */
static void writeHashEntry(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  blixemData  blixem_data = (blixemData)user_data ;

  writeExblxSeqblLine(feature, blixem_data) ;

  return ;
}

static void writeListEntry(gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  blixemData  blixem_data = (blixemData)user_data ;

  writeExblxSeqblLine(feature, blixem_data) ;

  return ;
}



static void writeExblxSeqblLine(ZMapFeature feature, blixemData  blixem_data)
{

  /* There is no way to interrupt g_hash_table_foreach() so instead if errorMsg is set
   * then it means there was an error in processing so we don't process any
   * more records, displaying the error when we return. */
  if (!blixem_data->errorMsg)
    {  
      /* Only process features we want to dump. We then filter those features according to the
       * following rules (inherited from acedb): alignment features must be wholly within the
       * blixem max/min to be included, for transcripts we include as many introns/exons as will fit. */
      if (feature->type == blixem_data->required_feature_type)
	{
	  gboolean status = TRUE ;

	  switch (feature->type)
	    {
	    case ZMAPSTYLE_MODE_ALIGNMENT:
	      {
		if (feature->feature.homol.type == blixem_data->align_type)
		  {
		    if ((feature->x1 >= blixem_data->min && feature->x1 <= blixem_data->max)
			&& (feature->x2 >= blixem_data->min && feature->x2 <= blixem_data->max))
		      {
			if ((*blixem_data->opts == 'X' && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
			    || (*blixem_data->opts == 'N' && feature->feature.homol.type == ZMAPHOMOL_N_HOMOL))
			  status = printAlignment(feature, blixem_data) ;
		      }
		  }
		break ;
	      }
	    case ZMAPSTYLE_MODE_TRANSCRIPT:
	      {
		status = printTranscript(feature, blixem_data) ;
		break ;
	      }
	    default:
	      break ;
	    }

	  blixem_data->line = g_string_truncate(blixem_data->line, 0) ;	/* Reset string buffer. */
	}
    }

  return ;
}


static gboolean printAlignment(ZMapFeature feature, blixemData  blixem_data)
{
  gboolean status = TRUE ;
  int min, max ;
  int score = 0 ;
  int qstart, qend, sstart, send ;
  char qframe_strand, sframe_strand ;
  int qframe = 0, sframe = 0 ;
  GString *line ;
  int x1, x2 ;


  min = blixem_data->min ;
  max = blixem_data->max ;
  line = blixem_data->line ;

  x1 = feature->x1 ;
  x2 = feature->x2 ;

  /* qstart and qend are the coordinates of the current homology relative to the viewing view,
   * rather than absolute coordinates. */
  if (feature->strand == ZMAPSTRAND_REVERSE)
    {
      qstart = x2 - blixem_data->min + 1;
      qend   = x1 - blixem_data->min + 1;
    }
  else		
    {
      qstart = x1 - blixem_data->min + 1;
      qend   = x2 - blixem_data->min + 1;
    }

  /* qframe is derived from the position of the start position
   * of this homology relative to one end of the viewing view
   * or the other, depending on strand.  Mod3 gives us the odd
   * bases if that distance is not a whole number of codons. */
  if (feature->strand == ZMAPSTRAND_REVERSE)
    {
      qframe_strand = '-' ;
      qframe = 1 + (((max - min + 1) - qstart) % 3) ;
    }
  else
    {
      qframe_strand = '+' ;
      qframe = 1 + ((qstart - 1) % 3) ;
    }


  sstart = feature->feature.homol.y1 ;
  send   = feature->feature.homol.y2 ;

  if (feature->feature.homol.strand == ZMAPSTRAND_REVERSE)
    {
      sframe_strand = '-' ;

      if (feature->feature.homol.length)
	sframe = (1 + ((feature->feature.homol.length) - sstart) % 3) ;
    }
  else
    {
      sframe_strand = '+' ;

      if (feature->feature.homol.length)
	sframe = (1 + (sstart-1) % 3) ;
    }


  /* Not sure about this.... */

  // mh17: doesn't look like a kosher RevComp, but these are just numbers that get printed out so no greta danger
  // sstart/send derived from homol.y1/y2 which get set from block_to_sequence.q1,q2 which are phrased as 1,Y

  if (blixem_data->view->revcomped_features)
    {
      double tmp ;

      tmp = sstart ;
      sstart = send ;
      send = tmp ;
    }



  score = feature->score + 0.5 ;			    /* round up to integer ????????? */

  /* Is this test correct ? no I don't think so...test later.................. */
  if (score)
    {
      char *seq_str = NULL ;
      char *description = NULL ;			    /* Unsupported currently. */
      GList *list_ptr ;

      if ((list_ptr = g_list_find_custom(blixem_data->local_sequences, feature, findFeature)))
	{
	  ZMapSequence sequence = (ZMapSequence)list_ptr->data ;

	  seq_str = sequence->sequence ;
	  blixem_data->curr_channel = blixem_data->seqbl_channel ;
	}
      else
	{
	  /* In theory we should be checking for a description for non-local sequences,
	   * see acedb code in doShowAlign...don't know how important this is..... */
	  seq_str = "" ;
	  blixem_data->curr_channel = blixem_data->exblx_channel ;
	}

      g_string_printf(line, "%d\t(%c%d)\t%d\t%d\t(%c%d)\t%d\t%d\t%s", 
		      score,
		      qframe_strand, qframe,
		      qstart, qend,
		      sframe_strand, sframe,
		      sstart, send,
		      g_quark_to_string(feature->original_id)) ;

      if (feature->feature.homol.align)
	{
	  int k, index, incr ;

	  g_string_append_printf(line, "%s", "\tGaps ") ;

		      
	  if (feature->strand == ZMAPSTRAND_REVERSE)
	    {
	      index = feature->feature.homol.align->len - 1 ;
	      incr = -1 ;
	    }
	  else
	    {
	      index = 0 ;
	      incr = 1 ;
	    }

	  for (k = 0 ; k < feature->feature.homol.align->len ; k++, index += incr)
	    {
	      ZMapAlignBlockStruct gap ;

	      gap = g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, index) ;

	      if (qstart > qend)
		zMapUtilsSwop(int, gap.t1, gap.t2) ;

	      g_string_append_printf(line, " %d %d %d %d", gap.q1, gap.q2,
				     (gap.t1 - min + 1), (gap.t2 - min + 1)) ;
	    }

	  g_string_append_printf(line, "%s", " ;") ;
	}
		  
      /* In theory we should be outputting description for some files.... */
      if ((seq_str && *seq_str) || (description && *description))
	{
	  char *tag, *text ;

	  if (seq_str)
	    {
	      tag = "Sequence" ;
	      text = seq_str ;
	    }
	  else
	    {
	      tag = "Description" ;
	      text = description ;
	    }

	  g_string_append_printf(line, "\t%s %s ;", tag, text) ;
	}

      line = g_string_append(line, "\n") ;
		  
      status = printLine(blixem_data, line->str) ;
    }

  return status ;
}


static gboolean printTranscript(ZMapFeature feature, blixemData  blixem_data)
{
  gboolean status = TRUE;
  int min, max ;
  int cds_start, cds_end ;
  int score = 0, qstart, qend, sstart, send;
  ZMapSpan span = NULL;
  char *qframe, *sframe ;
  gboolean cds_only = TRUE ;


  /* For nucleotide alignments we do all transcripts, for proteins we only do those with a cds. */
  if (blixem_data->align_type == ZMAPHOMOL_N_HOMOL)
    cds_only = FALSE ;


  if (!cds_only || feature->feature.transcript.flags.cds)
    {
      blixem_data->curr_channel = blixem_data->exblx_channel ;

      /* Do the exons... */
      status = processExons(blixem_data, feature, cds_only) ;


      /* Do the introns... */
      if (status && feature->feature.transcript.introns)
	{	      
	  int i ;

	  cds_start = feature->feature.transcript.cds_start ;
	  cds_end = feature->feature.transcript.cds_end ;
	  min = blixem_data->min ;
	  max = blixem_data->max ;

	  for (i = 0; i < feature->feature.transcript.introns->len && status; i++)
	    {
	      span = &g_array_index(feature->feature.transcript.introns, ZMapSpanStruct, i);
		      
	      /* Only print introns that are within the cds section of the transcript and
	       * within the blixem scope. */
	      if ((span->x1 < min || span->x2 > max)
		  || (cds_only && (span->x1 > cds_end || span->x2 < cds_start)))
		{
		  continue ;
		}
	      else
		{
		  GString *line ;

		  line = blixem_data->line ;

		  if (feature->strand == ZMAPSTRAND_REVERSE)
		    {
		      qstart = span->x2 - min + 1;
		      qend   = span->x1 - min + 1;
		    }
		  else
		    {
		      qstart = span->x1 - min + 1;
		      qend   = span->x2 - min + 1;
		    }
		      
		  sstart = send = 0 ;
		  score  = -2 ;

		  /* Note that sframe is meaningless so is set to an invalid value. */
		  if (feature->strand == ZMAPSTRAND_REVERSE)
		    {
		      qframe = "(-1)" ;
		      sframe = "(-0)" ;
		    }
		  else
		    {
		      qframe = "(+1)" ;
		      sframe = "(-1)" ;
		    }

		  g_string_printf(line, "%d\t%s\t%d\t%d\t%s\t%d\t%d\t%si\n", 
				  score,
				  qframe, qstart, qend,
				  sframe, sstart, send,
				  g_quark_to_string(feature->original_id)) ;

		  status = printLine(blixem_data, line->str) ;
		}
	    }
	}
    }

  return status ;
}


static gboolean printLine(blixemData blixem_data, char *line)
{
  gboolean status = TRUE ;
  GError *channel_error = NULL ;
  gsize bytes_written ;

  if (g_io_channel_write_chars(blixem_data->curr_channel, line, -1, 
			       &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
    {
      /* don't display an error at this point, just write it to blixem_data for later.
       * Set and return status to prevent further processing. */
      blixem_data->errorMsg = g_strdup_printf("Error writing data to exblx file: %50s... : %s",
					      line, channel_error->message) ;
      g_error_free(channel_error) ;
      channel_error = NULL;
      status = FALSE ;
    }
    
  return status ;
}



/* A GFunc() to step through the named feature sets and make a list of the features. */
static void getSetList(gpointer data, gpointer user_data)
{
  GQuark set_id = GPOINTER_TO_UINT(data) ;
  blixemData blixem_data = (blixemData)user_data ;
  ZMapFeatureSet feature_set ;

  if (!(feature_set = g_hash_table_lookup(blixem_data->block->feature_sets, GINT_TO_POINTER(set_id))))
    {
      zMapLogWarning("Could not find %s feature set \"%s\" in context feature sets.",
		     (blixem_data->required_feature_type == ZMAPSTYLE_MODE_ALIGNMENT
		      ? "alignment" : "transcript"),
		     g_quark_to_string(set_id)) ;
    }
  else
    {
      g_hash_table_foreach(feature_set->features, getFeatureCB, blixem_data) ;
    }

  return ;
}

static void getFeatureCB(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  blixemData  blixem_data = (blixemData)user_data ;

  /* Only process features we want to dump. We then filter those features according to the
   * following rules (inherited from acedb): alignment features must be wholly within the
   * blixem max/min to be included, for transcripts we include as many introns/exons as will fit. */
  if (feature->type == blixem_data->required_feature_type)
    {
      switch (feature->type)
	{
	case ZMAPSTYLE_MODE_ALIGNMENT:
	  {
	    if (feature->feature.homol.type == blixem_data->align_type)
	      {
		if ((feature->x1 >= blixem_data->min && feature->x1 <= blixem_data->max)
		    && (feature->x2 >= blixem_data->min && feature->x2 <= blixem_data->max))
		  {
		    if ((*blixem_data->opts == 'X' && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
			|| (*blixem_data->opts == 'N' && feature->feature.homol.type == ZMAPHOMOL_N_HOMOL))
		      blixem_data->align_list = g_list_append(blixem_data->align_list, feature) ;
		  }
	      }
	    break ;
	  }
	default:
	  break ;
	}
    }

  return ;
}


/* GCompareFunc() to sort alignment features based on score. */
static gint scoreOrderCB(gconstpointer a, gconstpointer b)
  {
    gint result ;
    ZMapFeature feat_1 = (ZMapFeature)a,  feat_2 = (ZMapFeature)b ;

    if (feat_1->score < feat_2->score)
      result = 1 ;
    else if (feat_1->score > feat_2->score)
      result = -1 ;
    else
      result = 0 ;

    return result ;
}




/* Write out the dna sequence into a file in fasta format, ie a header record with the 
 * sequence name, then as many lines 50 characters long as it takes to write out the 
 * sequence. We only export the dna for min -> max.
 * 
 * THE CODING LOGIC IS HORRIBLE HERE BUT I DON'T HAVE ANY TIME TO CORRECT IT NOW, EG.
 * 
 *  */
static gboolean writeFastAFile(blixemData blixem_data)
{
  gboolean status = TRUE ;
  ZMapFeature feature = NULL ;
  ZMapFeatureBlock block = NULL ;
  gsize    bytes_written ;
  GError  *channel_error = NULL ;
  char    *line = NULL ;
  enum { FASTA_CHARS = 50 } ;
  char     buffer[FASTA_CHARS + 2] ;			    /* FASTA CHARS + \n + \0 */
  int      lines = 0, chars_left = 0 ;
  char    *cp = NULL ;
  int      i ;


  feature = blixem_data->feature ;


  if (!(block = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_BLOCK))
      || !zMapFeatureBlockDNA(block, NULL, NULL, &cp))
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: writing to file, failed to get feature's DNA");

      status = FALSE ;
    }
  else
    {
      if (!(blixem_data->fasta_channel = g_io_channel_new_file(blixem_data->fastAFile, "w", &channel_error)))
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not open fastA file: %s",
		      channel_error->message) ;
	  g_error_free(channel_error) ;
	  channel_error = NULL;
	  status = FALSE;
	}
      else
	{
	  line = g_strdup_printf(">%s\n", 
				 g_quark_to_string(blixem_data->view->features->parent_name));

	  if (g_io_channel_write_chars(blixem_data->fasta_channel, 
				       line, -1, &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
	    {
	      zMapShowMsg(ZMAP_MSG_WARNING, "Error writing header record to fastA file: %50s... : %s",
			  line, channel_error->message) ;
	      g_error_free(channel_error) ;
	      channel_error = NULL;

	      status = FALSE ;
	    }

	  g_free(line) ;
	}


      if (status)
	{
	  int total_chars ;

	  total_chars = (blixem_data->max - blixem_data->min + 1) ;

	  lines = total_chars / FASTA_CHARS ;
	  chars_left = total_chars % FASTA_CHARS ;

          cp += (blixem_data->min - 1);

	  /* Do the full length lines.                                           */
	  if (lines > 0)
	    {
	      buffer[FASTA_CHARS] = '\n' ;
	      buffer[FASTA_CHARS + 1] = '\0' ; 

	      for (i = 0 ; i < lines && status ; i++)
		{
		  memcpy(&buffer[0], cp, FASTA_CHARS) ;
		  cp += FASTA_CHARS ;

		  if (g_io_channel_write_chars(blixem_data->fasta_channel,
					       &buffer[0], -1, 
					       &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
		    status = FALSE ;
		}
	    }

	  /* Do the last line.                                                   */
	  if (status && chars_left > 0)
	    {
	      memcpy(&buffer[0], cp, chars_left) ;
	      buffer[chars_left] = '\n' ;
	      buffer[chars_left + 1] = '\0' ;

	      if (g_io_channel_write_chars(blixem_data->fasta_channel,
					   &buffer[0], -1, 
					   &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
		status = FALSE ;
	    }

	  if (!status)
	    {
	      zMapShowMsg(ZMAP_MSG_WARNING, "Error writing to fastA file: %s",
			  channel_error->message) ;
	      g_error_free(channel_error) ;
	      channel_error = NULL ;
	    }
	}

      if (g_io_channel_shutdown(blixem_data->fasta_channel, TRUE, &channel_error) != G_IO_STATUS_NORMAL)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "Error closing fastA file: %s",
		      channel_error->message) ;
	  g_error_free(channel_error) ;
	  channel_error = NULL ;
	}
    }

  return status ;
}



/* Print out the exons taking account of the extent of the CDS within the transcript. */
static gboolean processExons(blixemData blixem_data, ZMapFeature feature, gboolean cds_only)
{
  gboolean status = TRUE ;
  int i ;
  ZMapSpan span = NULL ;
  int score = -1, qstart, qend, sstart, send ;
  int min, max ;
  int exon_base ;
  int cds_start, cds_end ;				    /* Only used if cds_only == TRUE. */
  GString *line ;

  line = blixem_data->line ;

  min = blixem_data->min ;
  max = blixem_data->max ;

  if (cds_only)
    {
      cds_start = feature->feature.transcript.cds_start ;
      cds_end = feature->feature.transcript.cds_end ;
    }

  exon_base = 0 ;


  /* We need to record how far we are along the exons in CDS relative coords, i.e. for the
   * reverse strand we need to calculate from the other end of the transcript. */
  if (feature->strand == ZMAPSTRAND_REVERSE)
    {
      for (i = 0 ; i < feature->feature.transcript.exons->len ; i++)
	{
	  int start, end ;

	  span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i) ;

	  if (cds_only && (span->x1 > cds_end || span->x2 < cds_start))
	    {
	      continue ;
	    }
	  else
	    {
	      start = span->x1 ;
	      end = span->x2 ;

	      /* Truncate to CDS start/end in transcript. */
	      if (cds_only)
		{
		  if (cds_start >= span->x1 && cds_start <= span->x2)
		    start = cds_start ;
		  if (cds_end >= span->x1 && cds_end <= span->x2)
		    end = cds_end ;
		}

	      exon_base += end - start + 1 ;
	    }
	}
    }


  for (i = 0 ; i < feature->feature.transcript.exons->len ; i++)
    {
      span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i) ;

      /* We are only interested in the cds section of the transcript. */
      if (cds_only && (span->x1 > cds_end || span->x2 < cds_start))
	{
	  continue ;
	}
      else
	{
	  char qframe_strand ;
	  int qframe ;
	  int start, end ;

	  start = span->x1 ;
	  end = span->x2 ;

	  /* Truncate to CDS start/end in transcript. */
	  if (cds_only)
	    {
	      if (cds_start >= span->x1 && cds_start <= span->x2)
		start = cds_start ;
	      if (cds_end >= span->x1 && cds_end <= span->x2)
		end = cds_end ;
	    }
	  
	  /* We only export exons that fit completely within the blixem scope. */
	  if (start >= min && end <= max)
	    {
	      char *sframe ;

	      /* Note that sframe is meaningless so is set to an invalid value. */
	      if (feature->strand == ZMAPSTRAND_REVERSE)
		{
		  qstart = end - min + 1;
		  qend   = start - min + 1;

		  qframe_strand = '-' ;
		  qframe = ( (((max - min + 1) - qstart) - (exon_base - (end - start + 1))) % 3) + 1 ;

		  sframe = "(-0)" ;
		  sstart = ((exon_base - (end - start + 1)) + 3) / 3 ;
		  send = (exon_base - 1) / 3 ;
		}
	      else
		{
		  qstart = start - min + 1;
		  qend   = end - min + 1;
		  
		  qframe_strand = '+' ;
		  qframe = ((qstart - 1 - exon_base) % 3) + 1 ;
		  
		  sframe = "(+0)" ;
		  sstart = (exon_base + 3) / 3 ;
		  send   = (exon_base + (end - start)) / 3 ;
		}
	      
	      g_string_printf(line, "%d\t(%c%d)\t%d\t%d\t%s\t%d\t%d\t%sx\n", 
			      score,
			      qframe_strand, qframe, qstart, qend,
			      sframe, sstart, send,
			      g_quark_to_string(feature->original_id)) ;
	      
	      status = printLine(blixem_data, line->str) ;

	      blixem_data->line = g_string_truncate(blixem_data->line, 0) ; /* Reset string buffer. */
	    }

	  if (feature->strand == ZMAPSTRAND_REVERSE)
	    {
	      exon_base -= (end - start + 1);
	    }
	  else
	    {
	      exon_base += (end - start + 1) ;
	    }
	}
    }

  
  return status ;
}


gboolean makeTmpfile(char *tmp_dir, char *file_prefix, char **tmp_file_name_out)
{
  gboolean status = FALSE ;
  char *tmpfile ;
  int file = 0 ;

  /* g_mkstemp replaces the XXXXXX with unique string of chars */
  tmpfile = g_strdup_printf("%sZMAP_%s_XXXXXX", tmp_dir, file_prefix) ;

  if ((file = g_mkstemp(tmpfile)))
    {
      *tmp_file_name_out = tmpfile ;
      close(file) ;
      status = TRUE ;
    }
  else
    {
      zMapShowMsg(ZMAP_MSG_WARNING, 
		  "Error: could not open temporary file %s for Blixem", tmpfile) ;
      g_free(tmpfile) ;
      status = FALSE ;
    }

  

  return status ;
}


static void checkForLocalSequence(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  blixemData  blixem_data = (blixemData)user_data ;

  /* If errorMsg is set then it means there was an error earlier on in processing so we don't
   * process any more records. */
  if (!blixem_data->errorMsg)
    {  
      /* Only process features we want to dump. We then filter those features according to the
       * following rules (inherited from acedb): alignment features must be wholly within the
       * blixem max/min to be included, for transcripts we include as many introns/exons as will fit. */
      if (feature->type == blixem_data->required_feature_type)
	{
	  if (feature->type == ZMAPSTYLE_MODE_ALIGNMENT
	      && feature->feature.homol.flags.has_sequence)
	    {
	      GQuark align_id = feature->original_id ;
	      
	      if (!(g_hash_table_lookup(blixem_data->known_sequences, GINT_TO_POINTER(align_id))))
		{

		  if (feature->feature.homol.type == blixem_data->align_type)
		    {
		      if ((feature->x1 >= blixem_data->min && feature->x1 <= blixem_data->max)
			  && (feature->x2 >= blixem_data->min && feature->x2 <= blixem_data->max))
			{
			  if ((*blixem_data->opts == 'X' && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
			      || (*blixem_data->opts == 'N' && feature->feature.homol.type == ZMAPHOMOL_N_HOMOL))
			    {
			      ZMapSequence new_sequence ;

			      g_hash_table_insert(blixem_data->known_sequences,
						  GINT_TO_POINTER(feature->original_id),
						  feature) ;
			      
			      new_sequence = g_new0(ZMapSequenceStruct, 1) ;
			      new_sequence->name = align_id ;   /* Is this the right id ??? no...???? */
			      
			      blixem_data->local_sequences = g_list_append(blixem_data->local_sequences,
									   new_sequence) ;
			    }
			}
		    }
		}
	    }
	}
    }


  return ;
}


/* A GCompareFunc() to find the sequence in a list of sequences that has the same
 * name as the given feature. */
static int findFeature(gconstpointer a, gconstpointer b)
{
  int result = -1 ;
  ZMapSequence sequence = (ZMapSequence)a ;
  ZMapFeature feature = (ZMapFeature)b ;

  if (feature->original_id == sequence->name)
    result = 0 ;

  return result ;
}

/* GFunc to free sequence structs for local sequences. */
static void freeSequences(gpointer data, gpointer user_data_unused)
{
  ZMapSequence sequence = (ZMapSequence)data ;

  zMapAssert(sequence && sequence->sequence) ;

  g_free(sequence->sequence) ;
  g_free(sequence) ;

  return ;
}

static gboolean check_edited_values(ZMapGuiNotebookAny note_any, const char *entry_text, gpointer user_data)
{
  char *text = (char *)entry_text;
  gboolean allowed = TRUE;

  if(!text || (text && !*text))
    allowed = FALSE;
  else if(note_any->name == g_quark_from_string("Launch script"))
    {
      if(!(allowed = zMapFileAccess(text, "x")))
	zMapWarning("%s is not executable.", text);
      allowed = TRUE;		/* just warn for now */
    }
  else if(note_any->name == g_quark_from_string("Config File"))
    {
      if(!(allowed = zMapFileAccess(text, "r")))
	zMapWarning("%s is not readable.", text);
      allowed = TRUE;		/* just warn for now */
    }
  else if(note_any->name == g_quark_from_string("Port"))
    {
      int tmp = 0;
      if(zMapStr2Int(text, &tmp))
	{
	  int min = 1024, max = 65535;
	  if(tmp < min || tmp > max)
	    {
	      allowed = FALSE;
	      zMapWarning("Port should be in range of %d to %d", min, max);
	    }
	}
    }
  else
    allowed = TRUE;

  return allowed;
}

static ZMapGuiNotebookChapter makeChapter(ZMapGuiNotebook note_book_parent)
{
  ZMapGuiNotebookChapter chapter = NULL ;
  ZMapGuiNotebookCBStruct user_CBs = {cancelCB, NULL, applyCB, NULL, check_edited_values, NULL, saveCB, NULL} ;
  ZMapGuiNotebookPage page ;
  ZMapGuiNotebookSubsection subsection ;
  ZMapGuiNotebookParagraph paragraph ;
  ZMapGuiNotebookTagValue tagvalue ;

  chapter = zMapGUINotebookCreateChapter(note_book_parent, "Blixem", &user_CBs) ;


  page = zMapGUINotebookCreatePage(chapter, BLIX_NB_PAGE_GENERAL) ;

  subsection = zMapGUINotebookCreateSubsection(page, NULL) ;

  paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
					     ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,
					     NULL, NULL) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Scope",
					   ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					   "int", blixem_config_curr_G.scope) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Maximum Homols Shown",
					   ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					   "int", blixem_config_curr_G.homol_max) ;


  page = zMapGUINotebookCreatePage(chapter, BLIX_NB_PAGE_PFETCH) ;

  subsection = zMapGUINotebookCreateSubsection(page, NULL) ;

  paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
					     ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,
					     NULL, NULL) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Host network id",
					   ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					   "string", blixem_config_curr_G.netid) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Port",
					   ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					   "int", blixem_config_curr_G.port) ;


  page = zMapGUINotebookCreatePage(chapter, BLIX_NB_PAGE_ADVANCED) ;

  subsection = zMapGUINotebookCreateSubsection(page, NULL) ;

  paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
					     ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,
					     NULL, NULL) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Config File",
					   ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					   "string", blixem_config_curr_G.config_file) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Launch script",
					   ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
					   "string", blixem_config_curr_G.script) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Keep temporary Files",
					   ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
					   "bool", blixem_config_curr_G.keep_tmpfiles) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Kill Blixem on Exit",
					   ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
					   "bool", blixem_config_curr_G.kill_on_exit) ;

  return chapter ;
}




static void readChapter(ZMapGuiNotebookChapter chapter)
{
  ZMapGuiNotebookPage page ;
  gboolean bool_value = FALSE ;
  int int_value = 0 ;
  char *string_value = NULL ;


  if ((page = zMapGUINotebookFindPage(chapter, BLIX_NB_PAGE_GENERAL)))
    {
      if (zMapGUINotebookGetTagValue(page, "Scope", "int", &int_value))
	{
	  if (int_value > 0)
	    blixem_config_curr_G.scope = int_value ;
	}

      if (zMapGUINotebookGetTagValue(page, "Maximum Homols Shown", "int", &int_value))
	{
	  if (int_value > 0)
	    blixem_config_curr_G.homol_max = int_value ;
	}
    }

  if ((page = zMapGUINotebookFindPage(chapter, BLIX_NB_PAGE_PFETCH)))
    {

      /* ADD VALIDATION.... */

      if (zMapGUINotebookGetTagValue(page, "Host network id", "string", &string_value))
	{
	  if (string_value && *string_value)
	    {
	      g_free(blixem_config_curr_G.netid) ;

	      blixem_config_curr_G.netid = g_strdup(string_value) ;
	    }
	}

      if (zMapGUINotebookGetTagValue(page, "Port", "int", &int_value))
	{
	  blixem_config_curr_G.port = int_value ;
	}

    }

  if ((page = zMapGUINotebookFindPage(chapter, BLIX_NB_PAGE_ADVANCED)))
    {

      /* ADD VALIDATION.... */

      if (zMapGUINotebookGetTagValue(page, "Config File", "string", &string_value))
	{
	  if (string_value && *string_value)
	    {
	      g_free(blixem_config_curr_G.config_file) ;

	      blixem_config_curr_G.config_file = g_strdup(string_value) ;
	    }
	}

      if (zMapGUINotebookGetTagValue(page, "Launch script", "string", &string_value))
	{
	  if (string_value && *string_value)
	    {
	      g_free(blixem_config_curr_G.script) ;

	      blixem_config_curr_G.script = g_strdup(string_value) ;
	    }
	}

      if (zMapGUINotebookGetTagValue(page, "Keep temporary Files", "bool", &bool_value))
	{
	  blixem_config_curr_G.keep_tmpfiles = bool_value ;
	}

      if (zMapGUINotebookGetTagValue(page, "Kill Blixem on Exit", "bool", &bool_value))
	{
	  blixem_config_curr_G.kill_on_exit = bool_value ;
	}

    }


  return ;
}

static void saveUserPrefs(BlixemConfigData prefs)
{
  ZMapConfigIniContext context = NULL;

  if((context = zMapConfigIniContextProvide()))
    {
      if(prefs->netid)
	zMapConfigIniContextSetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				      ZMAPSTANZA_BLIXEM_NETID, prefs->netid);

      zMapConfigIniContextSetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				 ZMAPSTANZA_BLIXEM_PORT, prefs->port);
      
      zMapConfigIniContextSetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				 ZMAPSTANZA_BLIXEM_SCOPE, prefs->scope);

      zMapConfigIniContextSetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				 ZMAPSTANZA_BLIXEM_MAX, prefs->homol_max);

      zMapConfigIniContextSetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				     ZMAPSTANZA_BLIXEM_KEEP_TEMP, prefs->keep_tmpfiles);

      zMapConfigIniContextSetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				     ZMAPSTANZA_BLIXEM_KILL_EXIT, prefs->kill_on_exit);

      if(prefs->script)
	zMapConfigIniContextSetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				      ZMAPSTANZA_BLIXEM_SCRIPT, prefs->script);

      if(prefs->config_file)
	zMapConfigIniContextSetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				      ZMAPSTANZA_BLIXEM_CONF_FILE, prefs->config_file);


#ifdef RDS_DONT_INCLUDE
      if(prefs->dna_sets)
	zMapConfigIniContextSetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				      ZMAPSTANZA_BLIXEM_DNA_FS, prefs->dna_sets);

      if(prefs->protein_sets)
	zMapConfigIniContextSetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				      ZMAPSTANZA_BLIXEM_PROT_FS, prefs->protein_sets);

      if(prefs->transcript_sets)
	zMapConfigIniContextSetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				      ZMAPSTANZA_BLIXEM_TRANS_FS, prefs->transcript_sets);
#endif

      zMapConfigIniContextSave(context);
    }

  return ;
}
static void saveChapter(ZMapGuiNotebookChapter chapter)
{
  saveUserPrefs(&blixem_config_curr_G);

  return ;
}

static void applyCB(ZMapGuiNotebookAny any_section, void *user_data_unused)
{
  readChapter((ZMapGuiNotebookChapter)any_section) ;

  return ;
}

static void saveCB(ZMapGuiNotebookAny any_section, void *user_data_unused)
{
  ZMapGuiNotebookChapter chapter = (ZMapGuiNotebookChapter)any_section;

  readChapter(chapter);

  saveChapter(chapter);

  return ;
}

static void cancelCB(ZMapGuiNotebookAny any_section, void *user_data_unused)
{

  return ;
}





/*************************** end of file *********************************/
