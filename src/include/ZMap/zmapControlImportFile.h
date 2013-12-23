/*  File: zmapControlImportFile.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2012: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Domain specific services called by main subsystems
 *              of zmap.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONTROL_IMPORT_H
#define ZMAP_CONTROL_IMPORT_H

#include <ZMap/zmapFeatureLoadDisplay.h>

typedef void (*ZMapControlImportFileCB)(gpointer user_data) ;

void zMapControlImportFile(ZMapControlImportFileCB user_func, gpointer user_data,
			    ZMapFeatureSequenceMap sequence_map, int req_start, int req_end) ;



#endif /* !ZMAP_CONTROL_IMPORT_H */
