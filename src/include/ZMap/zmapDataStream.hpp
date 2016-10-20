/*  File: zmapDataStream.hpp
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

#ifndef DATA_STREAM_H
#define DATA_STREAM_H

/*
 * Forward declaration of general source type.
 */
class ZMapDataStreamStruct ;
typedef ZMapDataStreamStruct *ZMapDataStream ;

/*
 * Enumeration to represent different source types.
 */
enum class ZMapDataStreamType {NONE, GIO, HTS, BCF, BED, BIGBED, BIGWIG,  /*UNK always last*/UNK} ;





/*
 * The rationale for this is that previously we were reading GFF data
 * only, always through a GIOChannel. Thus the abstraction of ZMapDataStream,
 * which has several types
 *
 *            ZMapDataStreamGIO       GIOChannel, synchronous OR asynchronous
 *            ZMapDataStreamHTS       HTS bam/sam/cram file, synchronous only
 *            ZMapDataStreamBCF       HTS bcf/vcf file, synchronous only
 *            ZMapDataStreamBED       blatSrc Bed file, synchronous only
 *            ZMapDataStreamnBIGBED   blatSrc bigBed file, synchronous only
 *            ZMapDataStreamBIGWIG    blatSrc bigWig file, synchronous only
 */
ZMapDataStream zMapDataStreamCreate(ZMapConfigSource config_source,
                                    const GQuark source_name, const char * const file_name, 
                                    const char *sequence, const int start, const int end, 
                                    GError **error_out = NULL) ;
bool zMapDataStreamIsOpen(ZMapDataStream const source) ;
bool zMapDataStreamDestroy( ZMapDataStream *source) ;
ZMapDataStreamType zMapDataStreamGetType(ZMapDataStream source ) ;
ZMapDataStreamType zMapDataStreamTypeFromFilename(const char * const, GError **error_out = NULL) ;
std::string zMapDataStreamTypeToFileType(ZMapDataStreamType &stream_type) ;
ZMapDataStreamType zMapDataStreamTypeFromFileType(const std::string &file_type, GError **error_out) ;


#endif

