/*  File: zmapGFFAttribute.c
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
 * Description: Attribute storage and data manipulation for GFFv3.
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapSO.h>
#include <ZMap/zmapGFF.h>
#include <ZMap/zmapGFFStringUtils.h>

#include "zmapGFF_P.h"
#include "zmapGFF3_P.h"

#define ATTRIBUTE_DEFINITIONS 1
#include "zmapGFFAttribute.h"
#undef ATTRIBUTE_DEFINITIONS




/*
 * Escaped strings that need to be removed from attribute strings.
 * These are the url-escaped sequences, and are the only ones allowed
 * GFF3 attributes.
 */
static const char *sEscapedComma = "%2C" ;
static const char *sEscapedEquals = "%3D" ;
static const char *sEscapedSemicolon = "%3B" ;
static const char *sEscapedSpace = "%20" ;

static const char *sComma = "," ;
static const char *sEquals = "=" ;
static const char *sSemicolon = ";" ;
static const char *sSpace = " " ;

static char* removeAllEscapedCharacters(const char * const sInput ) ;


/*
 * Return the name stored as a string (not necessarily the same as the
 * name from the above lookups).
 */
char * zMapGFFAttributeGetNamestring(ZMapGFFAttribute pAttribute)
{
  if (!pAttribute)
    return NULL ;
  return pAttribute->sName ;
}

/*
 * Return the temp string stored internally.
 */
char * zMapGFFAttributeGetTempstring(ZMapGFFAttribute pAttribute)
{
  if (!pAttribute)
    return NULL ;
  return pAttribute->sTemp ;
}




/*
 * Create a new attribute object of a given type
 */
ZMapGFFAttribute zMapGFFCreateAttribute()
{
  ZMapGFFAttribute pAttribute = NULL ;

  /*
   * Create attribute object and set name.
   */
  pAttribute = (ZMapGFFAttribute) g_malloc(sizeof(ZMapGFFAttributeStruct));

  /*
   * Set name and temp strings.
   */
  pAttribute->sName = NULL ;
  pAttribute->sTemp = NULL ;

  return pAttribute ;
}

/*
 * Destroy an attribute object.
 */
gboolean zMapGFFDestroyAttribute(ZMapGFFAttribute pAttribute)
{
  gboolean bResult = TRUE ;
 if (!pAttribute)
  return bResult ;

  /*
   * Free name and temp strings if allocated.
   */
  if (pAttribute->sName != NULL)
    g_free(pAttribute->sName) ;
  if (pAttribute->sTemp != NULL)
    g_free(pAttribute->sTemp) ;

  /*
   * Free the top-level struct.
   */
  g_free(pAttribute) ;

  return bResult ;
}



/*
 * Parse and create an attribute from the input string.
 *
 * Input data must have the form:
 * <sAttribute> ::= <name> <sDelim> <data>
 *
 * Where the delimiter is assumed to be version dependent and stored in
 * the parser struct. The delimiter can be missing and the data string
 * can be empty, in which case the name only is stored.
 *
 *
 * Note that the data part of the string is often quoted and there is a
 * flag say whether or not to attempt to remove leading or trailing
 * quotes from the string that is parsed out.
 */
ZMapGFFAttribute zMapGFFAttributeParse(ZMapGFFParser pParserBase, const char*  const sAttribute, gboolean bRemoveQuotes)
{
  static const unsigned int iExpectedTokens = 2 ,
    iTokenLimit = 2 ;
  static const gboolean bIncludeEmpty = TRUE ;
  unsigned int iNumTokens = 0, i;
  char **sTokens = NULL, *sBuff = NULL ;
  ZMapGFFVersion eVersion = ZMAPGFF_VERSION_UNKNOWN ;
  ZMapGFFAttribute pAttribute = NULL ;

  /*
   * Error tests.
   */
  if (!pParserBase)
    return pAttribute ;
  if (!sAttribute || !*sAttribute)
    return pAttribute ;
  eVersion = pParserBase->gff_version ;
  if (eVersion != ZMAPGFF_VERSION_3)
    return pAttribute ;

  /*
   * Cast to "derived" type.
   */
  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;
  sBuff = pParserBase->buffers[ZMAPGFF_BUF_TMP] ;

  /*
   * Tokenize the input string; we stop on encountering the first
   * delimiter and therefore should only get two tokens. The delimiter here
   * separates the attribute name from its associated data.
   */
  sTokens = zMapGFFStringUtilsTokenizer(pParser->cDelimAttValue, sAttribute, &iNumTokens,
                                 bIncludeEmpty, iTokenLimit, g_malloc, g_free, sBuff) ;
  if (iNumTokens > iExpectedTokens )
    {
      for (i=0; i<iNumTokens; ++i)
        if (sTokens[i] != NULL)
          g_free(sTokens[i]) ;
      return pAttribute ;
    }

  /*
   * Now create the attribute object.
   */
  pAttribute = zMapGFFCreateAttribute() ;

  /*
   * Now copy the name string.
   */
  pAttribute->sName = (char*) g_malloc(strlen(sTokens[0])+1) ;
  strcpy(pAttribute->sName, sTokens[0]) ;

  /*
   * For the moment we just copy over the data segment of
   * the input string.
   */
  if (iNumTokens == 1)
    {
      pAttribute->sTemp = NULL ;
    }
  else if (iNumTokens == iExpectedTokens )
    {
      pAttribute->sTemp = (char*) g_malloc(strlen(sTokens[1])+1) ;
      strcpy(pAttribute->sTemp, sTokens[1]) ;
      if (bRemoveQuotes)
        zMapGFFAttributeRemoveQuotes(pAttribute) ;
    }

  /*
   * Delete the token array now that we're done.
   */
  zMapGFFStringUtilsArrayDelete(sTokens, iNumTokens, g_free) ;

  return pAttribute ;
}


/*
 * Remove quotes from the tempstring data member of the attribute.
 *
 * Note that this ONLY removes quoes if they are simultaneously present at
 * the start and end of the attribute's tempstring. That is, it will remove
 * quotes from
 *                 '"<string>"'
 * but not from
 *                 '"<string>'   or    '<s1> "<s2>" <s3>'
 * etc.
 */
gboolean zMapGFFAttributeRemoveQuotes(ZMapGFFAttribute pAttribute )
{
  static const char cQuote = '"';
  unsigned int iLength = strlen(pAttribute->sTemp) ;
  char *sTemp = pAttribute->sTemp ;
  if ((iLength > 1) && pAttribute->sTemp[0] == cQuote && (pAttribute->sTemp[iLength-1] == cQuote))
    {
      while (*(sTemp+1))
        {
          *sTemp = *(sTemp+1) ;
          ++sTemp ;
        }
      pAttribute->sTemp[iLength-2] = '\0' ;
    }
  return TRUE ;
}




/*
 * Parse multiple attributes from input string. Calls the above parse function
 * for each attribute found.
 */
ZMapGFFAttribute* zMapGFFAttributeParseList(ZMapGFFParser pParserBase, const char * const sAttributes,
                                            unsigned int * const pnAttributes, gboolean bRemoveQuotes)
{
  unsigned int i = 0 ;
  char **sTokens ;
  gboolean bIncludeEmpty = FALSE,
    bFail = FALSE ;
  ZMapGFFAttribute *pAttributes = NULL ;
  ZMapGFFVersion eVersion = ZMAPGFF_VERSION_UNKNOWN ;

  /*
   * Some error tests.
   */
  if (!pParserBase)
    return pAttributes ;
  if (!sAttributes || !*sAttributes)
    return pAttributes ;
  if (!strlen(sAttributes))
    return pAttributes ;
  eVersion = pParserBase->gff_version ;
  if (eVersion != ZMAPGFF_VERSION_3)
    return pAttributes ;

  ZMapGFF3Parser pParser = (ZMapGFF3Parser) pParserBase ;

  /*
   * Initialize number of attributes found to 0.
   */
  *pnAttributes = 0 ;

  /*
   * Tokenize attribute string; we do have to worry about allowing quoted delimited
   * characters in this process.
   */
  bIncludeEmpty = FALSE ;
  sTokens = zMapGFFStringUtilsTokenizer02(pParser->cDelimAttributes, pParser->cDelimQuote, sAttributes, pnAttributes,
                                   bIncludeEmpty, g_malloc, g_free) ;
  if (*pnAttributes == 0)
    return pAttributes ;

  /*
   * Allocate and initialize array of attributes (well, pointers to attribute).
   */
  pAttributes = (ZMapGFFAttribute*) g_malloc(sizeof(ZMapGFFAttribute)* *pnAttributes) ;
  for (i=0; i<*pnAttributes; ++i)
    pAttributes[i] = NULL ;

  /*
   * Parse each of these attributes.
   */
  for (i=0; i<*pnAttributes; ++i)
    {
      pAttributes[i] = zMapGFFAttributeParse(pParserBase, sTokens[i], bRemoveQuotes) ;
      if (pAttributes[i] == NULL)
        {
          bFail = TRUE ;
          break ;
        }
    }

  /*
   * If one of these fails, then destroy all attribute objects,
   * set the number of attributes found to 0, and return NULL.
   */
  if (bFail)
    {
      for (i=0; i<*pnAttributes; ++i)
        if (pAttributes[i] != NULL)
          zMapGFFDestroyAttribute(pAttributes[i]) ;
      g_free(pAttributes) ;
      *pnAttributes = 0 ;
      pAttributes = NULL ;
    }

  /*
   * Destroy array of token strings.
   */
  zMapGFFStringUtilsArrayDelete(sTokens, *pnAttributes, g_free) ;

  return pAttributes ;
}



/*
 * Destroy a list of attributes as created above.
 */
gboolean zMapGFFAttributeDestroyList(ZMapGFFAttribute* pAttributes, unsigned int nAttributes)
{
  gboolean bResult = FALSE ;
  unsigned int iAtt;
  if (!pAttributes || !nAttributes )
    return bResult ;
  for (iAtt=0; iAtt<nAttributes; ++iAtt)
    if (pAttributes[iAtt] != NULL)
      zMapGFFDestroyAttribute(pAttributes[iAtt]) ;
  g_free(pAttributes) ;
  bResult = TRUE ;
  return bResult ;
}


/*
 * Queries a list of attributes to see if it contains one with the name passed in.
 * Returns a pointer to it or NULL if not found (or some other error).
 */
ZMapGFFAttribute zMapGFFAttributeListContains( ZMapGFFAttribute* pAtt, unsigned int nAttributes, const char * const sName)
{
  ZMapGFFAttribute pAttribute = NULL ;
  const ZMapGFFAttribute *pAttributes = pAtt ;
  unsigned int iAtt ;
  if (!pAttributes || !nAttributes)
    return pAttribute ;
  zMapReturnValIfFail(sName && *sName, pAttribute) ;

  for (iAtt=0; iAtt<nAttributes; ++iAtt)
    {
      if (!strcmp(zMapGFFAttributeGetNamestring(pAttributes[iAtt]), sName))
        {
          pAttribute = pAttributes[iAtt] ;
          break ;
        }
    }

  return pAttribute ;
}












/*
 * Inspect the "Class" attribute to convert to a homology type.
 */
gboolean zMapAttParseClass(ZMapGFFAttribute pAttribute, ZMapHomolType * const pcReturnValue)
{
  static const char* sMyName = "zMapAttParseClass()" ;
  gboolean bResult = FALSE ;
  *pcReturnValue = ZMAPHOMOL_NONE ;
  if (!pAttribute)
    {
      return bResult ;
    }
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Class, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s: %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  /*
   * Now extract data from the attribute value string
   */
  if (g_ascii_strncasecmp(sValue, "Sequence", 8) == 0)
    *pcReturnValue = ZMAPHOMOL_N_HOMOL ;
  else if (g_ascii_strncasecmp(sValue, "Protein", 7) == 0)
    *pcReturnValue = ZMAPHOMOL_X_HOMOL ;
  else if (g_ascii_strncasecmp(sValue, "Motif", 5) == 0)
    *pcReturnValue = ZMAPHOMOL_N_HOMOL ;
  else if (g_ascii_strncasecmp(sValue, "Mass_spec_peptide", 5) == 0)
    *pcReturnValue = ZMAPHOMOL_X_HOMOL ;
  else if (g_ascii_strncasecmp(sValue, "SAGE_tag", 5) == 0)
    *pcReturnValue = ZMAPHOMOL_N_HOMOL ;

  return TRUE ;
}



/*
 * Inspect the "percentID" attribute to convert to double.
 */
gboolean zMapAttParsePID(ZMapGFFAttribute pAttribute, double * const pdReturnValue)
{
  static const char* sMyName = "zMapAttParsePID()" ;
  static const unsigned int iExpectedFields = 1 ;
  static const char *sFormat = "%lg" ;
  gboolean bResult = FALSE ;
  *pdReturnValue = 0.0 ;
  if (!pAttribute || !pdReturnValue )
    {
      return bResult ;
    }
  const char* const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_percentID, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s: %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }
  if (sscanf(sValue, sFormat, pdReturnValue) == iExpectedFields )
    {
      bResult = TRUE ;
    }
  else
    {
      zMapLogWarning("Unable to parse in %s: %s %s ", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
    }

  return bResult ;
}



/*
 * Inspect the "Align" attribute to convert to two integer values, and a char.
 */
gboolean zMapAttParseAlign(ZMapGFFAttribute pAttribute, int * const piStart, int * const piEnd, ZMapStrand * const pStrand)
{
  static const char *sMyName = "zMapAttParseAlign()" ;
  static const unsigned int iExpectedFields = 3 ;
  static const char *sFormat = "%d%d %c" ;
  char cStrand = '\0' ;
  int iTemp ;
  gboolean bResult = FALSE ;
  if (!pAttribute || !piStart || !piEnd || !pStrand )
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Align, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  /*
   * Parse "Align 157 197 +"
   */
  if (sscanf(sValue, sFormat, piStart, piEnd, &cStrand) == iExpectedFields)
    {
        /* Always export as start < end */
        if (*piStart <= *piEnd)
          {

          }
        else
          {
            iTemp = *piEnd ;
            *piEnd = *piStart ;
            *piStart = iTemp ;
          }

        if (cStrand == '+')
          *pStrand = ZMAPSTRAND_FORWARD ;
        else
          *pStrand = ZMAPSTRAND_REVERSE ;

        bResult = TRUE ;
      }
    else
      {
        zMapLogWarning("Unable to parse in %s: %s %s ", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      }

  return bResult ;
}


/*
 * Parse the "start_not_found" attribute. We are just after an integer value; but it must be
 * within a certain numerical range.
 */
gboolean zMapAttParseCDSStartNotFound(ZMapGFFAttribute pAttribute, int * const piOut)
{
  static const char *sMyName = "zMapAttParseCDSStartNotFound()" ;
  static const unsigned int iExpectedFields = 1;
  static const char *sFormat = "%d %*s" ;
  int iTemp = 0 ;
  gboolean bResult = FALSE ;
  if (!pAttribute )
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_start_not_found, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (sscanf(sValue, sFormat, &iTemp) == iExpectedFields)
    {
      if (iTemp >= 1 && iTemp <= 3)
        {
          *piOut = iTemp ;
          bResult = TRUE ;
        }
    }

  return bResult ;
}

/*
 * Parse the "end_not_found" attribute. All we have to do here is check that the type of the attribute
 * is correct.
 */
gboolean zMapAttParseCDSEndNotFound(ZMapGFFAttribute pAttribute)
{
  static const char *sMyName = "zMapAttParseCDSEndNotFound()" ;
  gboolean bResult = FALSE ;
  if (!pAttribute )
    return bResult ;
  if (strcmp(sAttributeName_end_not_found, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute)) ;
      return bResult ;
    }

  bResult = TRUE ;

  return bResult ;
}




/*
 * Inspect the "length" attribute for a single integer variable. This is for V3 only; the
 * attribute name "length" was previous capitalised in our V2 code.
 */
gboolean zMapAttParseLength(ZMapGFFAttribute pAttribute , int* const piLength )
{
  static const char *sMyName = "zMapAttParseLength()" ;
  static const unsigned int iExpectedFields = 1;
  static const char *sFormat = "%d" ;
  gboolean bResult = FALSE ;
  int iLength = 0 ;
  if (!pAttribute || !piLength )
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_length, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (sscanf(sValue, sFormat, &iLength) == iExpectedFields )
    {
      *piLength = iLength ;
      bResult = TRUE ;
    }


  return bResult ;
}






/*
 * Parse a "Name" attribute for variation data. This was what we previously did in V2. There
 * is another version of ParseNameV3 for the new version.
 */
gboolean zMapAttParseNameV2(ZMapGFFAttribute pAttribute, GQuark *const SO_acc_out, char ** const name_str_out, char ** const variation_str_out)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseNameV2()" ;
  static const unsigned int iExpectedFields = 2 ;
  static const char *sFormat = "%s - %50[^\"]%*s" ;
  char name_str[ZMAPGFF_MAX_FIELD_CHARS + 1] = "" ;
  char variation_str[ZMAPGFF_MAX_FIELD_CHARS + 1] = "" ;
  GQuark SO_acc = 0 ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Name, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (sscanf(sValue, sFormat, name_str, variation_str) == iExpectedFields)
    {
      if ((SO_acc = zMapSOVariation2SO(variation_str)))
        {
          *SO_acc_out = SO_acc ;
          *name_str_out = g_strdup(name_str) ;
          *variation_str_out = g_strdup(variation_str) ;
          bResult = TRUE ;
        }
    }

  return bResult ;
}



/*
 * V3 "Name" parser. This just returns a copy of the string, having removed escaped equals and
 * escaped commas if any are present.
 */
gboolean zMapAttParseName(ZMapGFFAttribute pAttribute, char** const psOut)
{
  gboolean bReplaced = FALSE, bResult = FALSE ;
  static const char *sMyName = "zMapAttParseName()" ;
  char *sTemp = NULL ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Name, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (strlen(sValue))
    {
      bReplaced = zMapGFFStringUtilsSubstringReplace(sValue, sEscapedEquals, sEquals, &sTemp) ;
      bReplaced = zMapGFFStringUtilsSubstringReplace(sTemp, sEscapedComma, sComma, psOut) ;
      bResult = TRUE ;
    }
  else
    {
      *psOut = NULL ;
    }

  if (sTemp)
    {
      g_free(sTemp) ;
    }

  return bResult ;
}

/*
 * Parse the "Alias" attribute; just returns a copy of the string.
 */
gboolean zMapAttParseAlias(ZMapGFFAttribute pAttribute, char ** const sOut)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseAlias()" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Alias, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (strlen(sValue))
    *sOut = g_strdup(sValue) ;
  else
    *sOut = NULL ;
  bResult = TRUE ;

  return bResult ;

}



/*
 * Parse "Target" attribute, as defined in V3. We expect data of the form:
 *
 *                target_id start end [strand]
 *
 * in the value string of the attribute. The optional [strand] datum is
 * allowed to be '+' or '-' but this function converts to ZMapStrand.
 * The 'start' and 'end' data are assumed to be unsigned. At least one
 * space is required between <end> and <strand> if the latter is present.
 */
gboolean zMapAttParseTarget(ZMapGFFAttribute pAttribute, GQuark * const pgqOut, int * const piStart, int * const piEnd, ZMapStrand * const pStrand)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseTarget()" ;
  static const char *sFormat = "%s%u%u%*[ ]%c";
  static const char cPlus = '+', cMinus = '-' ;
  static int iRequiredFields = 3 ;
  char sStringBuff[ZMAPGFF_MAX_FIELD_CHARS + 1] = "" ;
  int iFields = 0, iStart = 0, iEnd = 0;
  char cStrand = '\0' ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Target, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if ((iFields = sscanf(sValue, sFormat, sStringBuff, &iStart, &iEnd, &cStrand)) >= iRequiredFields)
    {
      if ((iStart <= iEnd) && strlen(sStringBuff))
        {

          if (iFields == iRequiredFields) /* we did not have strand data */
            {
              *pStrand = ZMAPSTRAND_NONE ;
              bResult = TRUE ;
            }
          else if (iFields == iRequiredFields+1) /* we did find strand data */
            {
              if (cStrand == cPlus)
                {
                  *pStrand = ZMAPSTRAND_FORWARD ;
                  bResult = TRUE ;
                }
              else if (cStrand == cMinus)
                {
                  *pStrand = ZMAPSTRAND_REVERSE ;
                  bResult = TRUE ;
                }
              else
                {
                  /* strand data was not valid */
                }
            }

          if (bResult)
            {
              *pgqOut = g_quark_from_string(sStringBuff) ;
              *piStart = iStart ;
              *piEnd = iEnd ;
            }

        }
    }


  return bResult ;
}


/*
 * Parse the "Dbxref" attribute. We expect a colon seperated pair of strings. The second
 * may contain colons so we split on the first one encountered. If one is not encountered
 * at all, this is an error.
 */
gboolean zMapAttParseDbxref(ZMapGFFAttribute pAttribute, char ** const sOut1, char ** const sOut2)
{
  gboolean bResult = FALSE,
    bFoundSplit = FALSE ;
  static const char *sMyName = "zMapAttParseDbxref()" ;
  static const char cColon = ':' ;
  char *s1, *s2 ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  const char * const sEnd = sValue + strlen(sValue) ;
  const char * pC = sValue ;
  if (strcmp(sAttributeName_Dbxref, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  /*
   * Find split point.
   */
  while (*pC)
    {
      if (*pC == cColon)
        {
          bFoundSplit = TRUE ;
          break ;
        }
      ++pC;
    }
  if (!bFoundSplit)
    return bResult ;

  /*
   * Find substrings
   */
  s1 = zMapGFFStringUtilsSubstring(sValue, pC, g_malloc) ;
  s2 = zMapGFFStringUtilsSubstring(pC+1, sEnd, g_malloc) ;

  /*
   * If they are both non-empty then copy to output
   * and we are done.
   */
  if (s1 && s2 && strlen(s1) && strlen(s2))
    {
      *sOut1 = s1 ;
      *sOut2 = s2 ;
      bResult = TRUE ;
    }

  return bResult ;
}




/*
 * This parses the "Ontology_term" attribute; is the same as Dbxref other than the name.
 */
gboolean zMapAttParseOntology_term(ZMapGFFAttribute pAttribute , char ** const sOut1, char ** const sOut2 )
{
  gboolean bResult = FALSE,
    bFoundSplit = FALSE ;
  static const char *sMyName = "zMapAttParseOntology_term()" ;
  static const char cColon = ':' ;
  char *s1, *s2 ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  const char * const sEnd = sValue + strlen(sValue) ;
  const char * pC = sValue ;
  if (strcmp(sAttributeName_Ontology_term, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  /*
   * Find split point.
   */
  while (*pC)
    {
      if (*pC == cColon)
        {
          bFoundSplit = TRUE ;
          break ;
        }
      ++pC;
    }
  if (!bFoundSplit)
    return bResult ;

  /*
   * Find substrings
   */
  s1 = zMapGFFStringUtilsSubstring(sValue, pC, g_malloc) ;
  s2 = zMapGFFStringUtilsSubstring(pC+1, sEnd, g_malloc) ;

  /*
   * If they are both non-empty then copy to output
   * and we are done.
   */
  if (s1 && s2 && strlen(s1) && strlen(s2))
    {
      *sOut1 = s1 ;
      *sOut2 = s2 ;
      bResult = TRUE ;
    }

  return bResult ;
}


/*
 * Parse the "Is_circular" attribute. Allowed values are "true" and "false" and are taken
 * to be case sensitive.
 */
gboolean zMapAttParseIs_circular(ZMapGFFAttribute pAttribute, gboolean * const pbOut)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseIs_circular()" ;
  static const char *sTrue = "true" ;
  static const char *sFalse = "false" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Is_circular, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  /*
   * Parse according to string comparisons.
   */
  if (!strcmp(sValue, sTrue))
    {
      *pbOut = TRUE ;
      bResult = TRUE ;
    }
  else if (!strcmp(sValue, sFalse))
    {
      *pbOut = FALSE ;
      bResult = TRUE ;
    }

  return bResult;
}



/*
 * Parse the "Parent" attribute; just returns a copy of the string.
 */
gboolean zMapAttParseParent(ZMapGFFAttribute pAttribute, char ** const sOut)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseParent()" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Parent, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (strlen(sValue))
    *sOut = g_strdup(sValue) ;
  else
    *sOut = NULL ;
  bResult = TRUE ;

  return bResult ;

}




/*
 * Parse the URL attribute; this means just returning a copy of the string copied in
 * as long as its not empty. No attempt is made in the old code to see whether or
 * not the string is actually a valid URL or not.
 */
gboolean zMapAttParseURL(ZMapGFFAttribute pAttribute, char** const sOut)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseURL()" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_url, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (strlen(sValue))
    {
      *sOut = removeAllEscapedCharacters(sValue) ;
      bResult = TRUE ;
    }
  else
    {
      *sOut = NULL ;
    }

  return bResult ;
}






/*
 * Parse out the ensembl_variation attribute.
 */
gboolean zMapAttParseEnsemblVariation(ZMapGFFAttribute pAttribute, char ** const psOut)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseEnsembleVariation()" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_ensembl_variation, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (strlen(sValue))
    *psOut = g_strdup(sValue) ;
  else
    *psOut = NULL ;
  bResult = TRUE ;

  return bResult ;
}

/*
 * Parse out the allele_string attribute.
 */
gboolean zMapAttParseAlleleString(ZMapGFFAttribute pAttribute, char ** const psOut)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseAlleleString()" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_allele_string, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (strlen(sValue))
    *psOut = g_strdup(sValue) ;
  else
    *psOut = NULL ;
  bResult = TRUE ;

  return bResult ;
}







/*
 * Parse "Note" attribute; this means just send back a copy of the value as a string.
 */
gboolean zMapAttParseNote(ZMapGFFAttribute pAttribute, char ** const sOut)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseNote()" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Note, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (strlen(sValue))
    {
      *sOut = removeAllEscapedCharacters(sValue) ;
      bResult = TRUE ;
    }
  else
    {
      *sOut = NULL ;
    }

  return bResult ;
}

/*
 * Parse "Derives_from" attribute; this means just send back a copy of the value as a string.
 */
gboolean zMapAttParseDerives_from(ZMapGFFAttribute pAttribute, char ** const sOut)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseDerives_from()" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Derives_from, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (strlen(sValue))
    *sOut = g_strdup(sValue) ;
  else
    *sOut = NULL ;
  bResult = TRUE ;

  return bResult ;
}



/*
 * Parse the "ID" attribute. Translate the associated string into a GQuark.
 */
gboolean zMapAttParseID(ZMapGFFAttribute pAttribute, GQuark * const pgqOut )
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseID()" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_ID, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (strlen(sValue))
    {
      *pgqOut = g_quark_from_string(sValue) ;
      if (*pgqOut)
        {
          bResult = TRUE ;
        }
    }

  return bResult ;
}



/*
 * Parse the "sequence" attribute; simply return a copy of the string data.
 */
gboolean zMapAttParseSequence(ZMapGFFAttribute pAttribute, char ** const sOut)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseSequence()" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_sequence, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (strlen(sValue))
    *sOut = g_strdup(sValue) ;
  else
    *sOut = NULL ;
  bResult = TRUE ;

  return bResult ;
}

/*
 * Parse "Source" attribute; this means just send back a copy of the value as a string.
 */
gboolean zMapAttParseSource(ZMapGFFAttribute pAttribute , char ** const sOut )
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseSource()" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Source, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (strlen(sValue))
    *sOut = g_strdup(sValue) ;
  else
    *sOut = NULL ;
  bResult = TRUE ;

  return bResult ;
}













/*
 * Parse out a "Target" attribute. This is modified from the original version that was
 * in 'getFeatureName()' to read both of the strings that are present.
 */
gboolean zMapAttParseTargetV2(ZMapGFFAttribute pAttribute, char ** const sOut01, char ** const sOut02)
{
  static const char *sFormat = "%*[\"]%*[^:]%*[:]%50[^\"]%*[\"]%s" ;
  static const unsigned int iExpectedFields = 2 ;
  static const char *sMyName = "zMapAttParseTargetV2()" ;
  gboolean bResult = FALSE ;
  char sString01[ZMAPGFF_MAX_FIELD_CHARS + 1] = "" ;
  char sString02[ZMAPGFF_MAX_FIELD_CHARS + 1] = "" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Target, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (sscanf(sValue, sFormat, sString01, sString02) == iExpectedFields)
    {
      *sOut01 = g_strdup(sString01) ;
      *sOut02 = g_strdup(sString02) ;
      bResult = TRUE ;
    }

  return bResult ;
}











/*
 * Parse out a "Assembly_source" attribute. This is modified from the original version that was
 * in 'getFeatureName()' to read both of the strings that are present. Basically the same as the
 * "Target" parser above but with different tag.
 */
gboolean zMapAttParseAssemblySource(ZMapGFFAttribute pAttribute, char ** const sOut01, char ** const sOut02)
{
  static const char *sFormat = "%*[\"]%*[^:]%*[:]%50[^\"]%*[\"]%s" ;
  static const unsigned int iExpectedFields = 2 ;
  static const char *sMyName = "zMapAttParseAssemblySource()" ;
  gboolean bResult = FALSE ;
  char sString01[ZMAPGFF_MAX_FIELD_CHARS + 1] = "" ;
  char sString02[ZMAPGFF_MAX_FIELD_CHARS + 1] = "" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Assembly_source, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (sscanf(sValue, sFormat, sString01, sString02) == iExpectedFields)
    {
      *sOut01 = g_strdup(sString01) ;
      *sOut02 = g_strdup(sString02) ;
      bResult = TRUE ;
    }

  return bResult ;
}

/*
 * Parse out any attribute tag followed by two strings, the first of which is quoted.
 */
gboolean zMapAttParseAnyTwoStrings(ZMapGFFAttribute pAttribute, char ** const sOut01, char ** const sOut02)
{
  static const char *sFormat = "%*[\"]%50[^\"]%*[\"]%*s";
  static const unsigned int iExpectedFields = 2 ;
  static const char *sMyName = "zMapAttParseAnyTwoStrings()" ;
  gboolean bResult = FALSE ;
  char sString01[ZMAPGFF_MAX_FIELD_CHARS + 1] = "" ;
  char sString02[ZMAPGFF_MAX_FIELD_CHARS + 1] = "" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (!strlen(zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute has empty tag in in %s, %s", sMyName, sValue) ;
      return bResult ;
    }

  if (sscanf(sValue, sFormat, sString01, sString02) == iExpectedFields)
    {
      *sOut01 = g_strdup(sString01) ;
      *sOut02 = g_strdup(sString02) ;
      bResult = TRUE ;
    }

  return bResult ;
}


/*
 * Parse "locus" attribute and convert to GQuark.
 */
gboolean zMapAttParseLocus(ZMapGFFAttribute pAttribute, GQuark * const pGQ)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseLocus()" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  GQuark cGQ = 0 ;
  if (strcmp(sAttributeName_locus, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (!strlen(sValue))
    cGQ = 0 ;
  else
    cGQ = g_quark_from_string(sValue) ;
  *pGQ = cGQ ;
  bResult = TRUE ;

  return bResult ;
}



/*
 * Parse data from the "gaps" attribute; we expect to find zero or more comma-seperated groups of 4 integer values.
 * The value string of the attribute should be unquoted.
 */
gboolean zMapAttParseGaps(ZMapGFFAttribute pAttribute, GArray ** const pGaps, ZMapStrand cRefStrand, ZMapStrand cMatchStrand)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseGaps()" ;
  static const char *sFormat = "%d%d%d%d" ;
  static const unsigned int iExpectedFields = 4 ;
  if (!pAttribute)
    return bResult ;
  const char *sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  ZMapAlignBlockStruct gap = { 0 };
  if (strcmp(sAttributeName_gaps, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (!sValue)
    return bResult ;

  while ( *sValue )
    {

      /* We should be looking at "number number number number , ....more stuff....." */
      if (sscanf(sValue, sFormat, &gap.t1, &gap.t2, &gap.q1, &gap.q2) == iExpectedFields)
        {
          if (gap.q1<1 || gap.q2<1 || gap.t1<1 || gap.t2<1 || gap.q1>gap.q2 || gap.t1>gap.t2)
            {
              bResult = FALSE ;
              break ;
            }
          else
            {
              gap.t_strand = cRefStrand ;
              gap.q_strand = cMatchStrand ;
              *pGaps = g_array_append_val(*pGaps, gap) ;
            }
        }
      else
        {
          /* anything other than 4 is not a gap */
          bResult = FALSE ;
          break ;
        }

      while (*sValue && *sValue != ',' )
        {
          sValue++ ;
        };
      if (*sValue && *sValue == ',')
        ++sValue ;

    }

  return bResult ;

}

/*
 * Parse data from the "Gap" attribute. This is the GFFv3 standard attribute, and is taken to be the same as
 * cigar_exonerate (according to http://www.sequenceontology.org/resources/gff3.html).
 */
gboolean zMapAttParseGap(ZMapGFFAttribute pAttribute , GArray ** const pGaps,
  ZMapStrand cRefStrand, int iRefStart, int iRefEnd, ZMapStrand cMatchStrand, int iMatchStart, int iMatchEnd)

{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseGap()" ;
  if (!pAttribute)
    return bResult ;
  char *sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Gap, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  bResult = zMapFeatureAlignmentString2Gaps(ZMAPALIGN_FORMAT_GAP_GFF3,
                                            cRefStrand, iRefStart, iRefEnd,
                                            cMatchStrand, iMatchStart, iMatchEnd,
                                            sValue, pGaps) ;

  return bResult ;

}





/*
 * This parses out "cigar_exonerate" attribute.
 *
 * Note that the functions that are called to do these ones (that is
 * zMapFeatureAlignmentString2Gaps()), are in the file zmapFeatureAlignment.c.
 * The final one (vulgar exonerate) is not yet implemented and so will always
 * return false.
 */
gboolean zMapAttParseCigarExonerate(ZMapGFFAttribute pAttribute , GArray ** const pGaps,
  ZMapStrand cRefStrand, int iRefStart, int iRefEnd, ZMapStrand cMatchStrand, int iMatchStart, int iMatchEnd)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseCigarExonerate()" ;

  if (!pAttribute)
    return bResult ;
  char *sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_cigar_exonerate, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  bResult = zMapFeatureAlignmentString2Gaps(ZMAPALIGN_FORMAT_CIGAR_EXONERATE,
                                            cRefStrand, iRefStart, iRefEnd,
                                            cMatchStrand, iMatchStart, iMatchEnd,
                                            sValue, pGaps) ;

  return bResult ;
}




/*
 * This parses the "cigar_ensemble" attribute.
 */
gboolean zMapAttParseCigarEnsembl(ZMapGFFAttribute pAttribute, GArray ** const pGaps ,
  ZMapStrand cRefStrand, int iRefStart, int iRefEnd, ZMapStrand cMatchStrand, int iMatchStart, int iMatchEnd)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseCigarEnsembl()" ;

  if (!pAttribute)
    return bResult ;
  char *sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
    if (strcmp(sAttributeName_cigar_ensembl, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  bResult = zMapFeatureAlignmentString2Gaps(ZMAPALIGN_FORMAT_CIGAR_ENSEMBL,
                                            cRefStrand, iRefStart, iRefEnd,
                                            cMatchStrand, iMatchStart, iMatchEnd,
                                            sValue, pGaps) ;

  return bResult ;
};



/*
 * This parses the "cigar_bam" attribute.
 */
gboolean zMapAttParseCigarBam(ZMapGFFAttribute pAttribute , GArray ** const pGaps,
  ZMapStrand cRefStrand, int iRefStart, int iRefEnd, ZMapStrand cMatchStrand, int iMatchStart, int iMatchEnd )
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseCigarBam()" ;

  if (!pAttribute)
    return bResult ;
  char *sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_cigar_bam, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  bResult = zMapFeatureAlignmentString2Gaps(ZMAPALIGN_FORMAT_CIGAR_BAM,
                                            cRefStrand, iRefStart, iRefEnd,
                                            cMatchStrand, iMatchStart, iMatchEnd,
                                            sValue, pGaps) ;

  return bResult ;
}




/*
 * This parses the "vulgar_exonerate" attribute. This one is not yet implemented.
 */
gboolean zMapAttParseVulgarExonerate(ZMapGFFAttribute pAttribute , GArray ** const pGaps ,
  ZMapStrand cRefStrand, int iRefStart, int iRefEnd, ZMapStrand cMatchStrand, int iMatchStart, int iMatchEnd)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseVulgarExonerate()" ;
  static const char *sFormat = "%*[\"]%500[^\"]%*[\"] %*s" ;
  static const unsigned int iExpectedFields = 1 ;
  char sStringBuff[ZMAPGFF_MAX_FIELD_CHARS + 1] = "" ;

  if (!pAttribute)
    return bResult ;
  const char *sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_vulgar_exonerate, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  /*
   * First step is to extract the quoted part of the value string of the attribute.
   */
  if (sscanf(sValue, sFormat, sStringBuff) == iExpectedFields)
    {
      bResult = zMapFeatureAlignmentString2Gaps(ZMAPALIGN_FORMAT_VULGAR_EXONERATE,
                                                cRefStrand, iRefStart, iRefEnd,
                                                cMatchStrand, iMatchStart, iMatchEnd,
                                                sStringBuff, pGaps) ;
    }

  return bResult ;
}



/*
 * Parse "Known_name" attribute; this means just send back a copy of the value as a string.
 */
gboolean zMapAttParseKnownName(ZMapGFFAttribute pAttribute, char ** const sOut)
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseKnownName()" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_Known_name, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (strlen(sValue))
    *sOut = g_strdup(sValue) ;
  else
    *sOut = NULL ;
  bResult = TRUE ;

  return bResult ;
}




/*
 * Parse out assembly path data. Format of this attribute is:
 * v2 data: Assembly_source "Sequence:B0250" ; Assembly_strand + ; Assembly_length 39216 ; Assembly_regions 1 39110 [, start end]+ ;
 * v3 data: <ap_source>, <strand>, <length>, <rstart> <rend> [, <rstart>, <rend>]*
 *
 * Where <ap_source>               (unquoted) string
 *       <strand>                  +,-
 *       <length>                  non-negative integer value
 *       <rstart>, <rend>          non-negative integer values, <rstart> <= <rend>
 *
 */
gboolean zMapAttParseAssemblyPath(ZMapGFFAttribute pAttribute, char ** const psOut, ZMapStrand * const pcStrandOut,
                                  int * const piOut, GArray ** const paOut, char *sBuff)
{
  gboolean bResult = FALSE ;
  static const char cComma = ',';
  static const char *sMyName = "zMapAttParseAssemblyPath()",
    *sEmpty = "",
    *sStrandForward = "+",
    *sStrandReverse = "-" ;
  static const unsigned int iExpectedMinFields = 4 ;
  static const unsigned int iExpectedFields1 = 1 ;
  static const unsigned int iExpectedFields2 = 2 ;
  unsigned int i, iTokens = 0, iTokenLimit = 1000 ;
  int iLength = 0 ;
  char **sTokens = NULL ;
  gboolean bIncludeEmpty = TRUE ;
  ZMapStrand cStrand = ZMAPSTRAND_NONE ;
  GArray *pArray = NULL ;
  if (!pAttribute)
    return bResult ;
  if (!psOut || *psOut || !pcStrandOut || !piOut || !paOut || *paOut )
    return bResult ;
  const char *sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_assembly_path, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  /*
   * Tokenize on commas; we must have a minimum of 4 fields
   */
  sTokens = zMapGFFStringUtilsTokenizer(cComma, sValue, &iTokens, bIncludeEmpty, iTokenLimit, g_malloc, g_free, sBuff) ;

  for (i=0; i<iTokens; ++i)
  {
    printf("t%i = '%s'\n", i, sTokens[i]) ;
    fflush(stdout) ;
  }

  /*
   * Now extract data from each of these; must have been a minimum of iExpectedMinFields fields.
   */
  if ((iTokens >= iExpectedMinFields) && (iTokens <= iTokenLimit))
  {
    bResult = TRUE ;

    /*
     * Firstly test that none of the strings are empty.
     */
    for (i=0; i<iTokens; ++i)
      if (!strcmp(sTokens[i], sEmpty))
        bResult = FALSE ;

    /* */
    if (bResult)
      {
        /* ap source just has to be a non-empty string */

        /* strand */
        if (!strcmp(sTokens[1], sStrandForward))
          cStrand = ZMAPSTRAND_FORWARD ;
        else if (!strcmp(sTokens[1], sStrandReverse))
          cStrand = ZMAPSTRAND_REVERSE ;
        else
          bResult = FALSE ;

        /* length */
        if (bResult && (sscanf(sTokens[2], "%i", &iLength) != iExpectedFields1))
          bResult = FALSE ;

        /* start,end pairs */
        if (bResult)
          {
            pArray = g_array_new(FALSE, TRUE, sizeof(ZMapSpanStruct)) ;
            for (i=3; i<iTokens; ++i)
              {
                ZMapSpanStruct cSpan = {0} ;
                if ((sscanf(sTokens[i], "%i %i", &cSpan.x1, &cSpan.x2) == iExpectedFields2) && (cSpan.x1 <= cSpan.x2) )
                  {
                    pArray = g_array_append_val(pArray, cSpan) ;
                  }
                else
                  {
                    bResult = FALSE ;
                  }
              }
          }

      }

    /*
     * Copy data to output values, or delete temporary variables
     * if there was an error.
     */
     if (bResult)
       {
         *psOut = g_strdup(sTokens[0]) ;
         *pcStrandOut = cStrand ;
         *piOut = iLength ;
         *paOut = pArray ;
       }
     else
       {
         if (pArray)
           g_array_free(pArray, TRUE) ;
       }
  }


  /*
   * Clean up tokens.
   */
  zMapGFFStringUtilsArrayDelete(sTokens, iTokens, g_free) ;


  return bResult ;
}




/*
 * Parse the "read_pair_id" attribute. Translate the associated string into a GQuark.
 */
gboolean zMapAttParseReadPairID(ZMapGFFAttribute pAttribute, GQuark * const pgqOut )
{
  gboolean bResult = FALSE ;
  static const char *sMyName = "zMapAttParseReadPairID()" ;
  if (!pAttribute)
    return bResult ;
  const char * const sValue = zMapGFFAttributeGetTempstring(pAttribute) ;
  if (strcmp(sAttributeName_read_pair_id, zMapGFFAttributeGetNamestring(pAttribute)))
    {
      zMapLogWarning("Attribute wrong type in %s, %s %s", sMyName, zMapGFFAttributeGetNamestring(pAttribute), sValue) ;
      return bResult ;
    }

  if (strlen(sValue))
    {
      *pgqOut = g_quark_from_string(sValue) ;
      if (*pgqOut)
        {
          bResult = TRUE ;
        }
    }

  return bResult ;
}






/*
 * Remove all of the escaped characters permitted by GFF3 from the
 * input string and return a newly allocated string containing the
 * result.
 */
static char* removeAllEscapedCharacters(const char * const sInput )
{
  char *sResult = NULL, *sOut1 = NULL, *sOut2 = NULL, *sOut3 = NULL ;
  gboolean bReplaced = FALSE ;

  bReplaced = zMapGFFStringUtilsSubstringReplace(sInput, sEscapedEquals,    sEquals, &sOut1) ;
  bReplaced = zMapGFFStringUtilsSubstringReplace(sOut1,  sEscapedComma,     sComma, &sOut2) ;
  g_free(sOut1) ;
  bReplaced = zMapGFFStringUtilsSubstringReplace(sOut2,  sEscapedSemicolon, sSemicolon, &sOut3) ;
  g_free(sOut2) ;
  bReplaced = zMapGFFStringUtilsSubstringReplace(sOut3,  sEscapedSpace,     sSpace, &sResult) ;
  g_free(sOut3) ;

  return sResult ;
}





/*
 * Generate a "Gap" attribute string from the feature.
 */
gboolean zMapAttGenerateGap(char **p_s_out, ZMapFeature feature)
{
  gboolean bResult = FALSE ;
  char *align_str = NULL ;
  zMapReturnValIfFailSafe(   p_s_out
                      && feature
                      && feature->mode == ZMAPSTYLE_MODE_ALIGNMENT,
                      bResult) ;

  if ((bResult = zMapFeatureAlignmentGetAlignmentString(feature,
                                         ZMAPALIGN_FORMAT_GAP_GFF3,
                                         &align_str )))
    {
      if (align_str)
        {
          *p_s_out = g_strdup_printf("Gap=%s", align_str) ;
          g_free(align_str) ;
        }
    }

  return bResult ;
}

/*
 * Generates "Target" attribute from the feature.
 */
gboolean zMapAttGenerateTarget(char **p_s_out, ZMapFeature feature)
{
  gboolean bResult = FALSE ;
  char *target_string = NULL ;
  ZMapHomol homol = NULL ;
  zMapReturnValIfFail(p_s_out && feature && feature->mode == ZMAPSTYLE_MODE_ALIGNMENT, bResult ) ;
  homol = &feature->feature.homol ;

  target_string = g_strdup_printf("Target=%s %i %i %s",
                                  g_quark_to_string(homol->clone_id), homol->y1, homol->y2,
                                  zMapFeatureStrand2Str(homol->strand)) ;
  *p_s_out = target_string ;
  bResult = TRUE ;

  return bResult ;
}










