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
 * Last edited: Jul 21 11:14 2007 (edgrif)
 * Created: Thu Jun 28 18:10:08 2007 (edgrif)
 * CVS info:   $Id: zmapViewCallBlixem.c,v 1.2 2007-07-24 10:47:41 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <unistd.h>					    /* for getlogin() */
#include <string.h>					    /* for memcpy */
#include <glib.h>
#include <zmapView_P.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfig.h>


#define ZMAP_BLIXEM_CONFIG "blixem"

/* ARGV positions for building argv to pass to g_spawn_async*/
enum
  {
    BLX_ARGV_PROGRAM,           /* blixem */
    BLX_ARGV_NETID_PORT_FLAG,   /* -P */
    BLX_ARGV_NETID_PORT,        /* [hostname:port] */
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
  gchar         *Netid;                               /* eg pubseq */
  int            Port;                                /* eg 22100  */
  gchar         *Script;                              /* script to call blixem standalone */
  int            Scope;                               /* defaults to 40000 */
  int            HomolMax;                            /* score cutoff point */

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
  ZMapFeatureType required_feature_type ;		    /* specifies what type of feature
							     * needs to be processed. */

  /* User can specify sets of homologies that can be passed to blixem, if the set they selected
   * is in one of these sets then all the features in all the sets are sent to blixem. */
  ZMapHomolType align_type ;				    /* What type of alignment are we doing ? */
  GList *dna_sets ;
  GList *protein_sets ;
  GList *transcript_sets ;

  int offset ;
  int min, max ;
  int qstart, qend ; 
  GString *line ;

} blixemDataStruct, *blixemData ;


static gboolean initBlixemData(ZMapView view, ZMapFeature feature, blixemData blixem_data, char **err_msg) ;
static gboolean getUserPrefs     (blixemData blixem_data);
static gboolean addFeatureDetails(blixemData blixem_data) ;
static gboolean buildParamString (blixemData blixem_data, char **paramString);
static void     freeBlixemData   (blixemData blixem_data);

static void checkForLocalSequence(gpointer key, gpointer data, gpointer user_data) ;
static gboolean makeTmpfiles(blixemData blixem_data) ;
gboolean makeTmpfile(char *tmp_dir, char **tmp_file_name_out) ;

static void     processFeatureSet(gpointer key_id, gpointer data, gpointer user_data);
static void processSetList(gpointer data, gpointer user_data) ;

static gboolean writeFastAFile(blixemData blixem_data);
static gboolean writeExblxSeqblFiles(blixemData blixem_data);

static void writeExblxSeqblLine(gpointer key_id, gpointer data, gpointer user_data);
static gboolean printAlignment(ZMapFeature feature, blixemData  blixem_data) ;
static gboolean printTranscript(ZMapFeature feature, blixemData  blixem_data) ;
static gboolean printLine(blixemData blixem_data, char *line);
static gboolean processExons(blixemData blixem_data, ZMapFeature feature);

static int findFeature(gconstpointer a, gconstpointer b) ;
static void freeSequences(gpointer data, gpointer user_data_unused) ;


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
  blixem_data.required_feature_type = ZMAPFEATURE_ALIGNMENT ;


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
      g_hash_table_foreach(feature_set->features, writeExblxSeqblLine, blixem_data) ;
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
 * The function returns TRUE if blixem was successfully launched and also the pid of the blixem
 * process so that the blixems can be cleared up when the view exits.
 *  */
gboolean zmapViewCallBlixem(ZMapView view, ZMapFeature feature, GList *local_sequences, GPid *child_pid)
{
  gboolean status = TRUE ;
  char *argv[BLX_ARGV_ARGC + 1] = {NULL} ;
  blixemDataStruct blixem_data = {0} ;
  char *err_msg = "error in zmapViewCallBlixem()" ;


  zMapAssert(view && zMapFeatureIsValid((ZMapFeatureAny)feature) && child_pid) ;

  status = initBlixemData(view, feature, &blixem_data, &err_msg) ;

  blixem_data.local_sequences = local_sequences ;

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
      
      argv[BLX_ARGV_PROGRAM] = g_strdup_printf("%s", blixem_data.Script);

      if(!(g_spawn_async(cwd, &argv[0], envp, flags, pre_exec, pre_exec_data, &spawned_pid, &error)))
        {
          status = FALSE;
          err_msg = error->message;
        }
      else
        zMapLogMessage("Blixem process spawned successfully. PID = '%d'", spawned_pid);

      if(status && child_pid)
        *child_pid = spawned_pid;
    }

  freeBlixemData(&blixem_data) ;

  if (!status)
    zMapShowMsg(ZMAP_MSG_WARNING, err_msg) ;

  return status ;
}


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
    status = getUserPrefs(blixem_data) ;

  if (status)
    status = addFeatureDetails(blixem_data) ;

  return status ;
}



static gboolean getUserPrefs(blixemData blixem_data)
{
  gboolean            status = FALSE;
  ZMapConfig          config ;
  ZMapConfigStanzaSet list = NULL ;
  char               *stanza_name = ZMAP_BLIXEM_CONFIG ;
  ZMapConfigStanza    stanza ;
  ZMapConfigStanzaElementStruct elements[] = {{"netid"     , ZMAPCONFIG_STRING, {NULL}},
					      {"port"      , ZMAPCONFIG_INT   , {NULL}},
					      {"script"    , ZMAPCONFIG_STRING, {NULL}},
					      {"keep_tempfiles", ZMAPCONFIG_BOOL, {NULL}},
					      {"scope"     , ZMAPCONFIG_INT   , {NULL}},
					      {"homol_max" , ZMAPCONFIG_INT   , {NULL}},
					      {"dna_featuresets", ZMAPCONFIG_STRING, {NULL}},
					      {"protein_featuresets", ZMAPCONFIG_STRING, {NULL}},
					      {"transcript_featuresets", ZMAPCONFIG_STRING, {NULL}},
					      {NULL, -1, {NULL}}} ;
  if ((config = zMapConfigCreate()))
    {
      char *dnaset_string, *proteinset_string, *transcriptset_string ;

      /* get blixem user prefs from config file. */
      stanza = zMapConfigMakeStanza(stanza_name, elements) ;

      if (zMapConfigFindStanzas(config, stanza, &list))
	{
	  ZMapConfigStanza next ;
	  
	  /* Get the first blixem stanza found, we will ignore any others. */
	  next = zMapConfigGetNextStanza(list, NULL) ;
	  
	  blixem_data->Netid    = g_strdup(zMapConfigGetElementString(next, "netid"    ));
	  blixem_data->Port     = zMapConfigGetElementInt            (next, "port"      );
	  blixem_data->Script   = g_strdup(zMapConfigGetElementString(next, "script"   ));
	  blixem_data->Scope    = zMapConfigGetElementInt            (next, "scope"     );
	  blixem_data->HomolMax = zMapConfigGetElementInt            (next, "homol_max" );
	  dnaset_string = zMapConfigGetElementString(next, "dna_featuresets") ;
	  proteinset_string = zMapConfigGetElementString(next, "protein_featuresets") ;
	  transcriptset_string = zMapConfigGetElementString(next, "transcript_featuresets") ;

	  if (dnaset_string)
	    blixem_data->dna_sets = zMapFeatureString2QuarkList(dnaset_string) ;

	  if (proteinset_string)
	    blixem_data->protein_sets = zMapFeatureString2QuarkList(proteinset_string) ;

	  if (transcriptset_string)
	    blixem_data->transcript_sets = zMapFeatureString2QuarkList(transcriptset_string) ;

	  blixem_data->keep_tmpfiles = zMapConfigGetElementBool(next, "keep_tempfiles") ;
	  
	  zMapConfigDeleteStanzaSet(list) ;		    /* Not needed anymore. */
	}
      
      zMapConfigDestroyStanza(stanza) ;
      
      zMapConfigDestroy(config) ;
      
      if (blixem_data->Netid              /* scope and homolMax can be zero so don't check them */
	  && blixem_data->Port
	  && blixem_data->Script)
	{
	  char *tmp ;

	  tmp = blixem_data->Script ;

	  if ((blixem_data->Script = g_find_program_in_path(tmp)))
	    status = TRUE;
	  else
	    zMapShowMsg(ZMAP_MSG_WARNING, 
			"Either can't locate \"%s\" in your path or it is not executable by you.",
			tmp) ;

	  g_free(tmp) ;
	}
      else
	zMapShowMsg(ZMAP_MSG_WARNING, "Some or all of the compulsory blixem parameters "
		    "\"netid\", \"port\" or \"script\" are missing from your config file.");
    }

  return status;
}



static gboolean addFeatureDetails(blixemData blixem_data)
{
  gboolean status = TRUE;
  ZMapFeature feature;
  int scope = 40000 ;					    /* can be set from user prefs */
  int x1, x2;
  int origin ;

  if (blixem_data->Scope > 0)
    scope = blixem_data->Scope ;

  feature = blixem_data->feature ;

  x1 = feature->x1 ;
  x2 = feature->x2 ;

  /* Set min/max range for blixem, clamp to our sequence. */
  blixem_data->min = x1 - (scope / 2) ;
  blixem_data->max = x2 + (scope / 2) ;
  if (blixem_data->min < 1)
    blixem_data->min = 1 ;
  if (blixem_data->max > blixem_data->view->features->length)
    blixem_data->max = blixem_data->view->features->length ;


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
 * display alignments. */
static gboolean makeTmpfiles(blixemData blixem_data)
{
  gboolean    status = TRUE;
  char       *path;
  char       *login;

  if ((login = getlogin()))
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
    }

  if (path)
    g_free(path) ;

  /* Create the file to the hold the DNA in FastA format. */
  if (status)
    {
      status = makeTmpfile(blixem_data->tmpDir, &(blixem_data->fastAFile)) ;
    }

  /* Create the file to hold the features in exblx format. */
  if (status)
    {
      status = makeTmpfile(blixem_data->tmpDir, &(blixem_data->exblxFile)) ;
    }
     
  /* Create the file to hold the features in seqbl format. */
  if (status)
    {
      status = makeTmpfile(blixem_data->tmpDir, &(blixem_data->seqbl_file)) ;
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

     if (blixem_data->Netid && blixem_data->Port)
       {
         paramString[BLX_ARGV_NETID_PORT_FLAG] = g_strdup("-P");
         paramString[BLX_ARGV_NETID_PORT]      = g_strdup_printf("%s:%d", blixem_data->Netid, blixem_data->Port);
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
      g_string_printf(blixem_data->line, "# exblx\n# blast%c\n", *blixem_data->opts) ;
      blixem_data->curr_channel = blixem_data->exblx_channel ;
      status = printLine(blixem_data, blixem_data->line->str) ;
      blixem_data->line = g_string_truncate(blixem_data->line, 0) ;
    }
  else
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not open exblx file: %s",
		  channel_error->message) ;
      g_error_free(channel_error) ;
      status = FALSE ;
    }

  /* Open the seqbl file, if needed. */
  if (status)
    {
      if (seqbl_file)
	{
	  if ((blixem_data->seqbl_channel = g_io_channel_new_file(blixem_data->seqbl_file, "w", &channel_error)))
	    {
	      g_string_printf(blixem_data->line, "# seqbl\n# blast%c\n", *blixem_data->opts) ;
	      blixem_data->curr_channel = blixem_data->seqbl_channel ;
	      status = printLine(blixem_data, blixem_data->line->str) ;
	      blixem_data->line = g_string_truncate(blixem_data->line, 0) ;
	    }
	  else
	    {
	      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not open seqbl file: %s",
			  channel_error->message) ;
	      g_error_free(channel_error) ;
	      status = FALSE ;
	    }
	}
    }

  if (status)
    {
      ZMapFeature feature = blixem_data->feature ;
      ZMapFeatureSet feature_set ;
      GList *set_list ;

      blixem_data->errorMsg = NULL ;

      feature_set = (ZMapFeatureSet)(feature->parent) ;

      /* There is no way to interrupt g_hash_table_foreach(), so instead,
       * if printLine() encounters a problem, we store the error message
       * in blixem_data and skip all further processing, displaying the
       * error when we get back to writeExblxFile().  */

      /* Do requested Homology data first. */
      blixem_data->required_feature_type = ZMAPFEATURE_ALIGNMENT ;

      blixem_data->align_type = feature->feature.homol.type ;
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
	  g_hash_table_foreach(feature_set->features, writeExblxSeqblLine, blixem_data) ;
	}


      /* Now do transcripts (may need to filter further..... */
      blixem_data->required_feature_type = ZMAPFEATURE_TRANSCRIPT ;
      if (blixem_data->transcript_sets)
	{
	  g_list_foreach(blixem_data->transcript_sets, processSetList, blixem_data) ;
	}
      else
	{
	  g_hash_table_foreach(blixem_data->block->feature_sets, processFeatureSet, blixem_data) ;
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


/* Called when we need to process multiple sets of alignments. */
static void processFeatureSet(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data;
  blixemData     blixem_data = (blixemData)user_data;

  g_hash_table_foreach(feature_set->features, writeExblxSeqblLine, blixem_data) ;

  return ;
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
		     (blixem_data->required_feature_type == ZMAPFEATURE_ALIGNMENT
		      ? "alignment" : "transcript"),
		     g_quark_to_string(set_id)) ;
    }
  else
    {
      g_hash_table_foreach(feature_set->features, writeExblxSeqblLine, blixem_data);
    }

  return ;
}



static void writeExblxSeqblLine(gpointer key, gpointer data, gpointer user_data)
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
	  gboolean status = TRUE ;

	  switch (feature->type)
	    {
	    case ZMAPFEATURE_ALIGNMENT:
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
	    case ZMAPFEATURE_TRANSCRIPT:
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
  char qframe_strand ;
  int qframe ;
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


      sstart = feature->feature.homol.y2 ;
      send   = feature->feature.homol.y1 ;

    }
  else
    {
      qframe_strand = '+' ;
      qframe = 1 + ((qstart - 1) % 3) ;

      sstart = feature->feature.homol.y1 ;
      send   = feature->feature.homol.y2 ;

    }


  /* Not sure about this.... */
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

      g_string_printf(line, "%d\t(%c%d)\t%d\t%d\t%d\t%d %s %s", 
		      score, qframe_strand, qframe,
		      qstart, qend,
		      sstart, send,
		      g_quark_to_string(feature->original_id), seq_str) ;

      if (feature->feature.homol.align)
	{
	  int k, index, incr ;
		      
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
  char *qframe ;
  GString *line;


  line = blixem_data->line ;

  if (feature->feature.transcript.flags.cds)
    {
      zMapAssert(feature->feature.transcript.cds_start != 0
		 && feature->feature.transcript.cds_end != 0) ;

      blixem_data->curr_channel = blixem_data->exblx_channel ;

      /* Print the exons which require special handling for the CDS. */
      status = processExons(blixem_data, feature) ;

      min = blixem_data->min ;
      max = blixem_data->max ;

      cds_start = feature->feature.transcript.cds_start ;
      cds_end = feature->feature.transcript.cds_end ;


      if (status && feature->feature.transcript.introns)
	{	      
	  int i ;

	  for (i = 0; i < feature->feature.transcript.introns->len && status; i++)
	    {
	      span = &g_array_index(feature->feature.transcript.introns, ZMapSpanStruct, i);
		      
	      /* Only print introns that are within the cds section of the transcript and
	       * within the blixem scope. */
	      if ((span->x1 < min || span->x2 > max)
		  || (span->x1 > cds_end || span->x2 < cds_start))
		{
		  continue ;
		}
	      else
		{
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

		  if (feature->strand == ZMAPSTRAND_REVERSE)
		    qframe = "(-1)" ;
		  else
		    qframe = "(+1)" ;
		      
		  g_string_printf(line, "%d %s\t%d\t%d\t%d\t%d\t%si\n", 
				  score,
				  qframe, qstart, qend, sstart,
				  send,
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
      status = FALSE ;
    }
    
  return status ;
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
  enum { FASTA_CHARS = 50 };
  ZMapFeature feature = NULL;
  ZMapFeatureBlock block = NULL;
  gsize    bytes_written;
  GError  *channel_error = NULL;
  char    *line;
  gboolean status = TRUE;
  char     buffer[FASTA_CHARS + 2] ;			    /* FASTA CHARS + \n + \0 */
  int      lines = 0, chars_left = 0 ;
  char    *cp = NULL ;
  int      i ;

  feature = blixem_data->feature;

  if ((blixem_data->fasta_channel = g_io_channel_new_file(blixem_data->fastAFile, "w", &channel_error)))
    {
      line = g_strdup_printf(">%s\n", 
			     g_quark_to_string(blixem_data->view->features->parent_name));
      if (g_io_channel_write_chars(blixem_data->fasta_channel, 
				   line, -1, &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "Error writing header record to fastA file: %50s... : %s",
		      line, channel_error->message) ;
	  g_error_free(channel_error) ;
	  g_free(line);
	  status = FALSE ;
	}
      else if((block = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)feature,
                                                                   ZMAPFEATURE_STRUCT_BLOCK)) &&
              zMapFeatureBlockDNA(block, NULL, NULL, &cp))
	{
	  int total_chars ;

	  g_free(line);					    /* ugh...just horrible coding.... */

	  total_chars = (blixem_data->max - blixem_data->min + 1) ;

	  lines = total_chars / FASTA_CHARS ;
	  chars_left = total_chars % FASTA_CHARS ;

#ifdef RDS_DONT_INCLUDE          
	  cp = blixem_data->view->feature_context->sequence->sequence + blixem_data->min - 1 ;
#endif
          cp += (blixem_data->min - 1);

	  /* Do the full length lines.                                           */
	  if (lines > 0)
	    {
	      buffer[FASTA_CHARS] = '\n' ;
	      buffer[FASTA_CHARS + 1] = '\0' ; 
	      for (i = 0 ; i < lines && status; i++)
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
	  if (chars_left > 0 && status)
	    {
	      memcpy(&buffer[0], cp, chars_left) ;
	      buffer[chars_left] = '\n' ;
	      buffer[chars_left + 1] = '\0' ; 
	      if (g_io_channel_write_chars(blixem_data->fasta_channel,
					   &buffer[0], -1, 
					   &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
		status = FALSE ;
	    }
	  if (status == FALSE)
	    {
	      zMapShowMsg(ZMAP_MSG_WARNING, "Error: writing to fastA file: %s",
			  channel_error->message) ;
	      g_error_free(channel_error) ;
	    }
	}        /* if g_io_channel_write_chars(.... */
      else
        zMapShowMsg(ZMAP_MSG_WARNING, "Error: writing to file, failed to get feature's DNA");

      g_io_channel_shutdown(blixem_data->fasta_channel, TRUE, &channel_error);
      if (channel_error)
	g_error_free(channel_error);
    }
  else           /* if (blixem_data->fasta_channel = ... */
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not open fastA file: %s",
		  channel_error->message) ;
      g_error_free(channel_error) ;
      status = FALSE;
    }

  return status;
}



/* Print out the exons taking account of the extent of the CDS within the transcript. */
static gboolean processExons(blixemData blixem_data, ZMapFeature feature)
{
  gboolean status = TRUE ;
  int i ;
  ZMapSpan span = NULL ;
  int score = -1, qstart, qend, sstart, send ;
  int min, max ;
  int cds_start, cds_end, cds_base = 0 ;
  GString *line ;

  line = blixem_data->line ;

  min = blixem_data->min ;
  max = blixem_data->max ;

  cds_start = feature->feature.transcript.cds_start ;
  cds_end = feature->feature.transcript.cds_end ;


  /* We need to record how far we are along the exons in CDS relative coords, i.e. for the
   * reverse strand we need to calculate from the other end of the transcript. */
  cds_base = 0 ;
  if (feature->strand == ZMAPSTRAND_REVERSE)
    {
      for (i = 0 ; i < feature->feature.transcript.exons->len ; i++)
	{
	  int start, end ;

	  span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i) ;

	  if(span->x1 > cds_end || span->x2 < cds_start)
	    {
	      continue ;
	    }
	  else
	    {
	      start = span->x1 ;
	      end = span->x2 ;

	      /* Correct for CDS start/end in transcript. */
	      if (cds_start >= span->x1 && cds_start <= span->x2)
		start = cds_start ;
	      if (cds_end >= span->x1 && cds_end <= span->x2)
		end = cds_end ;

	      cds_base += end - start + 1 ;
	    }
	}
    }


  for (i = 0 ; i < feature->feature.transcript.exons->len ; i++)
    {
      span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i) ;

      /* We are only interested in the cds section of the transcript. */
      if(span->x1 > cds_end || span->x2 < cds_start)
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

	  /* Correct for CDS start/end in transcript. */
	  if (cds_start >= span->x1 && cds_start <= span->x2)
	    start = cds_start ;
	  if (cds_end >= span->x1 && cds_end <= span->x2)
	    end = cds_end ;
	  
	  /* We only export exons that fit completely within the blixem scope. */
	  if (start >= min && end <= max)
	    {
	      if (feature->strand == ZMAPSTRAND_REVERSE)
		{
		  qstart = end - min + 1;
		  qend   = start - min + 1;

		  qframe_strand = '-' ;
		  qframe = ( (((max - min + 1) - qstart) - (cds_base - (end - start + 1))) % 3) + 1 ;

		  sstart = ((cds_base - (end - start + 1)) + 3) / 3 ;
		  send = (cds_base - 1) / 3 ;
		}
	      else
		{
		  qstart = start - min + 1;
		  qend   = end - min + 1;
		  
		  qframe_strand = '+' ;
		  qframe = ((qstart - 1 - cds_base) % 3) + 1 ;
		  
		  sstart = (cds_base + 3) / 3 ;
		  send   = (cds_base + (end - start)) / 3 ;
		}
	      
	      g_string_printf(line, "%d\t(%c%d)\t%d\t%d\t%d\t%d\t%sx\n", 
			      score,
			      qframe_strand, qframe,
			      qstart, qend, sstart, send,
			      g_quark_to_string(feature->original_id)) ;
	      
	      status = printLine(blixem_data, line->str) ;

	      blixem_data->line = g_string_truncate(blixem_data->line, 0) ; /* Reset string buffer. */
	    }

	  if (feature->strand == ZMAPSTRAND_REVERSE)
	    {
	      cds_base -= (end - start + 1);
	    }
	  else
	    {
	      cds_base += (end - start + 1) ;
	    }
	}
    }

  
  return status ;
}


static void freeBlixemData(blixemData blixem_data)
{
  g_free(blixem_data->tmpDir);
  g_free(blixem_data->fastAFile);
  g_free(blixem_data->exblxFile);
  g_free(blixem_data->seqbl_file);
  g_free(blixem_data->Netid);
  g_free(blixem_data->Script);

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

}


gboolean makeTmpfile(char *tmp_dir, char **tmp_file_name_out)
{
  gboolean status = FALSE ;
  char *tmpfile ;
  int file = 0 ;

  /* g_mkstemp replaces the XXXXXX with unique string of chars */
  tmpfile = g_strdup_printf("%sZMAPXXXXXX", tmp_dir) ;

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
	  if (feature->type == ZMAPFEATURE_ALIGNMENT
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


/*************************** end of file *********************************/
