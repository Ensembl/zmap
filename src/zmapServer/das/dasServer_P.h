/*  File: dasServer_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * HISTORY:
 * Last edited: Jul  2 10:40 2004 (edgrif)
 * Created: Thu Mar 18 12:02:52 2004 (edgrif)
 * CVS info:   $Id: dasServer_P.h,v 1.2 2004-07-19 10:13:36 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DAS_SERVER_P_H
#define DAS_SERVER_P_H

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <saxparse.h>

/* Holds all the state we need to manage the das connection. */
typedef struct _DasServerStruct
{
  char *host ;
  int port ;
  char *userid ;
  char *passwd ;

  CURL *curl_handle ;

  int chunks ;						    /* for debugging at the moment... */

  SaxParser parser ;

  /* error stuff... */
  CURLcode curl_error ;					    /* curl specific error stuff */
  char *curl_errmsg ;
  char *last_errmsg ;					    /* The general das msg stuf, could be
							       curl could be my code. */

  /* this will not stay like this, it should more properly be a union of types of data that might
   * be returned.... */
  void *data ;

} DasServerStruct, *DasServer ;


#endif /* !DAS_SERVER_P_H */
