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
 * Last edited: Mar 22 13:03 2004 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapConfig.c,v 1.3 2004-03-22 13:18:19 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapConfig_P.h>


/* Initialise the configuration manager. */
ZMapConfig zMapConfigInit(char *config_file)
{
  ZMapConfig config ;

  /* assert for non-null config_file.... */

  config = g_new(ZMapConfigStruct, sizeof(ZMapConfigStruct)) ;

  config->config_file = g_strdup(config_file) ;

  return config ;
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
void zMapConfigDelete(ZMapConfig config)
{
  g_free(config->config_file) ;

  g_free(config) ;

  return ;
}


/*
 *  ------------------- Internal functions -------------------
 */


