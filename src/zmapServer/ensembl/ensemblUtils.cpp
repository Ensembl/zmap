/*  File: ensemblUtils.cpp
 *  Author: Gemma Guest (gb10@sanger.ac.uk)
 *  Copyright (c) 2015: Genome Research Ltd.
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


static list<string>* ensemblGetList(const char *host,
                                    const int port,
                                    const char *user,
                                    const char *passwd,
                                    const char *dbname,
                                    const string &query,
                                    GError **error) ;


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
} ZMapEnsemblUtilsError ;



list<string>* EnsemblGetDatabaseList(const char *host, 
                                     const int port,
                                     const char *user,
                                     const char *passwd,
                                     GError **error)
{
  list<string> *result = NULL ;

  string query("SHOW DATABASES") ;

  result = ensemblGetList(host, port, user, passwd, NULL, query, error) ;

  return result ;
}


list<string>* EnsemblGetFeaturesetsList(const char *host, 
                                        const int port,
                                        const char *user,
                                        const char *passwd,
                                        const char *dbname,
                                        GError **error)
{
  list<string> *result = NULL ;

  string query("SELECT logic_name FROM analysis") ;

  result = ensemblGetList(host, port, user, passwd, dbname, query, error) ;

  return result ;
}


list<string>* EnsemblGetBiotypesList(const char *host, 
                                     const int port,
                                     const char *user,
                                     const char *passwd,
                                     const char *dbname,
                                     GError **error)
{
  list<string> *result = NULL ;

  string query("SELECT biotype FROM gene GROUP BY biotype") ;

  result = ensemblGetList(host, port, user, passwd, dbname, query, error) ;

  return result ;
}



/* 
 *    Internal routines
 */

static list<string>* ensemblGetList(const char *host,
                                    const int port,
                                    const char *user,
                                    const char *passwd,
                                    const char *dbname,
                                    const string &query,
                                    GError **error)
{
  list<string> *result = NULL ;
  zMapReturnValIfFail(host, result) ;

  pthread_mutex_t mutex ;
  pthread_mutex_init(&mutex, NULL) ;
  pthread_mutex_lock(&mutex) ;

  MYSQL *mysql = mysql_init(NULL) ;
  mysql_real_connect(mysql, host, user, passwd, dbname, port, NULL, 0) ;

  if (mysql)
    {
      result = new list<string> ;

      mysql_real_query(mysql, query.c_str(), query.size()) ;

      MYSQL_RES *mysql_results = mysql_store_result(mysql) ;

      MYSQL_ROW mysql_row = mysql_fetch_row(mysql_results) ;

      while (mysql_row)
        {
          string string_val(mysql_row[0]) ;
          result->push_back(string_val) ;

          mysql_row = mysql_fetch_row(mysql_results) ;
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

#endif /* USE_ENSEMBL */
