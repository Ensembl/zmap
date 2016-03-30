/*  File: zmapGFF3parser.c
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


#include <ZMap/zmapGFF.hpp>
#include <ZMap/zmapSO.hpp>
#include <ZMap/zmapSOParser.hpp>
#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapFeatureLoadDisplay.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapGFFStringUtils.hpp>

#include <zmapGFFHeader.hpp>
#include <zmapGFF_P.hpp>
#include <zmapGFF3_P.hpp>
#include <zmapGFFDirective.hpp>
#include <zmapGFFAttribute.hpp>
#include <zmapGFFFeatureData.hpp>

/*
 * Functions for internal usage only.
 */
/* static gboolean removeCommentFromLine( char* sLine ) ; */
static ZMapGFFLineType parserLineType(const char * const sLine) ;
static ZMapGFFParserState parserFSM(ZMapGFFParserState eCurrentState, const char * const sNewLine) ;
static gboolean resizeFormatStrs(ZMapGFFParser pParser) ;
static gboolean resizeBuffers(ZMapGFFParser pParser, gsize iLineLength) ;
static gboolean isCommentLine(const char * const sLine) ;
static gboolean isAcedbError(const char* const ) ;
static gboolean parserStateChange(ZMapGFFParser pParser, ZMapGFFParserState eOldState, ZMapGFFParserState eNewState, const char * const sLine) ;
static gboolean initializeSequenceRead(ZMapGFFParser pParser, const char * const sLine) ;
static gboolean finalizeSequenceRead(ZMapGFFParser pParser , const char* const sLine) ;
static gboolean initializeFastaRead(ZMapGFFParser pParser, const char * const sLine) ;
static gboolean actionUponClosure(ZMapGFFParser pParser, const char* const sLine)  ;
/*static gboolean iterationFunctionID(GQuark gqID, GHashTable *pValueTable) ; */
static gboolean initParserForSequenceRead(ZMapGFFParser pParser) ;
static gboolean copySequenceData(ZMapSequence pSequence, GString *pData) ;

static gboolean parseHeaderLine_V3(ZMapGFFParser pParserBase, const char * const sLine) ;
static gboolean parseFastaLine_V3(ZMapGFFParser pParser, const char* const sLine) ;
static gboolean parseBodyLine_V3(ZMapGFFParser pParser, const char * const sLine) ;
static gboolean parseSequenceLine_V3(ZMapGFFParser pParser, const char * const sLine) ;

static gboolean addNewSequenceRecord(ZMapGFFParser pParser);
static gboolean appendToSequenceRecord(ZMapGFFParser pParser, const char * const sLine) ;
static gboolean parseDirective_GFF_VERSION(ZMapGFFParser pParser, const char * const line);
static gboolean parseDirective_SEQUENCE_REGION(ZMapGFFParser pParser, const char * const line);
static gboolean parseDirective_FEATURE_ONTOLOGY(ZMapGFFParser pParser, const char * const line);
static gboolean parseDirective_ATTRIBUTE_ONTOLOGY(ZMapGFFParser pParser, const char * const line);
static gboolean parseDirective_SOURCE_ONTOLOGY(ZMapGFFParser pParser, const char * const line);
static gboolean parseDirective_SPECIES(ZMapGFFParser pParser, const char * const line);
static gboolean parseDirective_GENOME_BUILD(ZMapGFFParser pParser, const char * const line);

static gboolean makeNewFeature_V3(ZMapGFFParser pParser, char ** const err_text, ZMapGFFFeatureData pFeatureData ) ;

static ZMapGFFParserFeatureSet getParserFeatureSet(ZMapGFFParser pParser, char* sFeatureSetName ) ;
static gboolean getFeatureName(const char * const sequence, ZMapGFFAttribute *pAttributes, unsigned int nAttributes,const char * const given_name, const char * const source, ZMapStyleMode feature_type,
  ZMapStrand strand, int start, int end, int query_start, int query_end, char ** const feature_name, char ** const feature_name_id) ;
static void destroyFeatureArray(gpointer data) ;

/*
 * Functions to create, augment or find names for various types of features based upon ZMapStyleMode.
 */
static ZMapFeature makeFeatureTranscript(ZMapGFF3Parser const, ZMapGFFFeatureData , ZMapFeatureSet , gboolean *, char **) ;
static gboolean makeFeatureLocus(ZMapGFFParser , ZMapGFFFeatureData , char ** ) ;
static ZMapFeature makeFeatureAlignment(ZMapGFFFeatureData , ZMapFeatureSet , char ** ) ;
static ZMapFeature makeFeatureAssemblyPath(ZMapGFFFeatureData , ZMapFeatureSet , char ** ) ;
static ZMapFeature makeFeatureDefault(ZMapGFFFeatureData, ZMapFeatureSet , char **) ;
static char * makeFeatureTranscriptNamePublic(ZMapGFFFeatureData ) ;
static gboolean clipFeatureLogic_General(ZMapGFF3Parser, ZMapGFFFeatureData) ;
static gboolean clipFeatureLogic_Complete(ZMapGFF3Parser, ZMapGFFFeatureData) ;
static gboolean requireLocusOperations(ZMapGFFParser , ZMapGFFFeatureData  ) ;
static gboolean findFeatureset(ZMapGFFParser , ZMapGFFFeatureData  , ZMapFeatureSet *) ;

/*
 * These functions are to deal with the composite_features hash table.
 */
static GQuark compositeFeaturesCreateID(ZMapGFFAttribute *pAttributes, unsigned int nAttributes ) ;
static GQuark compositeFeaturesFind(ZMapGFF3Parser const pParser, GQuark feature_id ) ;
static gboolean compositeFeaturesInsert(ZMapGFF3Parser const pParser, GQuark feature_id, GQuark feature_unique_id );


/*
 * See comments with function.
 */
static gboolean hack_SpecialColumnToSOTerm(const char * const, char * const ) ;

/*
 * These alternatives are in place temporarily until we have
 * settled down with the new behaviour (i.e. it has gone through
 * user testing).
 */
/* #define OLD_CLIPPING_BEHAVIOUR 1 */

#ifdef OLD_CLIPPING_BEHAVIOUR
/*
 * old clipping behaviour; clip alignments using the
 * General function
 */
#define CLIP_TRANSCRIPT_ON_PARSE 1
#define CLIP_ALIGNMENT_ON_PARSE 1
#define CLIP_ASSEMBLYPATH_ON_PARSE 1
#define CLIP_DEFAULT_ON_PARSE 1
#define CLIP_LOCUS_ON_PARSE 1

#else
/*
 * new clipping behaviour
 */
#define CLIP_ALIGNMENT_ON_PARSE 1
#define CLIP_LOCUS_ON_PARSE 1

#endif

/* #define DUMP_GFF_TO_FILE 1 */

#ifdef DUMP_GFF_TO_FILE
static const char * sFilename = "/nfs/users/nfs_s/sm23/Work/dumpfile.txt" ;
static FILE *pFile = NULL ;
#endif

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
/* CLO */{ZMAPGFF_PARSER_NON, ZMAPGFF_PARSER_SEQ, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_DIR, ZMAPGFF_PARSER_CLO, ZMAPGFF_PARSER_BOD, ZMAPGFF_PARSER_FAS, ZMAPGFF_PARSER_CLO, ZMAPGFF_PARSER_NON},
/* ERR */{ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR, ZMAPGFF_PARSER_ERR}
} ;


/*
 * Function to create a v3 parser object.
 */
ZMapGFFParser zMapGFFCreateParser_V3(char *sequence, int features_start, int features_end)
{
  ZMapGFF3Parser pParser = NULL ;

  
  /* If coords are given, check that they're sensible */
  if ((!features_start && !features_end) || ((features_start > 0) && (features_end >= features_start)))
    {
      pParser                                   = g_new0(ZMapGFF3ParserStruct, 1) ;
      if (pParser)
        {

          pParser->gff_version                      = ZMAPGFF_VERSION_3 ;
          pParser->pHeader                          = zMapGFFCreateHeader() ;
          if (!pParser->pHeader)
            return NULL ;
          pParser->pHeader->flags.got_ver           = TRUE ;
          pParser->state                            = ZMAPGFF_PARSER_NON ;
          pParser->line_count                       = 0 ;
          pParser->line_count_bod                   = 0 ;
          pParser->line_count_dir                   = 0 ;
          pParser->line_count_seq                   = 0 ;
          pParser->line_count_fas                   = 0 ;
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

          /* pParser->pMLF                             = zMapMLFCreate() ; */
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
          pParser->cSOSetInUse                      = ZMAPSO_USE_SOXP ;

          /*
           *
           */
          pParser->composite_features               = g_hash_table_new(NULL, NULL) ;

        }
    }

  return (ZMapGFFParser) pParser ;
}



/*
 * Function to destroy a parser.
 */
void zMapGFFDestroyParser_V3(ZMapGFFParser pParserBase)
{
  int iValue ;
  zMapReturnIfFail(pParserBase) ;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  if (pParser->error)
    g_error_free(pParser->error) ;
  if (pParser->raw_line_data)
    g_string_free(pParser->raw_line_data, TRUE) ;
  /* if (pParser->pMLF)
    zMapMLFDestroy(pParser->pMLF) ; */
  if (pParser->excluded_features)
    g_hash_table_destroy(pParser->excluded_features) ;

  if (pParser->pHeader)
    zMapGFFHeaderDestroy(pParser->pHeader) ;

  if (pParser->sequence_name)
    g_free(pParser->sequence_name) ;

  if (pParser->nSequenceRecords)
    {
      if (pParser->pSeqData)
        {
          for (iValue=0; iValue<pParser->nSequenceRecords; ++iValue)
            {
              zMapGFFSequenceDestroy(&pParser->pSeqData[iValue]) ;
            }
        }
      g_free(pParser->pSeqData) ;
    }

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

  /*
   * Destroy composite_features hash table.
   */
  if (pParser->composite_features)
    g_hash_table_destroy(pParser->composite_features) ;

  g_free(pParser) ;

  return ;
}




/*
 * Return the got_minimal_header flag. This style of header flags is only
 * used for version 3.
 */
gboolean zMapGFFGetHeaderGotMinimal_V3(ZMapGFFParser pParserBase)
{
  gboolean bResult = FALSE ;
  zMapReturnValIfFail(pParserBase, FALSE );
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  /*
   *
   */
  if ( pParser->pHeader->flags.got_ver )
    {
      if (pParser->pHeader->flags.got_sqr )
        {
          bResult = TRUE ;
        }
      else if ( pParser->features_start && pParser->features_end )
        {
          bResult = TRUE ;
        }
    }

  return bResult ; // (gboolean) pParser->pHeader->flags.got_min ;
}


/*
 * Return the got_sqr flag. This style of header flags is only
 * used for version 3.
 */
gboolean zMapGFFGetHeaderGotSequenceRegion_V3(ZMapGFFParser pParserBase)
{
  gboolean bResult = FALSE ;
  zMapReturnValIfFail(pParserBase, FALSE );
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  /*
   *
   */
  if ( pParser->pHeader->flags.got_sqr )
    {
      bResult = TRUE ;
    }

  return bResult ; // (gboolean) pParser->pHeader->flags.got_min ;
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
/*
static gboolean removeCommentFromLine( char* sLine )
{
  static const char cQuote = '"',
    cHash = '#',
    cNull = '\0' ;
  gboolean bIsQuoted = FALSE,
    bResult = FALSE ;
  zMapReturnValIfFail(sLine && *sLine, bResult) ;


  if (g_str_has_prefix(sLine, "##"))
    return FALSE ;

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
*/










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
static gboolean resizeBuffers(ZMapGFFParser pParser, gsize iLineLength)
{
  gboolean bNewAlloc = TRUE,
    bResized = FALSE ;
  char *sBuf[ZMAPGFF_NUMBER_PARSEBUF];
  unsigned int i, iNewLineLength ;

  zMapReturnValIfFail(pParser && iLineLength, bResized) ;

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
          sBuf[i] = (char*) g_malloc0(iNewLineLength) ;
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
static gboolean resizeFormatStrs(ZMapGFFParser pParser)
{
  gboolean bResized = TRUE ;
  GString *format_str ;
  static const char *align_format_str = "%%*[\"]" "%%%d" "[^\"]%%*[\"]%%*s" ;
  gsize iLength ;

  zMapReturnValIfFail(pParser, bResized) ;

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
                    G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s%%%" G_GSSIZE_FORMAT "s %%%" G_GSSIZE_FORMAT "[^\n]s",
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
static gboolean initializeSequenceRead(ZMapGFFParser pParserBase, const char * const sLine)
{
  gboolean bResult = TRUE ;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, FALSE) ;

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
static gboolean initializeFastaRead(ZMapGFFParser pParserBase, const char * const sLine)
{
  gboolean bResult = TRUE ;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, FALSE) ;

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

  zMapReturnValIfFail(pParser && pParser->pHeader, FALSE ) ;

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
/*
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
    }
  bResult = TRUE ;

  return bResult ;
}
*/





/*
 * This is the semantic action to be performed upon encountering the
 * "###" directive. Right now this is just a dummy function.
 */
static gboolean actionUponClosure(ZMapGFFParser pParserBase, const char* const sLine)
{
  gboolean bResult = TRUE ;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader && pParser->composite_features, FALSE) ;

  g_hash_table_remove_all(pParser->composite_features) ;

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

  zMapReturnValIfFail(pParser && pParser->pHeader, FALSE) ;

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
static gboolean finalizeSequenceRead(ZMapGFFParser pParserBase , const char* const sLine)
{
  gboolean bResult = TRUE,
    bCopyData = TRUE;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, FALSE ) ;

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
  gboolean bResult = FALSE ;
  zMapReturnValIfFail(pSequence && pData, bResult ) ;
  bResult = TRUE ;

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

  zMapReturnValIfFail(pParser && pParser->pHeader, bResult) ;

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

  zMapReturnValIfFail(pParser && pParser->pHeader, bResult) ;

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
static gboolean parseFastaLine_V3(ZMapGFFParser pParserBase, const char* const sLine)
{
  unsigned int iFields ;
  gboolean bResult = FALSE ;
  ZMapSequence pTheSequence = NULL ;
  unsigned int nSequenceRecords = 0 ;
  static const char *sFmt = "%1000s%1000s" ;
  char sBuf01[1001], sBuf02[1001] ;

  /*
   * Increment counter for this line type.
   */
  zMapGFFIncrementLineFas(pParserBase) ;

  /*
   * Cast to concrete type.
   */
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  /*
   * Some basic error checking.
   */
  zMapReturnValIfFail(pParser && pParser->pHeader, bResult) ;

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
          bResult = TRUE ;
        }
      else
        {
          pTheSequence->name = g_quark_from_string("ZMAPGFF_FASTA_NAME_NOT_FOUND") ;
        }

      bResult = TRUE ;
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
      else
        {
          bResult = TRUE ;
        }

      bResult = TRUE ;
    }

  return bResult ;
}





/*
 * Read a sequence line in old (v2) directive form, that is, something like
 *   ##ACTGACTGACTGACTGACTGAC...
 * No checking is done for validity of sequence.
 */
static gboolean parseSequenceLine_V3(ZMapGFFParser pParserBase, const char * const sLine)
{
  gboolean bResult = FALSE ;

  /*
   * Increment counter for this line type.
   */
  zMapGFFIncrementLineSeq(pParserBase) ;

  /*
   * Cast to concrete type.
   */
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, bResult) ;

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
static gboolean parseDirective_GFF_VERSION(ZMapGFFParser pParserBase, const char * const line)
{
  static const char *sFmt= "%*13s%d";
  static const unsigned int iExpectedFields = 1 ;
  gboolean bResult = TRUE ;
  unsigned int iFields,
    iVersion = (unsigned int) ZMAPGFF_VERSION_UNKNOWN ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, FALSE) ;

  /*
   * (sm23) I have removed this test to make sure that it's OK for
   * the parser to have had data for version set more than once. This is
   * a bit of a hack associated with the problem related to rewinding
   * the GIO channel running under otterlace.
   *
   * Set an error if this directive has been seen already.
   */
  /*if (pParser->pHeader->flags.got_ver)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Duplicate ##gff-version pragma, line %d: \"%s\"",
                    zMapGFFGetLineNumber(pParserBase), line) ;
      bResult = FALSE ;
      return bResult ;
    }*/

  /*
   * Now attempt to read the line for desired fields. If we don't get the appropriate
   * number an error should be set.
   */
  if ((iFields = sscanf(line, sFmt, &iVersion)) != iExpectedFields)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Bad ##gff-version line %d: \"%s\"",
                    zMapGFFGetLineNumber(pParserBase), line) ;
      bResult = FALSE ;
      return bResult ;
    }

  /*
   * Now do some checking and write to the parser if all is well.
   */
  if (iVersion != 3)
    {
       pParser->error =
         g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                     "Only GFF3 versions 2 or 3 supported, line %d: \"%s\"",
                     zMapGFFGetLineNumber(pParserBase), line) ;
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
    }

  return bResult ;
}



/*
 * Sequence region is a string followed by two non-negative integers.
 */
static gboolean parseDirective_SEQUENCE_REGION(ZMapGFFParser pParserBase, const char * const line)
{
  static const unsigned int iExpectedFields = 3 ;
  static const char *sFmt = "%*s%1000s%d%d" ;
  gboolean bResult = TRUE ;
  char sBuf[1001] = "" ;
  unsigned int  iFields ;
  int iStart = 0, iEnd = 0 ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, FALSE ) ;

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
                    zMapGFFGetLineNumber(pParserBase), line) ;
      bResult = FALSE ;
      return bResult ;
    }

  /* If the parser sequence details are not set, then set them from the sequence-region */
  if (!zMapGFFGetSequenceName((ZMapGFFParser)pParser))
    {
      pParser->sequence_name = g_strdup(sBuf) ;
      pParser->features_start = iStart ;
      pParser->features_end = iEnd ;
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
                        pParser->sequence_name, iStart, iEnd, zMapGFFGetLineNumber(pParserBase), line) ;
          bResult = FALSE ;
        }
      else if (iStart > pParser->features_end || iEnd < pParser->features_start)
        {
          pParser->error =
            g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                        "No overlap between original sequence/start/end:" " \"%s\" %d %d" " and header \"##sequence-region\" line %d: \"%s\"",
                        pParser->sequence_name, pParser->features_start, pParser->features_end, zMapGFFGetLineNumber(pParserBase), line) ;
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
        }

    } /* sequence name matches */


  return bResult ;
}


/*
 * The feature ontology should be a URL/URI string. This is not checked for validity in
 * any way.
 */
static gboolean parseDirective_FEATURE_ONTOLOGY(ZMapGFFParser pParserBase , const char * const line)
{
  static const char* sFmt = "%*s%1000s" ;
  static const int iExpectedFields = 1 ;
  gboolean bResult = TRUE ;
  char sBuf[1001] = "" ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, FALSE) ;

  /*
   * If we have read this directive already, then set an error.
   */
  if (pParser->pHeader->flags.got_feo)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Duplicate ##feature-ontology directive, line %d: \"%s\"",
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
                    "Extracted data is empty parseDirective_FEATURE_ONTOLOGY(); line = %i, line = '%s'",
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
static gboolean parseDirective_ATTRIBUTE_ONTOLOGY(ZMapGFFParser pParserBase, const char * const line)
{
  static const char* sFmt = "%*s%1000s" ;
  static const int iExpectedFields = 1 ;
  gboolean bResult = TRUE ;
  char sBuf[1001] = "" ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, bResult ) ;

  /*
   * If we have read this directive already, then set an error.
   */
  if (pParser->pHeader->flags.got_ato)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Duplicate ##attribute-ontology directive, line %d: \"%s\"",
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
                    "Extracted data is empty in parseDirective_ATTRIBUTE_ONTOLOGY(); line = %i, line = '%s'",
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
static gboolean parseDirective_SOURCE_ONTOLOGY(ZMapGFFParser pParserBase, const char * const line)
{
  static const char* sFmt = "%*s%1000s" ;
  static const int iExpectedFields = 1 ;
  gboolean bResult = TRUE ;
  char sBuf[1001] = "" ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, bResult ) ;

  /*
   * If we have read this directive already, then set an error.
   */
  if (pParser->pHeader->flags.got_soo)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Duplicate ##source-ontology directive, line %d: \"%s\"",
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
                    "Extracted data is empty in parseDirective_SOURCE_ONTOLOGY(); line = %i, line = '%s'",
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
static gboolean parseDirective_SPECIES(ZMapGFFParser pParserBase, const char * const line)
{
  static const char* sFmt = "%*s%1000s" ;
  static const int iExpectedFields = 1 ;
  gboolean bResult = TRUE ;
  char sBuf[1001] = "" ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, bResult) ;

  /*
   * If we have read this directive already, then set an error.
   */
  if (pParser->pHeader->flags.got_spe )
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Duplicate ##species directive, line %d: \"%s\"",
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
                    "Extracted data is empty in parseDirective_SPECIES(); line = %i, line = '%s'",
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
static gboolean parseDirective_GENOME_BUILD(ZMapGFFParser pParserBase, const char * const line)
{
  static const char* sFmt = "%*s%1000s%1000s" ;
  static const int iExpectedFields = 2 ;
  gboolean bResult = TRUE ;
  char sBuf01[1001] = "" , sBuf02[1001] = "" ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, bResult) ;

  /*
   * If we have read this directive alre/ady, then set an error.
   */
  if (pParser->pHeader->flags.got_gen)
    {
      pParser->error =
        g_error_new(pParser->error_domain, ZMAPGFF_ERROR_HEADER,
                    "Duplicate ##genome-build directive, line %d: \"%s\"",
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
                    "Extracted data is empty in parseDirective_GENOME_BUILD(); line = %i, line = '%s'",
                    zMapGFFGetLineNumber(pParserBase), line) ;
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
static gboolean parseHeaderLine_V3(ZMapGFFParser pParserBase, const char * const sLine)
{
  gboolean bResult = FALSE ;
  ZMapGFFDirectiveName eDirName ;

  /*
   * Increment counter for this type of line
   */
  zMapGFFIncrementLineDir(pParserBase) ;

  /*
   * Cast to concrete type.
   */
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, bResult ) ;

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

  /*
   * To be tested for after read of every directive line.
   */
  zMapGFFHeaderMinimalTest(pParser->pHeader) ;

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
gboolean zMapGFFParse_V3(ZMapGFFParser pParserBase, char * const sLine)
{
  gboolean bResult = TRUE ;
  ZMapGFFParserState eOldState = ZMAPGFF_PARSER_NON,
    eNewState = ZMAPGFF_PARSER_NON;
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, FALSE) ;

  /*
   * Increment line count and skip over blank lines and comments.
   */
  zMapGFFIncrementLineNumber(pParserBase) ;
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
  /* gboolean bCommentRemoved = removeCommentFromLine( sLine ) ; */

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
gboolean zMapGFFGetLogWarnings(ZMapGFFParser pParserBase )
{
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, FALSE ) ;

  return pParser->bLogWarnings ;
}







/*
 * Parse out a body line for data and then call functions to create
 * appropriate new features.
 */
static gboolean parseBodyLine_V3(ZMapGFFParser pParserBase, const char * const sLine)
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
    *sErrText                         = NULL,
    **sTokens                         = NULL ;

  const char *sSOIDName               = NULL ;

  double
    dScore                            = 0.0
  ;

  gboolean
    bResult                           = TRUE,
    bHasScore                         = FALSE,
    bIncludeEmpty                     = FALSE,
    bRemoveQuotes                     = FALSE,
    bIsValidSOID                      = FALSE
  ;

  ZMapStyleMode
    cType                             = ZMAPSTYLE_MODE_BASIC
  ;
  ZMapHomolType
    cHomol                            = ZMAPHOMOL_NONE
  ;
  ZMapStrand
    cStrand                           = ZMAPSTRAND_NONE
  ;
  ZMapPhase
    cPhase                            = ZMAPPHASE_NONE
  ;
  ZMapGFFAttribute
    *pAttributes                      = NULL
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

#ifdef DUMP_GFF_TO_FILE
  if (pFile == NULL)
    {
      pFile = fopen(sFilename, "w") ;
      if (pFile == NULL)
        {
          fprintf(stderr, "could not open gff dump file.\n") ;
        }
    }
  else
    {
      fprintf(pFile, "%s\n", sLine) ;
      fflush(pFile) ;
    }
#endif

  /*
   * Increment counter for this type of line.
   */
  zMapGFFIncrementLineBod(pParserBase) ;


  /*
   * Cast to concrete type for GFFV3.
   */
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  /*
   * Initial error check.
   */
  zMapReturnValIfFail(pParser && pParser->pHeader && sLine && *sLine, FALSE) ;

  /*
   * We must have a sequence name
   */
  if (!pParser->sequence_name)
    {
      if (pParser->error)
        {
          g_error_free(pParser->error) ;
          pParser->error = NULL ;
        }
      pParser->error = g_error_new(pParser->error_domain, ZMAPGFF_ERROR_BODY,
                                   "No sequence has been specified to load features for") ;

      bResult = FALSE ;
      goto return_point;
    }

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
                                   "Line length too long, line %d has length %d",
                                   zMapGFFGetLineNumber(pParserBase), iLineLength) ;
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
  sTokens = zMapGFFStringUtilsTokenizer(pParser->cDelimBodyLine, sLine, &iFields, bIncludeEmpty,
                                 iTokenLimit, g_malloc, g_free, pParser->buffers[ZMAPGFF_BUF_TMP]) ;

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
                                   "GFF line %d - Mandatory fields missing in: \"%s\"",
                                   zMapGFFGetLineNumber(pParserBase), sLine) ;
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
                                   "GFF line %d - too many \tab delimited fields in \"%s\"",
                                   zMapGFFGetLineNumber(pParserBase), sLine);
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
   * A CDS feature _must_ have a phase which is _not_ ZMAPPHASE_NONE.
   *
   * All features other than CDS _must_ have cPhase = '.' (i.e. ZMAPPHASE_NONE).
   * Anything else is an error.
   */
  if (!strcmp(sType, "CDS"))
    {
      if (cPhase == ZMAPPHASE_NONE)
        {
          bResult = FALSE ;
          sErrText = g_strdup_printf("CDS feature must not have ZMAPPHASE_NONE; line %i, '%s'",
                                     zMapGFFGetLineNumber(pParserBase), sLine) ;
          pParser->error = g_error_new(pParser->error_domain, ZMAPGFF_ERROR_BODY, "%s", sErrText) ;
          goto return_point ;
        }
    }
  else
    {
      if (cPhase != ZMAPPHASE_NONE)
        {
          bResult = FALSE ;
          sErrText = g_strdup_printf("non-CDS feature must have ZMAPPHASE_NONE; line %i, '%s'",
                                     zMapGFFGetLineNumber(pParserBase), sLine) ;
          pParser->error = g_error_new(pParser->error_domain, ZMAPGFF_ERROR_BODY, "%s", sErrText) ;
          goto return_point ;
        }
    }


  /*
   * Deal with hack to source -> SO term mapping
   */
  hack_SpecialColumnToSOTerm(sSource, sType ) ;

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
   * Now this is a hack to deal with cases where the source was not specified.
   * We need a different source name for every different possible style mode
   * at least. In this case I choose to create a name for every different SO term,
   * which will accomodate that possibility (and more).
   */
  if (!strcmp(sSource, "."))
    {
      char *sTemp = NULL ;
      sTemp = g_strdup_printf("anon_source (%s type)", zMapStyleMode2ShortText(zMapSOIDDataGetStyleMode(pSOIDData))) ;
      strcpy(sSource, sTemp) ;
      if (sTemp)
        g_free(sTemp) ;
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
            zMapLogWarning("SO Term at line %i is not valid; 'type' string is = '%s'",
                           zMapGFFGetLineNumber(pParserBase), sType) ;
        }
      else if (cSOErrorLevel == ZMAPGFF_SOERRORLEVEL_ERR)
        {
          sErrText = g_strdup_printf("SO Term at line %i is not valid; 'type' string is = '%s'", zMapGFFGetLineNumber(pParserBase), sType) ;
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
                                   "GFF line %d (a)- %s (\"%s\")",
                                   zMapGFFGetLineNumber(pParserBase), sErrText, sLine) ;
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
                                   "GFF body line; invalid ZMapGFFFEatureData object constructed; %i, '%s'",
                                   zMapGFFGetLineNumber(pParserBase), sLine) ;
      bResult = FALSE ;
      goto return_point ;
    }


  /*
   * Attempt to create the new feature object and add to the
   * appropriate feature set.
   */
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
                                   "GFF line %d. ERROR: %s (\"%s\")",
                                   zMapGFFGetLineNumber(pParserBase), sErrText, sLine) ;
      g_free(sErrText) ;
      sErrText = NULL ;

    }


  /*
   * Exit point for the function.
   */
return_point:

  /*
   * Clean up dynamically allocated data.
   */
  zMapGFFStringUtilsArrayDelete(sTokens, iFields, g_free) ;
  zMapGFFAttributeDestroyList(pAttributes, nAttributes) ;
  zMapGFFFeatureDataDestroy(pFeatureData) ;

  return bResult ;
}



/*
 * Create a name for a transcript feature only. This is the name to display to the user,
 * and is not used to create a feature->unique_id for the feature_set.
 *
 * The name is of the form "<name>", where
 *
 * (1) "Name = <name>", or
 * (2) <name> = "no_name_given" if there is no Name attribute
 *
 * This should only be called for the the top level object, rather than
 * components such as intron, exon, CDS, etc.
 *
 */
static char * makeFeatureTranscriptNamePublic( ZMapGFFFeatureData pFeatureData)
{
  ZMapGFFAttribute *pAttributes = NULL,
    pAttributeName = NULL ;
  unsigned int nAttributes = 0 ;
  char *sResult = NULL,
    *sFeatureAttributeName = NULL ;
  gboolean bParseValid = FALSE ;
  static const char *sNoName = "no_name_given" ;
  zMapReturnValIfFail(pFeatureData, sResult );
  pAttributes = zMapGFFFeatureDataGetAts(pFeatureData) ;
  nAttributes = zMapGFFFeatureDataGetNat(pFeatureData) ;
  pAttributeName = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_Name) ;
  if (pAttributeName)
    {
      bParseValid = zMapAttParseName(pAttributeName, &sFeatureAttributeName) ;
    }
  sResult = pAttributeName ? g_strdup(sFeatureAttributeName) : g_strdup(sNoName) ;
  if (sFeatureAttributeName)
    {
      g_free(sFeatureAttributeName) ;
    }
  return sResult ;
}

/*
 * This creates the name to be used for the feature->unique_id, and is simply
 * a wrapper for a call to zMapFeatureCreateName().
 */
static char * makeFeatureTranscriptNamePrivate(ZMapGFFFeatureData pFeatureData)
{
  ZMapGFFAttribute *pAttributes = NULL,
    pAttributeName = NULL ;
  int iStart, iEnd ;
  unsigned int nAttributes = 0 ;
  char *sResult = NULL,
    *sFeatureName = NULL ;
  ZMapStyleMode cStyleMode = ZMAPSTYLE_MODE_TRANSCRIPT ;
  ZMapStrand cStrand ;

  zMapReturnValIfFail(pFeatureData, sResult);

  pAttributes = zMapGFFFeatureDataGetAts(pFeatureData) ;
  nAttributes = zMapGFFFeatureDataGetNat(pFeatureData) ;
  pAttributeName = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_Name) ;
  sFeatureName = zMapGFFAttributeGetTempstring(pAttributeName) ;
  cStrand = zMapGFFFeatureDataGetStr(pFeatureData) ;
  iStart = zMapGFFFeatureDataGetSta(pFeatureData) ;
  iEnd = zMapGFFFeatureDataGetEnd(pFeatureData) ;

  sResult = zMapFeatureCreateName(cStyleMode, sFeatureName, cStrand, iStart, iEnd, 0, 0) ;

  return sResult ;
}



/*
 * Create a feature of ZMapStyleMode = TRANSCRIPT only.
 *
 * First case:    A transcript object: must _have_ ID and _not_ be intron/exon/CDS.
 *                We create a new object and add to featureset if possible.
 *                Feature must _not_ be already present.
 * Second case:   A component of a transcript: _must_ be intron, exon, or CDS, _and_ have Parent
 *                Find ID from Parent attribute
 *                We add data to an existing feature in the featureset. If it's not present already
 *                this is not handled.
 *                Feature _must_ be already present.
 * Third case:    Construct a transcript feature and add a single exon with the same length to
 *                it. A feature name is constructed from the Name attribute, start, end and strand
 *                and this is used for the ID for these steps.
 * Fourth case:   Construct a complete transcript feature from a single VULGAR string.
 *
 */
static ZMapFeature makeFeatureTranscript(ZMapGFF3Parser const pParser,
                                         ZMapGFFFeatureData pFeatureData,
                                         ZMapFeatureSet pFeatureSet,
                                         gboolean *pbNewFeatureCreated,
                                         char ** psError)
{
  typedef enum {NONE, FIRST, SECOND, THIRD, FOURTH} CaseToTreat ;

  ZMapFeature pFeature = NULL ;
  CaseToTreat cCase = NONE ;
  unsigned int iSOID = 0,
    nAttributes = 0 ;
  int iStart = 0,
    iEnd = 0,
    iStartNotFound = 0 ;
  const char * sSOType = NULL,
    *sFeatureName = NULL,
    *sFeatureNameID = NULL,
    *sSequence = NULL,
    *sSource = NULL ;
  double dScore = 0.0 ;
  gboolean bHasAttributeID = FALSE,
    bHasAttributeParent = FALSE,
    bHasAttributeVulgar = FALSE,
    bFeaturePresent = FALSE,
    bDataAdded = FALSE,
    bFeatureAdded = FALSE,
    bHasScore = FALSE,
    bIsExon = FALSE,
    bIsIntron = FALSE,
    bIsCDS = FALSE,
    bIsComponent = FALSE,
    bWithinParent = FALSE,
    bStartNotFound = FALSE,
    bEndNotFound = FALSE ;
  GQuark gqThisID = 0 , gqLocusID = 0, gqThisUniqueID = 0 ;
  ZMapSOIDData pSOIDData = NULL ;
  ZMapStyleMode cFeatureStyleMode = ZMAPSTYLE_MODE_BASIC ;
  ZMapGFFAttribute *pAttributes = NULL,
    pAttributeParent = NULL,
    pAttributeID = NULL,
    pAttributeLocus = NULL,
    pAttributeTarget = NULL,
    pAttributeVulgar = NULL,
    pAttribute = NULL ;
  ZMapSpanStruct cSpanItem = {0},
    *pSpanItem = NULL ;
  ZMapStrand cStrand = ZMAPSTRAND_NONE ;
  ZMapPhase cPhase = ZMAPPHASE_NONE ;


  zMapReturnValIfFail(pParser && psError && pFeatureData && pbNewFeatureCreated, NULL) ;


  *pbNewFeatureCreated = FALSE ;

  sSequence            = zMapGFFFeatureDataGetSeq(pFeatureData) ;
  sSource              = zMapGFFFeatureDataGetSou(pFeatureData) ;
  iStart               = zMapGFFFeatureDataGetSta(pFeatureData) ;
  iEnd                 = zMapGFFFeatureDataGetEnd(pFeatureData) ;
  pSOIDData            = zMapGFFFeatureDataGetSod(pFeatureData) ;
  bHasScore            = zMapGFFFeatureDataGetFlagSco(pFeatureData) ;
  dScore               = zMapGFFFeatureDataGetSco(pFeatureData) ;
  cStrand              = zMapGFFFeatureDataGetStr(pFeatureData) ;
  cPhase               = zMapGFFFeatureDataGetPha(pFeatureData) ;

  if (!pSOIDData)
    {
      *psError = g_strdup("invalid SOIDData pointer");
      return NULL ;
    }

  sSOType              = zMapSOIDDataGetName(pSOIDData) ;
  iSOID                = zMapSOIDDataGetID(pSOIDData) ;
  cFeatureStyleMode    = zMapSOIDDataGetStyleMode(pSOIDData) ;
  if (!sSOType)
    {
      *psError = g_strdup("invalid SO term name ") ;
      return NULL ;
    }
  if (!iSOID)
    {
      *psError = g_strdup("invalid SOID numerical ID") ;
      return NULL ;
    }
  if (cFeatureStyleMode != ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      *psError = g_strdup("ZMapStyleMode must be ZMAPSTYLE_MODE_TRANSCRIPT") ;
      return NULL ;
    }

  /*
   * Firstly get some attribute information.
   */
  pAttributes = zMapGFFFeatureDataGetAts(pFeatureData) ;
  nAttributes = zMapGFFFeatureDataGetNat(pFeatureData) ;
  pAttributeID = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_ID) ;
  if (pAttributeID)
    bHasAttributeID = TRUE ;
  pAttributeParent = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_Parent);
  if (pAttributeParent)
    bHasAttributeParent = TRUE ;


  // If a VULGAR string is present it completely describes the transcript so there's no need to look for
  // a CDS or anything else.   
  if ((pAttributeVulgar = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_vulgar_exonerate)))
    {
      bHasAttributeVulgar = TRUE ;
    }
  else
    {
      bIsExon      = strstr(sSOType, "exon") != NULL ? TRUE : FALSE ;
      bIsIntron    = strstr(sSOType, "intron") != NULL ? TRUE : FALSE ;
      bIsCDS       = strstr(sSOType, "CDS") != NULL ? TRUE : FALSE ;
      bIsComponent = (bIsExon || bIsIntron || bIsCDS );
    }


  /*
   * Now the logic branches dependent upon what type of transcript we are dealing with.
   * Cases treated are described above.
   */
  if (bHasAttributeVulgar)
    {
      cCase = FOURTH ;
      sFeatureName = makeFeatureTranscriptNamePublic(pFeatureData) ;
      sFeatureNameID = makeFeatureTranscriptNamePrivate(pFeatureData) ;
      gqThisID = compositeFeaturesCreateID(pAttributes, nAttributes) ;
      gqThisUniqueID = g_quark_from_string(sFeatureNameID) ;
    }
  else if (bHasAttributeID && !bIsComponent)
    {
      cCase = FIRST ;
      sFeatureName = makeFeatureTranscriptNamePublic(pFeatureData) ;
      sFeatureNameID = makeFeatureTranscriptNamePrivate(pFeatureData) ;
      gqThisID = compositeFeaturesCreateID(pAttributes, nAttributes) ;
      gqThisUniqueID = g_quark_from_string(sFeatureNameID) ;
    }
  else if (bHasAttributeParent && bIsComponent)
    {
      cCase = SECOND ;
      gqThisID = g_quark_from_string(zMapGFFAttributeGetTempstring(pAttributeParent)) ;
      gqThisUniqueID = compositeFeaturesFind(pParser, gqThisID) ;
    }
  else if (!bHasAttributeID && !bIsComponent)
    {
      cCase = THIRD ;
      sFeatureName = makeFeatureTranscriptNamePublic(pFeatureData) ;
      sFeatureNameID = makeFeatureTranscriptNamePrivate(pFeatureData) ;
      gqThisID = 0 ;
      gqThisUniqueID = g_quark_from_string(sFeatureNameID) ;
    }
  else
    {
      // NOTE...IS THIS REALLY AN ERROR ??   

      cCase = NONE ;
    }

  if (gqThisUniqueID == 0)
    return NULL ;

  /*
   * Lookup to see if a feature with this gqThisID is already in the
   * feature set.
   */
  if ((pFeature = (ZMapFeature)g_hash_table_lookup(((ZMapFeatureAny)pFeatureSet)->children,
                                                   GINT_TO_POINTER(gqThisUniqueID))))
    bFeaturePresent = TRUE ;


  /*
   * Now the feature creation logic is as follows;
   */
  if ((cCase == FIRST) && !bFeaturePresent)
    {
      /*
       * This is a new feature, so we must create from scratch and add standard data.
       */
      pFeature = zMapFeatureCreateEmpty() ;                 // Cannot fail.
      *pbNewFeatureCreated = TRUE ;

      /*
       * We have created names already. First insert the ID_attribute -> feature->unique_id
       * pair into the composite_features mapping, and then add standard data.
       */
      if (!compositeFeaturesInsert(pParser, gqThisID, gqThisUniqueID))
        {
          zMapLogWarning("could not register composite feature "
                         "IDs (%i, %i), name = '%s', name_id = '%s'",
                         gqThisID, gqThisUniqueID, sFeatureName, sFeatureNameID) ;
        }

      bDataAdded = zMapFeatureAddStandardData(pFeature,
                                              (char*)sFeatureNameID,
                                              (char*)sFeatureName,
                                              (char*)sSequence,
                                              (char*)sSOType,
                                              cFeatureStyleMode,
                                              &pFeatureSet->style,
                                              iStart,
                                              iEnd,
                                              bHasScore,
                                              dScore,
                                              cStrand) ;

      /*
       * Add feature to featureset
       */
      bFeatureAdded = zMapFeatureSetAddFeature(pFeatureSet, pFeature) ;
      if (!bFeatureAdded)
        {
          *psError = g_strdup_printf("feature with ID = %i and name = '%s' could not be added",
                                     (int)gqThisID, sFeatureName) ;
        }

      zMapFeatureAddText(pFeature, g_quark_from_string(sSource), (char*)sSource, NULL) ;
    }
  else if ((cCase == SECOND) && bFeaturePresent)
    {
      /*
       * Only add subfeature data if their start and end is within that
       * of the parent object. This traps extremely rare errors (probably due
       * to corrupted source data).
       */
      bWithinParent = (iStart >= pFeature->x1) && (iEnd <= pFeature->x2) ;
      if (bWithinParent)
        {
          if (bIsExon)
            {
              cSpanItem.x1 = iStart ;
              cSpanItem.x2 = iEnd ;
              pSpanItem = &cSpanItem;
              bDataAdded = zMapFeatureAddTranscriptExonIntron(pFeature, pSpanItem, NULL) ;
            }
          else if (bIsIntron)
            {
              cSpanItem.x1 = iStart ;
              cSpanItem.x2 = iEnd ;
              pSpanItem = &cSpanItem ;
              bDataAdded = zMapFeatureAddTranscriptExonIntron(pFeature, NULL, pSpanItem) ;
            }
          else if (bIsCDS)
            {
              /*
               * CDS data; also include start_not_found and end_not_found.
               */
              if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_start_not_found)))
                {
                  bStartNotFound = zMapAttParseCDSStartNotFound(pAttribute, &iStartNotFound) ;
                }
              if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_end_not_found)))
                {
                  bEndNotFound = zMapAttParseCDSEndNotFound(pAttribute) ;
                }
              bDataAdded = zMapFeatureAddTranscriptCDSDynamic(pFeature, iStart, iEnd, cPhase,
                                                              bStartNotFound, bEndNotFound, iStartNotFound) ;
            }
        }

      if (!bDataAdded)
        {
          *psError = g_strdup_printf("unable to add ZMapSpan to feature with ID = %i and name = '%s'",
                                     (int)gqThisID, sFeatureName) ;
        }

    }
  else if ((cCase == THIRD || cCase == FOURTH) && !bFeaturePresent)
    {
      /*
       * Firstly create the feature object as transcript.
       */
      pFeature = zMapFeatureCreateEmpty() ;
      *pbNewFeatureCreated = TRUE ;

      /*
       * We have names already so can add standard data.
       */
      bDataAdded = zMapFeatureAddStandardData(pFeature,
                                              (char*)sFeatureNameID,
                                              (char*)sFeatureName,
                                              (char*)sSequence,
                                              (char*)sSOType,
                                              cFeatureStyleMode, &pFeatureSet->style,
                                              iStart, iEnd, bHasScore, dScore, cStrand) ;

      bFeatureAdded = zMapFeatureSetAddFeature(pFeatureSet, pFeature) ;
      if (!bFeatureAdded)
        {
          *psError = g_strdup_printf("feature with ID = %i and name = '%s' could not be added",
                                     (int)gqThisID, sFeatureName) ;
        }

      zMapFeatureAddText(pFeature, g_quark_from_string(sSource), (char*)sSource, NULL) ;

      if (cCase == THIRD)
        {
          // Transcript line without any subparts so we hack a single exon spanning the whole feature.
          cSpanItem.x1 = iStart ;
          cSpanItem.x2 = iEnd ;
          pSpanItem = &cSpanItem;
          bDataAdded = zMapFeatureAddTranscriptExonIntron(pFeature, pSpanItem, NULL) ;
          if (!bDataAdded)
            {
              *psError = g_strdup_printf("unable to add ZMapSpan to feature with ID = %i and name = '%s'",
                                         (int)gqThisID, sFeatureName) ;
            }
        }
      else                                                  // cCase == FOURTH
        {
          // Transcript derived from a VULGAR string given by the "vulgar_exonerate" attribute.
          GQuark gqTargetID = 0 ;
          int iTargetStart = 0, iTargetEnd = 0 ;
          ZMapStrand cTargetStrand ;
          GArray *exons = NULL, *introns = NULL, *exon_aligns = NULL ;

          if ((pAttributeTarget = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_Target))
              && (zMapAttParseTarget(pAttributeTarget, &gqTargetID, &iTargetStart, &iTargetEnd, &cTargetStrand))
              && (zMapAttParseVulgarExonerate(pAttributeVulgar,
                                              cStrand, iStart, iEnd,
                                              cTargetStrand, iTargetStart, iTargetEnd,
                                              &exons, &introns, &exon_aligns)))
            {
              if (!zMapFeatureTranscriptAddSubparts(pFeature, exons, introns)
                  || !zMapFeatureTranscriptAddAlignparts(pFeature,
                                                         iTargetStart, iTargetEnd, cTargetStrand,
                                                         exon_aligns, pAttributeVulgar->sTemp))
                {
                  *psError = g_strdup("unable to add exons/introns/exon_aligns derived from vulgar string") ;
                }
              else
                {
                  /*
                   * We have created names already. First insert the ID_attribute -> feature->unique_id
                   * pair into the composite_features mapping, and then add standard data.
                   */
                  if (!compositeFeaturesInsert(pParser, gqThisID, gqThisUniqueID))
                    {
                      zMapLogWarning("could not register composite feature "
                                     "IDs (%i, %i), name = '%s', name_id = '%s'",
                                     gqThisID, gqThisUniqueID, sFeatureName, sFeatureNameID) ;
                    }

                }




            }
          else
            {
              *psError = g_strdup("unable to parse target or vulgar string") ;
            }
        }
    }

  /*
   * Deal with "locus" attribute if present for a newly-created feature only.
   */
  if (*pbNewFeatureCreated)
    {
      if ((pAttributeLocus = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_locus)))
        {
          if (zMapAttParseLocus(pAttributeLocus, &gqLocusID))
            {
              zMapFeatureAddLocus(pFeature, gqLocusID) ;
            }
        }
    }

  /*
   * If a new feature was created but not added to the feature set for some reason,
   * it must be destroyed to avoid a memory leak.
   */
  if (*pbNewFeatureCreated && !bFeatureAdded)
    {
      zMapFeatureDestroy(pFeature) ;
      pFeature = NULL ;
    }

  return pFeature ;
}






/*
 * Make a locus feature with data for the current "real" transcript feature.
 */
gboolean makeFeatureLocus(ZMapGFFParser pParser, ZMapGFFFeatureData pFeatureData , char ** psError)
{
  static const char *sType = "Locus" ;
  char *sLocusID = NULL, *sName = NULL, *sNameID = NULL ;
  int iStart = 0, iEnd = 0, iTargetStart = 0, iTargetEnd = 0 ;
  gboolean bResult = FALSE ;
  ZMapFeature pFeature = NULL ;
  ZMapGFFAttribute pAttribute = NULL, *pAttributes = NULL ;
  ZMapGFFFeatureData pFeatureDataLocus = NULL ;
  ZMapStrand cStrand = ZMAPSTRAND_NONE ;
  ZMapPhase cPhase = ZMAPPHASE_NONE ;
  ZMapHomolType cHomol = ZMAPHOMOL_NONE ;
  ZMapStyleMode cStyleMode = ZMAPSTYLE_MODE_TEXT ;
  ZMapSOIDData pSOIDData = NULL ;
  ZMapFeatureSet pFeatureSet = NULL ;
  GQuark gqLocusID = 0 ;
  unsigned int nAttributes = 0, iSOID = 0 ;

  zMapReturnValIfFail(pFeatureData && psError, bResult ) ;
  bResult = TRUE ;

  nAttributes          = zMapGFFFeatureDataGetNat(pFeatureData) ;
  pAttributes          = zMapGFFFeatureDataGetAts(pFeatureData) ;

  if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_locus) ) && bResult )
    {
      bResult = zMapAttParseLocus(pAttribute, &gqLocusID) ;
      sLocusID = g_strdup(zMapGFFAttributeGetTempstring(pAttribute)) ;
    }
  else
    {
      bResult = FALSE ;
    }

  if (bResult)
    {

      /*
       * Use the feature "Name" for the sequence
       * Use "Locus" for the source
       * Use "Locus" for ontology
       * Use the StyleMode TEXT
       * There is a faked SO accession for these.
       */
      pFeatureDataLocus = zMapGFFFeatureDataCC(pFeatureData) ;
      zMapGFFFeatureDataSetSeq(pFeatureDataLocus, sLocusID ) ;
      zMapGFFFeatureDataSetSou(pFeatureDataLocus, sType ) ;
      zMapGFFFeatureDataSetFlagSco(pFeatureDataLocus, FALSE) ;
      zMapGFFFeatureDataSetSco(pFeatureDataLocus, 0.0) ;
      zMapGFFFeatureDataSetStr(pFeatureDataLocus, cStrand) ;
      zMapGFFFeatureDataSetPha(pFeatureDataLocus, cPhase) ;
      iStart = zMapGFFFeatureDataGetSta(pFeatureDataLocus) ;
      iEnd = zMapGFFFeatureDataGetEnd(pFeatureDataLocus) ;

      /*
       * SOIDData must contain the hacked SO:000ijk number and map this
       * to ZMAPSTYLE_MODE_TEXT. The homol type must be NONE.
       */
      if ((iSOID = zMapSOSetIsNamePresent(ZMAPSO_USE_NONE, sType)) != ZMAPSO_ID_UNK)
        {
          pSOIDData = zMapSOIDDataCreateFromData(iSOID, sType, cStyleMode, cHomol ) ;
        }
      zMapGFFFeatureDataSetSod(pFeatureDataLocus, pSOIDData) ;

      /*
       * Find featureset and other data.
       */
      findFeatureset(pParser, pFeatureDataLocus, &pFeatureSet) ;

      /*
       * Make a name and a name_id for the feature. The name comes from the locus ID and
       * the name_id is created in the usual fashion.
       */
      sName = g_strdup(sLocusID) ;
      sNameID = zMapFeatureCreateName(cStyleMode, sName, ZMAPSTRAND_NONE, iStart, iEnd, iTargetStart, iTargetEnd ) ;

      /*
       * Make the feature, add standard data to it and add it to featureset.
       */
      pFeature = zMapFeatureCreateEmpty() ;
      if (!pFeature)
        {
          *psError = g_strdup_printf("makeFeatureLocus(); could not create feature with name_id = '%s' and name = '%s'",
                                      sNameID, sName) ;
          bResult = FALSE ;
        }

      if (bResult)
        {
          bResult = zMapFeatureAddStandardData(pFeature, sNameID, sName, sLocusID, (char*)sType,
                                              cStyleMode, &pFeatureSet->style,
                                              iStart, iEnd, FALSE, 0.0, cStrand) ;
        }
      else
        {
          zMapFeatureDestroy(pFeature) ;
          *psError = g_strdup_printf("makeFeatureLocus(); could not add standard data to name_id = '%s' and name = '%s'",
                                      sNameID, sName) ;
        }

      if (bResult)
        {
          bResult = zMapFeatureSetAddFeature(pFeatureSet, pFeature) ;
          if (!bResult)
            {
              zMapFeatureDestroy(pFeature) ;
              *psError = g_strdup_printf("makeFeatureLocus(); could not add feature with name_id = '%s' and name = '%s' to featureset",
                                         sNameID, sName) ;
            }
        }


    } /* if (bResult) */

  /*
   * Clean up
   */
  if (sLocusID)
    g_free(sLocusID) ;
  zMapGFFFeatureDataDestroy(pFeatureDataLocus) ;

  return bResult ;
}






/*
 * Create a feature of ZMapStyleMode = ALIGNMENT only.
 */
static ZMapFeature makeFeatureAlignment(ZMapGFFFeatureData pFeatureData,
                                        ZMapFeatureSet pFeatureSet,
                                        char ** psError)
{
  typedef enum {NONE, FIRST, SECOND} CaseToTreat ;
  CaseToTreat cCase = NONE ;
  unsigned int iSOID = 0,
    nAttributes = 0 ;
  int iStart = 0,
    iEnd = 0,
    iTargetStart = 0,
    iTargetEnd = 0,
    iLength = 0 ;
  const char *sSequence = NULL,
    *sSource = NULL,
    *sSOType = NULL ;
  char *sFeatureName = NULL,
    *sFeatureNameID = NULL ;
  const char * sIdentifier = NULL ;
  double dScore = 0.0,
    dPercentID = 0.0 ;
  gboolean bHasScore = FALSE,
    bNewFeatureCreated = FALSE,
    bFeatureAdded = FALSE,
    bParseAttribute = FALSE,
    bParseAttributeTarget = FALSE,
    bDataAdded = FALSE ;
  GArray *pGaps = NULL ;
  GQuark gqTargetID = 0 ;
  ZMapSOIDData pSOIDData = NULL ;
  ZMapFeature pFeature = NULL ;
  ZMapGFFAttribute *pAttributes = NULL,
    pAttribute = NULL,
    pAttributeTarget = NULL ;
  ZMapStrand cStrand = ZMAPSTRAND_NONE,
    cTargetStrand = ZMAPSTRAND_NONE ;
  ZMapPhase cPhase = ZMAPPHASE_NONE ;
  ZMapStyleMode cFeatureStyleMode = ZMAPSTYLE_MODE_BASIC ;
  ZMapHomolType cHomolType = ZMAPHOMOL_NONE ;

  zMapReturnValIfFail(pFeatureData && psError, pFeature) ;

  sSequence            = zMapGFFFeatureDataGetSeq(pFeatureData) ;
  sSource              = zMapGFFFeatureDataGetSou(pFeatureData) ;
  iStart               = zMapGFFFeatureDataGetSta(pFeatureData) ;
  iEnd                 = zMapGFFFeatureDataGetEnd(pFeatureData) ;
  pSOIDData            = zMapGFFFeatureDataGetSod(pFeatureData) ;
  bHasScore            = zMapGFFFeatureDataGetFlagSco(pFeatureData) ;
  dScore               = zMapGFFFeatureDataGetSco(pFeatureData) ;
  cStrand              = zMapGFFFeatureDataGetStr(pFeatureData) ;
  cPhase               = zMapGFFFeatureDataGetPha(pFeatureData) ;
  if (!pSOIDData)
    {
      *psError = g_strdup("makeFeatureAlignment(); invalid SOIDData pointer");
      return pFeature ;
    }
  sSOType              = zMapSOIDDataGetName(pSOIDData) ;
  iSOID                = zMapSOIDDataGetID(pSOIDData) ;
  cFeatureStyleMode    = zMapSOIDDataGetStyleMode(pSOIDData) ;
  cHomolType           = zMapSOIDDataGetHomol(pSOIDData) ;
  if (!sSOType)
    {
      *psError = g_strdup("makeFeatureAlignment(); invalid SO term name ") ;
      return pFeature ;
    }
  if (!iSOID)
    {
      *psError = g_strdup("makeFeatureAlignment(); invalid SOID numerical ID") ;
      return pFeature ;
    }
  if (cFeatureStyleMode != ZMAPSTYLE_MODE_ALIGNMENT)
    {
      *psError = g_strdup("makeFeatureAlignment(); ZMapStyleMode must be ZMAPSTYLE_MODE_ALIGNMENT") ;
      return pFeature ;
    }
  if (cHomolType == ZMAPHOMOL_NONE)
    {
      *psError = g_strdup("makeFeatureAlignment(); ZMapHomol must not be ZMAPHOMOL_NONE for ZMAPSTYLE_MODE_ALIGNMENT") ;
      return pFeature ;
    }

  /*
   * Firstly get basic data for attributes.
   */
  pAttributes = zMapGFFFeatureDataGetAts(pFeatureData) ;
  nAttributes = zMapGFFFeatureDataGetNat(pFeatureData) ;

  /*
   * (sm23) Modified on 01/08/2014 to remove need for ID attribute.
   *
   * ************ OLD COMMENT ***************
   * We treat two cases:
   * (a) First is an "ordinary" alignment where we are after the "Target" attribute.
   * (b) Second is a SO_term = "read", where we are interested in the "Target" and the "ID"
   *     attributes. The "ID" attribute is used to associated the two parts of a read pair.
   *
   */

  if ((pAttributeTarget = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_Target)))
    {
      bParseAttributeTarget = zMapAttParseTarget(pAttributeTarget, &gqTargetID,
                                                 &iTargetStart, &iTargetEnd, &cTargetStrand) ;
    }

  /*
  if ((pAttributeID = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_ID)))
    {
      bParseAttributeID = zMapAttParseID(pAttributeID, &gqTargetID) ;
    }



  if  (         !strstr(sRead, sSOType)
             && bParseAttributeTarget
      )
    {
      cCase = FIRST ;
    }
  else if (     strstr(sRead, sSOType)
             && bParseAttributeTarget
             && bParseAttributeID
          )
    {
      cCase = SECOND ;
    }
   */

  if (bParseAttributeTarget)
    cCase = FIRST ;

  /*
   * Get case dependent information
   */
  if (cCase == FIRST)
    {
      sIdentifier = g_quark_to_string(gqTargetID) ;
    }
  else if (cCase == SECOND)
    {
      sIdentifier = g_quark_to_string(gqTargetID) ;
    }

  /*
   * Create the feature with all appropriate data
   */
  if (
         cCase != NONE
     )
    {

      /*
       * Create a new feature.
       */
      pFeature = zMapFeatureCreateEmpty() ;
      if (pFeature)
        {
          bNewFeatureCreated = TRUE ;
        }
      else
        {
          *psError = g_strdup_printf("makeFeatureAlignment(); could not create new feature object ");
          return pFeature ;
        }

      sFeatureName = g_strdup(sIdentifier) ;
      sFeatureNameID = zMapFeatureCreateName(cFeatureStyleMode, sFeatureName, cStrand, iStart, iEnd, iTargetStart, iTargetEnd) ;

      bDataAdded = zMapFeatureAddStandardData(pFeature,
                                              (char*)sFeatureNameID,
                                              (char*)sFeatureName,
                                              (char*)sSequence,
                                              (char*)sSOType,
                                              cFeatureStyleMode, &pFeatureSet->style,
                                              iStart, iEnd, bHasScore, dScore, cStrand) ;

      /*
       * "percentID" attribute
       */
      if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_percentID)))
        {
          bParseAttribute = zMapAttParsePID(pAttribute, &dPercentID) ;
        }

      /*
       * "length" attribute
       */
      if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_length)))
        {
          bParseAttribute = zMapAttParseLength(pAttribute, &iLength) ;
        }

      /*
       * Now parse for gap data in various possible formats
       */
      if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_Gap)))
        {
          bParseAttribute = zMapAttParseGap(pAttribute, &pGaps, cStrand, iStart, iEnd, cTargetStrand, iTargetStart, iTargetEnd) ;
        }
      else if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_cigar_ensembl)))
        {
          bParseAttribute = zMapAttParseCigarEnsembl(pAttribute, &pGaps, cStrand, iStart, iEnd, cTargetStrand, iTargetStart, iTargetEnd) ;
        }
      else if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_cigar_exonerate)))
        {
          bParseAttribute = zMapAttParseCigarExonerate(pAttribute, &pGaps, cStrand, iStart, iEnd, cTargetStrand, iTargetStart, iTargetEnd);
        }
      else if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_cigar_bam)))
        {
          bParseAttribute = zMapAttParseCigarBam(pAttribute, &pGaps, cStrand, iStart, iEnd, cTargetStrand, iTargetStart, iTargetEnd);
        }
      if (!bParseAttribute && pGaps)
        {
          g_array_free(pGaps, TRUE) ;
          pGaps = NULL ;
        }

      /*
       * Add data to the feature.
       */
      bDataAdded = zMapFeatureAddAlignmentData(pFeature, gqTargetID, dPercentID,
                                               iTargetStart, iTargetEnd, cHomolType,
                                               iLength, cTargetStrand, ZMAPPHASE_0, pGaps,
                                               zMapStyleGetWithinAlignError(pFeatureSet->style), FALSE, NULL )  ;
      if (bDataAdded)
        {
          bFeatureAdded = zMapFeatureSetAddFeature(pFeatureSet, pFeature) ;
        }
      if (!bFeatureAdded)
        {
          *psError = g_strdup_printf("makeFeatureAlignment(); feature with ID = %i and name = '%s' could not be added",
                                     (int)pFeature->unique_id, sFeatureName) ;
        }

      zMapFeatureAddText(pFeature, g_quark_from_string(sSource), (char*)sSource, NULL) ;
    }

  /*
   * If a new feature was created, but could not be added to the featureset,
   * it must be destroyed to prevent a memory leak.
   */
  if (bNewFeatureCreated && !bFeatureAdded)
    {
      zMapFeatureDestroy(pFeature) ;
      pFeature = NULL ;
    }


  return pFeature ;
}


/*
 * Create a feature with ZMapStyleMode = ASSEMBLY_PATH; not yet implemented, although the
 * code to parse the appropriate attribute value is in zmapGFFAttribute.c.
 */
static ZMapFeature makeFeatureAssemblyPath(ZMapGFFFeatureData pFeatureData,
                                         ZMapFeatureSet pFeatureSet,
                                         char ** psError)
{
  ZMapFeature pFeature = NULL ;

  return pFeature ;
}


/*
 * Default feature creation function.
 */
static ZMapFeature makeFeatureDefault(ZMapGFFFeatureData pFeatureData,
  const ZMapFeatureSet pFeatureSet, char **psError)
{
  const char *sSequence = NULL,
    *sSource = NULL,
    *sSOType = NULL,
    *sAttributes = NULL ;
  char *sName = NULL,
    *sFeatureName = NULL,
    *sFeatureNameID = NULL ;
  unsigned int nAttributes = 0 ;
  int iStart = 0,
    iEnd = 0,
    iQueryStart = 0,
    iQueryEnd = 0,
    iSOID = 0 ;
  double dScore = 0.0 ;
  gboolean bFeatureHasName = FALSE,
    bDataAdded = FALSE,
    bFeatureAdded = FALSE,
    bNewFeatureCreated = FALSE,
    bHasScore = FALSE,
    bParseValid = FALSE ;
  ZMapFeature pFeature = NULL ;
  ZMapGFFAttribute *pAttributes = NULL,
    pAttribute = NULL ;
  ZMapStyleMode cFeatureStyleMode = ZMAPSTYLE_MODE_BASIC ;
  ZMapStrand cStrand = ZMAPSTRAND_NONE ;
  ZMapPhase cPhase = ZMAPPHASE_NONE ;
  ZMapSOIDData pSOIDData = NULL ;

  zMapReturnValIfFail(pFeatureData && pFeatureSet && psError, pFeature ) ;

  sSequence            = zMapGFFFeatureDataGetSeq(pFeatureData) ;
  sSource              = zMapGFFFeatureDataGetSou(pFeatureData) ;
  iStart               = zMapGFFFeatureDataGetSta(pFeatureData) ;
  iEnd                 = zMapGFFFeatureDataGetEnd(pFeatureData) ;
  cStrand              = zMapGFFFeatureDataGetStr(pFeatureData) ;
  cPhase               = zMapGFFFeatureDataGetPha(pFeatureData) ;
  bHasScore            = zMapGFFFeatureDataGetFlagSco(pFeatureData) ;
  dScore               = zMapGFFFeatureDataGetSco(pFeatureData) ;
  sAttributes          = zMapGFFFeatureDataGetAtt(pFeatureData) ;
  nAttributes          = zMapGFFFeatureDataGetNat(pFeatureData) ;
  pAttributes          = zMapGFFFeatureDataGetAts(pFeatureData) ;

  pSOIDData            = zMapGFFFeatureDataGetSod(pFeatureData) ;
  sSOType              = zMapSOIDDataGetName(pSOIDData) ;
  iSOID                = zMapSOIDDataGetID(pSOIDData) ;
  cFeatureStyleMode    = zMapSOIDDataGetStyleMode(pSOIDData) ;

  /*
   * Make sure that this function is _not_ being called with ZMapStyleMode is
   * TRANSCRIPT or ALIGNMENT becasuse they should have been dealt with before
   * this point.
   */
  if (    (cFeatureStyleMode == ZMAPSTYLE_MODE_TRANSCRIPT)
      ||  (cFeatureStyleMode == ZMAPSTYLE_MODE_ALIGNMENT)
      ||  (cFeatureStyleMode == ZMAPSTYLE_MODE_ASSEMBLY_PATH)
      )
    {
      *psError = g_strdup("makeFeatureDefault(); ZMapStyleMode must _not_ be TRANSCRIPT or ALIGNMENT or ASSEMBLY_PATH") ;
      return pFeature ;
    }

  /*
   * Get attributes.
   */
  pAttributes = zMapGFFFeatureDataGetAts(pFeatureData) ;
  nAttributes = zMapGFFFeatureDataGetNat(pFeatureData) ;

   /*
    * Look for "Name" attribute
    */
  pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_Name );
  if (pAttribute)
    {
      bParseValid = zMapAttParseName(pAttribute, &sName) ;
    }

  /*
   * Get a name for the feature.
   */
  bFeatureHasName = getFeatureName(sSequence, pAttributes, nAttributes, sName, sSource,
                                   cFeatureStyleMode, cStrand, iStart, iEnd, iQueryStart, iQueryEnd,
                                   &sFeatureName, &sFeatureNameID) ;
  if (!sFeatureNameID)
    {
      *psError = g_strdup_printf("makeFeatureDefault(); feature ignored as it has no name");
      return pFeature ;
    }

  /*
   * Create our new feature object
   */
  pFeature = zMapFeatureCreateEmpty() ;
  if (pFeature)
    bNewFeatureCreated = TRUE ;
  else
    {
      *psError = g_strdup_printf("makeFeatureDefault(); could not create new feature object ");
      return pFeature ;
    }

  /*
   * Add standard data to the feature.
   */
  bDataAdded = zMapFeatureAddStandardData(pFeature,
                                          (char*)sFeatureNameID,
                                          (char*)sFeatureName,
                                          (char*)sSequence,
                                          (char*)sSOType,
                                       cFeatureStyleMode, &pFeatureSet->style,
                                       iStart, iEnd, bHasScore, dScore, cStrand);

  /*
   * Handle the case where we have a splice site of some kind. These were originally handled
   * with the SO terms "splice3" and "splice5" in GFF2 output from AceDB.
   */
  if (bDataAdded)
    {
      if (!strcmp(sSOType, "five_prime_cis_splice_site"))
        bDataAdded = zMapFeatureAddSplice(pFeature, ZMAPBOUNDARY_5_SPLICE) ;
      else if (!strcmp(sSOType, "three_prime_cis_splice_site"))
        bDataAdded = zMapFeatureAddSplice(pFeature, ZMAPBOUNDARY_3_SPLICE) ;
    }

  /*
   * Add the feature to feature set
   */
  if (bDataAdded)
    {
      bFeatureAdded = zMapFeatureSetAddFeature(pFeatureSet, pFeature) ;
    }
  if (!bFeatureAdded)
    {
      *psError = g_strdup_printf("makeFeatureDefault(); feature with ID = %i and name = '%s' could not be added",
                                     (int)pFeature->unique_id, sFeatureName) ;
    }

  zMapFeatureAddText(pFeature, g_quark_from_string(sSource), (char*)sSource, NULL) ;

  /*
   * If a new feature was created, but could not be added to the featureset,
   * it must be destroyed to prevent a memory leak.
   */
  if (bNewFeatureCreated && !bFeatureAdded)
    {
      zMapFeatureDestroy(pFeature) ;
      pFeature = NULL ;
    }

  if (sName)
    {
      g_free(sName) ;
    }

  return pFeature ;
}



/*
 * This clipping function ignores the clip mode of the parser, and clips everything
 * that is completely outside of the [pParser->clip_start, pParser->clip_end] range;
 * If the values of clip_start and clip_end are not set in the parser, then we
 * include the feature.
 */
static gboolean clipFeatureLogic_Complete(ZMapGFF3Parser  pParser, ZMapGFFFeatureData pFeatureData )
{
  gboolean bIncludeFeature = TRUE ;
  int iStart = 0,
    iEnd = 0,
    iClipStart = 0,
    iClipEnd = 0 ;
  gboolean bFeatureOutside = FALSE ;
  ZMapGFFClipMode cClipMode = GFF_CLIP_NONE ;

  zMapReturnValIfFail(pParser && pParser->pHeader && pFeatureData, bIncludeFeature) ;

  cClipMode = pParser->clip_mode ;
  iClipStart = pParser->clip_start ;
  iClipEnd = pParser->clip_end ;
  if (!iClipStart && !iClipEnd)
    return bIncludeFeature ;

  /*
   * Get some data about the feature, and error check.
   */
  iStart               = zMapGFFFeatureDataGetSta(pFeatureData) ;
  iEnd                 = zMapGFFFeatureDataGetEnd(pFeatureData) ;
  zMapReturnValIfFail(iStart && iEnd, bIncludeFeature ) ;

  /*
   * Clipping logic.
   */
  if (cClipMode == GFF_CLIP_NONE)
    {

    }
  else
    {

      /* Exclude feature completely outisde parser boundaries. */
      bFeatureOutside = (iStart > iClipEnd) || (iEnd < iClipStart) ;
      if (bFeatureOutside)
          bIncludeFeature = FALSE ;


    }


  return bIncludeFeature ;
}


/*
 * Encapsulation of clipping logic. This now has to take into account the StyleMode
 * and other data concerning the feature of interest.
 *
 * Behaviour is dependent on the relationship between the parser clip boundaries, call them
 * [clip_start, clip_end] and the feature start and end, call that [iStart, iEnd]. We may
 * exclude a feature completely or truncate it.
 *
 * (1) CLIP_NONE          do nothing
 * (2) CLIP_ALL          { exclude features _completely_ outside }   _and_   exclude features that overlap boundaries
 * (3) CLIP_OVERLAP      { parser boundaries                     }   _and_   truncate features that overlap boundaries
 *
 */
static gboolean clipFeatureLogic_General(ZMapGFF3Parser  pParser, ZMapGFFFeatureData pFeatureData )
{
  gboolean bIncludeFeature = FALSE ;
  int iStart = 0,
    iEnd = 0,
    iClipStart = 0,
    iClipEnd = 0 ;
  const char *sSOType = NULL ;
  gboolean bFeatureOutside = FALSE,
    bFeatureOverlapStart = FALSE,
    bFeatureOverlapEnd = FALSE ;
  ZMapSOIDData pSOIDData = NULL ;
  ZMapStyleMode cFeatureStyleMode = ZMAPSTYLE_MODE_INVALID ;
  ZMapGFFClipMode cClipMode = GFF_CLIP_NONE ;

  zMapReturnValIfFail(pParser && pParser->pHeader && pFeatureData, bIncludeFeature) ;

  cClipMode = pParser->clip_mode ;
  iClipStart = pParser->clip_start ;
  iClipEnd = pParser->clip_end ;
  if (!iClipStart && !iClipEnd)
    return TRUE ;

  /*
   * Get some data about the feature, and error check.
   */
  iStart               = zMapGFFFeatureDataGetSta(pFeatureData) ;
  iEnd                 = zMapGFFFeatureDataGetEnd(pFeatureData) ;
  pSOIDData            = zMapGFFFeatureDataGetSod(pFeatureData) ;
  cFeatureStyleMode    = zMapSOIDDataGetStyleMode(pSOIDData) ;
  sSOType              = zMapSOIDDataGetName(pSOIDData) ;
  zMapReturnValIfFail(iStart && iEnd
                  && (cFeatureStyleMode != ZMAPSTYLE_MODE_INVALID), bIncludeFeature ) ;

  /*
   * Default behaviour is to include the feature.
   */
  bIncludeFeature = TRUE ;

  /*
   * Clipping logic.
   */
  if (cClipMode == GFF_CLIP_NONE)
    {

    }
  else
    {

      /* Exclude feature completely outisde parser boundaries. */
      bFeatureOutside = (iStart > iClipEnd) || (iEnd < iClipStart) ;
      if (bFeatureOutside)
          bIncludeFeature = FALSE ;

      /* Does the feature overlap the start and end of parser boundaries? */
      bFeatureOverlapStart = (iStart < iClipStart) ;
      bFeatureOverlapEnd = (iEnd > iClipEnd) ;

      /* Exclude feature that overlaps the parser boundaries. */
      if (bIncludeFeature && (cClipMode == GFF_CLIP_ALL))
        {
          if (bFeatureOverlapStart || bFeatureOverlapEnd )
              bIncludeFeature = FALSE ;
        }

      /* Truncate a feature that overlaps the parser boundaries. */
      if (bIncludeFeature && cClipMode == GFF_CLIP_OVERLAP)
        {
          if (bFeatureOverlapStart)
            {
              iStart = iClipStart ;
              zMapGFFFeatureDataSetSta(pFeatureData, iStart) ;
            }
          if (bFeatureOverlapEnd)
            {
              iEnd = iClipEnd ;
              zMapGFFFeatureDataSetEnd(pFeatureData, iEnd) ;
            }
        }

    }


  return bIncludeFeature ;
}


/*
 * This function is a nasty hack to get around the following problem:
 *
 * In the old v2 code, there were many columns that were fed through with
 * sType = "misc_feature"; these were then treated in a variety of fashions,
 * with different StyleMode. The type "misc_feature" is currently (December
 * 2013) translated into the "sequence_feature" SO term. However the mapping
 * from SO term to StyleMode must be many-to-one, so cannot be used in the
 * case of these columns.
 *
 * This function selects an appropriate SO term for the columns for which
 * this special treatment is required, which is then used as part of the
 * correct SO => StyleMode mapping that I've implemented for GFFv3. This
 * seems a bit arbitrary, but is at least expident. Main advantage is that
 * the style (in the ZMap written configuration) does not have to be altered
 * for the affected columns.
 *
 */
static gboolean hack_SpecialColumnToSOTerm(const char * const sSource, char * const sType )
{
  /*
   * List of special source names to be treated by this function.
   */
  static const char *sCol01 = "das_constrained_regions" ;
  static const char *sCol02 = "das_phastCons" ;
  static const char *sCol05 = "solexa_coverage" ;
  static const char *sCol06 = "das_ChromSig" ;
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(sSource && *sSource && sType, bResult ) ;

  if (!strcmp(sSource, sCol01))
    {
      strcpy(sType, "transcript" );
      bResult = TRUE ;
    }
  else if (strstr(sSource, sCol02)) /* several sources start with the string "das_phastCons" */
    {
      strcpy(sType, "das_phastCons") ;
      bResult = TRUE ;
    }
  else if (strstr(sSource, sCol05)) /* several sources start with the string "solexa_coverage" */
    {
      strcpy(sType, "solexa_coverage") ;
      bResult = TRUE ;
    }
  else if (!strcmp(sSource, sCol06))
    {
      strcpy(sType, "transcript") ;
    }

  return bResult ;
}



/*
 * Create a new feature and add it to the feature set.
 *
 */
static gboolean makeNewFeature_V3(ZMapGFFParser pParserBase,
                                  char ** const err_text,
                                  ZMapGFFFeatureData pFeatureData)
{
  unsigned int nAttributes = 0 ;

  char *sURL = NULL,
    *sVariation = NULL,
    *sNote = NULL ;
  const char *sSOType = NULL ;

  gboolean bResult = FALSE,
    bNewFeatureCreated = FALSE,
    bIncludeFeature = TRUE,
    bFoundFeatureset = FALSE,
    bLocusFeature = FALSE ;

  ZMapFeature pFeature = NULL ;
  ZMapFeatureSet pFeatureSet = NULL ;
  ZMapStyleMode cFeatureStyleMode = ZMAPSTYLE_MODE_INVALID ;
  ZMapSOIDData pSOIDData = NULL ;
  ZMapGFFAttribute pAttribute = NULL,
    *pAttributes = NULL ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader && pFeatureData, FALSE) ;

  /*
   * Take some data from the ZMapGFFFeatureData object
   */
  nAttributes          = zMapGFFFeatureDataGetNat(pFeatureData) ;
  pAttributes          = zMapGFFFeatureDataGetAts(pFeatureData) ;
  pSOIDData            = zMapGFFFeatureDataGetSod(pFeatureData) ;
  sSOType              = zMapSOIDDataGetName(pSOIDData) ;
  cFeatureStyleMode    = zMapSOIDDataGetStyleMode(pSOIDData) ;

  /*
   * This is doing some lookups to find styles for features (or at least for the
   * feature set at once). If it fails then we exit doing nothing.
   */
  bFoundFeatureset = findFeatureset(pParserBase, pFeatureData, &pFeatureSet) ;

  /*
   * Test the StyleMode of the feature against that of the Style itself
   */
  if (bFoundFeatureset)
    {
      if (!(bResult = pFeatureSet->style->mode == cFeatureStyleMode ))
        {
          *err_text = g_strdup_printf("makeNewFeature_V3(); feature StyleMode did not match FeatureSet; feature not created") ;
        }
    }

  /*
   * Now branch on the ZMapStyleMode of the current GFF line.
   */
  if (bFoundFeatureset && bResult && (cFeatureStyleMode != ZMAPSTYLE_MODE_INVALID))
    {

      if (cFeatureStyleMode == ZMAPSTYLE_MODE_TRANSCRIPT)
        {

#ifdef CLIP_TRANSCRIPT_ON_PARSE
          if ((bIncludeFeature = clipFeatureLogic_Transcript(pParser, pFeatureData )))
            {
#endif

              pFeature = makeFeatureTranscript(pParser, pFeatureData, pFeatureSet, &bNewFeatureCreated, err_text) ;
              if (pFeature)
                {
                  bResult = TRUE ;

                  // TESTING, TESTING, TESTING.....  
                  {
                    ZMapFeature feature_copy ;

                    feature_copy = (ZMapFeature)zMapFeatureAnyCopy((ZMapFeatureAny)pFeature) ;
                  
                    zMapFeatureAnyDestroy((ZMapFeatureAny)feature_copy) ;
                  }
                }

              if (bNewFeatureCreated)
                zMapGFFParserIncrementNumFeatures(pParserBase) ;

              if (requireLocusOperations(pParserBase, pFeatureData) && bNewFeatureCreated)
                {
#ifdef CLIP_LOCUS_ON_PARSE
                  if (clipFeatureLogic_General(pParser, pFeatureData ))
                    {
#endif
                      bLocusFeature = makeFeatureLocus(pParserBase, pFeatureData, err_text) ;
#ifdef CLIP_LOCUS_ON_PARSE
                    }
#endif
                }

#ifdef CLIP_TRANSCRIPT_ON_PARSE
            }
#endif

        }
      else if (cFeatureStyleMode == ZMAPSTYLE_MODE_ALIGNMENT)
        {
#ifdef CLIP_ALIGNMENT_ON_PARSE
          if ((bIncludeFeature = clipFeatureLogic_Complete(pParser, pFeatureData )))
            {
#endif
              pFeature = makeFeatureAlignment(pFeatureData, pFeatureSet, err_text) ;
              if (pFeature)
                {
                  bResult = TRUE ;
                  zMapGFFParserIncrementNumFeatures(pParserBase) ;
                }
#ifdef CLIP_ALIGNMENT_ON_PARSE
            }
#endif

        }
      else if (cFeatureStyleMode == ZMAPSTYLE_MODE_ASSEMBLY_PATH)
        {

#ifdef CLIP_ASSEMBLYPATH_ON_PARSE
          if ((bIncludeFeature = clipFeatureLogic_General(pParser, pFeatureData )))
            {
#else
          if ((bIncludeFeature = clipFeatureLogic_Complete(pParser, pFeatureData )))
            {
#endif

              pFeature = makeFeatureAssemblyPath(pFeatureData, pFeatureSet, err_text) ;
              if (pFeature)
                {
                  bResult = TRUE ;
                  zMapGFFParserIncrementNumFeatures(pParserBase) ;
                }

            }


        }
      else
        {

#ifdef CLIP_DEFAULT_ON_PARSE
          if ((bIncludeFeature = clipFeatureLogic_General(pParser, pFeatureData )))
            {
#else
          if ((bIncludeFeature = clipFeatureLogic_Complete(pParser, pFeatureData )))
            {
#endif

              pFeature = makeFeatureDefault(pFeatureData, pFeatureSet, err_text) ;
              if (pFeature)
                {
                  bResult = TRUE ;
                  zMapGFFParserIncrementNumFeatures(pParserBase) ;
                }

            }


        } /* final ZMapStyleMode clause */

      /*
       * Now we deal with some extra attributes used locally.
       */
      if (bIncludeFeature)
        {

          if(!zMapStyleGetGFFFeature(pFeatureSet->style))
            zMapStyleSetGFF(pFeatureSet->style, NULL, (char*)sSOType);

          /*
           * URL attribute.
           */
          if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_url)))
            {
              if (zMapAttParseURL(pAttribute, &sURL))
                {
                  if (*sURL)
                    {
                      zMapFeatureAddURL(pFeature, sURL) ;
                    }
                }
            }

          /*
           * EnsEMBL variation data.
           */
          if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_ensembl_variation)))
            {
              if (zMapAttParseEnsemblVariation(pAttribute, &sVariation))
                {
                  if (*sVariation)
                    {
                      zMapFeatureAddVariationString(pFeature, sVariation) ;
                    }
                }
            }

          /*
           * Handle the Note attribute
           */
          if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_Note)))
            {
              if (zMapAttParseNote(pAttribute, &sNote))
                {
                  if (*sNote)
                    {
                      zMapFeatureAddDescription(pFeature, sNote) ;
                    }
                }
            }

          /*
           * Insert handling of other attributes in here as necesary.
           */

        }


    } /* if (bFoundFeatureset && bResult) */

  /*
   *  Clean up
   */
  if (sURL)
    g_free(sURL) ;
  if (sNote)
    g_free(sNote) ;

  return bResult ;
}



/*
 * Perform all of the computations/lookups for dealing with source datasets and parser featuresets.
 * We also generate identifiers for the source and feature style.
 *
 * Return a boolean to indicate success or failure.
 */
static gboolean findFeatureset(ZMapGFFParser pParser, ZMapGFFFeatureData pFeatureData,
                                   ZMapFeatureSet *ppFeatureSet)
{
  gboolean bResult = TRUE ;
  ZMapFeatureSource pSourceData = NULL ;
  GQuark gqSourceID = 0, gqFeatureStyleID = 0 ;
  char *sSource = NULL ;
  unsigned int nAttributes = 0 ;
  ZMapGFFAttribute *pAttributes = NULL ;
  ZMapFeatureSet pFeatureSet = NULL ;
  ZMapGFFParserFeatureSet pParserFeatureSet = NULL ;
  ZMapFeatureTypeStyle pFeatureStyle = NULL ;
  ZMapSOIDData pSOIDData = NULL ;
  ZMapStyleMode cFeatureStyleMode = ZMAPSTYLE_MODE_INVALID ;

  zMapReturnValIfFail(pParser && pFeatureData && ppFeatureSet, FALSE ) ;

  /*
   * Firstly get the attributes.
   */
  sSource              = zMapGFFFeatureDataGetSou(pFeatureData) ;
  nAttributes          = zMapGFFFeatureDataGetNat(pFeatureData) ;
  pAttributes          = zMapGFFFeatureDataGetAts(pFeatureData) ;
  pSOIDData            = zMapGFFFeatureDataGetSod(pFeatureData) ;
  cFeatureStyleMode    = zMapSOIDDataGetStyleMode(pSOIDData) ;

  /*
   * Now deal with the source -> data mapping referred to in the parser.
   */
  if (pParser->source_2_sourcedata)
    {
      gqSourceID = zMapFeatureSetCreateID(sSource);

      if (!(pSourceData = (ZMapFeatureSource) g_hash_table_lookup(pParser->source_2_sourcedata, GINT_TO_POINTER(gqSourceID))))
        {
          pSourceData = g_new0(ZMapFeatureSourceStruct,1);
          pSourceData->source_id = gqSourceID;
          pSourceData->source_text = gqSourceID;

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
  if (!(pParserFeatureSet = getParserFeatureSet(pParser, sSource)))
    {
      bResult = FALSE ;
    }

  /*
   * Now deal with the feature set and parser featureset.
   */
  if (bResult)
    {

      if (!(pFeatureStyle = (ZMapFeatureTypeStyle)g_hash_table_lookup(pParserFeatureSet->feature_styles, GUINT_TO_POINTER(gqFeatureStyleID))))
        {
          if (!(pFeatureStyle = zMapFindFeatureStyle(pParser->sources, gqFeatureStyleID, cFeatureStyleMode)))
            {
              gqFeatureStyleID = g_quark_from_string(zMapStyleMode2ShortText(cFeatureStyleMode)) ;
            }

          if (!(pFeatureStyle = zMapFindFeatureStyle(pParser->sources, gqFeatureStyleID, cFeatureStyleMode)))
            bResult = FALSE ;

          if (bResult)
            {
              if (pSourceData)
                pSourceData->style_id = gqFeatureStyleID;

              g_hash_table_insert(pParserFeatureSet->feature_styles,GUINT_TO_POINTER(gqFeatureStyleID),(gpointer) pFeatureStyle);

              if (pSourceData && pFeatureStyle->unique_id != gqFeatureStyleID)
                pSourceData->style_id = pFeatureStyle->unique_id;

            }
        }

    }

  if (bResult)
    {
      pParserFeatureSet->feature_set->style = pFeatureStyle;
      pFeatureSet = pParserFeatureSet->feature_set ;
    }

  *ppFeatureSet = pFeatureSet ;

  return bResult ;
}





/*
 * Decide whether or not locus-based operations are required for
 * the current feature.
 */
static gboolean requireLocusOperations(ZMapGFFParser pParser, ZMapGFFFeatureData pFeatureData)
{
  gboolean bResult = FALSE ;
  ZMapGFFAttribute *pAttributes = NULL ;
  unsigned int nAttributes = 0 ;

  zMapReturnValIfFail(pParser && pFeatureData, bResult ) ;

  nAttributes          = zMapGFFFeatureDataGetNat(pFeatureData) ;
  pAttributes          = zMapGFFFeatureDataGetAts(pFeatureData) ;

  if (pParser->locus_set_id                                                                           &&
      zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_locus)                    &&
      zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_ID)                       &&
      !strcmp( zMapSOIDDataGetName(zMapGFFFeatureDataGetSod(pFeatureData)) , "transcript") )
    {
      bResult = TRUE ;
    }

  return bResult ;
}
















/*
 * Sets the SO collection flag. TRUE means use SO, FALSE means use SOFA.
 */
gboolean zMapGFFSetSOSetInUse(ZMapGFFParser  pParserBase, ZMapSOSetInUse cUse)
{
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  gboolean bSet = FALSE ;

  zMapReturnValIfFail(pParser && pParser->pHeader, bSet) ;

  pParser->cSOSetInUse = cUse ;
  bSet = TRUE ;
  return bSet ;
}

/*
 * Returns the SO collection flag.
 */
ZMapSOSetInUse zMapGFFGetSOSetInUse(ZMapGFFParser pParserBase )
{
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  zMapReturnValIfFail(pParser && pParser->pHeader, ZMAPSO_USE_NONE) ;

  return pParser->cSOSetInUse ;
}




/*
 * Set the SO Error level within the parser.
 */
gboolean zMapGFFSetSOErrorLevel(ZMapGFFParser pParserBase, ZMapSOErrorLevel cErrorLevel)
{
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  gboolean bSet = FALSE ;

  zMapReturnValIfFail(pParser && pParser->pHeader, bSet) ;

  bSet = TRUE ;
  pParser->cSOErrorLevel = cErrorLevel ;
  return bSet ;
}


/*
 * Return the error level setting within the parser.
 */
ZMapSOErrorLevel zMapGFFGetSOErrorLevel(ZMapGFFParser pParserBase )
{
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  zMapReturnValIfFail(pParser && pParser->pHeader, ZMAPGFF_SOERRORLEVEL_UNK);
  return pParser->cSOErrorLevel ;
}






/* This is a GDestroyNotify() and is called for each element in a GData list when
 * the list is cleared with g_datalist_clear (), the function must free the GArray
 * and GData lists. */
static void destroyFeatureArray(gpointer data)
{
  ZMapGFFParserFeatureSet parser_feature_set = (ZMapGFFParserFeatureSet)data ;
  zMapReturnIfFail(parser_feature_set) ;

  /* No data to free in this list, just clear it. */
  g_datalist_clear(&(parser_feature_set->multiline_features)) ;
  if(parser_feature_set->feature_set && parser_feature_set->feature_set->style)
      zMapStyleDestroy(parser_feature_set->feature_set->style);

  g_hash_table_destroy(parser_feature_set->feature_styles);

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

  zMapReturnValIfFail(pParser && pParser->pHeader && sFeatureSetName && *sFeatureSetName, pParserFeatureSet) ;

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

      pParser->src_feature_sets =g_list_prepend(pParser->src_feature_sets, GUINT_TO_POINTER(pFeatureSet->unique_id));

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
 *        B0250GF_splicesplice31061070.233743+.
 *        B0250GF_ATGatg38985389871.8345-0
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
  ZMapGFFAttribute * pAttributes,
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
  gboolean bHasName = FALSE,
    bAttributeHit = FALSE,
    bParseValid = FALSE ;
  char *sTempBuff01 = NULL,
  *sTempBuff02 = NULL ;
  unsigned int iAttribute = 0 ;

  zMapReturnValIfFail(sequence && *sequence && source && *source, bHasName) ;

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
  else if (pAttributes && (pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Name")))
    {
      bParseValid = zMapAttParseName(pAttribute, &*feature_name) ;
      *feature_name_id = zMapFeatureCreateName(feature_type, *feature_name, strand,
                                               start, end, query_start, query_end) ;
      bHasName = TRUE ;

    }
  else if (name_find != ZMAPGFF_NAME_USE_GIVEN_OR_NAME)
    {


      /* for BAM we have a basic feature with name so let's bodge this in */
      if (feature_type == ZMAPSTYLE_MODE_ALIGNMENT)
        {

          if (pAttributes && (pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Target")))
            {

                /* In acedb output at least, homologies all have the same format.
               * Strictly speaking this is V2 only, as target has a different meaning
               * in V3.
               */
              if (pAttributes && zMapAttParseTargetV2(pAttribute, &sTempBuff01, &sTempBuff02))
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
           ;
          if (pAttributes && (pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, "Assembly_source")) )
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
          if (pAttributes)
            {
              for (iAttribute=0; iAttribute<nAttributes; ++iAttribute)
                {
                  pAttribute = pAttributes[iAttribute] ;
                  if (zMapAttParseAnyTwoStrings(pAttribute, &sTempBuff01, &sTempBuff02) )
                    {
                      bAttributeHit = TRUE;
                      break ;
                    }
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


/*
 * Lookup the value of the ID attribute, and return the GQuark derived from it.
 * A return value of zero indicates failure (either there was no ID attribute or
 * it has NULL value).
 */
static GQuark compositeFeaturesCreateID(ZMapGFFAttribute *pAttributes, unsigned int nAttributes )
{
  GQuark gqResult = 0 ;
  ZMapGFFAttribute pAttribute = NULL ;

  if ((pAttribute = zMapGFFAttributeListContains(pAttributes, nAttributes, sAttributeName_ID)))
    {
      gqResult = g_quark_from_string(zMapGFFAttributeGetTempstring(pAttribute)) ;
    }

  return gqResult ;
}

/*
 * Lookup a feature that has a GQuark value feature_id derived from its ID attribute
 * value string. That is, feature_id = g_quark_to_string(<string>), where ID=<string>
 * in the feature attributes.
 */
static GQuark compositeFeaturesFind(ZMapGFF3Parser const pParser, GQuark feature_id )
{
  GQuark gqResult = 0 ;
  gpointer pValue = NULL ;
  zMapReturnValIfFail(pParser && pParser->composite_features && feature_id, gqResult ) ;

  if ((pValue = g_hash_table_lookup(pParser->composite_features, GINT_TO_POINTER(feature_id))))
    {
      gqResult = GPOINTER_TO_INT(pValue) ;
    }

  return gqResult ;
}

/*
 * Insert a (ID-quark, feature->unique_id) pair into the composite_features hash table.
 */
static gboolean compositeFeaturesInsert(ZMapGFF3Parser const pParser, GQuark feature_id, GQuark feature_unique_id )
{
  gboolean bResult = FALSE ;

  zMapReturnValIfFail(pParser && pParser->composite_features, bResult ) ;

  if (feature_id && feature_unique_id)
    {
      g_hash_table_insert(pParser->composite_features,
                          GINT_TO_POINTER(feature_id),
                          GINT_TO_POINTER(feature_unique_id)) ;
      bResult = TRUE ;
    }

  return bResult ;
}














































