/*  File: zmapWriteConfig.c
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
 * Exported functions: See zmapConfig_P.h
 * HISTORY:
 * Last edited: Nov 11 15:26 2004 (edgrif)
 * Created: Thu Apr  1 14:37:42 2004 (edgrif)
 * CVS info:   $Id: zmapConfigWrite.c,v 1.4 2006-11-08 09:23:48 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <zmapConfig_P.h>





/* When we do this we need to write version information in the header of the stanza file
 * AND get this parsed by the code to check for versions.
 * 
 * The version code should go in Utils.c I think and we can crib the macros from those
 * I used for acedb, including put the version string in the executable. */
gboolean zmapMakeUserConfig(ZMapConfig config)
{
  gboolean result = FALSE ;
  GIOChannel *config_file ;
  GError *gerror = NULL ;
  char *format_str = "##       %s Configuration File\n"
    "## date - %s\n"
    "## version - %s\n" ;



  if ((config_file = g_io_channel_new_file(config->config_file, "r+", &gerror)))
    {
      gsize bytes_written = 0 ;
      gchar *config_header ;

      config_header = g_strdup_printf(format_str, zMapGetAppName(), __DATE__ " " __TIME__,
				      zMapGetVersionString()) ;

      if (g_io_channel_write_chars(config_file, config_header, -1,
				   &bytes_written, &gerror) == G_IO_STATUS_NORMAL
	  && g_io_channel_shutdown(config_file, TRUE, &gerror) == G_IO_STATUS_NORMAL)
	result = TRUE ;
      else
	result = FALSE ;

      g_free(config_header) ;
    }

  return result ;
}

