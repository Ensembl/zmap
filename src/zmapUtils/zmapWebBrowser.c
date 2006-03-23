/*  File: zmapWebBrowser.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2006
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Functions to display a url in a web browser.
 *
 * Exported functions: See zmapUtils.h
 * HISTORY:
 * Last edited: Mar 23 16:27 2006 (edgrif)
 * Created: Thu Mar 23 13:35:10 2006 (edgrif)
 * CVS info:   $Id: zmapWebBrowser.c,v 1.1 2006-03-23 16:42:06 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <sys/utsname.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>


typedef struct
{
  char *name ;
  char *open_command ;

} BrowserConfigStruct, *BrowserConfig ;


typedef struct
{
  char *system ;
  char *browser ;
} Sys2BrowserStruct, *Sys2Browser ;



static char *findSys2Browser(GError **error) ;


/* Records information for running a specific browser. The intent here is to add enough
 * information to allow us to use stuff like Netscapes "open" subcommand which opens the
 * url in an already running netscape if there is one, otherwise in a new netscape.
 * 
 * (see w2/graphgdkremote.c in acedb for some useful stuff.)
 *  */
static BrowserConfigStruct browsers_G[] =
  {
    {"mozilla", NULL},
    {"netscape", NULL},
    {"safari", NULL},
    {NULL, NULL}					    /* Terminator record. */
  } ;


/* Maps operating systems to the preferred browser for that operating system. */
static Sys2BrowserStruct Sys2Browsers_G[] = 
  {
    {"OSF", "netscape"},
    {"Linux", "mozilla"},
    {"Mac", "safari"},
    {NULL, NULL}					    /* Terminator record. */
  } ;


/* Error handling stuff. */
static char *domain_G = "ZMAP_WEB" ;
enum {BROWSER_NOT_FOUND, BROWSER_COMMAND_FAILED, BROWSER_UNAME_FAILED, BROWSER_NOT_REGISTERED} ;
static GQuark err_domain_G = 0 ;




/*! @addtogroup zmaputils
 * @{
 *  */


/*!
 * Launches a web browser to display the specified link. The browser is chosen
 * from an internal list of browsers for different machines. As so much can go
 * wrong this function returns FALSE and a GError struct when an error occurs.
 * You should call this function like this:
 * 
 *     GError *error = NULL ;
 * 
 *     if (!(zMapLaunchWebBrowser("www.acedb.org", &error)))
 *       {
 *         printf("Error: %s\n", error->message) ;
 * 
 *         g_error_free(error) ;
 *       }
 *
 * @param    link              url to be shown in browser.
 * @param    error             pointer to NULL GError pointer for return of errors.
 * @return   gboolean          TRUE if launch of browser successful.
 */
gboolean zMapLaunchWebBrowser(char *link, GError **error)
{
  gboolean result = FALSE ;
  char *best_browser, *browser ;
  GQuark err_domain ;
  char *sys_cmd ;
  int sys_rc ;

  zMapAssert(link && *link && error && !(*error)) ;

  if (!err_domain_G)
    err_domain = g_quark_from_string(domain_G) ;


  /* Get the best browser. */
  best_browser = findSys2Browser(error) ;


  /* Look for the browser in the path. */
  if (best_browser)
    {
      if (!(browser = g_find_program_in_path(best_browser)))
	{
	  *error = g_error_new(err_domain_G, BROWSER_NOT_FOUND,
			       "Browser %s not in $PATH or not executable.", best_browser) ;
	}
    }


  /* Run the browser in a separate process. */
  if (browser)
    {
      /* NOTE that the trailing "&" means we do not wait for the command to be executed and
       * so therefore we cannot tell if the command actually worked, only that the shell
       * got exec'd */
      sys_cmd = g_strdup_printf("%s %s&", browser, link) ;

      /* We could much more to interpret what exactly failed here... */
      if ((sys_rc = system(sys_cmd)) == EXIT_SUCCESS)
	{
	  result = TRUE ;
	}
      else
	{
	  *error = g_error_new(err_domain_G, BROWSER_COMMAND_FAILED,
			       "Failed to run command \"%s\".", sys_cmd) ;
	}
    }


  return result ;
}


/*! @} end of zmaputils docs. */






/* 
 *                Internal functions.
 */




/* Gets the system name and then finds the best browser for the system from our
 * our internal list, returns NULL on any failure. */
static char *findSys2Browser(GError **error)
{
  char *browser = NULL ;
  struct utsname unamebuf ;

  if (uname(&unamebuf) == -1)
    {
      *error = g_error_new_literal(err_domain_G, BROWSER_UNAME_FAILED,
				   "uname() call to find system name failed") ;
    }
  else
    {
      Sys2Browser sys_browser = &(Sys2Browsers_G[0]) ;
      
      while (sys_browser->system != NULL)
	{
	  if (g_ascii_strcasecmp(sys_browser->system, unamebuf.sysname) == 0)
	    {
	      browser = sys_browser->browser ;
	      break ;
	    }
	  sys_browser++ ;
	}
    }

  if (!browser)
    {
      *error = g_error_new(err_domain_G, BROWSER_NOT_REGISTERED,
			   "No browser registered for system %s", unamebuf.sysname) ;
    }


  return browser ;
}
