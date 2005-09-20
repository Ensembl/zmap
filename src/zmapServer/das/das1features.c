/*  File: das1features.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Sep 16 11:11 2005 (rds)
 * Created: Wed Aug 31 18:37:38 2005 (rds)
 * CVS info:   $Id: das1features.c,v 1.3 2005-09-20 17:16:11 rds Exp $
 *-------------------------------------------------------------------
 */

#include <das1schema.h>
#include <ZMap/zmapFeature.h>


typedef struct _dasOneGroupStruct
{
  GQuark id;
  GQuark type;
  GQuark label;
} dasOneGroupStruct;


typedef struct _dasOneMethodStruct
{
  GQuark id;
  GQuark name;
} dasOneMethodStruct;

typedef struct _dasOneTargetStruct
{
  GQuark id;
  int start;
  int stop;
} dasOneTargetStruct;

typedef struct _dasOneTypeStruct
{
  GQuark id;
  GQuark name;
  GQuark category;
  dasOneRefProperties reference;
} dasOneTypeStruct;

typedef struct _dasOneFeatureStruct
{
  GQuark id;
  GQuark label;
  int start;
  int stop;
  double score;

  ZMapStrand orientation;
  ZMapPhase phase;

  dasOneType type;
  dasOneMethod method;
  dasOneTarget target;

  GList *groups;
  
} dasOneFeatureStruct;


dasOneFeature dasOneFeature_create1(GQuark id, GQuark label)
{
  dasOneFeature feature = NULL;
  feature = g_new0(dasOneFeatureStruct, 1);
  feature->id    = id;
  feature->label = label;

  feature->orientation = ZMAPSTRAND_NONE;
  feature->phase       = ZMAPPHASE_NONE;

  feature->type = dasOneType_create1(0,0,0);

  return feature;
}

GQuark dasOneFeature_id1(dasOneFeature feature)
{
  return feature->id;
}

gboolean dasOneFeature_getLocation(dasOneFeature feature,
                                  int *start_out, int *stop_out)
{
  gboolean res = TRUE;
  if(!feature->start || !feature->stop)
    res = FALSE;
  if(res)
    {
      if(start_out)
        *start_out = feature->start;
      if(stop_out)
        *stop_out = feature->stop;
    }
  return res;
}

void dasOneFeature_setProperties(dasOneFeature feature, 
                                 int start, int stop,
                                 char *score, char *orientation,
                                 char *phase)
{
  feature->start = start;
  feature->stop  = stop;

  zMapFeatureFormatScore(score, &(feature->score));
  zMapFeatureFormatStrand(orientation, &(feature->orientation));
  zMapFeatureFormatPhase(phase, &(feature->phase));

  return ;
}

void dasOneFeature_setTarget(dasOneFeature feature, 
                             dasOneTarget target)
{
  if(!target)
    return ;
  feature->target = target;
  return ;
}

void dasOneFeature_setType(dasOneFeature feature, 
                           dasOneType type)
{
  if(!type)
    return ;

  if(feature->type)
    dasOneType_free(feature->type);

  feature->type = type;

  return ;
}

gboolean dasOneFeature_getTargetBounds(dasOneFeature feature, 
                                       int *start_out, int *stop_out)
{
  gboolean res = TRUE;

  if(feature->target == NULL)
    res = FALSE;

  if(!(feature->type->reference & DASONE_REF_ISREFERENCE))
    res = FALSE;

  if(res)
    {
      if(start_out)
        *start_out = feature->target->start;
      if(stop_out)
        *stop_out = feature->target->stop;
    }

  return res;
}
void dasOneFeature_free(dasOneFeature feat)
{
  feat->id = 0;
  feat->label = 0;
  feat->start = 0;
  feat->stop = 0;
  feat->score = 0.0;
  
  feat->orientation = 0;
  feat->phase = 0;
  
  dasOneType_free(feat->type);
  feat->type = NULL;

  dasOneMethod_free(feat->method);
  feat->method = NULL;
  
  dasOneTarget_free(feat->target);
  feat->target = NULL;

  feat->groups = NULL;          /* This will need to be freed later */

  g_free(feat);

  return ;
}

dasOneType dasOneType_create1(GQuark id, GQuark name, GQuark category)
{
  dasOneType type = NULL;
  type = g_new0(dasOneTypeStruct,1);
  type->id       = id;
  type->name     = name;
  type->category = category;
  return type;
}

dasOneRefProperties dasOneType_refProperties(dasOneType type, char *ref, 
                                             char *sub, char *super, 
                                             gboolean set)
{
  dasOneRefProperties props = DASONE_REF_UNSET;
  if(!set)
    props = type->reference;
  else
    {
      GQuark yes      = g_quark_from_string("yes");
      type->reference = props;
      if(ref && *ref && (g_quark_from_string(g_ascii_strdown(ref, -1)) == yes))
        type->reference |= DASONE_REF_ISREFERENCE;
      if(sub && *sub && (g_quark_from_string(g_ascii_strdown(sub, -1)) == yes))
        type->reference |= DASONE_REF_HASSUBPARTS;
      if(super && *super && (g_quark_from_string(g_ascii_strdown(super, -1)) == yes))
        type->reference |= DASONE_REF_HASSUPPARTS;
      props = type->reference;
    }
  return props;
}

void dasOneType_free(dasOneType type)
{
  if(type)
    g_free(type);
  return ;
}

dasOneMethod dasOneMethod_create(char *id_or_name)
{
  dasOneMethod method = NULL;

  if(id_or_name && *id_or_name)
    {
      method     = g_new0(dasOneMethodStruct, 1);
      method->id = 
        method->name = g_quark_from_string(id_or_name);
    }

  return method;
}

void dasOneMethod_free(dasOneMethod meth)
{
  if(meth)
    g_free(meth);
  return ;
}

dasOneTarget dasOneTarget_create1(GQuark id, int start, int stop)
{
  dasOneTarget target = NULL;

  if((start > 0) && (stop > start))
    {
      target = g_new0(dasOneTargetStruct,1);
      target->id    = id;
      target->start = start;
      target->stop  = stop;
    }
    
  return target;
}
void dasOneTarget_free(dasOneTarget target)
{
  if(target)
    g_free(target);
  return ;
}


dasOneGroup dasOneGroup_create1(GQuark id, GQuark type, GQuark label)
{
  dasOneGroup grp = NULL;

  grp = g_new0(dasOneGroupStruct, 1);

  grp->id    = id;
  grp->type  = type;
  grp->label = label;

  return grp;
}
GQuark dasOneGroup_id1(dasOneGroup grp)
{
  return grp->id;
}
