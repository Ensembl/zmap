/*  File: zmapWindowSequenceFeature.h
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
 * Last edited: Jul 27 12:38 2009 (rds)
 * Created: Wed Dec  3 08:44:06 2008 (rds)
 * CVS info:   $Id: zmapWindowSequenceFeature.h,v 1.4 2009-07-27 12:09:29 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_SEQUENCE_FEATURE_H
#define ZMAP_WINDOW_SEQUENCE_FEATURE_H

#include <glib-object.h>

#define ZMAP_WINDOW_SEQUENCE_FEATURE_NAME "ZMapWindowSequenceFeature"


#define ZMAP_TYPE_WINDOW_SEQUENCE_FEATURE           (zMapWindowSequenceFeatureGetType())
#define ZMAP_WINDOW_SEQUENCE_FEATURE(obj)	     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_SEQUENCE_FEATURE, zmapWindowSequenceFeature))
#define ZMAP_WINDOW_SEQUENCE_FEATURE_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_SEQUENCE_FEATURE, zmapWindowSequenceFeature const))
#define ZMAP_WINDOW_SEQUENCE_FEATURE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_SEQUENCE_FEATURE, zmapWindowSequenceFeatureClass))
#define ZMAP_IS_WINDOW_SEQUENCE_FEATURE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_SEQUENCE_FEATURE))
#define ZMAP_WINDOW_SEQUENCE_FEATURE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_SEQUENCE_FEATURE, zmapWindowSequenceFeatureClass))


enum
  {
    ZMAP_WINDOW_SEQUENCE_0 = 0, /* invalid */
  };


/* Instance */
typedef struct _zmapWindowSequenceFeatureStruct  zmapWindowSequenceFeature, *ZMapWindowSequenceFeature ;


/* Class */
typedef struct _zmapWindowSequenceFeatureClassStruct  zmapWindowSequenceFeatureClass, *ZMapWindowSequenceFeatureClass ;

typedef gboolean (* ZMapWindowSequenceFeatureSelectionCB)(ZMapWindowSequenceFeature sequence_feature,
							  int text_first_char, int text_final_char);


/* Public funcs */
GType zMapWindowSequenceFeatureGetType(void);

gboolean zMapWindowSequenceFeatureSelectByRegion(ZMapWindowSequenceFeature sequence_feature,
						 int region_start, int region_end,
						 int select_flags_ignored_atm);



#endif /* ZMAP_WINDOW_SEQUENCE_FEATURE_H */
