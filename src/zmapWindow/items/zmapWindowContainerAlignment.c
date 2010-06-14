/*  File: zmapWindowContainerAlignment.c
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: May 11 16:13 2010 (edgrif)
 * Created: Mon Jul 30 13:09:33 2007 (rds)
 * CVS info:   $Id: zmapWindowContainerAlignment.c,v 1.6 2010-06-14 15:40:17 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <zmapWindowContainerAlignment_I.h>
#include <zmapWindowContainerUtils.h>

static void zmap_window_container_alignment_class_init  (ZMapWindowContainerAlignmentClass container_align_class);
static void zmap_window_container_alignment_init        (ZMapWindowContainerAlignment container_align);
static void zmap_window_container_alignment_set_property(GObject      *gobject, 
							 guint         param_id, 
							 const GValue *value, 
							 GParamSpec   *pspec);
static void zmap_window_container_alignment_get_property(GObject    *gobject, 
							 guint       param_id, 
							 GValue     *value, 
							 GParamSpec *pspec);
#ifdef EXTRA_DATA_NEEDS_FREE
static void zmap_window_container_alignment_dispose     (GObject *object);
static void zmap_window_container_alignment_finalize    (GObject *object);
#endif /* EXTRA_DATA_NEEDS_FREE */

#ifdef CHAINING_REQUIRED
static GObjectClass *parent_class_G = NULL;
#endif /* CHAINING_REQUIRED */

GType zmapWindowContainerAlignmentGetType(void)
{
  static GType group_type = 0;
  
  if (!group_type) 
    {
      static const GTypeInfo group_info = 
	{
	  sizeof (zmapWindowContainerAlignmentClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_container_alignment_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (zmapWindowContainerAlignment),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) zmap_window_container_alignment_init
      };
    
    group_type = g_type_register_static (ZMAP_TYPE_CONTAINER_GROUP,
					 ZMAP_WINDOW_CONTAINER_ALIGNMENT_NAME,
					 &group_info,
					 0);
  }
  
  return group_type;
}


ZMapWindowContainerAlignment zmapWindowContainerAlignmentCreate(FooCanvasGroup *parent)
{
  ZMapWindowContainerAlignment container_align = NULL;

  return container_align;
}

ZMapWindowContainerAlignment zmapWindowContainerAlignmentAugment(ZMapWindowContainerAlignment alignment,
								 ZMapFeatureAlignment feature)
{
  zmapWindowContainerAttachFeatureAny((ZMapWindowContainerGroup)alignment,
				      (ZMapFeatureAny)feature);

  return alignment;
}

ZMapWindowContainerAlignment zmapWindowContainerAlignmentDestroy(ZMapWindowContainerAlignment container_alignment)
{
  gtk_object_destroy(GTK_OBJECT(container_alignment));

  container_alignment = NULL;

  return container_alignment;
}


/* INTERNAL */

/* Object code */
static void zmap_window_container_alignment_class_init(ZMapWindowContainerAlignmentClass container_align_class)
{
  GObjectClass *gobject_class;
  zmapWindowContainerGroupClass *group_class ;

  gobject_class = (GObjectClass *)container_align_class;
  group_class = (zmapWindowContainerGroupClass *)container_align_class ;

  gobject_class->set_property = zmap_window_container_alignment_set_property;
  gobject_class->get_property = zmap_window_container_alignment_get_property;
#ifdef CHAINING_REQUIRED
  parent_class_G = g_type_class_peek_parent(container_align_class);
#endif /* CHAINING_REQUIRED */

  group_class->obj_size = sizeof(zmapWindowContainerAlignmentStruct) ;
  group_class->obj_total = 0 ;

#ifdef EXTRA_DATA_NEEDS_FREE
  gobject_class->dispose  = zmap_window_container_alignment_dispose;
  gobject_class->finalize = zmap_window_container_alignment_finalize;
#endif /* EXTRA_DATA_NEEDS_FREE */

  return ;
}

static void zmap_window_container_alignment_init(ZMapWindowContainerAlignment container_align)
{
  return ;
}

static void zmap_window_container_alignment_set_property(GObject      *gobject, 
							 guint         param_id, 
							 const GValue *value, 
							 GParamSpec   *pspec)
{

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_window_container_alignment_get_property(GObject    *gobject, 
							 guint       param_id, 
							 GValue     *value, 
							 GParamSpec *pspec)
{
  ZMapWindowContainerAlignment container_align;

  container_align = ZMAP_CONTAINER_ALIGNMENT(gobject);

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

#ifdef EXTRA_DATA_NEEDS_FREE
static void zmap_window_container_alignment_dispose(GObject *object)
{

  return ;
}

static void zmap_window_container_alignment_finalize(GObject *object)
{

  return ;
}
#endif /* EXTRA_DATA_NEEDS_FREE */
