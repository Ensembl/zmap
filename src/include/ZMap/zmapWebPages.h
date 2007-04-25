/*  File: zmapWebPages.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Last edited: Feb 28 10:12 2007 (edgrif)
 * Created: Wed Oct 18 11:19:12 2006 (edgrif)
 * CVS info:   $Id: zmapWebPages.h,v 1.11 2007-04-25 12:13:23 zmap Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WEBPAGES_H
#define ZMAP_WEBPAGES_H

/* http://wwwdev.sanger.ac.uk/Software/analysis/ZMap/doc/user_interface.shtml */

#define ZMAPWEB_URL "http://wwwdev.sanger.ac.uk/Software/analysis/ZMap"

#define ZMAPWEB_DOC_URL ZMAPWEB_URL "/doc"

#define ZMAPWEB_RELEASE_NOTES_DIR "Release_notes"

/* This line is parsed/updated by ZMap/scripts/zmapreleasenotes, do not alter its format without
 * updating that script as well. */
#define ZMAPWEB_RELEASE_NOTES "release_notes.2007_04_25.shtml"

#define ZMAPWEB_HELP_DOC  "user_interface.shtml"

#define ZMAPWEB_HELP_KEYBOARD_SECTION "keyboard_mouse"



#endif /* ZMAP_WEBPAGES_H */
