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

#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <mysql.h>

#include <ZMap/zmapEnsemblUtils.hpp>
#include <ZMap/zmapUtilsDebug.hpp>

using namespace std ;


list<string>* EnsemblGetDatabaseList(const char *host, 
                                     const int port,
                                     const char *user,
                                     const char *passwd)
{
  list<string> *result = NULL ;
  zMapReturnValIfFail(host, result) ;

  const char *db_name = NULL; //"homo_sapiens_core_80_38" ;

  pthread_mutex_t mutex ;
  pthread_mutex_init(&mutex, NULL) ;
  pthread_mutex_lock(&mutex) ;

  MYSQL *mysql = mysql_init(NULL) ;
  mysql_real_connect(mysql, host, user, passwd, db_name, port, NULL, 0) ;

  if (mysql)
    {
      char *statement = g_strdup("SHOW DATABASES") ;
      const int statement_len = strlen(statement) ;

      mysql_real_query(mysql, statement, statement_len) ;

      MYSQL_RES *mysql_results = mysql_store_result(mysql) ;

      MYSQL_ROW mysql_row = mysql_fetch_row(mysql_results) ;

      while (mysql_row)
        {
          result->push_back(string(mysql_row[0])) ;

          mysql_row = mysql_fetch_row(mysql_results) ;
        }
    }
  else
    {
      zMapLogWarning("Failed to open connection to '%s:%d", host, port) ;
    }

  pthread_mutex_unlock(&mutex) ;

  return result ;
}
