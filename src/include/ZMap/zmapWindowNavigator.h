/*  File: zmapWindowNavigator.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Feb  4 16:24 2009 (rds)
 * Created: Thu Sep  7 09:10:32 2006 (rds)
 * CVS info:   $Id: zmapWindowNavigator.h,v 1.8 2009-02-04 16:25:51 rds Exp $
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
                              gboolean raise_to_top,
                              double *x1_inout, double *y1_inout, 
                              double *x2_inout, double *y2_inout);
void zMapWindowNavigatorSetCurrentWindow(ZMapWindowNavigator navigate, ZMapWindow window);
void zMapWindowNavigatorMergeInFeatureSetNames(ZMapWindowNavigator navigate, 
                                               GList *navigator_sets);
void zMapWindowNavigatorSetStrand(ZMapWindowNavigator navigate, gboolean is_revesed);
void zMapWindowNavigatorDrawFeatures(ZMapWindowNavigator navigate, 
                                     ZMapFeatureContext  full_context,
				     GData              *styles);
void zMapWindowNavigatorDrawLocator(ZMapWindowNavigator navigate,
                                    double top, double bottom);
void zMapWindowNavigatorReset(ZMapWindowNavigator navigate);
void zMapWindowNavigatorDestroy(ZMapWindowNavigator navigate);



/* Widget Functions */

GtkWidget *zMapWindowNavigatorCreateCanvas(ZMapWindowNavigatorCallback callbacks, gpointer user_data);
void zMapWindowNavigatorPackDimensions(GtkWidget *widget, double *width, double *height);


#endif /*  ZMAP_WINDOW_NAVIGATOR_H  */
