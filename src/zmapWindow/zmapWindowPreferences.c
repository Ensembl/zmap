/*  File: zmapWindowPreferences.c
 *  Author: Roy Storey (roy@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <zmapWindow_P.h>


gboolean zmapWindowGetPFetchUserPrefs(PFetchUserPrefsStruct *pfetch)
{
  ZMapConfigIniContext context = NULL;
  gboolean result = FALSE;
  
  if((context = zMapConfigIniContextProvide()))
    {
      char *tmp_string;

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
					ZMAPSTANZA_APP_PFETCH_LOCATION, &tmp_string))
	pfetch->location = tmp_string ;

      if (zMapConfigIniContextGetFilePath(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
					  ZMAPSTANZA_APP_COOKIE_JAR, &tmp_string))
	pfetch->cookie_jar = tmp_string ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
				       ZMAPSTANZA_APP_PFETCH_MODE, &tmp_string))
	pfetch->mode = (tmp_string ? tmp_string : g_strdup("http"));

      pfetch->port = 80;
      pfetch->full_record = TRUE;

      if(pfetch->location)
	result = TRUE;

      zMapConfigIniContextDestroy(context);
    }

  return result ;
}
