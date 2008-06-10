/*  File: zmapWindowPreferences.c
 *  Author: Roy Storey (roy@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun 10 15:42 2008 (rds)
 * Created: Fri Jun  6 12:29:16 2008 (roy)
 * CVS info:   $Id: zmapWindowPreferences.c,v 1.2 2008-06-10 14:52:41 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapConfig.h>
#include <ZMap/zmapConfigStrings.h>
#include <zmapWindow_P.h>


static ZMapConfigStanzaElementStruct pfetch_stanza_elements_G[] = {
  { ZMAPSTANZA_APP_PFETCH_LOCATION, ZMAPCONFIG_STRING, {NULL} },
  { ZMAPSTANZA_APP_COOKIE_JAR,      ZMAPCONFIG_STRING, {NULL} },
  { ZMAPSTANZA_APP_PFETCH_MODE,     ZMAPCONFIG_STRING, {NULL} },
  { NULL,                           -1,                {NULL} },
};


gboolean zmapWindowGetPFetchUserPrefs(PFetchUserPrefsStruct *pfetch)
{
  ZMapConfig          config;
  ZMapConfigStanzaSet stanzas = NULL;
  ZMapConfigStanza    stanza;
  char               *stanza_name = ZMAPSTANZA_APP_CONFIG;
  gboolean            result = FALSE;

  if((config = zMapConfigCreate()))
    {
      ZMapConfigStanzaElement elements = pfetch_stanza_elements_G;

      stanza = zMapConfigMakeStanza(stanza_name, elements);

      if(zMapConfigFindStanzas(config, stanza, &stanzas))
	{
	  ZMapConfigStanza this_stanza;
	  char *location, *cookie_jar, *pfetch_mode;

	  this_stanza = zMapConfigGetNextStanza(stanzas, NULL);
	  
	  location    = zMapConfigGetElementString(this_stanza, 
						   ZMAPSTANZA_APP_PFETCH_LOCATION);
	  cookie_jar  = zMapConfigGetElementString(this_stanza,
						   ZMAPSTANZA_APP_COOKIE_JAR);
	  
	  pfetch_mode = zMapConfigGetElementString(this_stanza,
						   ZMAPSTANZA_APP_PFETCH_MODE);

	  pfetch->location    = g_strdup(location);
	  pfetch->cookie_jar  = g_strdup(cookie_jar);
	  pfetch->mode        = (pfetch_mode ? g_strdup(pfetch_mode) : g_strdup("http"));
	  pfetch->port        = 80;
	  pfetch->full_record = TRUE;

	  zMapConfigDeleteStanzaSet(stanzas);

	  result = TRUE;
	}

      zMapConfigDestroyStanza(stanza);

      zMapConfigDestroy(config);
    }

  return result;
}
