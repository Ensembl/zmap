/*  File: zmapUtils.h
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
 * Description: Utility functions for ZMap.
 * HISTORY:
 * Last edited: Mar 12 14:54 2004 (edgrif)
 * Created: Thu Feb 26 10:33:10 2004 (edgrif)
 * CVS info:   $Id: zmapUtils.h,v 1.2 2004-03-12 15:11:16 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_H
#define ZMAP_UTILS_H

#include <glib.h>


/* 
 *             Macros/routines for testing/debugging....
 */

/* Usual assert() usage. */
#define ZMAPASSERT(EXPR)                              \
g_assert((EXPR))


/* Put this in sections of code which should never be reached. */
#define ZMAPASSERTNOTREACHED()                        \
g_assert_not_reached()


/* If this global is TRUE then debugging messages will be displayed. */
extern gboolean zmap_debug_G ;

/* You can give this one a variable number of args,
 *                                      e.g.  ZMAP_DEBUG("format string", lots, of, args) ; */
#define ZMAP_DEBUG(...)             \
do {                                \
     if (zmap_debug_G)              \
       g_log (G_LOG_DOMAIN,         \
              G_LOG_LEVEL_MESSAGE,  \
              __VA_ARGS__) ;        \
   } while (0)



/* 
 *             Macros/routines for normal code execution....
 */

/* MAY WISH TO MIGRATE THIS TO ONE OF THE MACROS BELOW.... */
void zmapGUIShowMsg(char *msg) ;


/* We may want to add an fflush to the glib calls...think about this..... */


/* This is cribbed directly from glib.h, I wanted versions of their g_return_XXXX macros
 * that exit if they fail rather than just returning. I have kept them as g_XXX calls
 * because I hope to persuade glib to include them..... */

#ifdef G_DISABLE_CHECKS

#define g_fatal_if_fail(expr)			G_STMT_START{ (void)0; }G_STMT_END

#else /* !G_DISABLE_CHECKS */

#ifdef __GNUC__

#define g_fatal_if_fail(expr)		G_STMT_START{			\
     if G_LIKELY(expr) { } else       					\
       {								\
	 g_log (G_LOG_DOMAIN,						\
		G_LOG_LEVEL_ERROR,					\
		"(%s, %s(), line %d) - assertion `%s' failed",		\
		__FILE__,						\
		__PRETTY_FUNCTION__,					\
		__LINE__,						\
		#expr);							\
       };				}G_STMT_END

#else /* !__GNUC__ */

#define g_fatal_if_fail(expr)		G_STMT_START{		\
     if (expr) { } else						\
       {							\
	 g_log (G_LOG_DOMAIN,					\
		G_LOG_LEVEL_ERROR,				\
		"(%s, line %d) - assertion `%s' failed",	\
		__FILE__,					\
		__LINE__,					\
		#expr);						\
       };				}G_STMT_END

#endif /* !__GNUC__ */

#endif /* !G_DISABLE_CHECKS */


/* Use this for when you want to check a return code from a call (e.g. printf)
 * and exit in the unlikely event that it fails. */
#define ZMAPFATALEVAL(EXPR)                               \
g_fatal_if_fail((EXPR))


/* These all need to have the GNUC stuff for the function name added as above..... */


/* Use this one like this:             ZMAPERR("%s blah, blah, %d", str, int) ; */
#define ZMAPFATALERR(FORMAT, ...) \
	 g_log(G_LOG_DOMAIN,					\
	       G_LOG_LEVEL_ERROR,				\
	       "(%s, line %d) - " FORMAT,                     \
		__FILE__,					\
		__LINE__,					\
		__VA_ARGS__)

/* Use this macro like this:
 *      ZMAPFATALSYSERR(errno, "Really Bad News %s %d's", "always comes in", 3) ;
 */
#define ZMAPFATALSYSERR(ERRNO, FORMAT, ...) \
	 g_log(G_LOG_DOMAIN,					\
	       G_LOG_LEVEL_ERROR,				\
	       "(%s, line %d) - " FORMAT " (errno = \"%s\")",                     \
		__FILE__,					\
		__LINE__,					\
		__VA_ARGS__,                                    \
	       g_strerror(ERRNO))




#endif /* ZMAP_UTILS_H */
