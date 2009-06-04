/*  File: zmapWindowContainerChildren.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * Last edited: Jun  4 08:32 2009 (rds)
 * Created: Wed Dec  3 08:21:03 2008 (rds)
 * CVS info:   $Id: zmapWindowContainerChildren.h,v 1.3 2009-06-04 09:13:04 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_CONTAINER_CHILDREN_H
#define ZMAP_WINDOW_CONTAINER_CHILDREN_H

#include <glib-object.h>
#include <libfoocanvas/libfoocanvas.h>


#ifndef ZMAP_WINDOW_CONTAINER_H

/* Used by zmapWindowContainer calls for going forward/backward through lists. */
typedef enum
  {
    ZMAPCONTAINER_ITEM_FIRST = -1,			    /* These two _must_ be < 0 */
    ZMAPCONTAINER_ITEM_LAST = -2,
    ZMAPCONTAINER_ITEM_PREV,
    ZMAPCONTAINER_ITEM_NEXT
  } ZMapContainerItemDirection ;

typedef gboolean (*zmapWindowContainerItemTestCallback)(FooCanvasItem *item, gpointer user_data) ;

#endif

/* ==== Features ==== */

#define ZMAP_WINDOW_CONTAINER_FEATURES_NAME 	"ZMapWindowContainerFeatures"


#define ZMAP_TYPE_CONTAINER_FEATURES           (zmapWindowContainerFeaturesGetType())
#define ZMAP_CONTAINER_FEATURES(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_FEATURES, zmapWindowContainerFeatures))
#define ZMAP_CONTAINER_FEATURES_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_FEATURES, zmapWindowContainerFeatures const))
#define ZMAP_CONTAINER_FEATURES_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_CONTAINER_FEATURES, zmapWindowContainerFeaturesClass))
#define ZMAP_IS_CONTAINER_FEATURES(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_CONTAINER_FEATURES))
#define ZMAP_CONTAINER_FEATURES_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_CONTAINER_FEATURES, zmapWindowContainerFeaturesClass))


/* Instance */
typedef struct _zmapWindowContainerFeaturesStruct  zmapWindowContainerFeatures, *ZMapWindowContainerFeatures ;


/* Class */
typedef struct _zmapWindowContainerFeaturesClassStruct  zmapWindowContainerFeaturesClass, *ZMapWindowContainerFeaturesClass ;


/* Public funcs */
GType zmapWindowContainerFeaturesGetType(void);

ZMapWindowContainerFeatures zMapWindowContainerFeaturesCreate(FooCanvasGroup *parent);
ZMapWindowContainerFeatures zMapWindowContainerFeaturesDestroy(ZMapWindowContainerFeatures canvas_item);


/* ==== Underlay ==== */

#define ZMAP_WINDOW_CONTAINER_UNDERLAY_NAME 	"ZMapWindowContainerUnderlay"


#define ZMAP_TYPE_CONTAINER_UNDERLAY           (zmapWindowContainerUnderlayGetType())
#define ZMAP_CONTAINER_UNDERLAY(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_UNDERLAY, zmapWindowContainerUnderlay))
#define ZMAP_CONTAINER_UNDERLAY_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_UNDERLAY, zmapWindowContainerUnderlay const))
#define ZMAP_CONTAINER_UNDERLAY_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_CONTAINER_UNDERLAY, zmapWindowContainerUnderlayClass))
#define ZMAP_IS_CONTAINER_UNDERLAY(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_CONTAINER_UNDERLAY))
#define ZMAP_CONTAINER_UNDERLAY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_CONTAINER_UNDERLAY, zmapWindowContainerUnderlayClass))


/* Instance */
typedef struct _zmapWindowContainerUnderlayStruct  zmapWindowContainerUnderlay, *ZMapWindowContainerUnderlay ;


/* Class */
typedef struct _zmapWindowContainerUnderlayClassStruct  zmapWindowContainerUnderlayClass, *ZMapWindowContainerUnderlayClass ;


/* Public funcs */
GType zmapWindowContainerUnderlayGetType(void);
gboolean zmapWindowContainerUnderlayMaximiseItems(ZMapWindowContainerUnderlay underlay,
						  double x1, double y1,
						  double x2, double y2);
ZMapWindowContainerUnderlay zMapWindowContainerUnderlayCreate(FooCanvasGroup *parent);
ZMapWindowContainerUnderlay zMapWindowContainerUnderlayDestroy(ZMapWindowContainerUnderlay canvas_item);


/* ==== Overlay ==== */

#define ZMAP_WINDOW_CONTAINER_OVERLAY_NAME 	"ZMapWindowContainerOverlay"


#define ZMAP_TYPE_CONTAINER_OVERLAY           (zmapWindowContainerOverlayGetType())
#define ZMAP_CONTAINER_OVERLAY(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_OVERLAY, zmapWindowContainerOverlay))
#define ZMAP_CONTAINER_OVERLAY_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_OVERLAY, zmapWindowContainerOverlay const))
#define ZMAP_CONTAINER_OVERLAY_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_CONTAINER_OVERLAY, zmapWindowContainerOverlayClass))
#define ZMAP_IS_CONTAINER_OVERLAY(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_CONTAINER_OVERLAY))
#define ZMAP_CONTAINER_OVERLAY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_CONTAINER_OVERLAY, zmapWindowContainerOverlayClass))


/* Instance */
typedef struct _zmapWindowContainerOverlayStruct  zmapWindowContainerOverlay, *ZMapWindowContainerOverlay ;


/* Class */
typedef struct _zmapWindowContainerOverlayClassStruct  zmapWindowContainerOverlayClass, *ZMapWindowContainerOverlayClass ;


/* Public funcs */
GType zmapWindowContainerOverlayGetType(void);
gboolean zmapWindowContainerOverlayMaximiseItems(ZMapWindowContainerOverlay overlay,
						 double x1, double y1,
						 double x2, double y2);
ZMapWindowContainerOverlay zMapWindowContainerOverlayCreate(FooCanvasGroup *parent);
ZMapWindowContainerOverlay zMapWindowContainerOverlayDestroy(ZMapWindowContainerOverlay canvas_item);


/* ==== Background ==== */

#define ZMAP_WINDOW_CONTAINER_BACKGROUND_NAME 	"ZMapWindowContainerBackground"


#define ZMAP_TYPE_CONTAINER_BACKGROUND           (zmapWindowContainerBackgroundGetType())
#define ZMAP_CONTAINER_BACKGROUND(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_BACKGROUND, zmapWindowContainerBackground))
#define ZMAP_CONTAINER_BACKGROUND_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CONTAINER_BACKGROUND, zmapWindowContainerBackground const))
#define ZMAP_CONTAINER_BACKGROUND_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_CONTAINER_BACKGROUND, zmapWindowContainerBackgroundClass))
#define ZMAP_IS_CONTAINER_BACKGROUND(obj)	 (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_CONTAINER_BACKGROUND))
#define ZMAP_CONTAINER_BACKGROUND_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_CONTAINER_BACKGROUND, zmapWindowContainerBackgroundClass))


/* Instance */
typedef struct _zmapWindowContainerBackgroundStruct  zmapWindowContainerBackground, *ZMapWindowContainerBackground ;


/* Class */
typedef struct _zmapWindowContainerBackgroundClassStruct  zmapWindowContainerBackgroundClass, *ZMapWindowContainerBackgroundClass ;


/* Public funcs */
GType zmapWindowContainerBackgroundGetType(void);
void zmapWindowContainerBackgroundSetColour(ZMapWindowContainerBackground background,
					    GdkColor *new_colour);
void zmapWindowContainerBackgroundResetColour(ZMapWindowContainerBackground background);

ZMapWindowContainerBackground zMapWindowContainerBackgroundCreate(FooCanvasGroup *parent);
ZMapWindowContainerBackground zMapWindowContainerBackgroundDestroy(ZMapWindowContainerBackground canvas_item);


#endif /* ZMAP_WINDOW_CONTAINER_CHILDREN_H */
