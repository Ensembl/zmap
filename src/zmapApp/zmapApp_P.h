/*  File: zmapApp.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * and was written by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Nov  5 10:53 2004 (edgrif)
 * Created: Thu Jul 24 14:35:41 2003 (edgrif)
 * CVS info:   $Id: zmapApp_P.h,v 1.8 2004-11-05 14:22:06 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_APP_PRIV_H
#define ZMAP_APP_PRIV_H

#include <gtk/gtk.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapManager.h>


/* Minimum GTK version supported. */
enum {ZMAP_GTK_MAJOR = 2, ZMAP_GTK_MINOR = 2, ZMAP_GTK_MICRO = 1} ;


/* Overall application control struct. */
typedef struct
{
  GtkWidget *app_widg ;

  GtkWidget *sequence_widg ;
  GtkWidget *start_widg ;
  GtkWidget *end_widg ;

  GtkWidget *clist_widg ;

  ZMapManager zmap_manager ;
  ZMap selected_zmap ;

  ZMapLog logger ;

} ZMapAppContextStruct, *ZMapAppContext ;


/* cols in connection list. */
enum {ZMAP_NUM_COLS = 4} ;





int zmapMainMakeAppWindow(int argc, char *argv[]) ;
GtkWidget *zmapMainMakeMenuBar(ZMapAppContext app_context) ;
GtkWidget *zmapMainMakeConnect(ZMapAppContext app_context) ;
GtkWidget *zmapMainMakeManage(ZMapAppContext app_context) ;
void zmapAppCreateZMap(ZMapAppContext app_context, char *sequence, int start, int end) ;
void zmapAppExit(ZMapAppContext app_context) ;


#endif /* !ZMAP_APP_PRIV_H */
