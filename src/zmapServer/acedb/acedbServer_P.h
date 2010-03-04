/*  File: acedbServer_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 * HISTORY:
 * Last edited: Aug 28 09:47 2009 (edgrif)
 * Created: Wed Mar 17 16:23:17 2004 (edgrif)
 * CVS info:   $Id: acedbServer_P.h,v 1.24 2010-03-04 14:40:57 mh17 Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_SERVER_P_H
#define ACEDB_SERVER_P_H


/* This code and the acedb server must stay in step as the two are co-developed so
 * we set a minimum acedb version that the code requires to work properly. */
#define ACEDB_SERVER_MIN_VERSION "4.9.48"


#define ACEDB_PROTOCOL_STR "Acedb"			    /* For error messages. */


/* For an acedb server we can use ?Style or ?Method class objects, default is now styles. */
#define ACEDB_USE_METHODS       "use_methods"


/* Some tag labels... */
#define COL_PARENT "Column_parent"
#define COL_CHILD  "Column_child"
#define STYLE      "Style"


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
  AceConnection connection ;
  char *host ;
  int port ;

  /* Needed so we can return err msgs for aceconn errors. */
  AceConnStatus last_err_status ;			    
  char *last_err_msg ;

  char *version_str ;					    /* For checking server is at right level. */

  gboolean fetch_gene_finder_features ;			    /* Need to send additional requests to
							       server to get these. */
  ZMapFeatureContext req_context ;

  GList *all_methods ;					    /* List of all methods to be used in
							       seqget/seqfeatures calls. */

  gboolean has_new_tags ;				    /* TRUE => use new column tags/zmap style objects. */

  GHashTable *method_2_data ;				    /* Records data for each method
							       (NULL if acedb_styles == FALSE). */

  GHashTable *method_2_feature_set ;			    /* Records the feature set for each method
							       (NULL if acedb_styles == FALSE). */

  ZMapFeatureContext current_context ;

} AcedbServerStruct, *AcedbServer ;




#endif /* !ACEDB_SERVER_P_H */
