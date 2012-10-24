/*  File: zmapWindowContainerStrand.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>




#if USE_STRAND

#include <zmapWindowContainerStrand_I.h>


static void zmap_window_container_strand_class_init  (ZMapWindowContainerStrandClass container_class);
static void zmap_window_container_strand_init        (ZMapWindowContainerStrand      group);
static void zmap_window_container_strand_set_property(GObject               *object,
						      guint                  param_id,
						      const GValue          *value,
						      GParamSpec            *pspec);
static void zmap_window_container_strand_get_property(GObject               *object,
						      guint                  param_id,
						      GValue                *value,
						      GParamSpec            *pspec);
#ifdef EXTRA_DATA_NEEDS_FREE
static void zmap_window_container_strand_dispose     (GObject *object);
static void zmap_window_container_strand_finalize    (GObject *object);
#endif /* EXTRA_DATA_NEEDS_FREE */

#ifdef CHAINING_REQUIRED
static GObjectClass *parent_class_G = NULL;
#endif /* CHAINING_REQUIRED */

GType zmapWindowContainerStrandGetType(void)
{
  static GType group_type = 0;

  if (!group_type)
    {
      static const GTypeInfo group_info =
	{
	  sizeof (zmapWindowContainerStrandClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_container_strand_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowContainerStrand),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_container_strand_init

	};

      group_type = g_type_register_static (ZMAP_TYPE_CONTAINER_GROUP,
					   ZMAP_WINDOW_CONTAINER_STRAND_NAME,
					   &group_info,
					   0);
  }

  return group_type;
}



ZMapWindowContainerStrand zmapWindowContainerStrandAugment(ZMapWindowContainerStrand container_strand,
							   ZMapStrand strand)
{
  container_strand->strand = strand;

  return container_strand;
}

void zmapWindowContainerStrandSetAsSeparator(ZMapWindowContainerStrand container_strand)
{
  container_strand->strand = CONTAINER_STRAND_SEPARATOR;

  return ;
}

ZMapWindowContainerStrand zMapWindowContainerStrandDestroy(ZMapWindowContainerStrand container_strand)
{
  gtk_object_destroy(GTK_OBJECT(container_strand));

  container_strand = NULL;

  return container_strand;
}

static void zmap_window_container_strand_class_init  (ZMapWindowContainerStrandClass container_class)
{
  GObjectClass *gobject_class;
  FooCanvasItemClass *item_class;

  gobject_class = (GObjectClass *)container_class;
  item_class    = (FooCanvasItemClass *)container_class;

  gobject_class->set_property = zmap_window_container_strand_set_property;
  gobject_class->get_property = zmap_window_container_strand_get_property;

#ifdef CHAINING_REQUIRED
  parent_class_G = g_type_class_peek_parent(container_class);
#endif /* CHAINING_REQUIRED */


  return ;
}

static void zmap_window_container_strand_init        (ZMapWindowContainerStrand      container_strand)
{

  container_strand->strand = ZMAPSTRAND_NONE;

  return ;
}

static void zmap_window_container_strand_set_property(GObject               *object,
						      guint                  param_id,
						      const GValue          *value,
						      GParamSpec            *pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);

  return ;
}

static void zmap_window_container_strand_get_property(GObject               *object,
						      guint                  param_id,
						      GValue                *value,
						      GParamSpec            *pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);

  return ;
}

#ifdef EXTRA_DATA_NEEDS_FREE
static void zmap_window_container_strand_dispose(GObject *object)
{
  return ;
}

static void zmap_window_container_strand_finalize(GObject *object)
{
  return ;
}
#endif /* EXTRA_DATA_NEEDS_FREE */


#endif
