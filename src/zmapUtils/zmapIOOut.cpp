/*  File: zmapIOOut.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Supports output to strings or files (sockets, files etc.)
 *
 * Exported functions: See ZMap/zmapIO.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>






#include <string.h>
#include <ZMap/zmapUtils.hpp>
#include <zmapIO_P.hpp>


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
    out->output.string_value = g_string_new_len(init_value, size) ;
  else
    out->output.string_value = g_string_new(init_value) ;

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

  /* zMapAssert(zMapOutValid(out)) ; */
  if (!zMapOutValid(out))
    return str ;

  if (out->type != ZMAPIO_STRING)
    zMapLogWarning("%s", "zMapOutGetStr() failed, zMapIOOut does not contain a string.") ;
  else
    str = out->output.string_value->str ;

  return str ;
}


gboolean zMapOutWrite(ZMapIOOut out, char *text)
{
  gboolean result = FALSE ;

  /* zMapAssert(zMapOutValid(out) && text && *text) ;*/
  if (!zMapOutValid(out) || !text || !*text)
    return result ;

  switch (out->type)
    {
    case ZMAPIO_STRING:
      {
        out->output.string_value = g_string_append(out->output.string_value, text) ;

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
        zMapWarnIfReached() ;
        break ;
      }
    }

  return result ;
}



gboolean zMapOutWriteFormat(ZMapIOOut out, const char *format, ...)
{
  gboolean result = FALSE ;
  va_list args ;
  char *msg_string ;

  /* zMapAssert(zMapOutValid(out) && format && *format) ;*/
  if (!zMapOutValid(out) || !format || !*format)
    return result ;

  switch (out->type)
    {
    case ZMAPIO_STRING:
      {

        va_start(args, format) ;
        msg_string = g_strdup_vprintf(format, args) ;
        va_end(args) ;

        out->output.string_value = g_string_append(out->output.string_value, msg_string) ;

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
        zMapWarnIfReached() ;
        break ;
      }
    }


  return result ;
}




char *zMapOutErrorGetString(ZMapIOOut out)
{
  char *err_text = NULL ;

  /* zMapAssert(zMapOutValid(out)) ;*/
  if (!zMapOutValid(out))
    return err_text ;

  if (out->g_error)
    {
      err_text = out->g_error->message ;
    }

  return err_text ;
}


void zMapOutErrorClear(ZMapIOOut out)
{
  /* zMapAssert(zMapOutValid(out)) ; */
  if (!zMapOutValid(out))
    return ;

  if (out->g_error)
    g_clear_error(&(out->g_error)) ;

  return ;
}


/* destroy one... */
void zMapOutDestroy(ZMapIOOut out)
{
  /* zMapAssert(zMapOutValid(out)) ;*/
  if (!zMapOutValid(out))
    return ;

  switch (out->type)
    {
    case ZMAPIO_STRING:
      {
        g_string_free(out->output.string_value, TRUE) ;
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
        zMapWarnIfReached() ;
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

  out = (ZMapIOOut)g_slice_alloc0(sizeof(ZMapIOOutStruct)) ;
  out->valid = IO_valid_str_G ;
  out->type = type ;

  return out ;
}


static void deAllocIOOut(ZMapIOOut out)
{
  out->type = ZMAPIO_INVALID ;
  out->valid = NULL ;    /* Mark block as dirty in case reused. */
  g_slice_free1(sizeof(ZMapIOOutStruct), out) ;

  return ;
}
