/*  File: zmapDataStream.h
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description:
 * 
 *-------------------------------------------------------------------
 */

#ifndef DATA_STREAM_H
#define DATA_STREAM_H

/*
 * Forward declaration of general source type.
 */
typedef struct ZMapDataStreamStruct_ *ZMapDataStream ;

/*
 * Enumeration to represent different source types.
 */
enum class ZMapDataStreamType {GIO, HTS, UNK} ;


typedef struct ZMapDataStreamStruct_
  {
    ZMapDataStreamType type ;
  } ZMapDataStreamStruct ;




/*
 * The rationale for this is that previously we were reading GFF data
 * only, always through a GIOChannel. Thus the abstraction of ZMapDataStream,
 * which has two types
 *
 *            ZMapDataStreamGIO       GIOChannel, synchronous OR asynchronous
 *            ZMapDataStreamHTS       HTS file, synchronous only
 */
ZMapDataStream zMapDataStreamCreate(const char * const file_name, GError **error_out = NULL) ;
ZMapDataStream zMapDataStreamCreateFromGIO(GIOChannel * const io_channel) ;
gboolean zMapDataStreamDestroy(ZMapDataStream *source) ;

gboolean zMapDataStreamIsOpen(ZMapDataStream const source) ;
const char *zMapDataStreamGetSequence(ZMapDataStream const source) ;
ZMapDataStreamType zMapDataStreamGetType(ZMapDataStream source ) ;
gboolean zMapDataStreamReadHTSHeader(ZMapDataStream source, const char *sequence) ;
gboolean zMapDataStreamReadLine(ZMapDataStream const data_pipe , GString * const str) ;
gboolean zMapDataStreamGetGFFVersion(ZMapDataStream const source, int * const out_val) ;
ZMapDataStreamType zMapDataStreamTypeFromFilename(const char * const, GError **error_out = NULL) ;



#endif /* DATA_STREAM_H */

