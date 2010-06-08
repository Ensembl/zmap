/*  File: zmapWindowNavigator.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun 10 09:44 2009 (rds)
 * Created: Thu Sep  7 09:10:32 2006 (rds)
 * CVS info:   $Id: zmapWindowNavigator.h,v 1.11 2010-06-08 08:31:23 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_NAVIGATOR_H
#define ZMAP_WINDOW_NAVIGATOR_H

#include <ZMap/zmapWindow.h>


typedef void (*ZMapWindowNavigatorValueChanged)(void *user_data, double start, double end) ;

typedef struct _ZMapWindowNavigatorStruct *ZMapWindowNavigator;

typedef struct _ZMapNavigatorCallbackStruct
{
  ZMapWindowNavigatorValueChanged valueCB;
  ZMapWindowNavigatorValueChanged widthCB;
} ZMapWindowNavigatorCallbackStruct, *ZMapWindowNavigatorCallback;


ZMapWindowNavigator zMapWindowNavigatorCreate(GtkWidget *canvas_widget);
void zMapWindowNavigatorFocus(ZMapWindowNavigator navigate,
                              gboolean true_eq_focus);
void zMapWindowNavigatorSetCurrentWindow(ZMapWindowNavigator navigate, ZMapWindow window);
void zMapWindowNavigatorMergeInFeatureSetNames(ZMapWindowNavigator navigate,
                                               GList *navigator_sets);
void zMapWindowNavigatorSetStrand(ZMapWindowNavigator navigate, gboolean is_revesed);
void zMapWindowNavigatorDrawFeatures(ZMapWindowNavigator navigate,
                                     ZMapFeatureContext  full_context,
				     GHashTable              *styles);
void zMapWindowNavigatorDrawLocator(ZMapWindowNavigator navigate,
                                    double top, double bottom);
void zMapWindowNavigatorReset(ZMapWindowNavigator navigate);
void zMapWindowNavigatorDestroy(ZMapWindowNavigator navigate);



/* Widget Functions */

GtkWidget *zMapWindowNavigatorCreateCanvas(ZMapWindowNavigatorCallback callbacks, gpointer user_data);
void zMapWindowNavigatorPackDimensions(GtkWidget *widget, double *width, double *height);


#endif /*  ZMAP_WINDOW_NAVIGATOR_H  */
