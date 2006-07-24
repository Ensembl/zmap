/*  File: zmapUtils_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * HISTORY:
 * Last edited: May  3 13:22 2006 (edgrif)
 * Created: Wed Mar 31 11:53:45 2004 (edgrif)
 * CVS info:   $Id: zmapUtils_P.h,v 1.23 2006-07-24 12:36:19 zmap Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_P_H
#define ZMAP_UTILS_P_H

#include <ZMap/zmapUtils.h>


/* ZMap version stuff.... */
#define ZMAP_TITLE "ZMap"
#define ZMAP_DESCRIPTION "A multi-threaded genome browser and annotation tool."
#define ZMAP_VERSION 0
#define ZMAP_RELEASE 1
#define ZMAP_UPDATE 4


/* Make a single version number out of the version, release and update numbers. */
#define ZMAP_MAKE_VERSION_NUMBER(VERSION, RELEASE, UPDATE) \
((VERSION * 10000) + (RELEASE * 100) + UPDATE)

/* Make a single version string out of the version, release and update numbers. */
#define ZMAP_MAKE_VERSION_STRING(VERSION, RELEASE, UPDATE) \
ZMAP_MAKESTRING(VERSION) "." ZMAP_MAKESTRING(RELEASE) "." ZMAP_MAKESTRING(UPDATE)

/* Make a title string containing the title of the application/library and
 *    and the version, release and update numbers. */
#define ZMAP_MAKE_TITLE_STRING(TITLE, VERSION, RELEASE, UPDATE) \
TITLE " - " ZMAP_MAKE_VERSION_STRING(VERSION, RELEASE, UPDATE)




/* Create a copyright string for dialogs etc. */
#define ZMAP_COPYRIGHT_STRING()                                  \
"Copyright (c):   Sanger Institute, 2006"


/* Create a comments string for dialogs etc. */
#define ZMAP_COMMENTS_STRING(TITLE, VERSION, RELEASE, UPDATE)                                  \
"("ZMAP_MAKE_TITLE_STRING(TITLE, VERSION, RELEASE, UPDATE)", "              \
"compiled on - "__DATE__" "__TIME__")\n"                                                  \
"\n"                                                  \
"This application is part of the ZMap genome viewer/annotation package originally written by\n"    \
"Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk\n"                            \
"and Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk)\n"


/* Create a copyright string for dialogs etc. */
#define ZMAP_LICENSE_STRING()                                  \
"ZMap is distributed under the GNU  Public License, see http://www.gnu.org/copyleft/gpl.txt"



/* Create a copyright string to insert in the compiled application, this will show
 * up if someone looks through the executable or does a "what" on the executable. */
#define ZMAP_OBJ_COPYRIGHT_STRING(TITLE, VERSION, RELEASE, UPDATE, DESCRIPTION_STRING)          \
"@(#) \n"                                                                                       \
"@(#) ------------------------------------------------------------------------------------------ \n"             \
"@(#) Title/Version:  "ZMAP_MAKE_TITLE_STRING(TITLE, VERSION, RELEASE, UPDATE)"\n"              \
"@(#)      Compiled:  "__DATE__" "__TIME__"\n"                                                  \
"@(#)   Description:  " DESCRIPTION_STRING"\n"                                                  \
"@(#) Copyright (c):   Sanger Institute, 2006\n"                                                   \
"@(#) \n"                                                                                       \
"@(#) This application is part of the ZMap genome viewer/annotation package originally written by \n"    \
"@(#) 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk, \n"                            \
"@(#)   and Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk \n"                                \
"@(#) \n"                                                                                       \
"@(#) ZMap is distributed under the GNU  Public License, see http://www.gnu.org/copyleft/gpl.txt \n" \
"@(#) ------------------------------------------------------------------------------------------ \n"             \
"@(#) \n"





#define ZMAP_SEPARATOR "/"				    /* WE SHOULD BE ABLE TO CALL A FUNC
							       FOR THIS..... */


#endif /* !ZMAP_UTILS_P_H */
