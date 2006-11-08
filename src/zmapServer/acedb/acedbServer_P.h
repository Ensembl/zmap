/*  File: acedbServer_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Last edited: Aug  8 14:31 2006 (edgrif)
 * Created: Wed Mar 17 16:23:17 2004 (edgrif)
 * CVS info:   $Id: acedbServer_P.h,v 1.16 2006-11-08 09:24:27 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_SERVER_P_H
#define ACEDB_SERVER_P_H


/* For ZMap to be able to use the GFF returned by the acedb server we need the -rawmethods option
 * for the "seqfeatures" command and this is only available from acedb 4.9.28 onwards. */
#define ACEDB_SERVER_MIN_VERSION "4.9.34"


#define ACEDB_PROTOCOL_STR "Acedb"			    /* For error messages. */


/* Acedb handling of widths is quite complex: some methods do not have a width and there is
 * more than one default width (!). To complicate matters further, acedb screen units are
 * larger than zmap (foocanvas) units. We copy the default width most commonly used by
 * acedb and apply a magnification factor to make columns look similar in width. */
#define ACEDB_DEFAULT_WIDTH 2.0
#define ACEDB_MAG_FACTOR 8.0


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

  
  GHashTable *method_2_featureset ;			    /* Records which methods specified a
							       column_group (aka feature_set. */

  char *method_str ;


  ZMapFeatureContext current_context ;

  gboolean fetch_gene_finder_features ;			    /* Need to send additional requests to
							       server to get these. */


} AcedbServerStruct, *AcedbServer ;




#endif /* !ACEDB_SERVER_P_H */
