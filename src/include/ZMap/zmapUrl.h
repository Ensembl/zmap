/*  Last edited: 2009-11-26 12:52:29 (mgh) */
/*  Last edited: Aug 31 11:05 2005 (rds) */
/* Declarations for url.c.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

In addition, as a special exception, the Free Software Foundation
gives permission to link the code of its release of Wget with the
OpenSSL projects "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.  */

#ifndef ZMAPURL_H
#define ZMAPURL_H


/* Default port definitions */
#define DEFAULT_HTTP_PORT 80
#define DEFAULT_FTP_PORT 21      
#define DEFAULT_FILE_PORT 0      /* There is no port */
#define DEFAULT_PIPE_PORT 0      /* There is no port */
#define DEFAULT_ACEDB_PORT 23100 /* default sgifaceserver port */
#define DEFAULT_MYSQL_PORT 3306  
#define DEFAULT_HTTPS_PORT 443

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
const char *url_error(int) ;
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

#endif /* ZMAPURL_H */
