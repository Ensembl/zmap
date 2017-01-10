/*  File: zmapEnsemblUtils.h
 *  Author: Gemma Guest (gb10@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Utilities for querying ensembl databases.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_ENSEMBL_UTILS_H
#define ZMAP_ENSEMBL_UTILS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#ifdef USE_ENSEMBL
#include <list>
#include <string>

std::list<std::string> zMapEnsemblGetDatabaseList(const char *host, 
                                                  const int port,
                                                  const char *user,
                                                  const char *pass,
                                                  GError **error) ;

std::list<std::string> zMapEnsemblGetFeaturesetsList(const char *host, 
                                                     const int port,
                                                     const char *user,
                                                     const char *pass,
                                                     const char *dbname,
                                                     GError **error) ;

std::list<std::string> zMapEnsemblGetBiotypesList(const char *host, 
                                                  const int port,
                                                  const char *user,
                                                  const char *pass,
                                                  const char *dbname,
                                                  GError **error) ;


#endif /* USE_ENSEMBL */

#endif /* !ZMAP_ENSEMBL_UTILS_H */
