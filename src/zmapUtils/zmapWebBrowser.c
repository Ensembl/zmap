/*  File: zmapWebBrowser.c
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Functions to display a url in a web browser.
 *
 * Exported functions: See zmapUtils.h
 * HISTORY:
 * Last edited: Mar 28 15:07 2006 (edgrif)
 * Created: Thu Mar 23 13:35:10 2006 (edgrif)
 * CVS info:   $Id: zmapWebBrowser.c,v 1.4 2006-11-08 09:24:56 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <sys/utsname.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>



/* Describes various browsers, crude at the moment, we will probably need more options later. */
typedef struct
{
  char *system ;					    /* system name as in "uname -s" */
  char *executable ;					    /* executable name or full path. */
  char *open_command ;					    /* alternative command to start browser. */
} BrowserConfigStruct, *BrowserConfig ;


static BrowserConfig findSys2Browser(GError **error) ;
static void makeBrowserCmd(GString *cmd, BrowserConfig best_browser, char *url) ;
static char *translateCommas(char *orig_link) ;


/* Records information for running a specific browser. The intent here is to add enough
 * information to allow us to use stuff like Netscapes "open" subcommand which opens the
 * url in an already running netscape if there is one, otherwise in a new netscape.
 * 
 * (see w2/graphgdkremote.c in acedb for some useful stuff.)
 * 
 * (See manpage for more options for "open" on the Mac, e.g. we could specify to always
 * use Safari via the -a flag...which would also deal with badly specified urls...)
 * 
 * In the open_command the %U is substituted with the URL.
 * 
 * Note that if open_command is NULL then the executable name is simply combined with
 * the URL in the expected way to form the command:    "executable  URL"
 * 
 * 
 * Here's one from gnome...this will start in a new window or start a new browser as required.
 * /usr/bin/gnome-moz-remote --newwin www.acedb.org
 * 
 * 
 *  */
#define BROWSER_PATTERN "%U"

static BrowserConfigStruct browsers_G[] =
  {
    {"Linux",  "mozilla",  "mozilla -remote 'openurl(%U,new-window)' || mozilla %U"},
    {"OSF",    "netscape", NULL},
    {"Darwin", "/Applications/Safari.app/Contents/MacOS/Safari", "open %U"},
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
  BrowserConfig best_browser = NULL ;
  char *browser = NULL ;

  zMapAssert(link && *link && error && !(*error)) ;

  if (!err_domain_G)
    err_domain_G = g_quark_from_string(domain_G) ;


  /* Check we have a registered browser for this system. */
  best_browser = findSys2Browser(error) ;


  /* Look for the browser in the users path. */
  if (best_browser)
    {
      if (!(browser = g_find_program_in_path(best_browser->executable)))
	{
	  *error = g_error_new(err_domain_G, BROWSER_NOT_FOUND,
			       "Browser %s not in $PATH or not executable.", browser) ;
	}
    }


  /* Run the browser in a separate process. */
  if (browser)
    {
      char *url ;
      GString *sys_cmd ;
      int sys_rc ;


      /* Translate any "," to "%2C" see translateCommas() for explanation. */
      url = translateCommas(link) ;

      sys_cmd = g_string_sized_new(1024) ;		    /* Should be long enough for most urls. */

      if (best_browser->open_command)
	{
	  makeBrowserCmd(sys_cmd, best_browser, url) ;
	}
      else
	{
	  g_string_printf(sys_cmd, "%s %s", browser, url) ;
	}

      /* Make sure browser is run in background by the shell so we do not wait.
       * NOTE that because we do not wait for the command to be executed,
       * we cannot tell if the command actually worked, only that the shell
       * got exec'd */
      g_string_append(sys_cmd, " &") ;    

      /* We could do much more to interpret what exactly failed here... */
      if ((sys_rc = system(sys_cmd->str)) == EXIT_SUCCESS)
	{
	  result = TRUE ;
	}
      else
	{
	  *error = g_error_new(err_domain_G, BROWSER_COMMAND_FAILED,
			       "Failed to run command \"%s\".", sys_cmd->str) ;
	}

      g_string_free(sys_cmd, TRUE) ;
      g_free(url) ;
    }


  return result ;
}


/*! @} end of zmaputils docs. */






/* 
 *                Internal functions.
 */




/* Gets the system name and then finds the best browser for the system from our
 * our internal list, returns NULL on any failure. */
static BrowserConfig findSys2Browser(GError **error)
{
  BrowserConfig browser = NULL ;
  struct utsname unamebuf ;

  if (uname(&unamebuf) == -1)
    {
      *error = g_error_new_literal(err_domain_G, BROWSER_UNAME_FAILED,
				   "uname() call to find system name failed") ;
    }
  else
    {
      BrowserConfig curr_browser = &(browsers_G[0]) ;
      
      while (curr_browser->system != NULL)
	{
	  if (g_ascii_strcasecmp(curr_browser->system, unamebuf.sysname) == 0)
	    {
	      browser = curr_browser ;
	      break ;
	    }
	  curr_browser++ ;
	}
    }

  if (!browser)
    {
      *error = g_error_new(err_domain_G, BROWSER_NOT_REGISTERED,
			   "No browser registered for system %s", unamebuf.sysname) ;
    }

  return browser ;
}


static void makeBrowserCmd(GString *cmd, BrowserConfig best_browser, char *url)
{
  gboolean found ;

  cmd = g_string_append(cmd, best_browser->open_command) ;

  found = zMap_g_string_replace(cmd, BROWSER_PATTERN, url) ;

  zMapAssert(found) ;					    /* Must find at least one pattern. */

  return ;
}


/* If we use the netscape or Mozilla "OpenURL" remote commands to display links, then sadly the
 * syntax for this command is: "OpenURL(URL[,new-window])" and stupid   
 * netscape will think that any "," in the url (i.e. lots of cgi links have
 * "," to separate args !) is the "," for its OpenURL command and will then
 * usually report a syntax error in the OpenURL command.                   
 *                                                                         
 * To get round this we translate any "," into "%2C" which is the standard 
 * code for a "," in urls (thanks to Roger Pettet for this)....YUCH.
 * 
 * The returned string should be g_free'd when no longer needed.
 */
static char *translateCommas(char *orig_link)
{
  char *url = NULL ;
  GString *link ;
  char *target = "," ;
  char *source = "%2C" ;

  link = g_string_new(orig_link) ;

  zMap_g_string_replace(link, target, source) ;

  url = g_string_free(link, FALSE) ;

  return url ;
}






