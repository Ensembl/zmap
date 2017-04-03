/*  File: zmapWindowNavigator.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_NAVIGATOR_H
#define ZMAP_WINDOW_NAVIGATOR_H

#include <ZMap/zmapWindow.hpp>

typedef struct _ZMapWindowNavigatorStruct *ZMapWindowNavigator;

typedef void (*ZMapWindowNavigatorValueChanged)(void *user_data, double start, double end) ;
typedef void (*ZMapWindowNavigatorFunction)(ZMapWindowNavigator navigator);


class ZMapStyleTree ;


typedef struct _ZMapNavigatorCallbackStruct
{
#if RUN_AROUND
  ZMapWindowNavigatorValueChanged valueCB;
#endif
  ZMapWindowNavigatorValueChanged widthCB;
  ZMapWindowNavigatorFunction resizeCB;

} ZMapWindowNavigatorCallbackStruct, *ZMapWindowNavigatorCallback;


ZMapWindowNavigator zMapWindowNavigatorCreate(GtkWidget *canvas_widget);
void zMapWindowNavigatorFocus(ZMapWindowNavigator navigate,
                              gboolean true_eq_focus);
void zMapWindowNavigatorSetCurrentWindow(ZMapWindowNavigator navigate, ZMapWindow window);
void zMapWindowNavigatorMergeInFeatureSetNames(ZMapWindowNavigator navigate,
                                               GList *navigator_sets);
void zMapWindowNavigatorSetStrand(ZMapWindowNavigator navigate, gboolean is_reversed);
void zMapWindowNavigatorDrawFeatures(ZMapWindowNavigator navigate,
                                     ZMapFeatureContext  full_context,
				     ZMapStyleTree       &styles);
void zmapWindowNavigatorLocusRedraw(ZMapWindowNavigator navigate);

void zMapWindowNavigatorDrawLocator(ZMapWindowNavigator navigate,
                                    double top, double bottom);
void zMapWindowNavigatorReset(ZMapWindowNavigator navigate);
void zMapWindowNavigatorDestroy(ZMapWindowNavigator navigate);



/* Widget Functions */

GtkWidget *zMapWindowNavigatorCreateCanvas(ZMapWindowNavigatorCallback callbacks, gpointer user_data);
void zMapWindowNavigatorPackDimensions(GtkWidget *widget, double *width, double *height);

gboolean zMapNavigatorWidgetGetCanvasTextSize(FooCanvas *canvas,int *width_out,int *height_out);

#endif /*  ZMAP_WINDOW_NAVIGATOR_H  */
