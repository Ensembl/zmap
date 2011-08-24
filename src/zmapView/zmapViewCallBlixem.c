/*  Last edited: Jul 12 08:24 2011 (edgrif) */
/*  File: zmapViewCallBlixem.c
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Prepares input files and passes them to blixem
 *              for display. Files are FASTA for reference sequence,
 *              either GFFv3 or seqbl/exblx format.
 *
 * Exported functions: see zmapView_P.h
 *
 *-------------------------------------------------------------------
 */

#include <unistd.h>					    /* for getlogin() */
#include <string.h>					    /* for memcpy */
#include <sys/types.h>					    /* for chmod() */
#include <sys/stat.h>					    /* for chmod() */
#include <glib.h>

#include <ZMap/zmap.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapSO.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapThreads.h>       // for thread lock functions
#include <zmapView_P.h>



/* some blixem defaults.... */
enum
  {
    BLIX_DEFAULT_SCOPE = 40000				    /* default 'width' of blixem sequence/features. */
  } ;


#define ZMAP_BLIXEM_CONFIG "blixem"

#define BLIX_NB_PAGE_GENERAL  "General"
#define BLIX_NB_PAGE_PFETCH   "Pfetch Socket Server"
#define BLIX_NB_PAGE_ADVANCED "Advanced"

/* ARGV positions for building argv to pass to g_spawn_async*/
enum
  {
    BLX_ARGV_PROGRAM,           /* blixem */

    /* We either have a config_file or this host/port */
    BLX_ARGV_NETID_PORT_FLAG,   /* --fetch-server */
    BLX_ARGV_NETID_PORT,        /* [hostname:port] */

//    BLX_ARGV_CONFIGFILE_FLAG = BLX_ARGV_NETID_PORT_FLAG,   /* -c */
//    BLX_ARGV_CONFIGFILE = BLX_ARGV_NETID_PORT,             /* [filepath] */

// MH17 from my limited perspective this is safer and easier to understand
#define BLX_ARGV_CONFIGFILE_FLAG BLX_ARGV_NETID_PORT_FLAG   /* -c */
#define BLX_ARGV_CONFIGFILE  BLX_ARGV_NETID_PORT             /* [filepath] */

    BLX_ARGV_RM_TMP_FILES,      /* --remove-input-files */
    BLX_ARGV_START_FLAG,        /* -s */
    BLX_ARGV_START,             /* [start] */
    BLX_ARGV_OFFSET_FLAG,       /* -m */
    BLX_ARGV_OFFSET,            /* [offset] */
    BLX_ARGV_ZOOM_FLAG,         /* -z */
    BLX_ARGV_ZOOM,              /* [start:end] */
    BLX_ARGV_SHOW_WHOLE,        /* --zoom-whole */
    BLX_ARGV_NEGATE_COORDS,     /* -n */
    BLX_ARGV_REVERSE_STRAND,    /* -r */
    BLX_ARGV_TYPE_FLAG,         /* -t */
    BLX_ARGV_TYPE,              /* 'n' or 'p' */
    BLX_ARGV_SEQBL_FLAG,        /* -x */
    BLX_ARGV_SEQBL,             /* [seqbl file] */
    BLX_ARGV_FASTA_FILE,        /* [fasta file] */
    BLX_ARGV_EXBLX_FILE,        /* [exblx file] */
    BLX_ARGV_DATASET,           /* --dataset=thing */
    BLX_ARGV_COVERAGE,		  /* --show-coverage */
    BLX_ARGV_SQUASH,		  /* --squash_matches */
    BLX_ARGV_ARGC               /* argc ;) */
  } ;


typedef enum {BLX_FILE_FORMAT_INVALID, BLX_FILE_FORMAT_EXBLX, BLX_FILE_FORMAT_GFF} BlixemFileFormat ;



/* Data needed by GFF formatting routines. */
typedef struct GFFFormatDataStructType
{
  gboolean use_SO_ids ;					    /* TRUE => use accession ids. */
  gboolean maximise_ids ;				    /* Give every feature an id. */

  int alignment_count ;
  int transcript_count ;
  int exon_count ;
  int feature_count ;
} GFFFormatDataStruct,  *GFFFormatData ;



/* Control block for preparing data to call blixem. */
typedef struct BlixemDataStruct
{
  /* user preferences for blixem */
  gboolean kill_on_exit ;				    /* TRUE => remove this blixem on
							       program exit (default). */

  gchar         *script;				    /* script to call blixem standalone */

  char          *config_file ;				    /* Contains blixem config. information. */

  gchar         *netid;					    /* eg pubseq */
  int            port;					    /* eg 22100  */

  /* Blixem can either initially zoom to position or show the whole picture. */
  gboolean show_whole_range ;

  /* Blixem can rebase the coords to show them with a different origin, offset is used to do this. */
  int offset ;

  int position ;					    /* Tells blixem what position to
							       centre on initially. */
  int window_start, window_end ;
  int mark_start, mark_end ;


  /* The ref. sequence and features are shown in blixem over the range  (position +/- (scope / 2)) */
  /* These restrict the range over which ref sequence and features are displayed if a mark was
   * set. NOTE that if scope is set to mark then so are features. */
  int scope ;						    /* defaults to 40000 */

  gboolean scope_from_mark ;				    /* Use mark start/end for scope ? */
  gboolean features_from_mark ;

  int scope_min, scope_max ;				    /* Bounds of displayed sequence. */
  int features_min, features_max ;			    /* Bounds of displayed features. */

  gboolean negate_coords ;				    /* Show rev strand coords as same as
							       forward strand but with a leading '-'. */

  int            homol_max;				    /* score cutoff point */

  char          *opts;

  void          *format_data ;				    /* Some formats need "callback" data. */
  BlixemFileFormat file_format ;
  char          *tmpDir ;
  gboolean      keep_tmpfiles ;

  char          *fastAFile ;
  GIOChannel    *fasta_channel;
  char          *errorMsg;

  char          *exblxFile ;
  GIOChannel    *exblx_channel;
  char          *exblx_errorMsg;
  char          *seqbl_file ;
  GIOChannel    *seqbl_channel ;
  char          *seqbl_errorMsg ;

  char          *gff_file ;
  GIOChannel    *gff_channel ;
  char          *gff_errorMsg ;

  ZMapView     view;

  GList *features ;

  ZMapFeatureSet feature_set ;
  GList *source;

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

  ZMapWindowAlignSetType align_set ;			    /* Which set of alignments ? */

  GList *dna_sets ;
  GList *protein_sets ;
  GList *transcript_sets ;

  GString *line ;

  GList *align_list ;

  ZMapFeatureSequenceMap sequence_map;    /* where the sequwence comes from, used for BAM scripts */

} blixemDataStruct, *blixemData ;


/* Holds just the config data, some of which is user configurable. */
typedef struct BlixemConfigDataStructType
{
  gboolean init ;					    /* TRUE when struct has been initialised. */

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
  } is_set ;


  /* User configurable */
  gboolean kill_on_exit ;				    /* TRUE => remove this blixem on
							       program exit (default). */
  gchar         *script ;				    /* script to call blixem standalone */
  gchar         *config_file ;				    /* file containing blixem config. */

  gchar         *netid ;				    /* eg pubseq */
  int           port ;					    /* eg 22100  */

  int           scope ;					    /* defines range over which aligns are
							       collected to send to blixem, defaults to 40000 */

  gboolean scope_from_mark ;
  gboolean features_from_mark ;

  int           homol_max ;				    /* defines max. number of aligns sent
							       to blixem. */

  BlixemFileFormat file_format ;
  gboolean      keep_tmpfiles ;

  /* Not user configurable */
  GList *dna_sets ;
  GList *protein_sets ;
  GList *transcript_sets ;

} BlixemConfigDataStruct, *BlixemConfigData ;



/* types for creating a table for outputting basic features. */
typedef gboolean (*BasicFeatureDumpFunc)(GFFFormatData gff_data, GString *line,
					 char *ref_name, char *source_name, ZMapFeature feature) ;

typedef struct BasicFeatureDumpStructName
{
  char *source_name ;
  BasicFeatureDumpFunc dump_func ;
} BasicFeatureDumpStruct, *BasicFeatureDump ;





static gboolean initBlixemData(ZMapView view, ZMapFeatureBlock block,
			       ZMapHomolType align_type,
			       int offset, int position,
			       int window_start, int window_end,
			       int mark_start, int mark_end,
			       GList *features, ZMapFeatureSet feature_set,
			       ZMapWindowAlignSetType align_set,
			       blixemData blixem_data, char **err_msg) ;
static gboolean setBlixemScope(blixemData blixem_data) ;
static gboolean buildParamString (blixemData blixem_data, char **paramString);
static void freeBlixemData(blixemData blixem_data);

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
static gboolean writeFeatureFiles(blixemData blixem_data);
static gboolean initFeatureFile(char *filename, char *file_header, GString *buffer,
				 GIOChannel **gio_channel_out, blixemData blixem_data) ;

static void writeFeatureLine(ZMapFeature feature, blixemData  blixem_data) ;

static gboolean printAlignment(ZMapFeature feature, blixemData  blixem_data) ;
static gboolean formatAlignmentExbl(GString *line,
				    int min_range, int max_range,
				    char *match_name,
				    int qstart, int qend, ZMapStrand q_strand, int qframe,
				    int sstart, int send, ZMapStrand s_strand, int sframe,
				    float score, GArray *gaps, char *sequence, char *description) ;
static gboolean formatAlignmentGFF(GFFFormatData gff_data, GString *line,
				   int min_range, int max_range,
				   char *ref_name, char *match_name, char *source_name, ZMapHomolType homol_type,
				   int qstart, int qend, ZMapStrand q_strand,
				   int sstart, int send, ZMapStrand s_strand,
				   float score, double percent_id,
				   GArray *gaps, char *sequence, char *description) ;

static gboolean printTranscript(ZMapFeature feature, blixemData  blixem_data) ;

static gboolean processExonsGFF(blixemData blixem_data, ZMapFeature feature, gboolean cds_only_unused) ;
static gboolean printExonGFF(GFFFormatData gff_data, GString *line, int min, int max,
				    char *ref_name, char *source_name,
				    ZMapFeature feature, char *transcript_name,
				    int exon_start, int exon_end,
				    int qstrand) ;

static gboolean processExonsExblx(blixemData blixem_data, ZMapFeature feature, gboolean cds_only) ;
static gboolean printExonExblx(GString *line, int min, int max,
				      char *transcript_name,
				      float score,
				      int exon_base, int exon_start, int exon_end,
				      int qstart, int qend, int qstrand,
				      int sstart, int send) ;
static gboolean printIntronsExblx(ZMapFeature feature, blixemData  blixem_data, gboolean cds_only) ;

static gboolean printBasic(ZMapFeature feature, blixemData  blixem_data) ;
static gboolean formatPolyA(GFFFormatData gff_data, GString *line,
			    char *ref_name, char *source_name, ZMapFeature feature) ;
static gboolean formatVariant(GFFFormatData gff_data, GString *line,
			      char *ref_name, char *source_name, ZMapFeature feature) ;

static gboolean printLine(GIOChannel *gio_channel, char **err_msg_out, char *line) ;

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
static int calcBlockLength(ZMapSequenceType match_seq_type, int start, int end) ;
static int calcGapLength(ZMapSequenceType match_seq_type, int start, int end) ;

GList * zMapViewGetColumnFeatureSets(blixemData data,GQuark column_id);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printFunc(gpointer key, gpointer value, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */







/*
 *                Globals
 */

/* Configuration global, holds current config for blixem, gets overloaded with new file
 * settings and new user selections. */
static BlixemConfigDataStruct blixem_config_curr_G = {FALSE} ;


static gboolean debug_G = TRUE ;





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
  gboolean status = TRUE ;
  blixemDataStruct blixem_data = {0} ;
  char *err_msg = "error in zmapViewCallBlixem()" ;


  status = initBlixemData(view, block, align_type,
			  0, position,
			  0, 0, 0, 0, NULL, feature_set, ZMAPWINDOW_ALIGNCMD_NONE, &blixem_data, &err_msg) ;


  blixem_data.errorMsg = NULL ;

  blixem_data.sequence_map = view->view_sequence;

  /* Do requested Homology data first. */
  blixem_data.required_feature_type = ZMAPSTYLE_MODE_ALIGNMENT ;

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

  if (feature_set)
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
	{
	  status = FALSE ;
	}
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
			    GList *features, ZMapFeatureSet feature_set,
			    char *source, GList *local_sequences,
			    GPid *child_pid, gboolean *kill_on_exit)
{
  gboolean status = TRUE ;
  char *argv[BLX_ARGV_ARGC + 1] = {NULL} ;
  GFFFormatDataStruct gff_data = {FALSE} ;
  blixemDataStruct blixem_data = {0} ;
  char *err_msg = "error in zmapViewCallBlixem()" ;

  if ((status = initBlixemData(view, block, homol_type,
			       offset, position,
			       window_start, window_end,
			       mark_start, mark_end,
			       features, feature_set, align_set, &blixem_data, &err_msg)))
    {
      if (blixem_data.file_format == BLX_FILE_FORMAT_GFF)
	blixem_data.format_data = &gff_data ;

      blixem_data.local_sequences = local_sequences ;
      blixem_data.source = source;

      blixem_data.sequence_map = view->view_sequence;
    }


  if (status)
    status = makeTmpfiles(&blixem_data) ;

  if (status)
    status = buildParamString(&blixem_data, &argv[0]);

  if (status)
    status = writeFeatureFiles(&blixem_data);

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
	  GString *args_str ;
	  char **my_argp = argv ;

	  args_str = g_string_new("Blix args: ") ;

	  while (*my_argp)
	    {
	      g_string_append_printf(args_str, "%s ", *my_argp) ;

	      my_argp++ ;
	    }

	  zMapLogMessage("%s", args_str->str) ;

	  printf("%s\n",  args_str->str) ;
	  fflush(stdout) ;

	  g_string_free(args_str, TRUE) ;
	}


      if (!(g_spawn_async(cwd, &argv[0], envp, flags, pre_exec, pre_exec_data, &spawned_pid, &error)))
        {
          status = FALSE;
          err_msg = error->message;
        }
      else
	{
	  zMapLogMessage("Blixem process spawned successfully. PID = '%d'", spawned_pid);
	}

      zMapThreadForkUnlock();

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



/* Returns a ZMapGuiNotebookChapter containing all user settable blixem resources. */
ZMapGuiNotebookChapter zMapViewBlixemGetConfigChapter(ZMapGuiNotebook note_book_parent)
{
  ZMapGuiNotebookChapter chapter = NULL ;

  /* If the current configuration has not been set yet then read stuff from the config file. */
  if (!blixem_config_curr_G.init)
    getUserPrefs(&blixem_config_curr_G) ;

  chapter = makeChapter(note_book_parent) ; // mh17: this uses blixen_config_curr_G

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
			       blixemData blixem_data, char **err_msg)
{
  gboolean status = TRUE ;

  blixem_data->view  = view ;

  blixem_data->offset = offset ;
  blixem_data->position = position ;
  blixem_data->window_start = window_start ;
  blixem_data->window_end = window_end ;
  blixem_data->mark_start = mark_start ;
  blixem_data->mark_end = mark_end ;

  blixem_data->scope = BLIX_DEFAULT_SCOPE ;

  blixem_data->negate_coords = TRUE ;			    /* default for havana. */

  blixem_data->align_type = align_type ;

  blixem_data->align_set = align_set;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  blixem_data->feature = feature ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  blixem_data->features = features ;

  blixem_data->feature_set = feature_set ;

  blixem_data->block = block ;

  /* ZMap uses the new blixem so default format is GFF by default. */
  blixem_data->file_format = BLX_FILE_FORMAT_GFF ;

  if (!(zMapFeatureBlockDNA(block, NULL, NULL, NULL)))
    {
      status = FALSE;
      *err_msg = "No DNA attached to feature's parent so cannot call blixem." ;
    }

  if (status)
    {
      if ((status = getUserPrefs(&blixem_config_curr_G)))
	setPrefs(&blixem_config_curr_G, blixem_data) ;
    }

  if (status)
    status = setBlixemScope(blixem_data) ;

  return status ;
}



static void freeBlixemData(blixemData blixem_data)
{
  g_free(blixem_data->tmpDir);
  g_free(blixem_data->fastAFile);
  g_free(blixem_data->exblxFile);
  g_free(blixem_data->seqbl_file);
  g_free(blixem_data->gff_file);

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


/* Get any user preferences specified in config file. */
static gboolean getUserPrefs(BlixemConfigData curr_prefs)
{
  gboolean status = FALSE ;
  ZMapConfigIniContext context = NULL ;
  BlixemConfigDataStruct file_prefs = {FALSE} ;

  if ((context = zMapConfigIniContextProvide()))
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

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				       ZMAPSTANZA_BLIXEM_FILE_FORMAT, &tmp_string))
	{
	  g_strstrip(tmp_string) ;

	  if (g_ascii_strcasecmp(tmp_string, "exblx") == 0)
	    file_prefs.file_format = BLX_FILE_FORMAT_EXBLX ;
	  else if (g_ascii_strcasecmp(tmp_string, "gffv3") == 0)
	    file_prefs.file_format = BLX_FILE_FORMAT_GFF ;

	  g_free(tmp_string) ;
	}


      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
					ZMAPSTANZA_BLIXEM_KEEP_TEMP, &tmp_bool))
	file_prefs.keep_tmpfiles = tmp_bool;

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

  if ((file_prefs.script) && ((file_prefs.config_file) || (file_prefs.netid && file_prefs.port)))
    {
      char *tmp;
      tmp = file_prefs.script;

      if (!(file_prefs.script = g_find_program_in_path(tmp)))
  	    zMapShowMsg(ZMAP_MSG_WARNING,
		    "Either can't locate \"%s\" in your path or it is not executable by you.",
		    tmp) ;
      g_free(tmp) ;

      if (file_prefs.config_file && !zMapFileAccess(file_prefs.config_file, "rw"))
	    zMapShowMsg(ZMAP_MSG_WARNING,
		    "Either can't locate \"%s\" in your path or it is not read/writeable.",
		    file_prefs.config_file) ;
    }
  else
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Some or all of the compulsory blixem parameters "
		  "(\"netid\", \"port\") or config_file or \"script\" are missing from your config file.");
    }

  if (file_prefs.script && file_prefs.config_file)
     status = TRUE;

  file_prefs.init = TRUE;


  /* If curr_prefs is initialised then copy any curr prefs set by user into file prefs,
   * thus overriding file prefs with user prefs. Then copy file prefs into curr_prefs
   * so that curr prefs now represents latest config file and user prefs combined. */
  if (curr_prefs->init)
    {
      file_prefs.is_set = curr_prefs->is_set ;		    /* struct copy. */

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

      if (curr_prefs->is_set.kill_on_exit)
	file_prefs.kill_on_exit = curr_prefs->kill_on_exit ;
    }

  *curr_prefs = file_prefs ;				    /* Struct copy. */


  return status ;
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


  blixem_data->scope_from_mark = curr_prefs->scope_from_mark ;
  blixem_data->features_from_mark = curr_prefs->features_from_mark ;

  if (curr_prefs->homol_max > 0)
    blixem_data->homol_max = curr_prefs->homol_max ;

  blixem_data->keep_tmpfiles = curr_prefs->keep_tmpfiles ;

  blixem_data->kill_on_exit = curr_prefs->kill_on_exit ;

  if (curr_prefs->file_format)
    blixem_data->file_format = curr_prefs->file_format ;

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
static gboolean setBlixemScope(blixemData blixem_data)
{
  gboolean status = TRUE ;
  static gboolean scope_debug = FALSE ;


  /* We shouldn't need this here...window should take care of it..... */
  if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_SEQ && !(blixem_data->mark_start && blixem_data->mark_end))
    {
      zMapLogWarning("%s", "Request for short ZMAPWINDOW_ALIGNCMD_SEQ but no mark is set.") ;

      status = FALSE ;
    }

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
	  blixem_data->scope_min = blixem_data->mark_start ;
	  blixem_data->scope_max = blixem_data->mark_end ;
	}
      else
	{
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
      if (blixem_data->align_type == ZMAPHOMOL_X_HOMOL)	    /* protein */
	{
	  ZMapFeature feature ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* AGH....WHAT TO DO ABOUT THIS.... */

	  /* tmp...sort out later.... */
	  feature = (ZMapFeature)(blixem_data->features->data) ;

	  if (feature->strand == ZMAPSTRAND_REVERSE)
	    blixem_data->opts = "X-BR";
	  else
	    blixem_data->opts = "X+BR";
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  /* HACKED FOR NOW..... */
	  if (blixem_data->features)
	    feature = (ZMapFeature)(blixem_data->features->data) ;
	  else
	    feature = zMap_g_hash_table_nth(blixem_data->feature_set->features, 0) ;

	  if (feature->strand == ZMAPSTRAND_REVERSE)
	    blixem_data->opts = "X-BR";
	  else
	    blixem_data->opts = "X+BR";
	}
      else
	{
	  blixem_data->opts = "N+BR";			    /* dna */
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

  /* Create file(s) to hold features. */
  if (status)
    {
      if (blixem_data->file_format == BLX_FILE_FORMAT_EXBLX)
	{
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
	}
      else
	{
	  {
	    if ((status = makeTmpfile(blixem_data->tmpDir, "gff", &(blixem_data->gff_file))))
	      status = setTmpPerms(blixem_data->gff_file, FALSE) ;
	  }
	}
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
  gboolean status = TRUE ;
  int missed = 0;					    /* keep track of options we don't specify */

/* MH17 NOTE
   this code must operate in the same order as the enums at the top of the file
   better to recode without the 'missed flag' as this code is VERY error prone
 */


  /* we need to do this as blixem has pretty simple argv processing */

  if (blixem_data->config_file)
    {
      paramString[BLX_ARGV_CONFIGFILE_FLAG] = g_strdup("-c");
      paramString[BLX_ARGV_CONFIGFILE]      = g_strdup_printf("%s", blixem_data->config_file) ;
    }
  else if (blixem_data->netid && blixem_data->port)
    {
      paramString[BLX_ARGV_NETID_PORT_FLAG] = g_strdup("--fetch-server");
      paramString[BLX_ARGV_NETID_PORT]      = g_strdup_printf("%s:%d", blixem_data->netid, blixem_data->port);
    }
  else
    {
      missed += 2;
    }

  /* For testing purposes remove the "-r" flag to leave the temporary files.
   * (keep_tempfiles = true in blixem stanza of ZMap file) */
  if (!blixem_data->keep_tmpfiles)
    paramString[BLX_ARGV_RM_TMP_FILES - missed] = g_strdup("--remove-input-files");
  else
    missed += 1;

  /* Start with blixem centred here. */
  if (blixem_data->position)
    {
      int start, end ;

      start = end = blixem_data->position ;

      if (blixem_data->view->revcomped_features)
	zMapFeatureReverseComplementCoords(blixem_data->block, &start, &end) ;

      paramString[BLX_ARGV_START_FLAG - missed] = g_strdup("-s") ;
      paramString[BLX_ARGV_START - missed]      = g_strdup_printf("%d", start) ;
    }
  else
    {
      missed += 2;
    }

  /* Rebase coords in blixem by offset. */
  if (blixem_data->position)
    {
      int offset, tmp1 ;


      offset = tmp1 = blixem_data->offset ;

      paramString[BLX_ARGV_OFFSET_FLAG - missed] = g_strdup("-m") ;
#if MH17_WONT_WORK_POST_CHROMO_COORDS
      /* NOTE this function swaps start and end and inverts them, you have to provide both */
      if (blixem_data->view->revcomped_features)
		zMapFeatureReverseComplementCoords(blixem_data->block, &offset, &tmp1) ;
#endif


      paramString[BLX_ARGV_OFFSET - missed]      = g_strdup_printf("%d", offset) ;
    }
  else
    {
      missed += 2;
    }


  /* Set up initial view start/end.... */
    {
      int start, end ;

      start = blixem_data->window_start ;
      end = blixem_data->window_end ;

      if (blixem_data->view->revcomped_features)
	zMapFeatureReverseComplementCoords(blixem_data->block, &start, &end) ;

      paramString[BLX_ARGV_ZOOM_FLAG - missed] = g_strdup("-z") ;
      paramString[BLX_ARGV_ZOOM - missed] = g_strdup_printf("%d:%d", start, end) ;
    }


  /* Show whole blixem range ? */
  if (blixem_data->show_whole_range)
    paramString[BLX_ARGV_SHOW_WHOLE - missed] = g_strdup("--zoom-whole") ;
  else
    missed += 1 ;

  /* acedb users always like reverse strand to have same coords as forward strand but
   * with a '-' in front of the coord. */
  if (blixem_data->negate_coords)
    paramString[BLX_ARGV_NEGATE_COORDS - missed] = g_strdup("-n") ;
  else
    missed += 1 ;

  /* Start with reverse strand showing. */
  if (blixem_data->view->revcomped_features)
    paramString[BLX_ARGV_REVERSE_STRAND - missed] = g_strdup("-r") ;
  else
    missed += 1 ;


  /* type of alignment data, i.e. nucleotide or peptide. Compulsory. Note that type
   * can be NONE if blixem called for non-alignment column, something requested by
   * annotators for just looking at transcripts/dna. */
  paramString[BLX_ARGV_TYPE_FLAG - missed] = g_strdup("-t");
  if (blixem_data->align_type == ZMAPHOMOL_NONE || blixem_data->align_type == ZMAPHOMOL_N_HOMOL)
    paramString[BLX_ARGV_TYPE - missed] = g_strdup_printf("%c", 'N') ;
  else
    paramString[BLX_ARGV_TYPE - missed] = g_strdup_printf("%c", 'P') ;


  if (blixem_data->file_format == BLX_FILE_FORMAT_EXBLX
      && blixem_data->seqbl_file)
    {
      paramString[BLX_ARGV_SEQBL_FLAG - missed] = g_strdup("-x");
      paramString[BLX_ARGV_SEQBL - missed] = g_strdup_printf("%s", blixem_data->seqbl_file);
    }
  else
    {
      missed += 2;
    }

  paramString[BLX_ARGV_FASTA_FILE - missed] = g_strdup_printf("%s", blixem_data->fastAFile);

  if (blixem_data->file_format == BLX_FILE_FORMAT_EXBLX)
    {
      paramString[BLX_ARGV_EXBLX_FILE - missed] = g_strdup_printf("%s", blixem_data->exblxFile);
    }
  else
    {
      paramString[BLX_ARGV_EXBLX_FILE - missed] = g_strdup_printf("%s", blixem_data->gff_file) ;
    }

  if(blixem_data->sequence_map->dataset)
  {
      paramString[BLX_ARGV_DATASET - missed] = g_strdup_printf("--dataset=%s",blixem_data->sequence_map->dataset);
  }
  else
  {
      missed += 1;

  }

  if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_SEQ)
  {
  	paramString[BLX_ARGV_COVERAGE - missed] = g_strdup("--show-coverage");
  	paramString[BLX_ARGV_SQUASH - missed] = g_strdup("--squash-matches");
  }
  else
  {
  	missed += 2;
  }


  return status ;
}


static gboolean writeFeatureFiles(blixemData blixem_data)
{
  gboolean status = TRUE ;
  GError  *channel_error = NULL ;
  //  gboolean seqbl_file = FALSE ;

  char *header ;
  int start, end ;

  /* We just use the one gstring as a reusable buffer to format all output. */
  blixem_data->line = g_string_new("") ;


  /* sort out start/end if view is revcomp'd. */
  start = blixem_data->scope_min ;
  end = blixem_data->scope_max ;


  /*
   * Write the file headers.
   */
#if 0
  if (blixem_data->file_format == BLX_FILE_FORMAT_EXBLX)
    {
      /* Open the exblx file, always needed. */
      header = g_strdup_printf("# exblx_x\n# blast%c\n# sequence-region %s %d %d\n",
			       *blixem_data->opts,
			       g_quark_to_string(blixem_data->block->original_id),
			       start, end) ;

      status = initFeatureFile(blixem_data->exblxFile, header, blixem_data->line,
			       &(blixem_data->exblx_channel), blixem_data) ;

      g_free(header) ;

      /* Open the seqbl file, if needed. */
      if (status)
	{
	  if (blixem_data->local_sequences)
	    seqbl_file = TRUE ;

	  if (seqbl_file)
	    {
	      header = g_strdup_printf("# seqbl_x\n# blast%c\n# sequence-region %s %d %d\n",
				       *blixem_data->opts,
				       g_quark_to_string(blixem_data->block->original_id),
				       start, end) ;

	      status = initFeatureFile(blixem_data->seqbl_file, header, blixem_data->line,
				       &(blixem_data->seqbl_channel), blixem_data) ;

	      g_free(header) ;
	    }
	}
    }
  else
#endif

#warning legacy blixem file format code iffed out!
    {
      start = blixem_data->features_min ;
      end = blixem_data->features_max ;

      if (blixem_data->view->revcomped_features)
	zMapFeatureReverseComplementCoords(blixem_data->block, &start, &end) ;


      header = g_strdup_printf("##gff-version 3\n##sequence-region %s %d %d\n",
			       g_quark_to_string(blixem_data->block->original_id),
			       start, end) ;
      printf("Blixem file: %s",header);


      status = initFeatureFile(blixem_data->gff_file, header, blixem_data->line,
			       &(blixem_data->gff_channel), blixem_data) ;

      g_free(header) ;
    }


  /*
   * Write the features.
   */
  if (status)
    {
      ZMapFeatureSet feature_set ;
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

      if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_FEATURES)
	{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  blixem_data->align_list = zMapFeatureSetGetNamedFeatures(feature_set, feature->original_id) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  /* mmmm tricky.....should all features be highlighted...mmmmmm */
	  if (g_list_length(blixem_data->features) > 1)
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

      if (blixem_data->align_set == ZMAPWINDOW_ALIGNCMD_SEQ)
	/* NOTE the request data is a column which contains multiple featuresets
	 * we have to include a line for each one. Theese are given in the course GList
	 */
	{
	  // chr4-04     source1     region      215000      300000      0.000000    .     .     dataType=short-read

	  /* MH17: not sure whre eblx and seqbl come in
	   * the feature writing code just goes straight to gff
	   * via code like this:
	   */

	  GList *l;

	  for(l = blixem_data->source; l ; l = l->next)
	  {
		char *ref_name = (char *)g_quark_to_string(blixem_data->block->original_id);

		g_string_append_printf(blixem_data->line, "%s\t%s\t%s\t%d\t%d\t%f\t.\t.\t%s\n",
					ref_name, g_quark_to_string(GPOINTER_TO_UINT(l->data)),
					"region",          // zMapSOAcc2Term(feature->SO_accession),
					//                   blixem_data->mark_start, blixem_data->mark_end,
					start,end,		/* these have been revcomped and wwere set from the mark */
					0.0, "dataType=short-read" ) ;       // is this also SO??

		printLine(blixem_data->gff_channel, &(blixem_data->errorMsg), blixem_data->line->str) ;
		printf("Blixem file: %s",blixem_data->line->str);
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


      /*
       * Now do transcripts (may need to filter further...)
       */
      if (blixem_data->transcript_sets)
	{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* debug. */
	  g_hash_table_foreach(blixem_data->block->feature_sets, printFunc, NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

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


  if (blixem_data->gff_channel)
    {
      g_io_channel_shutdown(blixem_data->gff_channel, TRUE, &channel_error);
      if (channel_error)
	g_free(channel_error);
    }


  /* Free our string buffer. */
  g_string_free(blixem_data->line, TRUE) ;
  blixem_data->line = NULL ;

  return status ;
}



/* HACK...try to get rid of blixem_data param...needs curr_channel to go... */
/* Open and initially format a feature file. */
static gboolean initFeatureFile(char *filename, char *file_header, GString *buffer,
				GIOChannel **gio_channel_out, blixemData blixem_data)
{
  gboolean status = TRUE ;
  GError  *channel_error = NULL ;

  /* Open the exblx file, always needed. */
  if ((*gio_channel_out = g_io_channel_new_file(filename, "w", &channel_error)))
    {
      g_string_printf(buffer, "%s", file_header) ;

      status = printLine(*gio_channel_out, &(blixem_data->errorMsg), buffer->str) ;

      buffer = g_string_truncate(buffer, 0) ;
    }
  else
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not open file: %s", channel_error->message) ;
      g_error_free(channel_error) ;
      status = FALSE ;
    }

  return status ;
}



/* A GFunc() to step through the named feature sets and write them out for passing
 * to blixem. */
static void processSetList(gpointer data, gpointer user_data)
{
  GQuark set_id = GPOINTER_TO_UINT(data) ;
  GQuark canon_id ;
  blixemData blixem_data = (blixemData)user_data ;
  ZMapFeatureSet feature_set ;
  GList *column_2_featureset;

  canon_id = zMapFeatureSetCreateID((char *)g_quark_to_string(set_id)) ;

  feature_set = g_hash_table_lookup(blixem_data->block->feature_sets, GINT_TO_POINTER(canon_id));

  if (feature_set)
    {
      g_hash_table_foreach(feature_set->features, writeHashEntry, blixem_data);
    }
  else
    {
      /* assuming a mis-config treat the set id as a column id */
      column_2_featureset = zMapFeatureGetColumnFeatureSets(&blixem_data->view->context_map,canon_id,TRUE);

      if (!column_2_featureset)
	{
	  zMapLogWarning("Could not find %s feature set or column \"%s\" in context feature sets.",
			 (blixem_data->required_feature_type == ZMAPSTYLE_MODE_ALIGNMENT
			  ? "alignment" : "transcript"),
			 g_quark_to_string(set_id)) ;
	}


      for ( ; column_2_featureset ; column_2_featureset = column_2_featureset->next)
	{
	  if ((feature_set = g_hash_table_lookup(blixem_data->block->feature_sets, column_2_featureset->data)))
            {
	      g_hash_table_foreach(feature_set->features, writeHashEntry, blixem_data);
            }
#if MH17_GIVES_SPURIOUS_ERRORS
	  /*
	    We get a featureset to column mapping that includes all possible
	    but without features for each one the set does not get created
	    in which case here a not found error is not an error
	    but not finding the column or featureset in the first attempt is
	    previous code would not have found *_trunc featuresets!
	  */
	  else
            {
	      zMapLogWarning("Could not find %s feature set \"%s\" in context feature sets.",
			     (blixem_data->required_feature_type == ZMAPSTYLE_MODE_ALIGNMENT
			      ? "alignment" : "transcript"),
			     g_quark_to_string(GPOINTER_TO_UINT(column_2_featureset->data))) ;

            }
#endif
	}
    }

  return ;
}


/* These two functions enable writeExblxSeqblLine() to be called from a g_hash or a g_list
 * foreach function. */
static void writeHashEntry(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  blixemData  blixem_data = (blixemData)user_data ;

  writeFeatureLine(feature, blixem_data) ;

  return ;
}

static void writeListEntry(gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  blixemData  blixem_data = (blixemData)user_data ;

  writeFeatureLine(feature, blixem_data) ;

  return ;
}


static void writeFeatureLine(ZMapFeature feature, blixemData  blixem_data)
{

  /* There is no way to interrupt g_hash_table_foreach() so instead if errorMsg is set
   * then it means there was an error in processing so we don't process any
   * more records, displaying the error when we return. */
  if (!(blixem_data->errorMsg))
    {
      gboolean status = TRUE ;
      gboolean include_feature, fully_contained = FALSE ;   /* TRUE => feature must be fully
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
	  switch (feature->type)
	    {
	    case ZMAPSTYLE_MODE_ALIGNMENT:
	      {
		if (feature->feature.homol.type == blixem_data->align_type)
		  {
		    if ((*blixem_data->opts == 'X' && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
			|| (*blixem_data->opts == 'N' && feature->feature.homol.type == ZMAPHOMOL_N_HOMOL))
		      status = printAlignment(feature, blixem_data) ;
		  }

		break ;
	      }
	    case ZMAPSTYLE_MODE_TRANSCRIPT:
	      {
		status = printTranscript(feature, blixem_data) ;

		break ;
	      }
	    case ZMAPSTYLE_MODE_BASIC:
	      {
		if (blixem_data->file_format == BLX_FILE_FORMAT_GFF)
		  {
		    status = printBasic(feature, blixem_data) ;
		  }

		break ;
	      }
	    default:
	      {
		break ;
	      }
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
  int qstart, qend, sstart, send ;
  int qframe = 0, sframe = 0 ;
  GString *line ;
  int x1, x2 ;


  min = blixem_data->features_min ;
  max = blixem_data->features_max ;
  line = blixem_data->line ;


  /* If view is revcomp'd then recomp back so we always pass forward coords. */
  if (blixem_data->view->revcomped_features)
    zMapFeatureReverseComplement(blixem_data->view->features, feature) ;



  x1 = feature->x1 ;
  x2 = feature->x2 ;

  /* qstart and qend are the coordinates of the current homology relative to the viewing view,
   * rather than absolute coordinates. */
  if (feature->strand == ZMAPSTRAND_REVERSE)
    {
      qstart = x2 ;
      qend   = x1 ;
    }
  else
    {
      qstart = x1 ;
      qend   = x2 ;
    }

  /* qframe is derived from the position of the start position
   * of this homology relative to one end of the viewing view
   * or the other, depending on strand.  Mod3 gives us the odd
   * bases if that distance is not a whole number of codons. */
  if (feature->strand == ZMAPSTRAND_REVERSE)
    {
      qframe = 1 + ((max - qstart) % 3) ;
    }
  else
    {
      qframe = 1 + ((qstart - 1) % 3) ;
    }


  sstart = feature->feature.homol.y1 ;
  send   = feature->feature.homol.y2 ;

  if (feature->feature.homol.strand == ZMAPSTRAND_REVERSE)
    {
      if (feature->feature.homol.length)
	sframe = (1 + ((feature->feature.homol.length) - sstart) % 3) ;
    }
  else
    {
      if (feature->feature.homol.length)
	sframe = (1 + (sstart-1) % 3) ;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
   if (score)
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    {
      char *seq_str = NULL ;
      char *description = NULL ;			    /* Unsupported currently. */
      GList *list_ptr ;
      char *match_name ;
      char *source_name ;
      GIOChannel *curr_channel ;

      match_name = (char *)g_quark_to_string(feature->original_id) ;
      source_name = (char *)g_quark_to_string(feature->source_id) ;


      /* Phase out stupid curr_channel                    */

      /* need to put this choice for seqbl into the exblx routine below... */
      if (blixem_data->file_format == BLX_FILE_FORMAT_EXBLX)
	{
	  if ((list_ptr = g_list_find_custom(blixem_data->local_sequences, feature, findFeature)))
	    {
	      ZMapSequence sequence = (ZMapSequence)list_ptr->data ;

	      seq_str = sequence->sequence ;
	      curr_channel = blixem_data->seqbl_channel ;
	    }
	  else
	    {
	      /* In theory we should be checking for a description for non-local sequences,
	       * see acedb code in doShowAlign...don't know how important this is..... */
	      seq_str = "" ;
	      curr_channel = blixem_data->exblx_channel ;
	    }

	  status = formatAlignmentExbl(line,
				       min, max,
				       match_name,
				       qstart, qend, feature->strand, qframe,
				       sstart, send, feature->feature.homol.strand, sframe,
				       feature->score,
				       feature->feature.homol.align, seq_str, description) ;
	}
      else
	{
	  int tmp ;

	  if (feature->strand == ZMAPSTRAND_REVERSE)
	    tmp = qstart, qstart = qend, qend = tmp ;

	  curr_channel = blixem_data->gff_channel ;

	  status = formatAlignmentGFF(blixem_data->format_data, line,
				      min, max,
				      (char *)g_quark_to_string(blixem_data->block->original_id),
				      match_name, source_name, feature->feature.homol.type,
				      qstart, qend, feature->strand,
				      sstart, send, feature->feature.homol.strand,
				      feature->score, feature->feature.homol.percent_id,
				      feature->feature.homol.align, seq_str, description) ;
	}

      status = printLine(curr_channel, &(blixem_data->errorMsg), line->str) ;
    }


   /* If view is recomp'd we should swop back again now. */
  if (blixem_data->view->revcomped_features)
    zMapFeatureReverseComplement(blixem_data->view->features, feature) ;


  return status ;
}

static gboolean formatAlignmentExbl(GString *line,
				    int min, int max,
				    char *match_name,
				    int qstart, int qend, ZMapStrand q_strand, int qframe,
				    int sstart, int send, ZMapStrand s_strand, int sframe,
				    float score, GArray *gaps, char *sequence, char *description)
{
  gboolean status = TRUE ;
  char qframe_strand, sframe_strand ;
  int score_int ;

  if (q_strand == ZMAPSTRAND_REVERSE)
    qframe_strand = '-' ;
  else
    qframe_strand = '+' ;

  if (s_strand == ZMAPSTRAND_REVERSE)
    sframe_strand = '-' ;
  else
    sframe_strand = '+' ;

  score_int = (int)(score + 0.5) ;

  g_string_printf(line, "%d\t(%c%d)\t%d\t%d\t(%c%d)\t%d\t%d\t%s",
		  score_int,
		  qframe_strand, qframe,
		  qstart, qend,
		  sframe_strand, sframe,
		  sstart, send,
		  match_name) ;

  if (gaps)
    {
      int k, index, incr ;

      g_string_append_printf(line, "%s", "\tGaps ") ;

      if (q_strand == ZMAPSTRAND_REVERSE)
	{
	  index = gaps->len - 1 ;
	  incr = -1 ;
	}
      else
	{
	  index = 0 ;
	  incr = 1 ;
	}

      for (k = 0 ; k < gaps->len ; k++, index += incr)
	{
	  ZMapAlignBlockStruct gap ;

	  gap = g_array_index(gaps, ZMapAlignBlockStruct, index) ;

	  if (qstart > qend)
	    zMapUtilsSwop(int, gap.t1, gap.t2) ;

	  g_string_append_printf(line, " %d %d %d %d", gap.q1, gap.q2, gap.t1, gap.t2) ;
	}

      g_string_append_printf(line, "%s", " ;") ;
    }

  /* In theory we should be outputting description for some files.... */
  if ((sequence && *sequence) || (description && *description))
    {
      char *tag, *text ;

      if (sequence)
	{
	  tag = "Sequence" ;
	  text = sequence ;
	}
      else
	{
	  tag = "Description" ;
	  text = description ;
	}

      g_string_append_printf(line, "\t%s %s ;", tag, text) ;
    }

  g_string_append_c(line, '\n') ;

  return status ;
}


static gboolean formatAlignmentGFF(GFFFormatData gff_data, GString *line,
				   int min, int max,
				   char *ref_name, char *match_name, char *source_name, ZMapHomolType homol_type,
				   int qstart, int qend, ZMapStrand q_strand,
				   int sstart, int send, ZMapStrand s_strand,
				   float score, double percent_id,
				   GArray *gaps, char *sequence, char *description)
{
  gboolean status = TRUE ;
  char *id_str = NULL ;
  ZMapSequenceType match_seq_type ;


  if (homol_type == ZMAPHOMOL_N_HOMOL)
    match_seq_type = ZMAPSEQUENCE_DNA ;
  else
    match_seq_type = ZMAPSEQUENCE_PEPTIDE ;


  if (gff_data->maximise_ids)
    {
      gff_data->alignment_count++ ;

      id_str = g_strdup_printf("ID=match%d;", gff_data->alignment_count) ;
    }


  /* ctg123 . cDNA_match 1050  1500  5.8e-42  +  . ID=match00001;Target=cdna0123 12 462 */
  g_string_printf(line, "%s\t%s\t%s\t%d\t%d\t%f\t%c\t.\t%sTarget=%s %d %d %c",
		  ref_name, source_name,
		  (match_seq_type == ZMAPSEQUENCE_DNA ? "nucleotide_match" : "protein_match"),
		  qstart, qend,
		  score,
		  (q_strand == ZMAPSTRAND_REVERSE ? '-' : '+'),
		  (id_str ? id_str : ""),
		  match_name,
		  sstart, send,
		  (s_strand == ZMAPSTRAND_REVERSE ? '-' : '+')) ;

  if (percent_id)
    {
      g_string_append_printf(line, ";percentID=%g", percent_id) ;
    }

  if (gaps)
    {
      int k, index, incr ;
      GString *align_str ;

      align_str = g_string_sized_new(2000) ;

      /* Gap=M8 D3 M6 I1 M6 */

      /* correct, needed ?? */
      if (q_strand == ZMAPSTRAND_REVERSE)
	{
	  index = gaps->len - 1 ;
	  incr = -1 ;
	}
      else
	{
	  index = 0 ;
	  incr = 1 ;
	}

      for (k = 0 ; k < gaps->len ; k++, index += incr)
	{
	  ZMapAlignBlock gap ;
	  int coord ;

	  gap = &(g_array_index(gaps, ZMapAlignBlockStruct, index)) ;

	  coord = calcBlockLength(match_seq_type, gap->t1, gap->t2) ;
	  g_string_append_printf(align_str, "M%d ", coord) ;

	  if (k < gaps->len - 1)
	    {
	      ZMapAlignBlock next_gap ;
	      int curr_match, next_match, curr_ref, next_ref ;

	      next_gap = &(g_array_index(gaps, ZMapAlignBlockStruct, index + incr)) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      if (gap->q_strand == ZMAPSTRAND_FORWARD && gap->t_strand == ZMAPSTRAND_FORWARD)
		printf("forwards - forwards\n") ;
	      else if (gap->q_strand == ZMAPSTRAND_FORWARD && gap->t_strand == ZMAPSTRAND_REVERSE)
		printf("forwards - backwards\n") ;
	      else if (gap->q_strand == ZMAPSTRAND_REVERSE && gap->t_strand == ZMAPSTRAND_FORWARD)
		printf("backwards - forwards\n") ;
	      else if (gap->q_strand == ZMAPSTRAND_REVERSE && gap->t_strand == ZMAPSTRAND_REVERSE)
		printf("backwards - backwards\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      if (gap->q_strand == ZMAPSTRAND_FORWARD)
		{
		  curr_match = gap->q2 ;
		  next_match = next_gap->q1 ;
		}
	      else
		{
		  curr_match = next_gap->q2 ;
		  next_match = gap->q1 ;
		}

	      if (gap->t_strand == ZMAPSTRAND_FORWARD)
		{
		  curr_ref = gap->t2 ;
		  next_ref = next_gap->t1 ;
		}
	      else
		{
		  curr_ref = next_gap->t2 ;
		  next_ref = gap->t1 ;
		}

	      if (next_match > curr_match + 1)
		{
		  g_string_append_printf(align_str, "I%d ", (next_match - curr_match) - 1) ;
		}
	      else if (next_ref > curr_ref + 1)
		{
		  coord = calcGapLength(match_seq_type, curr_ref, next_ref) ;
		  g_string_append_printf(align_str, "D%d ", coord) ;
		}
	      else
		zMapLogWarning("Bad coords in GFFv3 writer: gap %d,%d -> %d, %d, next_gap %d, %d -> %d, %d",
			       gap->t1, gap->t2, gap->q1, gap->q2,
			       next_gap->t1, next_gap->t2, next_gap->q1, next_gap->q2) ;
	    }
	}

      g_string_append_printf(line, ";Gap=%s", align_str->str) ;

      g_string_free(align_str, TRUE) ;
    }


  if (sequence)
    {
      g_string_append_printf(line, ";Sequence=%s", sequence) ;
    }

  g_string_append_c(line, '\n') ;

  if (id_str)
    g_free(id_str) ;

  return status ;
}


/* Print a transcript. */
static gboolean printTranscript(ZMapFeature feature, blixemData  blixem_data)
{
  gboolean status = TRUE;
  gboolean cds_only = TRUE ;

  /* Swop to other strand..... */
  if (blixem_data->view->revcomped_features)
    zMapFeatureReverseComplement(blixem_data->view->features, feature) ;


  if (blixem_data->file_format == BLX_FILE_FORMAT_GFF)
    {
      status = processExonsGFF(blixem_data, feature, cds_only) ;
    }
  else
    {
      if (blixem_data->align_type == ZMAPHOMOL_N_HOMOL)
	cds_only = FALSE ;

      if (!cds_only || feature->feature.transcript.flags.cds)
	{
	  /* Do the exons... */
	  status = processExonsExblx(blixem_data, feature, cds_only) ;

	  /* Now do extra's. */
	  printIntronsExblx(feature, blixem_data, cds_only) ;
	}
    }


   /* And swop it back again. */
  if (blixem_data->view->revcomped_features)
    zMapFeatureReverseComplement(blixem_data->view->features, feature) ;

  return status ;
}



/* Print out the exons taking account of the extent of the CDS within the transcript. */
static gboolean processExonsGFF(blixemData blixem_data, ZMapFeature feature, gboolean cds_only_unused)
{
  gboolean status = TRUE ;
  GIOChannel *channel ;
  int i ;
  ZMapSpan span = NULL ;
  int min, max ;
  GString *line ;
  char *ref_name ;
  char *transcript_name ;
  char *source_name ;
  GFFFormatData gff_data = (GFFFormatData)(blixem_data->format_data) ;
  char *SO_rna_id = "mRNA" ;


  channel = blixem_data->gff_channel ;

  line = blixem_data->line ;

  min = blixem_data->features_min ;
  max = blixem_data->features_max ;

  ref_name = (char *)g_quark_to_string(blixem_data->block->original_id) ;
  transcript_name = (char *)g_quark_to_string(feature->original_id) ;
  source_name = (char *)g_quark_to_string(feature->source_id) ;

  /* Write out the transcript record:
   *            ctg123 . mRNA            1050  9000  .  +  .  ID=mRNA00001;Name=EDEN.1 */
  gff_data->transcript_count++ ;

  g_string_printf(line, "%s\t%s\t%s\t%d\t%d\t.\t%c\t.\tID=transcript%d;Name=%s\n",
		  ref_name, source_name, SO_rna_id,
		  feature->x1, feature->x2,
		  (feature->strand == ZMAPSTRAND_REVERSE ? '-' : '+'),
		  gff_data->transcript_count,
		  transcript_name) ;

  status = printLine(channel, &(blixem_data->errorMsg), line->str) ;

  blixem_data->line = g_string_truncate(blixem_data->line, 0) ; /* Reset string buffer. */


  /* Write out the exons...and if there is one the CDS sections. */
  for (i = 0 ; i < feature->feature.transcript.exons->len ; i++)
    {
      int exon_start, exon_end ;

      span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i) ;

      exon_start = span->x1 ;
      exon_end = span->x2 ;

      printExonGFF(blixem_data->format_data, line, min, max,
			  ref_name, source_name,
			  feature, transcript_name,
			  exon_start, exon_end,
			  feature->strand) ;

      status = printLine(channel, &(blixem_data->errorMsg), line->str) ;

      blixem_data->line = g_string_truncate(blixem_data->line, 0) ; /* Reset string buffer. */
    }

  return status ;
}


static gboolean printExonGFF(GFFFormatData gff_data, GString *line, int min, int max,
			     char *ref_name, char *source_name,
			     ZMapFeature feature, char *transcript_name,
			     int exon_start, int exon_end,
			     int qstrand)
{
  gboolean status = TRUE ;
  char *SO_exon_id = "exon" ;
  char *SO_CDS_id = "CDS" ;
  char *id_str = NULL ;

  if (gff_data->maximise_ids)
    {
      gff_data->exon_count++ ;

      id_str = g_strdup_printf("ID=exon%d;", gff_data->exon_count) ;
    }

  /* ctg123 . exon            1300  1500  .  +  .  Parent=mRNA00003 */
  g_string_append_printf(line, "%s\t%s\t%s\t%d\t%d\t.\t%c\t.\t%sParent=transcript%d\n",
			 ref_name, source_name, SO_exon_id,
			 exon_start, exon_end,
			 (qstrand == ZMAPSTRAND_REVERSE ? '-' : '+'),
			 (id_str ? id_str : ""),
			 gff_data->transcript_count) ;


  if (ZMAPFEATURE_HAS_CDS(feature))
    {
      int cds_start, cds_end ;

      cds_start = feature->feature.transcript.cds_start ;
      cds_end = feature->feature.transcript.cds_end ;

      if (exon_start > cds_end || exon_end < cds_start)
	{
	  ;
	}
      else
	{
	  int tmp_cds1, tmp_cds2, phase ;

	  /* ctg123 . CDS 1201 1500 . + 0 Parent=mRNA00001 */
	  if (zMapFeatureExon2CDS(feature,
				  exon_start, exon_end, &tmp_cds1, &tmp_cds2, &phase))
	    {
	      /* Only print if exon has cds section. */
	      g_string_append_printf(line, "%s\t%s\t%s\t%d\t%d\t.\t%c\t%d\t%sParent=transcript%d\n",
				     ref_name, source_name, SO_CDS_id,
				     tmp_cds1, tmp_cds2,
				     (qstrand == ZMAPSTRAND_REVERSE ? '-' : '+'),
				     phase,
				     (id_str ? id_str : ""),
				     gff_data->transcript_count) ;
	    }
	}
    }

  if (id_str)
    g_free(id_str) ;

  return status ;
}



/* ExBlx Format: print out the exons taking account of the extent of the CDS within the transcript. */
static gboolean processExonsExblx(blixemData blixem_data, ZMapFeature feature, gboolean cds_only)
{
  gboolean status = TRUE ;
  GIOChannel *channel ;
  int i ;
  ZMapSpan span = NULL ;
  int score = -1, qstart, qend, sstart, send ;
  int min, max ;
  int exon_base ;
  int cds_start, cds_end ;				    /* Only used if cds_only == TRUE. */
  GString *line ;
  char *ref_name ;
  char *transcript_name ;
  char *source_name ;

  channel = blixem_data->exblx_channel ;

  line = blixem_data->line ;

  min = blixem_data->features_min ;
  max = blixem_data->features_max ;

  ref_name = (char *)g_quark_to_string(blixem_data->block->original_id) ;
  transcript_name = (char *)g_quark_to_string(feature->original_id) ;
  source_name = (char *)g_quark_to_string(feature->source_id) ;

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
	  int exon_start, exon_end ;

	  exon_start = span->x1 ;
	  exon_end = span->x2 ;

	  /* Truncate to CDS start/end in transcript. */
	  if (cds_only)
	    {
	      if (cds_start >= span->x1 && cds_start <= span->x2)
		exon_start = cds_start ;
	      if (cds_end >= span->x1 && cds_end <= span->x2)
		exon_end = cds_end ;
	    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* We only export exons that fit completely within the blixem scope. */
	  if (exon_start >= min && exon_end <= max)
	    {
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      /* Note that sframe is meaningless so is set to an invalid value. */
	      if (feature->strand == ZMAPSTRAND_REVERSE)
		{
		  qstart = exon_end ;
		  qend   = exon_start ;
		  sstart = ((exon_base - (exon_end - exon_start + 1)) + 3) / 3 ;
		  send = (exon_base - 1) / 3 ;
		}
	      else
		{
		  qstart = exon_start ;
		  qend   = exon_end ;
		  sstart = (exon_base + 3) / 3 ;
		  send   = (exon_base + (exon_end - exon_start)) / 3 ;
		}

	      printExonExblx(line, min, max,
				    transcript_name,
				    score,
				    exon_base, exon_start, exon_end,
				    qstart, qend, feature->strand,
				    sstart, send) ;

	      status = printLine(channel, &(blixem_data->errorMsg), line->str) ;

	      blixem_data->line = g_string_truncate(blixem_data->line, 0) ; /* Reset string buffer. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	  if (feature->strand == ZMAPSTRAND_REVERSE)
	    {
	      exon_base -= (exon_end - exon_start + 1);
	    }
	  else
	    {
	      exon_base += (exon_end - exon_start + 1) ;
	    }
	}
    }


  return status ;
}


static gboolean printExonExblx(GString *line, int min, int max,
			       char *transcript_name,
			       float score_unused,
			       int exon_base, int exon_start, int exon_end,
			       int qstart, int qend, int qstrand,
			       int sstart, int send)
{
  gboolean status = TRUE;
  char qframe_strand ;
  int qframe ;
  char *sframe_str ;


  /* Note that sframe is meaningless so is set to an invalid value. */
  if (qstrand == ZMAPSTRAND_REVERSE)
    {
      qframe_strand = '-' ;
      qframe = ( ((max - qstart) - (exon_base - (exon_end - exon_start + 1))) % 3) + 1 ;

      sframe_str = "(-0)" ;
    }
  else
    {
      qframe_strand = '+' ;
      qframe = ((qstart - 1 - exon_base) % 3) + 1 ;

      sframe_str = "(+0)" ;
    }

  g_string_printf(line, "-1\t(%c%d)\t%d\t%d\t%s\t%d\t%d\t%sx\n",
		  qframe_strand, qframe, qstart, qend,
		  sframe_str, sstart, send,
		  transcript_name) ;

  return status ;
}


static gboolean printIntronsExblx(ZMapFeature feature, blixemData  blixem_data, gboolean cds_only)
{
  gboolean status = TRUE;
  int min, max ;
  int cds_start, cds_end ;
  int score = 0, qstart, qend, sstart, send;
  ZMapSpan span = NULL;
  char *qframe, *sframe ;


  /* Do the introns... */
  if (feature->feature.transcript.introns)
    {
      int i ;

      cds_start = feature->feature.transcript.cds_start ;
      cds_end = feature->feature.transcript.cds_end ;
      min = blixem_data->features_min ;
      max = blixem_data->features_max ;

      for (i = 0; i < feature->feature.transcript.introns->len && status; i++)
	{
	  span = &g_array_index(feature->feature.transcript.introns, ZMapSpanStruct, i);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* Only print introns that are within the cds section of the transcript and
	   * within the blixem scope. */
	  if ((span->x1 < min || span->x2 > max)
	      || (cds_only && (span->x1 > cds_end || span->x2 < cds_start)))
	    {
	      continue ;
	    }
	  else
	    {
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      GString *line ;

	      line = blixem_data->line ;

	      if (feature->strand == ZMAPSTRAND_REVERSE)
		{
		  qstart = span->x2 ;
		  qend   = span->x1 ;
		}
	      else
		{
		  qstart = span->x1 ;
		  qend   = span->x2 ;
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

	      status = printLine(blixem_data->exblx_channel, &(blixem_data->errorMsg), line->str) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	}
    }

  return status ;
}



/* This is were we really need gffv3.... */
static gboolean printBasic(ZMapFeature feature, blixemData  blixem_data)
{
  gboolean status = TRUE ;
  char *ref_name ;
  char *source_name ;
  static BasicFeatureDumpStruct dumpers[] =
    {
      {SO_ACC_DELETION, formatVariant},
      {SO_ACC_INSERTION,  formatVariant},
      {SO_ACC_POLYA_SIGNAL, formatPolyA},
      {SO_ACC_POLYA_SITE, formatPolyA},
      {SO_ACC_SEQ_ALT, formatVariant},
      {SO_ACC_SNP, formatVariant},
      {SO_ACC_SUBSTITUTION, formatVariant},
      {NULL, NULL}
    } ;
  BasicFeatureDump curr ;


  /* Swop to other strand..... */
  if (blixem_data->view->revcomped_features)
    zMapFeatureReverseComplement(blixem_data->view->features, feature) ;




  ref_name = (char *)g_quark_to_string(blixem_data->block->original_id) ;
  source_name = (char *)g_quark_to_string(feature->source_id) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (g_ascii_strcasecmp(source_name, "polya_site") == 0
      || g_ascii_strcasecmp(source_name, "polya_signal") == 0)
    printf("found it\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  curr = dumpers ;
  while (curr->source_name)
    {
      if (g_ascii_strcasecmp(g_quark_to_string(feature->SO_accession), curr->source_name) == 0)
	break ;
      else
	curr++ ;
    }

  if (curr->source_name)
    {
      status = curr->dump_func(blixem_data->format_data, blixem_data->line, ref_name, source_name, feature) ;
    }

  if (status)
    status = printLine(blixem_data->gff_channel, &(blixem_data->errorMsg), blixem_data->line->str) ;



   /* And swop it back again. */
  if (blixem_data->view->revcomped_features)
    zMapFeatureReverseComplement(blixem_data->view->features, feature) ;


  return status ;
}


static gboolean formatPolyA(GFFFormatData gff_data, GString *line,
			    char *ref_name, char *source_name, ZMapFeature feature)
{
  gboolean status = TRUE ;
  char *id_str = NULL ;


  if (gff_data->maximise_ids)
    {
      gff_data->feature_count++ ;

      id_str = g_strdup_printf("ID=feature%d;", gff_data->feature_count) ;
    }

  g_string_append_printf(line, "%s\t%s\t%s\t%d\t%d\t.\t%c\t.%s%s\n",
			 ref_name, source_name,
			 zMapSOAcc2Term(feature->SO_accession),
			 feature->x1, feature->x2,
			 (feature->strand == ZMAPSTRAND_REVERSE ? '-' : '+'),
			 (id_str ? "\t" : ""),
			 (id_str ? id_str : "")) ;

  return status ;
}


static gboolean formatVariant(GFFFormatData gff_data, GString *line,
			      char *ref_name, char *source_name, ZMapFeature feature)
{
  gboolean status = TRUE ;

  /* do mandatory fields */
  g_string_append_printf(line, "%s\t%s\t%s\t%d\t%d\t.\t%c\t.",
			 ref_name, source_name, zMapSOAcc2Term(feature->SO_accession),
			 feature->x1, feature->x2,
			 (feature->strand == ZMAPSTRAND_REVERSE ? '-' : '+')) ;

  /* Do attribute fields */
  g_string_append_printf(line, "\tName=%s;",
			 (char *)g_quark_to_string(feature->original_id)) ;

  if (gff_data->maximise_ids)
    {
      char *id_str = NULL ;

      gff_data->feature_count++ ;

      id_str = g_strdup_printf("ID=feature%d;", gff_data->feature_count) ;

      g_string_append(line, id_str) ;

      g_free(id_str) ;
    }

  if (feature->url)
    {
      char *url_escaped ;
      char *url_str = NULL ;

#if GLIB_MINOR_VERSION > 15
      /* The final arg is to allow utf_8 chars, I've put FALSE but I'm not sure. */
      url_escaped = g_uri_escape_string(feature->url, NULL, FALSE) ;
#else
      url_escaped = g_strdup(feature->url) ;
#endif

      url_str = g_strdup_printf("url=%s;", url_escaped) ;

      g_string_append(line, url_str) ;

      g_free(url_str) ;
      g_free(url_escaped) ;
    }

  if (feature->feature.basic.has_attr.variation_str)
    {
      char *variant_str = NULL ;

      variant_str = g_strdup_printf("variant_sequence=%s;", feature->feature.basic.variation_str) ;

      g_string_append(line, variant_str) ;

      g_free(variant_str) ;
    }

  g_string_append_c(line, '\n') ;

  return status ;
}





/* Output a line to file, returns FALSE if write fails and sets err_msg_out with
 * the reason. */
static gboolean printLine(GIOChannel *gio_channel, char **err_msg_out, char *line)
{
  gboolean status = TRUE ;
  GError *channel_error = NULL ;
  gsize bytes_written ;

  if (g_io_channel_write_chars(gio_channel, line, -1,
			       &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
    {
      *err_msg_out = g_strdup_printf("Could not write to file, error \"%s\" for line \"%50s...\"",
				     channel_error->message, line) ;
      g_error_free(channel_error) ;
      channel_error = NULL;
      status = FALSE ;
    }

  return status ;
}



/* A GFunc() to step through the named feature sets and write them out for passing
 * to blixem. */
static void getSetList(gpointer data, gpointer user_data)
{
  GQuark set_id = GPOINTER_TO_UINT(data) ;
  GQuark canon_id ;
  blixemData blixem_data = (blixemData)user_data ;
  ZMapFeatureSet feature_set ;
  GList *column_2_featureset;

  canon_id = zMapFeatureSetCreateID((char *)g_quark_to_string(set_id)) ;

  feature_set = g_hash_table_lookup(blixem_data->block->feature_sets, GINT_TO_POINTER(canon_id));

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
            if((feature_set = g_hash_table_lookup(blixem_data->block->feature_sets, column_2_featureset->data)))
            {
                  g_hash_table_foreach(feature_set->features, getFeatureCB, blixem_data);
            }
#if MH17_GIVES_SPURIOUS_ERRORS
/*
We get a featureset to column mapping that includes all possible
but without features for eachione the set does not get created
in which case here a not found error is not an error
but not finding the column or featureset in the first attempt is
previous code would not have found *_trunc featuresets!
*/
            else
            {
                 zMapLogWarning("Could not find %s feature set \"%s\" in context feature sets.",
                       (blixem_data->required_feature_type == ZMAPSTYLE_MODE_ALIGNMENT
                        ? "alignment" : "transcript"),
                       g_quark_to_string(GPOINTER_TO_UINT(column_2_featureset->data))) ;

            }
#endif
      }
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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		if ((feature->x1 >= blixem_data->min && feature->x1 <= blixem_data->max)
		    && (feature->x2 >= blixem_data->min && feature->x2 <= blixem_data->max))
		  {
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

		    if ((*blixem_data->opts == 'X' && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
			|| (*blixem_data->opts == 'N' && feature->feature.homol.type == ZMAPHOMOL_N_HOMOL))
		      blixem_data->align_list = g_list_append(blixem_data->align_list, feature) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

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
  gsize bytes_written ;
  GError *channel_error = NULL ;
  char *line = NULL ;
  enum { FASTA_CHARS = 50 } ;
  char buffer[FASTA_CHARS + 2] ;			    /* FASTA CHARS + \n + \0 */
  int lines = 0, chars_left = 0 ;
  char *cp = NULL ;
  int i ;


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

	  if (blixem_data->view->revcomped_features)
	    zMapFeatureReverseComplementCoords(blixem_data->block, &start, &end) ;

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
				  start, end, blixem_data->view->revcomped_features) ;

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
		      if ((feature->x1 >= blixem_data->features_min && feature->x1 <= blixem_data->features_max)
			  && (feature->x2 >= blixem_data->features_min && feature->x2 <= blixem_data->features_max))
			{
			  if ((*blixem_data->opts == 'X' && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
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

  if (!text || (text && !*text))
    allowed = FALSE;
  else if (note_any->name == g_quark_from_string("Launch script"))
    {
      if (!(allowed = zMapFileAccess(text, "x")))
	zMapWarning("%s is not executable.", text);
      allowed = TRUE;		/* just warn for now */
    }
  else if (note_any->name == g_quark_from_string("Config File"))
    {
      if (!(allowed = zMapFileAccess(text, "r")))
	zMapWarning("%s is not readable.", text);
      allowed = TRUE;		/* just warn for now */
    }
  else if (note_any->name == g_quark_from_string("Port"))
    {
      int tmp = 0;
      if (zMapStr2Int(text, &tmp))
	{
	  int min = 1024, max = 65535;
	  if (tmp < min || tmp > max)
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

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Restrict scope to Mark",
					   ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
					   "bool", blixem_config_curr_G.scope_from_mark) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, "Restrict features to Mark",
					   ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
					   "bool", blixem_config_curr_G.features_from_mark) ;

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

      /* ADD VALIDATION.... */

      if (zMapGUINotebookGetTagValue(page, "Host network id", "string", &string_value))
	{
	  if (string_value && *string_value
	      && strcmp(string_value, blixem_config_curr_G.netid) != 0)
	    {
	      g_free(blixem_config_curr_G.netid) ;

	      blixem_config_curr_G.netid = g_strdup(string_value) ;
	      blixem_config_curr_G.is_set.netid = TRUE ;
	    }
	}

      if (zMapGUINotebookGetTagValue(page, "Port", "int", &int_value))
	{
	  if (int_value != blixem_config_curr_G.port)
	    {
	      blixem_config_curr_G.port = int_value ;
	      blixem_config_curr_G.is_set.port = TRUE ;
	    }
	}

    }

  if ((page = zMapGUINotebookFindPage(chapter, BLIX_NB_PAGE_ADVANCED)))
    {

      /* ADD VALIDATION.... */

      if (zMapGUINotebookGetTagValue(page, "Config File", "string", &string_value))
	{
	  if (string_value && *string_value
	      && strcmp(string_value, blixem_config_curr_G.config_file) != 0)
	    {
	      g_free(blixem_config_curr_G.config_file) ;

	      blixem_config_curr_G.config_file = g_strdup(string_value) ;
	      blixem_config_curr_G.is_set.config_file = TRUE ;
	    }
	}

      if (zMapGUINotebookGetTagValue(page, "Launch script", "string", &string_value))
	{
	  if (string_value && *string_value
	      && strcmp(string_value, blixem_config_curr_G.script) != 0)
	    {
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

  if ((context = zMapConfigIniContextProvide()))
    {
      if (prefs->netid)
	zMapConfigIniContextSetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				      ZMAPSTANZA_BLIXEM_NETID, prefs->netid);

      zMapConfigIniContextSetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				 ZMAPSTANZA_BLIXEM_PORT, prefs->port);

      zMapConfigIniContextSetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				 ZMAPSTANZA_BLIXEM_SCOPE, prefs->scope);

      zMapConfigIniContextSetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				     ZMAPSTANZA_BLIXEM_SCOPE_MARK, prefs->scope_from_mark) ;

      zMapConfigIniContextSetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				     ZMAPSTANZA_BLIXEM_FEATURES_MARK, prefs->features_from_mark) ;

      zMapConfigIniContextSetInt(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				 ZMAPSTANZA_BLIXEM_MAX, prefs->homol_max);

      zMapConfigIniContextSetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				     ZMAPSTANZA_BLIXEM_KEEP_TEMP, prefs->keep_tmpfiles);

      zMapConfigIniContextSetBoolean(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				     ZMAPSTANZA_BLIXEM_KILL_EXIT, prefs->kill_on_exit);

      if (prefs->script)
	zMapConfigIniContextSetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				      ZMAPSTANZA_BLIXEM_SCRIPT, prefs->script);

      if (prefs->config_file)
	zMapConfigIniContextSetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				      ZMAPSTANZA_BLIXEM_CONF_FILE, prefs->config_file);


#ifdef RDS_DONT_INCLUDE
      if (prefs->dna_sets)
	zMapConfigIniContextSetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				      ZMAPSTANZA_BLIXEM_DNA_FS, prefs->dna_sets);

      if (prefs->protein_sets)
	zMapConfigIniContextSetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				      ZMAPSTANZA_BLIXEM_PROT_FS, prefs->protein_sets);

      if (prefs->transcript_sets)
	zMapConfigIniContextSetString(context, ZMAPSTANZA_BLIXEM_CONFIG, ZMAPSTANZA_BLIXEM_CONFIG,
				      ZMAPSTANZA_BLIXEM_FS, prefs->transcript_sets);
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

/* Length of a block. */
static int calcBlockLength(ZMapSequenceType match_seq_type, int start, int end)
{
  int coord ;

  coord = (end - start) + 1 ;

  if (match_seq_type == ZMAPSEQUENCE_PEPTIDE)
    coord /= 3 ;

  return coord ;
}

/* Length between two blocks. */
static int calcGapLength(ZMapSequenceType match_seq_type, int start, int end)
{
  int coord ;

  coord = (end - start) - 1 ;

  if (match_seq_type == ZMAPSEQUENCE_PEPTIDE)
    coord /= 3 ;

  return coord ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printFunc(gpointer key, gpointer value, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)value ;

  printf("Set name orig: \"%s\"\tuniq: \"%s\"\n",
	 g_quark_to_string(feature_set->original_id),
	 g_quark_to_string(feature_set->unique_id)) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





/*************************** end of file *********************************/
