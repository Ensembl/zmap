/*  File: zmapConfig.h
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
 * Last edited: Jan 23 13:16 2004 (edgrif)
 * Created: Fri Jan 23 13:10:06 2004 (edgrif)
 * CVS info:   $Id: zmapConfig.h,v 1.2 2004-03-03 12:50:39 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONFIG_H
#define ZMAP_CONFIG_H

#include <glib.h>

/* Opaque type, contains config info. */
typedef struct _ZMapConfigStruct *ZMapConfig ;


ZMapConfig zMapConfigInit(char *config_file) ;


gboolean zMapConfigGetServers(ZMapConfig config, char ***servers) ;


void zMapConfigDelete(ZMapConfig config) ;


#endif /* !ZMAP_CONFIG_H */
