/*  File: zmapUrl.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#ifndef ZMAPURL_H
#define ZMAPURL_H

#include <glib.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 


/* Default port definitions */
#define DEFAULT_HTTP_PORT 80
#define DEFAULT_FTP_PORT 21
#define DEFAULT_FILE_PORT 0      /* There is no port */
#define DEFAULT_PIPE_PORT 0      /* There is no port */
#define DEFAULT_ACEDB_PORT 23100 /* default sgifaceserver port */
#define DEFAULT_MYSQL_PORT 3306
#define DEFAULT_HTTPS_PORT 443
#define DEFAULT_ENSEMBL_PORT 5306
#define DEFAULT_TRACKHUB_PORT 0



/* Note: the ordering here is related to the order of elements in
   `supported_schemes' in url.c.  */

typedef enum {
  SCHEME_HTTP,
#ifdef HAVE_SSL
  SCHEME_HTTPS,
#endif
  SCHEME_FTP,
  SCHEME_ACEDB,
  SCHEME_FILE,
  SCHEME_PIPE,
  SCHEME_MYSQL,
  SCHEME_ENSEMBL,
  SCHEME_TRACKHUB,
  SCHEME_INVALID
} ZMapURLScheme;


/* Structure containing info on a URL.  */
typedef struct _ZMapURLStruct
{
  char *url;			/* Original URL */
  ZMapURLScheme scheme;	/* URL scheme */
  char *protocol;

  char *host;			/* Extracted hostname */
  int port;			/* Port number */

  /* URL components (URL-quoted). */
  char *path;
  char *params;
  char *query;
  char *fragment;

  /* Extracted path info (unquoted). */
  char *dir;
  char *file;

  /* Username and password (unquoted). */
  char *user;
  char *passwd;
} ZMapURLStruct, *ZMapURL;

/* Function declarations */

char *url_escape(const char *) ;

ZMapURL url_parse(const char *, int *) ;
const char *url_error(guint) ;
char *url_full_path(const ZMapURL) ;
void url_set_dir(ZMapURL, const char *) ;
void url_set_file(ZMapURL, const char *) ;
void url_free(ZMapURL) ;

ZMapURLScheme url_scheme(const char *) ;
int url_has_scheme(const char *) ;
int scheme_default_port(ZMapURLScheme) ;
void scheme_disable(ZMapURLScheme) ;

char *url_string(const ZMapURL, int) ;
char *url_file_name(const ZMapURL) ;

char *uri_merge(const char *, const char *) ;

char *rewrite_shorthand_url(const char *) ;
int schemes_are_similar_p(ZMapURLScheme a, ZMapURLScheme b) ;

char *zMapURLGetQueryValue(const char *full_query, const char *key) ;
gboolean zMapURLGetQueryBoolean(const char *full_query, const char *key) ;
int zMapURLGetQueryInt(const char *full_query, const char *key) ;

#endif /* ZMAPURL_H */
