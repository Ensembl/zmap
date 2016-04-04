/*  File: zmapWebPages.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Contains references to zmap web pages for current
 *              code release.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WEBPAGES_H
#define ZMAP_WEBPAGES_H


/* The docs should be installed alongside the executable in the share directory.
 * The share directory lives in the same parent directory as the bin directory
 * where the executable is, so we need to find the bin directory and then use
 * this relative path */
#define ZMAP_LOCAL_DOC_PATH "../share/doc/zmap"

/* Reorganising the zmap_update_web.sh and the directories, together
 * with inertia of the way the last release code works, means that the
 * Release_notes dir is no longer under the DOC_URL dir. */


#define ZMAPWEB_RELEASE_NOTES "release_notes.shtml"

#define ZMAPWEB_HELP_DOC  "index.shtml"
#define ZMAPWEB_HELP_QUICK_START  "zmap_quick_start.shtml"
#define ZMAPWEB_HELP_ALIGNMENT_SECTION "user_interface.shtml"
/* gb10: for some reason the open command fails with this: user_interface.shtml#alignment_display */

#define ZMAPWEB_KEYBOARD_DOC  "keyboard_mouse.shtml"
#define ZMAPWEB_FILTER_DOC  "filter_window.shtml"
#define ZMAPWEB_MANUAL  "ZMap_User_Manual.pdf"


#endif /* ZMAP_WEBPAGES_H */
