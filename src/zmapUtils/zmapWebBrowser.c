/*  File: zmapWebBrowser.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Functions to display a url in a web browser.
 *
 * Exported functions: See zmapUtils.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <sys/utsname.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>



/* Describes various browsers, crude at the moment, we will probably need more options later. */
typedef struct
{
  const char *system ;    /* system name as in "uname -s" */
  const char *executable ;    /* executable name or full path. */
  const char *open_command ;    /* alternative command to start browser. */
} BrowserConfigStruct, *BrowserConfig ;


static char *findBrowser(BrowserConfig browsers, BrowserConfig *browser_out, GError **error) ;
static void makeBrowserCmd(GString *cmd, BrowserConfig best_browser, char *url) ;
static char *translateURLChars(char *orig_link) ;


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

/* List of browsers for different systems, you can have more than one browser for a system. */
static BrowserConfigStruct browsers_G[] =
  {
    {"Linux",  "xdg-open", "xdg-open \"" BROWSER_PATTERN "\""},
    {"Linux",  "iceweasel", "iceweasel -new-window \"" BROWSER_PATTERN "\""},
    {"Linux",  "firefox", "firefox -browser \"" BROWSER_PATTERN "\""},
    {"Linux",  "mozilla", "mozilla -remote 'openurl(\"" BROWSER_PATTERN "\",new-window)' || mozilla \"" BROWSER_PATTERN "\""},
    {"OSF",    "netscape", NULL},
    {"Darwin", "/Applications/Safari.app/Contents/MacOS/Safari", "open \"" BROWSER_PATTERN "\""},
    {NULL, NULL}    /* Terminator record. */
  } ;


/* Error handling stuff. */
static char *domain_G = "ZMAP_WEB" ;
enum {BROWSER_NOT_FOUND, BROWSER_COMMAND_FAILED, BROWSER_UNAME_FAILED, BROWSER_NOT_REGISTERED} ;
static GQuark err_domain_G = 0 ;




/*! @addtogroup zmaputils
 * @{ || 
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
 *       { || 
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

  /* zMapAssert(link && *link && error && !(*error)) ; */
  if (!(link && *link && error && !(*error)) )
    return result ; 

  if (!err_domain_G)
    err_domain_G = g_quark_from_string(domain_G) ;


  /* Check we have a registered browser for this system. */
  browser = findBrowser(browsers_G, &best_browser, error) ;



  /* Run the browser in a separate process. */
  if (browser)
    {
      char *url ;
      GString *sys_cmd ;
      int sys_rc ;

      /* Translate troublesome chars to their url escape sequences, see translateURLChars() for explanation. */
      url = translateURLChars(link) ;  

      sys_cmd = g_string_sized_new(1024) ;    /* Should be long enough for most urls. */

      if (best_browser->open_command)
        {
          makeBrowserCmd(sys_cmd, best_browser, url) ;
        }
      else 
        {
          g_string_printf(sys_cmd, "%s \"%s\"", browser, url) ;
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




/* Gets the system name and then finds browsers for that system from our || 
 * our internal list, if it finds the browser is in the users path then
 * returns the path otherwise returns NULL and sets error to give details
 * of what went wrong. */
static char *findBrowser(BrowserConfig browsers_in, BrowserConfig *browser_out, GError **error)
{
  char *browser = NULL ;
  struct utsname unamebuf ;
  gboolean browser_in_list = FALSE ;

  if (uname(&unamebuf) == -1)
    {
      *error = g_error_new_literal(err_domain_G, BROWSER_UNAME_FAILED,
                                   "uname() call to find system name failed") ;
    }
  else 
    {
      BrowserConfig curr_browser = browsers_in ;
      
      while (curr_browser->system != NULL)
        {
          if (g_ascii_strcasecmp(curr_browser->system, unamebuf.sysname) == 0)
            {
              browser_in_list = TRUE ; 
        
              /* Look for the browser in the users path. */
              if ((browser = g_find_program_in_path(curr_browser->executable)))
                {
                  *browser_out = curr_browser ;
                
                  break ;
                }
            }

          curr_browser++ ;
        }
    }

  if (!browser)
    {
      if (browser_in_list)
        {
          *error = g_error_new(err_domain_G, BROWSER_NOT_FOUND,
               "Browser(s) registered with ZMap for this system (%s)"
               " but none found in $PATH or they were not executable.", unamebuf.sysname) ;
        }
      else
        {
          *error = g_error_new(err_domain_G, BROWSER_NOT_REGISTERED,
               "No browser registered for system %s", unamebuf.sysname) ;
        }
    }

  return browser ;
}


static void makeBrowserCmd(GString *cmd, BrowserConfig best_browser, char *url)
{
  gboolean found ;

  cmd = g_string_append(cmd, best_browser->open_command) ;

  found = zMap_g_string_replace(cmd, BROWSER_PATTERN, url) ;

  /* zMapAssert(found) ;*/    /* Must find at least one pattern. */

  return ;
}


/* URL standards provide escape sequences for all chars which gets round problems
 * with them being wrongly interpreted. This function translates chars in the url
 * that give us problems for various reasons:
 *
 * If we use the netscape or Mozilla "OpenURL" remote commands to display links, then sadly the
 * syntax for this command is: "OpenURL(URL[,new-window])" and stupid
 * netscape/mozilla will think that any "," in the url (i.e. lots of cgi links have
 * "," to separate args !) is the "," for its OpenURL command and will then
 * usually report a syntax error in the OpenURL command.                   
 *                                                                         
 * To get round this we translate any "," into "%2C" which is the standard 
 * code for a "," in urls (thanks to Roger Pettet for this)....YUCH.
 * 
 * Some urls have single quotes in them, if you pass the whole string through
 * to the shell these get wrongly interpreted by the shell in its normal
 * string fashion so we translate them all to "%27".
 * 
 * mh17: for security we need to patch out other shell special characters such as '|', ';', '`'. >,< should be harmless
 * Anything that allows a user to run another command is a no-no
 * This gets inefficient (perl and php might do this better)
 * we can quote the url, but that opens the door to interpretation, better to code metachars as hex.
 "
 * The returned string should be g_free'd when no longer needed.
 *
 *    mh17: second thoughts: don't use, these rely on quoting: thrid thoughts: no amount of escaped quoting seems to work
 */
static char *translateURLChars(char *orig_link)
{
  char *url = NULL ;
  
#if TRANSLATE

  GString *link ;
  char *target, *source ;

  link = g_string_new(orig_link) ;

  target = " " ;
  source = "%20" ;
  zMap_g_string_replace(link, target, source) ;

  target = ";" ;
  source = "%3B" ;
  zMap_g_string_replace(link, target, source) ;

  target = "," ;
  source = "%2C" ;
  zMap_g_string_replace(link, target, source) ;

  target = "'" ;
  source = "%27" ;
  zMap_g_string_replace(link, target, source) ;

  target = "&" ;
  source = "%26" ;
  zMap_g_string_replace(link, target, source) ;

  target = "|" ;
  source = "%7C" ;
  zMap_g_string_replace(link, target, source) ;

  target = "`" ;
  source = "%60" ;
  zMap_g_string_replace(link, target, source) ;

  url = g_string_free(link, FALSE) ;
#else
  url = g_strdup(orig_link);
#endif
  return url ;
}





