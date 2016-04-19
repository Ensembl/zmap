/*  File: zmapDataSource.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description:
 * Exported functions:
 *-------------------------------------------------------------------
 */

#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

/*
 * Forward declaration of general source type.
 */
class ZMapDataSourceStruct ;
typedef ZMapDataSourceStruct *ZMapDataSource ;

/*
 * Enumeration to represent different source types.
 */
enum class ZMapDataSourceType {GIO, HTS, BED, BIGBED, BIGWIG, UNK} ;





/*
 * The rationale for this is that previously we were reading GFF data
 * only, always through a GIOChannel. Thus the abstraction of ZMapDataSource,
 * which has several types
 *
 *            ZMapDataSourceGIO       GIOChannel, synchronous OR asynchronous
 *            ZMapDataSourceHTS       HTS file, synchronous only
 */
ZMapDataSource zMapDataSourceCreate(const char * const file_name, GError **error_out = NULL) ;
ZMapDataSource zMapDataSourceCreateFromGIO(GIOChannel * const io_channel) ;
gboolean zMapDataSourceIsOpen(ZMapDataSource const source) ;
gboolean zMapDataSourceDestroy( ZMapDataSource *source) ;
ZMapDataSourceType zMapDataSourceGetType(ZMapDataSource source ) ;
gboolean zMapDataSourceReadHTSHeader(ZMapDataSource source, const char *sequence) ;
gboolean zMapDataSourceReadLine (ZMapDataSource const data_pipe , GString * const str) ;
gboolean zMapDataSourceGetGFFVersion(ZMapDataSource const source, int * const out_val) ;
ZMapDataSourceType zMapDataSourceTypeFromFilename(const char * const, GError **error_out = NULL) ;




#endif

