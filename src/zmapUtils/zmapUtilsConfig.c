/*  File: zmapConfigUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * Last edited: Oct 18 11:03 2004 (edgrif)
 * Created: Mon Oct 18 09:05:27 2004 (edgrif)
 * CVS info:   $Id: zmapUtilsConfig.c,v 1.1 2004-10-19 14:58:58 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfig.h>



/* SHOULD MAKE THIS INTO A COVER FUNCTION FOR A MORE GENERALISED FUNCTION THAT GIVEN
 * A FLAG WILL RETURN ITS VALUE IN A UNION.... */
/* Looks in .ZMap to see if the specified debug flag is on or off.
 * Allows debugging for different parts of the application to be turned on/off
 * selectively. */
gboolean zMapUtilsConfigDebug(char *debug_domain, gboolean *value)
{
  gboolean result = FALSE ;
  ZMapConfigStanzaSet debug_list = NULL ;
  ZMapConfig config ;

  if ((config = zMapConfigCreate()))
    {
      ZMapConfigStanza template_stanza ;
      /* If you change this resource array be sure to check that the subesequent
       * initialisation is still correct. */
      ZMapConfigStanzaElementStruct debug_elements[] = {{debug_domain, ZMAPCONFIG_BOOL, {NULL}},
							{NULL, -1, {NULL}}} ;

      /* Set defaults for any element that is not a string. */
      debug_elements[0].data.b = FALSE ;

      template_stanza = zMapConfigMakeStanza("debug", debug_elements) ;

      if (zMapConfigFindStanzas(config, template_stanza, &debug_list))
	{
	  ZMapConfigStanza debug_stanza = NULL ;

	  if ((debug_stanza = zMapConfigGetNextStanza(debug_list, debug_stanza)) != NULL)
	    {
	      /* There is a hole in the interface for config, I can't test to see if a flag is
	       * found at all....may not want to set value if flag is not found.... */
	      *value = zMapConfigGetElementBool(debug_stanza, debug_domain) ;
	      result = TRUE ;
	    }
	}


      /* clean up. */
      if (debug_list)
	zMapConfigDeleteStanzaSet(debug_list) ;

      zMapConfigDestroyStanza(template_stanza) ;

      zMapConfigDestroy(config) ;
    }

  return result ;
}



