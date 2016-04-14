/*  File: trackhubServer_P.hpp
 *  Author: Gemma Barson (gb10@sanger.ac.uk)
 *  Copyright (c) 2016: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *-------------------------------------------------------------------
 */
#ifndef TRACKHUB_SERVER_P_H
#define TRACKHUB_SERVER_P_H

#include <gbtools/gbtools.hpp>


typedef struct _TrackhubServerStruct
{
  char *config_file ;

  gbtools::trackhub::Registry registry ; /* Class to access Track Hub Registry functions*/
  char *trackdb_id ;                     /* Track database ID to fetch tracks from */

  /* Results of server requests. */
  ZMapServerResponseType result ;
  gboolean error ;			 /* TRUE if any error occurred. */
  char *last_err_msg ;

  ZMapFeatureContext req_context ;

  char *sequence ;                       /* request sequence name for our one block */
  gint zmap_start, zmap_end ;		 /* request coordinates for our one block */

  gboolean req_featuresets_only ;        /* if true, only load req fsets */
  GList *req_featuresets ;               /* requested featuresets */
  GList *req_biotypes ;                  /* requested biotypes */
  GHashTable *source_2_sourcedata ;
  GHashTable *featureset_2_column ;

} TrackhubServerStruct, *TrackhubServer;


#endif /* !TRACKHUB_SERVER_P_H */
