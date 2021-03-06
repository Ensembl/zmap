/*  File: ensemblUtils.cpp
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
 * Description: Implements the method functions for a zmap server
 *              by making the appropriate calls to the ensembl server.
 *
 * Exported functions: 
 *-------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#ifdef USE_ENSEMBL

#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <mysql.h>

#include <ZMap/zmapEnsemblUtils.hpp>
#include <ZMap/zmapUtilsDebug.hpp>

using namespace std ;


// Unnamed namespace for internal linkage
namespace {

/*
 * We follow glib convention in error domain naming:
 *          "The error domain is called <NAMESPACE>_<MODULE>_ERROR"
 */
#define ZMAP_ENSEMBLUTILS_ERROR g_quark_from_string("ZMAP_ENSEMBLUTILS_ERROR")

/*
 * Some error types for use in the view
 */
typedef enum
{
  ZMAPENSEMBLUTILS_ERROR_CONNECT,
  ZMAPENSEMBLUTILS_ERROR_RESULTS
} ZMapEnsemblUtilsError ;



list<string> ensemblGetList(const char *host,
                            const int port,
                            const char *user,
                            const char *passwd,
                            const char *dbname,
                            const string &query,
                            GError **error)
{
  list<string> result ;
  zMapReturnValIfFail(host, result) ;

  pthread_mutex_t mutex ;
  pthread_mutex_init(&mutex, NULL) ;
  pthread_mutex_lock(&mutex) ;

  MYSQL *mysql = mysql_init(NULL) ;

  /* on unix, localhost will connect via a local socket by default and ignore the port. If the
   * user has specified a port then force mysql to use it by setting the protocol to TCP (this is
   * necessary if using ssh tunnels). */
  if (mysql && host && port && !strcmp(host, "localhost"))
    {
      const mysql_protocol_type arg = MYSQL_PROTOCOL_TCP ;
      const int status = mysql_options(mysql, MYSQL_OPT_PROTOCOL, &arg) ;

      if (status != 0)
        zMapLogWarning("Error setting mysql protocol to TCP for %s:%d", host, port) ;
    }

  if (mysql)
    {
      mysql_real_connect(mysql, host, user, passwd, dbname, port, NULL, 0) ;
    }

  if (mysql)
    {
      mysql_real_query(mysql, query.c_str(), query.size()) ;

      MYSQL_RES *mysql_results = mysql_store_result(mysql) ;

      if (mysql_results)
        {
          MYSQL_ROW mysql_row = mysql_fetch_row(mysql_results) ;

          while (mysql_row)
            {
              string string_val(mysql_row[0]) ;
              result.push_back(string_val) ;

              mysql_row = mysql_fetch_row(mysql_results) ;
            }
        }
      else
        {
          if (dbname)
            g_set_error(error, ZMAP_ENSEMBLUTILS_ERROR, ZMAPENSEMBLUTILS_ERROR_RESULTS,
                        "No results returned from database '%s'", dbname) ;
          else
            g_set_error(error, ZMAP_ENSEMBLUTILS_ERROR, ZMAPENSEMBLUTILS_ERROR_RESULTS,
                        "No results returned from host '%s:%d'", host, port) ;
        }
    }
  else
    {
      g_set_error(error, ZMAP_ENSEMBLUTILS_ERROR, ZMAPENSEMBLUTILS_ERROR_CONNECT,
                  "Failed to open connection to '%s:%d' for user '%s'", host, port, user) ;
    }

  pthread_mutex_unlock(&mutex) ;

  return result ;
}


} // Unnamed namespace



list<string> zMapEnsemblGetDatabaseList(const char *host, 
                                        const int port,
                                        const char *user,
                                        const char *passwd,
                                        GError **error)
{
  list<string> result ;

  string query("SHOW DATABASES") ;

  result = ensemblGetList(host, port, user, passwd, NULL, query, error) ;

  return result ;
}


list<string> zMapEnsemblGetFeaturesetsList(const char *host, 
                                           const int port,
                                           const char *user,
                                           const char *passwd,
                                           const char *dbname,
                                           GError **error)
{
  list<string> result ;

  string query("SELECT logic_name FROM analysis") ;

  result = ensemblGetList(host, port, user, passwd, dbname, query, error) ;

  return result ;
}


list<string> zMapEnsemblGetBiotypesList(const char *host, 
                                        const int port,
                                        const char *user,
                                        const char *passwd,
                                        const char *dbname,
                                        GError **error)
{
  list<string> result ;

  string query("SELECT biotype FROM gene GROUP BY biotype") ;

  result = ensemblGetList(host, port, user, passwd, dbname, query, error) ;

  return result ;
}



#endif /* USE_ENSEMBL */
