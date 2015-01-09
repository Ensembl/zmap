/*  File: zmapAppDebugConfig.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
  ZMAPAPPSERVICES_ERROR_NO_SEQUENCE,
  ZMAPAPPSERVICES_ERROR_CONFLICT_DATASET,
  ZMAPAPPSERVICES_ERROR_CONFLICT_SEQUENCE,
  ZMAPAPPSERVICES_ERROR_BAD_SEQUENCE_NAME
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


/* Set the sequence details in the given sequence map if not set already; if they are set,
 * merge the new details. Sets the error if the new values could not be merged (i.e. it's
 * a different sequence or the coords do not overlap). */
void zMapAppMergeSequenceName(ZMapFeatureSequenceMap seq_map, const char *sequence_name, 
                              const gboolean merge_details, GError **error)
{
  zMapReturnIfFail(seq_map) ;
  GError *tmp_error = NULL ;

  /* Check the sequence name */
  if (!seq_map->sequence)
    {
      /* No sequence name set yet so use the new one */
      seq_map->sequence = g_strdup(sequence_name) ;
    }
  else if (sequence_name && strcmp(sequence_name, seq_map->sequence) != 0)
    {
      /* Sequence name is already set and the new one does not match - error */
      g_set_error(&tmp_error, ZMAP_APP_SERVICES_ERROR, ZMAPAPPSERVICES_ERROR_BAD_SEQUENCE_NAME,
                  "Error merging sequence '%s'; name does not match the set sequence '%s'",
                  sequence_name, seq_map->sequence) ;
      
    }

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;
}


/* Validate that the given coords are compatible with those in the seq_map. If exact_match is true
 * then the ranges must match exactly or the error is set; otherwise they are considered valid if
 * the ranges overlap. If exact_match is false and merge_details is true then we attempt to merge
 * the ranges - in this case the error is set if they cannot be merged (i.e. they do not overlap). */
void zMapAppMergeSequenceCoords(ZMapFeatureSequenceMap seq_map, int start, int end, 
                                const gboolean exact_match, const gboolean merge_details, GError **error)
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
  else if (seq_map->start == start && seq_map->end == end)
    {
      /* New values match existing ones: nothing to do */
    }
  else if (exact_match)
    {
      /* The new details are different to the existing ones and we're looking for an exact match,
       * so this is an error */
      g_set_error(&tmp_error, ZMAP_APP_SERVICES_ERROR, ZMAPAPPSERVICES_ERROR_BAD_COORDS,
                  "The coord range [%d,%d] must match the existing range [%d,%d]",
                  start, end, seq_map->start, seq_map->end) ;
    }
  else if (seq_map->start && seq_map->end && start && end)
    {
      /* Both start/end are set in both ranges. See if they overlap. */
      if ((seq_map->start < start && seq_map->end < start) ||
          (start < seq_map->start && end < seq_map->end))
        {
          /* No overlap, so this is an error */
          g_set_error(&tmp_error, ZMAP_APP_SERVICES_ERROR, ZMAPAPPSERVICES_ERROR_BAD_COORDS,
                      "Error merging coords [%d,%d]; range does not overlap the set sequence range [%d,%d]",
                      start, end, seq_map->start, seq_map->end) ;
        }
      else if (merge_details)
        {
          /* We know there's an overlap and we want to merge the ranges, so join the ranges by
           * taking the maximum extent. */
          zMapLogMessage("Merging coordinate range [%d,%d] with the set sequence range [%d,%d]",
                         start, end, seq_map->start, seq_map->end) ;
          
          seq_map->start = (seq_map->start < start ? seq_map->start : start) ;
          seq_map->end = (seq_map->end > end ? seq_map->end : end) ;
        }
    }
  else
    {
      /* We shouldn't have a start coord set without an end coord or vice versa. If we do we get
       * here. Perhaps we could handle this but the logic is more complicated if we try to merge
       * a single coord, so for now calling functions are assumed to set both or neither. */
      zMapWarnIfReached() ;
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
          /* Pass merge_details as false - it's not currently used because we're not currently
           * allowing more than one sequence. We may want to change this in future but even so we
           * probably want the config file details to agree with the command-line details so we'd
           * pass merge_details as false here anyway. */
          zMapAppMergeSequenceName(seq_map, tmp_string, FALSE, &tmp_error) ;
        }

      if (!tmp_error)
        {
          gboolean got_start = zMapConfigIniContextGetInt(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                                          ZMAPSTANZA_APP_START, &start) ;

          gboolean got_end = zMapConfigIniContextGetInt(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                                        ZMAPSTANZA_APP_END, &end) ;
         
          if (got_start && got_end && !seq_map->sequence)
            {
              g_set_error(&tmp_error, ZMAP_APP_SERVICES_ERROR, ZMAPAPPSERVICES_ERROR_NO_SEQUENCE,
                          "The coords were specified in the config file but the sequence name was not; must specify all or none of sequence/start/end") ;
            }
          else if (got_start && got_end)
            {
              /* Pass merge_details as false because we want the config file range to match the
               * command-line range. (We could merge the ranges, but I'm not sure that makes sense 
               * because we wouldn't know which range applies to the DNA, if given.) */
              zMapAppMergeSequenceCoords(seq_map, start, end, TRUE, FALSE, &tmp_error) ;
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
