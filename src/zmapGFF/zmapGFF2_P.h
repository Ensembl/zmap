/*  File: zmapGFF2_P.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
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
 * originated by
 * 	    Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Private header file for GFF2-specific functions and
 * definitions (enumerations, etc).
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_GFF2_P_H
#define ZMAP_GFF2_P_H


#include <ZMap/zmapGFF.h>


/* Types of name find operation, depend on type of feature. */
typedef enum {NAME_FIND, NAME_USE_SOURCE, NAME_USE_SEQUENCE, NAME_USE_GIVEN, NAME_USE_GIVEN_OR_NAME } NameFindType ;



/* The main parser struct, this represents an instance of a parser. */
typedef struct ZMapGFF2ParserStruct_
{
  /*
   * Data common to both versions.
   */
  ZMAPGFF_PARSER_STRUCT_COMMON_DATA

  /*
   * Data for this verison only.
   */

  /* File data: some derived from the file directly. */
  struct
  {
    unsigned int done_header : 1 ;			            /* Is the header processed ? */
    unsigned int got_gff_version : 1 ;
    unsigned int got_sequence_region : 1 ;
  } header_flags ;

  ZMapGFFHeaderState header_state ;

  gboolean SO_compliant ;				    /* TRUE => use only SO terms for
							       feature types. */
  ZMapGFFClipMode clip_mode ;				    /* Decides how features that overlap
							       or are outside the start/end are
							       handled. */

  /* Parsing buffers, since lines can be long we allocate these dynamically from the
   * known line length and construct a format string for the scanf using this length. */
  gsize buffer_length ;
  char **buffers[GFF_BUF_NUM] ;
  char *format_str ;
  char *cigar_string_format_str ;

  /* Parsing DNA sequence data, used when DNA sequence is embedded in the file. */
  struct
  {
    unsigned int done_start : 1 ;
    unsigned int in_sequence_block : 1 ;
    unsigned int done_finished :1 ;
  } sequence_flags ;

  ZMapSequenceStruct seq_data ;

} ZMapGFF2ParserStruct, *ZMapGFF2Parser ;



/*
 * Code specific to this version of the parser only.
 */
ZMapGFFParser zMapGFFCreateParser2(char *sequence, int features_start, int features_end) ;
gboolean zMapGFFParseHeader_V2(ZMapGFFParser parser_base, char *line, gboolean *header_finished, ZMapGFFHeaderState *header_state) ;
gboolean zMapGFFParseSequence_V2(ZMapGFFParser parser_base, char *line, gboolean *sequence_finished) ;
gboolean zMapGFFParseLineLength_V2(ZMapGFFParser parser_base, char *line, gsize line_length) ;



#endif
