/*  File: zmapWebPages.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Contains references to zmap web pages for current
 *              code release.
 *
 * HISTORY:
 * Last edited: Jul 18 10:14 2008 (edgrif)
 * Created: Wed Oct 18 11:19:12 2006 (edgrif)
 * CVS info:   $Id: zmapWebPages.h,v 1.69 2011-05-13 13:53:38 zmap Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WEBPAGES_H
#define ZMAP_WEBPAGES_H

/* http://wwwdev.sanger.ac.uk/Software/analysis/ZMap/doc/user_interface.shtml */

#define ZMAPWEB_URL "http://wwwdev.sanger.ac.uk/Software/analysis/ZMap"

#define ZMAPWEB_DOC_URL ZMAPWEB_URL "/user_doc"

/* Reorganising the zmap_update_web.sh and the directories, together
 * with inertia of the way the last release code works, means that the
 * Release_notes dir is no longer under the DOC_URL dir. */

#define ZMAPWEB_RELEASE_NOTES_DIR "../Release_notes"

/* This line is parsed/updated by ZMap/scripts/zmapreleasenotes, do not alter its format without
 * updating that script as well. */
#define ZMAPWEB_RELEASE_NOTES "release_notes.2011_05_13.14_53_04.shtml"

#define ZMAPWEB_HELP_DOC  "user_interface.shtml"
#define ZMAPWEB_HELP_ALIGNMENT_SECTION "alignment_display"

#define ZMAPWEB_KEYBOARD_DOC  "keyboard_mouse.shtml"


#endif /* ZMAP_WEBPAGES_H */
