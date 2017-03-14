/*  File: zmapDataStream.hpp
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
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
                                    const char * const file_name, 
                                    const char *sequence, const int start, const int end, 
                                    GError **error_out = NULL) ;
bool zMapDataStreamIsOpen(ZMapDataStream const source) ;
bool zMapDataStreamDestroy( ZMapDataStream *source) ;
ZMapDataStreamType zMapDataStreamGetType(ZMapDataStream source ) ;
ZMapDataStreamType zMapDataStreamTypeFromFilename(const char * const, GError **error_out = NULL) ;
std::string zMapDataStreamTypeToFileType(ZMapDataStreamType &stream_type) ;
ZMapDataStreamType zMapDataStreamTypeFromFileType(const std::string &file_type, GError **error_out) ;


#endif

