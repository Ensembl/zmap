/*  File: zmapWindowPreferences.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */
#include <ZMap/zmap.hpp>

#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <zmapWindow_P.hpp>

#include <curl/curl.h>
#include <string.h>


gboolean zmapWindowGetPFetchUserPrefs(char *config_file, PFetchUserPrefsStruct *pfetch)
{
  ZMapConfigIniContext context = NULL;
  gboolean result = FALSE;

  pfetch->ipresolve = CURL_IPRESOLVE_WHATEVER ;
  pfetch->port = 80;
  
  if((context = zMapConfigIniContextProvide(config_file, ZMAPCONFIG_FILE_NONE)))
    {
      char *tmp_string;
      int tmp_int ;
      gboolean tmp_bool ;

      if (zMapConfigIniContextGetFilePath(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                        ZMAPSTANZA_APP_PFETCH_LOCATION, &tmp_string))
        {
          pfetch->location = tmp_string ;
        }

      if (zMapConfigIniContextGetFilePath(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                          ZMAPSTANZA_APP_COOKIE_JAR, &tmp_string))
        {
          pfetch->cookie_jar = tmp_string ;
        }

      if (zMapConfigIniContextGetFilePath(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                          ZMAPSTANZA_APP_PROXY, &tmp_string))
        {
          pfetch->proxy = tmp_string ;
        }

      if (zMapConfigIniContextGetFilePath(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                          ZMAPSTANZA_APP_CURL_CAINFO, &tmp_string))
        {
          pfetch->cainfo = tmp_string ;
        }

      if (zMapConfigIniContextGetInt(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                          ZMAPSTANZA_APP_PORT, &tmp_int))
        {
          pfetch->port = tmp_int ;
        }

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                       ZMAPSTANZA_APP_PFETCH_MODE, &tmp_string))
        {
          pfetch->mode = (tmp_string ? tmp_string : g_strdup("http"));
        }

      if(zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                        ZMAPSTANZA_APP_CURL_DEBUG, &tmp_bool))
        {
          pfetch->verbose = tmp_bool ;
        }

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                       ZMAPSTANZA_APP_CURL_IPRESOLVE, &tmp_string))
        {
          /* Valid values are "ipv4" or "ipv6" (case insensitive) */
          if (tmp_string && strlen(tmp_string) == 4)
            {
              if (g_ascii_strncasecmp(tmp_string, "ipv4", 4) == 0)
                pfetch->ipresolve = CURL_IPRESOLVE_V4 ;
              else if (g_ascii_strncasecmp(tmp_string, "ipv6", 4) == 0)
                pfetch->ipresolve = CURL_IPRESOLVE_V6 ;
            }

          if (tmp_string)
            g_free(tmp_string) ;
        }

      pfetch->full_record = TRUE;

      if(pfetch->location)
        result = TRUE;

      zMapConfigIniContextDestroy(context);
    }

  return result ;
}
