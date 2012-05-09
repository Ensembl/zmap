/*  File: zmapConfigKeyboard.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_CONFIG_KEYBOARD_H
#define ZMAP_CONFIG_KEYBOARD_H

#include <gdk/gdk.h>

/*!
 * @addtogroup zmapconfig
 * @{
 */

/*!
 * \brief 
 */
enum
  {
    ZMAP_MOUSE_LEFT_BUTTON   = 1,
    ZMAP_MOUSE_MIDDLE_BUTTON = 2,
    ZMAP_MOUSE_RIGHT_BUTTON  = 3
  };


/*!
 *
 */
enum                            /* modify behaviour to ... */
  {
    /* ... do XXXXXXXXXXX when shift key pressed */
    ZMAP_KEYBOARD_DO_ACTION_MASK     = GDK_SHIFT_MASK,

    /* ... do select when control key pressed */
    ZMAP_KEYBOARD_SELECT_ACTION_MASK = GDK_CONTROL_MASK,

    /* ... do XXXXXXXXXXX when lock (num or caps??) pressed */
    ZMAP_KEYBOARD_LOCKING_MASK       = GDK_LOCK_MASK,

    /* ... do SUPER clever stuff when (usually ALT) pressed */
    ZMAP_KEYBOARD_SUPER_USER_MASK    = GDK_MOD1_MASK,

    /* ... do everything!!! */
    ZMAP_KEYBOARD_EVERYTHING_MASK    = GDK_MODIFIER_MASK
  };
/* These masks make it a little simpler to see/make consistent the behaviour of keyboard shortcut */


/* <gdk/gdkkeysyms.h> */


/*! @} end of zmapconfig docs. */

#endif /* ZMAP_CONFIG_KEYBOARD_H */
