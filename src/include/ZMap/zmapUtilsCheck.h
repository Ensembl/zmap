/*  File: zmapUtilsCheck.h
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Contains macros, functions etc. for checking in production
 *              code.
 *              
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_CHECK_H
#define ZMAP_UTILS_CHECK_H

#include <stdlib.h>
#include <glib.h>


/* Use to test functions, e.g.
 *         zmapCheck(myfunc("x y z"), "%s", "disaster, my func should never fail") ;
 *  */
#define zMapCheck(EXPR, FORMAT, ...)                            \
G_STMT_START{			                                \
  if ((EXPR))                					\
    {								\
      g_printerr(ZMAP_MSG_FORMAT_STRING " '%s' " FORMAT,        \
		 ZMAP_MSG_FUNCTION_MACRO,    		        \
		 #EXPR,                                         \
		 __VA_ARGS__);					\
      exit(EXIT_FAILURE) ;                                      \
    };                                                          \
}G_STMT_END


#endif /* ZMAP_UTILS_CHECK_H */
