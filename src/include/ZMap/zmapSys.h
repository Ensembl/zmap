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
 * Last edited: Mar 12 14:06 2004 (edgrif)
 * Created: Thu Jul 24 14:36:17 2003 (edgrif)
 * CVS info:   $Id: zmapSys.h,v 1.2 2004-03-12 15:10:39 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SYS_H
#define ZMAP_SYS_H

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>


/* I would like the data passed to be ZMapWinConn but this produces circularities
 * in header dependencies between zmapApp.h and zmapManager.h....think about this. */
typedef void (*zmapAppCallbackFunc)(void *app_data, void * zmap) ;

/* Simple callback function. */
typedef void (*zmapVoidIntCallbackFunc)(void *cb_data, int reason_code) ;


#endif /* !ZMAP_SYS_H */
