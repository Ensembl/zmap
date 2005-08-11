/*  File: zmapWindowCallBlixem.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * and was written by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack    (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: calls standalone blixem.
 *
 *              
 * Exported functions: 
 * HISTORY:
 * Last edited: Jul 18 10:14 2005 (edgrif)
 * Created: Tue May  9 14:30 2005 (rnc)
 * CVS info:   $Id: zmapWindowCallBlixem.c,v 1.13 2005-08-11 13:28:43 rds Exp $
 *-------------------------------------------------------------------
 */

#include <unistd.h>           /* for getlogin() */
#include <string.h>           /* for memcpy */
#include <sys/wait.h>         /* for WIFEXITED */
#include <glib.h>
#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfig.h>

#define ZMAP_BLIXEM_CONFIG "blixem"


typedef struct BlixemDataStruct 
{
  int            min, max, offset;
  char          *tmpDir;
  char          *fastAFile;
  char          *exblxFile;
  char          *opts;
  ZMapWindow     window;
  FooCanvasItem *item;
  gboolean       oneType;                                  /* show stuff for just this type */
  GIOChannel    *channel;
  char          *errorMsg;
  /* user preferences for blixem */
  gchar         *Netid;                               /* eg pubseq */
  int            Port;                                /* eg 22100  */
  gchar         *Script;                              /* script to call blixem standalone */
  int            Scope;                               /* defaults to 40000 */
  int            HomolMax;                            /* score cutoff point */
} blixemDataStruct, *blixemData;



static gboolean getUserPrefs     (blixemData blixem_data);
static char    *buildParamString (blixemData blixem_data);
static gboolean writeExblxFile   (blixemData blixem_data);
static void     processFeatureSet(GQuark key_id, gpointer data, gpointer user_data);
static void     writeExblxLine   (GQuark key_id, gpointer data, gpointer user_data);
static gboolean printLine        (blixemData blixem_data, char *line);
static gboolean writeFastAFile   (blixemData blixem_data);
static gboolean processExons     (blixemData blixem_data, ZMapFeature feature);
static void     freeBlixemData   (blixemData blixem_data);
static void     adjustCoords     (blixemData blixem_data,
				  int *sstart, int *send, int *qstart, int *qend);



void zmapWindowCallBlixem(ZMapWindow window, FooCanvasItem *item, gboolean oneType)
{
  gboolean         status;
  char            *commandString;
  char            *paramString = NULL;
  blixemDataStruct blixem_data = {0};
  

  status = getUserPrefs(&blixem_data);

  if (status == TRUE)
    {
      blixem_data.window  = window;
      blixem_data.item    = item;
      blixem_data.oneType = oneType;
      paramString = buildParamString(&blixem_data);

      if (!paramString)
	status = FALSE;	 
    } 

  if (status == TRUE)
    status = writeExblxFile(&blixem_data);

  if (status == TRUE)
    status = writeFastAFile(&blixem_data);

  if (status == TRUE)
    {
      int sysrc;
      commandString = g_strdup_printf("%s %s &", blixem_data.Script, paramString);
      sysrc = system(commandString);
      /* printf("%s\n", commandString); useful when debugging */

      /* Note that since blixem is being called as a background process, 
      ** we can only tell whether or not the system call was successful,
      ** and not whether or not blixem ran successfully. */
      if (WIFEXITED(sysrc) == FALSE)
	zMapShowMsg(ZMAP_MSG_WARNING, "System call failed: blixem not invoked.") ;
      
      g_free(commandString);
    }
   
  if (paramString)   
    g_free(paramString);

  freeBlixemData(&blixem_data);

  return;
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
					      {"scope"     , ZMAPCONFIG_INT   , {NULL}},
					      {"homol_max" , ZMAPCONFIG_INT   , {NULL}},
					      {NULL, -1, {NULL}}} ;
  if ((config = zMapConfigCreate()))
    {
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
	  
	  zMapConfigDeleteStanzaSet(list) ;		    /* Not needed anymore. */
	}
      
      zMapConfigDestroyStanza(stanza) ;
      
      zMapConfigDestroy(config) ;
      
      if (blixem_data->Netid              /* scope and homolMax can be zero so don't check them */
	  && blixem_data->Port
	  && blixem_data->Script)
	{
	  if (g_file_test(blixem_data->Script, G_FILE_TEST_EXISTS)
	      && g_file_test(blixem_data->Script, G_FILE_TEST_IS_EXECUTABLE))
	    status = TRUE;
	  else
	    zMapShowMsg(ZMAP_MSG_WARNING, 
			"Can't run %s script; check it exists and is executable by you.",
			blixem_data->Script);
	}
      else
	zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not get blixem parameters from config file.");
    }

  return status;
}


static char *buildParamString(blixemData blixem_data)
{
  gboolean    status = TRUE;
  ZMapFeature feature;
  char       *paramString = NULL;
  char       *path;
  char       *login;
  char       *tmpfile;
  int         file = 0;
  int         start;
  int         scope = 40000;          /* set from user prefs */
  int         x1, x2;


  if (blixem_data->Scope > 0)
    scope = blixem_data->Scope;

  feature = g_object_get_data(G_OBJECT(blixem_data->item), "item_feature_data");
  zMapAssert(feature);       /* something badly wrong if no feature. */

  /* visible coords are 1-based, but internally we work with 0-based coords */
  x1 = feature->x1 - 1;
  x2 = feature->x2 - 1;

  blixem_data->min = x1 - scope/2;
  blixem_data->max = x2 + scope/2;
  if (blixem_data->min < 0)
    blixem_data->min = 0;

  if (blixem_data->max >= blixem_data->window->feature_context->sequence_to_parent.c2)
    blixem_data->max = blixem_data->window->feature_context->sequence_to_parent.c2 - 1;

  /* start tells blixem to position itself slightly
   * to the left of the centre of the overall viewing window.
   * This is blindly copied from acedb. */
  start   = x1 - blixem_data->min - 30;

  /* qstart and qend coordinates passed to blixem in the exblx file
   * are relative to the ends of the viewing window (depending on
   * strand), so offset tells blixem where that starts relative to
   * the start of the sequence in question. Since this is an offset,
   * not a coord, and context->s_to_p.c1 is 1-based, while min is
   * 0-based, we add 1. */
  blixem_data->offset  = blixem_data->min - blixem_data->window->feature_context->sequence_to_parent.c1 + 1;
  if (blixem_data->offset < 0)
    blixem_data->offset = 0;

  if (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)  /* protein */
    {
      if (feature->strand == ZMAPSTRAND_REVERSE)
	blixem_data->opts = "X-BR";
      else
	blixem_data->opts = "X+BR";
    }
  else
    blixem_data->opts = "N+BR";                     /* dna */
      

  login = getlogin();
  if (login)
    {
      path = g_strdup_printf("/tmp/%s_ZMAP_BLIXEM/", login);
      /*                         zMapGetDir(path, home-relative, create) */
      if ((blixem_data->tmpDir = zMapGetDir(path, FALSE, TRUE)))
	{
	  /* g_mkstemp replaces the XXXXXX with unique string of chars */
	  tmpfile = g_strdup_printf("%sZMAPXXXXXX", blixem_data->tmpDir);
	  if ((file = g_mkstemp(tmpfile)))
	    {
	      blixem_data->fastAFile = g_strdup(tmpfile);
	      close(file);
	      file = 0;  /* reuse for exblx file */
	    }
	  else
	    {
	      zMapShowMsg(ZMAP_MSG_WARNING, 
			  "Error: could not open temporary fastA for Blixem");
	      status = FALSE;
	    }
	  g_free(tmpfile);
	}
      else
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, 
		      "Error: could not create temp directory for Blixem.");
	  status = FALSE;
	}

      g_free(path);
    }
  else
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not determine your login.");
      status = FALSE;
    }


  if (status == TRUE)             /* exblx file */
    {
      tmpfile = g_strdup_printf("%sZMAPXXXXXX", blixem_data->tmpDir);
      if ((file = g_mkstemp(tmpfile)))
	{
	  blixem_data->exblxFile = g_strdup(tmpfile);
	  close(file);
	}
      else
	{   
	  zMapShowMsg(ZMAP_MSG_WARNING, 
		      "Error: could not open temporary exblx for Blixem");
	  status = FALSE;
	}
      g_free(tmpfile);
    }

      
  if (status == TRUE)
   {
     paramString = g_strdup_printf("-P %s:%d -S %d -O %d -o %s %s %s",
				   blixem_data->Netid, 
				   blixem_data->Port,
				   start,
				   blixem_data->offset,
				   blixem_data->opts,
				   blixem_data->fastAFile,
				   blixem_data->exblxFile);
     /* debugging code     
     printf("paramString: %s\n", paramString);
     */
   }

  return paramString;
}




static gboolean writeExblxFile(blixemData blixem_data)
{
  gsize    bytes_written;
  GError  *channel_error = NULL;
  char    *filepath, *line;
  gboolean status = TRUE;
  ZMapFeature feature;


  if ((blixem_data->channel = g_io_channel_new_file(blixem_data->exblxFile, "w", &channel_error)))
    {
      line = g_strdup_printf("# exblx\n# blast%c\n", *blixem_data->opts);
      if (g_io_channel_write_chars(blixem_data->channel, 
				   line, -1, &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "Error writing headers to exblx file: %50s... : %s",
		      line, channel_error->message) ;
	  g_error_free(channel_error) ;
	  status = FALSE ;
	}
      else
	{
	  blixem_data->errorMsg = NULL;
	  feature = g_object_get_data(G_OBJECT(blixem_data->item), "item_feature_data");

	  if (blixem_data->oneType)
	    {
	      g_datalist_foreach(&feature->parent_set->features, writeExblxLine, blixem_data);
	    }
	  else
	    {
	      g_datalist_foreach(&feature->parent_set->parent_block->feature_sets, 
				 processFeatureSet, blixem_data);
	    }
	  /* if there are errors writing the data to the file, the first such
	  ** error will be recorded in blixem_data->errorMsg. */
	  if (blixem_data->errorMsg)
	    {
	      zMapShowMsg(ZMAP_MSG_WARNING, blixem_data->errorMsg);
	      status = FALSE;
	      g_free(blixem_data->errorMsg);
	    }
	}

      g_free(line);
      g_io_channel_shutdown(blixem_data->channel, TRUE, &channel_error);
      if (channel_error)
	g_free(channel_error);
    }
  else
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not open exblx file: %s",
		  channel_error->message) ;
      g_error_free(channel_error) ;
      status = FALSE;
    }

  return status;
}



static void processFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data;
  blixemData     blixem_data = (blixemData)user_data;

  g_datalist_foreach(&(feature_set->features), writeExblxLine, blixem_data);

  return;
}



static void writeExblxLine(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature     = (ZMapFeature)data;
  blixemData  blixem_data = (blixemData)user_data;
  int   min, max, x1, x2, scope = 40000;          /* scope set from user prefs */
  int score = 0, qstart, qend, sstart, send;
  ZMapSpan span = NULL;
  char *sname = NULL;
  char *qframe = NULL;
  GString *line;
  int i, j;
  char buf[255];
  gboolean status = TRUE;


  /* There is no way to interrupt g_datalist_foreach(), so instead,
  ** if printLine() encounters a problem, we store the error message
  ** in blixem_data and skip all further processing, displaying the
  ** error when we get back to writeExblxFile().  */

  if (!blixem_data->errorMsg)
    {  
      line = g_string_new("");

      /* visible coords are 1-based but internally we use 0-based coords */
      x1    = feature->x1 - 1;
      x2    = feature->x2 - 1;


      if ((x1 >= blixem_data->min && x1 <= blixem_data->max)
	  && (x2 >= blixem_data->min && x2 <= blixem_data->max)
	  && ((*blixem_data->opts == 'X' && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
	      || (*blixem_data->opts == 'N' && feature->feature.homol.type == ZMAPHOMOL_N_HOMOL))
	  )
	{
	  min   = blixem_data->min;
	  max   = blixem_data->max;
	  sstart = send = 0;
	  
	  /* qstart and qend are the coordinates of the current
	   * homology relative to the viewing window, rather 
	   * than absolute coordinages. */
	  if (feature->strand == ZMAPSTRAND_REVERSE)
	    {
	      qstart = x2 - min + 1;
	      qend   = x1 - min + 1;
	    }
	  else		
	    {
	      qstart = x1 - min + 1;
	      qend   = x2 - min + 1;
	    }
	  
	  switch (feature->type)
	    {
	    case ZMAPFEATURE_HOMOL:
	      
	      sname  = g_strdup(g_quark_to_string(feature->original_id));
	      
	      /* qframe is derived from the position of the start position
	       * of this homology relative to one end of the viewing window
	       * or the other, depending on strand.  Mod3 gives us the odd
	       * bases if that distance is not a whole number of codons. */
	      if (feature->strand == ZMAPSTRAND_REVERSE)
		{
		  qframe = g_strdup_printf("(-%d)", 1 + ((max - blixem_data->offset + 1) - qstart) %3 );
		  sstart = feature->feature.homol.y2;
		  send   = feature->feature.homol.y1;
		}
	      else
		{
		  qframe = g_strdup_printf("(+%d)", 1 + (qstart - 1) %3 );
		  sstart = feature->feature.homol.y1;
		  send   = feature->feature.homol.y2;
		}

	      adjustCoords(blixem_data, &sstart, &send, &qstart, &qend);

	      score = feature->feature.homol.score + 0.5;  /* round up to integer */
	      
	      if (score)
		{
		  g_string_printf(line, "%d %s\t%d\t%d\t%d\t%d %s ", 
				  score, qframe, qstart, qend, sstart, send, sname);
		  if (feature->feature.homol.align)
		    {
		      ZMapAlignBlockStruct gap;
		      GString *gaps = g_string_new("");
		      int k;
		      
		      for (k = 0; k < feature->feature.homol.align->len; k++)
			{
			  gap = g_array_index(feature->feature.homol.align, ZMapAlignBlockStruct, k);
			  g_string_printf(gaps, " %d %d %d %d", gap.q1, gap.q2, gap.t1 - min, gap.t2 - min);
			  line = g_string_append(line, gaps->str);
			}
		    }
		  
		  line = g_string_append(line, "\n");
		  
		  status = printLine(blixem_data, line->str);
		}
	      g_free(qframe);
	      g_free(sname);
	      break;       /* case HOMOL */
	      
	    case ZMAPFEATURE_TRANSCRIPT:

	      status = processExons(blixem_data, feature);
	      
	      if (status == TRUE && feature->feature.transcript.introns)
		{	      
		  for (i = 0; i < feature->feature.transcript.introns->len && status == TRUE; i++)
		    {
		      span = &g_array_index(feature->feature.transcript.introns, ZMapSpanStruct, i);
		      
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
		      
		      sstart = send = 0;
		      score  = -2;
		      sname  = g_strdup_printf("%si", g_quark_to_string(feature->original_id));
		      
		      if (feature->strand == ZMAPSTRAND_REVERSE)
			qframe = g_strdup("(-1)");
		      else
			qframe = g_strdup("(+1)");
		      
		      adjustCoords(blixem_data, &sstart, &send, &qstart, &qend);

		      g_string_printf(line, "%d %s\t%d\t%d\t%d\t%d\t%s\n", 
				      score, qframe, qstart, qend, sstart, send, sname);
		      g_free(qframe);
		      g_free(sname);

		      status = printLine(blixem_data, line->str);

		    }        /* for .. introns .. */
		}          /* if (status == TRUE) */
	      
	      break;     /* case TRANSCRIPT */
	      
	    default:
	      break;
	    }            /* switch (feature->type)   */
	  
	}                /* if within the min-max range */
      g_string_free(line, TRUE);
    }                    /* if (!blixem_data->errorMsg) */
  return;
}




static gboolean printLine(blixemData blixem_data, char *line)
{
  GError *channel_error = NULL;
  gsize bytes_written;
  gboolean status = TRUE;

  if (g_io_channel_write_chars(blixem_data->channel, line, -1, 
			       &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
    {
      /* don't display an error at this point, just write it to blixem_data for later.
      ** Set and return status to prevent further processing. */
      blixem_data->errorMsg = g_strdup_printf("Error writing data to exblx file: %50s... : %s",
					      line, channel_error->message) ;
      g_error_free(channel_error) ;
      status = FALSE;
    }
    
  return status;
}


/* Write out the dna sequence into a file in fasta format, ie a header record with the 
 * sequence name, then as many lines 50 characters long as it takes to write out the 
 * complete sequence.  
 * Note that what we want is the sequence from the beginning of the "viewable window",
 * ie from min onwards.  This is given by blixem_data->offset because this is relative
 * to the start of the clone, whereas min is an absolute coordinate. */
static gboolean writeFastAFile(blixemData blixem_data)
{
  enum { FASTA_CHARS = 50 };
  gsize    bytes_written;
  GError  *channel_error = NULL;
  char    *line;
  gboolean status = TRUE;
  char     buffer[FASTA_CHARS + 2] ;			    /* FASTA CHARS + \n + \0 */
  int      lines = 0, chars_left = 0 ;
  char    *cp = NULL ;
  int      i ;


  if ((blixem_data->channel = g_io_channel_new_file(blixem_data->fastAFile, "w", &channel_error)))
    {
      line = g_strdup_printf(">%s\n", 
			     g_quark_to_string(blixem_data->window->feature_context->parent_name));
      if (g_io_channel_write_chars(blixem_data->channel, 
				   line, -1, &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "Error writing header record to fastA file: %50s... : %s",
		      line, channel_error->message) ;
	  g_error_free(channel_error) ;
	  g_free(line);
	  status = FALSE ;
	}
      else
	{
	  g_free(line);

	  lines      = (blixem_data->window->feature_context->length - blixem_data->offset) / FASTA_CHARS ;
	  chars_left = (blixem_data->window->feature_context->length - blixem_data->offset) % FASTA_CHARS ;
	  if (blixem_data->min > 0)
	    cp = blixem_data->window->feature_context->sequence.sequence + blixem_data->offset;
	  else
	    cp = blixem_data->window->feature_context->sequence.sequence;

	  /* Do the full length lines.                                           */
	  if (lines > 0)
	    {
	      buffer[FASTA_CHARS] = '\n' ;
	      buffer[FASTA_CHARS + 1] = '\0' ; 
	      for (i = 0 ; i < lines && status == TRUE; i++)
		{
		  memcpy(&buffer[0], cp, FASTA_CHARS) ;
		  cp += FASTA_CHARS ;
		  if (g_io_channel_write_chars(blixem_data->channel,
					       &buffer[0], -1, 
					       &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
		    status = FALSE ;
		}
	    }

	  /* Do the last line.                                                   */
	  if (chars_left > 0 && status == TRUE)
	    {
	      memcpy(&buffer[0], cp, chars_left) ;
	      buffer[chars_left] = '\n' ;
	      buffer[chars_left + 1] = '\0' ; 
	      if (g_io_channel_write_chars(blixem_data->channel,
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
      g_io_channel_shutdown(blixem_data->channel, TRUE, &channel_error);
      if (channel_error)
	g_error_free(channel_error);
    }
  else           /* if (blixem_data->channel = ... */
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not open fastA file: %s",
		  channel_error->message) ;
      g_error_free(channel_error) ;
      status = FALSE;
    }

  return status;
}



static gboolean processExons(blixemData blixem_data, ZMapFeature feature)
{
  int i, j, next_base_no = 0, tLen = 0;
  ZMapSpan span = NULL, jspan = NULL;
  int score = -1, qstart, qend, sstart, send, prev_sstart;
  int min, max;
  char *sname, *qframe;
  GString *line = g_string_new("");
  GArray *noDups = NULL;
  gboolean skip = FALSE;
  gboolean status = TRUE;


  /* Sometimes exons are present in the GFF dump more than once.   
  ** This loop copies unique exons to a new array and for reversed
  ** features accumulates the number of bases in the transcript. */
  for (i = 0; i < feature->feature.transcript.exons->len; i++)
    {
      span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);
      
      skip = FALSE;
      if (!noDups)
	{
	  noDups = g_array_new(FALSE, TRUE, sizeof(ZMapSpanStruct));
	  g_array_append_val(noDups, *span);
	  /* With reversed features, sstart and send need to start at the
	  ** far end and work backwards. Must avoid duplicate exons, too. 
	  ** Determine No of bases in transcript */
	  if (feature->strand == ZMAPSTRAND_REVERSE)
	    next_base_no += span->x2 - span->x1 + 1;
	}
      else 
	{
	  for (j = 0; j < noDups->len && skip == FALSE; j++)
	    {
	      jspan = &g_array_index(noDups, ZMapSpanStruct, j);
	      if (jspan->x1 == span->x1 && jspan->x2 == span->x2)
		  skip = TRUE;
	    }
	  if (skip == TRUE)
	    continue;
	  else
	    {
 	      g_array_append_val(noDups, *span);    

	      /* With reversed features, sstart and send need to start at the
	      ** far end and work backwards. Must avoid duplicate exons, too. 
	      ** Determine No of bases in transcript */
	      if (feature->strand == ZMAPSTRAND_REVERSE)
		next_base_no += span->x2 - span->x1 + 1;
	    }
	}
    }      


  /* now we just process the unique exons from noDups array */
  for (i = 0; i < noDups->len && status == TRUE; i++)
    {
      min = blixem_data->min;
      max = blixem_data->max;

      span = &g_array_index(noDups, ZMapSpanStruct, i);

      /* We need to keep track of the number of the first base of each exon
      ** within the transcript, with introns snipped out, so next_base_no is
      ** the base number we're expecting the next exon to start at. */

      if (feature->strand == ZMAPSTRAND_REVERSE)
	{
	  qstart = span->x2 - min + 1;
	  qend   = span->x1 - min + 1;

	  send   = next_base_no/3;
	  sstart = ((next_base_no - (span->x2 - span->x1))/3) + 1;

	  next_base_no -= (span->x2 - span->x1 + 1);
 	  qframe = g_strdup_printf("(-%d)", 
				   (1 + ((max - min + 1) - qstart - next_base_no) % 3));
	}
      else
	{
	  qstart = span->x1 - min + 1;
	  qend   = span->x2 - min + 1;
	  qframe = g_strdup_printf("(+%d)", ((qstart - next_base_no - 1) % 3) + 1);

	  sstart = (next_base_no + 3)/3;
	  send   = (next_base_no + span->x2 - span->x1)/3;
	  next_base_no += (span->x2 - span->x1);
	}

      adjustCoords(blixem_data, &sstart, &send, &qstart, &qend);

      sname  = g_strdup_printf("%sx", g_quark_to_string(feature->original_id));
      
      g_string_printf(line, "%d %s\t%d\t%d\t%d\t%d %s\n", 
		      score, qframe, qstart, qend, sstart, send, sname);
      g_free(sname);
      g_free(qframe);
	  
      status = printLine(blixem_data, line->str);

    }       /* for .. exons .. */
  
  g_string_free(line, TRUE);

  if (noDups)
    g_array_free(noDups, TRUE);
  
  return status;
}



static void freeBlixemData(blixemData blixem_data)
{
  g_free(blixem_data->tmpDir);
  g_free(blixem_data->fastAFile);
  g_free(blixem_data->exblxFile);

  g_free(blixem_data->Netid);
  g_free(blixem_data->Script);

  return;
}


static void adjustCoords(blixemData blixem_data, 
			 int *sstart, int *send, 
			 int *qstart, int *qend)
{
  ZMapFeature feature;
  int len = blixem_data->max - blixem_data->min;;


  feature = g_object_get_data(G_OBJECT(blixem_data->item), "item_feature_data");
  
  if (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)  /* protein */
    {
      if (*qstart < 1)
	{
	  *sstart += (3 - *qstart) / 3 ;
	  *qstart += 3 * ((3 - *qstart) / 3) ;
	}
      if (*qend > len + 1)
	{
	  *send -= (*qend + 2 - len) / 3 ;
	  *qend -= 3 * ((*qend + 2 - len) / 3) ;
	}
    }
  else				/* DNA */
    { if (*qstart < 1)
	{ *sstart += 1 - *qstart ;
	  *qstart  = 1 ;
	}
      if (*qend > len + 1)
	{ *send -= len + 1 - *qend ;
	  *qend  = len + 1 ;
	}
    }

  return;
}


/*************************** end of file *********************************/
