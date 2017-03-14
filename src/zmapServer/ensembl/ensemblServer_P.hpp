/*  File: ensemblServer_P.h
 *  Author: Gemma Barson (gb10@sanger.ac.uk)
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
  ZMapConfigSource source ;

  // Lock shared between all ensembl data sources to protect ensc-core library calls as they are
  // not thread safe, i.e. anything using dba, slice etc.
  pthread_mutex_t *mutex ;


  /* Connection details. */
  DBAdaptor *dba ;
  SliceAdaptor *slice_adaptor ;
  SequenceAdaptor *seq_adaptor ;
  const char* coord_system ;
  Slice *slice ;

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
  GList *req_biotypes ;                                     /* requested biotypes */
  GHashTable *source_2_sourcedata ;
  GHashTable *featureset_2_column ;

} EnsemblServerStruct, *EnsemblServer;


#endif /* !ENSEMBL_SERVER_P_H */
