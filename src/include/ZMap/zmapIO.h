/*  File: zmapIO.h
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
 * Description: Package to IO to strings, sockets, files etc.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_IO_H
#define ZMAP_IO_H

#include <glib.h>

typedef struct ZMapIOOutStruct_ *ZMapIOOut ;



gboolean zMapOutValid(ZMapIOOut out) ;
ZMapIOOut zMapOutCreateStr(char *init_value, int size) ;
ZMapIOOut zMapOutCreateFD(int fd) ;
char *zMapOutGetStr(ZMapIOOut out) ;
gboolean zMapOutWrite(ZMapIOOut out, char *text) ;
gboolean zMapOutWriteFormat(ZMapIOOut out, char *format, ...) ;
void zMapOutDestroy(ZMapIOOut out) ;



#endif /* !ZMAP_IO_H */
