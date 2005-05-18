/*  File: zmapWindowCallExternal.c
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
 * Description: calls an external program
 *
 *              
 * Exported functions: 
 * HISTORY:
 * Last edited: May 18 13:21 2005 (rnc)
 * Created: Tue May  9 14:30 2005 (rnc)
 * CVS info:   $Id: zmapWindowCallExternal.c,v 1.1 2005-05-18 12:38:32 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <unistd.h>           /* for getlogin() */
#include <glib.h>
#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>


typedef struct BlixemDataStruct 
{
  int   min, max;
  char *tmpDir;
  char *fastaFile;
  char *exblxFile;
  char *opts;
  ZMapWindow window;
  FooCanvasItem *item;
  gboolean oneType;
  GIOChannel *channel;
  char *errorMsg;
} blixemDataStruct, *blixemData;



static char *buildParamString (blixemData blixem_data);
static int   writeExblxFile   (blixemData blixem_data);
static void  processFeatureSet(GQuark key_id, gpointer data, gpointer user_data);
static void  writeExblxLine   (GQuark key_id, gpointer data, gpointer user_data);


void zmapWindowCallExternal(ZMapWindow window, FooCanvasItem *item, gboolean oneType)
{
  char *commandString;
  char *paramString;
  blixemDataStruct blixem_data = {NULL};

  if (!window->blixemScript)
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not get blixem parameters from config file.");
  else
    {
      blixem_data.window  = window;
      blixem_data.item    = item;
      blixem_data.oneType = oneType;

      if (paramString = buildParamString(&blixem_data))
	{
	  if (writeExblxFile(&blixem_data))
	    {
	      commandString = g_strdup_printf("%s %s", window->blixemScript, paramString);
	      printf("%s\n", commandString);
	      system(commandString);
	  
	      g_free(commandString);
	    }
	    
	  g_free(paramString);
	}
    }

  return;
}


static char *buildParamString(blixemData blixem_data)
{
  ZMapFeature feature;
  char *paramString = NULL;
  char *buf, *buf2;
  char *login;
  char *tmpfile;
  char *pfetchParams;
  int   start;
  int   offset;
  int   len;
  int   scope = 40000;          /* set from user prefs */
  int   status = TRUE;


  /* values from user prefs (ie config file) */
  if (blixem_data->window->blixemScope > 0)
    scope = blixem_data->window->blixemScope;

  pfetchParams = g_strdup_printf("%s:%d", blixem_data->window->blixemNetid, 
				 blixem_data->window->blixemPort);
  if (!pfetchParams)
    {
      zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not get pfetch parameters from config file.");
      status = FALSE;
    }
  /* end of user prefs */
  else
    {
      feature = g_object_get_data(G_OBJECT(blixem_data->item), "feature");
      len     = blixem_data->window->feature_context->length;
      blixem_data->min = feature->x1 - scope/2;
      blixem_data->max = feature->x2 + scope/2;
      if (blixem_data->max > len)
	blixem_data->max = len;
      start   = feature->x1 - blixem_data->min - 30;
      offset  = blixem_data->min - 1;


      if (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)  /* protein */
	{
	  if (feature->strand == ZMAPSTRAND_REVERSE)
	    blixem_data->opts = g_strdup("X-BR");
	  else
	    blixem_data->opts = g_strdup("X+BR");
	}
      else
	blixem_data->opts = g_strdup("N+BR");                     /* dna */
      
      if (status)
	{	
	  if (login = g_strdup(getlogin()))
	    {
	      buf = g_strdup_printf("/tmp/%s_ZMAP_BLIXEM/", login);
	      g_free(login);

	      if (blixem_data->tmpDir = zMapGetDir(buf, FALSE, TRUE))
		{
		  char *tmpfile;

		  /* NB if env var TMPDIR is defined, g_mkstemp() will use that & break this */
		  tmpfile = g_strdup_printf("%sZMAPXXXXXX", blixem_data->tmpDir);
		  if (g_mkstemp(tmpfile))
		    {
		      /* blixem wants only the unique filename, not the path */
		      blixem_data->fastaFile = g_strdup(g_strrstr(tmpfile, "/")+1);

		      sprintf(tmpfile, "%sZMAPXXXXXX", blixem_data->tmpDir);
		      if (g_mkstemp(tmpfile))
			{
			  blixem_data->exblxFile = g_strdup(g_strrstr(tmpfile, "/")+1);

			  /* temporary hack: copy existing fasta file across */
			  buf2 = g_strdup_printf("cp ~/ZMAPfasta %s%s", buf, blixem_data->fastaFile);
			  system(buf2);
			  g_free(buf2);
			  /* end temporary hack */
			  
			  paramString = g_strdup_printf("-P %s -S %d -O %d -o %s %s%s %s%s",
							pfetchParams, 
							start,
							offset,
							blixem_data->opts,
							blixem_data->tmpDir,
							blixem_data->fastaFile,
							blixem_data->tmpDir,
							blixem_data->exblxFile);
			  g_free(pfetchParams);
			  printf("paramString: %s\n", paramString);
			}
		      else     /* if (g_mkstmp(ZMAP... */
			zMapShowMsg(ZMAP_MSG_WARNING, 
				    "Error: could not open temporary exblx for Blixem");
		    }
		  else      /* if (g_mkstmp(ZMAP.... */
		    zMapShowMsg(ZMAP_MSG_WARNING, 
				"Error: could not open temporary fasta for Blixem");

		}       /* if (blixem_data->tmpDir... */
	      else
		zMapShowMsg(ZMAP_MSG_WARNING, 
			    "Error: could not create temp directory for Blixem.");
		  
	      if (buf)
		g_free(buf);
	    }        /* if (login) */
	  else
	    zMapShowMsg(ZMAP_MSG_WARNING, "Error: could not determine your login.");
	}        /* if (status) */
    }     /* if (!pfetchParams) */

  return paramString;
}


static int writeExblxFile(blixemData blixem_data)
{
  gsize    bytes_written;
  GError  *channel_error = NULL;
  char    *filepath, *line;
  gboolean status = TRUE;

  filepath = g_build_path("/", blixem_data->tmpDir, blixem_data->exblxFile, NULL);

  if (blixem_data->channel = g_io_channel_new_file(filepath, "w", &channel_error))
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
	  blixem_data->errorMsg = '\0';
	  if (blixem_data->oneType)
	    {
	      ZMapFeatureSet feature_set;
	      
	      feature_set = g_object_get_data(G_OBJECT(blixem_data->item), "feature_set");
	      g_datalist_foreach(&(feature_set->features), writeExblxLine, blixem_data);
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
      printf("closing %s\n", blixem_data->exblxFile);
      g_io_channel_shutdown(blixem_data->channel, TRUE, &channel_error);
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
  blixemData     blixem_data  = (blixemData)user_data;

  g_datalist_foreach(&(feature_set->features), writeExblxLine, blixem_data);

  return;
}

static void writeExblxLine(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature     = (ZMapFeature)data;
  blixemData  blixem_data = (blixemData)user_data;
  int   len, min, x1, x2, status = 1, scope = 40000;          /* set from user prefs */
  int score, qstart, qend, sstart, send;
  char *sname, *sseq;
  char *line, *qframe;
  ulong bytes_written;
  GError *channel_error = NULL;


  if ((*blixem_data->opts == 'N' && feature->feature.homol.type == ZMAPHOMOL_N_HOMOL)
      || (*blixem_data->opts == 'X' && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL))
    {
      if (feature->type == ZMAPFEATURE_HOMOL 
	  && (feature->x1 >= blixem_data->min && feature->x1 <= blixem_data->max)
	  && (feature->x1 >= blixem_data->min && feature->x2 <= blixem_data->max))
	{
	  len   = blixem_data->window->feature_context->length;
	  x1    = feature->x1;
	  x2    = feature->x2;
	  min   = blixem_data->min;
	  score = feature->feature.homol.score;
	  
	  if (score)
	    {
	      if (feature->strand == ZMAPSTRAND_REVERSE)
		{
		  qstart = x2 - min + 1;
		  qend   = x1 - min + 1;
		  qframe = g_strdup_printf("(-%d)", 1 + (qstart - 1)%3);
		}
	      else
		{
		  qstart = x1 - min + 1;
		  qend   = x2 - min + 1;
		  qframe = g_strdup_printf("(+%d)", 1 + (qstart - 1)%3);
		}
	      
	      sstart = feature->feature.homol.y1;
	      send   = feature->feature.homol.y2;
	      
	      sname = g_strdup(g_quark_to_string(feature->original_id));
	      /*  sseq  = 
		  
	      no no no, this is all much too simplistic.  Take another look at zmapfeatures.c
	      doShowAlign() which does all sorts of stuff depending. I think I won't have the
	      peptide sequence available for the sseq column until Ed's provided it.
	      */
	      line = g_strdup_printf("%d %s\t%d\t%d\t%d\t%d\t%s", 
				     score, qframe, qstart, qend, sstart, send, sname);
	      
	      g_free(sname);
	      g_free(qframe);
	      
	      if (feature->feature.homol.align)
		{
		  int i;
		  for (i = 0; i < feature->feature.homol.align->len; i++)
		    {
		      line = g_strdup_printf("%s %d %d %d %d", line, 
					     (g_array_index(feature->feature.homol.align, ZMapAlignBlock, i))->q1,
					     (g_array_index(feature->feature.homol.align, ZMapAlignBlock, i))->q2,
					     (g_array_index(feature->feature.homol.align, ZMapAlignBlock, i))->t1,
					     (g_array_index(feature->feature.homol.align, ZMapAlignBlock, i))->t2);
		    }
	    }
	      line = g_strdup_printf("%s\n", line);
	      
	      if (g_io_channel_write_chars(blixem_data->channel, line, -1, &bytes_written, &channel_error) != G_IO_STATUS_NORMAL)
		{
		  /* don't display an error at this point, just write it to blixem_data for later */
		  if (!blixem_data->errorMsg)
		    blixem_data->errorMsg = g_strdup_printf("Error writing data to exblx file: %50s... : %s",
							line, channel_error->message) ;
		  g_error_free(channel_error) ;
		}
	      g_free(line);
	    }
	}
    }

  return;
}

/*************************** end of file *********************************/
