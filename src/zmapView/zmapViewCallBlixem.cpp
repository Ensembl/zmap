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
#include <ZMap/zmapConfigDir.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapThreads.hpp>
#include <ZMap/zmapGFF.hpp>
#include <ZMap/zmapUrlUtils.hpp>
#include <ZMap/zmapGFFStringUtils.hpp>

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

typedef struct RadioButtonDataStruct_
{
  GQuark *result ;
  GQuark group_id ;
} RadioButtonDataStruct, *RadioButtonData ;

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

  /* Indicate whether to process alignments and other feature types */
  gboolean do_alignments ;
  gboolean do_transcripts ;
  gboolean do_basic ;

  /* User can specify sets of homologies that can be passed to blixem, if the set they selected
   * is in one of these sets then all the features in all the sets are sent to blixem. */
  ZMapHomolType align_type ;                /* What type of alignment are we doing ? */

  ZMapWindowAlignSetType align_set ;        /* Which set of alignments ? */

  GList *assoc_featuresets ;                /* list of associated featuresets */
  GList *column_groups ;                    /* the unique_id (GQuark) of any column groups that the
                                             * selected featureset is in */

  GString *line ;

  GList *align_list ;

  ZMapFeatureSequenceMap sequence_map;      /* where the sequence comes from, used for BAM scripts */

  gboolean coverage_done ;

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
    unsigned int assoc_featuresets : 1 ;
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

  GList *assoc_featuresets ;

} BlixemConfigDataStruct, *BlixemConfigData ;


static gboolean initBlixemData(ZMapView view, ZMapFeatureBlock block,
                               ZMapHomolType align_type,
                               int offset, int position,
                               int window_start, int window_end,
                               int mark_start, int mark_end, const bool features_from_mark,
                               GList *features, ZMapFeatureSet feature_set,
                               ZMapWindowAlignSetType align_set,
                               ZMapBlixemData blixem_data, char **err_msg) ;
static gboolean setBlixemScope(ZMapBlixemData blixem_data, const bool features_from_mark) ;
static gboolean buildParamString (ZMapBlixemData blixem_data, char **paramString);

static void deleteBlixemData(ZMapBlixemData *blixem_data);
static ZMapBlixemData createBlixemData() ;

static char ** createBlixArgArray();
static void deleteBlixArgArray(char ***) ;

static void setPrefs(BlixemConfigData curr_prefs, ZMapBlixemData blixem_data) ;
static gboolean getUserPrefs(ZMapView view, char *config_file, BlixemConfigData prefs) ;

static void checkForLocalSequence(gpointer key, gpointer data, gpointer user_data) ;
static gboolean makeTmpfiles(ZMapBlixemData blixem_data) ;
gboolean makeTmpfile(const char *tmp_dir, const char *file_prefix, char **tmp_file_name_out) ;
static gboolean setTmpPerms(const char *path, gboolean directory) ;

static void writeFeatureSetHash(gpointer key, gpointer data, gpointer user_data) ;
static void writeFeatureSetList(gpointer data, gpointer user_data) ;
static void writeFeatureSet(GQuark set_id, ZMapBlixemData blixem_data) ;

static void writeFeatureLineHash(gpointer key, gpointer data, gpointer user_data) ;
static void writeFeatureLineList(gpointer data, gpointer user_data) ;
static void writeFeatureLine(ZMapFeature feature, ZMapBlixemData  blixem_data) ;

static gboolean writeFastAFile(ZMapBlixemData blixem_data);
static gboolean writeFeatureFiles(ZMapBlixemData blixem_data);
static gboolean initFeatureFile(const char *filename, GString *buffer,
                                 GIOChannel **gio_channel_out, char ** err_out ) ;


static int findFeature(gconstpointer a, gconstpointer b) ;
static void freeSequences(gpointer data, gpointer user_data_unused) ;

static ZMapGuiNotebookChapter makeChapter(ZMapGuiNotebook note_book_parent, ZMapView view) ;
static void readChapter(ZMapGuiNotebookChapter chapter) ;
static void saveCB(ZMapGuiNotebookAny any_section, void *user_data_unused);
static void cancelCB(ZMapGuiNotebookAny any_section, void *user_data) ;
static void applyCB(ZMapGuiNotebookAny any_section, void *user_data) ;

static void featureSetGetAlignList(gpointer data, gpointer user_data) ;
static void featureSetWriteBAMList(gpointer data, gpointer user_data) ;
static void getAlignFeatureCB(gpointer key, gpointer data, gpointer user_data) ;
static gint scoreOrderCB(gconstpointer a, gconstpointer b) ;

GList * zMapViewGetColumnFeatureSets(ZMapBlixemData data,GQuark column_id);

static void saveUserPrefs(BlixemConfigData prefs) ;

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
                          0, 0, 0, 0, FALSE, 
                          NULL, feature_set, ZMAPWINDOW_ALIGNCMD_NONE, blixem_data, &err_msg) ;

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
                            int mark_start, int mark_end, const bool features_from_mark,
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
                               mark_start, mark_end, features_from_mark,
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
    getUserPrefs(view, view->view_sequence->config_file, &blixem_config_curr_G) ;


  chapter = makeChapter(note_book_parent, view) ; /* mh17: this uses blixen_config_curr_G */

  return chapter ;
}

gboolean zMapViewBlixemGetConfigFunctions(ZMapView view, gpointer *edit_func,
                                          gpointer *apply_func, gpointer save_func, gpointer *data)
{

  return TRUE;
}


void zMapViewBlixemSaveChapter(ZMapGuiNotebookChapter chapter, ZMapView view)
{
  /* By default, save to the input zmap config file, if there was one */
  saveUserPrefs(&blixem_config_curr_G);

  return ;
}


/*
 *                     Internal routines.
 */



static gboolean initBlixemData(ZMapView view, ZMapFeatureBlock block,
                               ZMapHomolType align_type,
                               int offset, int position,
                               int window_start, int window_end,
                               int mark_start, int mark_end, const bool features_from_mark,
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
      if ((status = getUserPrefs(view, blixem_data->config_file, &blixem_config_curr_G)))
        setPrefs(&blixem_config_curr_G, blixem_data) ;
    }

  if (status)
    status = setBlixemScope(blixem_data, features_from_mark) ;

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

  if (blixem_data->assoc_featuresets)
    g_list_free(blixem_data->assoc_featuresets) ;

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


/* Get the user prefs for the given file/file-type (file may be null if the file type determines
 * the file location, e.g. the global user prefs file). */
static void getUserPrefsFile(ZMapView view, 
                             char *config_file, 
                             BlixemConfigData curr_prefs,
                             BlixemConfigData file_prefs,
                             ZMapConfigIniFileType file_type)
{
  ZMapConfigIniContext context = NULL ;

  if ((context = zMapConfigIniContextProvide(config_file, file_type)))
    {
      char *tmp_string = NULL ;
      int tmp_int ;
      gboolean tmp_bool ;

      /* Read the config values from the config file. Note that where we ignore empty strings and 0
       * numbers this means that these values are unset */

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                       ZMAPSTANZA_BLIXEM_NETID, &tmp_string))
        {
          if (tmp_string)
            {
              file_prefs->netid = tmp_string ;
              file_prefs->is_set.netid = TRUE ;
            }
          else
            {
              g_free(tmp_string) ;
            }
        }

      if (zMapConfigIniContextGetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                    ZMAPSTANZA_BLIXEM_PORT, &tmp_int)
          && tmp_int != 0)
        {
          file_prefs->port = tmp_int ;
          file_prefs->is_set.port = TRUE ;
        }

      if (zMapConfigIniContextGetFilePath(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                          ZMAPSTANZA_BLIXEM_SCRIPT, &tmp_string))
        {
          if (tmp_string)
            {
              file_prefs->script = tmp_string ;
              file_prefs->is_set.script = TRUE ;
            }
          else
            {
              g_free(tmp_string) ;
            }
        }

      if (zMapConfigIniContextGetFilePath(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                          ZMAPSTANZA_BLIXEM_CONF_FILE, &tmp_string))
        {
          if (tmp_string)
            {
              file_prefs->config_file = tmp_string ;
              file_prefs->is_set.config_file = TRUE ;
            }
          else
            {
              g_free(tmp_string) ;
            }
        }


      if (zMapConfigIniContextGetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                    ZMAPSTANZA_BLIXEM_SCOPE, &tmp_int)
          && tmp_int != 0)
        {
          file_prefs->scope = tmp_int ;
          file_prefs->is_set.scope = TRUE ;
        }


      /* NOTE that if scope_from_mark is TRUE then features_from_mark must be. */
      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                        ZMAPSTANZA_BLIXEM_SCOPE_MARK, &tmp_bool))
        {
          file_prefs->scope_from_mark = tmp_bool;
          file_prefs->is_set.scope_from_mark = TRUE ;
        }

      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                        ZMAPSTANZA_BLIXEM_FEATURES_MARK, &tmp_bool))
        {
          file_prefs->features_from_mark = tmp_bool;
          file_prefs->is_set.features_from_mark = TRUE ;
        }

      if (file_prefs->scope_from_mark)
        {
          file_prefs->features_from_mark = TRUE ;
          file_prefs->is_set.scope_from_mark = TRUE ;
        }


      if (zMapConfigIniContextGetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                    ZMAPSTANZA_BLIXEM_MAX, &tmp_int)
          && tmp_int != 0)
        {
          file_prefs->homol_max = tmp_int ;
          file_prefs->is_set.homol_max = TRUE ;
        }

      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                        ZMAPSTANZA_BLIXEM_KEEP_TEMP, &tmp_bool))
        {
          file_prefs->keep_tmpfiles = tmp_bool;
          file_prefs->is_set.keep_tmpfiles = TRUE ;
        }

      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                        ZMAPSTANZA_BLIXEM_SLEEP, &tmp_bool))
        {
          file_prefs->sleep_on_startup = tmp_bool;
          file_prefs->is_set.sleep_on_startup = TRUE ;
        }

      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                        ZMAPSTANZA_BLIXEM_KILL_EXIT, &tmp_bool))
        {
          file_prefs->kill_on_exit = tmp_bool;
          file_prefs->is_set.kill_on_exit = TRUE ;
        }

      /* dna-featuresets and protein-featuresets have been replaced with a single featuresets
       * list. Check for them for backwards compatibility and add them into the assoc_featuresets
       * list if found. */
      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                       ZMAPSTANZA_BLIXEM_DNA_FS, &tmp_string))
        {
          GList *dna_sets = zMapConfigString2QuarkList(tmp_string,FALSE);
          file_prefs->assoc_featuresets = g_list_concat(file_prefs->assoc_featuresets, dna_sets) ;
          g_free(tmp_string);
          file_prefs->is_set.assoc_featuresets = TRUE ;
        }

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                       ZMAPSTANZA_BLIXEM_PROT_FS, &tmp_string))
        {
          GList *protein_sets = zMapConfigString2QuarkList(tmp_string,FALSE);
          file_prefs->assoc_featuresets = g_list_concat(file_prefs->assoc_featuresets, protein_sets) ;
          g_free(tmp_string);
          file_prefs->is_set.assoc_featuresets = TRUE ;
        }

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                       ZMAPSTANZA_BLIXEM_FS, &tmp_string))
        {
          GList *assoc_featuresets = zMapConfigString2QuarkList(tmp_string,FALSE);
          file_prefs->assoc_featuresets = g_list_concat(file_prefs->assoc_featuresets, assoc_featuresets) ;
          g_free(tmp_string);
          file_prefs->is_set.assoc_featuresets = TRUE ;
        }

      zMapConfigIniContextDestroy(context);
    }

  file_prefs->init = TRUE;
}


/* Validate the user prefs and fill in any missing required values with defaults. */
static gboolean finaliseUserPrefs(BlixemConfigData file_prefs)
{
  gboolean status = FALSE ;
  zMapReturnValIfFailSafe(file_prefs, status) ;

  /* We need a blixem script. If it wasn't given, set a default */
  if (!file_prefs->script)
    {
      file_prefs->script = g_strdup(BLIX_DEFAULT_SCRIPT) ;
    }

  /* Check that script exists and is executable */
  if (file_prefs->script)
    {
      char *tmp = NULL ;
      tmp = file_prefs->script;

      if ((file_prefs->script = g_find_program_in_path(tmp)))
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

  /* If a config file is specified, check that it can be found */
  if (file_prefs->config_file && !zMapFileAccess(file_prefs->config_file, "r"))
    {
      zMapShowMsg(ZMAP_MSG_WARNING,
                  "The Blixem config file \"%s\" cannot be found in your path or it is not read/writeable.",
                  file_prefs->config_file) ;
    }

  return status ;
}


/* Override the preferences in dest_prefs with those given in src_prefs, if set. This takes
 * ownership of any allocated memory in src_prefs. */
static void overridePrefs(BlixemConfigData dest_prefs, BlixemConfigData src_prefs)
{
  zMapReturnIfFailSafe(src_prefs && dest_prefs) ;

  /* If src_prefs is initialised then copy any prefs set in it into dest prefs,
   * thus overriding dest prefs with src prefs. */
  if (src_prefs->init)
    {
      if (src_prefs->is_set.scope)
        {
          dest_prefs->scope = src_prefs->scope ;
          dest_prefs->is_set.scope = TRUE ;
        }

      if (src_prefs->is_set.scope_from_mark)
        {
          dest_prefs->scope_from_mark = src_prefs->scope_from_mark ;
          dest_prefs->is_set.scope_from_mark = TRUE ;
        }

      if (src_prefs->is_set.features_from_mark)
        {
          dest_prefs->features_from_mark = src_prefs->features_from_mark ;
          dest_prefs->is_set.features_from_mark = TRUE ;
        }

      if (src_prefs->is_set.homol_max)
        {
          dest_prefs->homol_max = src_prefs->homol_max ;
          dest_prefs->is_set.homol_max = TRUE ;
        }

      if (src_prefs->is_set.netid)
        {
          if (dest_prefs->netid)
            g_free(dest_prefs->netid) ;

          dest_prefs->netid = src_prefs->netid ;
          dest_prefs->is_set.netid = TRUE ;
        }

      if (src_prefs->is_set.port)
        {
          dest_prefs->port = src_prefs->port ;
          dest_prefs->is_set.port = TRUE ;
        }

      if (src_prefs->is_set.config_file)
        {
          if (dest_prefs->config_file)
            g_free(dest_prefs->config_file) ;

          dest_prefs->config_file = src_prefs->config_file ;
          dest_prefs->is_set.config_file = TRUE ;
        }

      if (src_prefs->is_set.script)
        {
          if (dest_prefs->script)
            g_free(dest_prefs->script) ;

          dest_prefs->script = src_prefs->script ;
          dest_prefs->is_set.script = TRUE ;
        }

      if (src_prefs->is_set.keep_tmpfiles)
        {
          dest_prefs->keep_tmpfiles = src_prefs->keep_tmpfiles ;
          dest_prefs->is_set.keep_tmpfiles = TRUE ;
        }

      if (src_prefs->is_set.sleep_on_startup)
        {
          dest_prefs->sleep_on_startup = src_prefs->sleep_on_startup ;
          dest_prefs->is_set.sleep_on_startup = TRUE ;
        }

      if (src_prefs->is_set.kill_on_exit)
        {
          dest_prefs->kill_on_exit = src_prefs->kill_on_exit ;
          dest_prefs->is_set.kill_on_exit = TRUE ;
        }

      if (src_prefs->is_set.assoc_featuresets)
        {
          if (dest_prefs->assoc_featuresets)
            g_list_free(dest_prefs->assoc_featuresets) ;

          dest_prefs->assoc_featuresets = src_prefs->assoc_featuresets ;
          dest_prefs->is_set.assoc_featuresets = TRUE ;
        }
    }
}


/* Get any user preferences specified in the given config file and/or the global user settings. */
static gboolean getUserPrefs(ZMapView view, char *config_file, BlixemConfigData curr_prefs)
{
  gboolean status = FALSE ;
  zMapReturnValIfFailSafe(curr_prefs, status) ;

  BlixemConfigDataStruct config_file_prefs = {FALSE} ;
  BlixemConfigDataStruct user_prefs = {FALSE} ;

  /* Get the prefs from any user-specified config file */
  getUserPrefsFile(view, config_file, curr_prefs, &config_file_prefs, ZMAPCONFIG_FILE_USER) ;

  /* Get the global user preferences */
  getUserPrefsFile(view, NULL, curr_prefs, &user_prefs, ZMAPCONFIG_FILE_PREFS) ;

  /* Override the config file prefs with the global user prefs */
  overridePrefs(&config_file_prefs, &user_prefs) ;
  
  /* Override all file prefs with any currently-set prefs in this zmap session */
  overridePrefs(&config_file_prefs, curr_prefs) ;

  /* Reset is-set flags to those in curr_prefs, that is, we only want to flag that a value has
   * been "set" if it has been modified by the user in this session. */
  config_file_prefs.is_set = curr_prefs->is_set ;                    /* struct copy. */

  /* Validate and fill in any missing values with defaults */
  status = finaliseUserPrefs(&config_file_prefs) ;

  /* Then copy file prefs into curr_prefs so that curr prefs now represents latest config 
   * file and user prefs combined. */
  *curr_prefs = config_file_prefs ;                                    /* Struct copy. */

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

  if (blixem_data->assoc_featuresets)
    g_list_free(blixem_data->assoc_featuresets) ;
  if (curr_prefs->assoc_featuresets)
    blixem_data->assoc_featuresets = zMapFeatureCopyQuarkList(curr_prefs->assoc_featuresets) ;

  if (blixem_data->column_groups)
    g_list_free(blixem_data->column_groups) ;
  blixem_data->column_groups = NULL ;

  return ;
}


/* Set blixem scope for sequence/features and initial position of blixem window on sequence. */
static gboolean setBlixemScope(ZMapBlixemData blixem_data, const bool features_from_mark)
{
  gboolean status = FALSE ;
  static gboolean scope_debug = FALSE ;
  if (!blixem_data)
    return status ;
  status = TRUE ;

  if (status)
    {
      gboolean have_mark = FALSE ;
      int scope_range = 0 ;

      scope_range = blixem_data->scope / 2 ;

      if (blixem_data->mark_start < blixem_data->block->block_to_sequence.block.x1)
        blixem_data->mark_start = blixem_data->block->block_to_sequence.block.x1 ;
      if (blixem_data->mark_end > blixem_data->block->block_to_sequence.block.x2)
        blixem_data->mark_end = blixem_data->block->block_to_sequence.block.x2 ;


      /* Use the mark coords to set scope ? */
      have_mark = (blixem_data->mark_start && blixem_data->mark_end) ;

      /* Set min/max range for blixem scope. */
      if (have_mark && blixem_data->scope_from_mark)
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

      /* If features-from-mark is set (either in the prefs or in the passed-in variable, which
       * overrides the prefs) then set the features range to the mark, if the mark is set */
      if (have_mark && (features_from_mark || blixem_data->features_from_mark))
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
      if (zMapViewGetRevCompStatus(blixem_data->view))
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

      if (zMapViewGetRevCompStatus(blixem_data->view))
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
  if (status && zMapViewGetRevCompStatus(blixem_data->view))
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

  return status ;
}


/* Write the GFF header into the output file */
static gboolean writeGFFHeader(ZMapBlixemData blixem_data)
{
  gboolean status = FALSE ;
  zMapReturnValIfFail(blixem_data, status) ;

  /* Get the GFF header text */
  zMapGFFFormatHeader(TRUE, blixem_data->line,
                      g_quark_to_string(blixem_data->block->original_id), 
                      blixem_data->features_min, blixem_data->features_max) ;

  /* Initialise the output file with the header text */
  char * err_out = NULL ;

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

  return status ;
}


/* If a max number of homols is specified, then clip the align_list to that number of
 * entries. This keeps the highest scoring homols. */
static void limitToMaxHomol(ZMapBlixemData blixem_data)
{
  zMapReturnIfFail(blixem_data) ;

  if (blixem_data->homol_max)
    {
      const int num_homols = g_list_length(blixem_data->align_list) ;
      if (num_homols && blixem_data->homol_max < num_homols)
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
}


/* Set the align_list to the list of requested dna or peptide alignments
 * NOTE the request data is a column which contains multiple featuresets.
 * We have to include a line for each one. */
static void writeRequestedHomologyList(ZMapBlixemData blixem_data)
{
  zMapReturnIfFail(blixem_data) ;

  /* Set the flag so that we only process alignments */
  blixem_data->do_alignments = TRUE ;
  blixem_data->do_transcripts = FALSE ;
  blixem_data->do_basic = FALSE ;

  if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_FEATURES
      || blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_EXPANDED)
    {
      /* mh17: BAM compressed features can get the same names, we don't want to fetch others in */
      if ((g_list_length(blixem_data->features) > 1) || zMapStyleIsUnique(blixem_data->feature_set->style))
        {
          blixem_data->align_list = blixem_data->features ;
        }
      else
        {
          ZMapFeature feature = (ZMapFeature)(blixem_data->features->data) ;

          blixem_data->align_list = zMapFeatureSetGetNamedFeatures(blixem_data->feature_set, feature->original_id) ;
        }
    }
  else if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_SET)
    {
      /* Exclude bam featuresets */
      if (!blixem_data->view || !blixem_data->view->context_map.isSeqFeatureSet(blixem_data->feature_set->unique_id))
        g_hash_table_foreach(blixem_data->feature_set->features, getAlignFeatureCB, blixem_data) ;
    }
  else if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_MULTISET)
    {
      /* Include all features in the associated alignment featuresets. Note that this only adds
       * features that are alignments of the correct type (dna/protein) */
      if (blixem_data->assoc_featuresets)
        g_list_foreach(blixem_data->assoc_featuresets, featureSetGetAlignList, blixem_data) ;
    }

  /* If a max number of homols is set, clip the list to it */
  limitToMaxHomol(blixem_data) ;

  /* Write the alignments in the list to the file */
  if (blixem_data->align_list)
    g_list_foreach(blixem_data->align_list, writeFeatureLineList, blixem_data) ;
}


/* Write out a single line to the gff to indicate to blixem it needs to fetch bam data for a
 * particular source/region */
static void writeBAMLine(ZMapBlixemData blixem_data, const GQuark featureset_id, GString *attribute)
{
  zMapReturnIfFail(blixem_data && blixem_data->sequence_map) ;

  const char *sequence_name = g_quark_to_string(blixem_data->block->original_id) ;
  static const char * SO_region = "region" ;
  const char *featureset_name = g_quark_to_string(featureset_id) ;

  /* We'll check if the source has a script or file that we can pass to blixem in our custom
     'command' or 'file' gff tags */
  ZMapURL url = NULL ;

  gboolean ok = zMapGFFFormatMandatory(blixem_data->over_write, 
                                       blixem_data->line,
                                       sequence_name,
                                       featureset_name,
                                       SO_region,
                                       blixem_data->features_min,
                                       blixem_data->features_max,
                                       (float)0.0,
                                       ZMAPSTRAND_NONE, 
                                       ZMAPPHASE_NONE,
                                       TRUE,
                                       TRUE) ;

  if (ok)
    {
      /* Check if the url contains a script that we can pass as a command */
      char *url_str = blixem_data->sequence_map->getSourceURL(featureset_name) ;

      if (url_str)
        {
          int error = 0 ;
          url = url_parse(url_str, &error) ;
          g_free(url_str) ;
        }
    }

  if (ok && url && url->path)
    {
      if (url->scheme == SCHEME_PIPE)
        {
          char *args = NULL ;

          if (url->query)
            {
              /* Unescape the args in the url */
              args = g_uri_unescape_string(url->query, NULL) ;
              if (!args)
                args = g_strdup(url->query) ;

              /* Escape gff special chars */
              char *tmp = zMapGFFEscape(args) ;

              if (tmp)
                {
                  g_free(args) ;
                  args = tmp ;
                }

              /* Replace & between args with a space */
              if (zMapGFFStringUtilsSubstringReplace(args, "&", " ", &tmp))
                {
                  g_free(args) ;
                  args = tmp ;
                  tmp = NULL ;
                }
            }
          else
            {
              args = g_strdup("") ;
            }

          /* Replace start and end in the url with the blixem scope. */
          /* Actually, for now just append them to the end as this works for bam_get and it's
           * hard to program this more generically because we don't know whether the script has 
           * start/end args anyway. */
          g_string_append_printf(attribute,
                                 "command=%s %s --start%%3D%d --end%%3D%d",
                                 url->path, args, 
                                 blixem_data->features_min, blixem_data->features_max) ;
          g_free(args) ;
        }
      else if (url->scheme == SCHEME_FILE)
        {
          g_string_append_printf(attribute, "file=%s", url->path) ;
        }

      if (attribute->len > 0)
        {
          zMapGFFFormatAppendAttribute(blixem_data->line, attribute, FALSE, FALSE) ;
          g_string_append_c(blixem_data->line, '\n') ;
          g_string_truncate(attribute, (gsize)0);

          zMapGFFOutputWriteLineToGIO(blixem_data->gff_channel, &(blixem_data->errorMsg),
                                      blixem_data->line, TRUE) ;
        }
    }

  if (url)
    xfree(url) ;
}


/* If the requested set is a coverage column then write its data to the output file */
static void writeCoverageColumn(ZMapBlixemData blixem_data)
{
  zMapReturnIfFail(blixem_data) ;

  /* The multiset option will set coverage_done to true if it finds and successfully writes out
   * the group (actually, just any member in the group, but that's good enough). We need to check
   * this because it's possible the user attempted to blixem associated featuresets for a column
   * that is not in a group, in which case this flag will remain false and we will process the
   * single selected featureset instead. */
  blixem_data->coverage_done = FALSE ;

  /* Write out any associated bam featuresets */
  if (blixem_data->assoc_featuresets)
    g_list_foreach(blixem_data->assoc_featuresets, featureSetWriteBAMList, blixem_data) ;
  
  if (!blixem_data->coverage_done)
    {
      /* Write out the selected featureset(s) if it is bam */
      GList *l = NULL;
      GString *attribute = g_string_new(NULL) ;
      
      for(l = blixem_data->source; l ; l = l->next)
        {
          GQuark featureset_id = (GQuark)(GPOINTER_TO_UINT(l->data)) ;
          GQuark unique_id = zMapFeatureSetCreateID(g_quark_to_string(featureset_id)) ;

          if (blixem_data->view && blixem_data->view->context_map.isSeqFeatureSet(unique_id))
            writeBAMLine(blixem_data, featureset_id, attribute) ;
        }

      /* Free our string buffer. */
      g_string_free(attribute, TRUE) ;
    }

}


/* Called for each column group in the context. Adds the group to the blixem_data column_groups
 * list if the selected featureset is in the group. */
static void getColumnGroupCB(gpointer key, gpointer data, gpointer user_data)
{
  GQuark group_id = (GQuark)GPOINTER_TO_INT(key) ;
  ZMapBlixemData blixem_data = (ZMapBlixemData)user_data ;
  GList *group_featuresets = (GList*)data ;

  zMapReturnIfFail(blixem_data && group_featuresets) ;

  for (GList *group_item = group_featuresets; group_item ; group_item = group_item->next)
    {
      GQuark group_featureset = (GQuark)GPOINTER_TO_INT(group_item->data) ;
      GQuark group_featureset_id = zMapStyleCreateID(g_quark_to_string(group_featureset)) ;

      /* Check if the featureset is in this group. This is the given featureset for a
       * regular multiset command or the list of sources if given e.g. for a BAM multiset command. */
      gboolean found = FALSE ;

      if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_MULTISET)
        {
          if (blixem_data->source)
            {
              for (GList *item = blixem_data->source; item && !found; item = item->next)
                {
                  GQuark featureset_id = (GQuark)GPOINTER_TO_INT(item->data) ;
                  featureset_id = zMapStyleCreateID(g_quark_to_string(featureset_id))  ;
                  
                  found = (featureset_id == group_featureset_id) ;
                }
            }
          else if (blixem_data->feature_set)
            {
              found = (blixem_data->feature_set->unique_id == group_featureset_id) ;
            }
        }

      /* If this is the right group, add it to the result and exit */
      if (found)
        {
          blixem_data->column_groups = g_list_append(blixem_data->column_groups, GINT_TO_POINTER(group_id)) ;
          break ;
        }
    }
}


/* Get the column group(s) that the selected feature_set is in, if any. Sets the result in the
 * column_groups member of the blixem_data. The result should be free'd with g_list_free. */
static void getColumnGroups(ZMapBlixemData blixem_data)
{
  zMapReturnIfFail(blixem_data && (blixem_data->feature_set || blixem_data->source)) ;

  if (blixem_data && blixem_data->view && blixem_data->view->context_map.column_groups)
    {
      /* Clear any previous list first */
      if (blixem_data->column_groups)
        {
          g_list_free(blixem_data->column_groups) ;
          blixem_data->column_groups = NULL ;
        }

      g_hash_table_foreach(blixem_data->view->context_map.column_groups, getColumnGroupCB, blixem_data) ;
    }
}


/* Called when the user toggles the radio button on the group dialog. Updates the value in the
 * user data. */
static void radio_button_cb(GtkToggleButton *togglebutton, gpointer user_data)
{
  RadioButtonData data = (RadioButtonData)user_data ;

  if (gtk_toggle_button_get_active(togglebutton))
    {
      *(data->result) = data->group_id ;
    }
}


/* Show a dialog to ask the user which column-group they want to use. Returns 0 if the user
 * cancels or the column group id otherwise */
static GQuark showGroupDialog(ZMapBlixemData blixem_data)
{
  GQuark result = 0 ;
  zMapReturnValIfFail(blixem_data && blixem_data->column_groups, result) ;

  GtkWidget *dialog = gtk_dialog_new_with_buttons("Blixem column group",
                                                  NULL,
                                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                  GTK_STOCK_OK, 
                                                  GTK_RESPONSE_ACCEPT,
                                                  GTK_STOCK_CANCEL, 
                                                  GTK_RESPONSE_CANCEL,
                                                  NULL) ;


  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT) ;

  GtkBox *vbox = GTK_BOX((GTK_DIALOG(dialog))->vbox) ;
  gtk_box_pack_start(vbox, gtk_label_new("Select which column group you would like to Blixem:"), FALSE, FALSE, 0) ;

  GtkBox *hbox = GTK_BOX(gtk_hbox_new(FALSE, 0)) ;
  gtk_box_pack_start(vbox, GTK_WIDGET(hbox), FALSE, FALSE, 0) ;

  GtkBox *vbox1 = GTK_BOX(gtk_vbox_new(FALSE, 0)) ; /* labels */
  GtkBox *vbox2 = GTK_BOX(gtk_vbox_new(FALSE, 0)) ; /* radio buttons */
  gtk_box_pack_start(hbox, GTK_WIDGET(vbox1), FALSE, FALSE, 5) ;
  gtk_box_pack_start(hbox, GTK_WIDGET(vbox2), FALSE, FALSE, 5) ;

  GtkWidget *radio_group = NULL ;
  static GQuark last_selection = 0 ;
  GList *free_me = NULL ;

  for (GList *item = blixem_data->column_groups; item ; item = item->next)
    {
      RadioButtonData data = g_new0(RadioButtonDataStruct, 1) ;
      free_me = g_list_append(free_me, data) ;

      data->result = &result ;

      data->group_id = (GQuark)GPOINTER_TO_INT(item->data) ;
      GtkWidget *label = gtk_label_new(g_quark_to_string(data->group_id)) ;
      gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0) ;
      gtk_box_pack_start(vbox1, label, FALSE, FALSE, 0) ;

      GtkWidget *radio_button = NULL ;

      if (!radio_group)
        {
          radio_button = radio_group = gtk_radio_button_new(NULL) ;

          /* default to first group found */
          result = data->group_id ; 
          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button), TRUE) ;
        }
      else
        {
          radio_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(radio_group)) ;
        }

      /* If this is the group that we selected last time then select it by default */
      if (data->group_id == last_selection)
        {
          result = data->group_id ; 
          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button), TRUE) ;
        }        

      g_signal_connect(G_OBJECT(radio_button), "toggled", G_CALLBACK(radio_button_cb), data) ;
      gtk_box_pack_start(vbox2, radio_button, FALSE, FALSE, 0) ;
    }
  
  gtk_widget_show_all(dialog) ;

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_CANCEL)
    result = 0 ;
  else
    last_selection = result ;

  GList *item = free_me;

  for ( ; item; item = item->next)
    g_free(item->data) ;

  g_list_free(free_me);

  gtk_widget_destroy(dialog) ;

  return result ;
}


/* Get the list of associated featuresets from the column groups and append them to
 * any list already set from the config file (from the blixem stanza).  */
static void getAssocFeaturesets(ZMapBlixemData blixem_data)
{
  zMapReturnIfFail(blixem_data) ;

  if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_MULTISET)
    {
      /* First, populate the list of column groups that the selected feature set is in */
      getColumnGroups(blixem_data) ;

      if (blixem_data->column_groups)
        {
          GQuark group_id = (GQuark)GPOINTER_TO_INT(blixem_data->column_groups->data) ;

          if (g_list_length(blixem_data->column_groups) > 1)
            group_id = showGroupDialog(blixem_data) ;

          if (group_id)
            {
              GList *group_featuresets = (GList*)g_hash_table_lookup(blixem_data->view->context_map.column_groups, 
                                                                           GINT_TO_POINTER(group_id)) ;

              /* Make a copy so we have ownership */
              GList *new_list = zMapFeatureCopyQuarkList(group_featuresets) ;

              blixem_data->assoc_featuresets = g_list_concat(blixem_data->assoc_featuresets, new_list) ;
            }
        }
    }
}


static gboolean writeFeatureFiles(ZMapBlixemData blixem_data)
{
  zMapReturnValIfFail(blixem_data, FALSE) ;

  gboolean status = TRUE ;
  GError *channel_error = NULL ;
  const int start = blixem_data->features_min ;
  const int end = blixem_data->features_max ; ;

  /* Rev-comp the coords if necessary */
  if (zMapViewGetRevCompStatus(blixem_data->view))
    zMapFeatureReverseComplementCoords(blixem_data->view->features, &blixem_data->features_min, &blixem_data->features_max) ;

  /* Write the headers */
  status = writeGFFHeader(blixem_data) ;

  /* Write the features. */
  if (status)
    {
      ZMapFeatureSet feature_set = blixem_data->feature_set ;
      blixem_data->errorMsg = NULL ;
      blixem_data->align_list = NULL ;
      getAssocFeaturesets(blixem_data) ;

      /* Do coverage columns */
      writeCoverageColumn(blixem_data) ;

      /* gb10: not sure if these need reverting here but doing this to maintain original
       * behaviour */
      blixem_data->features_min = start ;
      blixem_data->features_max = end ;

      /* Do requested Homology data */
      writeRequestedHomologyList(blixem_data) ;

      /* If user clicked on something not an alignment then show that feature
       * and others nearby in blixems overview window. */
      if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_NONE && feature_set)
        {
          blixem_data->do_alignments = FALSE ;
          blixem_data->do_transcripts = TRUE ;
          blixem_data->do_basic = TRUE ;

          g_hash_table_foreach(feature_set->features, writeFeatureLineHash, blixem_data) ;
        }

      /* Now do transcripts and other associated featuresets. */
      if (blixem_data->assoc_featuresets)
        {
          /* Set do_alignments = false so we don't include alignments from the associated
           * featuresets as we've already done these with the requested homols */
          blixem_data->do_alignments = FALSE ;      
          blixem_data->do_transcripts = TRUE ;
          blixem_data->do_basic = TRUE ;

          g_list_foreach(blixem_data->assoc_featuresets, writeFeatureSetList, blixem_data) ;
        }
      else
        {
          blixem_data->do_alignments = TRUE ;
          blixem_data->do_transcripts = TRUE ;
          blixem_data->do_basic = TRUE ;

          g_hash_table_foreach(blixem_data->block->feature_sets, writeFeatureSetHash, blixem_data) ;
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

  blixem_data->features_min = start ;
  blixem_data->features_max = end ;
   
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
static void writeFeatureSetHash(gpointer key, gpointer data, gpointer user_data)
{
  GQuark set_id = GPOINTER_TO_UINT(data) ;
  ZMapBlixemData blixem_data = (ZMapBlixemData)user_data ;

  writeFeatureSet(set_id, blixem_data) ;
}

static void writeFeatureSetList(gpointer data, gpointer user_data)
{
  GQuark set_id = GPOINTER_TO_UINT(data) ;
  ZMapBlixemData blixem_data = (ZMapBlixemData)user_data ;

  writeFeatureSet(set_id, blixem_data) ;
}

/* This function does the work to write out a named feature set to file.*/
static void writeFeatureSet(GQuark set_id, ZMapBlixemData blixem_data)
{
  zMapReturnIfFail(blixem_data) ;

  GQuark canon_id = 0 ;
  ZMapFeatureSet feature_set = NULL ;

  canon_id = zMapFeatureSetCreateID((char *)g_quark_to_string(set_id)) ;

  feature_set = (ZMapFeatureSet)g_hash_table_lookup(blixem_data->block->feature_sets, GINT_TO_POINTER(canon_id));

  if (feature_set && feature_set->style)
    {
      /* Check if we're processing this type of style */
      gboolean process = FALSE ;

      switch (feature_set->style->mode)
        {
        case ZMAPSTYLE_MODE_ALIGNMENT:
          process = blixem_data->do_alignments ;
          break ;
      
        case ZMAPSTYLE_MODE_TRANSCRIPT:
          process = blixem_data->do_transcripts ;
          break ;

        default: /* everything else */
          process = blixem_data->do_basic ;
          break ;
        }

      if (process)
        g_hash_table_foreach(feature_set->features, writeFeatureLineHash, blixem_data);
    }

  return ;
}


/* These two functions enable writeFeatureLine() to be called from a g_hash or a g_list
 * foreach function. */
static void writeFeatureLineHash(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  ZMapBlixemData  blixem_data = (ZMapBlixemData)user_data ;

  writeFeatureLine(feature, blixem_data) ;

  return ;
}

static void writeFeatureLineList(gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  ZMapBlixemData  blixem_data = (ZMapBlixemData)user_data ;

  writeFeatureLine(feature, blixem_data) ;

  return ;
}


/* Returns true if the given feature should be included, false otherwise */
static gboolean includeFeature(ZMapBlixemData blixem_data, ZMapFeature feature)
{
  gboolean include_feature = FALSE ;
  zMapReturnValIfFail(blixem_data && feature, include_feature) ;

  gboolean fully_contained = FALSE ;   /* TRUE => feature must be fully
                                          contained, FALSE => just overlapping. */

  if (fully_contained)
    include_feature = (feature->x1 <= blixem_data->features_max && feature->x2 >= blixem_data->features_min) ;
  else
    include_feature =  !(feature->x1 > blixem_data->features_max || feature->x2 < blixem_data->features_min) ;

  return include_feature ;
}


/* Write out the given feature to file if it is an alignment and it is of the correct type */
static gboolean writeFeatureLineAlignment(ZMapBlixemData blixem_data, ZMapFeature feature)
{
  gboolean status = FALSE ;
  zMapReturnValIfFail(blixem_data && feature, status) ;

  if (blixem_data->do_alignments &&
      feature->mode == ZMAPSTYLE_MODE_ALIGNMENT && 
      feature->feature.homol.type == blixem_data->align_type)
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
          
          status = zMapGFFFormatAttributeSetAlignment(blixem_data->attribute_flags) ;

          status &= zMapGFFWriteFeatureAlignment(feature, blixem_data->attribute_flags, blixem_data->line,
                                                 blixem_data->over_write, seq_str) ;
        }
    }

  return status ;
}


/* Write out the given feature to file if it is a transcript */
static gboolean writeFeatureLineTranscript(ZMapBlixemData blixem_data, ZMapFeature feature)
{
  gboolean status = FALSE ;
  zMapReturnValIfFail(blixem_data && feature, status) ;

  if (blixem_data->do_transcripts && feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      ZMapFeatureSet feature_set = (ZMapFeatureSet)(feature->parent) ;
      GQuark feature_set_id = feature_set->unique_id ;

      // Complicated but user can click on a transcript column and then we can end up
      // exporting some transcripts twice, once for the column clicked on and once for the
      // transcript set.
      if (blixem_data->align_set != ZMAPWINDOW_ALIGNCMD_NONE
          || ((!(blixem_data->assoc_featuresets)
               || !(g_list_find(blixem_data->assoc_featuresets, GINT_TO_POINTER(feature_set_id))))))
        {
          status = zMapGFFFormatAttributeSetTranscript(blixem_data->attribute_flags) ;

          status &= zMapGFFWriteFeatureTranscript(feature, blixem_data->attribute_flags,
                                                  blixem_data->line, blixem_data->over_write) ;
        }
    }

  return status ;
}


static gboolean writeFeatureLineBasic(ZMapBlixemData blixem_data, ZMapFeature feature)
{
  gboolean status = FALSE ;
  zMapReturnValIfFail(blixem_data && feature, status) ;

  if (blixem_data->do_basic && feature->mode == ZMAPSTYLE_MODE_BASIC)
    {
      status = zMapGFFFormatAttributeSetBasic(blixem_data->attribute_flags) ;

      status &= zMapGFFWriteFeatureBasic(feature, blixem_data->attribute_flags,
                                         blixem_data->line, blixem_data->over_write) ;
    }

  return status ;
}


/* This function does the work to write a given feature to the output file */
static void writeFeatureLine(ZMapFeature feature, ZMapBlixemData  blixem_data)
{
  zMapReturnIfFail(feature && blixem_data) ;

  /* There is no way to interrupt g_hash_table_foreach() so instead if errorMsg is set
   * then it means there was an error in processing so we don't process any
   * more records, displaying the error when we return. */
  zMapReturnIfFailSafe(!blixem_data->errorMsg) ;

  /* Only process features we want to dump. We then filter those features according to the
   * following rules (inherited from acedb): alignment features must be wholly within the
   * blixem max/min to be included, for transcripts we include as many introns/exons as will fit. */

  if (includeFeature(blixem_data, feature))
    {
      /* First, revcomp the feature if necessary */
      if (zMapViewGetRevCompStatus(blixem_data->view))
        zMapFeatureReverseComplement(blixem_data->view->features, feature) ;

      /* Unset any GFF attributes */
      zMapGFFFormatAttributeUnsetAll(blixem_data->attribute_flags) ;

      gboolean status = FALSE ;

      if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT)
        status = writeFeatureLineAlignment(blixem_data, feature) ;
      else if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
        status = writeFeatureLineTranscript(blixem_data, feature) ;
      else if (feature->mode == ZMAPSTYLE_MODE_BASIC)
        status = writeFeatureLineBasic(blixem_data, feature) ;

      if (status)
        {
          status = zMapGFFOutputWriteLineToGIO(blixem_data->gff_channel,
                                               &(blixem_data->errorMsg),
                                               blixem_data->line, TRUE) ;
        }

      /* Undo any revcomp */
      if (zMapViewGetRevCompStatus(blixem_data->view))
        zMapFeatureReverseComplement(blixem_data->view->features, feature) ;
    }

  return ;
}


/* Callback called on each featureset in a GList. This writes out the gff line
 * to bam-fetch the features if this is a bam featureset type */
static void featureSetWriteBAMList(gpointer data, gpointer user_data)
{
  GQuark set_id = GPOINTER_TO_UINT(data) ;
  GQuark canon_id = 0 ;
  ZMapBlixemData blixem_data = (ZMapBlixemData)user_data ;
  ZMapFeatureSet feature_set = NULL ;

  if (!blixem_data)
    return ;

  canon_id = zMapFeatureSetCreateID((char *)g_quark_to_string(set_id)) ;

  feature_set = (ZMapFeatureSet)g_hash_table_lookup(blixem_data->block->feature_sets, GINT_TO_POINTER(canon_id));

  if(feature_set)
    {
      /* check it's a bam type */
      if (blixem_data->view && blixem_data->view->context_map.isSeqFeatureSet(feature_set->unique_id))
        {
          /* Indicate that we've processed a valid featureset */
          blixem_data->coverage_done = TRUE ;

          GString *attribute = g_string_new(NULL) ;

          writeBAMLine(blixem_data, feature_set->original_id, attribute) ;
            
          /* Free our string buffer. */
          g_string_free(attribute, TRUE) ;
        }
    }
}



/* Callback called on each featureset in a GList. This writes each feature from the featureset
 * into the align_list in the data. */
static void featureSetGetAlignList(gpointer data, gpointer user_data)
{
  GQuark set_id = GPOINTER_TO_UINT(data) ;
  GQuark canon_id = 0 ;
  ZMapBlixemData blixem_data = (ZMapBlixemData)user_data ;
  ZMapFeatureSet feature_set = NULL ;
  GList *column_2_featureset = NULL ;

  if (!blixem_data || !blixem_data->view)
    return ;

  canon_id = zMapFeatureSetCreateID((char *)g_quark_to_string(set_id)) ;

  feature_set = (ZMapFeatureSet)g_hash_table_lookup(blixem_data->block->feature_sets, GINT_TO_POINTER(canon_id));

  if(feature_set)
    {
      /* Check it's an alignment type and is not a bam type */
      if (feature_set->style && 
          feature_set->style->mode == ZMAPSTYLE_MODE_ALIGNMENT &&
          !blixem_data->view->context_map.isSeqFeatureSet(feature_set->unique_id))
        {
          /* Also check that it's the correct alignment type (dna/protein) - we don't want to
           * check every feature individually if we know it's not relevant.  */
          if (zMapFeatureSetGetHomolType(feature_set) == blixem_data->align_type)
            g_hash_table_foreach(feature_set->features, getAlignFeatureCB, blixem_data);
        }
    }
  else
    {
      /* assuming a mis-config treat the set id as a column id */
      column_2_featureset = blixem_data->view->context_map.getColumnFeatureSets(canon_id,TRUE);

      if(!column_2_featureset)
        {
          zMapLogWarning("Could not find alignment feature set or column \"%s\" in context feature sets.",
                         g_quark_to_string(set_id)) ;
        }

      /* Loop through each featureset in the column and add all features to the align_list */
      for(;column_2_featureset;column_2_featureset = column_2_featureset->next)
        {
          feature_set = (ZMapFeatureSet)g_hash_table_lookup(blixem_data->block->feature_sets, column_2_featureset->data) ;
          
          if (feature_set && 
              feature_set->style && 
              feature_set->style->mode == ZMAPSTYLE_MODE_ALIGNMENT &&
              !blixem_data->view->context_map.isSeqFeatureSet(feature_set->unique_id))
            {
              g_hash_table_foreach(feature_set->features, getAlignFeatureCB, blixem_data) ;
            }
        }
    }

  return ;
}


/* Callback called on each feature in a featureset. Adds the feature to the align_list if it is
 * an alignment of the correct type */
static void getAlignFeatureCB(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  ZMapBlixemData  blixem_data = (ZMapBlixemData)user_data ;

  if (!feature || !blixem_data)
    return ;

  if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT &&
      feature->feature.homol.type == blixem_data->align_type)
    {
      if ( (*blixem_data->opts == 'X' && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
           || (*blixem_data->opts == 'N' && feature->feature.homol.type == ZMAPHOMOL_N_HOMOL))
        {
          blixem_data->align_list = g_list_append(blixem_data->align_list, feature) ;
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

          if (zMapViewGetRevCompStatus(blixem_data->view))
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
                                  start, end, zMapViewGetRevCompStatus(blixem_data->view)) ;

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
  gboolean allowed = TRUE;

  return allowed;
}


static ZMapGuiNotebookChapter makeChapter(ZMapGuiNotebook note_book_parent, ZMapView view)
{
  ZMapGuiNotebookChapter chapter = NULL ;
  static ZMapGuiNotebookCBStruct user_CBs =
    {
      cancelCB, NULL, applyCB, NULL, check_edited_values, NULL, saveCB, view
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

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Scope",
                                           "The maximum length of sequence to send to Blixem",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                           "int", blixem_config_curr_G.scope) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Restrict scope to Mark",
                                           "Restricts the scope to the current marked region",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
                                           "bool", blixem_config_curr_G.scope_from_mark) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Restrict features to Mark",
                                           "Still uses the default scope but only sends features that are within the marked region to Blixem",
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

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Config File",
                                           "The path to the configuration file to use for Blixem",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                           "string", blixem_config_curr_G.config_file) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Launch script",
                                           "The path to the Blixem executable",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_SIMPLE,
                                           "string", blixem_config_curr_G.script) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Keep temporary Files",
                                           "Tell Blixem not to destroy the temporary input files that ZMap sends to it.",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
                                           "bool", blixem_config_curr_G.keep_tmpfiles) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Sleep on Startup",
                                           "Tell Blixem to sleep on startup (for debugging)",
                                           ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
                                           "bool", blixem_config_curr_G.sleep_on_startup) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Kill Blixem on Exit",
                                           "Close all Blixems that were started from this ZMap when ZMap exits",
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
          /* Allow an empty string - this will unset the value */
          if (blixem_config_curr_G.netid != string_value &&
              (!blixem_config_curr_G.netid || !string_value ||
               strcmp(string_value, blixem_config_curr_G.netid) != 0))
            {
              /* Add validation ... */
              if (blixem_config_curr_G.netid)
                g_free(blixem_config_curr_G.netid) ;

              blixem_config_curr_G.netid = string_value ? g_strdup(string_value) : NULL ;
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
          /* Allow an empty string - this will unset the value */
          if (blixem_config_curr_G.config_file != string_value &&
              (!blixem_config_curr_G.config_file || !string_value ||
               strcmp(string_value, blixem_config_curr_G.config_file) != 0))
            {
              /* If a string is given, check it's a valid file */
              if (string_value && *string_value)
                {
                  result = zMapFileAccess(string_value, "r") ;

                  if (!result)
                    zMapWarning("Blixem Config File '%s' is not readable.", string_value);
                }

              if (blixem_config_curr_G.config_file)
                g_free(blixem_config_curr_G.config_file) ;

              blixem_config_curr_G.config_file = string_value ? g_strdup(string_value) : NULL ;
              blixem_config_curr_G.is_set.config_file = TRUE ;
            }
        }

      if (zMapGUINotebookGetTagValue(page, "Launch script", "string", &string_value))
        {
          /* Allow an empty string - this will unset the value */
          if (blixem_config_curr_G.script != string_value &&
              (!blixem_config_curr_G.script || !string_value ||
               strcmp(string_value, blixem_config_curr_G.script) != 0))
            {
              /* If a string is given, check it's a valid executable */
              if (string_value)
                {
                  result = zMapFileAccess(string_value, "x") ;
              
                  if (!result)
                    zMapWarning("Blixem Launch Script '%s' is not executable.", string_value);
                }
              
              if (blixem_config_curr_G.script)
                g_free(blixem_config_curr_G.script) ;

              blixem_config_curr_G.script = string_value ? g_strdup(string_value) : NULL ;
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

/* Update the given context with any preferences that have been changed by the user */
void zMapViewBlixemUserPrefsUpdateContext(ZMapConfigIniContext context, const ZMapConfigIniFileType file_type)
{ 
  BlixemConfigData prefs = &blixem_config_curr_G ;
  gboolean changed = FALSE ;

  if (prefs->is_set.netid)
    {
      changed = TRUE ;

      zMapConfigIniContextSetString(context, file_type,
                                    ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                    ZMAPSTANZA_BLIXEM_NETID, prefs->netid);
    }

  if (prefs->is_set.port)
    {
      changed = TRUE ;
      
      zMapConfigIniContextSetInt(context, file_type,
                                 ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                 ZMAPSTANZA_BLIXEM_PORT, prefs->port);
    }

  if (prefs->is_set.scope)
    {
      changed = TRUE ;

      zMapConfigIniContextSetInt(context, file_type,
                                 ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                 ZMAPSTANZA_BLIXEM_SCOPE, prefs->scope);
    }

  if (prefs->is_set.scope_from_mark)
    {
      changed = TRUE ;

      zMapConfigIniContextSetBoolean(context, file_type,
                                     ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                     ZMAPSTANZA_BLIXEM_SCOPE_MARK, prefs->scope_from_mark) ;
    }

  if (prefs->is_set.features_from_mark)
    {
      changed = TRUE ;
      
      zMapConfigIniContextSetBoolean(context, file_type,
                                     ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                     ZMAPSTANZA_BLIXEM_FEATURES_MARK, prefs->features_from_mark) ;
    }

  if (prefs->is_set.homol_max)
    {
      changed = TRUE ;

      zMapConfigIniContextSetInt(context, file_type,
                                 ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                 ZMAPSTANZA_BLIXEM_MAX, prefs->homol_max);
    }

  if (prefs->is_set.keep_tmpfiles)
    {
      changed = TRUE ;
      
      zMapConfigIniContextSetBoolean(context, file_type,
                                     ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                     ZMAPSTANZA_BLIXEM_KEEP_TEMP, prefs->keep_tmpfiles);
    }

  if (prefs->is_set.sleep_on_startup)
    {
      changed = TRUE ;
      
      zMapConfigIniContextSetBoolean(context, file_type,
                                     ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                     ZMAPSTANZA_BLIXEM_SLEEP, prefs->sleep_on_startup);
    }

  if (prefs->is_set.kill_on_exit)
    {
      changed = TRUE ;

      zMapConfigIniContextSetBoolean(context, file_type,
                                     ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                     ZMAPSTANZA_BLIXEM_KILL_EXIT, prefs->kill_on_exit);
    }

  if (prefs->is_set.script)
    {
      changed = TRUE ;

      zMapConfigIniContextSetString(context, file_type,
                                    ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                    ZMAPSTANZA_BLIXEM_SCRIPT, prefs->script);
    }

  if (prefs->is_set.config_file)
    {
      changed = TRUE ;

      zMapConfigIniContextSetString(context, file_type,
                                    ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
                                    ZMAPSTANZA_BLIXEM_CONF_FILE, prefs->config_file);
    }

  /* Set the unsaved flag in the context if there were any changes */
  if (changed)
    {
      zMapConfigIniContextSetUnsavedChanges(context, file_type, TRUE) ;
    }
}


/* Check if advanced prefs have changed and if so ask user to confirm this is ok. Returns true if
 * ok to continue, false otherwise. */
static gboolean checkAdvancedChanged()
{
  gboolean ok = TRUE ;
  BlixemConfigData prefs = &blixem_config_curr_G ;

  if (prefs->is_set.netid ||
      prefs->is_set.port ||
      prefs->is_set.script ||
      prefs->is_set.config_file ||
      prefs->is_set.keep_tmpfiles ||
      prefs->is_set.sleep_on_startup ||
      prefs->is_set.kill_on_exit)
    {
      ok = zMapGUIMsgGetBool(NULL, ZMAP_MSG_WARNING, " \
You are about to save changes to the advanced settings. This\n\
will override settings in any config files you use for all future\n\
future sessions, which may interfere with the way that ZMap works.\n\
\n\
Are you sure you want to continue?\n\
\n\
If you are not sure, hit Cancel. You can still hit\n\
Apply on the Preferences dialog to apply the changes\n\
just for this session.") ;
    }

  return ok ;
}


static void saveUserPrefs(BlixemConfigData prefs)
{
  zMapReturnIfFail(prefs) ;

  ZMapConfigIniContext context = NULL;

  /* Check that it's ok to continue */
  if (checkAdvancedChanged())
    {
      /* Create the context from the existing prefs file (if any - otherwise create an empty
       * context). */
      if ((context = zMapConfigIniContextProvide(NULL, ZMAPCONFIG_FILE_PREFS)))
        {
          /* Update the settings in the context. Note that the file type of 'user' means the
           * user-specified config file, i.e. the one we passed in to ContextProvide */
          ZMapConfigIniFileType file_type = ZMAPCONFIG_FILE_PREFS ;

          zMapViewBlixemUserPrefsUpdateContext(context, file_type) ;

          zMapConfigIniContextSave(context, file_type);

          zMapConfigIniContextDestroy(context) ;
        }
    }

  return ;
}


static void applyCB(ZMapGuiNotebookAny any_section, void *user_data_unused)
{
  readChapter((ZMapGuiNotebookChapter)any_section) ;

  return ;
}

static void saveCB(ZMapGuiNotebookAny any_section, void *user_data)
{
  ZMapGuiNotebookChapter chapter = (ZMapGuiNotebookChapter)any_section;
  ZMapView view = (ZMapView)user_data ;

  readChapter(chapter);

  zMapViewBlixemSaveChapter(chapter, view);

  return ;
}

static void cancelCB(ZMapGuiNotebookAny any_section, void *user_data_unused)
{

  return ;
}

/*************************** end of file *********************************/
