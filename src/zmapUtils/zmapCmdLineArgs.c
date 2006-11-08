/*  File: zmapCmdLineArgs.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Creates a global object that contains command line
 *              args, the init function should be called once at 
 *              application start up. The object is accessed via a
 *              global reference. The code is not thread safe.
 *
 * Exported functions: See ZMap/zmapCmdLine.h
 * HISTORY:
 * Last edited: Apr 28 12:41 2006 (edgrif)
 * Created: Fri Feb  4 18:24:37 2005 (edgrif)
 * CVS info:   $Id: zmapCmdLineArgs.c,v 1.6 2006-11-08 09:24:38 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <zmapCmdLineArgs_P.h>



gboolean checkForCmdLineSequenceArgs(int argc, char *argv[],
				     char **sequence_out, int *start_out, int *end_out) ;
static void makeContext(int argc, char *argv[]) ;
static void makePoptContext(ZMapCmdLineArgs arg_context) ;
void setPoptArgPtr(char *arg_name, void *value_ptr) ;
void setPoptValPtr(char *arg_name, int new_val) ;
struct poptOption *findPoptEntry(char *arg_name) ;


static ZMapCmdLineArgs arg_context_G = NULL ;



/*! @defgroup cmdline_args   ZMap command line parsing
 * @{
 *
 * @section Overview
 * 
 * Zmap supports the standard Unix command line format:
 * 
 * <PRE>
 *            zmap  [command flags] [sequence name]
 * </PRE>
 * 
 * 
 * Command line flags are specified using the standard "long" format:
 * 
 * <PRE>
 *              --flag          e.g. --start
 * </PRE>
 *
 * Some flags require an argument which must be specified as follows:
 *
 * <PRE>
 *               --flag=value   e.g. --start=10000
 * </PRE>
 * 
 * The final (optional) argument on the command line specifies the name
 * of a sequence to be displayed, this must not be preceded by "--".
 * 
 * ZMap uses the popt package to do command line parsing, the functions
 * provided in the zmap interface to popt provide a simplified interface.
 *
 *  */



/*!
 * There is no context returned here because these commands create a single global context for the whole
 * application which is held internally.
 * 
 * Note that as this routine should be called once per application it is not
 * thread safe, it could be made so but is overkill as the GUI/master thread is
 * not thread safe anyway.
 * 
 * @param argc       The number of arguments in the argv array.
 * @param argv       A string array holding the command line arguments.
 * @return           nothing
 *
 *  */
void zMapCmdLineArgsCreate(int argc, char *argv[])
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  static ZMapCmdLineArgs arg_context = NULL ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  zMapAssert(!arg_context_G) ;

  zMapAssert(argc >= 1 && argv) ;

  makeContext(argc, argv) ;

  return ;
}



/*!
 * Retrieves the final commandline argument, if there is one. This is different
 * in that by unix convention it is not preceded with a "--". The string
 * must not be freed by the application.
 * 
 * @return           A pointer to the string which is the final argument on the command line.
 *
 *  */
char *zMapCmdLineFinalArg(void)
{
  char *final_arg = NULL ;
  ZMapCmdLineArgs arg_context ;

  zMapAssert(arg_context_G) ;
  arg_context = arg_context_G ;

  if (arg_context->sequence_arg)
    final_arg = arg_context->sequence_arg ;

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
  gboolean found ;
  ZMapCmdLineArgs arg_context ;
  struct poptOption *options_table, *option ;

  zMapAssert(arg_name && result) ;

  zMapAssert(arg_context_G) ;
  arg_context = arg_context_G ;

  options_table = arg_context->options_table ;
  found = FALSE ;
  while (!found && options_table->argInfo != 0)
    {
      option = options_table->arg ;

      while (!found && option->longName != NULL)
	{

	  if (strcmp(arg_name, option->longName) == 0)
	    {
	      found = TRUE ;

	      if (option->val == ARG_SET)
		{
		  if (option->argInfo == POPT_ARG_INT)
		    result->i = *((int *)option->arg) ;
		  else if (option->argInfo == POPT_ARG_DOUBLE)
		    result->f = *((double *)option->arg) ;
		  else if (option->argInfo == POPT_ARG_STRING)
		    result->s = *((char **)option->arg) ;
		  else if (option->argInfo == POPT_ARG_NONE)
		    result->b = *((gboolean *)option->arg) ;
		  else
		    zMapAssert("bad coding") ;

		  val_set = TRUE ;
		}
	    }

	  option++ ;
	}

      options_table++ ;
    }

  return val_set ;
}



/*!
 * Frees all resources for the command line parser.
 *
 * @return       nothing
 *
 *  */
void zMapCmdLineArgsDestroy(void)
{
  ZMapCmdLineArgs arg_context ;

  zMapAssert(arg_context_G) ;
  arg_context = arg_context_G ;

  arg_context->opt_context = poptFreeContext(arg_context->opt_context) ;

  g_free(arg_context) ;

  return ;
}



/*! @} end of cmdline_args docs. */



/* 
 *                     Internal routines. 
 * 
 */


static void makeContext(int argc, char *argv[])
{
  ZMapCmdLineArgs arg_context ;

  arg_context_G = arg_context = g_new0(ZMapCmdLineArgsStruct, 1) ;
  arg_context->argc = argc ;
  arg_context->argv = argv ;

  /* Set default values. */
  arg_context->version = FALSE ;
  arg_context->start = -1 ;
  arg_context->end = -1 ;
  arg_context->config_file_path = arg_context->config_dir = NULL ;
  arg_context->window = NULL;
  makePoptContext(arg_context) ;

  return ;
}


/* Note that long args must be specified as:   --opt=arg  -opt1=arg  etc. */
static void makePoptContext(ZMapCmdLineArgs arg_context)
{
  /* These are global tables in the sense that options for all parts of the program are registered
   * here. We could have a really fancy system where subcomponents of the code register their
   * flags/args but that introduces its own problems and seems not worth it for this application.
   * Note that they are static because we need to look in the tables to retrieve values later. */
  static struct poptOption globalOptionsTable[] =
    {
      {ZMAPARG_VERSION, '\0', POPT_ARG_NONE, NULL, ARG_VERSION,
       "Program version.", "<none>"},
      POPT_TABLEEND
    } ;
  static struct poptOption sequenceOptionsTable[] =
    {
      {ZMAPARG_SEQUENCE_START, '\0', POPT_ARG_INT, NULL, ARG_START,
       "Start coord in sequence, must be in range 1 -> seq_length.", "coord"},
      {ZMAPARG_SEQUENCE_END, '\0', POPT_ARG_INT, NULL, ARG_END,
       "End coord in sequence, must be in range start -> seq_length, "
       "but end == 0 means show to end of sequence.", "coord"},
      POPT_TABLEEND
    } ;
  static struct poptOption configFileOptionsTable[] =
    {
      {ZMAPARG_CONFIG_FILE, '\0', POPT_ARG_STRING, NULL, ARG_CONF_FILE,
       "Relative or full path to configuration file.", "file_path"},
      {ZMAPARG_CONFIG_DIR, '\0', POPT_ARG_STRING, NULL, ARG_CONF_DIR,
       "Relative or full path to configuration directory.", "directory"},
      {ZMAPARG_WINDOW_ID, '\0', POPT_ARG_STRING, NULL, ARG_WINID,
       "sdfghsjdfhghsjdfghsjdfhg win id, very drunk.", "0x0000000"},
      POPT_TABLEEND
    } ;
  static struct poptOption options_table[] =
    {
      {NULL, '\0', POPT_ARG_INCLUDE_TABLE,  NULL, 0,
       "Sequence Args", NULL },
      {NULL, '\0', POPT_ARG_INCLUDE_TABLE,  NULL, 0,
       "Config File Args", NULL },
      POPT_AUTOHELP
      POPT_TABLEEND
    } ;
  int popt_rc = 0 ;


  /* Set up the options tables....try this statically now ?? */
  options_table[0].arg = globalOptionsTable ;
  options_table[1].arg = sequenceOptionsTable ;
  options_table[2].arg = configFileOptionsTable ;
  arg_context->options_table = options_table ;


  /* Fill all the stuff that STUPID ANSI-C can't manage. */
  setPoptArgPtr(ZMAPARG_VERSION, &arg_context->version) ;
  setPoptArgPtr(ZMAPARG_SEQUENCE_START, &arg_context->start) ;
  setPoptArgPtr(ZMAPARG_SEQUENCE_END, &arg_context->end) ;
  setPoptArgPtr(ZMAPARG_CONFIG_FILE, &arg_context->config_file_path) ;
  setPoptArgPtr(ZMAPARG_CONFIG_DIR, &arg_context->config_dir) ;
  setPoptArgPtr(ZMAPARG_WINDOW_ID, &arg_context->window) ;


  /* Create the context. */
  arg_context->opt_context = poptGetContext(NULL, arg_context->argc,
					    (const char **)arg_context->argv, options_table, 0) ;

  /* Parse the arguments recording which ones actually get set. */
  while ((popt_rc = poptGetNextOpt(arg_context->opt_context)) > 0)
    {
      switch (popt_rc)
	{
	case ARG_VERSION:
	  setPoptValPtr(ZMAPARG_VERSION, ARG_SET) ;
	  break ;
	case ARG_START:
	  setPoptValPtr(ZMAPARG_SEQUENCE_START, ARG_SET) ;
	  break ;
	case ARG_END:
	  setPoptValPtr(ZMAPARG_SEQUENCE_END, ARG_SET) ;
	  break ;
	case ARG_CONF_FILE:
	  setPoptValPtr(ZMAPARG_CONFIG_FILE, ARG_SET) ;
	  break ;
	case ARG_CONF_DIR:
	  setPoptValPtr(ZMAPARG_CONFIG_DIR, ARG_SET) ;
	  break ;
	case ARG_WINID:
	  setPoptValPtr(ZMAPARG_WINDOW_ID, ARG_SET) ;
	  break ;
	default:
	  zMapAssert("coding error, bad popt value") ;
	break ;
	}
    }


  /* Check for errors or final args on command line. */
  if (popt_rc < -1)
    {
      fprintf(stderr, "%s: bad argument %s: %s\n",
	      *(arg_context->argv),
	      poptBadOption(arg_context->opt_context, POPT_BADOPTION_NOALIAS),
	      poptStrerror(popt_rc)) ;

      /* Don't bother cleaning up, just exit.... */
      exit(EXIT_FAILURE) ;
    }
  else /* if (popt_rc == -1) */
    {
      /* record the last arg if any.... */
      arg_context->sequence_arg = (char *)poptGetArg(arg_context->opt_context) ;
    }


  return ;
}


/* Set the address of a variable to receive the option value. */
void setPoptArgPtr(char *arg_name, void *value_ptr)
{
  struct poptOption *option ;

  option = findPoptEntry(arg_name) ;

  option->arg = value_ptr ;

  return ;
}


/* Set the val field, perhaps to indicate a value was/was not set. */
void setPoptValPtr(char *arg_name, int new_val)
{
  struct poptOption *option ;

  option = findPoptEntry(arg_name) ;

  option->val = new_val ;

  return ;
}


/* Find the named entry in the popt table. */
struct poptOption *findPoptEntry(char *arg_name)
{
  ZMapCmdLineArgs arg_context ;
  struct poptOption *options_table, *option ;
  gboolean found ;


  zMapAssert(arg_name) ;

  zMapAssert(arg_context_G) ;
  arg_context = arg_context_G ;

  options_table = arg_context->options_table ;
  found = FALSE ;
  while (!found && options_table->argInfo != 0)
    {
      option = options_table->arg ;

      while (!found && option->longName != NULL)
	{
	  if (strcmp(arg_name, option->longName) == 0)
	    found = TRUE ;
	  else
	    option++ ;
	}

      options_table++ ;
    }

  return option ;
}

