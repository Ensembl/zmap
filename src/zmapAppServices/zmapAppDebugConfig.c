
/* File needs renaming to be a general config getting file.... */


/*  File: zmapConfigUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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

#include <ZMap/zmap.h>

#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>

// headers for globals
#include <ZMap/zmapThreads.h>
#include <ZMap/zmapServerProtocol.h>
#include <ZMap/zmapUtilsDebug.h>


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
// very odd! callinf this function fropm zmaplogging.c resulted in some limk errors !!!
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



/* Read ZMap sequence/start/end from config file. */
gboolean zMapAppGetSequenceConfig(ZMapFeatureSequenceMap seq_map)
{
  gboolean result = FALSE ;
  ZMapConfigIniContext context;

  if ((context = zMapConfigIniContextProvide(NULL)))
    {
//      gboolean tmp_bool = FALSE;
      char *tmp_string  = NULL;
//      int tmp_int = 0;

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
					ZMAPSTANZA_APP_DATASET, &tmp_string))
	{
	  /* if not supplied needs to appear in all the pipe script URLs */
	  seq_map->dataset = tmp_string;
	}

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
					ZMAPSTANZA_APP_SEQUENCE, &tmp_string))
	{
	  seq_map->sequence = tmp_string;
	  if (zMapConfigIniContextGetInt(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
					 ZMAPSTANZA_APP_START, &seq_map->start))
            {
	      zMapConfigIniContextGetInt(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
					 ZMAPSTANZA_APP_END, &seq_map->end);

	      /* possibly worth checking csname and csver at some point */
	      tmp_string = NULL;
	      zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
					    ZMAPSTANZA_APP_CSNAME,&tmp_string);
	      zMapAssert(!tmp_string || !g_ascii_strcasecmp(tmp_string,"chromosome"));
            }
	}

      zMapConfigIniContextDestroy(context);

      result = TRUE;
    }

  return result ;
}


