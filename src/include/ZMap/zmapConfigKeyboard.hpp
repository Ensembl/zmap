/*  File: zmapConfigKeyboard.h
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
