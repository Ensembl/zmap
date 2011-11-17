/*  File: zmapUtils_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
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
<<<<<<< HEAD
#define ZMAP_UPDATE 138
=======
#define ZMAP_UPDATE 137
>>>>>>> production


/* Create a copyright string for dialogs etc. */
#define ZMAP_COPYRIGHT_STRING()                                  \
"Copyright (c) 2006-2010: Genome Research Ltd."


/* Create a website string for dialogs etc. */
#define ZMAP_WEBSITE_STRING()                                  \
"http://wwwdev.sanger.ac.uk/Software/analysis/ZMap/"


/* compile string. */
#define ZMAP_COMPILE_STRING()               \
__DATE__ " " __TIME__



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
"@(#) Copyright (c):   Genome Research Ltd., 2006-2010\n"                                                   \
"@(#) \n"                                                                                       \
"@(#) This application is part of the ZMap genome viewer/annotation package originally written by \n"    \
"@(#) 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk, \n"                            \
"@(#)   and Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk \n"                                \
"@(#)   and Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk \n"                            \
"@(#) \n"                                                                                       \
"@(#) ZMap is distributed under the GNU  Public License, see http://www.gnu.org/copyleft/gpl.txt \n" \
"@(#) ------------------------------------------------------------------------------------------ \n"             \
"@(#) \n"





#define ZMAP_SEPARATOR "/"				    /* WE SHOULD BE ABLE TO CALL A FUNC
							       FOR THIS..... */


char *zmapDevelopmentString(void) ;
char *zmapCompileString(void) ;




#endif /* !ZMAP_UTILS_P_H */
