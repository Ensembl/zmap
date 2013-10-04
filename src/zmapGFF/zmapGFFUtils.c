#include <glib.h>
#include <string.h>
#include <ZMap/zmapGFF.h>
/*
 * This function expects a string of the form
 *
 * ##gff-version <i>
 *
 * and parses this for the integer value <i> and writes this to the output
 * integer argument. Returns TRUE if this operation gives either 2 or 3,
 * and FALSE otherwise.
 */
gboolean zMapGFFGetVersionFromString(const char* const sString, int * const piOut)
{
  static const char *sFormat = "##gff-version %i" ;
  static const char iExpectedFields = 1 ;
  int iValue ;
  gboolean bResult = FALSE ;

  if (sscanf(sString, sFormat, &iValue) == iExpectedFields)
  {
    if ((iValue == 2) || (iValue == 3))
    {
      bResult = TRUE ;
      *piOut = iValue ;
    }
  }

  return bResult ;
}


/*
 * This function expects to be at the start of a GIOChannel and that the next line
 * to be read is of the form expected by zMapGFFGetVersionFromString(). Then parse
 * the line with this function and rewind the GIOChannel to the same point at which
 * we entered. Returns TRUE if this operation returns 2 or 3, FALSE otherwise.
 *
 * This is currently a dummy version of the function that returns the default
 * value.
 *
 */
gboolean zMapGFFGetVersionFromGIO(GIOChannel * const pChannel, int * const piOut )
{
  gboolean bResult = FALSE ;

  bResult = TRUE ;
  *piOut = 2 ;

  return bResult ;
}

