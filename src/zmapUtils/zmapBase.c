/*  File: zmapZMap.c
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See ZMap/zmapBase.h
 * HISTORY:
 * Last edited: Jan 12 11:52 2009 (rds)
 * Created: Thu Jun 12 12:02:12 2008 (rds)
 * CVS info:   $Id: zmapBase.c,v 1.10 2010-03-15 11:00:39 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <zmapBase_I.h>
#include <string.h>
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

static gboolean zmapBaseCopy(ZMapBase src, ZMapBase *dest_out, gboolean copy_by_reference);
static gboolean zmapBaseCopyValue(const GValue *src_value, GValue *dest_value, ZMapBaseValueCopyFunc value_copy);
static void zmapBaseCopyConstructor(const GValue *src_value, GValue *dest_value);



/* Public functions */

GType zMapBaseGetType (void)
{
  static GType type = 0;

  if (type == 0)
    {
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

/*!
 * \brief Copy a ZMapBase or ZMapBase Derived object.  The copy is
 * done by reference only and ultimately only calls g_object_ref()
 *
 * @param      The original object
 *
 * @return     The original object with a ref count + 1.
 */
ZMapBase zMapBaseCopy(ZMapBase src)
{
  ZMapBase dest = NULL;
  gboolean copy_by_reference = TRUE;

  /* Copy by reference only. g_object_ref() is all that happens */
  zmapBaseCopy(src, &dest, copy_by_reference);

  return dest;
}


/*!
 * \brief Copy a ZMapBase or ZMapBase Derived object. The copy is a
 * deep copy and results in the getting and setting of each property
 * in the original and new objects (respectively).
 *
 * @param    The original object
 * @param    The location to store the new object.
 *
 * @return   True if success was met.
 */
gboolean zMapBaseCCopy(ZMapBase src, ZMapBase *dest_out)
{
  gboolean copy_by_reference = FALSE;
  gboolean done = FALSE;

  /* Deep Copy... */
  done = zmapBaseCopy(src, dest_out, copy_by_reference);

  return done;
}

gboolean zMapBaseDebug(GObject *gobject)
{
  ZMapBase base = ZMAP_BASE(gobject);
  return base->debug;
}

/* Object implementation */

static void zmap_base_base_init   (ZMapBaseClass zmap_base_class)
{
  zmap_base_class->copy_set_property = NULL;    // mh17: this is for derived classes not zmapBase

  zmap_base_class->value_copy = zmapBaseCopyConstructor;

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


/*
 * zmapBaseCopy and zmapBaseCopyValue are the covering functions to
 * handle the copying of ZMapBase and their derived objects.  They
 * really just cover the zmapBaseCopyConstructor() or the default
 * g_value_object_copy_value(). zmapBaseCopyValue is a direct copy
 * of g_value_copy() with a small alteration so that we pass in the
 * GTypeValueTable->value_copy function rather than have it be used
 * directly from the table of the GType (g_type_value_table_peek).
 */

/*!
 * \brief Copy a ZMapBase or derived object, either by reference or
 * using class defined/overridden copy function.
 *
 * @param    The original object
 * @param    The location to store the new object
 * @param    Flag for copy by reference. TRUE  = Copy by reference
 *           FALSE = Use ZMapBaseClass->value_copy
 *
 * @return   How successful copy was. TRUE == Success
 */
static gboolean zmapBaseCopy(ZMapBase src, ZMapBase *dest_out, gboolean copy_by_reference)
{
  ZMapBaseValueCopyFunc value_copy_func = NULL;
  GValue src_value = {0}, dest_value = {0};
  ZMapBase dest = NULL;
  gboolean done = FALSE;
  GType gobject_type;

  if(dest_out)
    {
      gobject_type  = G_TYPE_FROM_INSTANCE(src);

      if(copy_by_reference)
	{
	  GTypeValueTable *value_table;

	  /* get vtable */
	  value_table = g_type_value_table_peek(gobject_type);

	  value_copy_func = value_table->value_copy;
	}
      else
	{
	  ZMapBaseClass base_class = NULL;

	  base_class = ZMAP_BASE_GET_CLASS(src);

	  /* Use the class defined value_copy */
	  value_copy_func = base_class->value_copy;
	}

      if(value_copy_func != NULL)
	{
	  g_value_init(&src_value,  gobject_type);
	  g_value_init(&dest_value, gobject_type);

	  g_value_set_object(&src_value,  src);

	  if((done = zmapBaseCopyValue(&src_value, &dest_value, value_copy_func)))
	    {
	      /* return it to caller */
	      dest      = g_value_get_object(&dest_value);
	      *dest_out = dest;
	    }

	  g_value_unset(&src_value);
	  g_value_unset(&dest_value);
	}
    }

  return done;
}

/*!
 * \brief Almost identical to g_value_copy().  We pass in the
 * value_copy function though.
 *
 * @param    The original GValue
 * @param    The new GValue
 * @param    The function to use to copy the GValue from
 *           origin to new.
 *
 * @return   How successful copying was. TRUE == Success
 */
static gboolean zmapBaseCopyValue(const GValue *src_value, GValue *dest_value, ZMapBaseValueCopyFunc value_copy)
{
  gboolean done = FALSE;

  g_return_val_if_fail (G_IS_VALUE (src_value), FALSE);
  g_return_val_if_fail (G_IS_VALUE (dest_value), FALSE);
  g_return_val_if_fail (g_value_type_compatible (G_VALUE_TYPE (src_value), G_VALUE_TYPE (dest_value)), FALSE);

  if (value_copy && src_value != dest_value)
    {
      GType dest_type = G_VALUE_TYPE (dest_value);
      GTypeValueTable *value_table = g_type_value_table_peek (dest_type);

      /* make sure dest_value's value is free()d */
      if (value_table->value_free)
	value_table->value_free (dest_value);

      /* setup and copy */
#ifdef RDS_INIT_PROBLEM
      g_value_init(dest_value, dest_type);
      /*
       * value_meminit (dest_value, dest_type);
       * g_value_copy() uses value_meminit here, but that's static so
       * I used g_value_init(), but that does some sanity checks.  The
       * solution is to reimplement value_meminit, which has the warning
       * keep this function in sync with gvaluecollector.h and gboxed.c
       */
#endif
      /* Reimplementation of value_meminit() */
      dest_value->g_type = dest_type;
      memset(dest_value->data, 0, sizeof(dest_value->data));
      /* End of value_meminit reimplementation. */

      value_copy(src_value, dest_value);

      done = TRUE;
    }

  return done;
}

/*!
 * \brief Copy all data from one ZMapBase, or derived, object to
 * another.  This involves _creating_ a new ZMapBase (g_object_new()),
 * getting properties from the original one and setting them on the
 * new one.  This avoids duplicating this code throughout any objects
 * we need to do this with.
 *
 * This function is called by zmapBaseCopyValue in the line
 * value_copy(src_value, dest_value);
 *
 * @param   The original GValue
 * @param   The new GValue (EMPTY)
 *
 * @return  Nothing
 */
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
      GParamSpec **param_specs = NULL, **ps_ptr;
      GObject     *gobject_dest;
      GType        gobject_type;
      guint        count = 0, i;


      /* Create the object which will the new copy. */
      gobject_type = G_OBJECT_TYPE(gobject_src);
      gobject_dest = g_object_new(gobject_type, NULL);

      /* Now copy all the original objects data, works by calling get_property on original
       * object and then set_property on new object. */
      param_specs  = g_object_class_list_properties(gobject_class, &count);
      ps_ptr       = param_specs;
      for (i = 0; param_specs && i < count; i++, param_specs++)
	{
	  GParamSpec *current = *param_specs, *redirect;
	  GType current_type  = G_PARAM_SPEC_VALUE_TYPE(current);
#ifdef COPY_CONSTRUCT_DEBUG
	  const char *name    = g_param_spec_get_name(current);
#endif /* COPY_CONSTRUCT_DEBUG */

	  /* Access to this is the _only_ problem here, according to docs it's not public.
	   * It does save g_object_get and allows us to use the object methods directly.
	   * Also the copy_set_property method can have the same signature as get/set_prop */
	  guint param_id      = current->param_id;
	  GValue value        = { 0, };

	  g_value_init(&value, current_type);

	  gobject_class = g_type_class_peek(current->owner_type);
	  zmap_class    = ZMAP_BASE_CLASS(gobject_class);

	  if((redirect  = g_param_spec_get_redirect_target(current)))
	    current     = redirect;

#ifdef COPY_CONSTRUCT_DEBUG
	  printf("Copy Constructor %s\n", name);
#endif

	  gobject_class->get_property(gobject_src, param_id, &value, current);

	  /* Store a pointer to the original object on the new object so it can be retrieved in the
	   * new objects copy code otherwise copy code cannot see original object ! */
	  g_object_set_data(gobject_dest, ZMAPBASECOPY_PARAMDATA_KEY, gobject_src) ;

	  /* do the copy... */
	  zmap_class->copy_set_property(gobject_dest, param_id, &value, current);

	  /* Now remove the pointer, V. IMPORTANT because "set-property" routine gets called
	   * both to set properties per se and during object copy and set routine decides
	   * which is which by whether pointer is NULL. */
	  g_object_set_data(gobject_dest, ZMAPBASECOPY_PARAMDATA_KEY, NULL) ;

	  g_value_unset(&value);
	}

      if(ps_ptr)
	g_free(ps_ptr);

      g_value_set_object(dest_value, gobject_dest);
    }

  return ;
}


