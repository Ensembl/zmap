/*  File: zmapCmdLineArgs.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Creates a global object that contains command line
 *              args, the init function should be called once at
 *              application start up. The object is accessed via a
 *              global reference. The code is not thread safe.
 *
 * Exported functions: See ZMap/zmapCmdLine.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <zmapCmdLineArgs_P.h>
#include <zmapUtils_P.h>



#define ARG_NO_FLAGS 0



typedef GOptionEntry *(* get_entries_func)(ZMapCmdLineArgs context) ;



static void makeContext(int argc, char *argv[]) ;
static void makeOptionContext(ZMapCmdLineArgs arg_context);
static GOptionEntry *get_main_entries(ZMapCmdLineArgs arg_context);
static GOptionEntry *get_config_entries(ZMapCmdLineArgs arg_context);



extern gboolean zmap_timing_G;



static ZMapCmdLineArgs arg_context_G = NULL ;



/* There is no context returned here because these commands create a single global context for the whole
 * application which is held internally.
 *
 * Note that as this routine should be called once per application it is not
 * thread safe, it could be made so but is overkill as the GUI/master thread is
 * not thread safe anyway.
 *
 *  */
void zMapCmdLineArgsCreate(int *argc, char *argv[])
{
  ZMapCmdLineArgs arg_context = NULL ;

  zMapAssert(!arg_context_G) ;

  zMapAssert(argc && *argc >= 1 && argv) ;

  makeContext(*argc, argv) ;

  zMapAssert(arg_context_G) ;

  arg_context = arg_context_G;

  *argc = arg_context->argc;

  return ;
}



/* Retrieves the final commandline argument, if there is one. This is different
 * in that by unix convention it is not preceded with a "--". The string
 * must not be freed by the application.
 *
 * @return           A pointer to the string which is the final argument on the command line.
 *
 *  */
/* NOTE the 'final arg' is actually zero or more files names and can be interspersed with options */
char **zMapCmdLineFinalArg(void)
{
  char **final_arg = NULL ;
  ZMapCmdLineArgs arg_context ;

  zMapAssert(arg_context_G) ;
  arg_context = arg_context_G ;

  if (arg_context->files_arg &&
      (arg_context->files_arg))
    final_arg = (arg_context->files_arg) ;

  return final_arg ;
}



/*!
 * Given a command line flag, returns the value specified on the command line for
 * that flag, the value may be NULL if the flag does not take a value. If the flag
 * cannot be found then returns FALSE.
 *
 * @param arg_name   The argument flag to search for.
 * @param result     The value for the argument flag.
 * @return           TRUE if arg_name was found in the args array, FALSE otherwise.
 *
 *  */
gboolean zMapCmdLineArgsValue(char *arg_name, ZMapCmdLineArgsType *result)
{
  gboolean val_set = FALSE ;
  ZMapCmdLineArgs arg_context ;
  enum {GET_ENTRIES_COUNT = 2} ;
  get_entries_func get_entries[GET_ENTRIES_COUNT] = { get_main_entries, get_config_entries };
  int i;

  zMapAssert(arg_context_G) ;
  arg_context = arg_context_G ;

  for(i = 0; i < GET_ENTRIES_COUNT; i++)
    {
      GOptionEntry *entries;
      if(!val_set)
	{
	  entries = (get_entries[i])(arg_context);
	  while(!val_set && entries && entries->long_name)
	    {
	      if(g_quark_from_string(entries->long_name) == g_quark_from_string(arg_name))
		{
		  if(entries->arg == G_OPTION_ARG_NONE &&
		     ZMAPARG_INVALID_BOOL != *(gboolean *)entries->arg_data)
		    {
		      if(result)
			result->b = *(gboolean *)(entries->arg_data);
		      val_set = TRUE;
		    }
		  else if(entries->arg == G_OPTION_ARG_INT &&
			  ZMAPARG_INVALID_INT != *(int *)entries->arg_data)
		    {
		      if(result)
			result->i = *(int *)(entries->arg_data);
		      val_set = TRUE;
		    }
		  else if(entries->arg == G_OPTION_ARG_DOUBLE &&
			  ZMAPARG_INVALID_FLOAT != *(double *)entries->arg_data)
		    {
		      if(result)
			result->f = *(double *)(entries->arg_data);
		      val_set = TRUE;
		    }
		  else if(entries->arg == G_OPTION_ARG_STRING &&
			  ZMAPARG_INVALID_STR != *(char **)entries->arg_data)
		    {
		      if(result)
			result->s = *(char **)(entries->arg_data);
		      val_set = TRUE;
		    }
		  else if(entries->arg == G_OPTION_ARG_STRING_ARRAY &&
			  ZMAPARG_INVALID_STR != entries->arg_data)
		    {
		      if(result)
			result->sa = (char **)(entries->arg_data);
		      val_set = TRUE;
		    }
		}
	      entries++;
	    }
	}
    }

  return val_set ;
}



/*!
 * Frees all resources for the command line parser.
 *
 * @return       nothing
 *
 *  */
/* MH17 NOTE not called */
void zMapCmdLineArgsDestroy(void)
{
  ZMapCmdLineArgs arg_context ;

  zMapAssert(arg_context_G) ;
  arg_context = arg_context_G ;

  if(arg_context->opt_context)
    g_option_context_free(arg_context->opt_context);

  g_free(arg_context) ;

  return ;
}




/*
 *                     Internal routines.
 */

static void makeContext(int argc, char *argv[])
{
  ZMapCmdLineArgs arg_context ;

  arg_context_G = arg_context = g_new0(ZMapCmdLineArgsStruct, 1) ;
  arg_context->argc = argc ;
  arg_context->argv = argv ;

  /* Set default values. */
  arg_context->version = ZMAPARG_INVALID_BOOL;
  arg_context->serial = ZMAPARG_INVALID_BOOL;
  arg_context->remote_debug = ZMAPARG_INVALID_STR ;
  arg_context->peer_name  = ZMAPARG_INVALID_STR ;
  arg_context->peer_clipboard  = ZMAPARG_INVALID_STR ;
  arg_context->start   = ZMAPARG_INVALID_INT;
  arg_context->end     = ZMAPARG_INVALID_INT ;
  arg_context->config_file_path = arg_context->config_dir = ZMAPARG_INVALID_STR;
  arg_context->window  = ZMAPARG_INVALID_STR;

  makeOptionContext(arg_context);

  return ;
}

static void makeOptionContext(ZMapCmdLineArgs arg_context)
{
  GError *error = NULL;
  GOptionContext *context;
  GOptionEntry *main_entries, *config_entries;
  GString *a_summary = g_string_sized_new(128);
  GString *a_description = g_string_sized_new(128);
  char *rest_args = NULL;

  context = g_option_context_new (rest_args);

  main_entries = get_main_entries(arg_context);

  g_option_context_add_main_entries(context, main_entries, NULL);

  g_string_append_printf(a_summary, "%s %s %s", zMapGetAppName(), zMapGetAppVersionString(), zMapGetCompileString()) ;
  g_string_append_printf(a_summary, "\n\n%s", ZMAP_DESCRIPTION);

  g_option_context_set_summary(context, a_summary->str);

  g_string_append_printf(a_description,
			 "\n%s\n%s\n\n%s\n\n%s\n",
			 zMapGetCommentsString(),
			 ZMAP_WEBSITE_STRING(),
			 ZMAP_COPYRIGHT_STRING(),
			 ZMAP_LICENSE_STRING());

  g_option_context_set_description(context, a_description->str);

  config_entries = get_config_entries(arg_context);

  g_option_context_add_main_entries(context, config_entries, NULL);


  /* Add gtk args to our list so things like --display get parsed, setting FALSE
   * tells gtk not to try and open the display at this stage.  */
  g_option_context_add_group (context, gtk_get_option_group (FALSE));


  arg_context->opt_context = context;

  if (!g_option_context_parse (context,
			       &arg_context->argc,
			       &arg_context->argv, &error))
    {
      g_print ("option parsing failed: %s\n", error->message);
      exit (1);
    }

  g_string_free(a_summary, TRUE);
  g_string_free(a_description, TRUE);

  return;
}


static GOptionEntry *get_main_entries(ZMapCmdLineArgs arg_context)
{
  static GOptionEntry entries[] = {

    /* long_name, short_name, flags, arg, arg_data, description, arg_description */

    { ZMAPARG_VERSION, 0, 0, G_OPTION_ARG_NONE, NULL, ZMAPARG_VERSION_DESC, ZMAPARG_NO_ARG },

    { ZMAPARG_RAW_VERSION, 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, NULL,
      ZMAPARG_RAW_VERSION_DESC, ZMAPARG_NO_ARG },

    { ZMAPARG_SERIAL,  0, 0, G_OPTION_ARG_NONE, NULL, ZMAPARG_SERIAL_DESC,  ZMAPARG_NO_ARG },

    { ZMAPARG_REMOTE_DEBUG, 0, 0, G_OPTION_ARG_STRING, NULL, ZMAPARG_REMOTE_DEBUG_DESC, ZMAPARG_REMOTE_DEBUG_ARG },

    { ZMAPARG_PEER_NAME, 0, 0, G_OPTION_ARG_STRING, NULL, ZMAPARG_PEER_NAME_DESC, ZMAPARG_PEER_NAME_ARG },

    { ZMAPARG_PEER_CLIPBOARD, 0, 0, G_OPTION_ARG_STRING, NULL, ZMAPARG_PEER_CLIPBOARD_DESC, ZMAPARG_PEER_CLIPBOARD_ARG },

    { ZMAPARG_SEQUENCE_START, 0, 0, G_OPTION_ARG_INT, NULL, ZMAPARG_SEQUENCE_START_DESC, ZMAPARG_COORD_ARG },

    { ZMAPARG_SEQUENCE_END,   0, 0, G_OPTION_ARG_INT, NULL, ZMAPARG_SEQUENCE_END_DESC, ZMAPARG_COORD_ARG },

    { ZMAPARG_SLEEP, 0, 0, G_OPTION_ARG_INT, NULL, ZMAPARG_SLEEP_DESC, ZMAPARG_NO_ARG },

    { ZMAPARG_TIMING,  0, 0, G_OPTION_ARG_NONE, NULL, ZMAPARG_TIMING_DESC,  ZMAPARG_NO_ARG },

    { ZMAPARG_SHRINK,  0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, NULL, ZMAPARG_SHRINK_DESC,  ZMAPARG_NO_ARG },

    { ZMAPARG_SEQUENCE, 0, ARG_NO_FLAGS, G_OPTION_ARG_STRING, NULL, ZMAPARG_SEQUENCE_DESC, ZMAPARG_SEQUENCE_ARG },

/* Must be the last entry. */
    { G_OPTION_REMAINING, 0, ARG_NO_FLAGS, G_OPTION_ARG_STRING_ARRAY, NULL, ZMAPARG_FILES, ZMAPARG_FILES_ARG },

    { NULL }
  } ;


  if (entries[0].arg_data == NULL)
    {
      entries[0].arg_data = &(arg_context->version);
      entries[1].arg_data = &(arg_context->serial);
      entries[2].arg_data = &(arg_context->remote_debug);
      entries[3].arg_data = &(arg_context->peer_name);
      entries[4].arg_data = &(arg_context->peer_clipboard);
      entries[5].arg_data = &(arg_context->start);
      entries[6].arg_data = &(arg_context->end);
      entries[7].arg_data = &(arg_context->sleep);
      entries[8].arg_data = &(zmap_timing_G);
      entries[9].arg_data = &(arg_context->shrink) ;
      entries[10].arg_data = &(arg_context->sequence_arg);
      entries[11].arg_data = &(arg_context->files_arg);
    }

  return &entries[0] ;
}

static GOptionEntry *get_config_entries(ZMapCmdLineArgs arg_context)
{
  static GOptionEntry entries[] = {
    { ZMAPARG_CONFIG_FILE, 0, 0,
      G_OPTION_ARG_STRING, NULL,
      ZMAPARG_CONFIG_FILE_DESC, ZMAPARG_FILE_ARG },
    { ZMAPARG_CONFIG_DIR, 0, 0,
      G_OPTION_ARG_STRING, NULL,
      ZMAPARG_CONFIG_DIR_DESC, ZMAPARG_DIR_ARG },
    { ZMAPARG_WINDOW_ID, 0, 0,
      G_OPTION_ARG_STRING, NULL,
      ZMAPARG_WINDOW_ID_DESC, ZMAPARG_WINID_ARG },
    { ZMAPARG_REMOTE_DEBUG, 0, 0, 
      G_OPTION_ARG_STRING, NULL,
      ZMAPARG_REMOTE_DEBUG_DESC, ZMAPARG_REMOTE_DEBUG_ARG },
    { ZMAPARG_PEER_NAME, 0, 0, 
      G_OPTION_ARG_STRING, NULL,
      ZMAPARG_PEER_NAME_DESC, ZMAPARG_PEER_NAME_ARG },
    { ZMAPARG_PEER_CLIPBOARD, 0, 0, 
      G_OPTION_ARG_STRING, NULL,
      ZMAPARG_PEER_CLIPBOARD_DESC, ZMAPARG_PEER_CLIPBOARD_ARG },
    { NULL }
  };

  if(entries[0].arg_data == NULL)
    {
      entries[0].arg_data = &(arg_context->config_file_path);
      entries[1].arg_data = &(arg_context->config_dir);
      entries[2].arg_data = &(arg_context->window);
      entries[3].arg_data = &(arg_context->remote_debug);
      entries[4].arg_data = &(arg_context->peer_name);
      entries[5].arg_data = &(arg_context->peer_clipboard);
    }

  return &entries[0];
}


