/*  File: zmapGetConfig.c
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
 * Exported functions: See zmapConfig_P.h
 * HISTORY:
 * Last edited: May  7 10:39 2004 (edgrif)
 * Created: Mon Apr  5 11:09:52 2004 (edgrif)
 * CVS info:   $Id: zmapConfigGet.c,v 1.3 2004-05-07 09:43:29 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <strings.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <zmapConfig_P.h>


static void glistFreeStanza(gpointer data, gpointer user_data_unused) ;





/* Look through the supplied config for the specified stanza and return all the instances
 * of that stanza as a "ZMapConfigStanzaSet" in stanzas_out.
 * Returns FALSE if the specified stanza could not be found and stanzas_out is untouched. */
gboolean zmapGetConfigStanzas(ZMapConfig config,
			      ZMapConfigStanza spec_stanza, ZMapConfigStanzaSet *stanzas_out)
{
  gboolean result = FALSE ;
  GList *config_stanza_list ;
  ZMapConfigStanzaSet stanzas ;

  stanzas = g_new(ZMapConfigStanzaSetStruct, 1) ;
  stanzas->name = NULL ;
  stanzas->stanzas = NULL ;

  /* For every stanza from the config file... */
  config_stanza_list = config->stanzas ;
  do
    {
      ZMapConfigStanza config_stanza = config_stanza_list->data ;

      /* If its the stanza we are looking for... */
      if (strcasecmp(spec_stanza->name, config_stanza->name) == 0)
	{
	  ZMapConfigStanza found_stanza ;
	  GList *found_element_list ;
	  gboolean found = FALSE ;

	  /* Copy the spec stanza, this way we will copy the default settings but they will be
	   * overwritten if we find the element in the resource file. */
	  found_stanza = zmapConfigCopyStanza(spec_stanza) ;

	  found_element_list = found_stanza->elements ;

	  /* For every element in the stanza we want to find....
	   * 
	   * Probably we should not complain if we can't find it, but should log if there is an
	   * option there that we didn't ask for...maybe.... perhaps we need a "strict" flag to
	   * trigger all this.... */
	  do
	    {
	      GList *config_element_list = config_stanza->elements ;
	      ZMapConfigStanzaElement found_element = found_element_list->data ;

	      /* For every element in the current stanza from the config file... */
	      do
		{
		  ZMapConfigStanzaElement config_element = config_element_list->data ;

		  if (strcasecmp(found_element->name, config_element->name) == 0
		      && found_element->type == config_element->type)
		    {
		      zMapDebug("found: %s\n", found_element->name) ;

		      zmapConfigCopyElementData(config_element, found_element) ;

		      found = TRUE ;			    /* Record that we've found something. */
		    }
		}
	      while ((config_element_list = g_list_next(config_element_list))) ;

	    }
	  while ((found_element_list = g_list_next(found_element_list))) ;


	  /* If the stanza contained some/all of the elements we were looking for then
	   * add it to the list of "found" stanzas, otherwise junk it. */
	  if (found)
	    {
	      stanzas->stanzas = g_list_append(stanzas->stanzas, found_stanza) ;
	    }
	  else
	    {
	      zmapConfigDestroyStanza(found_stanza) ;
	    }
	}
    }
  while ((config_stanza_list = g_list_next(config_stanza_list))) ;


  if (stanzas->stanzas)
    {
      stanzas->name = g_strdup(spec_stanza->name) ;
      *stanzas_out = stanzas ;

      result = TRUE ;
    }
  else
    {
      g_free(stanzas) ;

      result = FALSE ;
    }


  return result ;
}


/* Free a stanza set. */
void zmapConfigDeleteStanzaSet(ZMapConfigStanzaSet stanzas)
{
  /* Free all the stanzas. */
  if (stanzas->stanzas)
    g_list_foreach(stanzas->stanzas, glistFreeStanza, NULL ) ;

  /* Free the stanza set stuff. */
  g_free(stanzas->name) ;
  g_free(stanzas) ;

  return ;
}


/* This is a Glib GFunc function and is called for every Stanza of the stanza set GList of stanzas,
 * it in turn calls the routine to actually free the stanza. */
static void glistFreeStanza(gpointer data, gpointer user_data_unused)
{
  ZMapConfigStanza stanza = (ZMapConfigStanza)data ;

  zmapConfigDestroyStanza(stanza) ;

  return ;
}

