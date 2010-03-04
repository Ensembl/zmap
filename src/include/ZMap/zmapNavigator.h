/*  File: zmapNavigator.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * Description: External interface to the Navigator for sequence views.
 *              
 * HISTORY:
 * Last edited: Aug  1 12:24 2007 (rds)
 * Created: Fri Jan  7 14:38:22 2005 (edgrif)
 * CVS info:   $Id: zmapNavigator.h,v 1.7 2010-03-04 15:15:08 mh17 Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_NAVIGATOR_H
#define ZMAP_NAVIGATOR_H


typedef struct _ZMapNavStruct *ZMapNavigator ;


/* callback registered with zmapnavigator, gets called when user releases the scrollbar for
 * the window locator. */
typedef void (*ZMapNavigatorScrollValue)(void *user_data, double start, double end) ;


ZMapNavigator zMapNavigatorCreate(GtkWidget **top_widg_out, GtkWidget **canvas_out) ;
void zMapNavigatorSetWindowCallback(ZMapNavigator navigator,
					   ZMapNavigatorScrollValue cb_func, void *user_data) ;
int zMapNavigatorGetMaxWidth(ZMapNavigator navigator);
int zMapNavigatorSetWindowPos(ZMapNavigator navigator, double top_pos, double bot_pos) ;
void zMapNavigatorSetView(ZMapNavigator navigator, ZMapFeatureContext features,
			  double top, double bottom) ;
void zMapNavigatorDestroy(ZMapNavigator navigator) ;




#endif /* !ZMAP_NAVIGATOR_H */
