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
 * Last edited: Apr  7 12:25 2004 (edgrif)
 * Created: Thu Feb 26 10:33:10 2004 (edgrif)
 * CVS info:   $Id: zmapUtils.h,v 1.3 2004-04-08 16:15:35 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_H
#define ZMAP_UTILS_H

#include <glib.h>


/* Some compilers give more information than others so set up compiler dependant defines. */

#ifdef __GNUC__	
#define ZMAP_MSG_FORMAT_STRING  "(%s, %s(), line %d) - "
#define ZMAP_MSG_FUNCTION_MACRO __PRETTY_FUNCTION__,
#else
#define ZMAP_MSG_FORMAT_STRING  "(%s, line %d) - "
#define ZMAP_MSG_FUNCTION_MACRO        
#endif


#include <ZMap/zmapUtilsDebug.h>
#include <ZMap/zmapUtilsCheck.h>
#include <ZMap/zmapUtilsMesg.h>
#include <ZMap/zmapUtilsLog.h>





#endif /* ZMAP_UTILS_H */
