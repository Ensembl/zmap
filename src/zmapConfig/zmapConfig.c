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
 * Last edited: Apr  7 11:23 2004 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapConfig.c,v 1.4 2004-04-08 16:45:08 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <zmapConfig_P.h>


static ZMapConfig createConfig(void) ;
static void destroyConfig(ZMapConfig config) ;
static char *configGetDir(char *zmap_config_dir) ;
static char *configGetFile(char *zmap_dir_path, char *zmap_config_file) ;
static gboolean configFileEmpty(char *zmap_config_file) ;



/* Construct a stanza from a name and an array of elements, helps a lot because both
 * can just be statically declared in the code and then passed to this routine which
 * then makes the corresponding stanza. */
ZMapConfigStanza zMapConfigMakeStanza(char *stanza_name, ZMapConfigStanzaElement elements)
{
  ZMapConfigStanza stanza ;
  int i ;

  stanza = zmapConfigCreateStanza(stanza_name) ;

  while (elements->name != NULL)
    {
      ZMapConfigStanzaElement new_element ;

      new_element = zmapConfigCreateElement(elements->name, elements->type) ;

      switch(elements->type)
	{
	case ZMAPCONFIG_BOOL:
	  new_element->data.b = elements->data.b ;
	  break ;
	case ZMAPCONFIG_INT:
	  new_element->data.i = elements->data.i ;
	  break ;
	case ZMAPCONFIG_FLOAT:
	  new_element->data.f = elements->data.f ;
	  break ;
	case ZMAPCONFIG_STRING:
	  new_element->data.s = g_strdup(elements->data.s) ;
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

/* Can use this to iterate through a list of stanzas, if current_stanza is NULL then the first
 * stanza is returned. */
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





/* Initialise the configuration manager.
 * I'M NOT SURE WE WANT TO PASS IN THE CONFIGURATION FILE NAME, THAT MIGHT BE TOO MUCH
 * FLEXIBILITY.......... */
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


/* We should return an array of ptrs that it is the caller responsibility to free, the actual
 * array elements will remain always there, we just provide pointers into the array.... */
gboolean zMapConfigGetStanzas(ZMapConfig config,
			      ZMapConfigStanza stanza, ZMapConfigStanzaSet *stanzas_out)
{
  gboolean result = TRUE ;

  result = zmapGetConfigStanzas(config, stanza, stanzas_out) ;

  return result ;
}

/* Hacked up currently, these values would in reality be extracted from a configuration file
 * and loaded into memory which would then become a dynamic list of servers that could be
 * added into interactively. */
gboolean zMapConfigGetServers(ZMapConfig config, char ***servers)
{
  gboolean result = TRUE ;
  static char *my_servers[] = {"griffin2 20000 acedb",
			       "griffin2 20001 acedb",
			       "http://dev.acedb.org/das/ 0 das",
			       NULL} ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* for debugging it is often good to have only one thread running. */

  static char *my_servers[] = {"griffin2 20000 acedb",
			       NULL} ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




  *servers = my_servers ;

  return result ;
}



/* Delete of a config, will require freeing some members of the structure. */
void zMapConfigDestroy(ZMapConfig config)
{
  destroyConfig(config) ;

  g_free(config) ;

  return ;
}


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



