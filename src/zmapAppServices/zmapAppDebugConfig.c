/*  File: zmapAppDebugConfig.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *        Rob Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */


/* File needs renaming to be a general config getting file.... */


#include <ZMap/zmap.h>

#include <ZMap/zmapFeatureLoadDisplay.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>

// headers for globals
#include <ZMap/zmapThreads.h>
#include <ZMap/zmapServerProtocol.h>
#include <ZMap/zmapUtilsDebug.h>

#include <string.h>


/*
 * We follow glib convention in error domain naming:
 *          "The error domain is called <NAMESPACE>_<MODULE>_ERROR"
 */
#define ZMAP_APP_SERVICES_ERROR g_quark_from_string("ZMAP_APP_SERVICES_ERROR")

typedef enum
{
  ZMAPAPPSERVICES_ERROR_BAD_COORDS,
  ZMAPAPPSERVICES_ERROR_CONFLICT_DATASET,
  ZMAPAPPSERVICES_ERROR_CONFLICT_SEQUENCE,
} ZMapUtilsError;


/* SHOULD MAKE THIS INTO A COVER FUNCTION FOR A MORE GENERALISED FUNCTION THAT GIVEN
 * A FLAG WILL RETURN ITS VALUE IN A UNION.... */
/* Looks in .ZMap to see if the specified debug flag is on or off.
 * Allows debugging for different parts of the application to be turned on/off
 * selectively. */

gboolean zMapUtilsConfigDebug(char *config_file)
{
  ZMapConfigIniContext context = NULL;
  gboolean result = FALSE;

  if((context = zMapConfigIniContextProvide(config_file)))
    {
      result = TRUE;
#if 0
/* very odd! calling this function fropm zmaplogging.c resulted in some limk errors !!!*/
      zMapConfigIniContextGetBoolean(context,
					ZMAPSTANZA_DEBUG_CONFIG,
					ZMAPSTANZA_DEBUG_CONFIG,
					ZMAPSTANZA_DEBUG_APP_THREADS, &zmap_thread_debug_G);
      zMapConfigIniContextGetBoolean(context,
                              ZMAPSTANZA_DEBUG_CONFIG,
                              ZMAPSTANZA_DEBUG_CONFIG,
                              ZMAPSTANZA_DEBUG_APP_FEATURE2STYLE, &zmap_server_feature2style_debug_G);

      zMapConfigIniContextGetBoolean(context,
                              ZMAPSTANZA_DEBUG_CONFIG,
                              ZMAPSTANZA_DEBUG_CONFIG,
                              ZMAPSTANZA_DEBUG_APP_STYLES, &zmap_server_styles_debug_G);
#endif
      zMapConfigIniContextGetBoolean(context,
                              ZMAPSTANZA_DEBUG_CONFIG,
                              ZMAPSTANZA_DEBUG_CONFIG,
                              ZMAPSTANZA_DEBUG_APP_TIMING, &zmap_timing_G);

      zMapConfigIniContextGetBoolean(context,
                              ZMAPSTANZA_DEBUG_CONFIG,
                              ZMAPSTANZA_DEBUG_CONFIG,
                              ZMAPSTANZA_DEBUG_APP_DEVELOP, &zmap_development_G);

      zMapConfigIniContextDestroy(context);
    }

  return result;
}


/* Merge the given coords with those already set in the given sequence map. Sets the error if
 * they cannot be merged */
static void zmapAppMergeSequenceCoords(ZMapFeatureSequenceMap seq_map, int start, int end, GError **error)
{
  zMapReturnIfFail(seq_map) ;
  GError *tmp_error = NULL ;

  if (!seq_map->start && !seq_map->end)
    {
      /* No existing values are set so use the new values (even if they're not set: they'll just
       * remain unset) */
      seq_map->start = start ;
      seq_map->end = end ;
    }
  else if (!start && !end)
    {
      /* No new values: nothing to do */
    }
  else if (seq_map->start && seq_map->end && start && end)
    {
      /* Both start/end are set in both ranges. Try to merge them */
      if ((seq_map->start < start && seq_map->end < start) ||
          (start < seq_map->start && end < seq_map->end))
        {
          /* Check if there is a gap between the ranges, i.e. no overlap */
          g_set_error(&tmp_error, ZMAP_APP_SERVICES_ERROR, ZMAPAPPSERVICES_ERROR_BAD_COORDS,
                      "Coordinate range from config file [%d,%d] does not overlap set range [%d,%d]",
                      start, end, seq_map->start, seq_map->end) ;
        }
      else
        {
          /* We know there's an overlap so join the ranges by taking the maximum extent. */
          zMapLogMessage("Merging coordinate range from config file [%d,%d] with the set range [%d,%d]",
                         start, end, seq_map->start, seq_map->end) ;
          
          seq_map->start = (seq_map->start < start ? seq_map->start : start) ;
          seq_map->end = (seq_map->end > end ? seq_map->end : end) ;
        }
    }

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;
}


/* Read ZMap sequence/start/end from config file. Sets them in the given sequence map if found,
 * unless they are already set, in which case it validates them against the existing values and
 * sets the error if there are conflicts. */
void zMapAppGetSequenceConfig(ZMapFeatureSequenceMap seq_map, GError **error)
{
  ZMapConfigIniContext context;
  GError *tmp_error = NULL ;

  if (seq_map && seq_map->config_file && (context = zMapConfigIniContextProvide(seq_map->config_file)))
    {
      char *tmp_string  = NULL ;
      int start = 0 ;
      int end = 0 ;

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
					ZMAPSTANZA_APP_DATASET, &tmp_string))
	{
	  /* if not supplied needs to appear in all the pipe script URLs */
          if (!seq_map->dataset)
            {
              seq_map->dataset = tmp_string;
            }
          else if (strcmp(seq_map->dataset, tmp_string) != 0)
            {
              g_set_error(&tmp_error, ZMAP_APP_SERVICES_ERROR, ZMAPAPPSERVICES_ERROR_CONFLICT_DATASET,
                          "Dataset '%s' in the config file does not match the set dataset '%s'",
                          tmp_string, seq_map->dataset) ;
            }
	}

      if (!tmp_error && 
          zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
					ZMAPSTANZA_APP_SEQUENCE, &tmp_string))
	{
          if (!seq_map->sequence)
            {
              seq_map->sequence = tmp_string;
            }
          else
            {
              g_set_error(&tmp_error, ZMAP_APP_SERVICES_ERROR, ZMAPAPPSERVICES_ERROR_CONFLICT_SEQUENCE,
                          "Sequence '%s' in the config file does not match the set sequence '%s'",
                          tmp_string, seq_map->sequence) ;
            }
        }

      if (!tmp_error)
        {
          gboolean got_start = zMapConfigIniContextGetInt(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                                          ZMAPSTANZA_APP_START, &start) ;

          gboolean got_end = zMapConfigIniContextGetInt(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                                        ZMAPSTANZA_APP_END, &end) ;
         
          if (got_start && got_end)
            {
              zmapAppMergeSequenceCoords(seq_map, start, end, &tmp_error) ;
            }
          else if (got_start)
            {
              g_set_error(&tmp_error, ZMAP_APP_SERVICES_ERROR, ZMAPAPPSERVICES_ERROR_BAD_COORDS,
                          "The start coord was specified in the config file but the end coord is missing: must specify both or none") ;
            }
          else if (got_end)
            {
              g_set_error(&tmp_error, ZMAP_APP_SERVICES_ERROR, ZMAPAPPSERVICES_ERROR_BAD_COORDS,
                          "The end coord was specified in the config file but the start coord is missing: must specify both or none") ;
            }
        }

      if (!tmp_error)
        {
          /*! \todo possibly worth checking csname and csver at some point */
          tmp_string = NULL;
          zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                        ZMAPSTANZA_APP_CSNAME,&tmp_string);
        }

      zMapConfigIniContextDestroy(context) ;
    }

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;
}
