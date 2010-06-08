/*  File: zmapWindowBasicFeature.h
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
 * Last edited: May 25 12:20 2010 (edgrif)
 * Created: Wed Dec  3 08:44:06 2008 (rds)
 * CVS info:   $Id: zmapWindowBasicFeature.h,v 1.6 2010-06-08 08:31:26 mh17 Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_BASIC_FEATURE_H
#define ZMAP_WINDOW_BASIC_FEATURE_H

#include <glib-object.h>


#define ZMAP_WINDOW_BASIC_FEATURE_NAME "ZMapWindowBasicFeature"

#define ZMAP_TYPE_WINDOW_BASIC_FEATURE           (zMapWindowBasicFeatureGetType())

#if GOBJ_CAST
#define ZMAP_WINDOW_BASIC_FEATURE(obj)             ((ZMapWindowBasicFeature) obj)
#define ZMAP_WINDOW_BASIC_FEATURE_CONST(obj)     ((ZMapWindowBasicFeature const) obj)
#else
#define ZMAP_WINDOW_BASIC_FEATURE(obj)	         (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_BASIC_FEATURE, zmapWindowBasicFeature))
#define ZMAP_WINDOW_BASIC_FEATURE_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_BASIC_FEATURE, zmapWindowBasicFeature const))
#endif

#define ZMAP_WINDOW_BASIC_FEATURE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_BASIC_FEATURE, zmapWindowBasicFeatureClass))
#define ZMAP_IS_WINDOW_BASIC_FEATURE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_BASIC_FEATURE))
#define ZMAP_WINDOW_BASIC_FEATURE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_BASIC_FEATURE, zmapWindowBasicFeatureClass))


typedef enum
  {
    ZMAP_WINDOW_BASIC_0 = 0, 	/* invalid */
    ZMAP_WINDOW_BASIC_BOX,
    ZMAP_WINDOW_BASIC_GLYPH
  } ZMapWindowBasicFeatureType ;


/* Class */
typedef struct _zmapWindowBasicFeatureClassStruct  zmapWindowBasicFeatureClass, *ZMapWindowBasicFeatureClass ;

/* Instance */
typedef struct _zmapWindowBasicFeatureStruct  zmapWindowBasicFeature, *ZMapWindowBasicFeature ;

/* Public funcs */
GType zMapWindowBasicFeatureGetType(void) ;

#endif /* ZMAP_WINDOW_BASIC_FEATURE_H */
