/*  File: zmapUtils_P.h
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 * HISTORY:
 * Last edited: May  6 15:50 2010 (edgrif)
 * Created: Wed Mar 31 11:53:45 2004 (edgrif)
 * CVS info:   $Id: zmapUtils_P.h,v 1.171 2011-05-11 14:54:07 zmap Exp $
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
#define ZMAP_UPDATE 125




/* Create a copyright string for dialogs etc. */
#define ZMAP_COPYRIGHT_STRING()                                  \
"Copyright (c) 2006-2010: Genome Research Ltd."


/* Create a website string for dialogs etc. */
#define ZMAP_WEBSITE_STRING()                                  \
"http://wwwdev.sanger.ac.uk/Software/analysis/ZMap/"


/* Create a comments string for dialogs etc. */
#define ZMAP_COMMENTS_STRING(TITLE, VERSION, RELEASE, UPDATE)                                  \
"("ZMAP_MAKE_TITLE_STRING(TITLE, VERSION, RELEASE, UPDATE)", "              \
"compiled on - "__DATE__" "__TIME__")\n"                                                  \
"\n"                                                  \
"This application is part of the ZMap genome viewer/annotation package originally written by\n"    \
"    Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk\n"                            \
"and Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk)\n"\
"and Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk \n"


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


#endif /* !ZMAP_UTILS_P_H */
