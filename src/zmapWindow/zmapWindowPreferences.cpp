/*  File: zmapWindowPreferences.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
