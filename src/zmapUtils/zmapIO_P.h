/*  File: zmapIO_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Private header for IO package.
 *
 * HISTORY:
 * Last edited: Oct 17 10:27 2007 (edgrif)
 * Created: Mon Oct 15 13:56:41 2007 (edgrif)
 * CVS info:   $Id: zmapIO_P.h,v 1.3 2010-06-14 15:40:14 mh17 Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_IO_P_H
#define ZMAP_IO_P_H

#include <glib.h>
#include <ZMap/zmapIO.h>

typedef enum {ZMAPIO_INVALID, ZMAPIO_STRING, ZMAPIO_FILE, ZMAPIO_SOCKET} ZMapIOType ;

typedef struct ZMapIOOutStruct_
{
  char *valid ;
  ZMapIOType type ;

  union
  {
    GString *string ;
    GIOChannel *channel ;
  } output ;

  GError *g_error ;

} ZMapIOOutStruct ;




#endif /* !ZMAP_IO_P_H */
