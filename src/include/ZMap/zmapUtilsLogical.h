/*  File: zmapUtilsLogical.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Macros for manipulating bits and other logic things.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_UTILS_LOGICAL_H
#define ZMAP_UTILS_LOGICAL_H


/* Turn bits on/off. */
#define ZMAP_FLAG_ON(VAR, FLAG) \
  (VAR) |= (FLAG)

#define ZMAP_FLAG_OFF(VAR, FLAG) \
  (VAR) &= ~(FLAG)


/* Test bits */
#define ZMAP_FLAG_IS_ON(VAR, FLAG) \
  ((VAR) & (FLAG))


#endif /* ZMAP_UTILS_LOGICAL_H */
