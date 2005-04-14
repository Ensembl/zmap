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
 * Last edited: Apr 14 11:34 2005 (rds)
 * Created: Wed Nov  3 17:38:36 2004 (edgrif)
 * CVS info:   $Id: zmapControlRemote.c,v 1.5 2005-04-14 10:52:11 rds Exp $
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


/* OK THERE IS A GENERAL PROBLEM HERE....WE NEED TO BE ABLE TO SEND COMMANDS TO A SPECIFIC WINDOW
   WITHIN THE ZMAP.......TRICKY....WE NEED THE WINDOW ID REALLY....

   THINK ABOUT THIS........we will need the current window and all that stuff.....
*/


/* These seem ok here.  At least they're all in one place then */
static char *executeCommand(ZMap zmap, char *command_text) ; /* To execute one of the commands below */

static char *findFeature(ZMap zmap, char *command_text) ;
static char *createClient(ZMap zmap, char *command_text) ;



/* Needs top ZMap window.... */
zMapXRemoteObj zmapControlRemoteInstallable(GdkWindow *top_zmap_window, char *name)
{
  zMapXRemoteObj xremote;
  Window id;

  id = (Window)GDK_DRAWABLE_XID(top_zmap_window);

  if((xremote = zMapXRemoteNew()) != NULL){
    zMapXRemoteInitServer(xremote, id, PACKAGE_NAME, ZMAP_DEFAULT_REQUEST_ATOM_NAME, ZMAP_DEFAULT_RESPONSE_ATOM_NAME);
    zmapControlWriteWindowIdFile(id, name);
  }
  
  
  return xremote;
}

/* Needs top ZMap window.... and name for it. It'll need to be unique */
void zmapControlWriteWindowIdFile(Window id, char *window_name)
{
  GError *g_error = NULL ;
  GIOChannel *winid_file = NULL;
  gsize bytes = 0;
  char *config_dir = NULL;
  char *path, *id_line;

  if((config_dir = zMapConfigDirGetDir()))
    {
      path = g_strdup_printf("%s/%s.%s", config_dir, window_name, WINDOWID_SUFFIX);
      if((winid_file = g_io_channel_new_file(path, "w+", &g_error)))
        {
          g_error = NULL;
          id_line = g_strdup_printf("WindowID: 0x%lx\n", id);
          printf("%s", id_line);
          g_io_channel_write_chars(winid_file, id_line, -1, &bytes, &g_error); /* should catch not writing here */
          g_io_channel_flush(winid_file, &g_error);                            /* should catch not writing here */
          g_free(id_line);
        }
      else{
        zMapLogWarning("Error doing something to path '%s', %d", path, g_error);
      }
      if(path)
        g_free(path);
    }
  else
    {
      zMapLogWarning("%s","Unable to find a configuration directory.") ;
    }
}


/* Gets called for _ALL_ property events on the top level ZMap window,
 * but only responds to the ones identifiably from xremote. */
gint zmapControlPropertyEvent(GtkWidget *top_zmap_window, GdkEventProperty *ev, gpointer user_data)
{
  gint result = FALSE ;
  ZMap zmap = (ZMap)user_data ;
  zMapXRemoteObj xremote;
  static GdkAtom request_atom  = 0 ;
  static GdkAtom response_atom = 0 ;
  static GdkAtom string_atom   = 0 ;
  GdkAtom actualType ;
  gint actualFormat ;
  gint nitems ;
  guchar *command_text = NULL ;

  xremote = zmap->xremote;
  
  if (!request_atom)
    {
      request_atom  = zMapXRemoteGdkRequestAtom(xremote); //gdk_atom_intern(ZMAP_DEFAULT_REQUEST_ATOM_NAME, FALSE) ;
      response_atom = zMapXRemoteGdkResponseAtom(xremote);//gdk_atom_intern(ZMAP_DEFAULT_RESPONSE_ATOM_NAME, FALSE) ;
      string_atom   = gdk_atom_intern("STRING", FALSE) ;
    }

  if (ev->atom != request_atom)
    {
      /* not for us */
      result = FALSE ;
    }
  else if (ev->state != GDK_PROPERTY_NEW_VALUE)
    {
      /* THIS IS PROBABLY SIMONS COMMENT FROM ACEDB, I HAVEN'T INVESTIGATED THIS.... */

      /* zero is the value for PropertyNewValue, in X.h there's no gdk equivalent that I can find. */
      /* This looks the equivalent to me */
      /* typedef enum
         {
         GDK_PROPERTY_NEW_VALUE,
         GDK_PROPERTY_DELETE
         } GdkPropertyState;
      */
      result = TRUE ;
    }
  else if (!gdk_property_get(ev->window,
			     request_atom,
			     string_atom,
			     0,
			     (65536/ sizeof(long)),
			     TRUE,
			     &actualType,
			     &actualFormat,
			     &nitems,
			     &command_text))
    {
      zMapLogCritical("%s", "X-Atom remote control : unable to read _XREMOTE_COMMAND property") ;
      
      result = TRUE ;
    }
  else if (!command_text || nitems == 0)
    {
      zMapLogCritical("%s", "X-Atom remote control : invalid data on property") ;

      result = TRUE ;
    }
  else
    { 
      guchar *response_text;
      char *copy_command ;

      copy_command = g_malloc0(nitems + 1) ;
      memcpy(copy_command, command_text, nitems);
      copy_command[nitems] = 0 ;			    /* command_text is not zero terminated */

      response_text = (guchar *)executeCommand(zmap, (char *)copy_command) ;

      zMapAssert(response_text) ;

      gdk_property_change(ev->window,
			  response_atom,
			  string_atom,
			  8,
			  GDK_PROP_MODE_REPLACE,
			  response_text,
			  strlen((const char *)response_text));
      
      g_free(response_text) ;
      g_free(copy_command) ;

      result = TRUE ;
    }

  if (command_text)
    g_free(command_text) ;

  return result ;
}


/* Handle commands sent from xremote. */
/* Return is string in the style of ZMAP_XREMOTE_REPLY_FORMAT (see ZMap/zmapXRemote.h) */
/* Building the reply string is a bit arcane in that the xremote reply strings are really format
 * strings...perhaps not ideal...., but best in the cicrumstance I guess */
static char *executeCommand(ZMap zmap, char *command_text)
{
  char *response_text = NULL ;
  char *xml_reply     = NULL ;
  char *xml_stub      = NULL ;
  int statusCode      = ZMAPXREMOTE_INTERNAL; /* Default If it doesn't get
                                                 changed there must have been a
                                                 logic error */
  
  //command prop = value ; prop = value ; prop = value

  /* We assume command is first word in text with no preceding blanks. */
  if (g_str_has_prefix(command_text, "zoom_in"))
    {
      statusCode = 200;
      zmapControlWindowDoTheZoom(zmap, 2.0) ;
      xml_reply  = g_strdup_printf(ZMAP_XREMOTE_CONTENT_XML_FORMAT, "<magnification>1:10</magnification>");
      
      //statusCode = zmapControlWindowDoTheZoom(zmap, 2.0, xml_stub) ;
      //xml_reply  = g_strdup_printf(ZMAP_XREMOTE_CONTENT_XML_FORMAT, xml_stub);
    }
  else if (g_str_has_prefix(command_text, "zoom_out"))
    {
      statusCode = 200;
      zmapControlWindowDoTheZoom(zmap, 0.5) ;
      xml_reply  = g_strdup_printf(ZMAP_XREMOTE_CONTENT_XML_FORMAT, "<magnification>1:1000</magnification>");

      //statusCode = zmapControlWindowDoTheZoom(zmap, 0.5, xml_stub) ;
      //xml_reply  = g_strdup_printf(ZMAP_XREMOTE_CONTENT_XML_FORMAT, xml_stub);
    }
  else if (g_str_has_prefix(command_text, "feature_find"))
    {
      statusCode = 200;
      findFeature(zmap, command_text);
      xml_reply  = g_strdup_printf(ZMAP_XREMOTE_CONTENT_XML_FORMAT, "<feature>Found</feature>");
      //statusCode = findFeature(zmap, command_text, xml_stub);
      //xml_reply  = g_strdup_printf(ZMAP_XREMOTE_CONTENT_XML_FORMAT, xml_stub);
    }
  else if (g_str_has_prefix(command_text, "register_client"))
    {
      statusCode = 200;
      createClient(zmap, command_text);
      xml_reply  = g_strdup_printf(ZMAP_XREMOTE_CONTENT_XML_FORMAT, "<loaded>Ok</loaded>");
      //statusCode = createClient(zmap, command_text, xml_stub);
      //xml_reply  = g_strdup_printf(ZMAP_XREMOTE_CONTENT_XML_FORMAT, xml_stub);
    }
  else
    {
      statusCode = 404;
      xml_stub   = "<error><message>unknown command</message></error>";
      xml_reply = g_strdup_printf(ZMAP_XREMOTE_CONTENT_XML_FORMAT, xml_stub);
    }

  if(xml_stub)
    g_free(xml_stub);

  response_text = g_strdup_printf(ZMAP_XREMOTE_REPLY_FORMAT, statusCode, xml_reply) ;

  if(xml_reply)
    g_free(xml_reply);

  zMapAssert(response_text) ;				    /* Must return something. */

  return response_text ;
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
