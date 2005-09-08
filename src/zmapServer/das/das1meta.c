/*  File: das1dsn.c
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
 * Last edited: Sep  7 15:56 2005 (rds)
 * Created: Wed Aug 31 15:58:38 2005 (rds)
 * CVS info:   $Id: das1meta.c,v 1.2 2005-09-08 17:45:35 rds Exp $
 *-------------------------------------------------------------------
 */

#include <das1schema.h>
#include <ZMap/zmapFeature.h>

typedef struct _dasOneSegmentStruct
{
  GQuark id;
  int start;
  int stop;
  GQuark type;
  ZMapStrand orientation;
  GQuark description;
  dasOneRefProperties reference;
} dasOneSegmentStruct;

typedef struct _dasOneEntryPointStruct
{
  GQuark href;
  GQuark version;
  GList *segments;
} dasOneEntryPointStruct;

typedef struct _dasOneDSNStruct
{
  GQuark sourceId;
  GQuark version;
  GQuark name;
  GQuark mapMaster;
  GQuark description;           /* description href ignored for now! */
  dasOneEntryPoint entryPoint;
} dasOneDSNStruct;

/* DSN */
dasOneDSN dasOneDSN_create(char *id, char *version, char *name)
{
  GQuark qId = 0, qVersion = 0, qName = 0;

  if(id && *id)
    qId = g_quark_from_string(id);
  if(version && *version)
    qVersion = g_quark_from_string(version);
  if(name && *name)
    qName = g_quark_from_string(name);
  else
    qName = qId;

  return dasOneDSN_create1(qId, qVersion, qName);
}

dasOneDSN dasOneDSN_create1(GQuark id, GQuark version, GQuark name)
{
  dasOneDSN dsn = NULL;
  dsn = g_new0(dasOneDSNStruct, 1);

  dsn->sourceId = id;
  dsn->version  = version;
  dsn->name     = name;

  if(!dsn->sourceId)
    {
      dasOneDSN_free(dsn);
      return NULL;
    }

  return dsn;  
}

void dasOneDSN_setAttributes(dasOneDSN dsn, char *mapmaster, char *description)
{
  dasOneDSN_setAttributes1(dsn, 
                           ((mapmaster && *mapmaster)     ? g_quark_from_string(mapmaster)   : 0), 
                           ((description && *description) ? g_quark_from_string(description) : 0)
                           );
  return ;
}
void dasOneDSN_setAttributes1(dasOneDSN dsn, GQuark mapmaster, GQuark description)
{
  if(!dsn)
    return ;
  dsn->mapMaster   = mapmaster;
  dsn->description = description;
  return ;
}

dasOneEntryPoint dasOneDSN_entryPoint(dasOneDSN dsn, dasOneEntryPoint ep)
{
  if(!dsn)
    return NULL;

  if(ep != NULL)
    {
      if(dsn->entryPoint == NULL)
        dsn->entryPoint = ep;
      /* Should we have an else here? */
    }

  return (dsn->entryPoint);
}

GQuark dasOneDSN_id1(dasOneDSN dsn)
{
  return dsn->sourceId;
}

void dasOneDSN_free(dasOneDSN dsn)
{
  if(dsn->entryPoint)
    printf("freeing list\n");   /* free list here */
  g_free(dsn);
  return ;
}

/* ENTRY POINTS */

dasOneEntryPoint dasOneEntryPoint_create(char *href, char *version)
{
  return dasOneEntryPoint_create1(((href && *href)       ? g_quark_from_string(href)    : 0),
                                  ((version && *version) ? g_quark_from_string(version) : 0)
                                  );
}
dasOneEntryPoint dasOneEntryPoint_create1(GQuark href, GQuark version)
{
  dasOneEntryPoint ep = NULL;

  ep          = g_new0(dasOneEntryPointStruct, 1);
  ep->href    = href;
  ep->version = version;

  return ep;
}

void dasOneEntryPoint_addSegment(dasOneEntryPoint ep, dasOneSegment seg)
{
  if(!ep)
    return ;

  if(seg)
    ep->segments = g_list_append(ep->segments, seg);

  return ;

}
GList *dasOneEntryPoint_getSegmentsList(dasOneEntryPoint ep)
{
  GList *list = NULL;
  list = g_list_first(ep->segments);
  return list;
}

void dasOneEntryPoint_free(dasOneEntryPoint ep)
{
  if(!ep)
    return ;
  
  if(ep->segments)
    printf("freeing list\n");   /* free list here */

  g_free(ep);

  return ;
}

/* SEGEMENTS */

dasOneSegment dasOneSegment_create(char *id, int start, 
                                   int stop, char *type, 
                                   char *orientation)
{
  if(orientation && *orientation)
    {
      /* Check the orientation tediously to make sure everything's good */
    }

  return dasOneSegment_create1(((id && *id) ? g_quark_from_string(id) : 0),
                               start, stop,
                               ((type && *type) ? g_quark_from_string(type) : 0),
                               ((orientation && *orientation) ? g_quark_from_string(orientation) : 0)
                               );
}

dasOneSegment dasOneSegment_create1(GQuark id, int start, 
                                    int stop, GQuark type, 
                                    GQuark orientation)
{
  dasOneSegment seg = NULL;
  seg = g_new0(dasOneSegmentStruct, 1);

  seg->id    = id;
  seg->start = start;
  seg->stop  = stop;
  seg->type  = type;

  if(orientation)
    {
      if(orientation == g_quark_from_string("+"))
        seg->orientation = ZMAPSTRAND_FORWARD;
      if(orientation == g_quark_from_string("-"))
        seg->orientation = ZMAPSTRAND_REVERSE;
    }
  else{
    seg->orientation = ZMAPSTRAND_NONE;
  }
  return seg;
}

GQuark dasOneSegment_id1(dasOneSegment seg)
{
  return (seg->id);
}

void dasOneSegment_getBounds(dasOneSegment seg, int *start, int *end)
{
  if(start)
    *start = seg->start;
  if(end)
    *end   = seg->stop;

  return ;
}
dasOneRefProperties dasOneSegment_refProperties(dasOneSegment seg, 
                                                char *isRef,
                                                char *subparts,
#ifdef SEGMENTS_HAVE_SUPERPARTS
                                                char *superparts,
#endif /* SEGMENTS_HAVE_SUPERPARTS */
                                                gboolean set)
{
  dasOneRefProperties props = DASONE_REF_UNSET;
  
  if(set)
    {
      GQuark yes = 0;
      yes        = g_quark_from_string("yes");
      seg->reference = props;
      if(isRef && 
         *isRef && 
         (g_quark_from_string( g_ascii_strdown(isRef, -1) ) == yes))
        seg->reference |= DASONE_REF_ISREFERENCE;
        
      if(subparts && 
         *subparts && 
         (g_quark_from_string( g_ascii_strdown(subparts, -1) ) == yes))
        seg->reference |= DASONE_REF_HASSUBPARTS;
        
#ifdef SEGMENTS_HAVE_SUPERPARTS
      if(superparts && 
         *superparts && 
         (g_quark_from_string( g_ascii_strdown(superparts, -1) ) == yes))
        seg->reference |= DASONE_REF_HASSUPPARTS;
#endif /* SEGMENTS_HAVE_SUPERPARTS */
    }
  else
    props = seg->reference;

  return props;
}

char *dasOneSegment_description(dasOneSegment seg, char *desc)
{
  if(!seg)
    return NULL;
  
  if(desc && *desc)
    seg->description = g_quark_from_string(desc);
  else if(!seg->description)
    seg->description = g_quark_from_string("No description.");

  return (char *)g_quark_to_string( seg->description );
}
void dasOneSegment_free(dasOneSegment seg)
{
  if(seg)
    g_free(seg);

  return ;
}



