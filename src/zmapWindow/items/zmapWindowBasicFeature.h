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
 * Last edited: Feb 16 09:51 2010 (edgrif)
 * Created: Wed Dec  3 08:44:06 2008 (rds)
 * CVS info:   $Id: zmapWindowBasicFeature.h,v 1.4 2010-03-04 15:11:47 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_BASIC_FEATURE_H
#define ZMAP_WINDOW_BASIC_FEATURE_H

#include <glib-object.h>


typedef enum
  {
    ZMAP_WINDOW_BASIC_0 = 0, 	/* invalid */
    ZMAP_WINDOW_BASIC_BOX,
    ZMAP_WINDOW_BASIC_GLYPH
  } ZMapWindowBasicFeatureType ;



#define ZMAP_WINDOW_BASIC_FEATURE_NAME "ZMapWindowBasicFeature"

#define ZMAP_TYPE_WINDOW_BASIC_FEATURE           (zMapWindowBasicFeatureGetType())
#define ZMAP_WINDOW_BASIC_FEATURE(obj)	         (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_BASIC_FEATURE, zmapWindowBasicFeature))
#define ZMAP_WINDOW_BASIC_FEATURE_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_BASIC_FEATURE, zmapWindowBasicFeature const))
#define ZMAP_WINDOW_BASIC_FEATURE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_BASIC_FEATURE, zmapWindowBasicFeatureClass))
#define ZMAP_IS_WINDOW_BASIC_FEATURE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_BASIC_FEATURE))
#define ZMAP_WINDOW_BASIC_FEATURE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_BASIC_FEATURE, zmapWindowBasicFeatureClass))

/* Instance */
typedef struct _zmapWindowBasicFeatureStruct  zmapWindowBasicFeature, *ZMapWindowBasicFeature ;


/* Class */
typedef struct _zmapWindowBasicFeatureClassStruct  zmapWindowBasicFeatureClass, *ZMapWindowBasicFeatureClass ;


/* Public funcs */
GType zMapWindowBasicFeatureGetType(void);

#endif /* ZMAP_WINDOW_BASIC_FEATURE_H */
