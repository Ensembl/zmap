/*  File: acedbServer_P.h
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
 * Last edited: Jun 24 13:04 2005 (edgrif)
 * Created: Wed Mar 17 16:23:17 2004 (edgrif)
 * CVS info:   $Id: acedbServer_P.h,v 1.6 2005-06-24 13:21:47 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_SERVER_P_H
#define ACEDB_SERVER_P_H


/* For ZMap to be able to use the GFF returned by the acedb server we need the -rawmethods option
 * for the "seqfeatures" command and this is only available from acedb 4.9.28 onwards. */
#define ACEDB_SERVER_MIN_VERSION "4.9.28"


#define ACEDB_PROTOCOL_STR "Acedb"			    /* For error messages. */


/* Holds all the state we need to manage the acedb connection. */
typedef struct _AcedbServerStruct
{
  char *host ;

  AceConnection connection ;

  AceConnStatus last_err_status ;			    /* Needed so we can return err msgs
							       for aceconn errors. */
  char *last_err_msg ;

  char *version_str ;					    /* For checking server is at right level. */

  ZMapFeatureContext req_context ;

  GList *types ;
  char *method_str ;
  char *find_method_str ;

  ZMapFeatureContext current_context ;

} AcedbServerStruct, *AcedbServer ;




#endif /* !ACEDB_SERVER_P_H */
