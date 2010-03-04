/*  File: zmapDAS1EntryPoint.c
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 24 17:07 2006 (rds)
 * Created: Wed May 24 11:43:47 2006 (rds)
 * CVS info:   $Id: zmapDAS1DSN.c,v 1.3 2010-03-04 15:10:04 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapDAS.h>


typedef struct
{
  GString *query;
  char separator;
}refSegmentStruct, *refSegment;


static void segmentListToQueryString(gpointer data, gpointer user_data);

gboolean zMapDAS1DSNSegments(ZMapDAS1DSN dsn, GList **segments_out)
{
  ZMapDAS1EntryPoint entry_point = NULL;
  gboolean has_segs = FALSE;
  GList *list = NULL;

  if((entry_point = dsn->entry_point) && 
     (list = entry_point->segments))
    has_segs = TRUE;

  if(segments_out)
    *segments_out = list;

  return has_segs;
}

char *zMapDAS1DSNRefSegmentsQueryString(ZMapDAS1DSN dsn, char *separator)
{
  ZMapDAS1EntryPoint entry_point = NULL;
  GList *segments = NULL;
  char *query_string = NULL;
  refSegmentStruct full_data = {0};
  GString *query = NULL;

  full_data.separator = *separator;
  full_data.query = 
    query = g_string_sized_new(400); /* segment=ID:1,1000000 (length = 20) * 20 = 400 */

  if((entry_point = dsn->entry_point))
    {
      if((segments = entry_point->segments))
        g_list_foreach(segments, segmentListToQueryString, (gpointer)(&full_data));
    }

  if(query->str)
    query_string = query->str;

  g_string_free(query, FALSE);

  return query_string;          /* THIS NEEDS FREEING! */
}





static void segmentListToQueryString(gpointer data, gpointer user_data)
{
  ZMapDAS1Segment seg = (ZMapDAS1Segment)data;
  refSegment data_ptr = (refSegment)user_data;
  
  if(seg->is_reference && seg->has_subparts)
    g_string_append_printf(data_ptr->query, "segment=%s:%d,%d%c", 
                           g_quark_to_string(seg->id),
                           seg->start,
                           seg->stop,
                           data_ptr->separator);
  
  return ;
}


