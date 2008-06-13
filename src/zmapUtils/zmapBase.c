/*  File: zmapZMap.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * Last edited: Jun 13 09:28 2008 (rds)
 * Created: Thu Jun 12 12:02:12 2008 (rds)
 * CVS info:   $Id: zmapBase.c,v 1.1 2008-06-13 08:52:59 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapBase_I.h>

enum
  {
    NO_PROP,
    ZMAP_BASE_DEBUG,
  };

static void zmap_base_base_init   (ZMapBaseClass zmap_base_class);
static void zmap_base_class_init  (ZMapBaseClass zmap_base_class);
static void zmap_base_inst_init   (ZMapBase zmap_base);
static void zmap_base_set_property(GObject *gobject, 
				   guint param_id, 
				   const GValue *value, 
				   GParamSpec *pspec);
static void zmap_base_get_property(GObject *gobject, 
				   guint param_id, 
				   GValue *value, 
				   GParamSpec *pspec);
#ifdef ZMAP_BASE_NEEDS_DISPOSE_FINALIZE
static void zmap_base_dispose      (GObject *object);
static void zmap_base_finalize     (GObject *object);
#endif /* ZMAP_BASE_NEEDS_DISPOSE_FINALIZE */

static gboolean zmapBaseCopy(ZMapBase src, ZMapBase *dest_out, gboolean reference_copy);
static void zmapBaseCopyConstructor(const GValue *src_value, GValue *dest_value);



/* Public functions */

GType zMapBaseGetType (void)
{
  static GType type = 0;
  
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (zmapBaseClass),
      (GBaseInitFunc)      zmap_base_base_init,
      (GBaseFinalizeFunc)  NULL,
      (GClassInitFunc)     zmap_base_class_init,
      (GClassFinalizeFunc) NULL,
      NULL			/* class_data */,
      sizeof (zmapBase),
      0				/* n_preallocs */,
      (GInstanceInitFunc) zmap_base_inst_init,
      NULL
    };
    
    type = g_type_register_static (G_TYPE_OBJECT, "ZMapBase", &info, (GTypeFlags)0);
  }
  
  return type;
}

/* Copy by reference */
ZMapBase zMapBaseCopy(ZMapBase src)
{
  ZMapBase dest = NULL;
  gboolean done = TRUE;

  done = zmapBaseCopy(src, &dest, done);

  return dest;
}

/* Copy constructor. */
gboolean zMapBaseCCopy(ZMapBase src, ZMapBase *dest_out)
{
  gboolean done = FALSE;

  done = zmapBaseCopy(src, dest_out, done);

  return done;
}


/* Object implementation */

static void zmap_base_base_init   (ZMapBaseClass zmap_base_class)
{
  zmap_base_class->copy_set_property = NULL;

  return ;
}

static void zmap_base_class_init  (ZMapBaseClass zmap_base_class)
{
  GObjectClass *gobject_class;
  
  gobject_class = (GObjectClass *)zmap_base_class;

  gobject_class->set_property = zmap_base_set_property;
  gobject_class->get_property = zmap_base_get_property;

  g_object_class_install_property(gobject_class,
				  ZMAP_BASE_DEBUG,
				  g_param_spec_boolean("debug", "debug",
						       "Debugging flag",
						       FALSE, ZMAP_PARAM_STATIC_RW));

  /* copy constructor setup, by default this is the same as the set_property */
  zmap_base_class->copy_set_property = zmap_base_set_property;

#ifdef ZMAP_BASE_NEEDS_DISPOSE_FINALIZE
  gobject_class->dispose  = zmap_base_dispose;
  gobject_class->finalize = zmap_base_finalize;
#endif /* ZMAP_BASE_NEEDS_DISPOSE_FINALIZE */

  return ;
}

static void zmap_base_inst_init        (ZMapBase zmap_base)
{
  /* Nothing */
  return ;
}

static void zmap_base_set_property(GObject *gobject, 
				   guint param_id, 
				   const GValue *value, 
				   GParamSpec *pspec)
{
  ZMapBase base;

  g_return_if_fail(ZMAP_IS_BASE(gobject));

  base = ZMAP_BASE(gobject);

  switch(param_id)
    {
    case ZMAP_BASE_DEBUG:
      base->debug = g_value_get_boolean(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_base_get_property(GObject *gobject, 
				   guint param_id, 
				   GValue *value, 
				   GParamSpec *pspec)
{
  ZMapBase base;

  g_return_if_fail(ZMAP_IS_BASE(gobject));

  base = ZMAP_BASE(gobject);

  switch(param_id)
    {
    case ZMAP_BASE_DEBUG:
      g_value_set_boolean(value, base->debug);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

#ifdef ZMAP_BASE_NEEDS_DISPOSE_FINALIZE
static void zmap_base_dispose      (GObject *object)
{
  return ;
}

static void zmap_base_finalize     (GObject *object)
{
  return ;
}
#endif /* ZMAP_BASE_NEEDS_DISPOSE_FINALIZE */





/* INTERNAL */

static gboolean zmapBaseCopy(ZMapBase src, ZMapBase *dest_out, gboolean reference_copy)
{
  ZMapBase dest = NULL;
  ZMapBaseClass zmap_class;
  GObject *gobject_src;
  GType gobject_type;
  gboolean copied = FALSE;
  GTypeValueTable *value_table;
  gpointer value_copy;

  g_return_val_if_fail(dest_out != NULL, FALSE);
  gobject_src   = G_OBJECT(src);
  gobject_type  = G_OBJECT_TYPE(src);
  zmap_class    = ZMAP_BASE_GET_CLASS(gobject_src);

  /* get vtable */
  value_table = g_type_value_table_peek(gobject_type);
  /* save current value copy */
  value_copy = value_table->value_copy;
  
  if(reference_copy || zmap_class->copy_set_property)
    {
      GValue src_value = {0}, dest_value = {0};
      
      g_value_init(&src_value,  gobject_type);
      g_value_init(&dest_value, gobject_type);
      
      g_value_set_object(&src_value,  src);

      if(!reference_copy)
	value_table->value_copy = zmapBaseCopyConstructor;

      /* g_value_copy memset 0's dest_value.data so don't set here,
       * but in value_table->value_copy */
      g_value_copy(&src_value, &dest_value);
      
      /* return it to caller */
      dest      = g_value_get_object(&dest_value);
      *dest_out = dest;

      g_value_unset(&src_value);
      g_value_unset(&dest_value);

      copied = TRUE;
    }
  
  /* restore */
  value_table->value_copy = value_copy;
    
  return copied;
}

static void zmapBaseCopyConstructor(const GValue *src_value, GValue *dest_value)
{
  GObject      *gobject_src;
  GObjectClass *gobject_class;
  ZMapBaseClass zmap_class;

  gobject_src   = g_value_get_object(src_value);
  gobject_class = G_OBJECT_GET_CLASS(gobject_src);
  zmap_class    = ZMAP_BASE_GET_CLASS(gobject_src);

  if(zmap_class->copy_set_property)
    {
      GParamSpec **param_specs = NULL;
      GObject     *gobject_dest;
      GType        gobject_type;
      guint        count = 0, i;

      gobject_type = G_OBJECT_TYPE(gobject_src);
      gobject_dest = g_object_new(gobject_type, NULL);

      param_specs  = g_object_class_list_properties(gobject_class, &count);
      
      for(i = 0; param_specs && i < count; i++, param_specs++)
	{
	  GParamSpec *current = *param_specs, *redirect;
	  GType current_type  = G_PARAM_SPEC_VALUE_TYPE(current);
#ifdef GET_NAME_FOR_DEBUG
	  const char *name    = g_param_spec_get_name(current);
#endif /* GET_NAME_FOR_DEBUG */
	  /* Access to this is the _only_ problem here, according to docs it's not public.
	   * It does save g_object_get and allows us to use the object methods directly.
	   * Also the copy_set_property method can have the same signature as get/set_prop */
	  guint param_id      = current->param_id;
	  GValue value        = { 0, };
	  
	  g_value_init(&value, current_type);
	  
	  gobject_class = g_type_class_peek(current->owner_type);
	  if((redirect  = g_param_spec_get_redirect_target(current)))
	    current     = redirect;
	  
	  gobject_class->get_property(gobject_src,  param_id, &value, current);

	  zmap_class->copy_set_property(gobject_dest, param_id, &value, current);

	  g_value_unset(&value);
	}

      g_value_set_object(dest_value, gobject_dest);
    }


  return ;
}


