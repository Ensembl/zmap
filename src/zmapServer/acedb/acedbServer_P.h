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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 * HISTORY:
 * Last edited: Sep 29 09:56 2008 (edgrif)
 * Created: Wed Mar 17 16:23:17 2004 (edgrif)
 * CVS info:   $Id: acedbServer_P.h,v 1.20 2008-09-29 16:27:44 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_SERVER_P_H
#define ACEDB_SERVER_P_H


/* This code and the acedb server must stay in step as the two are developed so
 * we set a minimum acedb version that the code requires to work properly. */
#define ACEDB_SERVER_MIN_VERSION "4.9.45"


#define ACEDB_PROTOCOL_STR "Acedb"			    /* For error messages. */


/* For an acedb server we can use ?Style or ?Method class objects, default is now styles. */
#define ACEDB_USE_METHODS       "use_methods"


/* Some tag labels... */
#define COL_CHILD "Column_child"


/* Acedb handling of widths is quite complex: some methods do not have a width and there is
 * more than one default width (!). To complicate matters further, acedb screen units are
 * larger than zmap (foocanvas) units. We copy the default width most commonly used by
 * acedb and apply a magnification factor to make columns look similar in width. */
#define ACEDB_DEFAULT_WIDTH 2.0
#define ACEDB_MAG_FACTOR 8.0


/* Holds all the state we need to manage the acedb connection. */
typedef struct _AcedbServerStruct
{
  /* Connection details. */
  char *host ;
  int port ;

  AceConnection connection ;

  AceConnStatus last_err_status ;			    /* Needed so we can return err msgs
							       for aceconn errors. */
  char *last_err_msg ;

  char *version_str ;					    /* For checking server is at right level. */

  gboolean acedb_styles ;				    /* Use old method or new zmap style objects. */

  ZMapFeatureContext req_context ;


  GList *all_methods ;					    /* List of all methods to be used in
							       seqget/seqfeatures calls. */
  

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Is this actually needed now ???? */
  GHashTable *method_2_featureset ;			    /* Records which methods specified a
							       column_group (aka feature_set. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  ZMapFeatureContext current_context ;

  gboolean fetch_gene_finder_features ;			    /* Need to send additional requests to
							       server to get these. */


} AcedbServerStruct, *AcedbServer ;




#endif /* !ACEDB_SERVER_P_H */
