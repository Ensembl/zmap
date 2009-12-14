/*  File: zmapConfigLoader.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * HISTORY:
 * Last edited: Oct 28 12:14 2008 (edgrif)
 * Created: Tue Aug 26 12:39:42 2008 (rds)
 * CVS info:   $Id: zmapConfigLoader.h,v 1.5 2009-12-14 11:45:16 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_CONFIG_LOADER_H
#define ZMAP_CONFIG_LOADER_H

#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapConfigStanzaStructs.h>


ZMapConfigIniContext zMapConfigIniContextProvide() ;
ZMapConfigIniContext zMapConfigIniContextProvideNamed(char *stanza_name) ;

GList *zMapConfigIniContextGetSources(ZMapConfigIniContext context) ;
GList *zMapConfigIniContextGetNamed(ZMapConfigIniContext context, char *stanza_name) ;
GList *zMapConfigIniContextGetStyleList(ZMapConfigIniContext context,char *styles_list_in);

gboolean zMapConfigIniGetStylesFromFile(char *styles_list, char *styles_file, GData **styles_out);

void zMapConfigSourcesFreeList(GList *config_sources_list) ;
void zMapConfigStylesFreeList(GList *config_styles_list) ;

#endif /* ZMAP_CONFIG_LOADER_H */
