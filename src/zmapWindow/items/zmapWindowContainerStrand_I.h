/*  File: zmapWindowContainerStrand_I.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */
#ifndef __ZMAP_WINDOW_CONTAINER_STRAND_I_H__
#define __ZMAP_WINDOW_CONTAINER_STRAND_I_H__

#include <zmapWindowContainerStrand.h>
#include <zmapWindowContainerGroup_I.h>

typedef enum
  {
    CONTAINER_STRAND_FORWARD   = ZMAPSTRAND_FORWARD,
    CONTAINER_STRAND_REVERSE   = ZMAPSTRAND_REVERSE,
    CONTAINER_STRAND_SEPARATOR = (ZMAPSTRAND_FORWARD << 4),
  } ContainerStrand;

typedef struct _zmapWindowContainerStrandClassStruct
{
  zmapWindowContainerGroupClass __parent__;

} zmapWindowContainerStrandClassStruct;

typedef struct _zmapWindowContainerStrandStruct
{
  zmapWindowContainerGroup __parent__;

  ContainerStrand strand;

} zmapWindowContainerStrandStruct;


#endif /* ! __ZMAP_WINDOW_CONTAINER_STRAND_I_H__ */
