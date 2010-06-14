/*  File: zmapWindowContainerAlignment_I.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 20 10:04 2010 (edgrif)
 * Created: Fri Feb  6 11:49:03 2009 (rds)
 * CVS info:   $Id: zmapWindowContainerAlignment_I.h,v 1.4 2010-06-14 15:40:17 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_WINDOW_CONTAINER_ALIGNMENT_I_H__
#define __ZMAP_WINDOW_CONTAINER_ALIGNMENT_I_H__

#include <zmapWindowContainerAlignment.h>


typedef struct _zmapWindowContainerAlignmentStruct
{
  zmapWindowContainerGroup __parent__;

} zmapWindowContainerAlignmentStruct;


typedef struct _zmapWindowContainerAlignmentClassStruct
{
  zmapWindowContainerGroupClass __parent__;

} zmapWindowContainerAlignmentClassStruct;



#endif /* __ZMAP_WINDOW_CONTAINER_ALIGNMENT_I_H__ */
