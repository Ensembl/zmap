/*  File: zmapAppServices_P.hpp
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
 * Description:
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_APP_SERVICES_P_HPP
#define ZMAP_APP_SERVICES_P_HPP


/*
 * We follow glib convention in error domain naming:
 *          "The error domain is called <NAMESPACE>_<MODULE>_ERROR"
 */
#define ZMAP_APP_SERVICES_ERROR g_quark_from_string("ZMAP_APP_SERVICES_ERROR")

typedef enum
{
  ZMAPAPPSERVICES_ERROR_BAD_COORDS,
  ZMAPAPPSERVICES_ERROR_NO_SEQUENCE,
  ZMAPAPPSERVICES_ERROR_CONFLICT_DATASET,
  ZMAPAPPSERVICES_ERROR_CONFLICT_SEQUENCE,
  ZMAPAPPSERVICES_ERROR_BAD_SEQUENCE_NAME,
  ZMAPAPPSERVICES_ERROR_DATABASE,
  ZMAPAPPSERVICES_ERROR_NO_SOURCES,
  ZMAPAPPSERVICES_ERROR_CHECK_FILE,
  ZMAPAPPSERVICES_ERROR_CONTEXT,
  ZMAPAPPSERVICES_ERROR_URL
} ZMapAppServicesError;

#endif /* ZMAP_APP_SERVICES_P_HPP */
