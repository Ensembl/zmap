/*  File: zmapGFFGeneral.c
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: This file contains material that is used by both
 * the v2 and v3 parsers. This consists of some (but not all) of the
 * functions publically exported as listed in src/include/ZMap/zmapGFF.h.
 *-------------------------------------------------------------------
 */


#include <ZMap/zmapGFF.h>

#include "zmapGFF_P.h"
#include "zmapGFF2_P.h"
#include "zmapGFF3_P.h"



/*
 * Public interface parser creation function. Calls version specific functions.
 */
ZMapGFFParser zMapGFFCreateParser(int iGFFVersion, char *sequence, int features_start, int features_end)
{
  ZMapGFFParser parser = NULL ;

  if (iGFFVersion == ZMAPGFF_VERSION_2 )
  {
    parser = zMapGFFCreateParser2(sequence, features_start, features_end) ;
  }
  else if (iGFFVersion == ZMAPGFF_VERSION_3 )
  {
    parser = zMapGFFCreateParser3(sequence, features_start, features_end) ;
  }
  else
  {
    /* not a valid version */
  }

  return parser ;
}



/*
 * Public interface function to parse headers. Calls version specific functions.
 */
gboolean zMapGFFParseHeader(ZMapGFFParser parser, char *line, gboolean *header_finished, ZMapGFFHeaderState *header_state)
{
  gboolean bResult = FALSE ;
  if (parser->gff_version == ZMAPGFF_VERSION_2 )
  {
    bResult = zMapGFFParseHeader_V2(parser, line, header_finished, header_state) ;
  }
  else if (parser->gff_version == ZMAPGFF_VERSION_3 )
  {
    /* will be v3 version of function */
  }

  return bResult ;
}

/*
 * Public interface function to parse a section of sequence. Calls version specific functions.
 */
gboolean zMapGFFParseSequence(ZMapGFFParser parser, char *line, gboolean *sequence_finished)
{
  gboolean bResult = FALSE ;
  if (parser->gff_version == ZMAPGFF_VERSION_2 )
  {
    bResult = zMapGFFParseSequence_V2(parser, line, sequence_finished) ;
  }
  else if (parser->gff_version == ZMAPGFF_VERSION_3 )
  {
    /* will call v3 version */
  }

  return bResult ;
}




/*
 * Public interface function for the general body line parser. Calls version specific stuff.
 */
gboolean zMapGFFParseLine(ZMapGFFParser parser, char *line)
{
  gboolean result = FALSE ;

  if (!parser)
    return result ;

  if (parser->gff_version == ZMAPGFF_VERSION_2)
  {
    result = zMapGFFParseLineLength_V2(parser, line, 0) ;
  }
  else if (parser->gff_version == ZMAPGFF_VERSION_3 )
  {
    /* call appropriate v3 function */
  }

  return result ;
}

/*
 * Public interface for general body line parser. Calls version specific stuff.
 */
gboolean zMapGFFParseLineLength(ZMapGFFParser parser, char *line, gsize line_length)
{
  gboolean result = FALSE ;

  if (!parser)
    return result ;

  if (parser->gff_version == ZMAPGFF_VERSION_2)
  {
    result = zMapGFFParseLineLength_V2(parser, line, 0) ;
  }
  else if (parser->gff_version == ZMAPGFF_VERSION_3 )
  {
    /* call appropriate v3 function */
  }

  return result ;

}





/* the servers need styles to add DNA and 3FT
 * they used to create temp style and then destroy these but that's not very good
 * they don't have styles info directly but this is stored in the parser
 * during the protocol steps
 */
GHashTable *zMapGFFParserGetStyles(ZMapGFFParser parser)
{
  if(!parser)
    return NULL ;
  if ((parser->gff_version != ZMAPGFF_VERSION_2) && (parser->gff_version))
    return NULL ;
  return parser->sources;
}




/* We should do this internally with a var in the parser struct.... */
/* This function must be called prior to parsing feature lines, it is not required
 * for either the header lines or sequences. */
gboolean zMapGFFParserInitForFeatures(ZMapGFFParser parser, GHashTable *sources, gboolean parse_only)
{
  gboolean result = FALSE ;
  GQuark locus_id ;

  if (!parser)
    return result ;
  if ((parser->gff_version != ZMAPGFF_VERSION_2) && (parser->gff_version != ZMAPGFF_VERSION_3))
    return result ;

  parser->sources = sources ;
  parser->parse_only = parse_only ;

  /* Check for Locus as one of the sources as this needs to be constructed as we go along. */
  locus_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_LOCUS_NAME) ;
  if (!(parser->locus_set_style = zMapFindStyle(parser->sources, locus_id)))
    {
      zMapLogWarning("Locus set will not be created, "
		     "could not find style \"%s\" for feature set \"%s\".",
		     ZMAP_FIXED_STYLE_LOCUS_NAME, ZMAP_FIXED_STYLE_LOCUS_NAME) ;
    }
  else
    {
      parser->locus_set_id = locus_id ;
    }



  if (!parser->parse_only)
    {
      g_datalist_init(&(parser->feature_sets)) ;
      parser->free_on_destroy  = TRUE ;
    }
  else
    {
      parser->feature_sets =  NULL ;
      parser->free_on_destroy  = FALSE ;
    }

  return result ;
}
