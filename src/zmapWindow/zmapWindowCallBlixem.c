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
 * Last edited: May 27 15:50 2005 (rnc)
 * Created: Tue May  9 14:30 2005 (rnc)
 * CVS info:   $Id: zmapWindowCallBlixem.c,v 1.2 2005-05-27 15:00:26 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <unistd.h>           /* for getlogin() */
#include <glib.h>
#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfig.h>

#define ZMAP_BLIXEM_CONFIG "blixem"


typedef struct BlixemDataStruct 
{
  int            min, max;
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
static void     freeBlixemData   (blixemData blixem_data);



void zmapWindowCallBlixem(ZMapWindow window, FooCanvasItem *item, gboolean oneType)
{
  gboolean         status;
  char            *commandString;
  char            *paramString;
  blixemDataStruct blixem_data = {0};
  

  status = getUserPrefs(&blixem_data);

  if (status == TRUE)
    {
      blixem_data.window  = window;
      blixem_data.item    = item;
      blixem_data.oneType = oneType;

      if (!(paramString = buildParamString(&blixem_data)))
	status = FALSE;	 
    } 

  if (status == TRUE)
    {
      if (writeExblxFile(&blixem_data) == TRUE
	  && writeFastAFile(&blixem_data) == TRUE)
	{
	  commandString = g_strdup_printf("%s %s &", blixem_data.Script, paramString);
	  printf("%s\n", commandString);
	  /* Note that since blixem is being called as a background process, 
	  ** we can only tell whether or not the system call was successful,
	  ** and not whether or not blixem ran successfully. */
	  if (system(commandString) != 0)
	    zMapShowMsg(ZMAP_MSG_WARNING, "System call failed: blixem not invoked.") ;
	  
	  g_free(commandString);
	}
      
      g_free(paramString);
    }

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
      g_free(stanza_name);

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

  char       *pfetchParams;
  int         start;
  int         offset;
  int         len;
  int         scope = 40000;          /* set from user prefs */


  if (blixem_data->Scope > 0)
    scope = blixem_data->Scope;

  pfetchParams = g_strdup_printf("%s:%d", blixem_data->Netid, 
				 blixem_data->Port);
  feature = g_object_get_data(G_OBJECT(blixem_data->item), "feature");

  len     = blixem_data->window->feature_context->length;

  blixem_data->min = feature->x1 - scope/2;
  blixem_data->max = feature->x2 + scope/2;
  if (blixem_data->min < 0)
    blixem_data->min = 0;
  if (blixem_data->max > len)
    blixem_data->max = len;

  start   = feature->x1 - blixem_data->min - 30;
  offset  = blixem_data->min ? blixem_data->min - 1 : 0;


  if (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)  /* protein */
    {
      if (feature->strand == ZMAPSTRAND_REVERSE)
	blixem_data->opts = g_strdup("X-BR");
      else
	blixem_data->opts = g_strdup("X+BR");
    }
  else
    blixem_data->opts = g_strdup("N+BR");                     /* dna */
      

  if ((login = g_strdup(getlogin())))
    {
      path = g_strdup_printf("/tmp/%s_ZMAP_BLIXEM/", login);
      g_free(login);
    } 
  else
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not determine your login.");
      status = FALSE;
    }


  if (status == TRUE)
    {	  
      /*                         zMapGetDir(path, home-relative, create) */
      if ((blixem_data->tmpDir = zMapGetDir(path, FALSE, TRUE)))
	tmpfile = g_strdup_printf("%sZMAPXXXXXX", blixem_data->tmpDir);
      else
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, 
		      "Error: could not create temp directory for Blixem.");
	  status = FALSE;
	}

      g_free(path);
    }


  if (status == TRUE)            /* fastA file */
    {
      if ((file = g_mkstemp(tmpfile)))
	{
	  /* blixem wants only the unique filename, not the path */
	  blixem_data->fastAFile = g_strdup(g_strrstr(tmpfile, "/")+1);
	  
	  sprintf(tmpfile, "%sZMAPXXXXXX", blixem_data->tmpDir);
	  close(file);
	  file = 0;
	}
      else
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, 
		      "Error: could not open temporary fastA for Blixem");
	  status = FALSE;
	}
    }

  if (status == TRUE)             /* exblx file */
    {
      if ((file = g_mkstemp(tmpfile)))
	{
	  blixem_data->exblxFile = g_strdup(g_strrstr(tmpfile, "/")+1);
	  close(file);
	}
      else
	{   
	  zMapShowMsg(ZMAP_MSG_WARNING, 
		      "Error: could not open temporary exblx for Blixem");
	  status = FALSE;
	}
    }

      
  if (status == TRUE)
   {
     paramString = g_strdup_printf("-P %s -S %d -O %d -o %s %s%s %s%s",
				   pfetchParams, 
				   start,
				   offset,
				   blixem_data->opts,
				   blixem_data->tmpDir,
				   blixem_data->fastAFile,
				   blixem_data->tmpDir,
				   blixem_data->exblxFile);
     
     printf("paramString: %s\n", paramString);
   }

  if (tmpfile)
    g_free(tmpfile);
  
  g_free(pfetchParams);
 
  return paramString;
}




static gboolean writeExblxFile(blixemData blixem_data)
{
  gsize    bytes_written;
  GError  *channel_error = NULL;
  char    *filepath, *line;
  gboolean status = TRUE;

  filepath = g_build_path("/", blixem_data->tmpDir, blixem_data->exblxFile, NULL);

  if ((blixem_data->channel = g_io_channel_new_file(filepath, "w", &channel_error)))
    {
      line = g_strdup_printf("# exblx\n# blast%c\n", *blixem_data->opts);
      if (g_io_channel_write_chars(blixem_data->channel, 
				   line, -1, &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "Error writing headers to exblx file: %50s... : %s",
		      line, channel_error->message) ;
	  g_error_free(channel_error) ;
	  g_free(line);
	  status = FALSE ;
	}
      else
	{
	  g_free(line);
	  blixem_data->errorMsg = NULL;
	  if (blixem_data->oneType)
	    {
	      GData *feature_set;
	      ZMapFeature feature;
	      
	      feature = g_object_get_data(G_OBJECT(blixem_data->item), "feature");
	      feature_set = zMapFeatureFindSetInContext(blixem_data->window->feature_context, feature->style);
	      g_datalist_foreach(&feature_set, writeExblxLine, blixem_data);
	    }
	  else
	    {
	      g_datalist_foreach(&(blixem_data->window->feature_context->feature_sets), 
				 processFeatureSet, blixem_data);
	    }
	  /* if there are errors writing the data to the file, the first such
	  ** error will be recorded in blixem_data->errorMsg. */
	  if (blixem_data->errorMsg)
	    {
	      zMapShowMsg(ZMAP_MSG_WARNING, blixem_data->errorMsg);
	      status = FALSE;
	    }
	}
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

  g_free(filepath);

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
  int   len, min, max, x1, x2, scope = 40000;          /* scope set from user prefs */
  int score = 0, qstart, qend, sstart, send;
  char *sname = NULL, *sseq = NULL;
  char *qframe = NULL;
  GString *line;
  ZMapSpan span = NULL, jspan = NULL;
  int i, j;
  GArray *noDups = NULL;
  char buf[255];
  gboolean skip = FALSE;
  gboolean status = TRUE;

  /*
  sprintf(buf, "debug: %s    %d    %d type %d\n", 
	  g_quark_to_string(feature->original_id),
	  feature->x1, feature->x2,
	  feature->type);
  printLine(blixem_data, buf);
  */

  /* There is no way to interrupt g_datalist_foreach(), so instead,
  ** if printLine() encounters a problem, we store the error message
  ** in blixem_data and skip all further processing, displaying the
  ** error when we get back to writeExblxFile().  */

  line = g_string_new("");
  len   = blixem_data->window->feature_context->length;
  x1    = feature->x1;
  x2    = feature->x2;
  
  if ((x1 >= blixem_data->min && x1 <= blixem_data->max)
      && (x1 >= blixem_data->min && x2 <= blixem_data->max))
    {
      min   = blixem_data->min;
      max   = blixem_data->max;
      sstart = send = 0;
      
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
	  
	  if (feature->strand == ZMAPSTRAND_REVERSE)
	    {
	      qframe = g_strdup_printf("(-%d)", 1 + ((max - min + 1) - qstart) %3 );
	      sstart = feature->feature.homol.y2;
	      send   = feature->feature.homol.y1;
	    }
	  else
	    {
	      qframe = g_strdup_printf("(+%d)", 1 + (qstart - 1) %3 );
	      sstart = feature->feature.homol.y1;
	      send   = feature->feature.homol.y2;
	    }
	  
	  score = feature->feature.homol.score;
	  /*  sseq  =  */ 
	  
	  printf("Homol: %s %d %d\n", sname, qstart, feature->strand);
	  
	  if (score)
	    {
	      g_string_printf(line, "%d %s\t%d\t%d\t%d\t%d\t%s", 
			      score, qframe, qstart, qend, sstart, send, sname);
	      if (feature->feature.homol.align)
		{
		  int i;
		  GString *gaps = g_string_new("");
		  
		  for (i = 0; i < feature->feature.homol.align->len; i++)
		    {
		      g_string_printf(gaps, " %d %d %d %d", 
				      (g_array_index(feature->feature.homol.align, ZMapAlignBlock, i))->q1,
				      (g_array_index(feature->feature.homol.align, ZMapAlignBlock, i))->q2,
				      (g_array_index(feature->feature.homol.align, ZMapAlignBlock, i))->t1,
				      (g_array_index(feature->feature.homol.align, ZMapAlignBlock, i))->t2);
		      line = g_string_append(line, gaps->str);
		    }
		  g_string_free(gaps, TRUE);
		}
	      
	      line = g_string_append(line, "\n");
	      
	      status = printLine(blixem_data, line->str);
	    }
	  
	  break;       /* case HOMOL */
	  
	case ZMAPFEATURE_TRANSCRIPT:
	  
	  for (i = 0; i < feature->feature.transcript.exons->len; i++)
	    {
	      span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);
	      
	      /* Sometimes exons are present in the GFF dump more than once.   
	      ** This block of code detects duplicates and skips them.  If
	      ** giface is changed to skip duplicates this bit becomes 
	      ** redundant, but should do no harm. */
	      skip = FALSE;
	      if (!noDups)
		{
		  noDups = g_array_new(FALSE, TRUE, sizeof(ZMapSpanStruct));
		  g_array_append_val(noDups, *span);
		}
	      else 
		{
		  for (j = 0; j < noDups->len; j++)
		    {
		      jspan = &g_array_index(noDups, ZMapSpanStruct, j);
		      if (jspan->x1 == span->x1 && jspan->x2 == span->x2)
			{
			  skip = TRUE;
			  j = noDups->len;
			}
		    }
		  if (skip == TRUE)
		    continue;
		  else
		    g_array_append_val(noDups, *span);    
		}
	      
	      if (feature->strand == ZMAPSTRAND_REVERSE)
		{
		  qstart = span->x2 - min + 1;
		  qend   = span->x1 - min + 1;
		  qframe = g_strdup("(-99)");
		}
	      else
		{
		  qstart = span->x1 - min + 1;
		  qend   = span->x2 - min + 1;
		  qframe = g_strdup("(+99)");
		}
	      
	      sstart = 99999;
	      send   = 99999;
	      score  = -1;
	      sname  = g_strdup_printf("%sx", g_quark_to_string(feature->original_id));
	      printf("Transcript: %s %d %d\n", sname, qstart, feature->strand);
	      if (score)
		{
		  g_string_printf(line, "%d %s\t%d\t%d\t%d\t%d %s\n", 
				  score, qframe, qstart, qend, sstart, send, sname);
		  
		  status = printLine(blixem_data, line->str);
		}
	    }       /* for .. exons .. */
	  
	  if (noDups)
	    g_array_free(noDups, TRUE);
	  if (jspan)
	    g_free(jspan);
	  
	  if (status == TRUE)
	    {	      
	      for (i = 0; i < feature->feature.transcript.introns->len; i++)
		{
		  span = &g_array_index(feature->feature.transcript.introns, ZMapSpanStruct, i);
		  
		  if (feature->strand == ZMAPSTRAND_REVERSE)
		    {
		      qstart = span->x2 - min + 1;
		      qend   = span->x1 - min + 1;
		      qframe = g_strdup_printf("(-%d)", 1 + (qstart - 1)%3);
		    }
		  else
		    {
		      qstart = span->x1 - min + 1;
		      qend   = span->x2 - min + 1;
		      qframe = g_strdup_printf("(+%d)", 1 + (qstart - 1)%3);
		    }
		  
		  sstart = send = 0;
		  score  = -2;
		  sname  = g_strdup_printf("%si", g_quark_to_string(feature->original_id));
		  
		  if (feature->strand == ZMAPSTRAND_REVERSE)
		    qframe = g_strdup("(+1)");
		  else
		    qframe = g_strdup("(-1)");
		  
		  if (score)
		    {
		      g_string_printf(line, "%d %s\t%d\t%d\t%d\t%d\t%s\n", 
				      score, qframe, qstart, qend, sstart, send, sname);
		      
		      status = printLine(blixem_data, line->str);
		    }
		}        /* for .. introns .. */
	    }          /* if (status == TRUE) */
	  
	  break;     /* case TRANSCRIPT */
	  
	default:
	  sname = g_strdup(g_quark_to_string(feature->original_id));
	  printf("Other %s type %d\n", sname, feature->type);
	  break;
	}            /* switch (feature->type)   */
      
      if (sname)
	g_free(sname);
      if (sseq)
	g_free(sseq);
      if (qframe)
	g_free(qframe);
    }                /* if within the min-max range */
      
  g_string_free(line, TRUE);
  
  return;
}



static gboolean printLine(blixemData blixem_data, char *line)
{
  GError *channel_error = NULL;
  ulong bytes_written;
  gboolean status = TRUE;

  if (blixem_data->errorMsg == NULL)  /* redundant: should not get here with errorMsg set */
    {
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
    }
  return status;
}



static gboolean writeFastAFile(blixemData blixem_data)
{
  enum { FASTA_CHARS = 50 };
  gsize    bytes_written;
  GError  *channel_error = NULL;
  char    *filepath, *line;
  gboolean status = TRUE;
  char     buffer[FASTA_CHARS + 2] ;			    /* FASTA CHARS + \n + \0 */
  int      lines = 0, chars_left = 0 ;
  char    *cp = NULL ;
  int i ;

  filepath = g_build_path("/", blixem_data->tmpDir, blixem_data->fastAFile, NULL);

  if ((blixem_data->channel = g_io_channel_new_file(filepath, "w", &channel_error)))
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
	  /* this code lifted straight from acedb */
	  lines      = (blixem_data->window->feature_context->sequence.length - blixem_data->min - 1) / FASTA_CHARS ;
	  chars_left = (blixem_data->window->feature_context->sequence.length - blixem_data->min - 1) % FASTA_CHARS ;
	  if (blixem_data->min > 0)
	    cp = blixem_data->window->feature_context->sequence.sequence + blixem_data->min - 1;
	  else
	    cp = blixem_data->window->feature_context->sequence.sequence;

	  /* Do the full length lines.                                           */
	  if (lines != 0)
	    {
	      buffer[FASTA_CHARS] = '\n' ;
	      buffer[FASTA_CHARS + 1] = '\0' ; 
	      for (i = 0 ; i < lines && status ; i++)
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
	  if (chars_left != 0 && status == TRUE)
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
	      printf("%d left; buffer: %s\n", chars_left, buffer);
	      g_error_free(channel_error) ;
	    }
	}
      g_io_channel_shutdown(blixem_data->channel, TRUE, &channel_error);
      if (channel_error)
	g_error_free(channel_error);
    }
  else
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not open fastA file: %s",
		  channel_error->message) ;
      g_error_free(channel_error) ;
      status = FALSE;
    }

  g_free(filepath);

  return status;
}




static void freeBlixemData(blixemData blixem_data)
{
  g_free(blixem_data->tmpDir);
  g_free(blixem_data->fastAFile);
  g_free(blixem_data->exblxFile);
  g_free(blixem_data->opts);
  g_free(blixem_data->errorMsg);
  g_free(blixem_data->Netid);
  g_free(blixem_data->Script);

  return;
}

/*************************** end of file *********************************/
