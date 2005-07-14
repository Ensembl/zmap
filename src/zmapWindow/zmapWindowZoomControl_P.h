/*  File: zmapWindowZoomControl_P.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jul 14 14:35 2005 (rds)
 * Created: Tue Jul 12 16:02:52 2005 (rds)
 * CVS info:   $Id: zmapWindowZoomControl_P.h,v 1.1 2005-07-14 15:23:09 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAPWINDOWZOOMCONTROL_P_H
#define ZMAPWINDOWZOOMCONTROL_P_H

typedef struct _ZMapWindowZoomControlStruct
{
  double zF;
  double minZF;
  double maxZF;
  double lineHeight;
  
  int border;

  ZMapWindowZoomStatus status;
} ZMapWindowZoomControlStruct;

#endif
