/*  File: zmapConfigStanzaStructs.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Sep 30 12:50 2008 (rds)
 * Created: Tue Aug 26 12:38:28 2008 (rds)
 * CVS info:   $Id: zmapConfigStanzaStructs.h,v 1.1 2008-10-01 15:27:36 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAPCONFIGSTANZASTRUCTS_H
#define ZMAPCONFIGSTANZASTRUCTS_H

typedef struct _ZMapConfigSourceStruct
{
  const char *url, 
    *version,
    *featuresets,
    *navigatorsets,
    *stylesfile,
    *format;
  int timeout;
  gboolean writeback,
    sequence;
} ZMapConfigSourceStruct, *ZMapConfigSource;


#endif /* ZMAPCONFIGSTANZASTRUCTS_H */
