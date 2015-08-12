/*  File: ensemblServer_P.h
 *  Author: Gemma Barson (gb10@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
#ifndef ENSEMBL_SERVER_P_H
#define ENSEMBL_SERVER_P_H

/* ensc-core is compiled as C */
#ifdef __cplusplus
extern "C" {
#endif

#include <DBAdaptor.h>
#include <SliceAdaptor.h>

#ifdef __cplusplus
}
#endif


typedef struct _EnsemblServerStruct
{
  char *config_file ;

  /* Connection details. */
  DBAdaptor *dba ;
  SliceAdaptor *slice_adaptor ;
  SequenceAdaptor *seq_adaptor ;
  const char* coord_system ;
  Slice *slice ;
  GMutex* mutex ;                                           /* lock to protect ensc-core library
                                                             * calls as they are not thread safe,
                                                             * i.e. anything using dba, slice etc. */

  char *host ;
  int port ;
  char *user ;
  char *passwd ;
  char *db_name ;
  char *db_prefix ;

  /* Results of server requests. */
  ZMapServerResponseType result ;
  gboolean error ;					    /* TRUE if any error occurred. */
  char *last_err_msg ;

  ZMapFeatureContext req_context ;

  char *sequence ;                                          /* request sequence name for our one block */
  gint zmap_start, zmap_end ;				    /* request coordinates for our one block */

  gboolean req_featuresets_only ;                           /* if true, only load req fsets */
  GList *req_featuresets ;                                  /* requested featuresets */
  GHashTable *source_2_sourcedata ;
  GHashTable *featureset_2_column ;

} EnsemblServerStruct, *EnsemblServer;


#endif /* !ENSEMBL_SERVER_P_H */
