/*  File: zmapUtilsSpawn.c
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>





#if 0

commented out by malcolm 2009-11-30 14:52:12
these functions appear as is in libpfetch/libfpetch.c and are not called from anywhere

#include <zmapUtils_P.h>


static gboolean fd_to_GIOChannel_with_watch(gint fd, GIOCondition cond, GIOFunc func, 
					    gpointer data, GDestroyNotify destroy);

/*!
 * Wrapper around g_spawn_async_with_pipes() to aid with the logic
 * contained in making that call. In the process of making the call
 * the function converts requested File descriptors to non-blocking
 * GIOChannels adding the corresponding GIOFunc to the channel with
 * g_io_add_watch.
 *
 * Passing NULL for any of the GIOFunc parameters means the rules
 * g_spawn_async_with_pipes() uses apply as NULL will be passed in
 * place there.
 *
 * N.B. passing a GChildWatchFunc is untested... I waited for 
 * G_IO_HUP instead.
 *
 * @param **argv        Argument vector
 * @param stdin_writer  GIOFunc to handle writing to the pipe.
 * @param stdout_reader GIOFunc to handle reading from the pipe.
 * @param stderr_reader GIOFunc to handle reading from the pipe.
 * @param pipe_data     user data to pass to the GIOFunc
 * @param pipe_data_destroy GDestroyNotify for pipe_data
 * @param error         GError location to return into 
 *                       (from g_spawn_async_with_pipes)
 * @param child_func    A GChildWatchFunc for the spawned PID 
 * @param child_func_data data for the child_func
 * @return              gboolean from g_spawn_async_with_pipes()
 *
 *  */
gboolean zMapUtilsSpawnAsyncWithPipes(char *argv[], GIOFunc stdin_writer, GIOFunc stdout_reader, GIOFunc stderr_reader, 
				      gpointer pipe_data, GDestroyNotify pipe_data_destroy, GError **error,
				      GChildWatchFunc child_func, ZMapUtilsChildWatchData child_func_data)
{
  char *cwd = NULL, **envp = NULL;
  GSpawnChildSetupFunc child_setup = NULL;
  gpointer child_data = NULL;
  GPid child_pid;
  gint *stdin_ptr = NULL,
    *stdout_ptr = NULL,
    *stderr_ptr = NULL,
    stdin_fd,
    stdout_fd, 
    stderr_fd;
  GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
  gboolean result = FALSE;
  GError *our_error = NULL;	/* In case someone decides to live on the edge! */

  if(stdin_writer)
    stdin_ptr = &stdin_fd;
  if(stdout_reader)
    stdout_ptr = &stdout_fd;
  if(stderr_reader)
    stderr_ptr = &stderr_fd;

  if(child_func)
    flags |= G_SPAWN_DO_NOT_REAP_CHILD;

  if((result = g_spawn_async_with_pipes(cwd, argv, envp,
					flags, child_setup, child_data, 
					&child_pid, stdin_ptr,
					stdout_ptr, stderr_ptr, 
					&our_error)))
    {
      GIOCondition read_cond  = G_IO_IN  | G_IO_HUP | G_IO_ERR | G_IO_NVAL;
      GIOCondition write_cond = G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL;

      if(child_pid && child_func && child_func_data)
	child_func_data->source_id = g_child_watch_add(child_pid, child_func, child_func_data);

      /* looks good so far... Set up the readers and writer... */
      if(stderr_ptr && !fd_to_GIOChannel_with_watch(*stderr_ptr, read_cond, stderr_reader, pipe_data, pipe_data_destroy))
	zMapLogWarning("%s", "Failed to add watch to stderr");
      if(stdout_ptr && !fd_to_GIOChannel_with_watch(*stdout_ptr, read_cond, stdout_reader, pipe_data, pipe_data_destroy))
	zMapLogWarning("%s", "Failed to add watch to stdout");
      if(stdin_ptr && !fd_to_GIOChannel_with_watch(*stdin_ptr, write_cond, stdin_writer, pipe_data, pipe_data_destroy))
	zMapLogWarning("%s", "Failed to add watch to stdin");
    }
  else if(!error)      /* We'll Log the error */
    zMapLogWarning("%s", our_error->message);

  if(error)
    *error = our_error;

  return result;
}

#ifdef EXAMPLE_GIOFUNC
#define READ_SIZE 80		/* roughly average line length */
static gboolean example_stdout_io_func(GIOChannel *source, GIOCondition cond, gpointer user_data)
{
  gboolean call_again = FALSE;

  if(cond & G_IO_IN)		/* there is data to be read */
    {
      GIOStatus status;
      gchar text[READ_SIZE] = {0};
      gsize actual_read = 0;
      GError *error = NULL;

      if((status = g_io_channel_read_chars(source, &text[0], READ_SIZE, 
					   &actual_read, &error)) == G_IO_STATUS_NORMAL)
	{
	  /* Do some processing here... */
	  call_again = TRUE;
	}	  
      else
	{
	  switch(error->code)
	    {
	      /* Derived from errno */
	    case G_IO_CHANNEL_ERROR_FBIG:
	    case G_IO_CHANNEL_ERROR_INVAL:
	    case G_IO_CHANNEL_ERROR_IO:
	    case G_IO_CHANNEL_ERROR_ISDIR:
	    case G_IO_CHANNEL_ERROR_NOSPC:
	    case G_IO_CHANNEL_ERROR_NXIO:
	    case G_IO_CHANNEL_ERROR_OVERFLOW:
	    case G_IO_CHANNEL_ERROR_PIPE:
	      break;
	      /* Other */
	    case G_IO_CHANNEL_ERROR_FAILED:
	    default:
	      break;
	    }
	  zMapShowMsg(ZMAP_MSG_WARNING, "Error reading from pipe: %s", ((error->message) ? error->message : "UNKNOWN ERROR"));
	}
    }
  else if(cond & G_IO_HUP)	/* hung up, connection broken */
    zMapShowMsg(ZMAP_MSG_INFORMATION, "%s", "G_IO_HUP");
  else if(cond & G_IO_ERR)	/* error condition */
    zMapShowMsg(ZMAP_MSG_INFORMATION, "%s", "G_IO_ERR");
  else if(cond & G_IO_NVAL)	/* invalid request, file descriptor not open */
    zMapShowMsg(ZMAP_MSG_INFORMATION, "%s", "G_IO_NVAL");      
  else
    zMapAssertNotReached();

  return call_again;
}
#endif


/* INTERNALS */

static gboolean fd_to_GIOChannel_with_watch(gint fd, GIOCondition cond, GIOFunc func, 
					    gpointer data, GDestroyNotify destroy)
{
  gboolean success = FALSE;
  GIOChannel *io_channel;

  if((io_channel = g_io_channel_unix_new(fd)))
    {
      GError *flags_error = NULL;
      GIOStatus status;

      if((status = g_io_channel_set_flags(io_channel, 
					  (G_IO_FLAG_NONBLOCK | g_io_channel_get_flags(io_channel)), 
					  &flags_error)) == G_IO_STATUS_NORMAL)
	{
	  //g_io_channel_set_encoding(io_channel, "ISO8859-1", NULL);
	  g_io_add_watch_full(io_channel, G_PRIORITY_DEFAULT, cond, func, data, destroy);

	  success = TRUE;
	}
      else
	zMapLogCritical("%s", flags_error->message);

      g_io_channel_unref(io_channel); /* We don't need this anymore */
    }

  return success;
}

#endif
