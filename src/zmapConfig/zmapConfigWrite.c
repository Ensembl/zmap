/*  File: zmapWriteConfig.c
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
 * Last edited: Apr  2 13:18 2004 (edgrif)
 * Created: Thu Apr  1 14:37:42 2004 (edgrif)
 * CVS info:   $Id: zmapConfigWrite.c,v 1.1 2004-04-08 16:40:36 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <zmapConfig_P.h>


/* When we do this we need to write version information in the header of the stanza file
 * AND get this parsed by the code to check for versions.
 * 
 * The version code should go in Utils.c I think and we can crib the macros from those
 * I used for acedb, including put the version string in the executable. */
gboolean zmapMakeUserConfig(ZMapConfig config)
{
  gboolean result = TRUE ;





  return result ;
}

