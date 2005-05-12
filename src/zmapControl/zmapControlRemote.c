/*  File: zmapControlRemote.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Implements code to respond to commands sent to a
 *              ZMap via the xremote program. The xremote program
 *              is distributed as part of acedb but is not dependent
 *              on it (although it currently makes use of acedb utils.
 *              code in the w1 directory).
 *              
 * Exported functions: See zmapControl_P.h
 * HISTORY:
 * Last edited: May 12 16:45 2005 (rds)
 * Created: Wed Nov  3 17:38:36 2004 (edgrif)
 * CVS info:   $Id: zmapControlRemote.c,v 1.7 2005-05-12 16:23:55 rds Exp $
 *-------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapUtils.h>
#include <zmapControl_P.h>
#include <ZMap/zmapXRemote.h>

/* Looks like we need to include this here... */
#include <ZMap/zmapWindow.h>

/* OK THERE IS A GENERAL PROBLEM HERE....WE NEED TO BE ABLE TO SEND COMMANDS TO A SPECIFIC WINDOW
   WITHIN THE ZMAP.......TRICKY....WE NEED THE WINDOW ID REALLY.... ok so we've got that

   THINK ABOUT THIS........we will need the current window and all that stuff.....
*/


/* These seem ok here.  At least they're all in one place then */
static char *controlexecuteCommand(char *command_text, ZMap zmap, int *statusCode);

static char *findFeature(ZMap zmap, char *command_text) ;
static char *createClient(ZMap zmap, char *command_text) ;

static void destroyNotifyData(gpointer destroy_data);

void zmapControlRemoteInstaller(GtkWidget *widget, gpointer zmap_data)
{
  ZMap zmap = (ZMap)zmap_data;
  zMapXRemoteObj xremote;

  if((xremote = zMapXRemoteNew()) != NULL)
    {
      Window id;
      zMapXRemoteNotifyData notifyData;

      id = (Window)GDK_DRAWABLE_XID(widget->window);
      
      notifyData           = g_new0(zMapXRemoteNotifyDataStruct, 1);
      notifyData->xremote  = xremote;
      notifyData->callback = ZMAPXREMOTE_CALLBACK(controlexecuteCommand);
      notifyData->data     = zmap_data; 
      
      zMapXRemoteInitServer(xremote, id, PACKAGE_NAME, ZMAP_DEFAULT_REQUEST_ATOM_NAME, ZMAP_DEFAULT_RESPONSE_ATOM_NAME);
      zMapConfigDirWriteWindowIdFile(id, zmap->zmap_id);

      /* Makes sure we actually get the events!!!! Use add_events as set_events needs to be done BEFORE realize */
      gtk_widget_add_events(widget, GDK_PROPERTY_CHANGE_MASK) ;

      g_signal_connect_data(G_OBJECT(widget), "property_notify_event",
                            G_CALLBACK(zMapXRemotePropertyNotifyEvent), (gpointer)notifyData,
                            (GClosureNotify) destroyNotifyData, G_CONNECT_AFTER
                            );

      zmap->propertyNotifyData = notifyData;
    }
  
  return;
}
/*  */
static void destroyNotifyData(gpointer destroy_data)
{
  ZMap zmap;
  zMapXRemoteNotifyData destroy_me = (zMapXRemoteNotifyData)destroy_data;

  zmap = destroy_me->data;
  zmap->propertyNotifyData = NULL; /* Set this to null, as we're emptying the mem */
  g_free(destroy_me);           /* Is this all we need to destroy?? */

  return ;
}


/* Handle commands sent from xremote. */
/* Return is string in the style of ZMAP_XREMOTE_REPLY_FORMAT (see ZMap/zmapXRemote.h) */
/* Building the reply string is a bit arcane in that the xremote reply strings are really format
 * strings...perhaps not ideal...., but best in the cicrumstance I guess */
static char *controlexecuteCommand(char *command_text, ZMap zmap, int *statusCode)
{
  char *xml_reply     = NULL ;
  gboolean executed   = FALSE;

  //  zMapWindow win; 
          /* This will be the focus window. Not
                               sure what will happen if we want to
                               add a command 'change_focus_win
                               window_name' */
  //  win = zMapViewGetWindow(zmap->focus_viewwindow); /* Esp. as we get it here!! */

  
  //command prop = value ; prop = value ; prop = value

  /* We assume command is first word in text with no preceding blanks. */
  if (g_str_has_prefix(command_text, "zoom_in"))
    {
      zmapControlWindowDoTheZoom(zmap, 2.0) ;
      *statusCode = ZMAPXREMOTE_OK;
      xml_reply = "<a>b</a>";
    }
  else if (g_str_has_prefix(command_text, "zoom_out"))
    {
      zmapControlWindowDoTheZoom(zmap, 0.5) ;
      *statusCode = ZMAPXREMOTE_OK;
      xml_reply = "<a>b</a>";
    }
  else if (g_str_has_prefix(command_text, "feature_find"))
    {
      findFeature(zmap, command_text);
      *statusCode = ZMAPXREMOTE_OK;
      xml_reply = "<a>b</a>";
    }
  else if (g_str_has_prefix(command_text, "register_client"))
    {
      createClient(zmap, command_text);
      *statusCode = ZMAPXREMOTE_OK;
      xml_reply = "<a>b</a>";
    }
  else
    {
      *statusCode = ZMAPXREMOTE_UNKNOWNCMD;
      xml_reply   = "<message>unknown command</message>";
    }

  //if(!xml_reply)   // we can now get the xml_reply and statusCode from window->sense.
  //  xml_reply = functionToFindItFromAWindow(win, &statusCode);

  // gchar * g_markup_escape_text(const gchar *text, gssize length);
  // gchar * g_markup_printf_escaped(const char * format, ...);

  return xml_reply;
}


/* The feature_find command has the following format: 
 * 
 * "feature_find method = <method_id> ; type = <exon | homol | etc.> ; feature = <feature_name> ;
 *  strand = < C|F|+|-|. > ; q_start = 40 ; q_end = 387 ; t_start = 2613255 ; t_end = 2613598"
 * 
 *  "feature_find method = readpairs ; type = homol ; sequence = 20SNP45079-1505c12.p1c ;
 *   strand = C ; q_start = 40 ; q_end = 387 ; t_start = 2613255 ; t_end = 2613598"
 * 
 * um, actually not sure how generalistic we want this....should we cases for homols or exons or....
 * 
 *  */
static char *findFeature(ZMap zmap, char *command_text)
{
  char *result = NULL ;
  gboolean parse_error ;
  char *next ;
  char *keyword, *value ;
  gchar *style = NULL ;
  ZMapFeatureType feature_type = ZMAPFEATURE_INVALID ;
  char *feature_name = NULL ;
  ZMapStrand strand = ZMAPSTRAND_NONE ;
  int start = 0, end = 0, query_start = 0, query_end = 0 ;

  next = strtok(command_text, " ") ;			    /* Skip feature_find. */

  parse_error = FALSE ;
  while (!parse_error && (next = strtok(NULL, " ")))
    {
      keyword = g_strdup(next) ;			    /* Get keyword. */

      next = strtok(NULL, " ") ;			    /* skip "=" */

      next = strtok(NULL, " ") ;			    /* Get value. */
      value = g_strdup(next) ;

      next = strtok(NULL, " ") ;			    /* skip ";" */


      if (g_ascii_strcasecmp(keyword, "method") == 0)
	style = g_strdup(value) ;
      else if (g_ascii_strcasecmp(keyword, "type") == 0)
	{
	  /* we need a convertor from string -> SO compliant feature type here....needed in
	   * zmapGFF.c as well... */

	  if (g_ascii_strcasecmp(value, "homol") == 0)
	    feature_type = ZMAPFEATURE_HOMOL ;
	}
      else if (g_ascii_strcasecmp(keyword, "feature") == 0)
	feature_name = g_strdup(value) ;
      else if (g_ascii_strcasecmp(keyword, "strand") == 0)
	{
	  if (g_ascii_strcasecmp(value, "F") == 0
	      || g_ascii_strcasecmp(value, "+") == 0)
	    strand = ZMAPSTRAND_FORWARD ;
	  else if (g_ascii_strcasecmp(value, "C") == 0
		   || g_ascii_strcasecmp(value, "-") == 0)
	    strand = ZMAPSTRAND_REVERSE ;
	}
      else if (g_ascii_strcasecmp(keyword, "q_start") == 0)
	query_start = atoi(value) ;
      else if (g_ascii_strcasecmp(keyword, "q_end") == 0)
	query_end = atoi(value) ;
      else if (g_ascii_strcasecmp(keyword, "t_start") == 0)
	start = atoi(value) ;
      else if (g_ascii_strcasecmp(keyword, "t_end") == 0)
	end = atoi(value) ;
      else
	{
	  parse_error = TRUE ;
	  result = "";          /* XREMOTE_S_500_COMMAND_UNPARSABLE ; */
	}

      g_free(keyword) ;
      g_free(value) ;
    }


  if (!result)
    {
      if (style && *style && feature_type != ZMAPFEATURE_INVALID && feature_name && *feature_name
	  && strand
	  && start != 0 && end != 0
	  && (feature_type == ZMAPFEATURE_HOMOL && query_start != 0 && query_end != 0))
	{
	  ZMapWindow window ;
	  FooCanvasItem *item ;

	  window = zMapViewGetWindow(zmap->focus_viewwindow) ;

	  if ((item = zMapWindowFindFeatureItemByName(window, style, feature_type, feature_name,
						      strand, start, end, query_start, query_end))
	      && (zMapWindowScrollToItem(zMapViewGetWindow(zmap->focus_viewwindow), item)))
	    result = "";      /*  XREMOTE_S_200_COMMAND_EXECUTED ;  */

	  else
	    result = "";      /* XREMOTE_S_500_COMMAND_UNPARSABLE ; */
	}
      else
	{
	  result = "";        /* XREMOTE_S_500_COMMAND_UNPARSABLE ; */
	}
    }

  /* Clean up. */
  if (style)
    g_free(style) ;
  if (feature_name)
    g_free(feature_name) ;


  return result ;
}

static char *createClient(ZMap zmap, char *command_text)
{
  char *result = NULL ;
  gboolean parse_error ;
  char *next ;
  char *keyword, *value ;
  gchar *type = NULL ;
  char *remote, *request, *response;
  zMapXRemoteObj client;


  next = strtok(command_text, " ") ;			    /* Skip register_client. */

  parse_error = FALSE ;
  while (!parse_error && (next = strtok(NULL, " ")))
    {
      keyword = g_strdup(next) ;			    /* Get keyword. */

      next = strtok(NULL, " ") ;			    /* skip "=" */

      next = strtok(NULL, " ") ;			    /* Get value. */
      value = g_strdup(next) ;

      next = strtok(NULL, " ") ;			    /* skip ";" */


      if (g_ascii_strcasecmp(keyword, "id") == 0)
        {
          remote = g_strdup(value) ;
        }
      else if (g_ascii_strcasecmp(keyword, "req") == 0)
	{
          request = g_strdup(value) ;
	}
      else if (g_ascii_strcasecmp(keyword, "res") == 0)
        {
          response = g_strdup(value) ;
        }
      else
	{
	  parse_error = TRUE ;
	  result = "";          /* XREMOTE_S_500_COMMAND_UNPARSABLE ; */
	}

      g_free(keyword) ;
      g_free(value) ;
    }

  if (!result)
    {
      if((client = zMapXRemoteNew()) != NULL)
        {
          zMapXRemoteInitClient(client, strtoul(remote, (char **)NULL, 16));
          zMapXRemoteSetRequestAtomName(client, request);
          zMapXRemoteSetResponseAtomName(client, response);
        }
      else
	{
	  result = "";          /* XREMOTE_S_500_COMMAND_UNPARSABLE ; */
	}
    }



  /* Clean up. */
  if (remote)
    g_free(remote) ;
  if (request)
    g_free(request) ;
  if (response)
    g_free(response) ;


  return result ;
}
