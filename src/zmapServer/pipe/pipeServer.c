/*  File: pipeServer.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *      derived from fileServer.c by Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: These functions provide code to read the output of a script
 *		as though it were a server according to the interface defined
 *              for accessing servers. The aim is to allow ZMap to request
 *		arbritary data from external sources as defined in the config files.
 *
 * NB:	As for the fileServer module the data is read as a single pass and then discarded.
 *
 * Exported functions: See ZMap/zmapServerPrototype.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <sys/types.h>  /* for waitpid() */
#include <sys/wait.h>
#include <string.h>

#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapGFF.h>
#include <ZMap/zmapServerProtocol.h>
#include <zmapServerPrototype.h>
#include <pipeServer_P.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>


#ifdef MH_NOW_IN_PIPESERVER_STRUCT
/* Used to control getting features in all alignments in all blocks... */
typedef struct
{
  ZMapServerResponseType result ;
  PipeServer server ;
  ZMapGFFParser parser
  GString * gff_line ;
} GetFeaturesStruct, *GetFeatures ;
#endif


static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 char *config_file, ZMapURL url, char *format,
                                 char *version_str, int timeout) ;

static gboolean pipe_server_spawn(PipeServer server,GError **error);
static ZMapServerResponseType openConnection(void *server, ZMapServerReqOpen req_open) ;
static ZMapServerResponseType getInfo(void *server, ZMapServerInfo info) ;
static ZMapServerResponseType getFeatureSetNames(void *server,
						 GList **feature_sets_out,
						 GList *sources,
						 GList **required_styles,
						 GHashTable **featureset_2_stylelist_inout,
						 GHashTable **featureset_2_column_out,
						 GHashTable **source_2_sourcedata_out) ;
static ZMapServerResponseType getStyles(void *server, GHashTable **styles_out) ;
static ZMapServerResponseType haveModes(void *server, gboolean *have_mode) ;
static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout) ;
static ZMapServerResponseType setContext(void *server,  ZMapFeatureContext feature_context) ;
static ZMapServerResponseType getFeatures(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context_out) ;
static ZMapServerResponseType getContextSequence(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context_out) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType getStatus(void *server_conn, gint *exit_code, gchar **stderr_out);
static ZMapServerResponseType getConnectState(void *server_conn, ZMapServerConnectStateType *connect_state) ;
static ZMapServerResponseType closeConnection(void *server_in) ;
static ZMapServerResponseType destroyConnection(void *server) ;

static void addMapping(ZMapFeatureContext feature_context, int req_start, int req_end) ;
static void eachAlignment(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlock(gpointer key, gpointer data, gpointer user_data) ;
static gboolean sequenceRequest(PipeServer server, ZMapGFFParser parser, GString* gff_line,
				ZMapFeatureBlock feature_block) ;
static void setLastErrorMsg(PipeServer server, GError **gff_pipe_err_inout) ;

static gboolean getServerInfo(PipeServer server, ZMapServerInfo info) ;
static void setErrMsg(PipeServer server, char *new_msg) ;

static ZMapServerResponseType pipeGetHeader(PipeServer server);
static ZMapServerResponseType pipeGetSequence(PipeServer server);



/* Compulsory routine, without this we can't do anything....returns a list of functions
 * that can be used to contact a pipe server. The functions are only visible through
 * this struct. */
void pipeGetServerFuncs(ZMapServerFuncs pipe_funcs)
{
  pipe_funcs->global_init = globalInit ;
  pipe_funcs->create = createConnection ;
  pipe_funcs->open = openConnection ;
  pipe_funcs->get_info = getInfo ;
  pipe_funcs->feature_set_names = getFeatureSetNames ;
  pipe_funcs->get_styles = getStyles ;
  pipe_funcs->have_modes = haveModes ;
  pipe_funcs->get_sequence = getSequences ;
  pipe_funcs->set_context = setContext ;
  pipe_funcs->get_features = getFeatures ;
  pipe_funcs->get_context_sequences = getContextSequence ;
  pipe_funcs->errmsg = lastErrorMsg ;
  pipe_funcs->get_status = getStatus;
  pipe_funcs->get_connect_state = getConnectState ;
  pipe_funcs->close = closeConnection;
  pipe_funcs->destroy = destroyConnection ;

  return ;
}


/*
 *    Although these routines are static they form the external interface to the pipe server.
 */


/* For stuff that just needs to be done once at the beginning..... */
static gboolean globalInit(void)
{
  gboolean result = TRUE ;

  return result ;
}



/* Read ZMap application defaults. */
static void getConfiguration(PipeServer server)
{
  ZMapConfigIniContext context;

  if((context = zMapConfigIniContextProvide(server->config_file)))
    {
      char *tmp_string  = NULL;

#if MH17_REMOVED_BY_POPULAR_REQUEST
      /* default script directory to use */
      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
				       ZMAPSTANZA_APP_SCRIPTS, &tmp_string))
	{
	  server->script_dir = tmp_string;
	}
      else
#endif
      {
        server->script_dir = g_get_current_dir();
      }

       /* default directory to use */
      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                               ZMAPSTANZA_APP_DATA, &tmp_string))
      {
        server->data_dir = tmp_string;
      }
      else
      {
        server->data_dir = g_get_current_dir();
      }

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                               ZMAPSTANZA_APP_CSVER, &tmp_string))
	{
		if(!g_ascii_strcasecmp(tmp_string,"Otter"))
			server->is_otter = TRUE;
	}


      zMapConfigIniContextDestroy(context);
    }

  return ;
}



/* spawn a new process to run our script and open a pipe for read to get the output
 * global config of the default scripts directory is via '[ZMap] scripts-dir=there'
 * to get absolute path you need 4 slashes eg for pipe://<host>/<path>
 * where <host> is null and <path> begins with a /
 * For now we will assume that "host" contains the script name and then we just ignore the other
 * parameters....
 *
 *  */
static gboolean createConnection(void **server_out,
				 char *config_file, ZMapURL url, char *format,
                                 char *version_str, int timeout_unused)
{
  gboolean result = TRUE ;
  PipeServer server ;
  gchar *dir;

  zMapAssert(url->path) ;

  /* Always return a server struct as it contains error message stuff. */
  server = (PipeServer)g_new0(PipeServerStruct, 1) ;
  *server_out = (void *)server ;

  server->config_file = g_strdup(config_file) ;

  getConfiguration(server);	// get scripts directory

  server->scheme = url->scheme;

#if !SCRIPT_DIR
  server->script_path = zMapGetPath(url->path) ;	/* if absolute */

  if(server->scheme == SCHEME_FILE)
    {
      char *tmp_path;

      getConfiguration(server);	// get data directory for files
      dir = server->data_dir;

      if (*(url->path) != '/' && dir && *dir)
	{
	  tmp_path = g_strdup_printf("%s/%s", dir,url->path) ;		/* NB '//' works as '/' */
	  server->script_path = zMapGetPath(tmp_path) ;
	  g_free(tmp_path) ;
	}
    }
#if !USING_SPAWN_SERACH_PATH_FLAG
  else if(server->scheme == SCHEME_PIPE && *(url->path) != '/')	/* find the script on our path env */
    {
      extern char ** environ;
      char **env = environ;	/* C global */
      while(*env)
	{
	  char *p = *env++;

	  if(!g_ascii_strncasecmp(p,"PATH=",5))	/* there's no case sensitive version ! */
	    {
	      char ** paths = g_strsplit(p+5,":",0);
	      if(paths)
		{
		  char ** pathp = paths;

		  while(*pathp)
		    {
		      dir =  g_strdup_printf("%s/%s",*pathp,url->path);
		      if(g_file_test(dir,G_FILE_TEST_IS_EXECUTABLE))
			{
			  server->script_path = zMapGetPath(dir) ;
			  g_free(dir);

			  break;
			}
		      pathp++;
		    }

		  g_strfreev(paths);
		}
	    }
	}
    }
#endif
#else
  dir = (server->scheme == SCHEME_PIPE) ? server->script_dir : server->data_dir;
  /* We could check both format and version here if we wanted to..... */


  /* This code is a hack to get round the fact that the url code we use in zmapView.c to parse
   * urls from our config file will chop the leading "/" off the path....which causes the
   * zMapGetPath() call below to construct an incorrect path.... */
  {
    char *tmp_path = url->path ;

    if (*(url->path) != '/' && dir && *dir)
      tmp_path = g_strdup_printf("%s/%s", dir,url->path) ;

    server->script_path = zMapGetPath(tmp_path) ;

    if (tmp_path != url->path)
      g_free(tmp_path) ;
  }

#endif

  if(url->query)
    server->query = g_strdup_printf("%s",url->query);
  else
    server->query = g_strdup("");

  server->protocol = PIPE_PROTOCOL_STR;
  if(server->scheme == SCHEME_FILE)
    server->protocol = FILE_PROTOCOL_STR;

  server->zmap_start = 1;
  server->zmap_end = 0; // default to all of it

  return result ;
}




/*
 * fork and exec the script and read the output via a pipe
 * no data sent to STDIN and STDERR ignored
 * in case of errors or hangups eventually we will time out and an error popped up.
 * downside is limited to not having the data, which is what happens anyway
 */

typedef struct pipe_arg
{
      char *arg;
      char type;
#define PA_INT    1
#define PA_STRING   2
/*
 * this runs in a thread and can be accessed by several at once
 * must use auto variables for state
 */
      char flag;
} pipeArgStruct, *pipeArg;


#define PA_START  1
#define PA_END  2
#define PA_DATASET  4
#define PA_SEQUENCE  8

pipeArgStruct otter_args[] =
{
      { "start", PA_INT,PA_START },
      { "end", PA_INT,PA_END },
      { "dataset", PA_STRING,PA_DATASET },
      { "gff_seqname", PA_STRING,PA_SEQUENCE },
      { NULL, 0, 0 }
};

pipeArgStruct zmap_args[] =
{
      { "start", PA_INT,PA_START },
      { "end", PA_INT,PA_END },
//      { "dataset", PA_STRING,PA_DATASET },	may need when mapping available
      { "gff_seqname", PA_STRING,PA_SEQUENCE },
      { NULL, 0, 0 }
};

#define PIPE_MAX_ARGS   8    // extra args we add on to the query, including the program and terminating NULL

static char *make_arg(pipeArg pipe_arg, char *prefix,PipeServer server)
{
      char *q = NULL;

      switch(pipe_arg->flag)
      {
      case PA_START:
            if(server->zmap_start && server->zmap_end)
                  q = g_strdup_printf("%s%s=%d",prefix,pipe_arg->arg,server->zmap_start);
            break;
      case PA_END:
            if(server->zmap_start && server->zmap_end)
                  q = g_strdup_printf("%s%s=%d",prefix,pipe_arg->arg,server->zmap_end);
            break;
      case PA_DATASET:
            /* if NULL will not work but won't crash either */
            if(server->sequence_map->dataset)
                  q = g_strdup_printf("%s%s=%s",prefix,pipe_arg->arg,server->sequence_map->dataset);
            break;
      case PA_SEQUENCE:
            if(server->sequence_map->sequence)
                  q = g_strdup_printf("%s%s=%s",prefix,pipe_arg->arg,server->sequence_map->sequence);
            break;
      }
      return(q);
}

static gboolean pipe_server_spawn(PipeServer server,GError **error)
{
  gboolean result = FALSE;
  gchar **argv, **q_args;
  gint pipe_fd;
  GError *pipe_error = NULL;
  int i;
  gint err_fd;
  pipeArg pipe_arg;
  char arg_done = 0;
  char *mm = "--";
  char *minus = mm+2;   /* this gets set as per the first arg */
  char *p;
  int n_q_args = 0;

  q_args = g_strsplit(server->query,"&",0);
  if(q_args && *q_args)
    {
      p = q_args[0];
      minus = mm+2;
      if(*p == '-')     /* optional -- allow single as well */
	{
	  p++;
	  minus--;
	}
      if(*p == '-')
	{
	  minus--;
	}
      n_q_args = g_strv_length(q_args);
    }
  argv = (gchar **) g_malloc (sizeof(gchar *) * (PIPE_MAX_ARGS + n_q_args));
  argv[0] = server->script_path; // scripts can get exec'd, as long as they start w/ #!


  for(i = 1;q_args && q_args[i-1];i++)
    {
      /* if we request from the mark we have to adjust the start/end coordinates
       * these are supplied in the url and really should be configured explicitly
       * ... now they are in [ZMap].  We now work with chromosome coords.
       */

      char *q;
      p = q_args[i-1] + strlen(minus);

      pipe_arg = server->is_otter ? otter_args : zmap_args;
      for(; pipe_arg->type; pipe_arg++)
	{
	  if(!g_ascii_strncasecmp(p,pipe_arg->arg,strlen(pipe_arg->arg)))
            {
	      arg_done |= pipe_arg->flag;
	      q = make_arg(pipe_arg,minus,server);
	      if(q)
		{
		  g_free(q_args[i-1]);
		  q_args[i-1] = q;
		}

            }
	}

      argv[i] = q_args[i-1];
    }

  /* add on if not defined already */
  pipe_arg = server->is_otter ? otter_args : zmap_args;
  for(; pipe_arg->type; pipe_arg++)
    {
      if(!(arg_done & pipe_arg->flag))
	{
	  char *q;
	  q = make_arg(pipe_arg,minus,server);
	  if(q)
	    argv[i++] = q;

	}
    }

  //#define MH17_DEBUG_ARGS

  argv[i]= NULL;
#if defined  MH17_DEBUG_ARGS
  {
    char *x = "";
    int j;

    for(j = 0;argv[j] ;j++)
      {
	x = g_strconcat(x," ",argv[j],NULL);
      }
    zMapLogWarning("pipe server args: %s (%d,%d)",x,server->zmap_start,server->zmap_end);
  }
#endif


  /* Seems that g_spawn_async_with_pipes() is not thread safe so lock round it. */
  zMapThreadForkLock();

  result = g_spawn_async_with_pipes(server->script_dir,argv,NULL,
				    //      G_SPAWN_SEARCH_PATH |	 doesnt work
				    G_SPAWN_DO_NOT_REAP_CHILD, // can't not give a flag!
				    NULL, NULL, &server->child_pid, NULL, &pipe_fd, &err_fd, &pipe_error) ;
  if(result)
    {
      server->gff_pipe = g_io_channel_unix_new(pipe_fd);
      server->gff_stderr = g_io_channel_unix_new(err_fd);
      g_io_channel_set_flags(server->gff_stderr,G_IO_FLAG_NONBLOCK,&pipe_error);
    }
  else
    {
      /* didn't run so can't have clean exit code; */
      server->exit_code = 0x2ff;	/* ref w/ pipe_server_get_stderr() */
    }

  zMapThreadForkUnlock();



  g_free(argv);   // strings allocated and freed seperately
  if(q_args)
    g_strfreev(q_args);


  if(error)
    *error = pipe_error;
  return(result);
}


/*
 * read stderr from the external source and if non empty display and log some messages
 * use non-blocking i/o so we don't hang ???
 * gets called by setErrMsg() - if the server fails we read STDERR and possibly report why
 * if no failures we ignore STDERR
 * the last message is the one that gets popped up to the user
 */

// NB: otterlace would prefer to get all of STDERR, which needs a slight rethink

/* we read this on any explicit error and at the end of reading features (end of GFF)
 * This assumes we always do this and also that the script exits in a timely manner
 * we also get the exit code at this point as this is when we decide a script has finished
 */
void pipe_server_get_stderr(PipeServer server)
{
  GError *gff_pipe_err = NULL;
  //  GError *ignore = NULL;
#if !MH17_SINGLE_LINE
  int status;
  gsize length;

  if(server->child_pid)
    {
      g_io_channel_shutdown(server->gff_pipe,FALSE,NULL);	/* or else it may not exit */
      server->gff_pipe = NULL;	/* can't be 0 as we have that for stdin */
      /* thst didn't work ! */

      //	kill(server->child_pid,9);
      waitpid(server->child_pid,&status,0);
      server->child_pid = 0;
      if(WIFEXITED(status))
	{
	  server->exit_code =  WEXITSTATUS(status);  /* 8 bits only */
	}
      else
	{
	  server->exit_code = 0x1ff;     /* abnormal exit */
	}
    }
  /* if we are running a file:// then there's no gff_stderr */
  if(!server->gff_stderr || server->stderr_output)    /* only try once */
    return;

  /* MH17: NOTE this is a Linux/Mac only function,
   * GLib does not appear to wrap this stuff up in a portable way
   * which is in fact the reason for using it
   * if you can find a portable version of waitpid() then feel free to change this code
   */

  if(g_io_channel_read_to_end (server->gff_stderr, &server->stderr_output, &length, &gff_pipe_err) != G_IO_STATUS_NORMAL)
    {
      server->stderr_output = g_strdup_printf("failed to read pipe server STDERR: %s", gff_pipe_err? gff_pipe_err->message : "");
    }
#else

  GIOStatus status ;
  gsize terminator_pos = 0;
  gchar * msg = NULL;
  GString *line;

  line = g_string_sized_new(2000) ;      /* Probably not many lines will be > 2k chars. */

  while (1)
    {
      /* this doesn't work: doesn't hang, but doesn't read the error message anyway
       * there is a race condition on big files (i think) where on notsupported errors we look at stderr
       * and wait for ever as the script didn't finish yet as we didn't read the data.
       *      gc = g_io_channel_get_buffer_condition(server->gff_stderr);
       *      GIOCondition gc;
       *      if(!(gc & G_IO_IN))
       *            break;
       */
      status = g_io_channel_read_line_string(server->gff_stderr, line,
					     &terminator_pos,&gff_pipe_err);
      if(status != G_IO_STATUS_NORMAL)
        break;

      *(line->str + terminator_pos) = '\0' ; /* Remove terminating newline. */
      if(terminator_pos > 0)              // can get blank lines at the end
	{
	  if(msg)
	    g_free(msg);
	  msg = g_strdup(line->str);
	  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path,server->query,"%s", msg) ;
	}
    }

  g_string_free(line,TRUE);
  return(msg);

#endif
}


static ZMapServerResponseType openConnection(void *server_in, ZMapServerReqOpen req_open)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  PipeServer server = (PipeServer)server_in ;
  int retval = FALSE;

  if (server->gff_pipe)
    {
      zMapLogWarning("Feature script \"%s\" already active.", server->script_path) ;
    }
  else
    {
      GError *gff_pipe_err = NULL ;

      server->zmap_start = req_open->zmap_start;
      server->zmap_end = req_open->zmap_end;

      server->sequence_map = req_open->sequence_map;

      if(server->scheme == SCHEME_FILE)   // could spawn /bin/cat but there is no need
	{
	  if((server->gff_pipe = g_io_channel_new_file(server->script_path, "r", &gff_pipe_err)))

            retval = TRUE;
	}
      else
	{
	  if (pipe_server_spawn(server,&gff_pipe_err))
            retval = TRUE;
	}

      if (!retval)
	{
	  /* If it was a file error or failure to exec then set up message. */
	  if (gff_pipe_err)
	    setLastErrorMsg(server, &gff_pipe_err) ;

	  /* we don't know if it died or not
	   * but will find out via the getStatus request
	   * we need DIED status to make this happen
	   */
	  result = ZMAP_SERVERRESPONSE_SERVERDIED ;
	}
      else
	{
	  server->sequence_server = req_open->sequence_server; /* if not then drop any DNA data */
	  server->parser = zMapGFFCreateParser(server->sequence_map->sequence, server->zmap_start, server->zmap_end) ;
	  server->gff_line = g_string_sized_new(2000) ;	    /* Probably not many lines will be > 2k chars. */

	  result = pipeGetHeader(server);

          if (result == ZMAP_SERVERRESPONSE_OK)
            {
              // always read it: have to skip over if not wanted
              // need a flag here to say if this is a sequence server
              // ignore error response as we want to report open is OK
              pipeGetSequence(server);
            }
          else
	    {
	      /* we should not do this but if we donlt we get an obscure crash on GFF errors */
	      /* this prevents reporting the error but it's better than crashing */
	      /* the crash is ultimately caused the the hopeless system of status codes
		 in several layers that drives the server interface */
	      /* i'm trying to fix soemthing else right now */
	      //		  result = ZMAP_SERVERRESPONSE_SERVERDIED ;
	    }

	}
    }

  return result ;
}


static ZMapServerResponseType getInfo(void *server_in, ZMapServerInfo info)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  PipeServer server = (PipeServer)server_in ;


  if (getServerInfo(server, info))
    {
      result = ZMAP_SERVERRESPONSE_OK ;
    }
  else
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
      ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path,server->query,
			 "Could not get server info because: %s", server->last_err_msg) ;
    }

  return result ;
}


/* We could parse out all the "source" fields from the gff stream but I don't have time
 * to do this now. So we just return "unsupported", so if this function is called it
 * will alert the caller that something has gone wrong.
 *
 *  */
static ZMapServerResponseType getFeatureSetNames(void *server_in,
						 GList **feature_sets_inout,
						 GList *sources,
						 GList **required_styles_out,
						 GHashTable **featureset_2_stylelist_inout,
						 GHashTable **featureset_2_column_inout,
						 GHashTable **source_2_sourcedata_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  PipeServer server = (PipeServer)server_in ;

  zMapAssert(server) ;

      // these are needed by the GFF parser
  server->source_2_sourcedata = *source_2_sourcedata_inout;
  server->featureset_2_column = *featureset_2_column_inout;

/*
  setErrMsg(server, g_strdup("Feature Sets cannot be read from GFF stream.")) ;
  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path,server->query,
		 "%s", server->last_err_msg) ;
*/
  result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;

  return result ;
}



/* We cannot parse the styles from a gff stream, gff simply doesn't have display styles so we
 * just return "unsupported", so if this function is called it will alert the caller that
 * something has gone wrong.
 *
 *  */
static ZMapServerResponseType getStyles(void *server_in, GHashTable **styles_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  PipeServer server = (PipeServer)server_in ;

  zMapAssert(server) ;

      // can take this warning out as zmapView should now read the styles file globally
  setErrMsg(server, "Reading styles from a GFF stream is not supported.") ;
  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path, server->query,
		 "%s", server->last_err_msg) ;

  result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;

  return result ;
}


/* pipeServers use files froma ZMap style file and therefore include modes
 */
static ZMapServerResponseType haveModes(void *server_in, gboolean *have_mode)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  PipeServer server = (PipeServer)server_in ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  *have_mode = TRUE;

  return result ;
}


static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;

  return result ;
}




/* We don't check anything here, we could look in the data to check that it matches the context
 * I guess which is essentially what we do for the acedb server.
 * NB: as we process a stream we cannot search backwards or forwards
 */
static ZMapServerResponseType setContext(void *server_in, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  PipeServer server = (PipeServer)server_in ;

  server->req_context = feature_context ;


  /* Like the acedb server we should be getting the mapping from the gff data here....
   * then we could set up the block/etc stuff...this is where add mapping should be called from... */



  return result ;
}


// read the header data and exit when we get DNA or features or anything else
static ZMapServerResponseType pipeGetHeader(PipeServer server)
{
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  GError *gff_pipe_err = NULL ;
  GError *error = NULL ;
  gboolean done_header = FALSE ;			    /* read all the header lines */
  ZMapGFFHeaderState header_state = GFF_HEADER_NONE ;	    /* got all the ones we need ? */


  server->result = ZMAP_SERVERRESPONSE_REQFAIL ;  // to catch empty file

  if (server->sequence_server)
    zMapGFFParserSetSequenceFlag(server->parser);  // reset done flag for seq else skip the data

  /* Read the header, needed for feature coord range. */
  while ((status = g_io_channel_read_line_string(server->gff_pipe, server->gff_line,
						 &terminator_pos,
						 &gff_pipe_err)) == G_IO_STATUS_NORMAL)
    {
      server->result = ZMAP_SERVERRESPONSE_OK;   // now we have data default is 'OK'

      *(server->gff_line->str + terminator_pos) = '\0' ; /* Remove terminating newline. */

      if (zMapGFFParseHeader(server->parser, server->gff_line->str, &done_header, &header_state))
	{
	  if (done_header)
	    break ;
	  else
	    server->gff_line = g_string_truncate(server->gff_line, 0) ;  /* Reset line to empty. */
	}
      else
	{
	  if (!done_header)
	    {
	      error = zMapGFFGetError(server->parser) ;

	      if (!error)
		{
		  char *err_msg ;

		  err_msg = g_strdup_printf("zMapGFFParseHeader() failed with no GError for line %d: %s",
					    zMapGFFGetLineNumber(server->parser), server->gff_line->str) ;
		  setErrMsg(server, err_msg) ;
		  g_free(err_msg) ;

		  ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path,server->query,
				     "%s", server->last_err_msg) ;
		}
	      else
		{
		  /* If the error was serious we stop processing and return the error,
		   * otherwise we just log the error. */
		  if (zMapGFFTerminated(server->parser))
		    {
		      server->result = ZMAP_SERVERRESPONSE_REQFAIL ;

		      setErrMsg(server, error->message) ;
		    }
		  else
		    {
		      char *err_msg ;

		      err_msg = g_strdup_printf("zMapGFFParseHeader() failed for line %d: %s",
						zMapGFFGetLineNumber(server->parser),
						server->gff_line->str) ;
		      setErrMsg(server, err_msg) ;
		      g_free(err_msg) ;

		      ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path,server->query,
					 "%s", server->last_err_msg) ;
		    }
		}
	      server->result = ZMAP_SERVERRESPONSE_REQFAIL ;
	    }

	  break ;
	}
    }


  /* Sometimes the file contains only the gff header and no data, I don't know the reason for this
   * but in this case there's no point in going further. */
  /* MH17: see RT 227185 -> good header plus no data means no data not a failure
   * so return ok
   */
  if (header_state == GFF_HEADER_ERROR)
    {
      char *err_msg ;

      if(status == G_IO_STATUS_EOF)
      	err_msg = g_strdup_printf("EOF reached while trying to read header, at line %d",
				  zMapGFFGetLineNumber(server->parser)) ;
      else
      	err_msg = g_strdup_printf("Error in GFF header, at line %d",
				  zMapGFFGetLineNumber(server->parser)) ;

      setErrMsg(server, err_msg) ;
      g_free(err_msg) ;


      ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path, server->query,
			 "%s", server->last_err_msg) ;

      server->result = ZMAP_SERVERRESPONSE_REQFAIL ;
    }

  return(server->result) ;
}




// read any DNA data at the head of the stream and quit after error or ##end-dna
static ZMapServerResponseType pipeGetSequence(PipeServer server)
{
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  GError *gff_pipe_err = NULL ;
  GError *error = NULL ;
  gboolean sequence_finished = FALSE;
  gboolean first = TRUE;

  // read the sequence if it's there
  server->result = ZMAP_SERVERRESPONSE_OK;   // now we have data default is 'OK'

	/* we have already read the first non header line in pipeGetheader */
  do
    {
	if(!first)
		*(server->gff_line->str + terminator_pos) = '\0' ; /* Remove terminating newline. */
	first = FALSE;

      if(!zMapGFFParseSequence(server->parser, server->gff_line->str, &sequence_finished) || sequence_finished)
	break;
    }
  while ((status = g_io_channel_read_line_string(server->gff_pipe, server->gff_line,
						 &terminator_pos,
						 &gff_pipe_err)) == G_IO_STATUS_NORMAL);


  error = zMapGFFGetError(server->parser) ;

  if(error)
    {
      /* If the error was serious we stop processing and return the error,
       * otherwise we just log the error. */
      if (zMapGFFTerminated(server->parser))
	{
          server->result = ZMAP_SERVERRESPONSE_REQFAIL ;
          setErrMsg(server, error->message) ;
	}
      else
	{
	  char *err_msg ;

	  err_msg = g_strdup_printf("zMapGFFParseSequence() failed for line %d: %s - %s",
				    zMapGFFGetLineNumber(server->parser),
				    server->gff_line->str, error->message) ;
          setErrMsg(server, err_msg) ;
	  g_free(err_msg) ;

          ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path,server->query,
			     "%s", server->last_err_msg) ;
	}
      server->result = ZMAP_SERVERRESPONSE_REQFAIL;
    }
  else if(!sequence_finished)
    {
      server->result = ZMAP_SERVERRESPONSE_UNSUPPORTED;
    }

  return(server->result);
}


/* Get features sequence.
 * we assume we called pipeGetHeader() already and also pipeGetSequence()
 * so we are at the start of the BODY part of the stream. */
static ZMapServerResponseType getFeatures(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context)
{
  PipeServer server = (PipeServer)server_in ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  {
    /* DEBUG.... */
    GList *style_names ;

    zMap_g_quark_list_print(feature_context->req_feature_set_names) ;

    style_names = zMapStylesGetNames(feature_context->styles) ;
    zMap_g_quark_list_print(style_names) ;
  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  zMapGFFParseSetSourceHash(server->parser, server->featureset_2_column, server->source_2_sourcedata) ;

  zMapGFFParserInitForFeatures(server->parser, styles, FALSE) ;  // FALSE = create features
  zMapGFFSetDefaultToBasic(server->parser, TRUE);


  // default to OK, previous pipeGetSequence() could have set unsupported
  // if no DNA was provided

  server->result = ZMAP_SERVERRESPONSE_OK;
    {
      addMapping(feature_context, server->zmap_start, server->zmap_end) ;

      /* Fetch all the alignment blocks for all the sequences, this all hacky right now as really.
       * we would have to parse and reparse the stream....can be done but not needed this second. */
      /* can only have one alignment and one block ?? */
      g_hash_table_foreach(feature_context->alignments, eachAlignment, (gpointer)server) ;

      /* get the list of source featuresets */
      feature_context->src_feature_set_names = zMapGFFGetFeaturesets(server->parser);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {
	GError *error = NULL ;
            server->parser->state == ZMAPGFF_PARSE_BODY;
	zMapFeatureDumpStdOutFeatures(feature_context, &error) ;

      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


    }

	// see comment in zmapFeature,h/ ref: num_features
  feature_context->num_features = zMapGFFParserGetNumFeatures(server->parser);

  /* Clear up. -> in destroyConnection() */
//  zMapGFFDestroyParser(server->parser) ;
//  g_sring_free(server->gff_line, TRUE) ;


  return server->result ;
}



// "GFF doesn't understand multiple blocks" therefore there can only be one DNA sequence
// so we expect one alignment and one block
static void eachBlockSequence(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  PipeServer server = (PipeServer) user_data ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)  // no point getting DNA if features are not there
    {
      ZMapSequence sequence;
      if(!(sequence = zMapGFFGetSequence(server->parser)))
	{
	  GError *error;
	  char *estr;

	  error = zMapGFFGetError(server->parser);

	  if(error)
            estr = error->message;
	  else
            estr = "No error reported";

	  setErrMsg(server, estr) ;

	  ZMAPPIPESERVER_LOG(Warning, server->protocol,
			 server->script_path,server->query,
			 "%s", server->last_err_msg);
	}
      else
	{
	  ZMapFeatureContext context;
	  ZMapFeatureSet feature_set;

	  if(zMapFeatureDNACreateFeatureSet(feature_block, &feature_set))
	    {
	      ZMapFeatureTypeStyle dna_style = NULL;
	      ZMapFeature feature;
		GHashTable *hash = zMapGFFParserGetStyles(server->parser);

#if 0
	      /* This temp style creation feels wrong, and probably is,
	       * but we don't have the merged in default styles in here,
	       * or so it seems... */
	      dna_style = zMapStyleCreate(ZMAP_FIXED_STYLE_DNA_NAME,
					  ZMAP_FIXED_STYLE_DNA_NAME_TEXT);

	      feature = zMapFeatureDNACreateFeature(feature_block, dna_style,
						    sequence->sequence, sequence->length);

	      zMapStyleDestroy(dna_style);
#else
		/* the servers need styles to add DNA and 3FT
		 * they used to create temp style and then destroy these but that's not very good
		 * they don't have styles info directly but this is stored in the parser
		 * during the protocol steps, so i wrote a GFF function to supply that info
		 * Now that features have style ref'd indirectly via the featureset we can't use temp data
		 */

		if(hash)
			dna_style = g_hash_table_lookup(hash, GUINT_TO_POINTER(g_quark_from_string(ZMAP_FIXED_STYLE_DNA_NAME)));
		if(dna_style)
			feature = zMapFeatureDNACreateFeature(feature_block, dna_style,
						    sequence->sequence, sequence->length);
#endif
	    }


// this is insane: asking a pipe server for 3FT, however some old code might expect it
	  context = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)feature_block,
								  ZMAPFEATURE_STRUCT_CONTEXT) ;

	  /* I'm going to create the three frame translation up front! */
	  if (zMap_g_list_find_quark(context->req_feature_set_names, zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME)))
	    {
	      if ((zMapFeature3FrameTranslationCreateSet(feature_block, &feature_set)))
	      {
		  ZMapFeatureTypeStyle frame_style = NULL;
		  ZMapFeature feature;
#if 0
		/* NOTE: this old code has the wrong style name ! */
		  frame_style = zMapStyleCreate(ZMAP_FIXED_STYLE_DNA_NAME,
						ZMAP_FIXED_STYLE_DNA_NAME_TEXT);

		  zMapFeature3FrameTranslationSetCreateFeatures(feature_set, frame_style);

		  zMapStyleDestroy(frame_style);
#else
		/* the servers need styles to add DNA and 3FT
		 * they used to create temp style and then destroy these but that's not very good
		 * they don't have styles info directly but this is stored in the parser
		 * during the protocol steps, so i wrote a GFF function to supply that info
		 * Now that features have style ref'd indirectly via the featureset we can't use temp data
		 */
		GHashTable *hash = zMapGFFParserGetStyles(server->parser);

		if(hash)
			frame_style = g_hash_table_lookup(hash, GUINT_TO_POINTER(g_quark_from_string(ZMAP_FIXED_STYLE_3FT_NAME)));
		if(frame_style)
			feature = zMapFeatureDNACreateFeature(feature_block, frame_style,
						    sequence->sequence, sequence->length);
#endif
		}
	    }

	  g_free(sequence);
	}
    }
  return ;
}

/* Process all the alignments in a context. */
static void eachAlignmentSequence(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  PipeServer server = (PipeServer)user_data ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)
    g_hash_table_foreach(alignment->blocks, eachBlockSequence, (gpointer)server) ;

  return ;
}


/*
 * we have pre-read the sequence and simple copy/move the data over if it's there
 */
static ZMapServerResponseType getContextSequence(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context_out)
{
  PipeServer server = (PipeServer)server_in ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)
    {
      /* Fetch all the alignment blocks for all the sequences, this all hacky right now as really.
       * we would have to parse and reparse the stream....can be done but not needed this second. */
      g_hash_table_foreach(feature_context_out->alignments, eachAlignmentSequence, (gpointer)server) ;

    }

  return server->result ;
}


/* Return the last error message. */
static char *lastErrorMsg(void *server_in)
{
  char *err_msg = NULL ;
  PipeServer server = (PipeServer)server_in ;

  zMapAssert(server_in) ;

  if (server->last_err_msg)
    err_msg = server->last_err_msg ;

  return err_msg ;
}


static ZMapServerResponseType getStatus(void *server_conn, gint *exit_code, gchar **stderr_out)
{
  PipeServer server = (PipeServer)server_conn ;

  /* in case of a failure this may already have been done */
  pipe_server_get_stderr(server);

  *exit_code = server->exit_code;
  *stderr_out = server->stderr_output;

  //can't do this or else it won't be read
  //            if(server->exit_code)
  //                  return ZMAP_SERVERRESPONSE_SERVERDIED;

  return ZMAP_SERVERRESPONSE_OK;
}


/* Is the pipe server connected ? */
static ZMapServerResponseType getConnectState(void *server_conn, ZMapServerConnectStateType *connect_state)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  PipeServer server = (PipeServer)server_conn ;

  if (server->child_pid)
    *connect_state = ZMAP_SERVERCONNECT_STATE_CONNECTED ;
  else
    *connect_state = ZMAP_SERVERCONNECT_STATE_UNCONNECTED ;

  return result ;
}


static ZMapServerResponseType closeConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  PipeServer server = (PipeServer)server_in ;
  GError *gff_pipe_err = NULL ;

  if (server->child_pid)
    g_spawn_close_pid(server->child_pid) ;

  if (server->parser)
    zMapGFFDestroyParser(server->parser) ;

  if (server->gff_line)
    g_string_free(server->gff_line, TRUE) ;

  if (server->gff_pipe)
    {
      if (g_io_channel_shutdown(server->gff_pipe, FALSE, &gff_pipe_err) != G_IO_STATUS_NORMAL)
	{
	  zMapLogCritical("Could not close feature pipe \"%s\"", server->script_path) ;

	  setLastErrorMsg(server, &gff_pipe_err) ;

	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	}
      else
	{
	  /* this seems to be required to destroy the GIOChannel.... */
	  g_io_channel_unref(server->gff_pipe) ;
	  server->gff_pipe = NULL ;
	}
    }

  if (server->gff_stderr )
    {
      if(g_io_channel_shutdown(server->gff_stderr, FALSE, &gff_pipe_err) != G_IO_STATUS_NORMAL)
	{
	  zMapLogCritical("Could not close error pipe \"%s\"", server->script_path) ;

	  setLastErrorMsg(server, &gff_pipe_err) ;

	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	}
      else
	{
	  /* this seems to be required to destroy the GIOChannel.... */
	  g_io_channel_unref(server->gff_stderr) ;
	  server->gff_stderr = NULL ;
	}
    }

  return result ;
}

static ZMapServerResponseType destroyConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  PipeServer server = (PipeServer)server_in ;

//printf("pipeserver destroy_connection\n");
  if (server->script_path)
    g_free(server->script_path) ;

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  /* Clear up. -> in destroyConnection() */
/* crashes...
  if(server->parser)
      zMapGFFDestroyParser(server->parser) ;
  if(server->gff_line)
      g_string_free(server->gff_line, TRUE) ;
*/

  g_free(server) ;

  return result ;
}



/*
 * ---------------------  Internal routines.  ---------------------
 */


/* A bit of a lash up for now, we need the parent->child mapping for a sequence and since
 * the code in this file so far simply reads a GFF stream for now, we just fake it by setting
 * everything to be the same for child/parent... */
static void addMapping(ZMapFeatureContext feature_context, int req_start, int req_end)
{
  ZMapFeatureBlock feature_block = NULL;		    // feature_context->master_align->blocks->data ;

  feature_block = (ZMapFeatureBlock)(zMap_g_hash_table_nth(feature_context->master_align->blocks, 0)) ;

  /* We just override whatever is already there...this may not be the thing to do if there
   * are several streams.... */
  feature_context->parent_name = feature_context->sequence_name ;

  // refer to comment in zmapFeature.h 'Sequences and Block Coordinates'
  // NB at time of writing parent_span not always initialised
  feature_context->parent_span.x1 = 1;
  if (feature_context->parent_span.x2 < req_end)
    feature_context->parent_span.x2 = req_end ;

  // seq range  from parent sequence
  if (!feature_context->master_align->sequence_span.x2)
    {
      feature_context->master_align->sequence_span.x1 = req_start ;
      feature_context->master_align->sequence_span.x2 = req_end ;
    }

  // seq coords for our block NOTE must be block coords not features sub-sequence in case of req from mark
  if (!feature_block->block_to_sequence.block.x2)
    {
      feature_block->block_to_sequence.block.x1 = req_start ;
      feature_block->block_to_sequence.block.x2 = req_end ;
    }

  // parent sequence coordinates if not pre-specified
  if (!feature_block->block_to_sequence.parent.x2)
    {
      feature_block->block_to_sequence.parent.x1 = req_start ;
      feature_block->block_to_sequence.parent.x2 = req_end ;
    }

  return ;
}



/* Process all the alignments in a context. */
static void eachAlignment(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  PipeServer server = (PipeServer)user_data ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)
    g_hash_table_foreach(alignment->blocks, eachBlock, (gpointer)server) ;

  return ;
}



static void eachBlock(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  PipeServer server = (PipeServer)user_data ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)
    {
      if (!sequenceRequest(server, server->parser,
			   server->gff_line, feature_block))
	{
	  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path,
                  server->query,
			 "Could not map %s because: %s",
			 g_quark_to_string(server->req_context->sequence_name),
			 server->last_err_msg) ;
	}
    }

  return ;
}


static gboolean sequenceRequest(PipeServer server, ZMapGFFParser parser, GString* gff_line,
				ZMapFeatureBlock feature_block)
{
  gboolean result = FALSE ;
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  gboolean free_on_destroy = FALSE ;
  GError *gff_pipe_err = NULL ;
  gboolean first ;

  /* Tempting to check that block is inside features_start/end, but it may be a different
   * sequence....we don't really deal with this...  */


  /* The caller may only want a small part of the features in the stream so we set the
   * feature start/end from the block, not the gff stream start/end. */

  if(server->zmap_end)
  {
      zMapGFFSetFeatureClipCoords(parser,
			      server->zmap_start,
			      server->zmap_end) ;
      zMapGFFSetFeatureClip(parser,GFF_CLIP_ALL);       // mh17: needs config added to server stanza for clip type
  }

  first = TRUE ;
  do
    {
      if (first)
	first = FALSE ;
      else
	*(gff_line->str + terminator_pos) = '\0' ;	    /* Remove terminating newline. */

      if (!zMapGFFParseLine(parser, gff_line->str))
	{
	  GError *error = zMapGFFGetError(parser) ;

	  if (!error)
	    {
	      server->last_err_msg =
		g_strdup_printf("zMapGFFParseLine() failed with no GError for line %d: %s",
				zMapGFFGetLineNumber(parser), gff_line->str) ;
	      ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path,server->query,
			     "%s", server->last_err_msg) ;

	      result = FALSE ;
	    }
	  else
	    {
	      /* If the error was serious we stop processing and return the error,
	       * otherwise we just log the error. */
	      if (zMapGFFTerminated(parser))
		{
		  result = FALSE ;
		  setErrMsg(server, error->message) ;
		}
	      else
		{
		  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path,server->query,
				 "%s", error->message) ;
		}
	    }
	}

      gff_line = g_string_truncate(gff_line, 0) ;   /* Reset line to empty. */

    } while ((status = g_io_channel_read_line_string(server->gff_pipe, gff_line, &terminator_pos,
						     &gff_pipe_err)) == G_IO_STATUS_NORMAL) ;


  /* If we reached the end of the stream then all is fine, so return features... */
  free_on_destroy = TRUE ;
  if (status == G_IO_STATUS_EOF)
    {
      if (zMapGFFGetFeatures(parser, feature_block))
	{
	  free_on_destroy = FALSE ;			    /* Make sure parser does _not_ free
							       our data ! */

	  result = TRUE ;
	}
    }
  else
    {
      zMapLogWarning("Could not read GFF features from stream \"%s\"", server->script_path) ;

      setLastErrorMsg(server, &gff_pipe_err) ;

      result = FALSE ;
    }

  zMapGFFSetFreeOnDestroy(parser, free_on_destroy) ;

  return result ;
}


static gboolean getServerInfo(PipeServer server, ZMapServerInfo info)
{
  gboolean result = FALSE ;

  result = TRUE ;
  info->database_path = g_strdup(server->script_path) ;
  info->request_as_columns = FALSE;

  return result ;
}



/* Small utility to deal with error messages from the GIOchannel package.
 * Note that we set gff_pipe_err_inout to NULL */
static void setLastErrorMsg(PipeServer server, GError **gff_pipe_err_inout)
{
  GError *gff_pipe_err ;
  char *msg = "failed";

  zMapAssert(server && gff_pipe_err_inout);


  gff_pipe_err = *gff_pipe_err_inout ;
  if (gff_pipe_err)
    msg = gff_pipe_err->message;

  setErrMsg(server, msg) ;

  if (gff_pipe_err)
    g_error_free(gff_pipe_err) ;

  *gff_pipe_err_inout = NULL ;

  return ;
}


/* It's possible for us to have reported an error and then another error to come along. */
/* mgh: if we get an error such as pipe broken then let's look at stderr and if there's a message there report it */
static void setErrMsg(PipeServer server, char *new_msg)
{
  char *error_msg ;

  pipe_server_get_stderr(server);         /* get this regardless */
  /* normally we get stderr on the getStatus call but in case of an error we don't get there */

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  error_msg = g_strdup_printf("SERVER: \"%s\",   REQUEST: \"%s\",   ERROR: \"%s\"",
			      server->script_path, server->query, new_msg) ;

  server->last_err_msg = error_msg ;

  return ;
}

