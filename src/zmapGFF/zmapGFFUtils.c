#include <glib.h>

#include <ZMap/zmapGFF.h>

/*
 * This function expects a string of the form
 *
 * ##gff-version <i>
 *
 * and parses this for the integer value <i> and writes this to the output
 * integer argument. Returns TRUE if this operation gives either 2 or 3,
 * and FALSE otherwise.
 *
 *
 * At the moment, this is a dummy function that just returns TRUE with v = 2.
 */
gboolean zMapGFFGetVersionFromString(const char* const sString, int * const piOut)
{
  gboolean bResult = TRUE ;

  *piOut = 2 ;

  return bResult ;
}


/*
 * This function expects to be at the start of a GIOChannel and that the next line
 * to be read is of the form expected by zMapGFFGetVersionFromString(). Then parse
 * the line with this function and rewind the GIOChannel to the same point at which
 * we entered. Returns TRUE if this operation returns 2 or 3, FALSE otherwise.
 *
 * This is currently a dummy version of the function that returns value from the
 * above string parser.
 *
 */
gboolean zMapGFFGetVersionFromGIO(GIOChannel * const pChannel, int * const piOut )
{
  return zMapGFFGetVersionFromString(NULL, piOut) ;
}

