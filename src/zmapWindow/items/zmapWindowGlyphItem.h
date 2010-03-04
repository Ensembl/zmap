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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Apr  6 14:45 2009 (rds)
 * Created: Fri Jan 16 14:01:12 2009 (rds)
 * CVS info:   $Id: zmapWindowGlyphItem.h,v 1.4 2010-03-04 15:12:22 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_GLYPH_ITEM_H
#define ZMAP_WINDOW_GLYPH_ITEM_H

#define ZMAP_WINDOW_GLYPH_ITEM_NAME "ZMapWindowGlyphItem"

#define ZMAP_TYPE_WINDOW_GLYPH_ITEM           (zMapWindowGlyphItemGetType())
#define ZMAP_WINDOW_GLYPH_ITEM(obj)	      (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_GLYPH_ITEM, zmapWindowGlyphItem))
#define ZMAP_WINDOW_GLYPH_ITEM_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_GLYPH_ITEM, zmapWindowGlyphItem const))
#define ZMAP_WINDOW_GLYPH_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_GLYPH_ITEM, zmapWindowGlyphItemClass))
#define ZMAP_IS_WINDOW_GLYPH_ITEM(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_GLYPH_ITEM))
#define ZMAP_WINDOW_GLYPH_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_GLYPH_ITEM, zmapWindowGlyphItemClass))


typedef enum
  {
    ZMAP_GLYPH_ITEM_STYLE_INVALID,
    ZMAP_GLYPH_ITEM_STYLE_SLASH_FORWARD,
    ZMAP_GLYPH_ITEM_STYLE_SLASH_REVERSE,
    ZMAP_GLYPH_ITEM_STYLE_WALKING_STICK_FORWARD,
    ZMAP_GLYPH_ITEM_STYLE_WALKING_STICK_REVERSE,
    ZMAP_GLYPH_ITEM_STYLE_TRIANGLE_FORWARD,
    ZMAP_GLYPH_ITEM_STYLE_TRIANGLE_REVERSE,
    ZMAP_GLYPH_ITEM_STYLE_TRIANGLE,
    ZMAP_GLYPH_ITEM_STYLE_DIAMOND,
    ZMAP_GLYPH_ITEM_STYLE_ASTERISK,
    ZMAP_GLYPH_ITEM_STYLE_CROSS,
    ZMAP_GLYPH_ITEM_STYLE_CIRCLE,
  } ZMapWindowGlyphItemStyle;


/* Instance */
typedef struct _zmapWindowGlyphItemStruct  zmapWindowGlyphItem, *ZMapWindowGlyphItem ;


/* Class */
typedef struct _zmapWindowGlyphItemClassStruct  zmapWindowGlyphItemClass, *ZMapWindowGlyphItemClass ;


/* Public funcs */
GType zMapWindowGlyphItemGetType(void);

int zmapWindowIsGlyphItem(FooCanvasItem *foo);

#endif /* ZMAP_WINDOW_GLYPH_ITEM_H */
