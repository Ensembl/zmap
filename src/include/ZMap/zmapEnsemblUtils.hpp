/*  File: zmapEnsemblUtils.h
 *  Author: Gemma Guest (gb10@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
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
