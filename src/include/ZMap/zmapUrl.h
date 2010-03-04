/*  File: zmapUrl.h
 *  Author: Roy Story (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * originally written by:
 *
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Created: Thu Nov 26 10:30:21 2009 (mh17)
 * CVS info:   $Id: zmapUrl.h,v 1.6 2010-03-04 15:35:32 mh17 Exp $
 *-------------------------------------------------------------------
 */

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
