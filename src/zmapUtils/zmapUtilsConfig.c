/*  File: zmapConfigUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Oct  1 16:07 2008 (rds)
 * Created: Mon Oct 18 09:05:27 2004 (edgrif)
 * CVS info:   $Id: zmapUtilsConfig.c,v 1.5 2009-12-14 16:37:59 mh17 Exp $
 *-------------------------------------------------------------------
 */


#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>


/* SHOULD MAKE THIS INTO A COVER FUNCTION FOR A MORE GENERALISED FUNCTION THAT GIVEN
 * A FLAG WILL RETURN ITS VALUE IN A UNION.... */
/* Looks in .ZMap to see if the specified debug flag is on or off.
 * Allows debugging for different parts of the application to be turned on/off
 * selectively. */

gboolean zMapUtilsConfigDebug(char *debug_domain, gboolean *value)
{
  ZMapConfigIniContext context = NULL;
  gboolean result = FALSE;

  if((value) && (context = zMapConfigIniContextProvide()))
    {
      gboolean tmp_bool;
      
      if(zMapConfigIniContextGetBoolean(context, 
					ZMAPSTANZA_DEBUG_CONFIG, 
					ZMAPSTANZA_DEBUG_CONFIG,
					debug_domain, &tmp_bool))
	{
	  *value = tmp_bool;
	  result = TRUE;
	}
      
      zMapConfigIniContextDestroy(context);
    }

  return result;
}


