/*  File: zmapConfig.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * and was written by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See zmapConfig.h
 * HISTORY:
 * Last edited: Feb 10 16:01 2005 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapConfig.c,v 1.14 2005-02-10 16:36:22 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <strings.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfigDir.h>
#include <zmapConfig_P.h>


static ZMapConfig createConfig(void) ;
static void destroyConfig(ZMapConfig config) ;


/*! @defgroup zmapconfig   zMapConfig: configuration reading/writing
 * @{
 * 
 * \brief  Configuration File Handling.
 * 
 * zMapConfig routines read and write data from configuration files for ZMap.
 * 
 * The configuration file(s) for ZMap are a series of stanzas of the form:
 * 
 * <PRE>
 * stanza_name                e.g.     server
 * {                                   {
 * resource = value                    host = "griffin"
 * resource = value                    port = 18100
 * }                                   protocol = "acedb"
 *                                     }
 *  </PRE>
 * 
 * Resources may take boolean, integer, float or string values.
 *
 * String values must be enclosed in double quotes, e.g. host = "griffin"
 *
 * Boolean values must be specified as either true or false, e.g. logging = false
 * 
 * zMapConfigCreate() reads all the configuration files and merges them
 * into a unified array of resources. Specific stanzas can then be found
 * and returned using zMapConfigFindStanzas(). When the configuration is
 * finished with it should be destroyed using zMapConfigDestroy().
 * 
 * The intention of the code is to make it easy for the programmer to specify
 * via static initialisation of a simple stanza array what stanza/elements
 * they are looking for and then extract the data from them.
 *
 * Usage:
 * <PRE>
 *      // Make the "server" stanza so we can search for it in the configuration data.
 *      ZMapConfigStanza server_stanza ;
 *      ZMapConfigStanzaElementStruct server_elements[] = {{"host", ZMAPCONFIG_STRING, {NULL}},
 *                                                         {"port", ZMAPCONFIG_INT, {NULL}},
 *	 	    		  	                   {"protocol", ZMAPCONFIG_STRING, {NULL}},
 *						           {NULL, -1, {NULL}}} ;
 *
 *      // Set any defaults that are not equivalent to string NULL.
 *      zMapConfigGetStructInt(server_elements, "port") = -1 ;
 *
 *	server_stanza = zMapConfigMakeStanza("server", server_elements) ;
 *
 *      // Find all the "server" stanzas and process them.
 *  	if (zMapConfigFindStanzas(zmap->config, server_stanza, &server_list))
 *  	  {
 *	    ZMapConfigStanza next_server = NULL ;
 *
 *	    while ((next_server = zMapConfigGetNextStanza(server_list, next_server)) != NULL)
 *	      {
 *		ZMapConfigStanzaElement element ;
 *		char *machine ;
 *
 *		machine = zMapConfigGetElementString(next_server, "host") ;
 *	      }
 *
 *	    // clean up.
 *	    if (server_list)
 *	      zMapConfigDeleteStanzaSet(server_list) ;
 *	  }
 * </PRE>
 *
 *
 *  */





/*!
 * Finds the named element in an array of ZMapConfigStanzaElementStruct's and returns
 * a pointer to that element. The main use of this routine is via macros defined in
 * ZMap/zmapConfig.h to intialise the data fields in the array.
 * This is necessary because with ANSI-C its not possible to set the default values
 * via static initialisation of the array.
 *
 * @param elements     An array of stanza elements, each initialised with the name of the
 *                     element, its type and a default value.
 * @param element_name The name of the element to be returned.
 * @return Returns a pointer to the named element or NULL if not found.
 *  */
ZMapConfigStanzaElement zMapConfigFindStruct(ZMapConfigStanzaElementStruct elements[],
					     char *element_name)
{
  ZMapConfigStanzaElement element = NULL ;
  int i ;

  i = 0 ;
  while (elements[i].name != NULL)
    {
      if (strcasecmp(element_name, elements[i].name) == 0)
	{
	  element = &(elements[i]) ;
	  break ;
	}
      i++ ;
    }

  return element ;
}



/*!
 * Construct a stanza from a name and an array of elements. The array of elements
 * can just be statically declared in the code and then passed to this routine which
 * then makes the corresponding stanza. NOTE that the last stanza element in the 
 * elements array must be a dummy entry with a NULL name.
 *
 * The only tedious bit is that it is not possible
 * with ANSI-C to set the default parameters via a static array, they must be set
 * explicitly.
 *
 * @param stanza_name The name of the Stanza.
 * @param elements    An array of stanza elements, each initialised with the name of the
 *                    element, its type and a default value.
 * @return A stanza which can be used as input to the stanza search/manipulation routines.
 *  */
ZMapConfigStanza zMapConfigMakeStanza(char *stanza_name, ZMapConfigStanzaElementStruct elements[])
{
  ZMapConfigStanza stanza ;
  int i ;

  stanza = zmapConfigCreateStanza(stanza_name) ;

  for (i = 0 ; elements[i].name != NULL ; i++)
    {
      ZMapConfigStanzaElement new_element ;

      new_element = zmapConfigCreateElement(elements[i].name, elements[i].type) ;

      switch(elements[i].type)
	{
	case ZMAPCONFIG_BOOL:
	  new_element->data.b = elements[i].data.b ;
	  break ;
	case ZMAPCONFIG_INT:
	  new_element->data.i = elements[i].data.i ;
	  break ;
	case ZMAPCONFIG_FLOAT:
	  new_element->data.f = elements[i].data.f ;
	  break ;
	case ZMAPCONFIG_STRING:
	  new_element->data.s = NULL ;
	  if (elements[i].data.s)
	    new_element->data.s = g_strdup(elements[i].data.s) ;
	  break ;
	default:
	  zMapCrash("%s", "Code Error: unrecognised data type for stanza element union") ;
	  break ;
	}

      stanza->elements = g_list_append(stanza->elements, new_element) ;
    }

  return stanza ;
}


/*!
 * Destroy a stanza, frees all the elements and associated data.
 *
 * @param stanza      The stanza to be destroyed.
 * @return            nothing.
 *  */
void zMapConfigDestroyStanza(ZMapConfigStanza stanza)
{
  zmapConfigDestroyStanza(stanza) ;

  return ;
}


/*!
 * Call in a while loop to iterate through a list of stanzas, the function returns the stanza
 * following the current_stanza. It returns the the first stanza if current_stanza is NULL and
 * returns NULL if current_stanza is the last stanza. Hence by feeding back into the function
 * the stanza returned by the previous call to the function it is possible to iterate through
 * the stanzas.
 *
 * @param stanzas         A set of stanzas to iterate through.
 * @param current_stanza  NULL or a stanza returned by a previous call to this function.
 * @return The stanza following current_stanza or NULL if no more stanzas.
 *  */
ZMapConfigStanza zMapConfigGetNextStanza(ZMapConfigStanzaSet stanzas, ZMapConfigStanza current_stanza)
{
  ZMapConfigStanza next_stanza = NULL ;

  if (current_stanza)
    {
      GList *stanza_list ;

      if ((stanza_list = g_list_find(stanzas->stanzas, current_stanza))
	  && (stanza_list = g_list_next(stanza_list)))
	{
	  next_stanza = (ZMapConfigStanza)(stanza_list->data) ;
	}
    }
  else
    next_stanza = (ZMapConfigStanza)(stanzas->stanzas->data) ;

  return next_stanza ;
}


/*!
 * Delete a stanza set (usually created by a call to zMapConfigFindStanzas().
 *
 * @param stanzas     The stanza set to be deleted.
 * @return            nothing
 *  */
void zMapConfigDeleteStanzaSet(ZMapConfigStanzaSet stanzas)
{
  zmapConfigDeleteStanzaSet(stanzas) ;
  
  return ;
}


/*!
 * Return the element corresponding to the given element name in a stanza, note if an element
 * is duplicated in a stanza, only the first one found is returned.
 *
 * @param stanza          The stanza to search for the specified element.
 * @param element_name    The name of the element to be searched for.
 * @return                The element or NULL if the element is not found.
 *  */
ZMapConfigStanzaElement zMapConfigFindElement(ZMapConfigStanza stanza, char *element_name)
{
  ZMapConfigStanzaElement result = NULL ;
  GList *element_list ;

  if ((element_list = stanza->elements))
    {
      do
	{
	  ZMapConfigStanzaElement element = (ZMapConfigStanzaElement)element_list->data ;

	  if (strcasecmp(element_name, element->name) == 0)
	    {
	      result = element ;
	      break ;
	    }
	}
      while ((element_list = g_list_next(element_list))) ;
    }

  return result ;
}


/*!
 * Create a configuration which can then be searched for configuration stanzas.
 * The configuration is read partly from internal hard-coded data and partly
 * from the default (hard-coded) user configuration file.
 *
 * @param   none
 * @return                    The configuration if all the configuration files etc. were
 *                            successfully read, NULL otherwise.
 *  */
ZMapConfig zMapConfigCreate(void)
{
  ZMapConfig config = NULL ;
  char *filepath = NULL ;

  if ((filepath = zMapConfigDirGetFile()))
    config = zMapConfigCreateFromFile(filepath) ;

  return config ;
}


/*!
 * Create a configuration which can then be searched for configuration stanzas
 * from a named file, the file must exist.
 *
 * @param config_file         Path of configuration file to be read.
 * @return                    The configuration if all the configuration files etc. were
 *                            successfully read, NULL otherwise.
 *  */
ZMapConfig zMapConfigCreateFromFile(char *config_file)
{
  ZMapConfig config = NULL ;
  gboolean status = TRUE ;

  zMapAssert(config_file && *config_file) ;

  if (zMapFileAccess(config_file) && !zMapFileEmpty(config_file))
    {
      config = createConfig() ;

      config->config_file = g_strdup(config_file) ;

      /* Get the configuration (may have to create a default one). */
      if (zMapFileEmpty(config->config_file))
	status = zmapMakeUserConfig(config) ;
      else
	status = zmapGetUserConfig(config) ;

      /* Clean up if we have failed. */
      if (!status)
	{
	  destroyConfig(config) ;
	  config = NULL ;
	}
    }

  return config ;
}


/*!
 * Return all instances of the spec_stanza found in the config, returned as a set of
 * stanzas in stanzas_out. Returns FALSE if none are found.
 *
 * Note that only those elements specified in the spec_stanza will be returned from
 * the configuration, elements not recognised will not be returned for any one stanza.
 * Note also that multiple stanzas may be returned.
 *
 * @param config       The configuration to be searched for the specified stanza.
 * @param spec_stanza  The stanza to look for in the configuration.
 * @param stanzas_out  The set of stanzas found that match spec_stanza, this is unaltered
 *                     if no matching stanzas were found.
 * @return             TRUE if any matching stanza found, FALSE otherwise.
 *  */
gboolean zMapConfigFindStanzas(ZMapConfig config,
			       ZMapConfigStanza spec_stanza, ZMapConfigStanzaSet *stanzas_out)
{
  gboolean result = FALSE ;

  /* There may be no stanzas, i.e. config file is empty. */
  if (config->stanzas)
    result = zmapGetConfigStanzas(config, spec_stanza, stanzas_out) ;

  return result ;
}


/*!
 * Destroy a config, should be called when the config is finished with.
 *
 * @param config       The config to be destroyed.
 * @return             nothing.
 *  */
void zMapConfigDestroy(ZMapConfig config)
{
  destroyConfig(config) ;

  return ;
}


/*! @} end of zmapconfig docs. */


/*
 *  ------------------- Internal functions -------------------
 */


static ZMapConfig createConfig(void)
{
  ZMapConfig config ;

  config = g_new0(ZMapConfigStruct, 1) ;

  config->config_file = NULL ;

  config->stanzas = NULL ;

  return config ;
}


static void destroyConfig(ZMapConfig config)
{
  if (config->config_file)
    g_free(config->config_file) ;

  g_free(config) ;

  return ;
}

