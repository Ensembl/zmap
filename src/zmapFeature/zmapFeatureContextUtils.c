/*  File: zmapFeatureContextUtils.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 30 15:06 2007 (edgrif)
 * Created: Thu May 24 10:34:47 2007 (rds)
 * CVS info:   $Id: zmapFeatureContextUtils.c,v 1.2 2007-05-30 14:08:07 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapFeature.h>
#include <ZMap/zmapUtils.h>

static ZMapFeatureContextExecuteStatus addModeCB(GQuark key_id, 
						 gpointer data, 
						 gpointer user_data,
						 char **error_out) ;

static void addFeatureModeCB(gpointer key, gpointer data, gpointer user_data) ;




gboolean zMapFeatureAnyAddModesToStyles(ZMapFeatureAny feature_any)
{
  gboolean result = TRUE;

  zMapFeatureContextExecuteSubset(feature_any, 
                                  ZMAPFEATURE_STRUCT_FEATURESET,
                                  addModeCB,
                                  &result);

  return result;
}


/* go through all the feature sets in a context and find out what type of features are in the
 * set and set the style mode from that...a bit hacky really...think about this.... */
gboolean zMapFeatureContextAddModesToStyles(ZMapFeatureContext context)
{
  gboolean result = TRUE ;


  zMapFeatureContextExecute((ZMapFeatureAny)context,
			    ZMAPFEATURE_STRUCT_FEATURESET,
			    addModeCB,
			    &result) ;

  return result ;
}


static ZMapFeatureContextExecuteStatus addModeCB(GQuark key_id, 
						 gpointer data, 
						 gpointer user_data,
						 char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data ;
  ZMapFeatureStructType feature_type ;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;

  feature_type = feature_any->struct_type ;

  switch(feature_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
    case ZMAPFEATURE_STRUCT_ALIGN:
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
	break ;
      }
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
	ZMapFeatureSet feature_set ;

        feature_set = (ZMapFeatureSet)feature_any ;

	g_hash_table_foreach(feature_set->features, addFeatureModeCB, feature_set) ;

	break;
      }
    case ZMAPFEATURE_STRUCT_FEATURE:
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      {
	status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
	zMapAssertNotReached();
	break;
      }
    }

  return status ;
}



/* A GDataForeachFunc() to add a mode to the styles for all features in a set, note that
 * this is not efficient as we go through all features but we would need more information
 * stored in the feature set to avoid this. */

static void addFeatureModeCB(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  ZMapFeatureSet feature_set = (ZMapFeatureSet)user_data ;

  if (!zMapStyleHasMode(feature->style))
    {
      ZMapStyleMode mode ;

      switch (feature->type)
	{
	case ZMAPFEATURE_BASIC:
	  mode = ZMAPSTYLE_MODE_BASIC ;

	  if (g_ascii_strcasecmp(g_quark_to_string(zMapStyleGetID(feature->style)), "GF_splice") == 0)
	    {
	      mode = ZMAPSTYLE_MODE_GLYPH ;
	      zMapStyleSetGlyphMode(feature->style, ZMAPSTYLE_GLYPH_SPLICE) ;

	      zMapStyleSetColours(feature->style, ZMAPSTYLE_COLOURTARGET_FRAME0, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  "red", NULL, NULL) ;
	      zMapStyleSetColours(feature->style, ZMAPSTYLE_COLOURTARGET_FRAME1, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  "blue", NULL, NULL) ;
	      zMapStyleSetColours(feature->style, ZMAPSTYLE_COLOURTARGET_FRAME2, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  "green", NULL, NULL) ;
	    }
	  break ;
	case ZMAPFEATURE_ALIGNMENT:
	  mode = ZMAPSTYLE_MODE_ALIGNMENT ;
	  break ;
	case ZMAPFEATURE_TRANSCRIPT:
	  mode = ZMAPSTYLE_MODE_TRANSCRIPT ;
	  break ;
	case ZMAPFEATURE_RAW_SEQUENCE:
	  mode = ZMAPSTYLE_MODE_TEXT ;
	  break ;
	case ZMAPFEATURE_PEP_SEQUENCE:
	  mode = ZMAPSTYLE_MODE_TEXT ;
	  break ;
	  /* What about glyph and graph..... */

	default:
	  zMapAssertNotReached() ;
	  break ;
	}

      /* Tricky....we can have features within a single feature set that have _different_
       * styles, if this is the case we must be sure to set the mode in feature_set style
       * (where in fact its kind of useless as this is a style for the whole column) _and_
       * we must set it in the features own style. */
      zMapStyleSetMode(feature_set->style, mode) ;

      if (feature_set->style != feature->style)
	zMapStyleSetMode(feature->style, mode) ;
    }


  return ;
}
