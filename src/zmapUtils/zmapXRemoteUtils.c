/*  File: zmapXRemoteUtils.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jul 16 15:47 2007 (rds)
 * Created: Tue Jul 10 09:09:53 2007 (rds)
 * CVS info:   $Id: zmapXRemoteUtils.c,v 1.1 2007-07-16 17:44:30 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtilsXRemote.h>
#include <ZMap/zmapUtils.h>

/* This data struct gets passed to the PropertyEvent Handler which
 * processes the event and if it's a valid event (for us!) execute the
 * callback with the data.
 */
typedef struct _ZMapXRemoteNotifyDataStruct
{
  ZMapXRemoteObj  xremote;      /* The xremote object which has the atoms for us to check */
  ZMapXRemoteCallback callback; /* The callback which does something when the property notify event happens */
  gpointer data;                /* The data which is passed to the callback above. */
  gulong realize_handler_id;
  char *app_name, *req_name, *res_name;
} ZMapXRemoteNotifyDataStruct, *ZMapXRemoteNotifyData;


static gboolean zmapXRemotePropertyNotifyEvent(GtkWidget *widget, GdkEventProperty *ev, gpointer user_data);
static void destroy_property_notify_data(gpointer user_data);
static void realize_handler(GtkWidget *widget, gpointer realize_data);


void zMapXRemoteInitialiseWidget(GtkWidget *widget, char *app, char *request, char *response, 
                                 ZMapXRemoteCallback callback, gpointer user_data)
{
  ZMapXRemoteNotifyData notify_data;
  ZMapXRemoteObj xremote;

  if((xremote = zMapXRemoteNew()))
    {
      notify_data           = g_new0(ZMapXRemoteNotifyDataStruct, 1);
      notify_data->xremote  = xremote;
      notify_data->callback = callback;
      notify_data->data     = user_data; 
      
      notify_data->app_name = g_strdup(app);
      notify_data->req_name = g_strdup(request);
      notify_data->res_name = g_strdup(response);

      notify_data->realize_handler_id = 0; /* make sure this really is zeroed */

      if(GTK_WIDGET_REALIZED(widget) != TRUE)
        {
          notify_data->realize_handler_id = g_signal_connect(G_OBJECT(widget), "realize",
                                                             G_CALLBACK(realize_handler), notify_data);
        }
      else
        realize_handler(widget, notify_data);
    }
}

gboolean zMapXRemoteValidateStatusCode(int *code)
{
  gboolean valid = FALSE;
  
  switch(*code)
    {
    case ZMAPXREMOTE_OK:
    case ZMAPXREMOTE_METAERROR:
    case ZMAPXREMOTE_NOCONTENT:
    case ZMAPXREMOTE_BADREQUEST:
    case ZMAPXREMOTE_FORBIDDEN:
    case ZMAPXREMOTE_UNKNOWNCMD:
    case ZMAPXREMOTE_CONFLICT:
    case ZMAPXREMOTE_PRECOND:
    case ZMAPXREMOTE_INTERNAL:
    case ZMAPXREMOTE_UNKNOWNATOM:
    case ZMAPXREMOTE_NOCREATE:
    case ZMAPXREMOTE_UNAVAILABLE:
    case ZMAPXREMOTE_TIMEDOUT:
      valid = TRUE;
      break;
    default:
      zMapLogWarning("Invalid status code '%d', returning INTERNAL_ERROR", *code);
      *code = ZMAPXREMOTE_INTERNAL;
      valid = FALSE;
      break;
    }

  return valid;
}

/* user_data _MUST_ == ZMapXRemoteParseCommandData */
gboolean zMapXRemoteXMLGenericClientStartCB(gpointer user_data, 
                                            ZMapXMLElement client_element,
                                            ZMapXMLParser parser)
{
  ZMapXRemoteParseCommandData parsing_data = (ZMapXRemoteParseCommandData)user_data;
  ClientParameters client_params = &(parsing_data->common.client_params);
  ZMapXMLAttribute xid_attr, req_attr, res_attr;

  if((xid_attr = zMapXMLElementGetAttributeByName(client_element, "xwid")) != NULL)
    {
      char *xid;
      xid = (char *)g_quark_to_string(zMapXMLAttributeGetValue(xid_attr));
      client_params->xid = strtoul(xid, (char **)NULL, 16);
    }
  else
    zMapXMLParserRaiseParsingError(parser, "id is a required attribute for client.");

  if((req_attr  = zMapXMLElementGetAttributeByName(client_element, "request_atom")) != NULL)
    client_params->request = zMapXMLAttributeGetValue(req_attr);
  if((res_attr  = zMapXMLElementGetAttributeByName(client_element, "response_atom")) != NULL)
    client_params->response = zMapXMLAttributeGetValue(res_attr);

  return FALSE;
}


/* INTERNAL */

static gboolean zmapXRemotePropertyNotifyEvent(GtkWidget *widget, GdkEventProperty *ev, gpointer user_data)
{
  ZMapXRemoteNotifyData notify_data = (ZMapXRemoteNotifyData)user_data;
  Atom event_atom;
  gboolean result = FALSE;

  event_atom = gdk_x11_atom_to_xatom(ev->atom);

  if(zMapXRemoteHandlePropertyNotify(notify_data->xremote, 
                                     event_atom, ev->state, 
                                     notify_data->callback, 
                                     notify_data->data))
    result = TRUE;

  return result;
}

static void destroy_property_notify_data(gpointer user_data)
{
  ZMapXRemoteNotifyData notify_data = (ZMapXRemoteNotifyData)user_data;

  if(notify_data->xremote)
    zMapXRemoteDestroy(notify_data->xremote);
  notify_data->callback  = NULL;
  notify_data->data      = NULL;

  if(notify_data->app_name)
    g_free(notify_data->app_name);
  if(notify_data->req_name)
    g_free(notify_data->req_name);
  if(notify_data->res_name)
    g_free(notify_data->res_name);

  g_free(notify_data);

  return ;
}

static void realize_handler(GtkWidget *widget, gpointer realize_data)
{
  ZMapXRemoteNotifyData notify_data = (ZMapXRemoteNotifyData)realize_data;

  externalPerl = FALSE;
 
  if(notify_data->realize_handler_id)
    {
      g_signal_handler_disconnect(G_OBJECT(widget), notify_data->realize_handler_id);
      notify_data->realize_handler_id = 0;
    }

  zMapAssert(GTK_WIDGET_NO_WINDOW(widget) == FALSE);

  if(GTK_WIDGET_REALIZED(widget) && !(GTK_WIDGET_NO_WINDOW(widget)))
    {
      Window id = (Window)GDK_DRAWABLE_XID(widget->window);
      
      /* Moving this (add_events) BEFORE the call to InitServer stops
       * some Xlib BadWindow errors (turn on debugging in zmapXRemote 2 c
       * them).  This doesn't feel right, but I couldn't bear the
       * thought of having to store a handler id for an expose_event
       * in the zmap struct just so I could use it over realize in
       * much the same way as the zmapWindow code does.  This
       * definitely points to some problem with GTK2.  The Widget
       * reports it's realized GTK_WIDGET_REALIZED(widget) has a
       * window id, but then the XChangeProperty() fails in the
       * zmapXRemote code.  Hmmm.  If this continues to be a problem
       * then I'll change it to use expose.  Only appeared on Linux. */
      /* Makes sure we actually get the events!!!! Use add_events as set_events needs to be done BEFORE realize */
      gtk_widget_add_events(widget, GDK_PROPERTY_CHANGE_MASK) ;
      
      /*
        zMapXRemoteInitServer(xremote, id, PACKAGE_NAME, 
                            ZMAP_DEFAULT_REQUEST_ATOM_NAME, 
                            ZMAP_DEFAULT_RESPONSE_ATOM_NAME);
      */
      zMapXRemoteInitServer(notify_data->xremote, id, 
                            notify_data->app_name, 
                            notify_data->req_name, 
                            notify_data->res_name);

      g_signal_connect_data(G_OBJECT(widget), "property_notify_event",
                            G_CALLBACK(zmapXRemotePropertyNotifyEvent), (gpointer)notify_data,
                            (GClosureNotify)destroy_property_notify_data, G_CONNECT_AFTER
                            );
    }

  return ;
}

