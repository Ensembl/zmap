/*  File: zmapSys.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * and was written by
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Aug  6 10:37 2003 (edgrif)
 * Created: Thu Jul 24 14:36:17 2003 (edgrif)
 * CVS info:   $Id: zmapSys.h,v 1.1 2004-03-03 12:51:52 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SYS_H
#define ZMAP_SYS_H

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>


/* Debugging messages, TRUE for on... */
extern gboolean zmap_debug_G ;

/* Use this one like this:   ZMAP_DEBUG(("format string" lots, of, args)) ; */
#define ZMAP_DEBUG(ALL_ARGS_WITH_BRACKETS) \
do { \
     if (zmap_debug_G) \
       printf ALL_ARGS_WITH_BRACKETS ; \
   } while (0)



/* Zmap error messages. */
/* I'm defining this for now, I don't really want to use the whole messXXXX() thing
 * unless it gets chopped out into a file on its own.... */

/* Use this one like this:   ZMAPERR("complete error text") ; */
#define ZMAPERR(TEXT) \
do { \
     fprintf(stderr, "%s (line %d), error:: \"%s\"\n", \
             __FILE__, __LINE__, TEXT) ; \
     exit(EXIT_FAILURE) ; \
   } while (0)


/* Use this one like this: ZMAPSYSERR(errno_value, "complete error text") ;   */
#define ZMAPSYSERR(CODE, TEXT) \
do { \
     fprintf(stderr, "%s (line %d), error:: \"%s\" (errno = \"%s\")\n", \
             __FILE__, __LINE__, TEXT, g_strerror(CODE)) ; \
     exit(EXIT_FAILURE) ; \
   } while (0)



/* I would like the data passed to be ZMapWinConn but this produces circularities
 * in header dependencies between zmapApp.h and zmapManager.h....think about this. */
typedef void (*zmapAppCallbackFunc)(void *app_data, void * zmap) ;

/* Simple callback function. */
typedef void (*zmapVoidIntCallbackFunc)(void *cb_data, int reason_code) ;


#endif /* !ZMAP_SYS_H */
