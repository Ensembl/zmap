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
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May  6 15:46 2004 (edgrif)
 * Created: Wed Mar 31 11:53:45 2004 (edgrif)
 * CVS info:   $Id: zmapUtils_P.h,v 1.2 2004-05-07 09:24:20 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_P_H
#define ZMAP_UTILS_P_H

#include <ZMap/zmapUtils.h>



#define ZMAP_SEPARATOR "/"				    /* WE SHOULD BE ABLE TO CALL A FUNC
							       FOR THIS..... */




void zmapGUIShowMsg(ZMapMsgType msg_type, char *msg) ;



#endif /* !ZMAP_UTILS_P_H */
