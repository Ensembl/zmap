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
 * Last edited: Apr 23 23:10 2004 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapConfig.c,v 1.5 2004-04-27 09:44:30 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <zmapConfig_P.h>


static ZMapConfig createConfig(void) ;
static void destroyConfig(ZMapConfig config) ;
static char *configGetDir(char *zmap_config_dir) ;
static char *configGetFile(char *zmap_dir_path, char *zmap_config_file) ;
static gboolean configFileEmpty(char *zmap_config_file) ;


/*! @defgroup zmapconfig   zMapConfig: configuration reading/writing
 * @{
 * 
 * \brief  Configuration File Handling.
 * 
 * zMapConfig routines read and write data from configuration files for ZMap.
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
 *      /* Make the "server" stanza so we can search for it in the configuration data. */
 *      ZMapConfigStanza server_stanza ;
 *      ZMapConfigStanzaElementStruct server_elements[] = {{"host", ZMAPCONFIG_STRING, {NULL}},
 *                                                         {"port", ZMAPCONFIG_INT, {NULL}},
 *	 	    		  	                   {"protocol", ZMAPCONFIG_STRING, {NULL}},
 *						           {NULL, -1, {NULL}}} ;
 *      server_elements[1].data.i = -1 ;
 *	server_stanza = zMapConfigMakeStanza("server", server_elements) ;
 *
 *      /* Find all the "server" stanzas and process them. */
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
 *	    /* clean up. */
 *	    if (server_list)
 *	      zMapConfigDeleteStanzaSet(server_list) ;
 *	  }
 * </PRE>
 *
 *
 *  */


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
	  new_element->data.s = g_strdup(elements[i].data.s) ;
	  break ;
	default:
	  zMapCrash("%s", "Code Error: unrecognised data type for stanza element union") ;
	  break ;
	}

      stanza->elements = g_list_append(stanza->elements, new_element) ;

      elements++ ;
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
      while (element_list = g_list_next(element_list)) ;
    }

  return result ;
}



/*!
 * Create a configuration which can then be searched for configuration stanzas.
 * Currently it takes an unused argument of a configuration filename,
 * I'M NOT SURE WE WANT TO PASS IN THE CONFIGURATION FILE NAME, THAT MIGHT BE TOO MUCH
 * FLEXIBILITY..........THINK ABOUT THIS....
 *
 * @param config_file_unused  Name of a configuration file to be read, currently UNUSED.
 * @return                    The configuration if all the configuration files etc. were
 *                            successfully read, NULL otherwise.
 *  */
ZMapConfig zMapConfigCreate(char *config_file_unused)
{
  ZMapConfig config ;
  gboolean status = TRUE ;

  config = createConfig() ;

  /* Get the config directory (may have to create one). */
  if (status)
    {
      if (!(config->config_dir = configGetDir(ZMAP_USER_CONFIG_DIR)))
	{
	  status = FALSE ;
	}
    }

  /* Get the config file (may have to create one). */
  if (status)
    {
      if (!(config->config_file = configGetFile(config->config_dir, ZMAP_CONFIG_FILE)))
	{
	  status = FALSE ;
	}
    }

  /* Get the configuration (may have to create a default one). */
  if (status)
    {
      if (configFileEmpty(config->config_file))
	{
	  status = zmapMakeUserConfig(config) ;
	}
      else
	{
	  status = zmapGetUserConfig(config) ;
	}
    }


  /* Clean up if we have failed. */
  if (!status)
    {
      destroyConfig(config) ;
      g_free(config) ;
      config = NULL ;
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
  gboolean result = TRUE ;

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

  g_free(config) ;

  return ;
}


/*! @} end of zmapconfig docs. */


/*
 *  ------------------- Internal functions -------------------
 */


static ZMapConfig createConfig(void)
{
  ZMapConfig config ;

  config = g_new(ZMapConfigStruct, sizeof(ZMapConfigStruct)) ;

  config->config_dir = config->config_file = NULL ;

  config->stanzas = NULL ;

  return config ;
}


static void destroyConfig(ZMapConfig config)
{

  if (config->config_dir)
    g_free(config->config_dir) ;

  if (config->config_file)
    g_free(config->config_file) ;

  return ;
}


/* Does the user have a configuration directory for ZMap. */
static char *configGetDir(char *zmap_config_dir)
{
  char *config_dir = NULL ;
  gboolean status = FALSE ;
  const char *home_dir ;
  struct stat stat_buf ;

  home_dir = g_get_home_dir() ;
  config_dir = g_build_path(ZMAP_SEPARATOR, home_dir, zmap_config_dir, NULL) ;

  /* Is there a config dir in the users home directory which is readable/writeable/executable ? */
  if (stat(config_dir, &stat_buf) == 0)
    {
      if (S_ISDIR(stat_buf.st_mode) && (stat_buf.st_mode & S_IRWXU))
	{
	  status = TRUE ;
	}
    }
  else
    {
      if (mkdir(config_dir, S_IRWXU) == 0)
	{
	  status = TRUE ;
	}
    }

  if (!status)
    {
      g_free(config_dir) ;
      config_dir = NULL ;
    }

  return config_dir ;
}



/* Does the user have a configuration file for ZMap. */
static char *configGetFile(char *zmap_dir_path, char *zmap_config_file)
{
  gboolean status = FALSE ;
  char *config_file ;
  struct stat stat_buf ;

  config_file = g_build_path(ZMAP_SEPARATOR, zmap_dir_path, zmap_config_file, NULL) ;

  /* Is there a configuration file in the config dir that is readable/writeable ? */
  if (stat(config_file, &stat_buf) == 0)
    {
      if (S_ISREG(stat_buf.st_mode) && (stat_buf.st_mode & S_IRWXU))
	{
	  status = TRUE ;
	}
    }
  else
    {
      int file ;

      if ((file = open(config_file, (O_RDWR | O_CREAT | O_EXCL), (S_IRUSR | S_IWUSR)) != -1)
	  && (close(file) != -1))
	status = TRUE ;
    }

  if (!status)
    {
      g_free(config_file) ;
      config_file = NULL ;
    }

  return config_file ;
}


/* Is the config file empty ? */
static gboolean configFileEmpty(char *zmap_config_file)
{
  gboolean result = FALSE ;
  struct stat stat_buf ;

  if (stat(zmap_config_file, &stat_buf) == 0)
    {
      if (stat_buf.st_size == 0)
	{
	  result = TRUE ;
	}
    }

  return result ;
}



