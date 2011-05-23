/*  File: zmapFeatureData.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May  6 12:29 2011 (edgrif)
 * Created: Fri Jun 26 11:10:15 2009 (rds)
 * CVS info:   $Id: zmapFeatureData.c,v 1.12 2011-05-06 11:29:46 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <glib-object.h>
#include <gobject/gvaluecollector.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapSO.h>
#include <ZMap/zmapStyle.h>


#define ZMAP_FEATURE_DATA_NAME "ZMapFeatureDataName"


/* GTypes are just gulongs really */

#define FEATURE_DATA_TYPE_FEATURE_CONTEXT    (ZMAPFEATURE_STRUCT_CONTEXT)
#define FEATURE_DATA_TYPE_FEATURE_ALIGNMENT  (ZMAPFEATURE_STRUCT_ALIGN)
#define FEATURE_DATA_TYPE_FEATURE_BLOCK      (ZMAPFEATURE_STRUCT_BLOCK)
#define FEATURE_DATA_TYPE_FEATURE_FEATURESET (ZMAPFEATURE_STRUCT_FEATURESET)
#define FEATURE_DATA_TYPE_FEATURE            (ZMAPFEATURE_STRUCT_FEATURE)

#define FEATURE_DATA_FEATURE_SHAPE_SHIFT (3) /* Up to 7 context levels, increase if required. */

/* There should be a smaller number of these. */
#define FEATURE_DATA_TYPE_BASIC         (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_BASIC         << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_ALIGNMENT     (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_ALIGNMENT     << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_TRANSCRIPT    (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_TRANSCRIPT    << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_SEQUENCE  (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_SEQUENCE  << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_ASSEMBLY_PATH (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_ASSEMBLY_PATH << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_TEXT          (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_TEXT          << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_GRAPH         (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_GRAPH         << FEATURE_DATA_FEATURE_SHAPE_SHIFT))
#define FEATURE_DATA_TYPE_GLYPH         (FEATURE_DATA_TYPE_FEATURE + (ZMAPSTYLE_MODE_GLYPH         << FEATURE_DATA_FEATURE_SHAPE_SHIFT))


#define PROP_DATA_PARAM_LIST(_)                                    \
_(PROP_DATA_ZERO,         , "!invalid",     "zero is invalid", "") \
_(PROP_DATA_NAME,         , "name",         "name",            "") \
_(PROP_DATA_TERM,         , "term",         "term",            "") \
_(PROP_DATA_SOFA_TERM,    , "so-term",      "SO term",         "") \
_(PROP_DATA_TOTAL_LENGTH, , "total-length", "total length",    "") \
_(PROP_DATA_INDEX,        , "index",        "index",           "") \
_(PROP_DATA_START,        , "start",        "start",           "") \
_(PROP_DATA_END,          , "end",          "end",             "") \
_(PROP_DATA_LENGTH,       , "length",       "length",          "") \
_(PROP_DATA_STRAND,       , "strand",       "strand",          "") \
_(PROP_DATA_CDS_LENGTH,   , "cds-length",   "cds length",      "") \
_(PROP_DATA_5_UTR_LENGTH, , "utr-5-length", "5' utr length",   "") \
_(PROP_DATA_3_UTR_LENGTH, , "utr-3-length", "3' utr length",   "") \
_(PROP_DATA_LOCUS,        , "locus",        "locus",           "") \
_(PROP_DATA_QUERY_START,  , "query-start",  "query start",     "") \
_(PROP_DATA_QUERY_END,    , "query-end",    "query end",       "") \
_(PROP_DATA_QUERY_LENGTH, , "query-length", "query length",    "") \
_(PROP_DATA_QUERY_STRAND, , "query-strand", "query strand",    "") \
_(PROP_DATA_FINAL,        , "!final",       "also invalid",    "")

ZMAP_DEFINE_ENUM(ZMapFeatureDataProperty, PROP_DATA_PARAM_LIST);

typedef struct
{
  ZMapFeatureAny         feature_any;
  ZMapFeatureSubPartSpan sub_feature; /* Can be NULL */
} FeatureSubFeatureStruct, *FeatureSubFeature;

typedef struct
{
  ZMapFeatureDataProperty param_id;
  GType                   g_type;
  int                    *apply_to;
} DataTypeForFeaturesStruct, *DataTypeForFeatures;

typedef struct _zmapFeatureDataClassStruct
{
  GObjectClass __parent__;
  GParamSpecPool *pspec_pool;
} zmapFeatureDataClassStruct, zmapFeatureDataClass, *ZMapFeatureDataClass;

typedef struct _zmapFeatureDataStruct
{
  GObject __parent__;
} zmapFeatureDataStruct, zmapFeatureData, *ZMapFeatureData;


typedef gpointer (*CreateVFunc)(gpointer user_data, guint n_params, GParameter *parameters);
typedef gboolean (*GetFunc)(gpointer user_data, guint param_spec_id, GValue *value, GParamSpec *pspec);

static void zmap_feature_data_init (ZMapFeatureData data);
static void zmap_feature_data_class_init (ZMapFeatureDataClass data_class);


static char *gtype_to_message_string(GType feature_any_gtype);
static gboolean alignment_get_sub_feature_info(gpointer user_data, guint param_spec_id,
					       GValue *value, GParamSpec *pspec);
static gboolean transcript_get_sub_feature_info(gpointer user_data, guint param_spec_id,
						GValue *value, GParamSpec *pspec);
static gboolean basic_get_sub_feature_info(gpointer user_data, guint param_spec_id,
					   GValue *value, GParamSpec *pspec);

static gboolean invoke_get_func_valist(gpointer        user_data,
				       GParamSpecPool *pspec_pool,
				       GType           pool_member_type,
				       GetFunc         get_func,
				       const gchar    *first_property_name,
				       va_list	        var_args);

static ZMAP_ENUM_AS_NAME_STRING_DEC(zmapFeatureDataPropertyName, ZMapFeatureDataProperty);
static ZMAP_ENUM_TO_SHORT_TEXT_DEC(zmapFeatureDataPropertyNick, ZMapFeatureDataProperty);

static GParamSpecPool *pspec_pool_G = NULL;

static gboolean fail_on_bad_requests_G = FALSE;

static GType zmapFeatureAnyGType(ZMapFeatureAny feature_any)
{
  GType gtype = 0;

  gtype = feature_any->struct_type;

  if(gtype == ZMAPFEATURE_STRUCT_FEATURE)
    gtype += (((ZMapFeature)feature_any)->type) << FEATURE_DATA_FEATURE_SHAPE_SHIFT ;

  return gtype;
}

GType zMapFeatureDataGetType(void)
{
  static GType type = 0;

  if(type == 0)
    {
      static const GTypeInfo info =
	{
	  sizeof (zmapFeatureDataClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_feature_data_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof(zmapFeatureData),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc)zmap_feature_data_init,
	  NULL
	};

      type = g_type_register_static (G_TYPE_OBJECT,
				     ZMAP_FEATURE_DATA_NAME,
				     &info, 0);
    }

  return type;
}


static void apply(GParamSpecPool *pool, const char *name,
		  const char *nick, const char *blurb,
		  DataTypeForFeatures children)
{
  GParamSpec *pspec;
  GType g_type;
  int i, max;
  int *apply_to;

  if((apply_to = children->apply_to) && (max = apply_to[0]))
    {
      apply_to++;
      g_type = children->g_type;

      for(i = 0; i < max; i++, apply_to++)
	{
	  if(g_type == G_TYPE_INT)
	    {
	      pspec = g_param_spec_int(name, nick, blurb, 0, G_MAXINT, 0, G_PARAM_READABLE);
	    }
	  else if(g_type == G_TYPE_STRING)
	    {
	      pspec = g_param_spec_string(name, nick, blurb, NULL, G_PARAM_READABLE);
	    }
	  else
	    {
	      pspec = NULL;
	    }

	  if(pspec)
	    {
	      pspec->param_id = children->param_id;
	      g_param_spec_pool_insert(pool, pspec, apply_to[0]);
	    }
	}
    }

  return ;
}

static void zmap_feature_data_init (ZMapFeatureData data)
{
  g_warning("not for instantiating");
  return ;
}


static void zmap_feature_data_class_init (ZMapFeatureDataClass data_class)
{
  GParamSpecPool *pspec_pool;

  if((pspec_pool = pspec_pool_G) == NULL)
    {
      int apply_to_all[] = {
	14,				/* how many?  */
	FEATURE_DATA_TYPE_FEATURE_CONTEXT,
	FEATURE_DATA_TYPE_FEATURE_ALIGNMENT,
	FEATURE_DATA_TYPE_FEATURE_BLOCK,
	FEATURE_DATA_TYPE_FEATURE_FEATURESET,
	FEATURE_DATA_TYPE_FEATURE,
	FEATURE_DATA_TYPE_BASIC,
	FEATURE_DATA_TYPE_ALIGNMENT,
	FEATURE_DATA_TYPE_TRANSCRIPT,
	FEATURE_DATA_TYPE_SEQUENCE,
	FEATURE_DATA_TYPE_ASSEMBLY_PATH,
	FEATURE_DATA_TYPE_TEXT,
	FEATURE_DATA_TYPE_GRAPH,
	FEATURE_DATA_TYPE_GLYPH,
      };
      int apply_to_alignments[]  = {1, FEATURE_DATA_TYPE_ALIGNMENT };
      int apply_to_transcripts[] = {1, FEATURE_DATA_TYPE_TRANSCRIPT };
      DataTypeForFeaturesStruct fname       = { PROP_DATA_NAME,      G_TYPE_STRING, apply_to_all };
      DataTypeForFeaturesStruct term        = { PROP_DATA_TERM,      G_TYPE_STRING, apply_to_all };
      DataTypeForFeaturesStruct so_term     = { PROP_DATA_SOFA_TERM, G_TYPE_STRING, apply_to_all };

      DataTypeForFeaturesStruct total_length = { PROP_DATA_TOTAL_LENGTH, G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct index       = { PROP_DATA_INDEX,  G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct start       = { PROP_DATA_START,  G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct end         = { PROP_DATA_END,    G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct length      = { PROP_DATA_LENGTH, G_TYPE_INT, apply_to_all };
      DataTypeForFeaturesStruct strand      = { PROP_DATA_STRAND, G_TYPE_INT, apply_to_all };

      DataTypeForFeaturesStruct cds_length  = { PROP_DATA_CDS_LENGTH,   G_TYPE_INT, apply_to_transcripts };
      DataTypeForFeaturesStruct utr5_length = { PROP_DATA_5_UTR_LENGTH, G_TYPE_INT, apply_to_transcripts };
      DataTypeForFeaturesStruct utr3_length = { PROP_DATA_3_UTR_LENGTH, G_TYPE_INT, apply_to_transcripts };
      DataTypeForFeaturesStruct locus       = { PROP_DATA_LOCUS,      G_TYPE_STRING, apply_to_transcripts };

      DataTypeForFeaturesStruct query_start  = { PROP_DATA_QUERY_START,  G_TYPE_INT, apply_to_alignments };
      DataTypeForFeaturesStruct query_end    = { PROP_DATA_QUERY_END,    G_TYPE_INT, apply_to_alignments };
      DataTypeForFeaturesStruct query_length = { PROP_DATA_QUERY_LENGTH, G_TYPE_INT, apply_to_alignments };
      DataTypeForFeaturesStruct query_strand = { PROP_DATA_QUERY_STRAND, G_TYPE_INT, apply_to_alignments };
      /* now make the table */
      DataTypeForFeatures full_table[PROP_DATA_FINAL+1] =
	{
	  NULL,
	  &fname, &term, &so_term,
	  &total_length, &index, &start, &end, &length, &strand,
	  &cds_length, &utr5_length, &utr3_length, &locus,
	  &query_start, &query_end, &query_length, &query_strand,
	  NULL
      } ;
      DataTypeForFeatures *table; /* and a pointer */
      const char *name, *nick, *blurb = NULL;
      int i;

      pspec_pool = g_param_spec_pool_new(FALSE);

      table = &full_table[0];

      for(i = PROP_DATA_ZERO; i < PROP_DATA_FINAL; i++, table++)
	{
	  name  = zmapFeatureDataPropertyName(i);
	  nick  = zmapFeatureDataPropertyNick(i);
#ifdef RDS_DONT_INCLUDE
	  blurb = zmapFeatureDataPropertyBlurb(i);
#endif
	  if (name && name[0] != '!' && table[0]->param_id == i) /* filter invalid names */
	    {
	      apply(pspec_pool, name, nick, blurb, table[0]);
	    }
	}

      pspec_pool_G = data_class->pspec_pool = pspec_pool;
    }

  return ;
}



static char *gtype_to_message_string(GType feature_any_gtype)
{
  static char *string_array[1 << 16] = {NULL};
  char *message = NULL;

  if(string_array[FEATURE_DATA_TYPE_FEATURE_CONTEXT] == NULL)
    {
      int i;

      string_array[FEATURE_DATA_TYPE_FEATURE_CONTEXT]    = "ZMapFeatureContext";
      string_array[FEATURE_DATA_TYPE_FEATURE_ALIGNMENT]  = "ZMapFeatureAlignment";
      string_array[FEATURE_DATA_TYPE_FEATURE_BLOCK]      = "ZMapFeatureBlock";
      string_array[FEATURE_DATA_TYPE_FEATURE_FEATURESET] = "ZMapFeatureSet";
      string_array[FEATURE_DATA_TYPE_FEATURE]            = "ZMapFeature";

      for(i = ZMAPSTYLE_MODE_BASIC; i <= ZMAPSTYLE_MODE_GLYPH; i++)
	{
	  string_array[FEATURE_DATA_TYPE_FEATURE + (i << FEATURE_DATA_FEATURE_SHAPE_SHIFT)] =
	    g_strdup_printf("ZMapFeature [%s]", zMapStyleMode2ExactStr(i));
	}
    }

  message = string_array[feature_any_gtype];

  return message;
}




static gboolean alignment_get_sub_feature_info(gpointer user_data, guint param_spec_id,
					       GValue *value, GParamSpec *pspec)
{
  gboolean result = FALSE ;
  FeatureSubFeature feature_data = (FeatureSubFeature)user_data ;
  ZMapFeature feature ;
  ZMapFeatureSubPartSpan sub_feature ;

  feature = (ZMapFeature)feature_data->feature_any;
  sub_feature = feature_data->sub_feature ;

  switch(param_spec_id)
    {
    case PROP_DATA_INDEX:
    case PROP_DATA_START:
    case PROP_DATA_END:
    case PROP_DATA_LENGTH:
    case PROP_DATA_TERM:
    case PROP_DATA_SOFA_TERM:

      result = basic_get_sub_feature_info(user_data, param_spec_id, value, pspec);
      break;

    case PROP_DATA_TOTAL_LENGTH:
      if (feature->feature.homol.length)
	{
	  g_value_set_int(value, feature->feature.homol.length) ;
	  result = TRUE ;
	}
      break;

    case PROP_DATA_QUERY_START:
    case PROP_DATA_QUERY_END:
    case PROP_DATA_QUERY_LENGTH:
      {
	GArray *gaps_array ;
	ZMapFeatureSubPartSpan sub_feature;

	if ((sub_feature = feature_data->sub_feature) && (gaps_array  = feature->feature.homol.align))
	  {
	    if (sub_feature->subpart == ZMAPFEATURE_SUBPART_MATCH)
	      {
		ZMapAlignBlock align_block;
		int i;
		/* easy case, just look through the gaps array */
		for(i = 0; i < gaps_array->len; i++)
		  {
		    align_block = &(g_array_index(gaps_array, ZMapAlignBlockStruct, i));
		    if(align_block->t1 == sub_feature->start &&
		       align_block->t2 == sub_feature->end)
		      {
			if(param_spec_id == PROP_DATA_QUERY_START)
			  g_value_set_int(value, align_block->q1);
			else if(param_spec_id == PROP_DATA_QUERY_END)
			  g_value_set_int(value, align_block->q2);
			else
			  g_value_set_int(value, align_block->q2 - align_block->q1 + 1);
		      }
		  }
		result = TRUE;
	      }
	    else if(sub_feature->subpart == ZMAPFEATURE_SUBPART_GAP)
	      {
		/* need to run through and find matching matches and calculate the gap... */
	      }
	  }
	else
	  {
	    result = TRUE;
	    if(param_spec_id == PROP_DATA_QUERY_START)
	      g_value_set_int(value, feature->feature.homol.y1);
	    else if(param_spec_id == PROP_DATA_QUERY_END)
	      g_value_set_int(value, feature->feature.homol.y2);
	    else
	      g_value_set_int(value, feature->feature.homol.y2 - feature->feature.homol.y1 + 1);
	  }
      }
      break;
    case PROP_DATA_QUERY_STRAND:
      {
	g_value_set_int(value, feature->feature.homol.strand);

	result = TRUE;
      }
      break;
    default:
      result = FALSE;
      break;
    }

  return result;
}

static gboolean transcript_get_sub_feature_info(gpointer user_data, guint param_spec_id,
						GValue *value, GParamSpec *pspec)
{
  gboolean result = FALSE;

  switch(param_spec_id)
    {
    case PROP_DATA_INDEX:
    case PROP_DATA_START:
    case PROP_DATA_END:
    case PROP_DATA_LENGTH:
    case PROP_DATA_TERM:
    case PROP_DATA_SOFA_TERM:
      result = basic_get_sub_feature_info(user_data, param_spec_id, value, pspec);
      break;

    case PROP_DATA_LOCUS:
      {
	FeatureSubFeature feature_data = (FeatureSubFeature)user_data;
	ZMapFeature feature;

	feature = (ZMapFeature)feature_data->feature_any;

	if (feature->feature.transcript.locus_id)
	  g_value_set_static_string(value, g_quark_to_string(feature->feature.transcript.locus_id));


	result = TRUE;

	break;
      }
    default:
      result = FALSE;
      break;
    }

  return result;
}

static gboolean basic_get_sub_feature_info(gpointer user_data, guint param_spec_id,
					   GValue *value, GParamSpec *pspec)
{
  gboolean result;

  switch(param_spec_id)
    {
    case PROP_DATA_INDEX:
    case PROP_DATA_START:
    case PROP_DATA_END:
    case PROP_DATA_LENGTH:
      {
	FeatureSubFeature feature_data = (FeatureSubFeature)user_data;
	ZMapFeature feature;

	feature = (ZMapFeature)feature_data->feature_any;

	if (feature_data->sub_feature == NULL)
	  {
	    if(param_spec_id == PROP_DATA_START)
	      g_value_set_int(value, feature->x1);
	    else if(param_spec_id == PROP_DATA_END)
	      g_value_set_int(value, feature->x2);
	    else
	      g_value_set_int(value, feature->x2 - feature->x1 + 1);
	  }
	else
	  {
	    if (param_spec_id == PROP_DATA_INDEX)
	      g_value_set_int(value, feature_data->sub_feature->index) ;
	    else if (param_spec_id == PROP_DATA_START)
	      g_value_set_int(value, feature_data->sub_feature->start);
	    else if (param_spec_id == PROP_DATA_END)
	      g_value_set_int(value, feature_data->sub_feature->end);
	    else
	      g_value_set_int(value, feature_data->sub_feature->end - feature_data->sub_feature->start + 1);
	  }

	result = TRUE;
      }
      break;
    case PROP_DATA_TERM:
    case PROP_DATA_SOFA_TERM:
      {
	FeatureSubFeature feature_data = (FeatureSubFeature)user_data ;
	ZMapFeature feature;

	feature = (ZMapFeature)feature_data->feature_any ;

	if (feature_data->sub_feature == NULL)
	  {
	    GQuark term_id;

	    if (feature->SO_accession)
	      {
		term_id = zMapSOAcc2TermID(feature->SO_accession) ;
	      }
	    else
	      {
		switch(feature->type)
		  {
		  case ZMAPSTYLE_MODE_BASIC:
		    term_id = g_quark_from_string("Basic") ;
		    break ;
		  case ZMAPSTYLE_MODE_TRANSCRIPT:
		    term_id = g_quark_from_string("Transcript") ;
		    break ;
		  case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
		    term_id = g_quark_from_string("Assembly Path") ;
		    break ;
		  case ZMAPSTYLE_MODE_ALIGNMENT:
		    term_id = g_quark_from_string("Alignment") ;
		    break ;
		  case ZMAPSTYLE_MODE_GRAPH:
		    term_id = g_quark_from_string("Graph") ;
		    break ;
		  case ZMAPSTYLE_MODE_GLYPH:
		    term_id = g_quark_from_string("Glyph") ;
		    break ;
		  case ZMAPSTYLE_MODE_TEXT:
		    term_id = g_quark_from_string("Text") ;
		    break ;
		  case ZMAPSTYLE_MODE_SEQUENCE:
		    term_id = g_quark_from_string("Sequence") ;
		    break ;
		  default:
		    term_id = g_quark_from_string("<UNKNOWN>") ;
		    break ;
		  }
	      }

	    g_value_set_static_string(value, g_quark_to_string(term_id)) ;
	  }
	else
	  {
	    GQuark term_id;

	    switch(feature_data->sub_feature->subpart)
	      {
	      case ZMAPFEATURE_SUBPART_GAP:
		term_id = g_quark_from_string("Gap");
		break;
	      case ZMAPFEATURE_SUBPART_MATCH:
		term_id = g_quark_from_string("Match");
		break;
	      case ZMAPFEATURE_SUBPART_EXON_CDS:
		term_id = g_quark_from_string("Coding Exon");
		break;
	      case ZMAPFEATURE_SUBPART_EXON:
		if (feature->feature.transcript.flags.cds)
		  term_id = g_quark_from_string("Non-coding Exon") ;
		else
		  term_id = g_quark_from_string("Exon") ;
		break;
	      case ZMAPFEATURE_SUBPART_INTRON:
	      case ZMAPFEATURE_SUBPART_INTRON_CDS:
		term_id = g_quark_from_string("Intron");
		break;
	      default:
		term_id = g_quark_from_string("<UNKNOWN>");
		break;
	      }

	    g_value_set_static_string(value, g_quark_to_string(term_id));
	  }

	result = TRUE;
      }
      break;
    default:
      result = FALSE;
      break;
    }

  return result;
}



/*!
 * \brief GObject style access to feature data.
 *
 * \details This makes ZMapFeatures kind of inside out
 * gobjects. ZMapFeatures are _NOT_ and should _NEVER_ be GObjects,
 * but it would be nice to have some of the functionality the GObject
 * code affords. Probably shouldn't be used in a loop! The code
 * inspects the feature and any supplied sub feature for the property
 * names supplied and fills in the pointers supplied just as
 * g_object_get would do.
 *
 * \param feature_any The ZMapFeatureAny feature to inspect.
 * \param sub_feature The ZMapFeatureSubPartSpan to inspect (can be NULL)
 * \param first_property_name name of the property.
 *
 * \return TRUE on success, FALSE on failure.
 */

gboolean zMapFeatureGetInfo(ZMapFeatureAny         feature_any,
			    ZMapFeatureSubPartSpan sub_feature,
			    const gchar           *first_property_name,
			    ...)
{
  ZMapFeature feature;
  GType feature_type = 0;
  GetFunc get_func_pointer = NULL;
  gboolean result = FALSE;
  va_list var_args;

  feature_type = zmapFeatureAnyGType(feature_any);

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
	feature = (ZMapFeature)feature_any;
	switch(feature->type)
	  {
	  case ZMAPSTYLE_MODE_ALIGNMENT:
	    get_func_pointer = alignment_get_sub_feature_info;
	    break;
	  case ZMAPSTYLE_MODE_TRANSCRIPT:
	    get_func_pointer = transcript_get_sub_feature_info;
	    break;
	  case ZMAPSTYLE_MODE_BASIC:
	  default:
	    get_func_pointer = basic_get_sub_feature_info;
	    break;
	  }
      }
      break;
    default:
      break;
    }

  if(get_func_pointer && pspec_pool_G && feature_type != 0)
    {
      FeatureSubFeatureStruct get_info_data = {NULL};
      result = TRUE;

      get_info_data.feature_any = feature_any;
      get_info_data.sub_feature = sub_feature;

      va_start(var_args, first_property_name);

      if(result || (!fail_on_bad_requests_G))
	result = invoke_get_func_valist(&get_info_data, pspec_pool_G, feature_type,
					get_func_pointer, first_property_name, var_args);

      va_end(var_args);
    }

  return result;
}



/* like g_object_get_valist */
static gboolean invoke_get_func_valist(gpointer        user_data,
				       GParamSpecPool *pspec_pool,
				       GType           pool_member_type,
				       GetFunc         get_func,
				       const gchar    *first_property_name,
				       va_list	        var_args)
{
  const gchar *name;
  gboolean result = FALSE;

  name = first_property_name;

  while (name)
    {
      GValue value = { 0, };
      GParamSpec *pspec;
      gchar *error;

      result = FALSE;

      pspec = g_param_spec_pool_lookup (pspec_pool,
					name,
					pool_member_type,
					TRUE);

      if (fail_on_bad_requests_G && !pspec)
	{
	  g_warning ("%s: type `%s' has no property named `%s'",
		     G_STRFUNC,
		     gtype_to_message_string(pool_member_type),
		     name);
	  break;
	}
      else if(pspec)
	{
	  if (fail_on_bad_requests_G && !(pspec->flags & G_PARAM_READABLE))
	    {
	      g_warning ("%s: property `%s' of object class `%s' is not readable",
			 G_STRFUNC,
			 pspec->name,
			 gtype_to_message_string(pool_member_type));
	      break;
	    }

	  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));

	  if(get_func)
	    result = (get_func)(user_data, pspec->param_id, &value, pspec);

	  G_VALUE_LCOPY (&value, var_args, 0, &error);

	  if (error)
	    {
	      g_warning ("%s: %s", G_STRFUNC, error);
	      g_free (error);
	      g_value_unset (&value);
	      break;
	    }

	  g_value_unset (&value);
	}

      name = va_arg (var_args, gchar*);
    }

  return result;
}


static ZMAP_ENUM_AS_NAME_STRING_FUNC(zmapFeatureDataPropertyName, ZMapFeatureDataProperty, PROP_DATA_PARAM_LIST);
static ZMAP_ENUM_TO_SHORT_TEXT_FUNC(zmapFeatureDataPropertyNick, ZMapFeatureDataProperty, PROP_DATA_PARAM_LIST);




#ifdef NEVER_GONNA_BE_FOR_FEATURES

/* like g_object_new_valist */
static gpointer invoke_create_func_valist(gpointer        user_data,
					  GParamSpecPool *pspec_pool,
					  GType           pool_member_type,
					  CreateVFunc     create_func,
					  const gchar    *first_property_name,
					  va_list         var_args)
{
  gpointer return_pointer = NULL;
  GParameter *params;
  const gchar *name;
  guint n_params = 0, n_alloced_params = 16;

  if(!create_func)
    return return_pointer;

  if (!first_property_name)
    {
      return_pointer = (create_func)(user_data, 0, NULL);
    }
  else
    {
      params = g_new (GParameter, n_alloced_params);
      name   = first_property_name;
      while (name)
	{
	  gchar *error = NULL;
	  GParamSpec *pspec = g_param_spec_pool_lookup (pspec_pool,
							name,
							pool_member_type,
							TRUE);
	  if (!pspec)
	    {
	      g_warning ("%s: type `%s' has no property named `%s'",
			 G_STRFUNC,
			 g_type_name (pool_member_type),
			 name);
	      break;
	    }

	  if (n_params >= n_alloced_params)
	    {
	      n_alloced_params += 16;
	      params = g_renew (GParameter, params, n_alloced_params);
	    }

	  params[n_params].name = name;
	  params[n_params].value.g_type = 0;
	  g_value_init (&params[n_params].value, G_PARAM_SPEC_VALUE_TYPE (pspec));
	  G_VALUE_COLLECT (&params[n_params].value, var_args, 0, &error);
	  if (error)
	    {
	      g_warning ("%s: %s", G_STRFUNC, error);
	      g_free (error);
	      g_value_unset (&params[n_params].value);
	      break;
	    }
	  n_params++;
	  name = va_arg (var_args, gchar*);
	}

      return_pointer = (create_func)(user_data, n_params, params);

      while (n_params--)
	{
	  g_value_unset (&params[n_params].value);
	}
      g_free (params);

    }

  return return_pointer;

}

#endif /* NEVER_GONNA_BE_FOR_FEATURES */



