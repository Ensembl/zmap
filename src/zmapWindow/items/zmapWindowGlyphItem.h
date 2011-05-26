/*  File: zmapWindowGlyphItem.h
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Apr  6 14:45 2009 (rds)
 * Created: Fri Jan 16 14:01:12 2009 (rds)
 * CVS info:   $Id: zmapWindowGlyphItem.h,v 1.8 2010-06-14 15:40:18 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_GLYPH_ITEM_H
#define ZMAP_WINDOW_GLYPH_ITEM_H

#include <ZMap/zmapStyle.h>         // for shape struct and style data


#define ZMAP_WINDOW_GLYPH_ITEM_NAME "ZMapWindowGlyphItem"

#define ZMAP_TYPE_WINDOW_GLYPH_FEATURE        (zMapWindowGlyphItemGetType())

#if GOBJ_CAST
#define ZMAP_WINDOW_GLYPH_ITEM(obj)       ((ZMapWindowGlyphItem) obj)
#define ZMAP_WINDOW_GLYPH_ITEM_CONST(obj) ((ZMapWindowGlyphItem const) obj)
#else
#define ZMAP_WINDOW_GLYPH_ITEM(obj)	      (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_GLYPH_FEATURE, zmapWindowGlyphItem))
#define ZMAP_WINDOW_GLYPH_ITEM_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_GLYPH_FEATURE, zmapWindowGlyphItem const))
#endif
#define ZMAP_WINDOW_GLYPH_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_GLYPH_FEATURE, zmapWindowGlyphItemClass))
#define ZMAP_IS_WINDOW_GLYPH_ITEM(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_GLYPH_FEATURE))
#define ZMAP_WINDOW_GLYPH_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_GLYPH_FEATURE, zmapWindowGlyphItemClass))





/* Instance */
typedef struct _zmapWindowGlyphItemStruct  zmapWindowGlyphItem, *ZMapWindowGlyphItem ;


/* Class */
typedef struct _zmapWindowGlyphItemClassStruct  zmapWindowGlyphItemClass, *ZMapWindowGlyphItemClass ;


/* Public funcs */
GType zMapWindowGlyphItemGetType(void);

int zmapWindowIsGlyphItem(FooCanvasItem *foo);

ZMapWindowGlyphItem zMapWindowGlyphItemCreate(FooCanvasGroup *parent,
      ZMapFeatureTypeStyle style, int which,
      double x_coord, double y_coord, double score,gboolean rev_strand);


#endif /* ZMAP_WINDOW_GLYPH_ITEM_H */
