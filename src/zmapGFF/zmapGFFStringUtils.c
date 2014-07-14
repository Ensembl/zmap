/*  File: zmapGFFStringUtils.c
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 * Description: Some string handling utilities used in dealing with
 * GFFv3 data; most important thing in here is the tokenizer.
 *
 *-------------------------------------------------------------------
 */

#include "zmapGFFStringUtils.h"

/*
 * Find unquoted (by q) ocurrances of the character c in the
 * string s; count them and store their positions.
 */
unsigned int *zMapGFFStr_find_unquoted(const char * const sIn,
                                       char cQuote,
                                       char cToFind,
                                       unsigned int *n,
                                       void*(*local_malloc)(size_t),
                                       void(*local_free)(void*))
{
  gboolean bQuoted = FALSE ;
  const char *s = sIn ;
  unsigned int iPos = 0,
    i = 0,
    *pResult = NULL,
    *pTemp = NULL;
    *n = 0 ;
  while (*s)
    {
      if (*s == cQuote)
        bQuoted = !bQuoted;
      if ((*s == cToFind) && !bQuoted)
        {
          pTemp = (unsigned int *) local_malloc((*n + 1) * sizeof(unsigned int)) ;
          for (i=0; i<*n; ++i)
            pTemp[i] = pResult[i] ;
          ++*n ;
          pTemp[i] = iPos ;
          if (pResult)
            {
              local_free(pResult) ;
              pResult = NULL ;
            }
          pResult = pTemp ;
        }
      ++iPos;
      ++s;
    }
  return pResult ;
}


/*
 * Take an array of strings as argument, and create a new array one
 * element longer, with the new element set to NULL. This frees the old
 * array once the new one has been copied. Note that the pointers to
 * strings are copied and therefore do not have to be reallocated and
 * their contents copied.
 *
 * Note that I have passed in the allocation/free-ing functions
 * as arguments. This is so that either malloc()/free() or
 * the gLib versions can be used without hard coding them in.
 */
void zMapGFFStr_array_add_element(char ***psArray, unsigned int *piLen,
                           void*(*local_malloc)(size_t), void(*local_free)(void*))
{
  unsigned int i ;
  char** sArrayResult = NULL ;

  if (!psArray || !piLen || !local_malloc || !local_free)
    return ;

  sArrayResult = (char**) local_malloc(sizeof(char*)*(*piLen+1)) ;
  for (i=0; i<*piLen; ++i)
    sArrayResult[i] = (*psArray)[i] ;
  sArrayResult[i] = NULL ;
  if (*psArray)
    {
      local_free( (void*) *psArray ) ;
      *psArray = NULL ;
    }
  *psArray = sArrayResult ;
  ++*piLen;
}


/*
 * Free an array of the form char** (e.g. allocated within str_array_add_element()).
 */
void zMapGFFStr_array_delete(char**psArray, unsigned int iLength, void(*local_free)(void*))
{
  unsigned int i ;

  if (!psArray || !iLength || !local_free)
    return ;

  for (i=0; i<iLength; ++i)
    if (psArray[i])
      local_free(psArray[i]) ;
  local_free(psArray) ;

  return ;
}


/*
 * Return the substring [sS1, sS2). They are assumed to
 * come from a contiguous chunk of memory that can be
 * traversed in the appropriate way.
 */
char * zMapGFFStr_substring(const char* const sS1, const char* const sS2, void*(*local_malloc)(size_t))
{
  char *sResult = NULL ;
  unsigned int i ;
  size_t iSize = 0 ;

  if (!sS1 || !sS2 || (sS2 < sS1) || !local_malloc)
    return sResult ;

  /*
   * allocate memory and copy substring
   */
  iSize = (size_t) (sS2 - sS1);
  sResult = (char *) local_malloc(iSize+1) ;
  for (i=0 ; i<iSize ; ++i)
    sResult[i] = sS1[i] ;
  sResult[i] = '\0' ;

  return sResult ;
}


/*
 * This function looks for the first occurrence of sToFind in sInput and replaces it with sReplacement.
 * A newly allocated string is returned.
 */
gboolean zMapGFFStr_substring_replace(const char * const sInput, const char * const sToFind, const char * const sReplacement,
  char ** sOut )
{
  gboolean bResult = FALSE ;
  char *sFirst = NULL,
    *sSecond = NULL,
    *sPos = NULL ;
  size_t iQueryLength ;
  if (!sInput || !*sInput || !sToFind || !*sToFind || !sReplacement || !*sReplacement )
    return bResult ;
  iQueryLength = strlen(sToFind) ;
  sPos = strstr(sInput, sToFind) ;
  if (sPos)
    {
      sFirst = g_strndup(sInput, (size_t)(sPos-sInput)) ;
      sSecond = g_strdup(sPos+iQueryLength) ;
      *sOut = g_strdup_printf("%s%s%s", sFirst, sReplacement, sSecond) ;

      if (sFirst)
        g_free(sFirst) ;
      if (sSecond)
        g_free(sSecond) ;

      bResult = TRUE ;
    }
  else
    {
      *sOut = g_strdup(sInput) ;
      bResult = FALSE ;
    }

  return bResult ;
}


/*
 * Replace all occurrences of sToFind with sReplacement.
 */
gboolean zMapGFFStr_substring_replace_n(const char * const sInput, const char * const sToFind, const char * const sReplacement,
  char ** psOut)
{
  gboolean bResult = FALSE, bReplaced = FALSE  ;
  if (!sInput || !*sInput || !sToFind || !*sToFind || !sReplacement || !*sReplacement )
    return bResult ;

  //bResult = zMapGFFStr_substring_replace(sInput, sToFind, sReplacement, psOut) ;

  /*
   * Now, how to do this many times
   */
  char *sInputTemp = g_strdup(sInput),
    *sOutputTemp = NULL ;
  while (1)
    {
      bReplaced = zMapGFFStr_substring_replace(sInputTemp, sToFind, sReplacement, &sOutputTemp) ;
      g_free(sInputTemp) ;
      sInputTemp = sOutputTemp ;

      if (!bReplaced)
        {
          break ;
        }
      else
        {
          bResult = TRUE ;
        }
    }

  *psOut = sOutputTemp ;

  return bResult ;
}



/*
 * Substring based tokenizer; returns dynamically allocated
 * array of strings. We allow empty tokens to be stored if flag is set.
 * Note that this function also removes leading and trailing spaces
 * from the tokens that are found.
 */
#define NEW_TOKENIZER_METHOD 1
char** zMapGFFStr_tokenizer(char cDelim, const char * const sTarg, unsigned int * piNumTokens,
                     gboolean bIncludeEmpty, unsigned int iTokenLimit,
                     void*(*local_malloc)(size_t), void(*local_free)(void*))
{
  static const char cToRemove = ' ';
  static const unsigned int iBuffLen = 5000;
  char sBuff[iBuffLen] ;
  unsigned int iLength = 0,
    iTokLen = 0,
    iNumTokens = 0 ;
  char *sPosLast = (char *) sTarg,
    *sPos = NULL,
    **sTokens = NULL ;
  gboolean bInclude = TRUE ;

#ifdef NEW_TOKENIZER_METHOD

  if (!sTarg || !*sTarg || !piNumTokens || !local_malloc || !local_free )
    return NULL ;

  iLength = strlen(sTarg) ;
  while ((sPos = strchr(sPosLast, cDelim)))
    {
      bInclude = TRUE ;
      iTokLen = sPos-sPosLast ;
      if (!iTokLen && !bIncludeEmpty)
        {
          bInclude = FALSE ;
        }
      if (bInclude)
        {
          /* include token here */
          memset(sBuff, 0, iBuffLen) ;
          strncpy(sBuff, sPosLast, iTokLen) ;
          zMapGFFStr_remove_char(sBuff, cToRemove) ;
          zMapGFFStr_array_add_element(&sTokens, &iNumTokens, local_malloc, local_free) ;
          sTokens[iNumTokens-1] = g_strdup(sBuff) ;
        }
      sPosLast = sPos + 1 ;

      if (iNumTokens == (iTokenLimit-1))
        break ;
    }
  if (sPosLast-sTarg <= iLength)
    {
      bInclude = TRUE ;
      iTokLen = sTarg+iLength - sPosLast ;
      if (!iTokLen && !bIncludeEmpty)
        {
          bInclude = FALSE ;
        }
      if (bInclude)
        {
          /* include token here */
          memset(sBuff, 0, iBuffLen) ;
          strncpy(sBuff, sPosLast, iTokLen) ;
          zMapGFFStr_remove_char(sBuff, cToRemove) ;
          zMapGFFStr_array_add_element(&sTokens, &iNumTokens, local_malloc, local_free) ;
          sTokens[iNumTokens-1] = g_strdup(sBuff) ;
        }
    }

  *piNumTokens = iNumTokens ;
  return sTokens ;

#else

  /* old version */
  unsigned int iDelimLength;
  const char *sTarget = sTarg,
   *sEndTarget = NULL,
    *sResult = NULL ;
  char  *sToken ;

  /*
   * Some error checks.
   */
  if (!sTarget
      || !*sTarget
      || !local_malloc
      || !local_free
      || (iTokenLimit < 2))
    return sTokens ;

  iDelimLength = 1 ;
  sEndTarget = sTarget + strlen(sTarget) ;

  /*
   * Loop whilst we walk along the string.
   */
  while ((sResult = strchr(sTarget, cDelim)))
    {

      bInclude = TRUE ;
      if ((sResult == sTarget) && !bIncludeEmpty)
        bInclude = FALSE ;
      if (bInclude)
        {
          sToken = zMapGFFStr_substring(sTarget, sResult, local_malloc) ;
          zMapGFFStr_remove_char(sToken, cToRemove) ;
          if (strlen(sToken))
            {
              zMapGFFStr_array_add_element(&sTokens, &iNumTokens, local_malloc, local_free) ;
              sTokens[iNumTokens-1] = sToken ;
            }
          else if (sToken)
            local_free(sToken) ;
        }
      sTarget = sResult + iDelimLength ;

      if (iNumTokens == (iTokenLimit-1))
        break ;
    }

  /*
   * Do the final one.
   */
  bInclude = TRUE ;
  if ((sEndTarget == sTarget) && !bIncludeEmpty)
    bInclude = FALSE ;
  if (bInclude)
    {
      sToken = zMapGFFStr_substring(sTarget, sEndTarget, local_malloc) ;
      zMapGFFStr_remove_char(sToken, cToRemove) ;
      if (strlen(sToken))
        {
          zMapGFFStr_array_add_element(&sTokens, &iNumTokens, local_malloc, local_free) ;
          sTokens[iNumTokens-1] = sToken ;
        }
      else if (sToken)
        local_free(sToken) ;
    }

  /*
   * Store the number of tokens found.
   */
  *piNumTokens = iNumTokens ;

  return sTokens ;

#endif

}


/*
 * Tokenizes the data passed in using the function above to find positions of the
 * delimiter character cDelim. Ignores delimiter characters that are surrounded by
 * instances of the argument cQuote. However, it does not check that the cQuote
 * characters are in matched pairs; thus we could go off the end of the input string.
 */
char** zMapGFFStr_tokenizer02(char cDelim,
                              char cQuote,
                              const char * const sTarget,
                              unsigned int * piNumTokens,
                              gboolean bIncludeEmpty,
                              void*(*local_malloc)(size_t),
                              void(*local_free)(void*))
{
  static const char cToRemove = ' ';
  char ** sTokens = NULL, *sToken = NULL ;
  const char *sStart = NULL, *sEnd = NULL ;
  unsigned int *pPositions = NULL,
    i = 0,  n = 0 ,
    iNumTokens = 0,
    iLength = 0;
  gboolean bInclude = TRUE ;

  /*
   * Some error checks.
   */
  if (   !cDelim
      || !cQuote
      || !sTarget
      || !*sTarget
      || !local_malloc
      || !local_free
     )
    return sTokens ;

  /*
   * Length of target string.
   */
  iLength = strlen(sTarget) ;


  /*
   * Find positions of unquoted delimiters in the input string.
   */
  pPositions = zMapGFFStr_find_unquoted(sTarget, cQuote, cDelim, &n, local_malloc, local_free) ;
  *piNumTokens = 0 ;

  /*
   * Create a collection of tokens from the position data and
   * add to the return array.
   */
  for (i=0; i<n; ++i)
    {
      sStart = sTarget + (i==0 ? 0 : pPositions[i-1]+1);
      sEnd = sTarget + pPositions[i];

      bInclude = TRUE ;
      if ((sStart == sEnd) && !bIncludeEmpty)
        bInclude = FALSE ;
      if (bInclude)
        {
          sToken = zMapGFFStr_substring(sStart, sEnd, local_malloc);
          zMapGFFStr_remove_char(sToken, cToRemove) ;

          if (strlen(sToken))
            {
              zMapGFFStr_array_add_element(&sTokens, &iNumTokens, local_malloc, local_free) ;
              sTokens[iNumTokens-1] = sToken ;
              ++*piNumTokens;
            }
          else if (sToken)
            {
             local_free(sToken) ;
            }
        }
    }

  /*
   * This is the final one (or the first one if n == 0).
   */
  sStart = n ? sTarget+pPositions[n-1]+1 : sTarget ;
  sEnd = sTarget+iLength ;
  bInclude = TRUE ;
  if ((sStart >= sEnd) && !bIncludeEmpty)
    bInclude = FALSE ;
  if (bInclude)
    {
      sToken = zMapGFFStr_substring(sStart, sEnd, local_malloc);
      zMapGFFStr_remove_char(sToken, cToRemove) ;

      if (strlen(sToken))
        {
          zMapGFFStr_array_add_element(&sTokens, &iNumTokens, local_malloc, local_free) ;
          sTokens[iNumTokens-1] = sToken ;
          ++*piNumTokens ;
        }
      else if (sToken)
        {
          local_free(sToken) ;
        }
    }

  if (pPositions)
    local_free(pPositions) ;

  return sTokens ;
}




/*
 * Function to remove LEADING and TRAILING characters from the input
 * null-terminated string. DOES NOT remove character from anywhere else.
 *
 * Leading hits  are removed by moving the contents of the string
 * back in the array, and trailing hits are removed by writing a new
 * null-terminating character in the appropriate place.
 */
void zMapGFFStr_remove_char(char* sInput, char cToRemove)
{
  static const char cNull = '\0' ;
  char *sStart = sInput ;
  unsigned int iMove = 0 ;

  /*
   * If the string is empty, do nothing.
   */
  if (!sInput && !*sInput)
    return ;

  /*
   * Do beginning of string. Count the number of cToRemove  at the front, and
   * then move the rest of the string back by that number; then append a
   * terminating null character.
   */
  while (*sStart == cToRemove)
    {
      ++sStart; ++iMove ;
    }
  sStart = sInput ;
  while (*(sStart+iMove))
    {
      *sStart = *(sStart+iMove) ;
      ++sStart ;
    }
  *sStart = cNull ;

  /*
   * Now do end of string. sStart is at the terminating null so we can work back
   * and replace cToRemove with null characters. On encountering the first non-cToRemove
   * character, we break.
   */
  while (sStart > sInput)
    {
      --sStart ;
      if (*sStart == cToRemove)
        {
          *sStart = cNull ;
        }
      else
        {
          break ;
        }
    }
}
