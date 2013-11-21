/*  File: zmapGFF3parser.c
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2013: Genome Research Ltd.
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
 * Description: Parser and all associated internal functions for GFF
 * version 3 format.
 *
 *-------------------------------------------------------------------
 */


#include <ZMap/zmapGFF.h>
#include <ZMap/zmapSO.h>
#include <ZMap/zmapSOParser.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapFeatureLoadDisplay.h>
#include "zmapGFFHeader.h"
#include "zmapGFF_P.h"
#include "zmapGFF3_P.h"
#include "zmapGFFStringUtils.h"
#include "zmapGFFDirective.h"
#include "zmapGFFAttribute.h"
#include "zmapGFFFeatureData.h"

/*
 * Functions for internal usage only.
 */
static gboolean removeCommentFromLine( char* sLine ) ;
static ZMapGFFLineType parserLineType(const char * const sLine) ;
static ZMapGFFParserState parserFSM(ZMapGFFParserState eCurrentState, const char * const sNewLine) ;
static gboolean resizeFormatStrs(ZMapGFFParser const pParser) ;
static gboolean resizeBuffers(ZMapGFFParser const pParser, gsize iLineLength) ;
static gboolean isCommentLine(const char * const sLine) ;
static gboolean isAcedbError(const char* const ) ;
static gboolean parserStateChange(ZMapGFFParser pParser, ZMapGFFParserState eOldState, ZMapGFFParserState eNewState, const char * const sLine) ;
static gboolean initializeSequenceRead(ZMapGFFParser const pParser, const char * const sLine) ;
static gboolean finalizeSequenceRead(ZMapGFFParser const pParser , const char* const sLine) ;
static gboolean initializeFastaRead(ZMapGFFParser const pParser, const char * const sLine) ;
static gboolean actionUponClosure(ZMapGFFParser const pParser, const char* const sLine)  ;
static gboolean iterationFunctionID(GQuark gqID, GHashTable *pValueTable) ;
static gboolean initParserForSequenceRead(ZMapGFFParser pParser) ;
static gboolean copySequenceData(ZMapSequence pSequence, GString *pData) ;

static gboolean parseHeaderLine_V3(ZMapGFFParser const pParserBase, const char * const sLine) ;
static gboolean parseFastaLine_V3(ZMapGFFParser const pParser, const char* const sLine) ;
static gboolean parseBodyLine_V3(ZMapGFFParser const pParser, const char * const sLine) ;
static gboolean parseSequenceLine_V3(ZMapGFFParser const pParser, const char * const sLine) ;

static gboolean addNewSequenceRecord(ZMapGFFParser pParser);
static gboolean appendToSequenceRecord(ZMapGFFParser pParser, const char * const sLine) ;
static gboolean parseDirective_GFF_VERSION(ZMapGFFParser const pParser, const char * const line);
static gboolean parseDirective_SEQUENCE_REGION(ZMapGFFParser const pParser, const char * const line);
static gboolean parseDirective_FEATURE_ONTOLOGY(ZMapGFFParser const pParser, const char * const line);
static gboolean parseDirective_ATTRIBUTE_ONTOLOGY(ZMapGFFParser const pParser, const char * const line);
static gboolean parseDirective_SOURCE_ONTOLOGY(ZMapGFFParser const pParser, const char * const line);
static gboolean parseDirective_SPECIES(ZMapGFFParser const pParser, const char * const line);
static gboolean parseDirective_GENOME_BUILD(ZMapGFFParser const pParser, const char * const line);

static gboolean makeNewFeature_V3(const ZMapGFFParser const pParser, char ** const err_text, const ZMapGFFFeatureData const pFeatureData ) ;

static ZMapGFFParserFeatureSet getParserFeatureSet(ZMapGFFParser pParser, char* sFeatureSetName ) ;
static gboolean getFeatureName(const char * const sequence, const ZMapGFFAttribute * const pAttributes, unsigned int nAttributes,const char * const given_name, const char * const source, ZMapStyleMode feature_type,
  ZMapStrand strand, int start, int end, int query_start, int query_end, char ** const feature_name, char ** const feature_name_id) ;
static void destroyFeatureArray(gpointer data) ;


/*
 * Parser FSM transitions. Row is current state, columns is line type.
 */
static const ZMapGFFParserState ZMAP_GFF_PARSER_TRANSITIONS[ZMAPGFF_NUMBER_PARSER_STATES][ZMAPGFF_NUMBER_LINE_TYPES] =
{      /*   ZMAPGFF_LINE_EMP,   ZMAPGFF_LINE_DNA,   ZMAPGFF_LINE_EDN,   ZMAPGFF_LINE_DIR,   ZMAPGFF_LINE_COM,   ZMAPGFF_LINE_BOD,   ZMAPGFF_LINE_FAS,   ZMAPGFF_LINE_CLO, ZMAPGFF_LINE_OTH */
/* NON */{ZMAPGFF_PARSER_NON, ZMAPGFF_PARSER_SEQ, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_DIR, ZMAPGFF_PARSER_NON, ZMAPGFF_PARSER_BOD, ZMAPGFF_PARSER_FAS, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_NON},
/* HED */{ZMAPGFF_PARSER_DIR, ZMAPGFF_PARSER_SEQ, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_DIR, ZMAPGFF_PARSER_DIR, ZMAPGFF_PARSER_BOD, ZMAPGFF_PARSER_FAS, ZMAPGFF_PARSER_CLO, ZMAPGFF_PARSER_ERR},
/* BOD */{ZMAPGFF_PARSER_BOD, ZMAPGFF_PARSER_SEQ, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_DIR, ZMAPGFF_PARSER_BOD, ZMAPGFF_PARSER_BOD, ZMAPGFF_PARSER_FAS, ZMAPGFF_PARSER_CLO, ZMAPGFF_PARSER_ERR},
/* SEQ */{ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_NON, ZMAPGFF_PARSER_SEQ, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR},
/* FAS */{ZMAPGFF_PARSER_FAS, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_FAS, ZMAPGFF_PARSER_FAS, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_FAS},
/* CLO */{ZMAPGFF_PARSER_NON, ZMAPGFF_PARSER_SEQ, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_DIR, ZMAPGFF_PARSER_CLO, ZMAPGFF_PARSER_BOD, ZMAPGFF_PARSER_FAS, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_NON},
/* ERR */{ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR}
} ;


/*
 * Create a v3 parser object.
 */
ZMapGFFParser zMapGFFCreateParser_V3(char *sequence, int features_start, int features_end)
{
  ZMapGFF3Parser pParser = NULL ;
  if (!sequence || !*sequence )
    return NULL ;

  if ((sequence && *sequence) && (features_start > 0 && features_end >= features_start))
    {
      pParser                                   = g_new0(ZMapGFF3ParserStruct, 1) ;
      if (!pParser)
        return NULL ;

      pParser->gff_version                      = ZMAPGFF_VERSION_3 ;
      pParser->pHeader                          = zMapGFFCreateHeader() ;
      if (!pParser->pHeader)
        return NULL ;
      pParser->state                            = ZMAPGFF_PARSER_NON ;
      pParser->line_count                       = 0 ;
      pParser->num_features                     = 0 ;
      pParser->parse_only                       = FALSE ;
      pParser->free_on_destroy                  = TRUE ;
      pParser->bLogWarnings                     = FALSE ;
      pParser->iNumWrongSequence                = 0 ;


      pParser->bCheckSequenceLength             = FALSE ;
      pParser->raw_line_data                    = NULL ;
      pParser->nSequenceRecords                 = 0 ;
      pParser->pSeqData                         = NULL ;

      pParser->error                            = NULL ;
      pParser->error_domain                     = g_quark_from_string(ZMAP_GFF_ERROR) ;
      pParser->stop_on_error                    = FALSE ;

      pParser->SO_compliant                     = FALSE ;
      pParser->cSOErrorLevel                    = ZMAPGFF_SOERRORLEVEL_ERR ;
      pParser->default_to_basic                 = FALSE ;

      pParser->clip_mode                        = GFF_CLIP_NONE ;
      pParser->clip_end                         = 0 ;
      pParser->clip_start                       = 0 ;

      pParser->pMLF                             = zMapMLFCreate() ;
      pParser->source_2_feature_set             = NULL ;
      pParser->source_2_sourcedata              = NULL ;
      pParser->excluded_features                = g_hash_table_new(NULL, NULL) ;

      pParser->sequence_name                    = g_strdup(sequence) ;
      pParser->features_start                   = features_start ;
      pParser->features_end                     = features_end ;

      pParser->locus_set_id                     = 0 ;
      pParser->sources                          = NULL ;
      pParser->feature_sets                     = NULL ;
      pParser->src_feature_sets                 = NULL ;

      /* Set initial buffer & format string size */
      resizeBuffers((ZMapGFFParser) pParser, ZMAPGFF_BUF_INIT_SIZE) ;
      pParser->format_str                       = NULL ;
      pParser->cigar_string_format_str          = NULL ;
      resizeFormatStrs((ZMapGFFParser) pParser) ;

      /*
       * Some format characters and delimiters.
       */
      pParser->cDelimBodyLine                   = '\t' ;
      pParser->cDelimAttributes                 = ';' ;
      pParser->cDelimAttValue                   = '=' ;
      pParser->cDelimQuote                      = '"' ;

      /*
       * Flag for SO term set in use.
       */
      pParser->cSOSetInUse                      = ZMAPSO_USE_SOFA ;
    }

  return (ZMapGFFParser) pParser ;
}



/*
 * Function to destroy a parser.
 */
void zMapGFFDestroyParser_V3(ZMapGFFParser const pParserBase)
{
  unsigned int iValue ;
  if (!pParserBase)
    return ;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (pParser->error)
    g_error_free(pParser->error) ;
  if (pParser->raw_line_data)
    g_string_free(pParser->raw_line_data, TRUE) ;
  if (pParser->pMLF)
    zMapMLFDestroy(pParser->pMLF) ;
  if (pParser->excluded_features)
    g_hash_table_destroy(pParser->excluded_features) ;

  if (pParser->pHeader)
    zMapGFFHeaderDestroy(pParser->pHeader) ;

  if (pParser->sequence_name)
    g_free(pParser->sequence_name) ;

  /*
  if (pParser->nSequenceRecords)
  {
    if (pParser->pSeqData)
    {
      for (iValue=0; iValue<pParser->nSequenceRecords; ++iValue)
      {
        zMapSequenceDestroy(&pParser->pSeqData[iValue]) ;
      }
    }
  }
  */

  /*
   * The parser should not attempt to control the lifetime of the
   * sources hash table, since it did not create them.
   *
   * if (pParser->sources)
   *   g_hash_table_destroy(pParser->sources) ;
   */

  for (iValue=0; iValue<ZMAPGFF_NUMBER_PARSEBUF; ++iValue)
    g_free(pParser->buffers[iValue]) ;

  if (pParser->format_str)
    g_free(pParser->format_str) ;

  if (pParser->cigar_string_format_str)
    g_free(pParser->cigar_string_format_str) ;

  g_free(pParser) ;

  return ;
}




/*
 * Return the got_minimal_header flag. This style of header flags is only
 * used for version 3.
 */
gboolean zMapGFFGetHeaderGotMinimal_V3(const ZMapGFFParser const pParserBase)
{
  if (!pParserBase)
    return FALSE ;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  return (gboolean) pParser->pHeader->flags.got_min ;
}








/*
 * Remove everything including and after an unquoted '#' character
 * from the input string. Quoted here means "...", rather than single.
 * Simply replaces the '#' that is finds with '\0'.
 *
 * There is a test to skip over lines that start with "##" to avoid
 * truncatation of GFF directives.
 *
 * Returns TRUE if the operation was performed, FALSE otherwise.
 */
static gboolean removeCommentFromLine( char* sLine )
{
  static const char cQuote = '"',
    cHash = '#',
    cNull = '\0' ;
  gboolean bIsQuoted = FALSE,
    bResult = FALSE ;


  /*
   * We want to skip directive lines.
   */
  if (g_str_has_prefix(sLine, "##"))
    return FALSE ;

  /*
   * And operate upon the line.
   */
  for ( ; *sLine ; ++sLine )
    {
      if(*sLine == cQuote)
        bIsQuoted = !bIsQuoted;
      if(!bIsQuoted && *sLine == cHash)
        {
          *sLine = cNull ;
          bResult = TRUE ;
          break;
        }
    }

  return bResult ;
}











/*
 * Determine the line type that we are looking at.
 * See zmapGFF.h for the enum with definitions.
 */
static ZMapGFFLineType parserLineType(const char * const sLine)
{
  ZMapGFFLineType cTheType = ZMAPGFF_LINE_OTH ;

  if (zMapStringBlank((char*)sLine))
    {
      cTheType = ZMAPGFF_LINE_EMP ;
    }
  else if (g_str_has_prefix(sLine, "#"))
    {
      if (g_str_has_prefix(sLine, "##"))
        {
          if (g_str_has_prefix(sLine, "###"))
            cTheType = ZMAPGFF_LINE_CLO ;
          else if (g_str_has_prefix(sLine, "##DNA"))
            cTheType = ZMAPGFF_LINE_DNA ;
          else if (g_str_has_prefix(sLine, "##end-DNA"))
            cTheType = ZMAPGFF_LINE_EDN ;
          else if (g_str_has_prefix(sLine, "##FASTA"))
            cTheType = ZMAPGFF_LINE_FAS ;
          else
            cTheType = ZMAPGFF_LINE_DIR ;
        }
      else
        {
          cTheType = ZMAPGFF_LINE_COM ;
        }
    }
  else
    {
      cTheType = ZMAPGFF_LINE_BOD ;
    }

  return cTheType ;
}















/*
 * Encapsulate the finite state machine logic of the parser.
 */
static ZMapGFFParserState parserFSM(ZMapGFFParserState eCurrentState, const char * const sNewLine)
{
  ZMapGFFParserState eNewState = eCurrentState ;

  /*
   * If new line is blank or a comment then never change state.
   */
  ZMapGFFLineType eNewLineType = parserLineType(sNewLine) ;
  if ((eNewLineType == ZMAPGFF_LINE_EMP) || (eNewLineType == ZMAPGFF_LINE_COM))
    return eNewState ;

  /*
   * State changes under other cirsumstances are in the table above.
   */
  eNewState = ZMAP_GFF_PARSER_TRANSITIONS[eCurrentState][eNewLineType] ;

  return eNewState ;
}

















/*
 * Given a line length, will allocate buffers so they cannot overflow when parsing a line of this
 * length. The rationale here is that we might get handed a line that had just one field in it
 * that was the length of the line. By allocating buffers that are the line length we cannot
 * overrun our buffers even in this bizarre case !!
 *
 * Note that we attempt to avoid frequent reallocation by making buffer twice as large as required
 * (not including the final null char....).
 */
static gboolean resizeBuffers(ZMapGFFParser const pParser, gsize iLineLength)
{
  gboolean bNewAlloc = TRUE,
    bResized = FALSE ;
  char *sBuf[ZMAPGFF_NUMBER_PARSEBUF];
  unsigned int i, iNewLineLength ;

  if (!pParser || !iLineLength )
    return bResized ;

  if (iLineLength > pParser->buffer_length)
    {
      iNewLineLength = iLineLength * ZMAPGFF_BUF_MULT ;

      /*
       * Initialize our pointers and allocate some
       * new memory.
       */
      for (i=0; i<ZMAPGFF_NUMBER_PARSEBUF; ++i)
        sBuf[i] = NULL ;
      for (i=0; i<ZMAPGFF_NUMBER_PARSEBUF; ++i)
        {
          sBuf[i] = g_malloc0(iNewLineLength) ;
          if (!sBuf[i])
            {
              bNewAlloc = FALSE ;
              break ;
            }
        }

      /*
       * _if_ one or more of the allocations above failed,
       * then we delete them all and return a fail.
       *
       * _else_ new allocation succeeded and we can delete
       * the old memory and copy over pointers to new sections
       * and return success.
       */
      if (!bNewAlloc)
        {
          for (i=0; i<ZMAPGFF_NUMBER_PARSEBUF; ++i)
            {
              if (sBuf[i])
                g_free(sBuf) ;
            }
        }
      else
        {
          for (i=0; i<ZMAPGFF_NUMBER_PARSEBUF; ++i)
            {
              g_free(pParser->buffers[i]) ;
              pParser->buffers[i] = sBuf[i] ;
            }

          pParser->buffer_length = iNewLineLength ;
          bResized = TRUE ;
        }
    }


  return bResized ;
}






/*
 * OR... you could simply tokenize the line on \t characters and then deal with each of the
 * resulting tokens on it's own. Wouldn't that be easier? Possibly slower than using sscanf,
 * but it avoids this function completely, and is a hell of a lot easier.
 *
 *
 *
 *
 * Construct format strings to parse the main GFF3 fields and also sub-parts of a GFF3 line.
 * This needs to be done dynamically because we may need to change buffer size and hence
 * string format max length.
 *
 *
 * Notes on the format string for the main GFF3 fields:
 *
 * GFF3 version 2 format for a line is:
 *
 * <sequence> <source> <feature> <start> <end> <score> <strand> <phase> [attributes] [#comments]
 *
 * The format_str matches the above, splitting everything into its own strings, note that
 * some fields, although numerical, default to the char "." so cannot be simply scanned into
 * a number. The only tricky bit is to get at the attributes and comments which have
 * white space in them, this scanf format string seems to do it:
 *
 *  format_str = "%<length>s%<length>s%<length>s%d%d%<length>s%<length>s%<length>s %<length>[^#] %<length>c"
 *
 *  " %<length>[^#]" Jumps white space after the last mandatory field and then gets everything up to
 *              the next "#", so this will fail if people put a "#" in their attributes !
 *
 *  " %<length>c"    Reads everything from (and including) the "#" found by the previous pattern.
 *
 * If it turns out that people do have "#" chars in their attributes we will have do our own
 * parsing of this section of the line.
 *
 * mh17 NOTE BAM data has # in its attributes
 */
static gboolean resizeFormatStrs(ZMapGFFParser const pParser)
{
  gboolean bResized = TRUE ;
  GString *format_str ;
  static const char *align_format_str = "%%*[\"]" "%%%d" "[^\"]%%*[\"]%%*s" ;
  gsize iLength ;

  if (!pParser)
    return bResized ;

  /*
   * Max length of string we can parse using scanf(), the "- 1"
   * allows for the terminating null.
   */
  iLength = pParser->buffer_length - 1 ;

  /*
   * Redo main gff field parsing format string.
   */
  g_free(pParser->format_str) ;
  format_str = g_string_sized_new(ZMAPGFF_BUF_FORMAT_SIZE) ;

  /*
   * Lot's of "%"s here because "%%" is the way to escape a "%" !!
   * The G_GSSIZE_FORMAT is a portable way to printf a gsize.
   * get attributes + comments together and split later
   */
  g_string_append_printf(format_str,
    "%%%" G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s%%d%%d%%%"
    G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s %%%"G_GSSIZE_FORMAT "[^\n]s",
    iLength, iLength, iLength, iLength, iLength, iLength, iLength) ;

  pParser->format_str = g_string_free(format_str, FALSE) ;


  /*
   * Redo main gff field parsing format string.
   */
  g_free(pParser->cigar_string_format_str) ;
  format_str = g_string_sized_new(ZMAPGFF_BUF_FORMAT_SIZE) ;

  /*
   * this is what I'm trying to get:  "XXXXX %*[\"]%50[^\"]%*[\"]%*s" which parses a string
   * like this:
   *
   *       XXXXXXX "M335ID55M"  where XXXXX will be one of the supported alignment formats.
   *
   */
  g_string_append_printf(format_str, align_format_str, iLength) ;
  pParser->cigar_string_format_str = g_string_free(format_str, FALSE) ;

  return bResized ;
}


/*
 * Test whether or not the input line is a comment.
 */
static gboolean isCommentLine(const char * const sLine)
{
  if (!sLine || !*sLine)
    {
      return FALSE ;
    }
  else if (g_str_has_prefix(sLine, "#"))
    {
      if (g_str_has_prefix(sLine, "##") || g_str_has_prefix(sLine, "###"))
        return FALSE ;
      else
        return TRUE ;
    }
  else
    {
      return FALSE ;
    }
}


/*
 * Test whether or not line is Acedb error format. We should not see lines
 * of this form, but during the testing process they were passed through
 * accidentally.
 */
static gboolean isAcedbError(const char * const sLine)
{
  if (!sLine || !*sLine)
    {
      return FALSE ;
    }
  else if (g_str_has_prefix(sLine, "// ERROR"))
    {
      return TRUE ;
    }
  return FALSE ;
}





/*
 * Actions to initialize reading a sequence section. Current line must
 * be "##DNA" only.
 */
static gboolean initializeSequenceRead(ZMapGFFParser const pParserBase, const char * const sLine)
{
  gboolean bResult = TRUE ;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  if (!pParser || !pParser->pHeader)
    return FALSE ;

  /*
   * We must have already made the transition into the
   * parser state ZMAPGFF_PARSER_SEQ
   */
  if (pParser->state != ZMAPGFF_PARSER_SEQ)
    return FALSE ;

  if (!g_str_has_prefix(sLine, "##DNA"))
    {
      bResult = FALSE ;
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Error set in initializeSequenceRead(); current line does not have prefix ##DNA.") ;
      return bResult ;
    }

  /*
   * If this is true then we already have already seen line = "##DNA"
   */
  if (pParser->pHeader->flags.got_dna)
    {
      bResult = FALSE ;
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Error set in initializeSequenceRead(); have already set pHeader->flags.got_dna.") ;
      return bResult ;
    }

  /*
   * Then this is the first time that line = "##DNA"
   * has been seen.
   */
  pParser->pHeader->flags.got_dna = TRUE ;

  /*
   * Initialize reading a sequence.
   */
  pParser->seq_data.type = ZMAPSEQUENCE_DNA ;
  pParser->seq_data.name = g_quark_from_string(pParser->sequence_name) ;

  /*
   * Init parser for reading sequence data
   */
  if (!initParserForSequenceRead(pParserBase))
    {
      bResult = FALSE ;
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_FASTA,
                    "Error set in initializeSequenceRead(); unable to initialize parser for sequence reading") ;
      return bResult ;
    }

  return bResult ;
}







/*
 * Actions to initialize reading a fasta section. All this does is sets a flag if we
 * are seeing ##FASTA for the first time and returns an error otherwise.
 */
static gboolean initializeFastaRead(ZMapGFFParser const pParserBase, const char * const sLine)
{
  gboolean bResult = TRUE ;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  if (!pParser || !pParser->pHeader)
    return FALSE ;

  /*
   * We must have already made the transition into the
   * parser state ZMAPGFF_PARSER_FAS
   */
  if (pParser->state != ZMAPGFF_PARSER_FAS)
    return FALSE ;

  if (!g_str_has_prefix(sLine, "##FASTA"))
    {
      bResult = FALSE ;
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Error set in initializeFastaRead(); current line does not have prefix ##FASTA.") ;
      return bResult ;
    }

  /*
   * If this is true then we already have already seen line = "##FASTA"
   */
  if (pParser->pHeader->flags.got_fas)
    {
      bResult = FALSE ;
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Error set in initializeSequenceRead(); have already set pHeader->flags.got_fas") ;
      return bResult ;
    }

  /*
   * Then this is the first time that line = "##FASTA"
   * has been seen.
   */
  pParser->pHeader->flags.got_fas = TRUE ;

  return bResult ;
}








/*
 * Initialize the internal storage of the parser for some sequence to be read. Used for
 * both ##DNA and FASTA records. All we are doing here is making sure we start with an
 * empty g_string object to read into.
 */
static gboolean initParserForSequenceRead(ZMapGFFParser pParserBase)
{
  gboolean bResult = FALSE ;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  if (!pParser || !pParser->pHeader )
    return bResult ;

  bResult = TRUE ;
  if (pParser->raw_line_data != NULL )
    {
      g_string_free(pParser->raw_line_data, TRUE) ;
      pParser->raw_line_data = NULL ;
    }
  pParser->raw_line_data = g_string_new(NULL);
  if (!pParser->raw_line_data)
    {
      bResult = FALSE ;
    }

  return bResult ;
}








/*
 * Dummy function just for testing. Iterates over every feature in the
 * argument table. This is a model for functions to be called by
 * actionUponClosure().
 */
static gboolean iterationFunctionID(GQuark gqID, GHashTable *pValueTable)
{
  gboolean bResult = FALSE ;
  GHashTableIter ghIterator ;
  gpointer pKey = NULL, pValue = NULL;
  GQuark gqFeatureID = 0 ;
  if (!pValueTable)
    return bResult ;

  g_hash_table_iter_init(&ghIterator, pValueTable) ;
  while (g_hash_table_iter_next(&ghIterator, &pKey, &pValue))
    {
      gqFeatureID = GPOINTER_TO_INT( pKey ) ;
      /*
       * Work on feature->unique_id = gqFeatureID
       *
       * May also require access to parser here for context etc, so calling through
       * zMapMLFIDIteration() may not be appropriate solution.
       */
    }
  bResult = TRUE ;

  return bResult ;
}






/*
 * This is the semantic action to be performed upon encountering the
 * "###" directive. This should be doing something with the MLF data
 * stored in the parser. At the moment, just calls a dummy function.
 */
static gboolean actionUponClosure(ZMapGFFParser const pParserBase, const char* const sLine)
{
  gboolean bResult = TRUE ;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  if (!pParser || !pParser->pHeader || !pParser->pMLF)
    return FALSE ;

  printf("actionUponClosure() called, nID = %i\n", zMapMLFNumID(pParser->pMLF)) ;

  /*
   * Process datasets.
   */
  zMapMLFIDIteration(pParser->pMLF,  iterationFunctionID ) ;

  /*
   * When finished we empty the parser MLF structure.
   */
  if (!zMapMLFEmpty(pParser->pMLF))
    {
       pParser->error = g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                                    "Error in actionUponClosure(); line = %i",  pParser->line_count) ;
    }

  return bResult ;
}





/*
 * This function calls special actions that are required once and once only when
 * there is a change in parser state. This function should only be called if the
 * given new state and old states differ.
 *
 * Specific actions upon given state changes must be put in here explicitly; there
 * is no general mechanism for managing this, since it is relatively sparse.
 *
 */
static gboolean parserStateChange(ZMapGFFParser pParserBase,
                                  ZMapGFFParserState eOldState, ZMapGFFParserState eNewState,
                                  const char * const sLine)
{
  gboolean bResult = TRUE ;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  if (!pParser || !pParser->pHeader)
    return FALSE ;
  if (eOldState == eNewState)
    return FALSE ;

  /*
   * If we are entering ZMAPGFF_PARSER_SEQ
   * (i.e. current line is "##DNA"), call to initialize sequence reading
   */
  if (eNewState == ZMAPGFF_PARSER_SEQ)
    {
       bResult = initializeSequenceRead(pParserBase, sLine) ;
       return bResult ;
    }

  /*
   * If we are exiting ZMAPGFF_PARSER_SEQ
   * (i.e. current line is "##end-DNA"), call to close sequence reading
   */
  if (eOldState == ZMAPGFF_PARSER_SEQ)
    {
      bResult = finalizeSequenceRead(pParserBase, sLine) ;
      return bResult ;
    }

  /*
   * If we are entering ZMAPGFF_PARSER_FAS
   */
  if (eNewState == ZMAPGFF_PARSER_FAS)
    {
      bResult = initializeFastaRead(pParserBase, sLine) ;
      return bResult ;
    }

  /*
   * If we are entering ZMAPGFF_PARSER_CLO
   * (i.e. current line is "###"), special action for closure of features
   */
  if (eNewState == ZMAPGFF_PARSER_CLO)
    {
      bResult = actionUponClosure(pParserBase, sLine) ;
      return bResult ;
    }

  return bResult ;
}







/*
 * Actions to finalize reading a sequence section. Current line must be
 * "##end-DNA" only.
 */
static gboolean finalizeSequenceRead(ZMapGFFParser const pParserBase , const char* const sLine)
{
  gboolean bResult = TRUE,
    bCopyData = TRUE;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  if (!pParser || !pParser->pHeader )
    return FALSE ;

  if (!g_str_has_prefix(sLine, "##end-DNA"))
    {
      bResult = FALSE ;
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Error set in finalizeSequenceRead(); current line does not have prefix ##end-DNA.") ;
      return bResult ;
    }

  /*
   * If this is not true, then we have not yet seen line = "##DNA"
   */
  if (!pParser->pHeader->flags.got_dna)
    {
      bResult = FALSE ;
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Error set in finalizeSequenceRead(); pParser->pHeader->flags.got_dna has not already been set.") ;
      return bResult ;
    }

  /*
   * Finalise reading a sequence;
   */
  if (pParser->bCheckSequenceLength)
    {
      if ((pParser->features_end - pParser->features_start + 1) != pParser->seq_data.length)
        bCopyData = FALSE ;
    }

  if (bCopyData)
    {
      if (!copySequenceData(&pParser->seq_data, pParser->raw_line_data))
        {
          bResult = FALSE ;
          pParser->error =
            g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                        "Error set in finalizeSequenceRead(); failed to copy data.") ;
        }
    }

  /*
   * Free internal storage buffer.
   */
  g_string_free(pParser->raw_line_data, TRUE) ;
  pParser->raw_line_data = NULL ;

  return bResult ;
}




/*
 * Copy the data into the ZMapSequence object. Current sequence data is to be
 * overwritten. Data object copied from is freed if the flag is set.
 */
static gboolean copySequenceData(ZMapSequence pSequence, GString *pData)
{
  gboolean bResult = TRUE ;
  if (!pSequence || !pData)
    {
      bResult = FALSE ;
      return bResult ;
    }

  if (pSequence->sequence)
    {
      g_free(pSequence->sequence) ;
      pSequence->sequence = NULL ;
    }
  pSequence->sequence = g_strdup(pData->str) ;
  if (!pSequence->sequence)
    {
      bResult = FALSE ;
    }
  else
    {
      pSequence->length = pData->len ;
    }

  return bResult ;
}




/*
 * Add a new (empty) sequence record to the parser argument.
 */
static gboolean addNewSequenceRecord(ZMapGFFParser pParserBase)
{
  gboolean bResult = FALSE  ;
  ZMapSequenceStruct *pSeqTemp = NULL ;
  unsigned int nSequenceRecords , iSeq ;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  if (!pParser || !pParser->pHeader )
    return bResult ;
  nSequenceRecords = pParser->nSequenceRecords ;
  bResult = TRUE ;

  /*
   * create a new array one larger than the one in the parser
   */
  pSeqTemp = g_new0(ZMapSequenceStruct, nSequenceRecords+1) ;
  if (!pSeqTemp)
    {
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * copy existsing objects from RHS and initialize new one
   */
  for (iSeq=0; iSeq<nSequenceRecords; ++iSeq)
    {
      pSeqTemp[iSeq] = pParser->pSeqData[iSeq] ;
    }
  pSeqTemp[iSeq].name = 0;
  pSeqTemp[iSeq].length = 0;
  pSeqTemp[iSeq].type = ZMAPSEQUENCE_DNA ;
  pSeqTemp[iSeq].sequence = NULL ;

  /*
   * delete the old array and copy the new one
   */
  g_free(pParser->pSeqData) ;
  pParser->pSeqData = pSeqTemp ;

  /*
   * increment pParser->nSequenceRecords
   */
  ++pParser->nSequenceRecords ;

  return bResult ;
}




/*
 * Append current line to the current sequence record (the last one in the array).
 */
static gboolean appendToSequenceRecord(ZMapGFFParser pParserBase, const char * const sLine)
{
  gboolean bResult = FALSE ;
  unsigned int nSequenceRecords ;
  ZMapSequence pTheSequence = NULL ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (!pParser || !pParser->pHeader )
    return bResult ;

  bResult = TRUE ;

  /*
   * Access to current record.
   */
  nSequenceRecords = pParser->nSequenceRecords ;
  pTheSequence = &pParser->pSeqData[nSequenceRecords-1] ;

  /*
   * we should definitely not have entered here if no records have been
   * created...
   */
  if (nSequenceRecords == 0)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_FASTA,
                    "Error set in appendToSequenceRecord(); found data line not having seen '>...' line. l = '%s'", sLine) ;
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * collect new data line and append to pParser->raw_line_data and copy to
   * to the ZMapSequenceStruct object.
   */
  if (!g_string_append(pParser->raw_line_data, sLine))
    {
      bResult = FALSE ;
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_FASTA,
                    "Error set in appendToSequenceRecord(); call to g_string_append() returned NULL. l = '%s'", sLine) ;
      return bResult ;
    }

  /*
   * Copy the data into the appropriate sequence object.
   */
  if (!copySequenceData(pTheSequence, pParser->raw_line_data))
    {
      bResult = FALSE ;
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_FASTA,
                    "Error set in appendToSequenceRecord(); failed to copy sequence data.") ;
    }

  return bResult ;
}





/*
 * Parse a fasta line and assemble possibly more than one fasta records.
 * This means something of the form
 *       >record_name record_data
 *       <sequence data>*
 * As with all of the other parsing functions, blank lines and comments
 * are ignored. This is a relaxation of some FASTA specifications.
 *
 * Note that we assume that the sequence is DNA. This is not tested.
 */
static gboolean parseFastaLine_V3(ZMapGFFParser const pParserBase, const char* const sLine)
{
  unsigned int iFields ;
  gboolean bResult = FALSE ;
  ZMapSequence pTheSequence = NULL ;
  unsigned int nSequenceRecords = 0 ;
  static const char *sFmt = "%1000s%1000s" ;
  char sBuf01[1001], sBuf02[1001] ;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  /*
   * Some basic error checking.
   */
  if (!pParser || !pParser->pHeader)
    return bResult ;

  /*
   * If we are given sLine = "##FASTA" then this is the start, and just return
   * TRUE. Otherwise an error should be set if any other "##" line is encountered.
   */
  if (g_str_has_prefix(sLine, "##"))
    {
      if (g_str_has_prefix(sLine, "##FASTA"))
        {
          pParser->pHeader->flags.got_fas = TRUE ;
          bResult = TRUE ;
        }
      else
        {
          pParser->error =
            g_error_new(pParser->error_domain, ZMAPGFF_ERROR_FASTA,
                        "Error set in parseFastaLine(); found another directive at l = '%s'. ", sLine) ;
          bResult = FALSE ;
        }
      return bResult ;
    }

  /*
   * Now assume there are two types of lines only that we may encounter here.
   * (1) fasta record header, sLine = ">string1 string2"
   * (2) fasta data line
   * The following actions deal with these possibilities.
   */

  if (g_str_has_prefix(sLine, ">"))
    {

      /*
       * Init for reading sequence data
       */
      if (!initParserForSequenceRead(pParserBase))
        {
          bResult = FALSE ;
          pParser->error =
            g_error_new(pParser->error_domain, ZMAPGFF_ERROR_FASTA,
                        "Error set in parseFastaLine(); unable to initialize for sequence reading with l = '%s'", sLine) ;
          return bResult ;
        }

      /*
       * create a new array one larger than the current one
       */
      if (!addNewSequenceRecord(pParserBase))
        {
          bResult = FALSE ;
          pParser->error =
            g_error_new(pParser->error_domain, ZMAPGFF_ERROR_FASTA,
                        "Error set in parseFastaLine(); unable to add new sequence record with l = '%s'", sLine) ;
          return bResult ;
        }

      /*
       * Parse some data out of the current line and write to the
       * current sequence object; is it an error to have no strings in
       * the header of a fasta record?
       */
      nSequenceRecords = pParser->nSequenceRecords;
      pTheSequence = &pParser->pSeqData[nSequenceRecords-1] ;
      iFields = sscanf(sLine+1, sFmt, sBuf01, sBuf02 ) ;
      if ((iFields == 1) || (iFields == 2))
        {
          pTheSequence->name = g_quark_from_string(sBuf01) ;
        }
      else
        {
          /*
          bResult = FALSE ;
          pParser->error = g_error_new(pParser->error_domain, ZMAPGFF_ERROR_FASTA,
                                       "Error set in parseFastaLine(); unable to parse fasta header with l = '%s'", sLine) ;
          */
          pTheSequence->name = g_quark_from_string("ZMAPGFF_FASTA_NAME_NOT_FOUND") ;
        }

    }
  else
    {

      if (!appendToSequenceRecord(pParserBase, sLine))
        {
          bResult = FALSE ;
          pParser->error =
            g_error_new(pParser->error_domain, ZMAPGFF_ERROR_FASTA,
                        "Error set in parseFastaLine(); unable to append to sequence record with l = '%s'", sLine) ;
          return bResult ;
        }

    }

  return bResult ;
}





/*
 * Read a sequence line in old (v2) directive form, that is, something like
 *   ##ACTGACTGACTGACTGACTGAC...
 * No checking is done for validity of sequence.
 */
static gboolean parseSequenceLine_V3(ZMapGFFParser const pParserBase, const char * const sLine)
{
  gboolean bResult = FALSE ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (!pParser || !pParser->pHeader)
    return bResult ;

  if (pParser->state != ZMAPGFF_PARSER_SEQ)
    return bResult ;

  if (!g_str_has_prefix(sLine, "##"))
    return bResult ;

  /*
   * If this is called with the opening line, then
   * return doing nothing.
   */
  bResult = TRUE ;
  if (g_str_has_prefix(sLine, "##DNA"))
    {
      return bResult ;
    }

  /*
   * Now append the appropriate bit of the string to the parser's
   * sequence buffer.
   */
  if (!g_string_append(pParser->raw_line_data, sLine+2))
    {
      bResult = FALSE ;
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Error set in parseSequenceLine(); call to g_string_append() returned NULL. ") ;
    }

  return bResult ;
}








/*
 * What follows is a series of functions for each of the possible
 * directive types that we support. Appropriate error handling logic
 * is in the functions themselves, since this is hard to generalize.
 */


/*
 * GFF Version requires a single non-negative integer value. We support only 2 or 3.
 */
static gboolean parseDirective_GFF_VERSION(ZMapGFFParser const pParserBase, const char * const line)
{
  static const char *sFmt= "%*13s%d";
  static const unsigned int iExpectedFields = 1 ;
  gboolean bResult = TRUE ;
  unsigned int iFields,
    iVersion = (unsigned int) ZMAPGFF_VERSION_UNKNOWN ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (!pParser || !pParser->pHeader)
    return FALSE ;

  /*
   * Set an error if this directive has been seen already.
   */
  if (pParser->pHeader->flags.got_ver)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Duplicate ##gff-version pragma, line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * Now attempt to read the line for desired fields. If we don't get the appropriate
   * number an error should be set.
   */
  if ((iFields = sscanf(line, sFmt, &iVersion)) != iExpectedFields)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Bad ##gff-version line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * Now do some checking and write to the parser if all is well.
   */
  if ((iVersion != 3 ) && (iVersion != 2))
    {
       pParser->error =
         g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                     "Only GFF3 versions 2 or 3 supported, line %d: \"%s\"",
                     pParser->line_count, line) ;
       bResult = FALSE ;
    }
  else if (iVersion == 3 && pParser->line_count != 1)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "GFF3v3 \"##gff-version\" must be first line in file, line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
    }
  else
    {
      /* write data to general directive */
      pParser->pHeader->pDirective[ZMAPGFF_DIR_VER]->piData[0] = iVersion ;

      /* set header flag */
      pParser->pHeader->flags.got_ver = TRUE ;

      /* write data to parser */
      pParser->gff_version = (ZMapGFFVersion) iVersion ;

      /* special operation for this directive */
      zMapGFFHeaderMinimalTest(pParser->pHeader) ;
    }

  return bResult ;
}



/*
 * Sequence region is a string followed by two non-negative integers.
 */
static gboolean parseDirective_SEQUENCE_REGION(ZMapGFFParser const pParserBase, const char * const line)
{
  static const unsigned int iExpectedFields = 3 ;
  static const char *sFmt = "%*s%1000s%d%d" ;
  gboolean bResult = TRUE ;
  char sBuf[1001] = "" ;
  unsigned int  iFields, iStart = 0, iEnd = 0 ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (!pParser || !pParser->pHeader )
    return FALSE ;

  /*
   * If we have already read a directive of this form, we do nothing else, but this is
   * not considered to be an error.
   */
  if (pParser->pHeader->flags.got_sqr )
    return bResult ;

  /*
   * Attempt to read from the string. If the appropriate number of fields is not
   * recovered then an error should be set.
   */
  if ((iFields = sscanf(line, sFmt, &sBuf[0], &iStart, &iEnd)) != iExpectedFields)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Bad \"##sequence-region\" line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
      return bResult ;
    }


  /*
   * We require the names to match up here.
   */
  if (g_ascii_strcasecmp(sBuf, pParser->sequence_name) == 0)
    {

      if (iEnd < iStart)
        {
          pParser->error =
            g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                        "Invalid sequence/start/end:" " \"%s\" %d %d" " in header \"##sequence-region\" line %d: \"%s\"",
                        pParser->sequence_name, iStart, iEnd, pParser->line_count, line) ;
          bResult = FALSE ;
        }
      else if (iStart > pParser->features_end || iEnd < pParser->features_start)
        {
          pParser->error =
            g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                        "No overlap between original sequence/start/end:" " \"%s\" %d %d" " and header \"##sequence-region\" line %d: \"%s\"",
                        pParser->sequence_name, pParser->features_start, pParser->features_end, pParser->line_count, line) ;
          bResult = FALSE ;
        }
      else
        {
          /* write data to general directive */
          pParser->pHeader->pDirective[ZMAPGFF_DIR_SQR]->piData[0] = iStart ;
          pParser->pHeader->pDirective[ZMAPGFF_DIR_SQR]->piData[1] = iEnd ;
          pParser->pHeader->pDirective[ZMAPGFF_DIR_SQR]->psData[0] = g_strdup(sBuf) ;

          /* set header flag */
          pParser->pHeader->flags.got_sqr = TRUE ;

          /* write specific info to parser */
          pParser->features_start = iStart ;
          pParser->features_end = iEnd ;

          /* special operation for this directive */
          zMapGFFHeaderMinimalTest(pParser->pHeader) ;
        }

    } /* sequence name matches */


  return bResult ;
}


/*
 * The feature ontology should be a URL/URI string. This is not checked for validity in
 * any way.
 */
static gboolean parseDirective_FEATURE_ONTOLOGY(ZMapGFFParser const pParserBase , const char * const line)
{
  static const char* sFmt = "%*s%1000s" ;
  static const unsigned int iExpectedFields = 1 ;
  gboolean bResult = TRUE ;
  char sBuf[1001] = "" ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (!pParser || !pParser->pHeader)
    return FALSE ;

  /*
   * If we have read this directive already, then set an error.
   */
  if (pParser->pHeader->flags.got_feo)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Duplicate ##feature-ontology directive, line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * Now attempt to read the appropriate number of fields from the input string.
   */
  if (sscanf(line, sFmt, &sBuf[0]) != iExpectedFields )
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Bad \"##feature-ontology\" line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * Do a bit of error checking and write to parser if all is OK.
   */
  if (zMapStringBlank(sBuf))
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Extracted data is empty parseDirective_FEATURE_ONTOLOGY(); pParser->line_count = %i, line = '%s'",
                    pParser->line_count, line) ;
      bResult = FALSE ;
    }
  else
    {
      /* write data to directive object */
      pParser->pHeader->pDirective[ZMAPGFF_DIR_FEO]->psData[0] = g_strdup(sBuf) ;

      /* set header flag */
      pParser->pHeader->flags.got_feo  = TRUE ;
    }

  return bResult ;
}


/*
 * Function for the attribute ontology directive. Again, just reads a string.
 */
static gboolean parseDirective_ATTRIBUTE_ONTOLOGY(ZMapGFFParser const pParserBase, const char * const line)
{
  static const char* sFmt = "%*s%1000s" ;
  static const unsigned int iExpectedFields = 1 ;
  gboolean bResult = TRUE ;
  char sBuf[1001] = "" ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (!pParser || !pParser->pHeader)
    return bResult ;

  /*
   * If we have read this directive already, then set an error.
   */
  if (pParser->pHeader->flags.got_ato)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Duplicate ##attribute-ontology directive, line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * Now attempt to read the appropriate number of fields from the input string.
   */
  if (sscanf(line, sFmt, &sBuf[0]) != iExpectedFields )
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Bad \"##attribute-ontology\" line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * Do a bit of error checking and write to parser if all is OK.
   */
  if (zMapStringBlank(sBuf))
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Extracted data is empty in parseDirective_ATTRIBUTE_ONTOLOGY(); pParser->line_count = %i, line = '%s'",
                    pParser->line_count, line) ;
      bResult = FALSE ;
    }
  else
    {
      /* write data to directive object */
      pParser->pHeader->pDirective[ZMAPGFF_DIR_ATO]->psData[0] = g_strdup(sBuf) ;

      /* set header flag */
      pParser->pHeader->flags.got_ato  = TRUE ;
    }


  return bResult ;
}

/*
 * Function for the source ontology. Again, reads a single string.
 */
static gboolean parseDirective_SOURCE_ONTOLOGY(ZMapGFFParser const pParserBase, const char * const line)
{
  static const char* sFmt = "%*s%1000s" ;
  static const unsigned int iExpectedFields = 1 ;
  gboolean bResult = TRUE ;
  char sBuf[1001] = "" ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (!pParser || !pParser->pHeader)
    return bResult ;

  /*
   * If we have read this directive already, then set an error.
   */
  if (pParser->pHeader->flags.got_soo)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Duplicate ##source-ontology directive, line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
      return bResult ;
    }

/*
   * Now attempt to read the appropriate number of fields from the input string.
   */
  if (sscanf(line, sFmt, &sBuf[0]) != iExpectedFields )
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Bad \"##source-ontology\" line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * Do a bit of error checking and write to parser if all is OK.
   */
  if (zMapStringBlank(sBuf))
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Extracted data is empty in parseDirective_SOURCE_ONTOLOGY(); pParser->line_count = %i, line = '%s'",
                    pParser->line_count, line) ;
      bResult = FALSE ;
    }
  else
    {
      /* write data to directive object */
      pParser->pHeader->pDirective[ZMAPGFF_DIR_SOO]->psData[0] = g_strdup(sBuf) ;

      /* set header flag */
      pParser->pHeader->flags.got_soo  = TRUE ;
    }


  return bResult ;
}


/*
 * Function for the ##species; one string only.
 */
static gboolean parseDirective_SPECIES(ZMapGFFParser const pParserBase, const char * const line)
{
  static const char* sFmt = "%*s%1000s" ;
  static const unsigned int iExpectedFields = 1 ;
  gboolean bResult = TRUE ;
  char sBuf[1001] = "" ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (!pParser || !pParser->pHeader)
    return bResult ;

  /*
   * If we have read this directive already, then set an error.
   */
  if (pParser->pHeader->flags.got_spe )
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Duplicate ##species directive, line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * Now attempt to read the appropriate number of fields from the input string.
   */
  if (sscanf(line, sFmt, &sBuf[0]) != iExpectedFields )
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Bad \"##species\" line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * Do a bit of error checking and write to parser if all is OK.
   */
  if (zMapStringBlank(sBuf))
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Extracted data is empty in parseDirective_SPECIES(); pParser->line_count = %i, line = '%s'",
                    pParser->line_count, line) ;
      bResult = FALSE ;
    }
  else
    {
      /* write data to directive object */
      pParser->pHeader->pDirective[ZMAPGFF_DIR_SPE]->psData[0] = g_strdup(sBuf) ;

      /* set header flag */
      pParser->pHeader->flags.got_spe  = TRUE ;
    }


  return bResult ;
}




/*
 * Genome build; requires two strings for the name and source of assembly.
 */
static gboolean parseDirective_GENOME_BUILD(ZMapGFFParser const pParserBase, const char * const line)
{
  static const char* sFmt = "%*s%1000s%1000s" ;
  static const unsigned int iExpectedFields = 2 ;
  gboolean bResult = TRUE ;
  char sBuf01[1001] = "" , sBuf02[1001] = "" ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (!pParser || !pParser->pHeader)
    return bResult ;

  /*
   * If we have read this directive alre/ady, then set an error.
   */
  if (pParser->pHeader->flags.got_gen)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Duplicate ##genome-build directive, line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * Now attempt to read the appropriate number of fields from the input string.
   */
  if (sscanf(line, sFmt, &sBuf01[0], &sBuf02[0]) != iExpectedFields )
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Bad \"##genome-build\" line %d: \"%s\"",
                    pParser->line_count, line) ;
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * Do a bit of error checking and write to parser if all is OK.
   */
  if (zMapStringBlank(sBuf01) || zMapStringBlank(sBuf02))
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Extracted data is empty in parseDirective_GENOME_BUILD(); pParser->line_count = %i, line = '%s'",
                    pParser->line_count, line) ;
      bResult = FALSE ;
    }
  else
    {
      /* write data to directive object */
      pParser->pHeader->pDirective[ZMAPGFF_DIR_GEN]->psData[0] = g_strdup(sBuf01) ;
      pParser->pHeader->pDirective[ZMAPGFF_DIR_GEN]->psData[1] = g_strdup(sBuf02) ;

      /* set header flag */
      pParser->pHeader->flags.got_gen  = TRUE ;
    }

  return bResult ;
}






/*
 * This function expects a null-terminated C string that contains a GFF header line
 * which is a special form of comment line starting with a "##" in column 1.
 *
 * Functions are called for each particular type of directive we have implemented.
 *
 * Returns FALSE if passed a line that is not a header comment OR if there
 * was a parse error. In the latter case parser->error will have been set.
 */
static gboolean parseHeaderLine_V3(ZMapGFFParser const pParserBase, const char * const sLine)
{
  gboolean bResult = FALSE ;
  ZMapGFFDirectiveName eDirName ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (!pParser || !pParser->pHeader )
    return bResult ;

  if (!g_str_has_prefix(sLine, "##"))
    return bResult ;

  if (pParser->state != ZMAPGFF_PARSER_DIR)
    return bResult ;

#ifdef OUTPUT_V3_DATA
  writeHeaderLine( sLine ) ;
#endif

  eDirName = zMapGFFGetDirectiveName(sLine) ;

  /*
   * Now switch on the directive type.
   */
  switch (eDirName)
  {
    case ZMAPGFF_DIR_VER :
      bResult = parseDirective_GFF_VERSION(pParserBase, sLine) ;
      break ;
    case ZMAPGFF_DIR_SQR :
      bResult = parseDirective_SEQUENCE_REGION(pParserBase, sLine) ;
      break ;
    case ZMAPGFF_DIR_FEO :
      bResult = parseDirective_FEATURE_ONTOLOGY(pParserBase, sLine) ;
      break ;
    case ZMAPGFF_DIR_ATO :
      bResult = parseDirective_ATTRIBUTE_ONTOLOGY(pParserBase, sLine) ;
      break ;
    case ZMAPGFF_DIR_SOO :
      bResult = parseDirective_SOURCE_ONTOLOGY(pParserBase, sLine) ;
      break ;
    case ZMAPGFF_DIR_SPE :
      bResult = parseDirective_SPECIES(pParserBase, sLine) ;
      break ;
    case ZMAPGFF_DIR_GEN :
      bResult = parseDirective_GENOME_BUILD(pParserBase, sLine) ;
      break ;
    default:
      bResult = TRUE ;
      break ;
  }

  /*
   * If any of the above failed, communicate an error.
   */
  if (!bResult)
    pParser->pHeader->eState = GFF_HEADER_ERROR ;
  else
    pParser->pHeader->eState = ZMAPGFF_HEADER_OK ;

  /*
   * If the parser had an error set, then also communicate
   * an error.
   */
  if (pParser->error)
    {
      pParser->pHeader->eState = GFF_HEADER_ERROR;
      bResult = FALSE ;
    }

  return bResult ;
}






/*
 * New function for parsing GFF lines; incorporates the logic of the
 * zMapGFFParseHeader and zMapGFFParseLine functions.
 *
 * Essentially this works by there being two series of actions. The first
 * are actions to perform on lines whilst _in_ a given state (usually
 * parsing those lines for data). The second are actions to perform upon
 * encountering a change of state; there are fewer actions (and indeed
 * fewer functions) of this second type.
 */
gboolean zMapGFFParse_V3(ZMapGFFParser const pParserBase, char * const sLine)
{
  gboolean bResult = TRUE, bCommentRemoved = FALSE ;
  ZMapGFFParserState eOldState = ZMAPGFF_PARSER_NON,
    eNewState = ZMAPGFF_PARSER_NON;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  if (!pParser || !pParser->pHeader)
    return FALSE ;

  /*
   * Increment line count and skip over blank lines and comments.
   */
  pParser->line_count++ ;
  if (zMapStringBlank(sLine))
    return bResult ;
  if (isCommentLine(sLine))
    return bResult ;
  if (isAcedbError(sLine))
    return bResult ;

  /*
   * Remove trailing comment from line. Should do nothing to
   * directive lines (i.e. ones that start with "##").
   */
  bCommentRemoved = removeCommentFromLine( sLine ) ;

  /*
   * Parser FSM logic. Will be unaltered if we have a blank
   * line or a comment.
   */
  eOldState = pParser->state ;
  eNewState = parserFSM(eOldState, sLine) ;
  pParser->state = eNewState ;

  /*
   * Parser operations upon change of state.
   */
  if (eOldState != eNewState)
    {

      bResult = parserStateChange(pParserBase, eOldState, eNewState, sLine) ;
      if (!bResult && pParser->error)
        {
          pParser->state = ZMAPGFF_PARSER_ERR ;
          return bResult ;
        }

    }

  /*
   * Parser operations in various states.
   */
  if (pParser->state == ZMAPGFF_PARSER_BOD)
    {

      bResult = parseBodyLine_V3(pParserBase, sLine) ;
      if (!bResult && pParser->error && pParser->stop_on_error)
        {
          pParser->state = ZMAPGFF_PARSER_ERR ;
          return bResult ;
        }

    }
  else if (pParser->state == ZMAPGFF_PARSER_DIR)
    {

      bResult = parseHeaderLine_V3(pParserBase, sLine) ;
        if (!bResult && pParser->error)
        {
          pParser->state = ZMAPGFF_PARSER_ERR ;
          return bResult ;
        }

    }
  else if (pParser->state == ZMAPGFF_PARSER_SEQ)
    {

      bResult = parseSequenceLine_V3(pParserBase, sLine) ;
      if (!bResult && pParser->error && pParser->stop_on_error)
        {
          pParser->state = ZMAPGFF_PARSER_ERR ;
          return bResult ;
        }

    }
  else if (pParser->state == ZMAPGFF_PARSER_FAS)
    {

      bResult = parseFastaLine_V3(pParserBase, sLine) ;
      if (!bResult && pParser->error && pParser->stop_on_error)
        {
          pParser->state = ZMAPGFF_PARSER_ERR ;
          return bResult ;
        }
    }


  return bResult ;
}









/*
 * Return the flag whether or not to log warnings.
 */
gboolean zMapGFFGetLogWarnings(const ZMapGFFParser const pParserBase )
{
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (!pParser || !pParser->pHeader )
    return FALSE ;

  return pParser->bLogWarnings ;
}







/*
 * Parse out a body line for data and then call functions to create
 * appropriate new features.
 */
static gboolean parseBodyLine_V3(
                        ZMapGFFParser const pParserBase,
                        const char * const sLine
                      )
{
  static const unsigned int
    iTokenLimit                       = 1000
  ;

  unsigned int
    iStart                            = 0,
    iEnd                              = 0,
    iLineLength                       = 0,
    iFields                           = 0,
    nAttributes                       = 0,
    iSOID                             = ZMAPSO_ID_UNK
  ;

  char
    *sSequence                        = NULL,
    *sSource                          = NULL,
    *sType                            = NULL,
    *sScore                           = NULL,
    *sStrand                          = NULL,
    *sPhase                           = NULL,
    *sAttributes                      = NULL,
    *sSOIDName                        = NULL,
    *sErrText                         = NULL,
    **sTokens                         = NULL ;

  double
    dScore                            = 0.0
  ;

  gboolean
    bResult                           = TRUE,
    bHasScore                         = FALSE,
    bIncludeFeature                   = TRUE,
    bIncludeEmpty                     = FALSE,
    bRemoveQuotes                     = TRUE,
    bIsValidSOID                      = FALSE
  ;

  GQuark
    gqNameID                          = 0
  ;

  ZMapStyleMode
    cType                             = ZMAPSTYLE_MODE_BASIC
  ;
  ZMapHomol
    cHomol                            = ZMAPHOMOL_NONE
  ;
  ZMapStrand
    cStrand                           = ZMAPSTRAND_NONE
  ;
  ZMapPhase
    cPhase                            = ZMAPPHASE_NONE
  ;
  ZMapGFFAttribute
    *pAttributes                      = NULL,
    pAttribute                        = NULL
  ;
  ZMapGFFFeatureData
    pFeatureData                      = NULL
  ;
  ZMapSOErrorLevel
    cSOErrorLevel                     = ZMAPGFF_SOERRORLEVEL_UNK
  ;
  ZMapSOIDData
    pSOIDData                         = NULL
  ;

  printf("%s\n", sLine) ;
  fflush (stdout) ;

  /*
   * Cast to concrete type for GFFV3.
   */
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  /*
   * Initial error check.
   */
  if (!pParser || !pParser->pHeader)
    {
      bResult = FALSE ;
      goto return_point ;
    };

  /*
   * We only remove quotes from attribute strings with version 2, so this is
   * set to be always false in this function.
   */
  bRemoveQuotes = FALSE ;


  /*
   * If the line length is too large, then we exit with an error set.
   */
  iLineLength = strlen(sLine) ;
  if (iLineLength > ZMAPGFF_MAX_LINE_LEN)
    {
      if (pParser->error)
        {
          g_error_free(pParser->error) ;
          pParser->error = NULL ;
        }
      pParser->error = g_error_new(pParser->error_domain, ZMAPGFF_ERROR_BODY,
                    "Line length too long, line %d has length %d", pParser->line_count, iLineLength) ;
      bResult = FALSE ;
      goto return_point;
    }



  /*
   * Resize buffers if they are too small but within our maximum.
   */
  if (iLineLength > pParser->buffer_length)
    {
      /* If iLineLength increases then increase the length of the buffers that receive text so that
       * they cannot overflow and redo format string to read in more chars. */
      resizeBuffers(pParserBase, iLineLength) ;
      resizeFormatStrs(pParserBase) ;
      if (zMapGFFGetLogWarnings(pParserBase))
        zMapLogWarning("GFF parser buffers had to be resized to new line length: %d", pParser->buffer_length) ;
    }

  /*
   * These variables are for legibility.
   */
  sSequence       =   pParser->buffers[ZMAPGFF_BUF_SEQ] ;
  sSource         =   pParser->buffers[ZMAPGFF_BUF_SOU] ;
  sType           =   pParser->buffers[ZMAPGFF_BUF_TYP] ;
  sScore          =   pParser->buffers[ZMAPGFF_BUF_SCO] ;
  sStrand         =   pParser->buffers[ZMAPGFF_BUF_STR] ;
  sPhase          =   pParser->buffers[ZMAPGFF_BUF_PHA] ;
  sAttributes     =   pParser->buffers[ZMAPGFF_BUF_ATT] ;


  /*
   * Zero out buffers before use.
   */
  memset(sSequence,       0,    pParser->buffer_length) ;
  memset(sSource,         0,    pParser->buffer_length) ;
  memset(sType,           0,    pParser->buffer_length) ;
  memset(sScore,          0,    pParser->buffer_length) ;
  memset(sStrand,         0,    pParser->buffer_length) ;
  memset(sPhase,          0,    pParser->buffer_length) ;
  memset(sAttributes,     0,    pParser->buffer_length) ;

  /*
   * Tokenize input line. Don't have to worry about quoted delimiter characters here.
   */
  sTokens = zMapGFFStr_tokenizer(pParser->cDelimBodyLine, sLine, &iFields, bIncludeEmpty, iTokenLimit, g_malloc, g_free) ;

  /*
   * Check number of tokens found.
   */
  if (iFields < ZMAPGFF_MANDATORY_FIELDS)
    {
      if (pParser->error)
        {
          g_error_free(pParser->error) ;
          pParser->error = NULL ;
        }
      pParser->error = g_error_new(pParser->error_domain, ZMAPGFF_ERROR_BODY,
                                   "GFF line %d - Mandatory fields missing in: \"%s\"", pParser->line_count, sLine) ;
      bResult = FALSE ;
      goto return_point ;
    }

  /*
   * Parse each of these into the parser buffers.
   * We are using strcpy() here because some of these strings may
   * contain whitespace which makes use of sscanf() awkward.
   */
  if (iFields >= ZMAPGFF_MANDATORY_FIELDS)
    {
      strcpy(sSequence,    sTokens[0]) ;
      strcpy(sSource,      sTokens[1]) ;
      strcpy(sType,        sTokens[2]) ;
      sscanf(sTokens[3],   "%i", &iStart) ;
      sscanf(sTokens[4],   "%i", &iEnd) ;
      strcpy(sScore,       sTokens[5]) ;
      strcpy(sStrand,      sTokens[6]) ;
      strcpy(sPhase,       sTokens[7]) ;

    }
  if (iFields == ZMAPGFF_MANDATORY_FIELDS+1)
    {
      strcpy(sAttributes, sTokens[ZMAPGFF_MANDATORY_FIELDS]) ;
    }
  if (iFields > ZMAPGFF_MANDATORY_FIELDS+1)
    {
      if (pParser->error)
        {
          g_error_free(pParser->error) ;
          pParser->error = NULL ;
        }
      pParser->error = g_error_new(pParser->error_domain, ZMAPGFF_ERROR_BODY,
                                   "GFF line %d - too many \tab delimited fields in \"%s\"", pParser->line_count, sLine);
      bResult = FALSE ;
      goto return_point ;
    }


  /*
   * Ignore any lines with a different sequence name.
   */
  if (g_ascii_strcasecmp(sSequence, pParser->sequence_name) != 0)
    {
      bResult = TRUE ;
      /*
       * If we return false from here we end up with a failure with no
       * GError set, so I have modified to return TRUE but increment a counter
       * instead.
       */
      ++pParser->iNumWrongSequence ;
      goto return_point ;
    }

  /*
   * Parse/examine the mandatory fields first and throw an error if they
   * cannot be dealt with.
   */
  sErrText = NULL ;
  if (g_ascii_strcasecmp(sSequence, ".") == 0)
    sErrText = g_strdup("sSequence cannot be '.'") ;
  else if (g_ascii_strcasecmp(sSource, ".") == 0)
    sErrText = g_strdup("sSource cannot be '.'") ;
  else if (g_ascii_strcasecmp(sType, ".") == 0)
    sErrText = g_strdup("sType cannot be '.'") ;
  else if (!zMapFeatureFormatType(pParser->SO_compliant, pParser->default_to_basic, sType, &cType))
    sErrText = g_strdup_printf("feature_type not recognised: %s", sType) ;
  else if (iStart > iEnd)
    sErrText = g_strdup_printf("start > end, start = %d, end = %d", iStart, iEnd) ;
  else if (!zMapFeatureFormatScore(sScore, &bHasScore, &dScore))
    sErrText = g_strdup_printf("score format not recognised: %s", sScore) ;
  else if (!zMapFeatureFormatStrand(sStrand, &cStrand))
    sErrText = g_strdup_printf("strand format not recognised: %s", sStrand) ;
  else if (!zMapFeatureFormatPhase(sPhase, &cPhase))
    sErrText = g_strdup_printf("phase format not recognised: %s", sPhase) ;

  /*
   * sType must be either an accession number or a valid name from the
   * SO set that is currently is use.
   *
   * First we test to see if sType holds something of the form SO:XXXXXXX
   * where the number must be less than ZMAPSO_ID_UNK. If it is, then check
   * whether this is present in the SO set used.
   *
   * If the string is not of the form SO:XXXXXXX then check to see if it is
   * a name present in the same set. If either of these are true then we
   * have a valid SO term.
   *
   */
  if ((iSOID = zMapSOIDParseString(sType)) != ZMAPSO_ID_UNK) /* we have something of the form SO:XXXXXXX */
    {
      if ((sSOIDName = zMapSOSetIsIDPresent(pParser->cSOSetInUse, iSOID)) != NULL) /* that is present in the SO set */
        {
          bIsValidSOID = TRUE ;
          cType = zMapSOSetGetStyleModeFromID(pParser->cSOSetInUse, iSOID) ;
          cHomol = zMapSOSetGetHomolFromID(pParser->cSOSetInUse, iSOID) ;
          pSOIDData = zMapSOIDDataCreateFromData(iSOID, sSOIDName, cType, cHomol ) ;
        }
    }
  else if ((iSOID = zMapSOSetIsNamePresent(pParser->cSOSetInUse, sType)) != ZMAPSO_ID_UNK) /* we have sType is a name in the set */
    {
      bIsValidSOID = TRUE ;
      cType = zMapSOSetGetStyleModeFromName(pParser->cSOSetInUse, sType ) ;
      cHomol = zMapSOSetGetHomolFromID(pParser->cSOSetInUse, iSOID) ;
      pSOIDData = zMapSOIDDataCreateFromData(iSOID, sType, cType, cHomol ) ;
    }

  /*
   * If not a valid term, then perform error action specified
   * by the SO error level flag.
   */
  if (!bIsValidSOID)
    {
      cSOErrorLevel = zMapGFFGetSOErrorLevel(pParserBase) ;
      if (cSOErrorLevel == ZMAPGFF_SOERRORLEVEL_NONE)
        {
          /* Do nothing */
        }
      else if (cSOErrorLevel == ZMAPGFF_SOERRORLEVEL_WARN)
        {
          if (zMapGFFGetLogWarnings(pParserBase))
            zMapLogWarning("SO Term at line %i is not valid; 'type' string is = '%s'", pParser->line_count, sType) ;
        }
      else if (cSOErrorLevel == ZMAPGFF_SOERRORLEVEL_ERR)
        {
          sErrText = g_strdup_printf("SO Term at line %i is not valid; 'type' string is = '%s'", pParser->line_count, sType) ;
        }
    }

  /*
   * If we have set an error at this point, these are regarded as unrecoverable with
   * respect to the current feature so we set the parser error object and return.
   */
  if (sErrText)
    {
      if (pParser->error)
        {
          g_error_free(pParser->error) ;
          pParser->error = NULL ;
        }
      pParser->error = g_error_new(pParser->error_domain, ZMAPGFF_ERROR_BODY,
                                   "GFF line %d (a)- %s (\"%s\")", pParser->line_count, sErrText, sLine) ;
      g_free(sErrText) ;
      bResult = FALSE ;
      goto return_point ;
    }

  /*
   * Now tokenize the attributes if there are any.
   */
  if (iFields == ZMAPGFF_MANDATORY_FIELDS+1)
    {
      nAttributes = 0 ;
      pAttributes = zMapGFFAttributeParseList(pParserBase, sAttributes, &nAttributes, bRemoveQuotes) ;
    }

#ifdef OUTPUT_V3_DATA
  writeBodyLine(sTokens, iFields, pAttributes, nAttributes) ;
#endif

  /*
   * Fill in ZMapGFFFeatureData object here with the data parsed out so far.
   */
  pFeatureData = zMapGFFFeatureDataCreate() ;
  zMapGFFFeatureDataSet(pFeatureData,
                        sSequence,
                        sSource,
                        iStart,
                        iEnd,
                        bHasScore,
                        dScore,
                        cStrand,
                        cPhase,
                        sAttributes,
                        pAttributes,
                        nAttributes,
                        pSOIDData) ;
  if (!zMapGFFFeatureDataIsValid(pFeatureData))
    {

      if (pParser->error)
        {
          g_error_free(pParser->error) ;
          pParser->error = NULL ;
        }
      pParser->error = g_error_new(pParser->error_domain, ZMAPGFF_ERROR_BODY,
                                   "GFF body line; invalid ZMapGFFFEatureData object constructed; %i, '%s'", pParser->line_count, sLine) ;
      bResult = FALSE ;
      goto return_point ;
    }


  /*
   * Clip start/end as specified in clip_mode, default is to exclude.
   */
  if (pParser->clip_mode != GFF_CLIP_NONE)
    {

      /*
       * Name is needed; if there is no "Name" then this will fail (but would have in v2 anyway).
       */
      gqNameID = 0 ;
      pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Name" );
      if (pAttribute)
        {
          gqNameID = g_quark_from_string(zMapGFFAttributeGetTempstring(pAttribute)) ;
        }


      /* Anything outside always excluded. */
      if (pParser->clip_mode == GFF_CLIP_ALL || pParser->clip_mode == GFF_CLIP_OVERLAP)
        {
          if (iStart > pParser->clip_end || iEnd < pParser->clip_start)
            {
              bIncludeFeature = FALSE ;
            }
        }

      /* Exclude overlaps for CLIP_ALL */
      if (bIncludeFeature && pParser->clip_mode == GFF_CLIP_ALL)
        {
          if (iStart < pParser->clip_start || iEnd > pParser->clip_end)
            {
              bIncludeFeature = FALSE ;
            }
        }

      /* Clip overlaps for CLIP_OVERLAP */
      if (bIncludeFeature && pParser->clip_mode == GFF_CLIP_OVERLAP)
        {
          if (iStart < pParser->clip_start)
            iStart = pParser->clip_start ;
          if (iEnd > pParser->clip_end)
            iEnd = pParser->clip_end ;
        }

      /*
       * Children of features to be excluded must also be excluded, so names of excluded features
       * must be stored.
       */
      if (!bIncludeFeature)
        {
          g_hash_table_insert(pParser->excluded_features, GINT_TO_POINTER(gqNameID), GINT_TO_POINTER(gqNameID)) ;
          bResult = TRUE ;
        }
      else if ((g_hash_table_lookup(pParser->excluded_features, GINT_TO_POINTER(gqNameID))))
        {
          bIncludeFeature = FALSE ;
        }


    } /* if (pParser->clip_mode != ZMAPGFF_CLIP_NONE) */


    /*
     * If the feature is to be included then create the new feature object
     * and add to the appropriate feature set.
     */
    if (bIncludeFeature)
    {

      if ((bResult = makeNewFeature_V3(pParserBase,
                                    &sErrText,
                                    pFeatureData)))
        {
          /*
           * A new feature was successfully created and added to the
           * feature set.
           */
        }
      else
        {
          /*
           * otherwise we have an error
           */
          if (pParser->error)
            {
              g_error_free(pParser->error) ;
              pParser->error = NULL ;
            }
          pParser->error = g_error_new(pParser->error_domain, ZMAPGFF_ERROR_BODY,
                                       "GFF line %d (b) - %s (\"%s\")",  pParser->line_count, sErrText, sLine) ;
          g_free(sErrText) ;
          sErrText = NULL ;

        }

    }


  /*
   * Exit point for the function.
   */
return_point:

  /*
   * Clean up dynamically allocated data.
   */
  zMapGFFStr_array_delete(sTokens, iFields, g_free) ;
  zMapGFFAttributeDestroyList(pAttributes, nAttributes) ;
  zMapGFFFeatureDataDestroy(pFeatureData) ;

  return bResult ;
}






/*
 * Create a feature of ZMapStyleMode = TRANSCRIPT only.
 */
static ZMapFeature makeFeatureTranscript(const ZMapGFFFeatureData const pFeatureData,
                                         const ZMapFeatureSet const pFeatureSet,
                                         char ** psError)
{
  int iSOID = 0 ;
  char * sSOType = NULL ;
  ZMapFeature pFeature = NULL ;
  ZMapSOIDData pSOIDData = NULL ;
  ZMapStyleMode cFeatureStyleMode = ZMAPSTYLE_MODE_BASIC ;

  if (!psError || *psError)
    return pFeature ;
  if (!pFeatureData)
    return pFeature ;

  pSOIDData            = zMapGFFFeatureDataGetSod(pFeatureData) ;
  sSOType              = zMapSOIDDataGetName(pSOIDData) ;
  iSOID                = zMapSOIDDataGetID(pSOIDData) ;
  cFeatureStyleMode    = zMapSOIDDataGetStyleMode(pSOIDData) ;

  if (!pSOIDData)
    return pFeature ;
  if (!sSOType)
    return pFeature ;
  if (!iSOID)
    return pFeature ;
  if (cFeatureStyleMode != ZMAPSTYLE_MODE_TRANSCRIPT)
    return pFeature ;

  return pFeature ;
}


/*
 * Create a feature of ZMapStyleMode = ALIGNMENT only.
 */
static ZMapFeature makeAlignmentFeature(const ZMapGFFFeatureData const pFeatureData,
                                        char ** psError)
{
  ZMapFeature pFeature = NULL ;

  return pFeature ;
}


/*
 * Create a feature of ZMapStyleMode = BASIC only. This can be forced, even if the
 * style mode stored in the feature data is not BASIC using the boolean flag.
 */
static ZMapFeature makeBasicFeature(const ZMapGFFFeatureData const pFeatureData,
                                    gboolean bForce,
                                    char **psError)
{
  ZMapFeature pFeature = NULL ;

  return pFeature ;
}






/*
 * Create a new feature and add it to the feature set.
 *
 *
 */
static gboolean makeNewFeature_V3(
                         const ZMapGFFParser const pParserBase,
                         char ** const err_text,
                         const ZMapGFFFeatureData const pFeatureData
                       )
{
  unsigned int
    iStart                  = 0,
    iEnd                    = 0,
    nAttributes             = 0,
    iSOID                   = 0
  ;

  int
    iQueryStart             = 0,
    iQueryEnd               = 0,
    iTargetStart            = 0,
    iTargetEnd              = 0
  ;

  double
    dScore                  = 0.0
  ;

  char
    *sFeatureNameID         = NULL,
    *sFeatureName           = NULL,
    *sSequence              = NULL,
    *sSource                = NULL,
    *sAttributes            = NULL,
    *sSOType                = NULL,
    *sName                  = NULL,
    *sTargetID              = NULL
  ;

  gboolean
    bDataAdded              = FALSE,
    bHasAttributeID         = FALSE,
    bHasAttributeParent     = FALSE,
    bFeaturePresent         = FALSE,
    bFeatureAdded           = FALSE,
    bResult                 = FALSE,
    bFeatureHasName         = FALSE,
    bHasScore               = FALSE,
    bValidTarget            = FALSE
  ;

  GQuark
    gqThisID                = 0,
    gqFeatureStyleID        = 0,
    gqSourceID              = 0
  ;

  GArray
    *pGaps                  = NULL
  ;

  ZMapFeature
    pFeature                = NULL
  ;
  ZMapFeatureSet
    pFeatureSet             = NULL
  ;
  ZMapFeatureTypeStyle
    pFeatureStyle           = NULL
  ;
  ZMapGFFParserFeatureSet
    pParserFeatureSet       = NULL
  ;
  ZMapFeatureSource
    pSourceData             = NULL
  ;
  ZMapStyleMode
    cFeatureStyleMode       = ZMAPSTYLE_MODE_BASIC
  ;
  ZMapStrand
    cStrand                 = ZMAPSTRAND_NONE,
    cTargetStrand           = ZMAPSTRAND_NONE
  ;
  ZMapPhase
    cPhase                  = ZMAPPHASE_NONE
  ;
  ZMapSOIDData
    pSOIDData               = NULL
  ;
  ZMapGFFAttribute
    pAttribute              = NULL,
    pAttributeID            = NULL,
    pAttributeParent        = NULL,
    pAttributeTarget        = NULL,
    pAttributeGap           = NULL,
    *pAttributes            = NULL
  ;
  ZMapSpanStruct
    cSpanItem               = {0},
    *pSpanItem               = NULL
  ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (!pParser || !pParser->pHeader )
  {
    bResult = FALSE ;
    goto return_point ;
  }

  /*
   * Take some data from the ZMapGFFFeatureData object
   */
  sSequence            = zMapGFFFeatureDataGetSeq(pFeatureData) ;
  sSource              = zMapGFFFeatureDataGetSou(pFeatureData) ;
  iStart               = zMapGFFFeatureDataGetSta(pFeatureData) ;
  iEnd                 = zMapGFFFeatureDataGetEnd(pFeatureData) ;
  bHasScore            = zMapGFFFeatureDataGetFlagSco(pFeatureData) ;
  dScore               = zMapGFFFeatureDataGetSco(pFeatureData) ;
  cStrand              = zMapGFFFeatureDataGetStr(pFeatureData) ;
  cPhase               = zMapGFFFeatureDataGetPha(pFeatureData) ;
  sAttributes          = zMapGFFFeatureDataGetAtt(pFeatureData) ;
  nAttributes          = zMapGFFFeatureDataGetNat(pFeatureData) ;
  pAttributes          = zMapGFFFeatureDataGetAts(pFeatureData) ;

  pSOIDData            = zMapGFFFeatureDataGetSod(pFeatureData) ;
  sSOType              = zMapSOIDDataGetName(pSOIDData) ;
  iSOID                = zMapSOIDDataGetID(pSOIDData) ;
  cFeatureStyleMode    = zMapSOIDDataGetStyleMode(pSOIDData) ;

  /*
   * A CDS feature _must_ have a phase which is not ZMAPPHASE_NONE.
   *
   * All features other than CDS _must_ have cPhase = '.' (i.e. ZMAPPHASE_NONE).
   * Anything else is an error.
   */
  if (!strcmp(sSOType, "CDS"))
    {
      if (cPhase == ZMAPPHASE_NONE)
        {
          bResult = FALSE ;
          goto return_point ;
        }
    }
  else
    {
      if (cPhase != ZMAPPHASE_NONE)
        {
          bResult = FALSE ;
          goto return_point ;
        }
    }


  /*
   * If the parser was given a source -> data mapping then
   * use that to get the style id and other
   * data otherwise use the source itself.
   */
  if (pParser->source_2_sourcedata)
    {
      gqSourceID = zMapFeatureSetCreateID(sSource);

      if (!(pSourceData = g_hash_table_lookup(pParser->source_2_sourcedata, GINT_TO_POINTER(gqSourceID))))
        {
          /* need to invent this for autoconfigured servers */
          pSourceData = g_new0(ZMapFeatureSourceStruct,1);
          pSourceData->source_id = gqSourceID;
          pSourceData->source_text = gqSourceID;

          /* this is the same hash owned by the view & window */
          g_hash_table_insert(pParser->source_2_sourcedata,GINT_TO_POINTER(gqSourceID), pSourceData);
        }

      if (pSourceData->style_id)
        gqFeatureStyleID = zMapStyleCreateID((char *) g_quark_to_string(pSourceData->style_id)) ;
      else
        gqFeatureStyleID = zMapStyleCreateID((char *) g_quark_to_string(pSourceData->source_id)) ;

      gqSourceID = pSourceData->source_id ;
      pSourceData->style_id = gqFeatureStyleID;
    }
  else
    {
      gqSourceID = gqFeatureStyleID = zMapStyleCreateID(sSource) ;
    }



  /*
   * Find the feature set from the parser associated with the source, and
   * create it if it does not already exist.
   */
  if (!(pParserFeatureSet = getParserFeatureSet(pParserBase, sSource)))
    {
      bResult = FALSE ;
      goto return_point ;
    }


  /*
   * Now this is doing some lookups to find styles for features (or at least for the
   * feature set at once).
   */
  if (!(pFeatureStyle = (ZMapFeatureTypeStyle)g_hash_table_lookup(pParserFeatureSet->feature_styles, GUINT_TO_POINTER(gqFeatureStyleID))))
    {
      if (!(pFeatureStyle = zMapFindFeatureStyle(pParser->sources, gqFeatureStyleID, cFeatureStyleMode)))
        {
          gqFeatureStyleID = g_quark_from_string(zmapStyleMode2ShortText(cFeatureStyleMode)) ;
        }

      if (!(pFeatureStyle = zMapFindFeatureStyle(pParser->sources, gqFeatureStyleID, cFeatureStyleMode)))
        {
          *err_text = g_strdup_printf("feature ignored, could not find style \"%s\" for feature set \"%s\".",
                                      g_quark_to_string(gqFeatureStyleID), sSource) ;
          bResult = FALSE ;
          goto return_point ;
        }

      if (pSourceData)
        pSourceData->style_id = gqFeatureStyleID;

      g_hash_table_insert(pParserFeatureSet->feature_styles,GUINT_TO_POINTER(gqFeatureStyleID),(gpointer) pFeatureStyle);

      if (pSourceData && pFeatureStyle->unique_id != gqFeatureStyleID)
        pSourceData->style_id = pFeatureStyle->unique_id;
    }


  /*
   * with one type of feature in a featureset this should be ok
   */
  pParserFeatureSet->feature_set->style = pFeatureStyle;
  pFeatureSet = pParserFeatureSet->feature_set ;

  /*
   * Now check that the ZMapStyleMode of the current style also has the same ZMapStyleMode.
   * If it is not, then I treat this as an unrecoverable error for the current feature.
   */
  //cStyleStyleMode = pFeatureStyle->mode ;
  //if (cFeatureStyleMode != cStyleStyleMode)
  //  {
  //    *err_text = g_strdup_printf("feature ignored, ZMapStyleMode for feature and Style do not match. Style = '%s', source = '%s'",
  //                                g_quark_to_string(gqFeatureStyleID), sSource) ;
  //    bResult = FALSE ;
  //    goto return_point ;
  //  }


  /*
   * This construction is only to be attempted if we are dealing with a
   * transcript-like feature.
   */
  if (cFeatureStyleMode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {

      /*
       * We first look for ID and Parent attributes.
       */
      pAttributeID = zMapGFFAttributeListContains(pAttributes, nAttributes, "ID") ;
      if (pAttributeID)
        bHasAttributeID = TRUE ;
      pAttributeParent = zMapGFFAttributeListContains(pAttributes, nAttributes, "Parent");
      if (pAttributeParent)
        bHasAttributeParent = TRUE ;

      /*
       * Find the feature->unique_id we are dealing with.
       */
      if (bHasAttributeID)
        {
          pAttribute = pAttributeID ;
        }
      else if (bHasAttributeParent)
        {
          pAttribute = pAttributeParent ;
        }
      gqThisID = g_quark_from_string(zMapGFFAttributeGetTempstring(pAttribute)) ;
      if (gqThisID == 0)
        goto return_point ;

      /*
       * Lookup to see if a feature with this unique_id is already in the
       * feature set.
       */
      pFeature = g_hash_table_lookup(((ZMapFeatureAny)pFeatureSet)->children, GINT_TO_POINTER(gqThisID)) ;
      if (pFeature)
        bFeaturePresent = TRUE ;

      /*
       * Now the logic branches depending on whether or not we found the ID or Parent
       * attribute in the current line.
       *
       * (1) ID => gqThisID:
       *
       * (2) Parent => gqThisID:
       *
       */
      if (bHasAttributeID)
        {

          if (bFeaturePresent)
            {
              /*
               * Then we must have seen a "ID" _or_ "Parent" line associated with gqThisID already.
               */
            }
          else
            {
              /*
               * This is a new feature, so we must create from scratch and add standard data.
               */
              pFeature = zMapFeatureCreateEmpty() ;
              ++pParser->num_features ;

              /*
               * Name of feature is taken from ID attribute value.
               */
              sFeatureName = g_strdup(g_quark_to_string(gqThisID)) ;
              sFeatureNameID = g_strdup(g_quark_to_string(gqThisID)) ;
              bResult = zMapFeatureAddStandardData(pFeature, sFeatureNameID, sFeatureName, sSequence, sSOType,
                                                   cFeatureStyleMode, &pParserFeatureSet->feature_set->style,
                                                   iStart, iEnd, bHasScore, dScore, cStrand) ;

              /*
               * Add feature to featureset
               */
              bFeatureAdded = zMapFeatureSetAddFeature(pFeatureSet, pFeature) ;
              if (!bFeatureAdded)
                {
                  *err_text = g_strdup_printf("feature with gqThisID = %i and name = '%s' could not be added", (int)gqThisID, sFeatureName) ;
                  bResult = FALSE ;
                  goto return_point ;
                }

            }

        }
      else if (bHasAttributeParent)
        {
          if (bFeaturePresent)
            {
              /*
               * Then we must have seen a "ID" and created the basic data for the object. Look
               * for exon, intron or CDS data.
               */
               if (!strcmp(sSOType, "exon"))
                 {
                   cSpanItem.x1 = iStart ;
                   cSpanItem.x2 = iEnd ;
                   pSpanItem = &cSpanItem;
                   bDataAdded = zMapFeatureAddTranscriptExonIntron(pFeature, pSpanItem, NULL) ;
                 }
               else if (!strcmp(sSOType, "intron"))
                 {
                   cSpanItem.x1 = iStart ;
                   cSpanItem.x2 = iEnd ;
                   pSpanItem = &cSpanItem ;
                   bDataAdded = zMapFeatureAddTranscriptExonIntron(pFeature, NULL, pSpanItem) ;
                 }
               else if (!strcmp(sSOType, "CDS"))
                 {
                   /*
                    * How/where is the phase datum added to the CDS feature?
                    */
                   cSpanItem.x1 = iStart ;
                   cSpanItem.x2 = iEnd ;
                   pSpanItem = &cSpanItem ;
                   bDataAdded = zMapFeatureAddTranscriptCDSDynamic(pFeature, iStart, iEnd, cPhase) ;
                 }

               if (!bDataAdded)
                 {
                   *err_text = g_strdup_printf("unable to add ZMapSpan to feature with gqThisID = %i and name = '%s'",
                                               (int)gqThisID, sFeatureName) ;
                   bResult = FALSE ;
                   goto return_point ;
                 }

            }
          else
            {
              /* This case is not treated. */
            }

        }

    } /* if (cFeatureStyleMode == ZMAPSTYLE_MODE_TRANSCRIPT) */

  else if (cFeatureStyleMode == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      pAttributeTarget = zMapGFFAttributeListContains(pAttributes, nAttributes, "Target") ;
      if (pAttributeTarget)
        bValidTarget = zMapAttParseTarget(pAttributeTarget, &sTargetID, &iTargetStart, &iTargetEnd, &cTargetStrand) ;

      if (bValidTarget)
        {
          /*
           * We have a valid Target attribute: we now:
           * (1) create feature
           * (2) find feature name
           * (3) add standard data
           * (2) look for "Gap" attribute (which might not be there)
           * (3) add data to feature using zMapFeatureAddAlignmentData
           * (4) add feature to feature set
           */
          pFeature = zMapFeatureCreateEmpty() ;
          ++pParser->num_features ;
          bFeatureAdded = FALSE ;

          //sFeatureName = NULL; // g_strdup(g_quark_to_string(gqThisID)) ;
          //sFeatureNameID = NULL; // g_strdup(g_quark_to_string(gqThisID)) ;

          //bResult = zMapFeatureAddStandardData(pFeature, sFeatureNameID, sFeatureName, sSequence, sSOType,
          //                                     cFeatureStyleMode, &pParserFeatureSet->feature_set->style,
          //                                     iStart, iEnd, bHasScore, dScore, cStrand) ;
          //pAttributeGap = zMapGFFAttributeListContains(pAttributes, nAttributes, "Gap") ;
          //if (pAttributeGap)
          //  {
          //     /* Parse this into a Gaps array... */
          //  }
          //bDataAdded = zMapFeatureAddAlignmentData(pFeature, 0, 0.0, iTargetStart, iTargetEnd, ZMAPHOMOL_NONE,
          //                                         0, cTargetStrand, cPhase, pGaps,
          //                                         zMapStyleGetWithinAlignError(pFeatureStyle),
          //                                         FALSE, NULL )  ;
          //bFeatureAdded = zMapFeatureSetAddFeature(pFeatureSet, pFeature) ;
          //if (!bFeatureAdded)
          //  {
          //    *err_text = g_strdup_printf("feature with name = '%s', '%s' could not be added", sFeatureName, sFeatureNameID) ;
          //    bResult = FALSE ;
          //    goto return_point ;
          //  }

        }


    } /* if (cFeatureStyleMode == ZMAPSTYLE_MODE_ALIGNMENT */

  else
    {


      /*
       * The following section creates a new feature from scratch and adds to the featureset.
       * This is more or less a simplified version of what's in v2 code.
       */

      /*
       * Look for "Name" attribute
       */
      pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Name" );
      if (pAttribute)
        {
          sName = zMapGFFAttributeGetTempstring(pAttribute) ;
        }
      /*
       * Get the sFeatureName which may not be unique and sFeatureNameID which
       * _must_ be unique.
       */
      bFeatureHasName = getFeatureName(sSequence, pAttributes, nAttributes, sName, sSource,
                                       cFeatureStyleMode, cStrand, iStart, iEnd, iQueryStart, iQueryEnd,
                                       &sFeatureName, &sFeatureNameID) ;
      if (!sFeatureNameID)
        {
          *err_text = g_strdup_printf("feature ignored as it has no name");
          bResult = FALSE ;
          goto return_point ;
        }

      /*
       * Create our new feature object
       */
      pFeature = zMapFeatureCreateEmpty() ;
      ++pParser->num_features ;

      /*
       * Add standard data to the feature.
       */
      bResult = zMapFeatureAddStandardData(pFeature, sFeatureNameID, sFeatureName, sSequence, sSOType,
                                           cFeatureStyleMode, &pParserFeatureSet->feature_set->style,
                                           iStart, iEnd, bHasScore, dScore, cStrand);

      if (!bResult)
        {
          goto return_point ;
        }

      /*
       * Add the feature to feature set
       */
      bFeatureAdded = zMapFeatureSetAddFeature(pFeatureSet, pFeature) ;
      if (!bFeatureAdded)
        {
          *err_text = g_strdup_printf("feature with name = '%s' could not be added", sFeatureName) ;
          bResult = FALSE ;
          goto return_point ;
        }


    } /* final ZMapStyleMode clause */



  /*
   * Now deal with other attributes as necessary.
   */


  /*
   * "Alias" Attribute
   */
  //pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Alias") ;
  //if (pAttribute)
  //{
  //  if (zMapAttParseAlias(pAttribute, &sAlias_v3A))
  //  {
  //      /* Do something with these data */
  //  }
  //}


  /*
   * "Parent" Attribute
   */
  //pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Parent") ;
  //if (pAttribute)
  //{
  //  if (zMapAttParseParent(pAttribute, &sParent_v3A))
  //  {
  //    /* Do something with these data */
  //  }
  //}


  /*
   * "Target" Attribute
   */
  //pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Target") ;
  //if (pAttribute)
  //{
  //  if (zMapAttParseTarget(pAttribute, &sTarget_v3A, &iStart_v3a, &iEnd_v3A, &cStrand_v3a))
  //  {
  //    /* Do something with these data */
  //  }
  //}


  /*
   * "Gap" Attribute
   */
  //pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Gap") ;
  //if (pAttribute)
  //{
  //  pGapSeries = zMapGapSeriesCreate() ;
  //  if (zMapAttParseGap(pAttribute, pGapSeries))
  //  {
  //    printf("zMapAttParseGap() sucessfully called. \n") ;
  //    /* Do something with the data here */
  //  }
  //}


  /*
   * "Derives_from" Attribute
   */
  //pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Derives_from") ;
  //if (pAttribute)
  //{
  //  if (zMapAttParseDerives_from(pAttribute, &sDerives_from_v3A))
  //  {
  //    /* Do something with data here */
  //  }
  //}


  /*
   * "Note" Attribute
   */
  //pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Note") ;
  //if (pAttribute)
  //{
  //  if (zMapAttParseNote(pAttribute, &sNote_v3A))
  //  {
  //    /* Do something with data here */
  //  }
  //}


  /*
   * "Dbxref" Attribute
   */
  //pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Dbxref") ;
  //if (pAttribute)
  //{
  //  if (zMapAttParseDbxref(pAttribute, &sDBX1_v3A, &sDBX2_v3A))
  //  {
  //    /* Do something with data here */
  //  }
  //}


  /*
   * "Ontology_term" Attribute
   */
  //pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Ontology_term") ;
  //if (pAttribute)
  //{
  //  if (zMapAttParseOntology_term(pAttribute, &sOT1_v3A, &sOT2_v3A))
  //  {
  //    /* Do something with data here */
  //  }
  //}


  /*
   * "Is_circular" Attribute
   */
  //pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Is_circular") ;
  //if (pAttribute)
  //{
  //  if (zMapAttParseIs_circular(pAttribute, &bIsCircular_v3A))
  //  {
  //    /* Do something with data here */
  //  }
  //}



  /*
   *
   * Now test for some "non-standard" attributes we also use.
   *
   *
   */


  /*
   * "cigar_exonerate" Attribute
   */
  //pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "cigar_exonerate") ;
  //if (pAttribute)
  //{
  //  if (zMapAttParseCigarExonerate(pAttribute, &pGaps, cStrand, iStart, iEnd, cQueryStrand, iQueryStart, iQueryEnd) )
  //  {
  //    /* Do something with data here  */
  //    printf("cigar_exonerate passed.\n") ;
  //    fflush(stdout) ;
  //  }
  //  else
  //  {
  //    printf("cigar_exonerate failed.\n") ;
  //    fflush(stdout) ;
  //  }
  //}


  /*
   * "cigar_ensembl" Attribute
   */
  //pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "cigar_ensembl") ;
  //if (pAttribute)
  //{
  //  if (zMapAttParseCigarEnsembl(pAttribute, &pGaps, cStrand, iStart, iEnd, cQueryStrand, iQueryStart, iQueryEnd))
  //  {
  //    /* Do something with data here */
  //    printf("cigar_ensembl passed.\n") ;
  //    fflush(stdout) ;
  //  }
  //  else
  //  {
  //    printf("cigar_ensembl failed.\n") ;
  //    fflush(stdout) ;
  //  }
  //}


  /*
   * "cigar_bam" Attribute
   */
  //pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "cigar_bam" ) ;
  //if (pAttribute)
  //{
  //  if (zMapAttParseCigarBam(pAttribute, &pGaps, cStrand, iStart, iEnd, cQueryStrand, iQueryStart, iQueryEnd))
  //  {
  //    /* Do something with data here */
  //    printf("cigar_bam passed.\n") ;
  //    fflush(stdout) ;
  //  }
  //  else
  //  {
  //    printf("cigar_bam failed.\n") ;
  //    fflush(stdout) ;
  //  }
  //}


  /*
   * Return point for the function. Temporary, probably.
   */
return_point:

  /*
   *  Clean up
   */
  if (!bFeatureAdded && pFeature)
    zMapFeatureDestroy(pFeature) ;
  if (sFeatureName)
    g_free(sFeatureName) ;
  if (sFeatureNameID)
    g_free(sFeatureNameID) ;
  if (sTargetID)
    g_free(sTargetID) ;

  return bResult ;
}


















    /*
     * This is old stuff taken out of parseBodyLine_V3() that I'm not doing any more.
     * Note in particular that "Locus" is not a valid attribute name in V3.
     */

    /* pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Locus" );
    if ( pParser->locus_set_id &&  pAttribute )
    {

      if (!zMapFeatureFormatType(pParser->SO_compliant, pParser->default_to_basic, ZMAP_FIXED_STYLE_LOCUS_NAME, &cType))
        sErrText = g_strdup_printf("feature_type not recognised: %s", ZMAP_FIXED_STYLE_LOCUS_NAME) ;

      pFeatureData02 = zMapGFFFeatureDataCreate() ;
      zMapGFFFeatureDataSet(pFeatureData02,
                            zMapGFFAttributeGetTempstring(pAttribute),
                            ZMAP_FIXED_STYLE_LOCUS_NAME,
                            ZMAP_FIXED_STYLE_LOCUS_NAME,
                            cType,
                            iStart,
                            iEnd,
                            FALSE,
                            0.0,
                            ZMAPSTRAND_NONE,
                            ZMAPPHASE_NONE,
                            sAttributes,
                            pAttributes,
                            nAttributes) ;

      if ((bResult = makeNewFeature_V3((ZMapGFFParser) pParser,
                                    ZMAPGFF_NAME_USE_SEQUENCE,
                                    &sErrText,
                                    pFeatureData02)))
      {

      }
      else
      {
        if (pParser->error)
        {
          g_error_free(pParser->error) ;
          pParser->error = NULL ;
        }
        pParser->error = g_error_new(pParser->error_domain, ZMAPGFF_ERROR_BODY,
                                     "GFF line %d (c) - %s (\"%s\")", pParser->line_count, sErrText, sLine) ;
        g_free(sErrText) ;
        sErrText = NULL ;
      }

      zMapGFFFeatureDataDestroy(pFeatureData02) ;
    }

  */























/*
 * Sets the SO collection flag. TRUE means use SO, FALSE means use SOFA.
 */
gboolean zMapGFFSetSOSetInUse(ZMapGFFParser const pParserBase, ZMapSOSetInUse cUse)
{
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  gboolean bSet = FALSE ;
  if (!pParser || !pParser->pHeader )
    return bSet ;
  pParser->cSOSetInUse = cUse ;
  bSet = TRUE ;
  return bSet ;
}

/*
 * Returns the SO collection flag.
 */
ZMapSOSetInUse zMapGFFGetSOSetInUse(const ZMapGFFParser const pParserBase )
{
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  if (!pParser || !pParser->pHeader )
    return ZMAPSO_USE_NONE ;
  return pParser->cSOSetInUse ;
}




/*
 * Set the SO Error level within the parser.
 */
gboolean zMapGFFSetSOErrorLevel(ZMapGFFParser const pParserBase, ZMapSOErrorLevel cErrorLevel)
{
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  gboolean bSet = FALSE ;
  if (!pParser || !pParser->pHeader || cErrorLevel < ZMAPGFF_SOERRORLEVEL_UNK )
    return bSet ;
  bSet = TRUE ;
  pParser->cSOErrorLevel = cErrorLevel ;
  return bSet ;
}


/*
 * Return the error level setting within the parser.
 */
ZMapSOErrorLevel zMapGFFGetSOErrorLevel(const ZMapGFFParser const pParserBase )
{
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  if (!pParser || !pParser->pHeader)
    return 0 ;
  return pParser->cSOErrorLevel ;
}






/* This is a GDestroyNotify() and is called for each element in a GData list when
 * the list is cleared with g_datalist_clear (), the function must free the GArray
 * and GData lists. */
static void destroyFeatureArray(gpointer data)
{
  ZMapGFFParserFeatureSet parser_feature_set = (ZMapGFFParserFeatureSet)data ;

  /* No data to free in this list, just clear it. */
  g_datalist_clear(&(parser_feature_set->multiline_features)) ;
  if(parser_feature_set->feature_set && parser_feature_set->feature_set->style)
      zMapStyleDestroy(parser_feature_set->feature_set->style);

  g_hash_table_destroy(parser_feature_set->feature_styles);       // styles remain pointed at by features

  g_free(parser_feature_set) ;

  return ;
}





/*
 * Get the feature set associated with a particular name from the parser; if it
 * does not exist, it should be created.
 */
static ZMapGFFParserFeatureSet getParserFeatureSet(ZMapGFFParser pParserBase, char* sFeatureSetName )
{
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  ZMapGFFParserFeatureSet pParserFeatureSet = NULL ;
  ZMapFeatureSet pFeatureSet = NULL ;
  ZMapSpan span;

  pParserFeatureSet = (ZMapGFFParserFeatureSet)g_datalist_get_data(&(pParser->feature_sets), sFeatureSetName);

  /*
   * If we don't have this feature_set yet, then make one.
   */
  if (!pParserFeatureSet)
    {

      pParserFeatureSet = g_new0(ZMapGFFParserFeatureSetStruct, 1) ;

      g_datalist_set_data_full(&(pParser->feature_sets), sFeatureSetName, pParserFeatureSet, destroyFeatureArray) ;

      pFeatureSet = pParserFeatureSet->feature_set = zMapFeatureSetCreate(sFeatureSetName , NULL) ;

      /* record the region we are getting */
      span = (ZMapSpan) g_new0(ZMapSpanStruct,1);
      span->x1 = pParser->features_start;
      span->x2 = pParser->features_end;
      pFeatureSet->loaded = g_list_append(NULL,span);

      pParser->src_feature_sets =	g_list_prepend(pParser->src_feature_sets, GUINT_TO_POINTER(pFeatureSet->unique_id));

      /*
       * we need to copy as these may be re-used. styles have already been
       * inherited by this point by zmapView code and passed back to us
       */
      pParserFeatureSet->feature_styles = g_hash_table_new(NULL,NULL);

      pParserFeatureSet->multiline_features = NULL ;
      g_datalist_init(&(pParserFeatureSet->multiline_features)) ;

      pParserFeatureSet->parser = pParserBase ;
    }

  return pParserFeatureSet ;
}








/*
 **********************************************************************************
 * (sm23) NOTE this is lifted straight from v2 code, and can probably be discarded
 * at some point, preferably sooner rather than later.
 **********************************************************************************
 *
 *
 *
 *
 *
 * This routine attempts to find the feature's name from the GFF attributes field,
 * acedb has been modified so that with -zmap option it will output the name as:
 *
 *          Name "object name"    e.g.   Name "Sequence B0250.1"
 *
 * Some features do not have an obvious name though or the feature name is shared
 * amongst many features and so we must construct a name from the feature name
 * and the coords.
 *
 * For homology features the name is in the attributes in the form:
 *
 *        Target "Classname:objname" query_start query_end
 *
 * For assembly path features the name is in the attributes in the form:
 *
 *        Assembly_source "Classname:objname"
 *
 * For genefinder features they all have a source field that starts "GF_" so we use that.
 *
 *        B0250	GF_splice	splice3	106	107	0.233743	+	.
 *        B0250	GF_ATG	atg	38985	38987	1.8345	-	0
 *        etc.
 *
 * Some features have their name given in the "Note" field appended to some GFF records.
 * This field is also used for general comments however so the code must attempt to
 * deal with this, here are some examples:
 *
 * Here's one that is a feature name:
 *
 *        Note "RP5-931H19"
 *
 * and here's some that aren't:
 *
 *        Note "Left: B0250"
 *        Note "7 copies of 31mer"
 *
 * and so on....
 *
 *
 */
static gboolean getFeatureName(const char * const sequence,
  const ZMapGFFAttribute * const pAttributes,
  unsigned int nAttributes,
  const char * const given_name,
  const char * const source,
  ZMapStyleMode feature_type,
  ZMapStrand strand,
  int start,
  int end,
  int query_start,
  int query_end,
  char ** const feature_name,
  char ** const feature_name_id)
{
  NameFindType name_find = ZMAPGFF_NAME_FIND ;
  gboolean bHasName = FALSE, bAttributeHit = FALSE ;
  char *sValue = NULL ;
  char *sTempBuff01 = NULL,
  *sTempBuff02 = NULL ;
  unsigned int iAttribute = 0 ;

  ZMapGFFAttribute pAttribute = NULL ;

  if (name_find == ZMAPGFF_NAME_USE_SEQUENCE)
    {
      bHasName = TRUE ;
      *feature_name = g_strdup(sequence) ;
      *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
                                               start, end, query_start, query_end) ;
    }
  else if (name_find == ZMAPGFF_NAME_USE_SOURCE)
    {
      bHasName = TRUE ;
      *feature_name = g_strdup(source) ;
      *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
                                               start, end, query_start, query_end) ;
    }
  else if ((name_find == ZMAPGFF_NAME_USE_GIVEN || name_find == ZMAPGFF_NAME_USE_GIVEN_OR_NAME) && given_name && *given_name)
    {
      bHasName = TRUE ;
      *feature_name = g_strdup(given_name) ;
      *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
                                               start, end, query_start, query_end) ;
    }
  else if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Name")))
    {

      sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
      if (sValue && sValue[0])
        {
          *feature_name = g_strdup(zMapGFFAttributeGetTempstring(pAttribute)) ;
          *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
                                                   start, end, query_start, query_end) ;
          bHasName = TRUE ;
        }

    }
  else if (name_find != ZMAPGFF_NAME_USE_GIVEN_OR_NAME)
    {


      /* for BAM we have a basic feature with name so let's bodge this in */
      if (feature_type == ZMAPSTYLE_MODE_ALIGNMENT)
        {

          if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Target")))
            {

                /* In acedb output at least, homologies all have the same format.
               * Strictly speaking this is V2 only, as target has a different meaning
               * in V3.
               */
              if (zMapAttParseTargetV2(pAttribute, &sTempBuff01, &sTempBuff02))
                {
                  bHasName = FALSE ;
                  *feature_name = g_strdup(sTempBuff01) ;
                  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
                                                           start, end, query_start, query_end) ;
                }

            }
        }
      else if (feature_type == ZMAPSTYLE_MODE_ASSEMBLY_PATH)
        {

          /*
           * (sm23) This block does not appear to be entered with any of my datasets.
           */

          /* if ((tag_pos = strstr(attributes, "Assembly_source"))) */
          pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Assembly_source") ;
          if (pAttribute)
            {
              /* if (sscanf(tag_pos, attr_format_str02, name) == iExpectedFields1) */
              if (zMapAttParseAssemblySource(pAttribute, &sTempBuff01, &sTempBuff02))
                {
                  bHasName = FALSE ;
                  *feature_name = g_strdup(sTempBuff01) ;
                  *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
                                                           start, end, query_start, query_end) ;
                }
            }

        }
      else if ((feature_type == ZMAPSTYLE_MODE_BASIC || feature_type == ZMAPSTYLE_MODE_GLYPH)
                 && (g_str_has_prefix(source, "GF_") || (g_ascii_strcasecmp(source, "hexexon") == 0)))
        {

          /* Genefinder features, we use the source field as the name.... */
          bHasName = FALSE ;
          *feature_name = g_strdup(given_name) ;
          *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
                                                   start, end, query_start, query_end) ;

        }
      else
        {

          bHasName = FALSE ;
          /*
           * (sm23) Again, why are we only looking for the "Note" attribute at the beginning
           * of the attribute string?
           */
          for (iAttribute=0; iAttribute<nAttributes; ++iAttribute)
            {
              pAttribute = pAttributes[iAttribute] ;
              if (zMapAttParseAnyTwoStrings(pAttribute, &sTempBuff01, &sTempBuff02) )
              {
                bAttributeHit = TRUE;
                break ;
              }
            }

          /* if (sscanf(attributes, attr_format_str03, sClass, name) == iExpectedFields2) */
          if (bAttributeHit)
            {
              bHasName = TRUE ;
              *feature_name = g_strdup(sTempBuff02) ;
              *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
                                                       start, end, query_start, query_end) ;
            }
          else
            {
              *feature_name = g_strdup(sequence) ;
              *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
                                                       start, end, query_start, query_end) ;
            }

        }

    } /* last clause */

  /* mh17: catch all to create names for totally anonymous features
   * me, i'd review all the stuff above and simplify it...
   * use case in particular is bigwig OTF request from File menu, we get a basic feature with no
   * name (if we don'tl specify a style)
   * normally they'd be graph features which somehow ends up with a made up name
   * also fixes RT 238732
   */
  if (!*feature_name_id)
  {
    *feature_name = g_strdup(sequence) ;
    *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
                                             start, end, query_start, query_end) ;
  }

  /*
   * Clean up
   */
  if (sTempBuff01)
    g_free(sTempBuff01) ;
  if (sTempBuff02)
    g_free(sTempBuff02) ;

  return bHasName ;
}
