/*  File: zmapIOOut.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Supports output to strings or files (sockets, files etc.)
 *
 * Exported functions: See ZMap/zmapIO.h
 * HISTORY:
 * Last edited: Sep  2 16:05 2008 (rds)
 * Created: Mon Oct 15 13:50:02 2007 (edgrif)
 * CVS info:   $Id: zmapIOOut.c,v 1.4 2010-06-14 15:40:14 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapIO_P.h>


static ZMapIOOut allocIOOut(ZMapIOType type) ;
static void deAllocIOOut(ZMapIOOut out) ;


ZMAP_MAGIC_NEW(IO_valid_str_G, ZMapIOOutStruct) ;

GQuark zmapOutErrorDomain(void)
{
  return g_quark_from_static_string("zmap-out");
}

gboolean zMapOutValid(ZMapIOOut out)
{
  gboolean result = FALSE ;

  if (out && ZMAP_MAGIC_IS_VALID(IO_valid_str_G, out->valid) && out->type != ZMAPIO_INVALID)
    result = TRUE ;

  return result ;
}




/* Make one... */
ZMapIOOut zMapOutCreateStr(char *init_value, int size)
{
  ZMapIOOut out = NULL ;

  out = allocIOOut(ZMAPIO_STRING) ;

  if (size > 0)
    out->output.string = g_string_new_len(init_value, size) ;
  else
    out->output.string = g_string_new(init_value) ;

  return out ;
}


ZMapIOOut zMapOutCreateFD(int fd)
{
  ZMapIOOut out = NULL ;
  GIOChannel *channel ;

  if ((channel = g_io_channel_unix_new(fd)))
    {
      out = allocIOOut(ZMAPIO_FILE) ;

      out->output.channel = channel ;
    }

  return out ;
}

ZMapIOOut zMapOutCreateFile(char *full_path, char *mode, GError **error_out)
{
  ZMapIOOut out = NULL;
  GIOChannel *channel;
  GError *error = NULL;

  if((channel = g_io_channel_new_file(full_path, mode, &error)) && (!error))
    {
      out = allocIOOut(ZMAPIO_FILE);

      out->output.channel = channel;
    }
  else if(error)
    {
      if(error_out)
	*error_out = error;
    }
  else
    {
      if(error_out)
	*error_out = g_error_new(zmapOutErrorDomain(),
				 -1,
				 "Failed to open file '%s', mode '%s'. "
				 "No error returned from g_io_channel_new_file!",
				 full_path, mode);
    }
  
  return out;
}

char *zMapOutGetStr(ZMapIOOut out)
{
  char *str = NULL ;

  zMapAssert(zMapOutValid(out)) ;

  if (out->type != ZMAPIO_STRING)
    zMapLogWarning("%s", "zMapOutGetStr() failed, zMapIOOut does not contain a string.") ;
  else
    str = out->output.string->str ;

  return str ;
}


gboolean zMapOutWrite(ZMapIOOut out, char *text)
{
  gboolean result = FALSE ;

  zMapAssert(zMapOutValid(out) && text && *text) ;

  switch (out->type)
    {
    case ZMAPIO_STRING:
      {
	out->output.string = g_string_append(out->output.string, text) ;

	result = TRUE ;
	break ;
      }
    case ZMAPIO_FILE:
      {
	GIOStatus status ;
	gsize bytes_written ;

	if ((status = g_io_channel_write_chars(out->output.channel,
					       text, -1, &bytes_written,
					       &(out->g_error))) == G_IO_STATUS_NORMAL)
	  result = TRUE ;
	break ;
      }
    default:
      {
	zMapAssertNotReached() ;
	break ;
      }
    }

  return result ;
}



gboolean zMapOutWriteFormat(ZMapIOOut out, char *format, ...)
{
  gboolean result = FALSE ;
  va_list args ;
  char *msg_string ;

  zMapAssert(zMapOutValid(out) && format && *format) ;

  switch (out->type)
    {
    case ZMAPIO_STRING:
      {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	/* I would like to simply use this but it requires glib 2.14 */
	g_string_append_vprintf(out->output.string, format, args) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	va_start(args, format) ;
	msg_string = g_strdup_vprintf(format, args) ;
	va_end(args) ;

	out->output.string = g_string_append(out->output.string, msg_string) ;

	g_free(msg_string) ;
	result = TRUE ;
	break ;
      }
    case ZMAPIO_FILE:
      {
	GIOStatus status ;
	gsize bytes_written ;
	char *msg_string ;

	va_start(args, format) ;
	msg_string = g_strdup_vprintf(format, args) ;
	va_end(args) ;

	if ((status = g_io_channel_write_chars(out->output.channel,
					       msg_string, -1, &bytes_written,
					       &(out->g_error))) == G_IO_STATUS_NORMAL)
	  result = TRUE ;

	g_free(msg_string) ;
	break ;
      }
    default:
      {
	zMapAssertNotReached() ;
	break ;
      }
    }


  return result ;
}




char *zMapOutErrorGetString(ZMapIOOut out)
{
  char *err_text = NULL ;

  zMapAssert(zMapOutValid(out)) ;

  if (out->g_error)
    {
      err_text = out->g_error->message ;
    }

  return err_text ;
}


void zMapOutErrorClear(ZMapIOOut out)
{
  zMapAssert(zMapOutValid(out)) ;

  if (out->g_error)
    g_clear_error(&(out->g_error)) ;

  return ;
}


/* destroy one... */
void zMapOutDestroy(ZMapIOOut out)
{
  zMapAssert(zMapOutValid(out)) ;

  switch (out->type)
    {
    case ZMAPIO_STRING:
      {
	g_string_free(out->output.string, TRUE) ;
	break ;
      }
    case ZMAPIO_FILE:
      {
	/* We use unref here instead of shutdown because shutdown _always_ closes the file
	 * and we don't always want this, e.g. if file is stdout ! unref will close disk
	 * files but leave open everything else (sockets, pipes etc) which is what we want. */
	g_io_channel_unref(out->output.channel) ;

	/* Free any g_error stuff. */
	if (out->g_error)
	  g_clear_error(&(out->g_error)) ;

	break ;
      }
    default:
      {
	zMapAssertNotReached() ;
	break ;
      }
    }

  deAllocIOOut(out) ;

  return ;
}



/* 
 *                Internal functions 
 */


static ZMapIOOut allocIOOut(ZMapIOType type)
{
  ZMapIOOut out ;

  out = g_slice_alloc0(sizeof(ZMapIOOutStruct)) ;
  out->valid = IO_valid_str_G ;
  out->type = type ;

  return out ;
}


static void deAllocIOOut(ZMapIOOut out)
{
  out->type = ZMAPIO_INVALID ;
  out->valid = NULL ;					    /* Mark block as dirty in case reused. */
  g_slice_free1(sizeof(ZMapIOOutStruct), out) ;

  return ;
}
