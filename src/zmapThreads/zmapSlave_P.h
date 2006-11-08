/*  File: zmapSlave_P.h
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
 * and was written by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * HISTORY:
 * Last edited: Feb  2 11:30 2005 (edgrif)
 * Created: Wed Aug  6 15:48:47 2003 (edgrif)
 * CVS info:   $Id: zmapSlave_P.h,v 1.8 2006-11-08 09:24:34 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SLAVE_P_H
#define ZMAP_SLAVE_P_H

#include <glib.h>
#include <ZMap/zmapThreads.h>



typedef struct
{
  ZMapThread thread ;
  gboolean thread_died ;
  gchar *initial_error ;				    /* Holds string description of first
							       serious error that caused thread
							       termination. */

  void *slave_data ;					    /* Any state required by slave. */
} zmapThreadCBstruct, *zmapThreadCB ;




/* Error messages, needs some refining and changing now thread bit is more generalised. */
#define ZMAPTHREAD_SLAVECREATE  "Thread creation failed"
#define ZMAPTHREAD_SLAVEOPEN    "Thread open failed"
#define ZMAPTHREAD_SLAVECONTEXT "Thread create context failed"
#define ZMAPTHREAD_SLAVEREQUEST "Thread request failed"




#endif /* !ZMAP_SLAVE_P_H */
