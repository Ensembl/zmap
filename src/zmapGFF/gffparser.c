/*  File: gffparser.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <glib.h>
#include <stdio.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGFF.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapConfigIni.h>


typedef struct
{
  ZMapGFFParser parser;
  GIOChannel   *file;
  GString      *gff_line;
} parserFileStruct, *parserFile ;



static int parseFile(char *filename, char *sequence, GHashTable *styles) ;
static GIOChannel *openFileOrDie(char *filename) ;
static int readHeader(parserFile data) ;
static gboolean readFeatures(parserFile data) ;


int main(int argc, char *argv[])
{
  int main_rc ;
  GHashTable *styles = NULL;

#if !defined G_THREADS_ENABLED || defined G_THREADS_IMPL_NONE || !defined G_THREADS_IMPL_POSIX
#error "Cannot compile, threads not properly enabled."
#endif

  g_thread_init(NULL) ;
  if (!g_thread_supported())
    g_thread_init(NULL);

  zMapConfigDirCreate(NULL, NULL) ;

  zMapLogCreate(NULL) ;

  zMapConfigIniGetStylesFromFile(argv[3], argv[4], &styles);     //  styles  = zMapFeatureTypeGetFromFile(argv[2], argv[3]) ;
  main_rc = parseFile(argv[1], argv[2], styles) ;

  zMapLogDestroy() ;

  return(main_rc) ;
}





/*
 *                  Internal routines.
 */


static int parseFile(char *filename, char *sequence, GHashTable *styles)
{
  parserFileStruct data = {NULL};

  data.file     = openFileOrDie(filename);

  data.parser   = zMapGFFCreateParser(sequence, 0, 0) ;

  zMapGFFParserInitForFeatures(data.parser, styles, FALSE) ;
  data.gff_line = g_string_sized_new(2000);

  if(!(readHeader(&data)))
     readFeatures(&data);

  zMapGFFDestroyParser(data.parser) ;
  g_string_free(data.gff_line, TRUE) ;

  return 0 ;
}


static GIOChannel *openFileOrDie(char *filename)
{
  GIOChannel *fp = NULL;
  GError *error = NULL;
  if((fp = g_io_channel_new_file(filename, "r", &error)))
    printf("Opened file %s\n", filename);
  else
    {
      printf("Failed to open %s\n", filename);
      exit(1);
    }
  return fp;
}

static int readHeader(parserFile data)
{
  ZMapGFFHeader header ;
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  GError *gff_file_err = NULL ;
  int error_occurred = 0;

  while ((status = g_io_channel_read_line_string(data->file, data->gff_line,
						 &terminator_pos,
						 &gff_file_err)) == G_IO_STATUS_NORMAL)
    {
      gboolean done_header = FALSE ;
      gboolean header_ok = FALSE ;

      *(data->gff_line->str + terminator_pos) = '\0';
// MH17 -> there seems to be a stutter in this source code!
      if(!zMapGFFParseHeader(data->parser, data->gff_line->str, &done_header,&header_ok))
        {
          if (!zMapGFFParseHeader(data->parser, data->gff_line->str, &done_header,&header_ok))
            {
              GError *error = zMapGFFGetError(data->parser) ;

              if (!error && (header = zMapGFFGetHeader(data->parser)))
                {
                  /* Header finished..ugh poor interface.... */
                  break ;
                }

              if (!error)
                {
                  printf("error %d\n", __LINE__);
                  error_occurred = 1;
                }
              else
                {
                  /* If the error was serious we stop processing and return the error,
                   * otherwise we just log the error. */
                  if (zMapGFFTerminated(data->parser))
                    {
                      printf("error %d\n", __LINE__);
                      error_occurred = 1;
                    }
                  else
                    {
                      printf("error %d\n", __LINE__);
                      error_occurred = 1;
                    }
                }

              break ;
            }
          data->gff_line = g_string_truncate(data->gff_line, 0) ;
          /* Reset line to empty. */
        }
    }

  zMapGFFFreeHeader(header) ;

  return error_occurred;
}

static gboolean readFeatures(parserFile data)
{
  gboolean result = FALSE ;
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  gboolean free_on_destroy = FALSE;
  GError *gff_file_err = NULL ;
  ZMapGFFParser parser = NULL;
  GIOChannel *gff_file = NULL;
  GString    *gff_line = NULL;

  parser = data->parser;
  gff_file = data->file;
  gff_line = data->gff_line;

  while ((status = g_io_channel_read_line_string(gff_file, gff_line, &terminator_pos,
						 &gff_file_err)) == G_IO_STATUS_NORMAL)
    {
      *(gff_line->str + terminator_pos) = '\0' ;	    /* Remove terminating newline. */

      if (!zMapGFFParseLine(parser, gff_line->str))
	{
	  GError *error = zMapGFFGetError(parser) ;

	  if (!error)
	    {
              printf("zMapGFFParseLine() failed with no GError for line %d: %s",
                     zMapGFFGetLineNumber(parser), gff_line->str) ;
	      result = FALSE ;
	    }
	  else
	    {
	      /* If the error was serious we stop processing and return the error,
	       * otherwise we just log the error. */
	      if (zMapGFFTerminated(parser))
		{
		  result = FALSE ;
		  printf("%s", error->message) ;
		}
	      else
		{
                  /* Ignore these for this debugging.
		  printf("%s", error->message) ;
                  */
		}
	    }
	}

      gff_line = g_string_truncate(gff_line, 0) ;   /* Reset line to empty. */
    }


  /* If we reached the end of the file then all is fine, so return features... */
  free_on_destroy = TRUE ;
  if (status == G_IO_STATUS_EOF)
    {
#ifdef RDS_DONT_INCLUDE
      if (zMapGFFGetFeatures(parser, feature_block))
	{
	  free_on_destroy = FALSE ;			    /* Make sure parser does _not_ free
							       our data ! */

	  result = TRUE ;
	}
#endif
      result = TRUE;
    }
  else
    {
      printf("Could not read GFF features from file \n");

      result = FALSE ;
    }

  zMapGFFSetFreeOnDestroy(parser, free_on_destroy) ;

  return result;
}




