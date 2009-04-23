/*  File: zmapWindowTextFeature.h
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
 * Last edited: Jan 13 13:40 2009 (rds)
 * Created: Wed Dec  3 08:44:06 2008 (rds)
 * CVS info:   $Id: zmapWindowTextFeature.h,v 1.1 2009-04-23 09:12:46 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_TEXT_FEATURE_H
#define ZMAP_WINDOW_TEXT_FEATURE_H

#define ZMAP_WINDOW_TEXT_FEATURE_NAME "ZMapWindowTextFeature"

#define ZMAP_TYPE_WINDOW_TEXT_FEATURE           (zMapWindowTextFeatureGetType())
#define ZMAP_WINDOW_TEXT_FEATURE(obj)	         (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_TEXT_FEATURE, zmapWindowTextFeature))
#define ZMAP_WINDOW_TEXT_FEATURE_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_TEXT_FEATURE, zmapWindowTextFeature const))
#define ZMAP_WINDOW_TEXT_FEATURE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_TEXT_FEATURE, zmapWindowTextFeatureClass))
#define ZMAP_IS_WINDOW_TEXT_FEATURE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_TEXT_FEATURE))
#define ZMAP_WINDOW_TEXT_FEATURE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_TEXT_FEATURE, zmapWindowTextFeatureClass))


/* Instance */
typedef struct _zmapWindowTextFeatureStruct  zmapWindowTextFeature, *ZMapWindowTextFeature ;


/* Class */
typedef struct _zmapWindowTextFeatureClassStruct  zmapWindowTextFeatureClass, *ZMapWindowTextFeatureClass ;


/* Public funcs */
GType zMapWindowTextFeatureGetType(void);


#endif /* ZMAP_WINDOW_TEXT_FEATURE_H */
