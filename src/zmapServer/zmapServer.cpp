/*  File: zmapServer.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * and was written by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 * Exported functions: See ZMap/zmapServer.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>

#include <config.h>
#include <ZMap/zmapUrl.hpp>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <zmapServer_P.hpp>


static void zMapServerSetErrorMsg(ZMapServer server,char *message);
static gboolean server_functions_valid(ZMapServerFuncs serverfuncs ) ;


/* We need matching serverInit and serverCleanup functions that are only called once
 * for each server type, libcurl needs this to avoid memory leaks but maybe this is not
 * such an issue for us as its once per program run...... */


/* This routine must be called before any other server routine and must only be called once,
 * it is the callers responsibility to make sure this is true....otherwise results are
 * undefined and may result in a crash.
 *
 * If the function returns NULL then no servers can be called.
 */
gboolean zMapServerGlobalInit(ZMapURL url, void **server_global_data_out)
{
  gboolean result = FALSE ;
  ZMapServerFuncs serverfuncs ;

  zMapReturnValIfFail((server_global_data_out), FALSE) ;

  zMapReturnValIfFail((url->scheme == SCHEME_FILE
                       || url->scheme == SCHEME_TRACKHUB
                       || url->scheme == SCHEME_PIPE 
                       || url->scheme == SCHEME_HTTP 
                       || url->scheme == SCHEME_FTP
#ifdef HAVE_SSL
                       || url->scheme == SCHEME_HTTPS 
#endif
#ifdef USE_ACECONN
                       || url->scheme == SCHEME_ACEDB
#endif
#ifdef USE_ENSEMBL
                       || url->scheme == SCHEME_ENSEMBL
#endif
                       ),
                      FALSE) ;

  serverfuncs = g_new0(ZMapServerFuncsStruct, 1) ;

  /* Set up the server according to the protocol, this is all a bit hard coded but it
   * will do for now.... */
  /* Probably I should do this with a table of protocol and function stuff...perhaps
   * even using dynamically constructed function names....  */
  switch(url->scheme)
    {
    case SCHEME_FILE:     // file only now...
      fileGetServerFuncs(serverfuncs) ;
      break ;

    case SCHEME_PIPE:
      pipeGetServerFuncs(serverfuncs);
      break;

#ifdef HAVE_SSL
    case SCHEME_HTTPS: // fall through
#endif
    case SCHEME_HTTP: // fall through
    case SCHEME_FTP:
      fileGetServerFuncs(serverfuncs) ;
      break;

#ifdef USE_ACECONN
    case SCHEME_ACEDB:
      acedbGetServerFuncs(serverfuncs) ;
      break;
#endif

#ifdef USE_ENSEMBL
    case SCHEME_ENSEMBL:
      ensemblGetServerFuncs(serverfuncs);
      break;
#endif

    default:
      /* We should not get here if zMapReturnValIfFail() check worked. */
      zMapWarnIfReached() ;
      break;
    }

  /* All functions MUST be specified. */
  if (server_functions_valid(serverfuncs))
    {
      /* Call the global init function. */
      result = (serverfuncs->global_init)() ;

      if (result)
        *server_global_data_out = (void *)serverfuncs ;
    }

  if (!result)
    g_free(serverfuncs) ;


  return result ;
}


// Make the server connection, note that a server is _always_created here even if there is an
// error. What this means is that if the caller does not pass in server_out this code will
// segfault, not a great design but there's not point in changing it now.
ZMapServerResponseType zMapServerCreateConnection(ZMapServer *server_out, void *global_data,
                                                  ZMapConfigSource config_source)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  ZMapServer server ;
  ZMapServerFuncs serverfuncs = (ZMapServerFuncs)global_data ;
  int parse_error ;

  // If there's no server_out we will segfault...would be better to throw here but would need a rewrite.
  server = g_new0(ZMapServerStruct, 1) ;
  *server_out = server ;

  if (result == ZMAP_SERVERRESPONSE_OK)
    {
      if (serverfuncs)
        {
          /* set function table. */
          server->funcs = serverfuncs ;
        }
      else
        {
          char *err_msg ;

          result = ZMAP_SERVERRESPONSE_BADREQ ;
          err_msg = g_strdup("No server callbacks specified.") ;
          zMapServerSetErrorMsg(server, err_msg) ;
        }
    }

  if (result == ZMAP_SERVERRESPONSE_OK)
    {
      if (!config_source->urlObj() || !(config_source->urlObj()->url))
        {
          char *err_msg ;

          result = ZMAP_SERVERRESPONSE_BADREQ ;
          err_msg = g_strdup("No server url specified.") ;
          zMapServerSetErrorMsg(server, err_msg) ;
        }
      else
        {
          /* COPY/REPARSE the url into server... with paranoia as it managed to parse 1st time! */
          if (!(server->url = url_parse(config_source->urlObj()->url, &parse_error)))
            {
              result = ZMAP_SERVERRESPONSE_REQFAIL ;
              zMapServerSetErrorMsg(server, ZMAPSERVER_MAKEMESSAGE(config_source->urlObj()->protocol,
                                                                   config_source->urlObj()->host, "%s",
                                                                   url_error(parse_error))) ;
            }
        }
    }

  if (result == ZMAP_SERVERRESPONSE_OK)
    {
      server->config_file = g_strdup(config_source->configFile()) ;
    }

  if (result == ZMAP_SERVERRESPONSE_OK)
    {
      if ((server->funcs->create)(&(server->server_conn), config_source))
        {
          zMapServerSetErrorMsg(server, NULL) ;
          result = ZMAP_SERVERRESPONSE_OK ;
        }
      else
        {
          zMapServerSetErrorMsg(server,ZMAPSERVER_MAKEMESSAGE(server->url->protocol,
                                                              server->url->host, "%s",
                                                              (server->funcs->errmsg)(server->server_conn))) ;
          result = ZMAP_SERVERRESPONSE_REQFAIL ;
        }
    }

  server->last_response  = result ;

  return result ;
}


ZMapServerResponseType zMapServerOpenConnection(ZMapServer server, ZMapServerReqOpen req_open)
{
  ZMapServerResponseType result = server->last_response ;

  result = server->last_response = (server->funcs->open)(server->server_conn, req_open) ;

  if (result != ZMAP_SERVERRESPONSE_OK)
    zMapServerSetErrorMsg(server,ZMAPSERVER_MAKEMESSAGE(server->url->protocol,
                                                        server->url->host, "%s",
                                                        (server->funcs->errmsg)(server->server_conn))) ;

  return result ;
}



ZMapServerResponseType zMapServerFeatureSetNames(ZMapServer server,
                                                 GList **feature_sets_inout,
                                                 GList **biotypes_inout,
                                                 GList *sources,
                                                 GList **required_styles_out,
                                                 GHashTable **featureset_2_stylelist_out,
                                                 GHashTable **featureset_2_column_out,
                                                 GHashTable **source_2_sourcedata_out)
{
  ZMapServerResponseType result = server->last_response ;

  zMapReturnValIfFail((server && !*required_styles_out), ZMAP_SERVERRESPONSE_BADREQ) ;


  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED && server->last_response != ZMAP_SERVERRESPONSE_REQFAIL)
    {
      result = server->last_response
        = (server->funcs->feature_set_names)(server->server_conn,
                                             feature_sets_inout,
                                             biotypes_inout,
                                             sources,
                                             required_styles_out,
                                             featureset_2_stylelist_out,
                                             featureset_2_column_out,
                                             source_2_sourcedata_out) ;

      if (result != ZMAP_SERVERRESPONSE_OK)
        zMapServerSetErrorMsg(server, ZMAPSERVER_MAKEMESSAGE(server->url->protocol,
                                                             server->url->host, "%s",
                                                             (server->funcs->errmsg)(server->server_conn))) ;
    }
  return result ;
}


ZMapServerResponseType zMapServerGetStyles(ZMapServer server, GHashTable **styles_out)
{
  ZMapServerResponseType result = server->last_response ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED && server->last_response != ZMAP_SERVERRESPONSE_REQFAIL)
    {
      result = server->last_response = (server->funcs->get_styles)(server->server_conn, styles_out) ;

      if (result != ZMAP_SERVERRESPONSE_OK)
        zMapServerSetErrorMsg(server, ZMAPSERVER_MAKEMESSAGE(server->url->protocol,
                                                             server->url->host, "%s",
                                                             (server->funcs->errmsg)(server->server_conn))) ;
    }
  return result ;
}


ZMapServerResponseType zMapServerStylesHaveMode(ZMapServer server, gboolean *have_mode)
{
  ZMapServerResponseType result = server->last_response ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED && server->last_response != ZMAP_SERVERRESPONSE_REQFAIL)
    {
      result = server->last_response = (server->funcs->have_modes)(server->server_conn, have_mode) ;

      if (result != ZMAP_SERVERRESPONSE_OK)
        zMapServerSetErrorMsg(server,ZMAPSERVER_MAKEMESSAGE(server->url->protocol,
                                                            server->url->host, "%s",
                                                            (server->funcs->errmsg)(server->server_conn))) ;
    }
  return result ;
}


ZMapServerResponseType zMapServerGetStatus(ZMapServer server, gint *exit_code)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  /* always do this, it must return a died status if appropriate */
  result = server->last_response = (server->funcs->get_status)(server->server_conn, exit_code) ;

  if (result != ZMAP_SERVERRESPONSE_OK)
    zMapServerSetErrorMsg(server, ZMAPSERVER_MAKEMESSAGE(server->url->protocol,
                                                         server->url->host, "%s",
                                                         (server->funcs->errmsg)(server->server_conn))) ;

  return result ;
}



ZMapServerResponseType zMapServerGetConnectState(ZMapServer server, ZMapServerConnectStateType *connect_state)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  result = server->last_response = (server->funcs->get_connect_state)(server->server_conn, connect_state) ;

  if (result != ZMAP_SERVERRESPONSE_OK)
    zMapServerSetErrorMsg(server, ZMAPSERVER_MAKEMESSAGE(server->url->protocol,
                                                         server->url->host, "%s",
                                                         (server->funcs->errmsg)(server->server_conn))) ;

  return result ;
}



ZMapServerResponseType zMapServerGetSequence(ZMapServer server, GList *sequences_inout)
{
  ZMapServerResponseType result = server->last_response ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED && server->last_response != ZMAP_SERVERRESPONSE_REQFAIL)
    {

      result = server->last_response = (server->funcs->get_sequence)(server->server_conn, sequences_inout) ;

      if (result != ZMAP_SERVERRESPONSE_OK)
        zMapServerSetErrorMsg(server, ZMAPSERVER_MAKEMESSAGE(server->url->protocol,
                                                             server->url->host, "%s",
                                                             (server->funcs->errmsg)(server->server_conn))) ;
    }
  return result ;
}


ZMapServerResponseType zMapServerGetServerInfo(ZMapServer server, ZMapServerReqGetServerInfo info)
{
  ZMapServerResponseType result = server->last_response ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED && server->last_response != ZMAP_SERVERRESPONSE_REQFAIL)
    {

      result = server->last_response = (server->funcs->get_info)(server->server_conn, info) ;

      if (result != ZMAP_SERVERRESPONSE_OK)
        zMapServerSetErrorMsg(server, ZMAPSERVER_MAKEMESSAGE(server->url->protocol,
                                                             server->url->host, "%s",
                                                             (server->funcs->errmsg)(server->server_conn))) ;
    }
  return result ;
}


ZMapServerResponseType zMapServerSetContext(ZMapServer server, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = server->last_response ;

  zMapReturnValIfFail((feature_context), ZMAP_SERVERRESPONSE_BADREQ) ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED && server->last_response != ZMAP_SERVERRESPONSE_REQFAIL)
    {
      result = server->last_response
        = (server->funcs->set_context)(server->server_conn, feature_context) ;

      if (result != ZMAP_SERVERRESPONSE_OK)
        zMapServerSetErrorMsg(server, ZMAPSERVER_MAKEMESSAGE(server->url->protocol,
                                                             server->url->host, "%s",
                                                             (server->funcs->errmsg)(server->server_conn))) ;
    }

  return result ;
}


ZMapServerResponseType zMapServerGetFeatures(ZMapServer server,
                                             ZMapStyleTree &styles, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = server->last_response ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED  && server->last_response != ZMAP_SERVERRESPONSE_REQFAIL)
    {
      result = server->last_response
        = (server->funcs->get_features)(server->server_conn, styles, feature_context) ;

      if (result != ZMAP_SERVERRESPONSE_OK)
        zMapServerSetErrorMsg(server, ZMAPSERVER_MAKEMESSAGE(server->url->protocol,
                                                             server->url->host, "%s",
                                                             (server->funcs->errmsg)(server->server_conn))) ;
    }

  return result ;
}


ZMapServerResponseType zMapServerGetContextSequences(ZMapServer server, ZMapStyleTree &styles,
                                                     ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = server->last_response ;   // Can be called after a previous failure.


  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED && server->last_response != ZMAP_SERVERRESPONSE_REQFAIL)
    {
      char *sequence_name ;
      int start, end, dna_length = 0 ;
      char *dna_sequence = NULL ;
      ZMapFeatureAlignment align ;
      ZMapFeatureBlock feature_block ;

      // HACK.....in the acedbServer code it went through all the aligns and all the blocks of
      // an align to get all the DNA for all displayed blocks....we never really used that so
      // here I'm hacking to get the first align and the first block which is all we really use.
      align = feature_context->master_align ;
      feature_block = (ZMapFeatureBlock)zMap_g_hash_table_nth(align->blocks, 0) ;

      sequence_name = (char *)g_quark_to_string(feature_context->sequence_name) ;
      start = feature_block->block_to_sequence.block.x1 ;
      end = feature_block->block_to_sequence.block.x2 ;


      result = server->last_response
        = (server->funcs->get_context_sequences)(server->server_conn,
                                                 sequence_name, start, end,
                                                 &dna_length, &dna_sequence) ;

      if (result != ZMAP_SERVERRESPONSE_OK)
        {
          zMapServerSetErrorMsg(server, ZMAPSERVER_MAKEMESSAGE(server->url->protocol,
                                                               server->url->host, "%s",
                                                               (server->funcs->errmsg)(server->server_conn))) ;
        }
      else
        {
          ZMapFeature feature = NULL;
          ZMapFeatureSet feature_set = NULL;
          ZMapFeatureTypeStyle dna_style = NULL;


          /* dna, 3-frame and feature translation are all created up front. (BUT I don't know why
           * it was done this way - Ed) */

          GList *req_sets = feature_context->req_feature_set_names ;
          GQuark dna_quark = zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME) ;
          GQuark threeft_quark = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME) ;
          GQuark orf_quark = zMapStyleCreateID(ZMAP_FIXED_STYLE_ORF_NAME) ;
          GQuark showtrans_quark = zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME) ;

          /* We need the DNA if we are showing DNA, 3FT or ShowTranslation */
          if (zMap_g_list_find_quark(req_sets, dna_quark) ||
              zMap_g_list_find_quark(req_sets, threeft_quark) ||
              zMap_g_list_find_quark(req_sets, showtrans_quark))
            {
              if (zMapFeatureDNACreateFeatureSet(feature_block, &feature_set))
                {
                  if ((dna_style = styles.find_style(dna_quark)))
                    feature = zMapFeatureDNACreateFeature(feature_block, dna_style, dna_sequence, dna_length);
                }
            }

          if (zMap_g_list_find_quark(feature_context->req_feature_set_names, threeft_quark))
            {
              ZMapFeatureSet translation_fs = NULL;

              if ((zMapFeature3FrameTranslationCreateSet(feature_block, &feature_set)))
                {
                  translation_fs = feature_set;
                  ZMapFeatureTypeStyle frame_style = NULL;

                  if((frame_style = styles.find_style(threeft_quark)))
                    zMapFeature3FrameTranslationSetCreateFeatures(feature_set, frame_style);
                }

              if ((zMapFeatureORFCreateSet(feature_block, &feature_set)))
                {
                  ZMapFeatureTypeStyle orf_style = NULL;

                  if ((orf_style = styles.find_style(orf_quark)))
                    zMapFeatureORFSetCreateFeatures(feature_set, orf_style, translation_fs);
                }
            }


          /* I'm going to create the show translation up front! */
          if (zMap_g_list_find_quark(feature_context->req_feature_set_names, showtrans_quark))
            {
              if ((zMapFeatureShowTranslationCreateSet(feature_block, &feature_set)))
                {
                  ZMapFeatureTypeStyle frame_style = NULL;

                  if ((frame_style = styles.find_style(showtrans_quark)))
                    zMapFeatureShowTranslationSetCreateFeatures(feature_set, frame_style) ;
                }
            }
        }
      

    }

  return result ;
}



ZMapServerResponseType zMapServerCloseConnection(ZMapServer server)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  zMapReturnValIfFail(server, ZMAP_SERVERRESPONSE_REQFAIL) ;

  result = server->last_response
    = (server->funcs->close)(server->server_conn) ;

  if (result != ZMAP_SERVERRESPONSE_OK)
    zMapServerSetErrorMsg(server,ZMAPSERVER_MAKEMESSAGE((server->url ? server->url->protocol : ""),
                                                         (server->url ? server->url->host : ""), "%s",
                                                        (server->funcs->errmsg)(server->server_conn))) ;

  return result ;
}

ZMapServerResponseType zMapServerFreeConnection(ZMapServer server)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  /* This function is a bit different, as we free the server struct the only thing we can do
   * is return the status from the destroy call. */
  result = (server->funcs->destroy)(server->server_conn) ;

  /* Free ZMapURL!!!! url_free(server->url)*/
  
  if (server->config_file)
    g_free(server->config_file) ;

  g_free(server) ;

  return result ;
}


const char *zMapServerLastErrorMsg(ZMapServer server)
{
  return server->last_error_msg ;
}


static void zMapServerSetErrorMsg(ZMapServer server, char *message)
{
  if (server->last_error_msg)
    g_free(server->last_error_msg) ;

  server->last_error_msg = message ;

  return ;
}


/*
 *                  Internal routines.
 */

static gboolean server_functions_valid(ZMapServerFuncs serverfuncs )
{
  gboolean result ;

  result = (serverfuncs->global_init
            && serverfuncs->create
            && serverfuncs->open
            && serverfuncs->get_info
            && serverfuncs->feature_set_names
            && serverfuncs->get_styles
            && serverfuncs->have_modes
            && serverfuncs->get_sequence
            && serverfuncs->set_context
            && serverfuncs->get_features
            && serverfuncs->get_context_sequences
            && serverfuncs->errmsg
            && serverfuncs->get_status
            && serverfuncs->close
            && serverfuncs->destroy) ;

  return result ;
}



