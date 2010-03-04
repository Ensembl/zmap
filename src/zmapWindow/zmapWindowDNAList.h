/*  File: zmapGUITreeView.h
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
 * Last edited: Jun 16 15:11 2008 (rds)
 * Created: Thu May 22 10:45:05 2008 (rds)
 * CVS info:   $Id: zmapWindowDNAList.h,v 1.4 2010-03-04 15:12:48 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_WINDOWDNALIST_H__
#define __ZMAP_WINDOWDNALIST_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <ZMap/zmapGUITreeView.h>

#define ZMAP_WINDOWDNALIST_MATCH_COLUMN_NAME   "Match"
#define ZMAP_WINDOWDNALIST_START_COLUMN_NAME   "-start-"
#define ZMAP_WINDOWDNALIST_END_COLUMN_NAME     "-end-"
#define ZMAP_WINDOWDNALIST_STRAND_COLUMN_NAME  "Strand"
#define ZMAP_WINDOWDNALIST_FRAME_COLUMN_NAME   "Frame"
#define ZMAP_WINDOWDNALIST_SEQTYPE_COLUMN_NAME "-match-type-"

#define ZMAP_WINDOWDNALIST_STRAND_ENUM_COLUMN_NAME  "-strand-enum-"
#define ZMAP_WINDOWDNALIST_FRAME_ENUM_COLUMN_NAME   "-frame-enum-"


/*
 * Type checking and casting macros
 */

#define ZMAP_TYPE_WINDOWDNALIST           (zMapWindowDNAListGetType())
#define ZMAP_WINDOWDNALIST(obj)	          G_TYPE_CHECK_INSTANCE_CAST((obj), zMapWindowDNAListGetType(), zmapWindowDNAList)
#define ZMAP_WINDOWDNALIST_CONST(obj)	  G_TYPE_CHECK_INSTANCE_CAST((obj), zMapWindowDNAListGetType(), zmapWindowDNAList const)
#define ZMAP_WINDOWDNALIST_CLASS(klass)	  G_TYPE_CHECK_CLASS_CAST((klass),  zMapWindowDNAListGetType(), zmapWindowDNAListClass)
#define ZMAP_IS_WINDOWDNALIST(obj)	  G_TYPE_CHECK_INSTANCE_TYPE((obj), zMapWindowDNAListGetType())
#define ZMAP_WINDOWDNALIST_GET_CLASS(obj) G_TYPE_INSTANCE_GET_CLASS((obj),  zMapWindowDNAListGetType(), zmapWindowDNAListClass)


/*
 * Main object structure
 */

typedef struct _zmapWindowDNAListStruct *ZMapWindowDNAList;

typedef struct _zmapWindowDNAListStruct  zmapWindowDNAList;

/*
 * Class definition
 */
typedef struct _zmapWindowDNAListClassStruct *ZMapWindowDNAListClass;

typedef struct _zmapWindowDNAListClassStruct  zmapWindowDNAListClass;

/*
 * Public methods
 */
GType zMapWindowDNAListGetType (void);
ZMapWindowDNAList zMapWindowDNAListCreate(void);
void zMapWindowDNAListAddMatch(ZMapWindowDNAList zmap_tv,
			       ZMapDNAMatch match);
void zMapWindowDNAListAddMatches(ZMapWindowDNAList zmap_tv,
				 GList *list_of_matches);


#endif /* __ZMAP_GUITREEVIEW_H__ */
