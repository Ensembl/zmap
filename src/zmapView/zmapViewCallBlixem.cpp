/*  File: zmapViewCallBlixem.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk),
 *          Steve Miller (sm23@sanger.ac.uk)
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
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Prepares input files and passes them to blixem
 *              for display. Files are FASTA for reference sequence,
 *              either GFFv3 or seqbl/exblx format.
 *              Note: the latter two formats are now deprecated and
 *              have been removed.
 *
 * Exported functions: see zmapView_P.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <unistd.h>                                            /* for getlogin() */
#include <string.h>                                            /* for memcpy */
#include <sys/types.h>
#include <sys/stat.h>                                            /* for chmod() */
#include <glib.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapSO.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapUtilsGUI.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapThreads.hpp>
#include <ZMap/zmapGFF.hpp>

/* private header for this module */
#include <zmapView_P.hpp>



/* some blixem defaults.... */
enum
  {
    BLIX_DEFAULT_SCOPE = 40000   /* default 'width' of blixem sequence/features. */
  } ;

#define BLIX_DEFAULT_SCRIPT "blixemh"                       /* default executable name */

#define ZMAP_BLIXEM_CONFIG "blixem"

#define BLIX_NB_PAGE_GENERAL  "General"
#define BLIX_NB_PAGE_PFETCH   "Pfetch Socket Server"
#define BLIX_NB_PAGE_ADVANCED "Advanced"

#define BLX_ARGV_ARGC_MAX 100

/* Control block for preparing data to call blixem. */
typedef struct ZMapBlixemDataStruct_
{
  /* user preferences for blixem */
  gboolean kill_on_exit ;                 /* TRUE => remove this blixem on
                                             program exit (default). */

  gchar         *script;                  /* script to call blixem standalone */

  char          *config_file ;            /* Contains blixem config. information. */

  gchar         *netid;                   /* eg pubseq */
  int            port;                    /* eg 22100  */

  /* Blixem can either initially zoom to position or show the whole picture. */
  gboolean show_whole_range ;

  /* Blixem can rebase the coords to show them with a different origin, offset is used to do this. */
  int offset ;

  int position ;                           /* Tells blixem what position to
                                              centre on initially. */
  int window_start, window_end ;
  int mark_start, mark_end ;


  /* The ref. sequence and features are shown in blixem over the range  (position +/- (scope / 2)) */
  /* These restrict the range over which ref sequence and features are displayed if a mark was
   * set. NOTE that if scope is set to mark then so are features. */
  int scope ;                              /* defaults to 40000 */

  gboolean scope_from_mark ;               /* Use mark start/end for scope ? */
  gboolean features_from_mark ;

  int scope_min, scope_max ;               /* Bounds of displayed sequence. */
  int features_min, features_max ;         /* Bounds of displayed features. */

  gboolean negate_coords ;                 /* Show rev strand coords as same as
                                              forward strand but with a leading '-'. */
  gboolean isSeq;

  int            homol_max;                /* score cutoff point */

  const char          *opts;
  char          *errorMsg;

  gboolean      keep_tmpfiles ;
  gboolean      sleep_on_startup ;

  char          *fastAFile ;
  GIOChannel    *fasta_channel;
  char          *gff_file ;
  GIOChannel    *gff_channel ;

  ZMapGFFAttributeFlags attribute_flags ;
  gboolean over_write ;

  ZMapView     view;

  GList *features ;

  ZMapFeatureSet feature_set ;
  GList *source;

  ZMapFeatureBlock block ;

  GHashTable *known_sequences ;             /* Used to check if we already have a sequence. */
  GList *local_sequences ;                  /* List of any sequences held in
                                               server and not in pfetch. */

  /* Used for processing individual features. */
  ZMapStyleMode required_feature_type ;     /* specifies what type of feature
                                             * needs to be processed. */

  /* User can specify sets of homologies that can be passed to blixem, if the set they selected
   * is in one of these sets then all the features in all the sets are sent to blixem. */
  ZMapHomolType align_type ;                /* What type of alignment are we doing ? */

  ZMapWindowAlignSetType align_set ;        /* Which set of alignments ? */

  GList *dna_sets ;
  GList *protein_sets ;
  GList *transcript_sets ;

  GString *line ;

  GList *align_list ;

  ZMapFeatureSequenceMap sequence_map;      /* where the sequence comes from, used for BAM scripts */

} ZMapBlixemDataStruct, *ZMapBlixemData ;


/*
 * Holds just the config data, some of which is user configurable.
 */
typedef struct BlixemConfigDataStructType
{
  gboolean init ;                           /* TRUE when struct has been initialised. */

  /* Used to detect when user has set data fields. */
  struct
  {
    unsigned int kill_on_exit : 1 ;
    unsigned int script : 1 ;
    unsigned int config_file : 1 ;
    unsigned int netid : 1 ;
    unsigned int port : 1 ;
    unsigned int scope : 1 ;
    unsigned int scope_from_mark : 1 ;
    unsigned int features_from_mark : 1 ;
    unsigned int homol_max : 1 ;
    unsigned int keep_tmpfiles : 1 ;
    unsigned int sleep_on_startup : 1 ;
  } is_set ;


  /* User configurable */
  gboolean kill_on_exit ;                     /* TRUE => remove this blixem on
                                                 program exit (default). */
  gchar         *script ;                     /* script to call blixem standalone */
  gchar         *config_file ;                /* file containing blixem config. */

  gchar         *netid ;                      /* eg pubseq */
  int           port ;                        /* eg 22100  */

  int           scope ;                       /* defines range over which aligns are
                                                 collected to send to blixem, defaults to 40000 */

  gboolean scope_from_mark ;
  gboolean features_from_mark ;

  int           homol_max ;                   /* defines max. number of aligns sent
                                                 to blixem. */
  gboolean      keep_tmpfiles ;
  gboolean      sleep_on_startup ;

  /* Not user configurable */
  GList *dna_sets ;
  GList *protein_sets ;
  GList *transcript_sets ;

} BlixemConfigDataStruct, *BlixemConfigData ;


static gboolean initBlixemData(ZMapView view, ZMapFeatureBlock block,
                               ZMapHomolType align_type,
                               int offset, int position,
                               int window_start, int window_end,
                               int mark_start, int mark_end,
                               GList *features, ZMapFeatureSet feature_set,
                               ZMapWindowAlignSetType align_set,
                               ZMapBlixemData blixem_data, char **err_msg) ;
static gboolean setBlixemScope(ZMapBlixemData blixem_data) ;
static gboolean buildParamString (ZMapBlixemData blixem_data, char **paramString);

static void deleteBlixemData(ZMapBlixemData *blixem_data);
static ZMapBlixemData createBlixemData() ;

static char ** createBlixArgArray();
static void deleteBlixArgArray(char ***) ;

static void setPrefs(BlixemConfigData curr_prefs, ZMapBlixemData blixem_data) ;
static gboolean getUserPrefs(char *config_file, BlixemConfigData prefs) ;

static void checkForLocalSequence(gpointer key, gpointer data, gpointer user_data) ;
static gboolean makeTmpfiles(ZMapBlixemData blixem_data) ;
gboolean makeTmpfile(const char *tmp_dir, const char *file_prefix, char **tmp_file_name_out) ;
static gboolean setTmpPerms(const char *path, gboolean directory) ;

static void processSetHash(gpointer key, gpointer data, gpointer user_data) ;
static void processSetList(gpointer data, gpointer user_data) ;

static void writeHashEntry(gpointer key, gpointer data, gpointer user_data) ;
static void writeListEntry(gpointer data, gpointer user_data) ;

static gboolean writeFastAFile(ZMapBlixemData blixem_data);
static gboolean writeFeatureFiles(ZMapBlixemData blixem_data);
static gboolean initFeatureFile(const char *filename, GString *buffer,
                                 GIOChannel **gio_channel_out, char ** err_out ) ;

static void writeFeatureLine(ZMapFeature feature, ZMapBlixemData  blixem_data) ;


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

GList * zMapViewGetColumnFeatureSets(ZMapBlixemData data,GQuark column_id);


/*
 *                Globals
 */

/* Configuration global, holds current config for blixem, gets overloaded with new file
 * settings and new user selections. */
static BlixemConfigDataStruct blixem_config_curr_G = {FALSE} ;



/*
 *                External routines
 */

/* Checks to see if any of the requested alignments have their sequence stored locally in the
 * database instead of in pfetch DB.
 * Note - position is the centre of the section of reference sequence displayed.
 *      - feature is only used to get the type of alignment to be displayed.
 *  */
gboolean zmapViewBlixemLocalSequences(ZMapView view,
                                      ZMapFeatureBlock block, ZMapHomolType align_type,
                                      int offset, int position,
                                      ZMapFeatureSet feature_set, GList **local_sequences_out)
{
  char *err_msg = NULL;
  gboolean status = FALSE ;
  ZMapBlixemData blixem_data = NULL ;

  blixem_data = createBlixemData() ;
  if (!blixem_data)
    return status ;
  status = TRUE ;

  status = initBlixemData(view, block, align_type,
                          0, position,
                          0, 0, 0, 0, NULL, feature_set, ZMAPWINDOW_ALIGNCMD_NONE, blixem_data, &err_msg) ;

  blixem_data->errorMsg = NULL ;
  blixem_data->sequence_map = view->view_sequence;

  /* Do requested Homology data first. */
  blixem_data->required_feature_type = ZMAPSTYLE_MODE_ALIGNMENT ;

  /* Make a list of all the sequences held locally in the database. */
  blixem_data->known_sequences = g_hash_table_new(NULL, NULL) ;

  if (feature_set)
    g_hash_table_foreach(feature_set->features, checkForLocalSequence, blixem_data) ;

  g_hash_table_destroy(blixem_data->known_sequences) ;
  blixem_data->known_sequences = NULL ;

  /* Return result if all ok and we found some local sequences.... */
  if (status)
    {
      if (blixem_data->local_sequences)
        {
          *local_sequences_out = blixem_data->local_sequences ;
          blixem_data->local_sequences = NULL ; /* So its not free'd by deleteBlixemData. */
        }
      else
        {
          status = FALSE ;
        }
    }
  else
    {
      zMapShowMsg(ZMAP_MSG_WARNING, err_msg) ;
    }

  if (blixem_data)
    deleteBlixemData(&blixem_data) ;
  if (err_msg)
    g_free(err_msg) ;

  return status ;
}


/* Calls blixem to display alignments to the context block sequence from the feature set given by
 * the parameter, some of the sequences of these alignments are given by the local_sequences
 * parameter, the others blixem will fetch using the pfetch program.
 *
 * The function returns TRUE if blixem was successfully launched and also returns the pid of the blixem
 * process so that the blixems can be cleared up when the view exits.
 *
 * Note - position is where blixems cursor will start, window_start/end tell blixem
 *        what to display initially, cursor is within this range.
 *      - feature is only used to get the type of alignment to be displayed.
 *  */
gboolean zmapViewCallBlixem(ZMapView view,
                            ZMapFeatureBlock block, ZMapHomolType homol_type,
                            int offset, int position,
                            int window_start, int window_end,
                            int mark_start, int mark_end,
                            ZMapWindowAlignSetType align_set,
                            gboolean isSeq,
                            GList *features, ZMapFeatureSet feature_set,
                            GList *source, GList *local_sequences,
                            GPid *child_pid, gboolean *kill_on_exit)
{
  char *err_msg = NULL ;
  gboolean status = FALSE;
  char **argv    = NULL ;
  ZMapBlixemData blixem_data = NULL ;

  blixem_data = createBlixemData() ;
  if (!blixem_data)
    return status ;
  status = TRUE ;

  argv = createBlixArgArray() ;
  if (!argv)
    status = FALSE ;

  if ((status = initBlixemData(view, block, homol_type,
                               offset, position,
                               window_start, window_end,
                               mark_start, mark_end,
                               features, feature_set, align_set, blixem_data, &err_msg)))
    {
      blixem_data->local_sequences = local_sequences ;
      blixem_data->source = source;
      blixem_data->sequence_map = view->view_sequence;
      blixem_data->isSeq = isSeq;
    }

  if (status)
    {
      status = makeTmpfiles(blixem_data) ;
    }

  if (status)
    {
      status = buildParamString(blixem_data, argv) ;
    }

  if (status)
    {
      status = writeFeatureFiles(blixem_data);
    }

  if (status)
    {
      status = writeFastAFile(blixem_data);
    }


  /* Finally launch blixem passing it the temporary files. */
  if (status)
    {
      char *cwd = NULL, **envp = NULL; /* inherit from parent */
      GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
      GSpawnChildSetupFunc pre_exec = NULL;
      gpointer pre_exec_data = NULL;
      GPid spawned_pid = 0;
      GError *error = NULL;

      /*
       * I'm inserting a lock here until I can check if g_spawn_async() shares code with
       * g_spawn_async_with_pipes().
       *
       * (sm23 08/01/15) It calls  g_spawn_async_with_pipes() without pipes, so yes would
       * appear to be the answer to that.
       */
      zMapThreadForkLock() ;

      if (!(g_spawn_async(cwd, argv, envp, flags, pre_exec, pre_exec_data, &spawned_pid, &error)))
        {
          status = FALSE;
          if (error)
            {
              g_error_free(error) ;
            }
        }
      else
        {
          zMapLogMessage("Blixem process spawned successfully. PID = '%d'", spawned_pid);
        }

      zMapThreadForkUnlock();

      if (status && child_pid)
        *child_pid = spawned_pid ;

      if (kill_on_exit)
        *kill_on_exit = blixem_data->kill_on_exit ;
    }

  if (!status)
    {
      if (err_msg)
        zMapWarning("%s", err_msg) ;
      else
        zMapWarning("%s", "Error starting blixem") ;
    }
  else
    {
      /* N.B. we block for a couple of seconds here to make sure user can see message. */
      zMapGUIShowMsgFull(NULL, "blixem launched and will display shortly.",
                         ZMAP_MSG_EXIT,
                         GTK_JUSTIFY_CENTER, 3, FALSE) ;
    }

  if (blixem_data)
    deleteBlixemData(&blixem_data) ;
  if (argv)
    deleteBlixArgArray(&argv) ;
  if (err_msg)
    g_free(err_msg) ;

  return status ;
}



/* Returns a ZMapGuiNotebookChapter containing all user settable blixem resources. */
ZMapGuiNotebookChapter zMapViewBlixemGetConfigChapter(ZMapView view, ZMapGuiNotebook note_book_parent)
{
  ZMapGuiNotebookChapter chapter = NULL ;

  /* If the current configuration has not been set yet then read stuff from the config file. */
  if (!blixem_config_curr_G.init)
    getUserPrefs(view->view_sequence->config_file, &blixem_config_curr_G) ;


  chapter = makeChapter(note_book_parent) ; /* mh17: this uses blixen_config_curr_G */

  return chapter ;
}

gboolean zMapViewBlixemGetConfigFunctions(ZMapView view, gpointer *edit_func,
                                          gpointer *apply_func, gpointer save_func, gpointer *data)
{

  return TRUE;
}




/*
 *                     Internal routines.
 */



static gboolean initBlixemData(ZMapView view, ZMapFeatureBlock block,
                               ZMapHomolType align_type,
                               int offset, int position,
                               int window_start, int window_end,
                               int mark_start, int mark_end,
                               GList *features, ZMapFeatureSet feature_set,
                               ZMapWindowAlignSetType align_set,
                               ZMapBlixemData blixem_data, char **err_msg)
{
  gboolean status = FALSE  ;
  if (!blixem_data || !view)
    return status ;
  status = TRUE ;

  blixem_data->view  = view ;

  blixem_data->config_file = g_strdup(((ZMapFeatureSequenceMap)(view->sequence_mapping->data))->config_file) ;
  blixem_data->line = g_string_new(NULL) ;
  blixem_data->offset = offset ;
  blixem_data->position = position ;
  blixem_data->window_start = window_start ;
  blixem_data->window_end = window_end ;
  blixem_data->mark_start = mark_start ;
  blixem_data->mark_end = mark_end ;
  blixem_data->scope = BLIX_DEFAULT_SCOPE ;
  blixem_data->negate_coords = TRUE ;                            /* default for havana. */
  blixem_data->align_type = align_type ;
  blixem_data->align_set = align_set;
  blixem_data->features = features ;
  blixem_data->feature_set = feature_set ;
  blixem_data->block = block ;
  blixem_data->over_write = TRUE ;
  zMapGFFFormatAttributeUnsetAll(blixem_data->attribute_flags) ;

  if (!(zMapFeatureBlockDNA(block, NULL, NULL, NULL)))
    {
      status = FALSE;
      *err_msg = g_strdup("No DNA attached to feature's parent so cannot call blixem.") ;
    }

  if (status)
    {
      if ((status = getUserPrefs(blixem_data->config_file, &blixem_config_curr_G)))
        setPrefs(&blixem_config_curr_G, blixem_data) ;
    }

  if (status)
    status = setBlixemScope(blixem_data) ;

  return status ;
}

/*
 * Create an object of ZMapBlixemData type.
 */
static ZMapBlixemData createBlixemData()
{
  ZMapBlixemData result = NULL ;

  result = (ZMapBlixemData) g_new0(ZMapBlixemDataStruct, 1) ;
  result->attribute_flags = (ZMapGFFAttributeFlags) g_new0(ZMapGFFAttributeFlagsStruct, 1) ;

  return result ;
}

/*
 * Delete an object of ZMapBlixemData type.
 */
static void deleteBlixemData(ZMapBlixemData *p_blixem_data)
{
  if (!p_blixem_data || !*p_blixem_data)
    return ;
  ZMapBlixemData blixem_data = *p_blixem_data ;

  if (blixem_data->fastAFile)
    g_free(blixem_data->fastAFile);
  if (blixem_data->gff_file)
    g_free(blixem_data->gff_file);
  if (blixem_data->line)
    g_string_free(blixem_data->line, TRUE ) ;
  if (blixem_data->attribute_flags)
    g_free(blixem_data->attribute_flags) ;
  if (blixem_data->netid)
    g_free(blixem_data->netid);
  if (blixem_data->script)
    g_free(blixem_data->script);
  if (blixem_data->config_file)
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

  memset(blixem_data, 0, sizeof(ZMapBlixemDataStruct)) ;
  g_free(blixem_data) ;
  *p_blixem_data = NULL ;

  return ;
}


/* Get any user preferences specified in config file. */
static gboolean getUserPrefs(char *config_file, BlixemConfigData curr_prefs)
{
  gboolean status = FALSE ;
  ZMapConfigIniContext context = NULL ;
  BlixemConfigDataStruct file_prefs = {FALSE} ;
  if (!curr_prefs)
    return status ;

  if ((context = zMapConfigIniContextProvide(config_file)))
    {
      char *tmp_string = NULL ;
      int tmp_int ;
      gboolean tmp_bool ;

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                       ZMAPSTANZA_BLIXEM_NETID, &tmp_string))
        file_prefs.netid = tmp_string ;

      if (zMapConfigIniContextGetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                    ZMAPSTANZA_BLIXEM_PORT, &tmp_int))
        file_prefs.port = tmp_int ;

      if (zMapConfigIniContextGetFilePath(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                          ZMAPSTANZA_BLIXEM_SCRIPT, &tmp_string))
        file_prefs.script = tmp_string ;

      if (zMapConfigIniContextGetFilePath(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                          ZMAPSTANZA_BLIXEM_CONF_FILE, &tmp_string))
        file_prefs.config_file = tmp_string ;


      if (zMapConfigIniContextGetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                    ZMAPSTANZA_BLIXEM_SCOPE, &tmp_int))
        file_prefs.scope = tmp_int ;


      /* NOTE that if scope_from_mark is TRUE then features_from_mark must be. */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                        ZMAPSTANZA_BLIXEM_SCOPE_MARK, &tmp_bool))
        file_prefs.scope_from_mark = tmp_bool;

      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                        ZMAPSTANZA_BLIXEM_FEATURES_MARK, &tmp_bool))
        file_prefs.features_from_mark = tmp_bool;

      if (file_prefs.scope_from_mark)
        file_prefs.features_from_mark = TRUE ;


      if (zMapConfigIniContextGetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                    ZMAPSTANZA_BLIXEM_MAX, &tmp_int))
        file_prefs.homol_max = tmp_int ;

      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                        ZMAPSTANZA_BLIXEM_KEEP_TEMP, &tmp_bool))
        file_prefs.keep_tmpfiles = tmp_bool;

      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                        ZMAPSTANZA_BLIXEM_SLEEP, &tmp_bool))
        file_prefs.sleep_on_startup = tmp_bool;

      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                        ZMAPSTANZA_BLIXEM_KILL_EXIT, &tmp_bool))
        file_prefs.kill_on_exit = tmp_bool;

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                       ZMAPSTANZA_BLIXEM_DNA_FS, &tmp_string))
        {
          file_prefs.dna_sets = zMapConfigString2QuarkList(tmp_string,FALSE);
          g_free(tmp_string);
        }

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                       ZMAPSTANZA_BLIXEM_PROT_FS, &tmp_string))
        {
          file_prefs.protein_sets = zMapConfigString2QuarkList(tmp_string,FALSE);
          g_free(tmp_string);
        }

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                       ZMAPSTANZA_BLIXEM_FS, &tmp_string))
        {
          file_prefs.transcript_sets = zMapConfigString2QuarkList(tmp_string,FALSE);
          g_free(tmp_string);
        }
      zMapConfigIniContextDestroy(context);
    }

  /* We need a blixem script. If it wasn't given, set a default */
  if (!file_prefs.script)
    {
      file_prefs.script = g_strdup(BLIX_DEFAULT_SCRIPT) ;
    }

  /* Check that script exists and is executable */
  if (file_prefs.script)
    {
      char *tmp = NULL ;
      tmp = file_prefs.script;

      if ((file_prefs.script = g_find_program_in_path(tmp)))
        {
          status = TRUE ;
        }
      else
        {
          zMapShowMsg(ZMAP_MSG_WARNING,
                      "Either can't locate \"%s\" in your path or it is not executable by you.",
                      tmp) ;
        }

      g_free(tmp) ;
    }
  else
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "The blixem script is not specified.") ;
    }

  file_prefs.init = TRUE;

  /* If curr_prefs is initialised then copy any curr prefs set by user into file prefs,
   * thus overriding file prefs with user prefs. Then copy file prefs into curr_prefs
   * so that curr prefs now represents latest config file and user prefs combined. */
  if (curr_prefs->init)
    {
      file_prefs.is_set = curr_prefs->is_set ;                    /* struct copy. */

      if (curr_prefs->is_set.scope)
        file_prefs.scope = curr_prefs->scope ;

      if (curr_prefs->is_set.scope_from_mark)
        file_prefs.scope_from_mark = curr_prefs->scope_from_mark ;

      if (curr_prefs->is_set.features_from_mark)
        file_prefs.features_from_mark = curr_prefs->features_from_mark ;

      if (curr_prefs->is_set.homol_max)
        file_prefs.homol_max = curr_prefs->homol_max ;

      if (curr_prefs->is_set.netid)
        {
          g_free(file_prefs.netid) ;
          file_prefs.netid = curr_prefs->netid ;
        }

      if (curr_prefs->is_set.port)
        file_prefs.port = curr_prefs->port ;

      if (curr_prefs->is_set.config_file)
        {
          g_free(file_prefs.config_file) ;
          file_prefs.config_file = curr_prefs->config_file ;
        }

      if (curr_prefs->is_set.script)
        {
          g_free(file_prefs.script) ;
          file_prefs.script = curr_prefs->script ;
        }

      if (curr_prefs->is_set.keep_tmpfiles)
        file_prefs.keep_tmpfiles = curr_prefs->keep_tmpfiles ;

      if (curr_prefs->is_set.sleep_on_startup)
        file_prefs.sleep_on_startup = curr_prefs->sleep_on_startup ;

      if (curr_prefs->is_set.kill_on_exit)
        file_prefs.kill_on_exit = curr_prefs->kill_on_exit ;
    }

  /* If a config file is specified, check that it can be found */
  if (file_prefs.config_file && !zMapFileAccess(file_prefs.config_file, "r"))
    {
      zMapShowMsg(ZMAP_MSG_WARNING,
                  "The Blixem config file \"%s\" cannot be found in your path or it is not read/writeable.",
                  file_prefs.config_file) ;
    }

  *curr_prefs = file_prefs ;                                    /* Struct copy. */

  return status ;
}



/* Set blixem_data from current user preferences. */
static void setPrefs(BlixemConfigData curr_prefs, ZMapBlixemData blixem_data)
{
  if (!curr_prefs || !blixem_data)
    return ;

  if (blixem_data->netid)
    g_free(blixem_data->netid);
  blixem_data->netid = g_strdup(curr_prefs->netid) ;

  blixem_data->port = curr_prefs->port ;

  g_free(blixem_data->script);
  blixem_data->script = g_strdup(curr_prefs->script) ;

  g_free(blixem_data->config_file);
  blixem_data->config_file = g_strdup(curr_prefs->config_file) ;

  if (curr_prefs->scope > 0)
    blixem_data->scope = curr_prefs->scope ;


  blixem_data->scope_from_mark = curr_prefs->scope_from_mark ;
  blixem_data->features_from_mark = curr_prefs->features_from_mark ;

  if (curr_prefs->homol_max > 0)
    blixem_data->homol_max = curr_prefs->homol_max ;

  blixem_data->keep_tmpfiles = curr_prefs->keep_tmpfiles ;

  blixem_data->sleep_on_startup = curr_prefs->sleep_on_startup ;

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


/* Set blixem scope for sequence/features and initial position of blixem window on sequence. */
static gboolean setBlixemScope(ZMapBlixemData blixem_data)
{
  gboolean status = FALSE ;
  static gboolean scope_debug = FALSE ;
  if (!blixem_data)
    return status ;
  status = TRUE ;

  if (status)
    {
      gboolean is_mark ;
      int scope_range ;

      scope_range = blixem_data->scope / 2 ;

      if (blixem_data->mark_start < blixem_data->block->block_to_sequence.block.x1)
        blixem_data->mark_start = blixem_data->block->block_to_sequence.block.x1 ;
      if (blixem_data->mark_end > blixem_data->block->block_to_sequence.block.x2)
        blixem_data->mark_end = blixem_data->block->block_to_sequence.block.x2 ;


      /* Use the mark coords to set scope ? */
      is_mark = (blixem_data->mark_start && blixem_data->mark_end) ;

      /* Set min/max range for blixem scope. */
      if (is_mark && blixem_data->scope_from_mark)
        {
          /* Just use the mark */
          blixem_data->scope_min = blixem_data->mark_start ;
          blixem_data->scope_max = blixem_data->mark_end ;
        }
      else if (blixem_data->position < blixem_data->window_start
               || blixem_data->position > blixem_data->window_end)
        {
          /* We can't set the range to be centred on the given position because the position is not in
           * the visible window and this would be confusing for users. Instead, set the position
           * to the centre of the visible region. */
          blixem_data->position = ((blixem_data->window_end - blixem_data->window_start) / 2) + blixem_data->window_start ;
          blixem_data->scope_min = blixem_data->position - scope_range ;
          blixem_data->scope_max = blixem_data->position + scope_range ;
        }
      else
        {
          /* Set the scope to be centred on the position */
          blixem_data->scope_min = blixem_data->position - scope_range ;
          blixem_data->scope_max = blixem_data->position + scope_range ;
        }

      /* Clamp to block, needed if user runs blixem near to either end of sequence. */
      if (blixem_data->scope_min < blixem_data->block->block_to_sequence.block.x1)
        blixem_data->scope_min = blixem_data->block->block_to_sequence.block.x1 ;

      if (blixem_data->scope_max > blixem_data->block->block_to_sequence.block.x2)
        blixem_data->scope_max = blixem_data->block->block_to_sequence.block.x2 ;

      /* Set min/max range for blixem features. */
      blixem_data->features_min = blixem_data->scope_min ;
      blixem_data->features_max = blixem_data->scope_max ;

      if (is_mark && (blixem_data->features_from_mark || blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_SEQ))
        {
          blixem_data->features_min = blixem_data->mark_start ;
          blixem_data->features_max = blixem_data->mark_end ;
        }


      /* Clamp to scope. */
      if (blixem_data->features_min < blixem_data->scope_min)
        blixem_data->features_min = blixem_data->scope_min ;

      if (blixem_data->features_max > blixem_data->scope_max)
        blixem_data->features_max = blixem_data->scope_max ;

      /* Now clamp window start/end to scope start/end. */
      if (blixem_data->window_start < blixem_data->scope_min)
        blixem_data->window_start = blixem_data->scope_min ;

      if (blixem_data->window_end > blixem_data->scope_max)
        blixem_data->window_end = blixem_data->scope_max ;
    }

  if (status)
    {
      if (blixem_data->align_type == ZMAPHOMOL_X_HOMOL)            /* protein */
        {
          ZMapFeature feature ;

          /* HACKED FOR NOW..... */
          if (blixem_data->features)
            feature = (ZMapFeature)(blixem_data->features->data) ;
          else
            feature = (ZMapFeature)zMap_g_hash_table_nth(blixem_data->feature_set->features, 0) ;

          if (feature->strand == ZMAPSTRAND_REVERSE)
            blixem_data->opts = "X-BR";
          else
            blixem_data->opts = "X+BR";
        }
      else
        {
          blixem_data->opts = "N+BR";                            /* dna */
        }
    }


  zMapDebugPrint(scope_debug,
                 "\n"
                 "        block start/end\t  %d <---------------------> %d\n"
                 "         mark start/end\t  %d <---------------------> %d\n"
                 "          scope min/max\t  %d <--------------------->, %d\n"
                 "        feature min/max\t  %d <---------------------> %d\n"
                 "window min/position/max\t  %d <--- %d ---> %d\n"
                 "\n",
                 blixem_data->block->block_to_sequence.block.x1, blixem_data->block->block_to_sequence.block.x2,
                 blixem_data->mark_start, blixem_data->mark_end,
                 blixem_data->scope_min, blixem_data->scope_max,
                 blixem_data->features_min, blixem_data->features_max,
                 blixem_data->window_start, blixem_data->position, blixem_data->window_end) ;


  return status ;
}





/* Make the temporary files which are the sole input to the blixem program to get it to
 * display alignments.
 *
 * The directory and files are created so the user has write access but all others
 * can read.
 *  */
static gboolean makeTmpfiles(ZMapBlixemData blixem_data)
{
  gboolean    status = FALSE ;
  char *path = NULL,
    *login = NULL,
    *dir = NULL ;
  if (!blixem_data)
    return status ;
  status = TRUE ;

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
      if (!(dir = zMapGetDir(path, FALSE, TRUE)))
        {
          zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not create temp directory for Blixem.") ;
          status = FALSE;
        }
      else
        {
          status = setTmpPerms(dir, TRUE) ;
        }
    }


  /* Create the file to the hold the DNA in FastA format. */
  if (status)
    {
      if ((status = makeTmpfile(dir, "fasta", &(blixem_data->fastAFile))))
        status = setTmpPerms(blixem_data->fastAFile, FALSE) ;
    }

  /* Create file(s) to hold features. */
  if (status)
    {
      if ((status = makeTmpfile(dir, "gff", &(blixem_data->gff_file))))
        status = setTmpPerms(blixem_data->gff_file, FALSE) ;
    }


 /*
  * These need to be freed once finished.
  */
 if (path)
    g_free(path) ;
  if (dir)
    g_free(dir) ;

  return status ;
}

/* A very noddy routine to set good permissions on our tmp directory/files... */
static gboolean setTmpPerms(const char *path, gboolean directory)
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

/*
 * These functions are to allocate and delete the argument array used for calling
 * blixem. The reason that it's done this way is because of the requirements for
 * calling to g_spawn_async() on linux.
 */
static char ** createBlixArgArray()
{
  char ** arg_array = (char**) g_new0(char*, BLX_ARGV_ARGC_MAX) ;
  return arg_array ;
}

static void deleteBlixArgArray(char ***p_arg_array)
{
  int i = 0;
  char ** arg_array = NULL ;
  if (p_arg_array)
    {
      arg_array = *p_arg_array ;
      if (arg_array)
        {
          for (i=0; i<BLX_ARGV_ARGC_MAX; ++i)
            {
              if (arg_array[i])
                g_free(arg_array[i]) ;
            }
          g_free(arg_array) ;
          *p_arg_array = NULL ;
        }
    }
}

static gboolean checkBlixArgNum(int icount)
{
  if (icount >= BLX_ARGV_ARGC_MAX)
    return FALSE ;
  else
    return TRUE ;
}

/*
 * Construct the array of arguments and check against the maximum number allowed.
 * Note that when this array is passed into to the g_spawn_*() functions, you have to
 * have a NULL terminator, so you can't pass in an array which has all the elements
 * allocated to strings.
 */
static gboolean buildParamString(ZMapBlixemData blixem_data, char ** paramString)
{
  gboolean status = FALSE ;
  int icount = 0 ;

  if (blixem_data && paramString)
    status = TRUE ;

  if (status)
    {
      paramString[icount] = g_strdup(blixem_data->script) ;
      ++icount ;
      status = checkBlixArgNum(icount) ;
    }

  if (status && blixem_data->config_file )
    {
      paramString[icount] = g_strdup("-c");
      ++icount ;
      status = checkBlixArgNum(icount) ;

      if (status)
        {
          paramString[icount] = g_strdup_printf("%s", blixem_data->config_file) ;
          ++icount ;
          status = checkBlixArgNum(icount) ;
        }

    }
  else if (status && blixem_data->netid && blixem_data->port )
    {
      paramString[icount] = g_strdup("--fetch-server");
      ++icount ;
      status = checkBlixArgNum(icount) ;

      if (status)
        {
          paramString[icount] = g_strdup_printf("%s:%d", blixem_data->netid, blixem_data->port);
          ++icount ;
          status = checkBlixArgNum(icount) ;
        }
    }

  if (status && blixem_data->view->multi_screen)
    {
      paramString[icount] = g_strdup("--screen") ;
      ++icount ;
      status = checkBlixArgNum(icount) ;

      if (status)
        {
          paramString[icount] = g_strdup_printf("%d", blixem_data->view->blixem_screen) ;
          ++icount ;
          status = checkBlixArgNum(icount) ;
        }
    }

  if ( status && !blixem_data->keep_tmpfiles  )
    {
      paramString[icount] = g_strdup("--remove-input-files") ;
      ++icount ;
      status = checkBlixArgNum(icount) ;
    }


  /* For testing purposes tell blixem to sleep on startup. */
  if (status && blixem_data->sleep_on_startup)
    {
      paramString[icount] = g_strdup("-w") ;
      ++icount ;
      status = checkBlixArgNum(icount) ;
    }

  /* Should abbreviated window titles be turned on. */
  if (status && zMapGUIGetAbbrevTitlePrefix())
    {
      paramString[icount] = g_strdup("--abbrev-title-on") ;
      ++icount ;
      status = checkBlixArgNum(icount) ;
    }

  /* */
  if (status && blixem_data->position )
    {
      int start, end ;

      start = end = blixem_data->position ;
      if (blixem_data->view->flags[ZMAPFLAG_REVCOMPED_FEATURES])
        zMapFeatureReverseComplementCoords(blixem_data->view->features, &start, &end) ;

      paramString[icount] = g_strdup("-s") ;
      ++icount ;
      status = checkBlixArgNum(icount) ;

      if (status)
        {
          paramString[icount] = g_strdup_printf("%d", start) ;
          ++icount ;
          status = checkBlixArgNum(icount) ;
        }

    }

  if (status && blixem_data->position )
    {
      int offset, tmp1 ;

      offset = tmp1 = blixem_data->offset ;

      paramString[icount] = g_strdup("-m") ;
      ++icount ;
      status = checkBlixArgNum(icount) ;

      if (status)
        {
          paramString[icount] = g_strdup_printf("%d", offset) ;
          ++icount ;
          status = checkBlixArgNum(icount) ;
        }
    }

  /* Set up initial view start/end.... */
  if (status )
    {
      int start = blixem_data->window_start,
          end  = blixem_data->window_end ;

      if (blixem_data->view->flags[ZMAPFLAG_REVCOMPED_FEATURES])
        zMapFeatureReverseComplementCoords(blixem_data->view->features, &start, &end) ;

      paramString[icount] = g_strdup("-z") ;
      ++icount ;
      status = checkBlixArgNum(icount) ;

      if (status)
        {
          paramString[icount] = g_strdup_printf("%d:%d", start, end) ;
          ++icount ;
          status = checkBlixArgNum(icount) ;
        }

    }

  /* Show whole blixem range ? */
  if (status && blixem_data->show_whole_range )
    {
      paramString[icount] = g_strdup("--zoom-whole") ;
      ++icount ;
      status = checkBlixArgNum(icount) ;
    }

  /* acedb users always like reverse strand to have same coords as forward strand but
   * with a '-' in front of the coord. */
  if (status && blixem_data->negate_coords)
    {
      paramString[icount] = g_strdup("-n") ;
      ++icount ;
      status = checkBlixArgNum(icount) ;
    }

  /* Start with reverse strand showing. */
  if (status && blixem_data->view->flags[ZMAPFLAG_REVCOMPED_FEATURES])
    {
      paramString[icount] = g_strdup("-r") ;
      ++icount ;
      status = checkBlixArgNum(icount) ;
    }

  /* type of alignment data, i.e. nucleotide or peptide. Compulsory. Note that type
   * can be NONE if blixem called for non-alignment column, something requested by
   * annotators for just looking at transcripts/dna. */
  if (status)
    {
      paramString[icount] = g_strdup("-t");
      ++icount ;
      status = checkBlixArgNum(icount) ;

      if (status)
        {
          if (blixem_data->align_type == ZMAPHOMOL_NONE || blixem_data->align_type == ZMAPHOMOL_N_HOMOL)
            {
              paramString[icount] = g_strdup_printf("%c", 'N') ;
            }
          else
            {
              paramString[icount] = g_strdup_printf("%c", 'P') ;
            }
          ++icount ;
          status = checkBlixArgNum(icount) ;
        }

    }

  if (status)
    {
      paramString[icount] = g_strdup_printf("%s", blixem_data->fastAFile);
      ++icount ;
      status = checkBlixArgNum(icount) ;

      if (status)
        {
          paramString[icount] = g_strdup_printf("%s", blixem_data->gff_file) ;
          ++icount ;
          status = checkBlixArgNum(icount) ;
        }
    }

  if (status && blixem_data->sequence_map->dataset )
    {
      paramString[icount] = g_strdup_printf("--dataset=%s", blixem_data->sequence_map->dataset);
      ++icount ;
      status = checkBlixArgNum(icount) ;
    }

  if (status && ((blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_SEQ) || blixem_data->isSeq) )
    {
      paramString[icount] = g_strdup("--show-coverage");
      ++icount ;
      status = checkBlixArgNum(icount) ;

      if (status)
        {
          paramString[icount] = g_strdup("--squash-matches");
          ++icount ;
          status = checkBlixArgNum(icount) ;
        }

      if (status)
        {
          paramString[icount] = g_strdup("--sort-mode=p");
          ++icount ;
          status = checkBlixArgNum(icount) ;
        }
    }

  return status ;
}



static gboolean writeFeatureFiles(ZMapBlixemData blixem_data)
{
  static const char * SO_region = "region" ;
  gboolean status = FALSE ;
  GString *attribute = NULL ;
  GError *channel_error = NULL ;
  int start=0,
    end=0 ;
  char * err_out = NULL ;

  if (!blixem_data)
    return status ;
  status = TRUE ;
  attribute = g_string_new(NULL) ;

  /*
   * Write the file headers.
   */
  start = blixem_data->features_min ;
  end = blixem_data->features_max ;

  if (blixem_data->view->flags[ZMAPFLAG_REVCOMPED_FEATURES])
    zMapFeatureReverseComplementCoords(blixem_data->view->features, &start, &end) ;

  zMapGFFFormatHeader(TRUE, blixem_data->line,
                      g_quark_to_string(blixem_data->block->original_id), start, end) ;
  status = initFeatureFile(blixem_data->gff_file, blixem_data->line,
                           &(blixem_data->gff_channel), &err_out) ;
  if (!status)
    {
      blixem_data->errorMsg =
        g_strdup_printf("Error in zMapViewCallBlixem::writeFeatureFiles(); could not open '%s'.",
                        blixem_data->gff_file) ;
      if (err_out)
        {
          g_free(err_out) ;
        }
    }

  /*
   * Write the features.
   */
  if (status)
    {
      ZMapFeatureSet feature_set = NULL ;
      GList *set_list = NULL ;

      blixem_data->errorMsg = NULL ;

      feature_set = blixem_data->feature_set ;

      /*
       * Do requested Homology data first.
       */
      blixem_data->required_feature_type = ZMAPSTYLE_MODE_ALIGNMENT ;

      /* Do a homol max list here.... */
      int num_homols = 0 ;

      blixem_data->align_list = NULL ;

      if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_FEATURES
          || blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_EXPANDED)
        {
          /* mh17: BAM compressed features can get the same names, we don't want to fetch others in */
          if ((g_list_length(blixem_data->features) > 1) || zMapStyleIsUnique(feature_set->style))
            {
              blixem_data->align_list = blixem_data->features ;
            }
          else
            {
              ZMapFeature feature = (ZMapFeature)(blixem_data->features->data) ;

              blixem_data->align_list = zMapFeatureSetGetNamedFeatures(feature_set, feature->original_id) ;
            }
        }
      else if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_SET)
        {
          g_hash_table_foreach(feature_set->features, getFeatureCB, blixem_data) ;
        }
      else if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_MULTISET)
        {
          if (blixem_data->align_type == ZMAPHOMOL_N_HOMOL)
            set_list = blixem_data->dna_sets ;
          else
            set_list = blixem_data->protein_sets ;

          g_list_foreach(set_list, getSetList, blixem_data) ;
        }


      /* NOTE the request data is a column which contains multiple featuresets
       * we have to include a line for each one. Theese are given in the course GList
       */
      if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_SEQ)
        {
          GList *l = NULL;
          for(l = blixem_data->source; l ; l = l->next)
            {
              const char *sequence_name = g_quark_to_string(blixem_data->block->original_id) ;

              if (zMapGFFFormatMandatory(blixem_data->over_write, blixem_data->line, sequence_name,
                                     g_quark_to_string(GPOINTER_TO_UINT(l->data)), SO_region,
                                     start, end,
                                     (float)0.0, ZMAPSTRAND_NONE, ZMAPPHASE_NONE,
                                     TRUE, TRUE) )
                {
                  g_string_append_printf(attribute, "dataType=short-read") ;
                  zMapGFFFormatAppendAttribute(blixem_data->line, attribute, FALSE, FALSE) ;
                  g_string_append_c(blixem_data->line, '\n') ;
                  g_string_truncate(attribute, (gsize)0);

                  zMapGFFOutputWriteLineToGIO(blixem_data->gff_channel, &(blixem_data->errorMsg),
                            blixem_data->line, TRUE) ;
                }
            }
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

      /* Write out remaining alignments. */
      if (blixem_data->align_list)
        g_list_foreach(blixem_data->align_list, writeListEntry, blixem_data) ;

      /* If user clicked on something not an alignment then show that feature
       * and others nearby in blixems overview window. */
      if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_NONE && feature_set)
        {
          g_hash_table_foreach(feature_set->features, writeHashEntry, blixem_data) ;
        }

      /*
       * Now do transcripts (may need to filter further...)
       */
      if (blixem_data->transcript_sets)
        {
          g_list_foreach(blixem_data->transcript_sets, processSetList, blixem_data) ;
        }
      else
        {
          g_hash_table_foreach(blixem_data->block->feature_sets, processSetHash, blixem_data) ;
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

  /* Shut down if open. */
  if (blixem_data->gff_channel)
    {
      g_io_channel_shutdown(blixem_data->gff_channel, TRUE, &channel_error);
      if (channel_error)
        g_free(channel_error);
    }

  /* Free our string buffer. */
  g_string_free(attribute, TRUE ) ;

  return status ;
}



/*
 * Open and initially format a feature file.
 */
static gboolean initFeatureFile(const char *filename, GString *buffer,
                                GIOChannel **gio_channel_out, char ** err_out)
{
  gboolean status = FALSE ;
  GError  *channel_error = NULL ;
  if (!filename || !buffer || !gio_channel_out || !err_out )
    return status ;
  status = TRUE ;

  /* Open the file, always needed. */
  if ((*gio_channel_out = g_io_channel_new_file(filename, "w", &channel_error)))
    {
      status = zMapGFFOutputWriteLineToGIO(*gio_channel_out, err_out, buffer, TRUE) ;
    }
  else
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not open file: %s", channel_error->message) ;
      g_error_free(channel_error) ;
      status = FALSE ;
    }

  return status ;
}


/* GFunc()'s to step through the named feature sets and write them out for passing
 * to blixem. One function to process hash table entries, the other list entries */
static void processSetHash(gpointer key, gpointer data, gpointer user_data)
{
  GQuark set_id = GPOINTER_TO_UINT(data) ;
  GQuark canon_id = 0;
  ZMapBlixemData blixem_data = (ZMapBlixemData)user_data ;
  ZMapFeatureSet feature_set = NULL;
  if (!blixem_data)
    return ;

  canon_id = zMapFeatureSetCreateID((char *)g_quark_to_string(set_id)) ;

  feature_set = (ZMapFeatureSet)g_hash_table_lookup(blixem_data->block->feature_sets, GINT_TO_POINTER(canon_id));

  if (feature_set)
    {
      g_hash_table_foreach(feature_set->features, writeHashEntry, blixem_data);
    }

  return ;
}

static void processSetList(gpointer data, gpointer user_data)
{
  GQuark set_id = GPOINTER_TO_UINT(data) ;
  GQuark canon_id = 0 ;
  ZMapBlixemData blixem_data = (ZMapBlixemData)user_data ;
  ZMapFeatureSet feature_set = NULL ;
  if (!blixem_data)
    return ;

  canon_id = zMapFeatureSetCreateID((char *)g_quark_to_string(set_id)) ;

  feature_set = (ZMapFeatureSet)g_hash_table_lookup(blixem_data->block->feature_sets, GINT_TO_POINTER(canon_id));

  if (feature_set)
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
  ZMapBlixemData  blixem_data = (ZMapBlixemData)user_data ;

  writeFeatureLine(feature, blixem_data) ;

  return ;
}

static void writeListEntry(gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  ZMapBlixemData  blixem_data = (ZMapBlixemData)user_data ;

  writeFeatureLine(feature, blixem_data) ;

  return ;
}


static void writeFeatureLine(ZMapFeature feature, ZMapBlixemData  blixem_data)
{
  if (!feature || !blixem_data )
    return ;

  /* There is no way to interrupt g_hash_table_foreach() so instead if errorMsg is set
   * then it means there was an error in processing so we don't process any
   * more records, displaying the error when we return. */
  if (!(blixem_data->errorMsg))
    {
      gboolean status = TRUE,
      include_feature = FALSE,
      fully_contained = FALSE ;   /* TRUE => feature must be fully
                                     contained, FALSE => just overlapping. */

      /* Only process features we want to dump. We then filter those features according to the
       * following rules (inherited from acedb): alignment features must be wholly within the
       * blixem max/min to be included, for transcripts we include as many introns/exons as will fit. */

      if (fully_contained)
        include_feature = (feature->x1 <= blixem_data->features_max && feature->x2 >= blixem_data->features_min) ;
      else
        include_feature =  !(feature->x1 > blixem_data->features_max || feature->x2 < blixem_data->features_min) ;

      if (include_feature)
        {
          if (blixem_data->view->flags[ZMAPFLAG_REVCOMPED_FEATURES])
            zMapFeatureReverseComplement(blixem_data->view->features, feature) ;

          zMapGFFFormatAttributeUnsetAll(blixem_data->attribute_flags) ;

          if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT)
            {
              if (feature->feature.homol.type == blixem_data->align_type)
                {
                  if (   (*blixem_data->opts == 'X' && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
                      || (*blixem_data->opts == 'N' && feature->feature.homol.type == ZMAPHOMOL_N_HOMOL))
                    {
                       /*
                        * this if could be tidied up, but let's leave existing logic alone
                        * of old 'local sequence' meant in ACEDB, for BAM we get it in GFF and save it in the feature
                        */
                      char *seq_str = NULL ;
                      GList *list_ptr = NULL ;
                      if((list_ptr = g_list_find_custom(blixem_data->local_sequences, feature, findFeature)))
                        {
                          ZMapSequence sequence = (ZMapSequence)list_ptr->data ;
                          seq_str = sequence->sequence ;
                        }
                      status = zMapGFFFormatAttributeSetAlignment(blixem_data->attribute_flags)
                            && zMapGFFWriteFeatureAlignment(feature, blixem_data->attribute_flags, blixem_data->line,
                                                            blixem_data->over_write, seq_str) ;
                    }
                }
            }
          else if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
            {
              ZMapFeatureSet feature_set = (ZMapFeatureSet)(feature->parent) ;
              GQuark feature_set_id = feature_set->unique_id ;

              // Complicated but user can click on a transcript column and then we can end up
              // exporting some transcripts twice, once for the column clicked on and once for the
              // transcript set.
              if (blixem_data->align_set != ZMAPWINDOW_ALIGNCMD_NONE
                  || ((!(blixem_data->transcript_sets)
                       || !(g_list_find(blixem_data->transcript_sets, GINT_TO_POINTER(feature_set_id))))))
                {
                  status = (zMapGFFFormatAttributeSetTranscript(blixem_data->attribute_flags)
                            && zMapGFFWriteFeatureTranscript(feature, blixem_data->attribute_flags,
                                                             blixem_data->line, blixem_data->over_write)) ;
                }
            }
          else if (feature->mode == ZMAPSTYLE_MODE_BASIC)
            {
              status = zMapGFFFormatAttributeSetBasic(blixem_data->attribute_flags)
                    && zMapGFFWriteFeatureBasic(feature, blixem_data->attribute_flags,
                                                blixem_data->line, blixem_data->over_write) ;
            }

          if (status)
            {
              status = zMapGFFOutputWriteLineToGIO(blixem_data->gff_channel,
                                                   &(blixem_data->errorMsg),
                                                   blixem_data->line, TRUE) ;
            }

           if (blixem_data->view->flags[ZMAPFLAG_REVCOMPED_FEATURES])
             zMapFeatureReverseComplement(blixem_data->view->features, feature) ;
        }
    }


  return ;
}




/* A GFunc() to step through the named feature sets and write them out for passing
 * to blixem. */
static void getSetList(gpointer data, gpointer user_data)
{
  GQuark set_id = GPOINTER_TO_UINT(data) ;
  GQuark canon_id = 0 ;
  ZMapBlixemData blixem_data = (ZMapBlixemData)user_data ;
  ZMapFeatureSet feature_set = NULL ;
  GList *column_2_featureset = NULL ;

  if (!blixem_data)
    return ;

  canon_id = zMapFeatureSetCreateID((char *)g_quark_to_string(set_id)) ;

  feature_set = (ZMapFeatureSet)g_hash_table_lookup(blixem_data->block->feature_sets, GINT_TO_POINTER(canon_id));

  if(feature_set)
    {
      g_hash_table_foreach(feature_set->features, getFeatureCB, blixem_data);
    }
  else
    {
      /* assuming a mis-config treat the set id as a column id */
      column_2_featureset = zMapFeatureGetColumnFeatureSets(&blixem_data->view->context_map,canon_id,TRUE);
      if(!column_2_featureset)
        {
            zMapLogWarning("Could not find %s feature set or column \"%s\" in context feature sets.",
                  (blixem_data->required_feature_type == ZMAPSTYLE_MODE_ALIGNMENT
                  ? "alignment" : "transcript"),
                  g_quark_to_string(set_id)) ;
        }

      for(;column_2_featureset;column_2_featureset = column_2_featureset->next)
        {
          if((feature_set = (ZMapFeatureSet)g_hash_table_lookup(blixem_data->block->feature_sets, column_2_featureset->data)))
            {
                  g_hash_table_foreach(feature_set->features, getFeatureCB, blixem_data) ;
            }
        }
    }

  return ;
}

static void getFeatureCB(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  ZMapBlixemData  blixem_data = (ZMapBlixemData)user_data ;

  if (!feature || !blixem_data)
    return ;

  /* Only process features we want to dump. We then filter those features according to the
   * following rules (inherited from acedb): alignment features must be wholly within the
   * blixem max/min to be included, for transcripts we include as many introns/exons as will fit. */
  if (feature->mode == blixem_data->required_feature_type)
    {
      switch (feature->mode)
        {
        case ZMAPSTYLE_MODE_ALIGNMENT:
          {
            if (feature->feature.homol.type == blixem_data->align_type)
              {
                if (   (*blixem_data->opts == 'X' && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
                    || (*blixem_data->opts == 'N' && feature->feature.homol.type == ZMAPHOMOL_N_HOMOL))
                  blixem_data->align_list = g_list_append(blixem_data->align_list, feature) ;
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
    gint result = 0;
    ZMapFeature feat_1 = (ZMapFeature)a,  feat_2 = (ZMapFeature)b ;

    if (!feat_1 || !feat_2 || !feat_1->flags.has_score || !feat_2->flags.has_score )
      return result ;

    if (feat_1->score < feat_2->score)
      result = 1 ;
    else if (feat_1->score > feat_2->score)
      result = -1 ;

    return result ;
}




/* Write out the dna sequence into a file in fasta format, ie a header record with the
 * sequence name, then as many lines 50 characters long as it takes to write out the
 * sequence. We only export the dna for min -> max.
 *
 * THE CODING LOGIC IS HORRIBLE HERE BUT I DON'T HAVE ANY TIME TO CORRECT IT NOW, EG.
 *
 *  */
static gboolean writeFastAFile(ZMapBlixemData blixem_data)
{
  gboolean status = TRUE ;
  gsize bytes_written ;
  GError *channel_error = NULL ;
  char *line = NULL ;
  enum { FASTA_CHARS = 50 } ;
  char buffer[FASTA_CHARS + 2] ;                            /* FASTA CHARS + \n + \0 */
  int lines = 0, chars_left = 0 ;
  char *cp = NULL ;
  int i = 0;


  if (!zMapFeatureBlockDNA(blixem_data->block, NULL, NULL, &cp))
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error creating FASTA file, failed to get feature's DNA");

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
          int start, end ;

          start = blixem_data->scope_min ;
          end = blixem_data->scope_max ;

          if (blixem_data->view->flags[ZMAPFLAG_REVCOMPED_FEATURES])
            zMapFeatureReverseComplementCoords(blixem_data->view->features, &start, &end) ;

          /* Write header as:   ">seq_name start end" so file is self describing, note that
           * start/end are ref sequence coords (e.g. chromosome), not local zmap display coords. */
          line = g_strdup_printf(">%s %d %d\n",
                                 g_quark_to_string(blixem_data->view->features->parent_name),
                                 start, end) ;

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
          char *dna ;
          int start, end ;

          start = blixem_data->scope_min ;
          end = blixem_data->scope_max ;

          dna = zMapFeatureGetDNA((ZMapFeatureAny)(blixem_data->block),
                                  start, end, blixem_data->view->flags[ZMAPFLAG_REVCOMPED_FEATURES]) ;

          total_chars = (end - start + 1) ;

          lines = total_chars / FASTA_CHARS ;
          chars_left = total_chars % FASTA_CHARS ;

          cp = dna ;

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


          g_free(dna) ;
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



gboolean makeTmpfile(const char *tmp_dir, const char *file_prefix, char **tmp_file_name_out)
{
  gboolean status = FALSE ;
  char *tmpfile = NULL ;
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
  ZMapBlixemData  blixem_data = (ZMapBlixemData)user_data ;

  if (!blixem_data || !feature)
    return ;

  /* If errorMsg is set then it means there was an error earlier on in processing so we don't
   * process any more records. */
  if (!blixem_data->errorMsg)
    {
      /* Only process features we want to dump. We then filter those features according to the
       * following rules (inherited from acedb): alignment features must be wholly within the
       * blixem max/min to be included, for transcripts we include as many introns/exons as will fit. */
      if (feature->mode == blixem_data->required_feature_type)
        {
          if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT
              && feature->feature.homol.flags.has_sequence)
            {
              GQuark align_id = feature->original_id ;

              if (!(g_hash_table_lookup(blixem_data->known_sequences, GINT_TO_POINTER(align_id))))
                {

                  if (feature->feature.homol.type == blixem_data->align_type)
                    {
                      if ((feature->x1 >= blixem_data->features_min && feature->x1 <= blixem_data->features_max)
                          && (feature->x2 >= blixem_data->features_min && feature->x2 <= blixem_data->features_max))
                        {
                          if (   (*blixem_data->opts == 'X' && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
                              || (*blixem_data->opts == 'N' && feature->feature.homol.type == ZMAPHOMOL_N_HOMOL))
                            {
                              ZMapSequence new_sequence ;

                              g_hash_table_insert(blixem_data->known_sequences,
                                                  GINT_TO_POINTER(feature->original_id),
                                                  feature) ;

                              new_sequence = g_new0(ZMapSequenceStruct, 1) ;
                              new_sequence->name = align_id ;   /* Is this the right id ???
                                                                   no...???? */

                              if (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
                                new_sequence->type = ZMAPSEQUENCE_PEPTIDE ;
                              else
                                new_sequence->type = ZMAPSEQUENCE_DNA ;

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

  if (!feature || !sequence)
    return result ;

  if (feature->original_id == sequence->name)
    result = 0 ;

  return result ;
}

/* GFunc to free sequence structs for local sequences. */
static void freeSequences(gpointer data, gpointer user_data_unused)
{
  ZMapSequence sequence = (ZMapSequence)data ;

  if (sequence)
    {
      if (sequence->sequence)
        g_free(sequence->sequence) ;
      g_free(sequence) ;
    }

  return ;
}

static gboolean check_edited_values(ZMapGuiNotebookAny note_any, const char *entry_text, gpointer user_data)
{
  char *text = (char *)entry_text;
  gboolean allowed = TRUE;

  if (!text || (text && !*text))
    allowed = FALSE;
  else
    allowed = TRUE;

  return allowed;
}

static ZMapGuiNotebookChapter makeChapter(ZMapGuiNotebook note_book_parent)
{
  ZMapGuiNotebookChapter chapter = NULL ;
  static ZMapGuiNotebookCBStruct user_CBs =
    {
      cancelCB, NULL, applyCB, NULL, check_edited_values, NULL, saveCB, NULL
    } ;
  ZMapGuiNotebookPage page = NULL ;
  ZMapGuiNotebookSubsection subsection = NULL ;
  ZMapGuiNotebookParagraph paragraph = NULL ;
  ZMapGuiNotebookTagValue tagvalue = NULL ;

  chapter = zMapGUINotebookCreateChapter(note_book_parent, "Blixem", &user_CBs) ;


  page = zMapGUINotebookCreatePage(chapter, BLIX_NB_PAGE_GENERAL) ;

  subsection = zMapGUINotebookCreateSubsection(page, NULL) ;

  paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
                                             ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,
                                             NULL, NULL) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Scope", "The maximum length of sequence to send to Blixem",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                           "int", blixem_config_curr_G.scope) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Restrict scope to Mark", "Restricts the scope to the current marked region",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
                                           "bool", blixem_config_curr_G.scope_from_mark) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Restrict features to Mark", "Still uses the default scope but only sends features that are within the marked region to Blixem",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
                                           "bool", blixem_config_curr_G.features_from_mark) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Maximum Homols Shown", NULL,
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                           "int", blixem_config_curr_G.homol_max) ;


  page = zMapGUINotebookCreatePage(chapter, BLIX_NB_PAGE_PFETCH) ;

  subsection = zMapGUINotebookCreateSubsection(page, NULL) ;

  paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
                                             ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,
                                             NULL, NULL) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Host network id", NULL,
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                           "string", blixem_config_curr_G.netid) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Port", NULL,
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                           "int", blixem_config_curr_G.port) ;


  page = zMapGUINotebookCreatePage(chapter, BLIX_NB_PAGE_ADVANCED) ;

  subsection = zMapGUINotebookCreateSubsection(page, NULL) ;

  paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
                                             ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,
                                             NULL, NULL) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Config File", "The path to the configuration file to use for Blixem",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                           "string", blixem_config_curr_G.config_file) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Launch script", "The path to the Blixem executable",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                           "string", blixem_config_curr_G.script) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Keep temporary Files", "Tell Blixem not to destroy the temporary input files that ZMap sends to it.",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
                                           "bool", blixem_config_curr_G.keep_tmpfiles) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Sleep on Startup", "Tell Blixem to sleep on startup (for debugging)",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
                                           "bool", blixem_config_curr_G.sleep_on_startup) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Kill Blixem on Exit", "Close all Blixems that were started from this ZMap when ZMap exits",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
                                           "bool", blixem_config_curr_G.kill_on_exit) ;

  return chapter ;
}




static void readChapter(ZMapGuiNotebookChapter chapter)
{
  /* This result flag is set to false if the user entered invalid values but is currently not
   * used other than to issue a warning. Ideally we should not save the settings but leave 
   * the dialog open if a value is invalid. */
  gboolean result = TRUE ;

  ZMapGuiNotebookPage page = NULL ;
  gboolean bool_value = FALSE ;
  int int_value = 0 ;
  char *string_value = NULL ;


  if ((page = zMapGUINotebookFindPage(chapter, BLIX_NB_PAGE_GENERAL)))
    {
      if (zMapGUINotebookGetTagValue(page, "Scope", "int", &int_value))
        {
          if (int_value != blixem_config_curr_G.scope)
            {
              blixem_config_curr_G.scope = int_value ;
              blixem_config_curr_G.is_set.scope = TRUE ;
            }
        }


      /* Restricting scope and features are interlocked: if scope is restricted to
       * mark then features _must_ also be restricted to mark.
       * Restricting features to mark is independent of scope. */
      if (zMapGUINotebookGetTagValue(page, "Restrict scope to Mark", "bool", &bool_value))
        {
          if (blixem_config_curr_G.scope_from_mark != bool_value)
            {
              blixem_config_curr_G.scope_from_mark = bool_value ;
              blixem_config_curr_G.is_set.scope_from_mark = TRUE ;
            }
        }

      if (zMapGUINotebookGetTagValue(page, "Restrict features to Mark", "bool", &bool_value))
        {
          if (blixem_config_curr_G.features_from_mark != bool_value)
            {
              blixem_config_curr_G.features_from_mark = bool_value ;
              blixem_config_curr_G.is_set.features_from_mark = TRUE ;
            }

          /* Force features from mark if scope is set to mark. */
          if (blixem_config_curr_G.scope_from_mark)
            {
              blixem_config_curr_G.features_from_mark = TRUE ;
              blixem_config_curr_G.is_set.features_from_mark = TRUE ;
            }
        }


      if (zMapGUINotebookGetTagValue(page, "Maximum Homols Shown", "int", &int_value))
        {
          if (int_value != blixem_config_curr_G.homol_max)
            {
              blixem_config_curr_G.homol_max = int_value ;
              blixem_config_curr_G.is_set.homol_max = TRUE ;
            }
        }
    }

  if ((page = zMapGUINotebookFindPage(chapter, BLIX_NB_PAGE_PFETCH)))
    {
      if (zMapGUINotebookGetTagValue(page, "Host network id", "string", &string_value))
        {
          if (string_value && *string_value &&
              (!blixem_config_curr_G.netid ||
               strcmp(string_value, blixem_config_curr_G.netid) != 0))
            {
              /* Add validation ... */
              g_free(blixem_config_curr_G.netid) ;

              blixem_config_curr_G.netid = g_strdup(string_value) ;
              blixem_config_curr_G.is_set.netid = TRUE ;
            }
        }

      if (zMapGUINotebookGetTagValue(page, "Port", "int", &int_value))
        {
          if (int_value != blixem_config_curr_G.port)
            {
              /* Check it's in the allowed range */
              static int min_val = 1024, max_val = 65535;

              if (int_value < min_val || int_value > max_val)
                {
                  result = FALSE;
                  zMapWarning("Port should be in range of %d to %d", min_val, max_val);
                }
              else
                {
                  blixem_config_curr_G.port = int_value ;
                  blixem_config_curr_G.is_set.port = TRUE ;
                }
            }
        }

    }

  if ((page = zMapGUINotebookFindPage(chapter, BLIX_NB_PAGE_ADVANCED)))
    {
      if (zMapGUINotebookGetTagValue(page, "Config File", "string", &string_value))
        {
          if (string_value && *string_value &&
              (!blixem_config_curr_G.config_file ||
               strcmp(string_value, blixem_config_curr_G.config_file) != 0))
            {
              result = zMapFileAccess(string_value, "r") ;

              if (!result)
                zMapWarning("Blixem Config File '%s' is not readable.", string_value);

              g_free(blixem_config_curr_G.config_file) ;

              blixem_config_curr_G.config_file = g_strdup(string_value) ;
              blixem_config_curr_G.is_set.config_file = TRUE ;
            }
        }

      if (zMapGUINotebookGetTagValue(page, "Launch script", "string", &string_value))
        {
          if (string_value && *string_value &&
              (!blixem_config_curr_G.script ||
               strcmp(string_value, blixem_config_curr_G.script) != 0))
            {
              result = zMapFileAccess(string_value, "x") ;
              
              if (!result)
                zMapWarning("Blixem Launch Script '%s' is not executable.", string_value);

              g_free(blixem_config_curr_G.script) ;

              blixem_config_curr_G.script = g_strdup(string_value) ;
              blixem_config_curr_G.is_set.script = TRUE ;
            }
        }

      if (zMapGUINotebookGetTagValue(page, "Keep temporary Files", "bool", &bool_value))
        {
          if (blixem_config_curr_G.keep_tmpfiles != bool_value)
            {
              blixem_config_curr_G.keep_tmpfiles = bool_value ;
              blixem_config_curr_G.is_set.keep_tmpfiles = TRUE ;
            }
        }

      if (zMapGUINotebookGetTagValue(page, "Sleep on Startup", "bool", &bool_value))
        {
          if (blixem_config_curr_G.sleep_on_startup != bool_value)
            {
              blixem_config_curr_G.sleep_on_startup = bool_value ;
              blixem_config_curr_G.is_set.sleep_on_startup = TRUE ;
            }
        }

      if (zMapGUINotebookGetTagValue(page, "Kill Blixem on Exit", "bool", &bool_value))
        {
          if (blixem_config_curr_G.kill_on_exit != bool_value)
            {
              blixem_config_curr_G.kill_on_exit = bool_value ;
              blixem_config_curr_G.is_set.kill_on_exit = TRUE ;
            }
        }

    }


  return ;
}

static void saveUserPrefs(BlixemConfigData prefs)
{
  ZMapConfigIniContext context = NULL;
  if (!prefs)
    return ;

  if ((context = zMapConfigIniContextProvide(prefs->config_file)))
    {
      /*! \todo This updates the config in the 'user' file, but I think this is
       * incorrect because the user file is for zmap config, and this exports the blixem config! */
      ZMapConfigIniFileType file_type = ZMAPCONFIG_FILE_USER ;

      if (prefs->netid)
        zMapConfigIniContextSetString(context, file_type,
                                      ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                      ZMAPSTANZA_BLIXEM_NETID, prefs->netid);

      zMapConfigIniContextSetInt(context, file_type,
                                 ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                 ZMAPSTANZA_BLIXEM_PORT, prefs->port);

      zMapConfigIniContextSetInt(context, file_type,
                                 ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                 ZMAPSTANZA_BLIXEM_SCOPE, prefs->scope);

      zMapConfigIniContextSetBoolean(context, file_type,
                                     ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                     ZMAPSTANZA_BLIXEM_SCOPE_MARK, prefs->scope_from_mark) ;

      zMapConfigIniContextSetBoolean(context, file_type,
                                     ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                     ZMAPSTANZA_BLIXEM_FEATURES_MARK, prefs->features_from_mark) ;

      zMapConfigIniContextSetInt(context, file_type,
                                 ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                 ZMAPSTANZA_BLIXEM_MAX, prefs->homol_max);

      zMapConfigIniContextSetBoolean(context, file_type,
                                     ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                     ZMAPSTANZA_BLIXEM_KEEP_TEMP, prefs->keep_tmpfiles);

      zMapConfigIniContextSetBoolean(context, file_type,
                                     ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                     ZMAPSTANZA_BLIXEM_SLEEP, prefs->sleep_on_startup);

      zMapConfigIniContextSetBoolean(context, file_type,
                                     ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                     ZMAPSTANZA_BLIXEM_KILL_EXIT, prefs->kill_on_exit);

      if (prefs->script)
        zMapConfigIniContextSetString(context, file_type,
                                      ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                      ZMAPSTANZA_BLIXEM_SCRIPT, prefs->script);

      if (prefs->config_file)
        zMapConfigIniContextSetString(context, file_type,
                                      ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                      ZMAPSTANZA_BLIXEM_CONF_FILE, prefs->config_file);

      zMapConfigIniContextSave(context, file_type);

      zMapConfigIniContextDestroy(context) ;
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
