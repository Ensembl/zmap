/*  File: zmapFeatureTypes.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Functions for manipulating Type structs and sets of
 *              type structs.c
 *              
 * Exported functions: See ZMap/zmapFeature.h
 * HISTORY:
 * Last edited: Feb  1 09:30 2005 (edgrif)
 * Created: Tue Dec 14 13:15:11 2004 (edgrif)
 * CVS info:   $Id: zmapFeatureTypes.c,v 1.1 2005-02-02 15:08:36 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfig.h>
#include <zmapFeature_P.h>


/* Think about defaults, how should they be set, should we force user to set them ? */
#define ZMAPFEATURE_DEFAULT_WIDTH 10.0			    /* of features being displayed */


static void doTypeSets(GQuark key_id, gpointer data, gpointer user_data) ;
static void typePrintFunc(GQuark key_id, gpointer data, gpointer user_data) ;



/* Create a new type for displaying features. */
ZMapFeatureTypeStyle zMapFeatureTypeCreate(char *name,
					   char *outline, char *foreground, char *background,
					   float width, gboolean show_up_strand, int min_mag)
{
  ZMapFeatureTypeStyle new_type = NULL ;
  GString *name_canonical ;

  /* I am unsure if this is a good thing to do here, I'm attempting to make sure the type is
   * "sane" but no more than that. */
  zMapAssert(name && *name
	     && outline && *outline && foreground && *foreground && background && *background
	     && width > 0.0 && min_mag >= 0) ;


  new_type = g_new0(ZMapFeatureTypeStyleStruct, 1) ;

  /* lowercase the name to make it canonical. */
  name_canonical = g_string_new(name) ;
  name_canonical = g_string_ascii_down(name_canonical) ;
  new_type->name = g_strdup(name_canonical->str) ;
  g_string_free(name_canonical, TRUE) ;

  gdk_color_parse(outline, &new_type->outline) ;
  gdk_color_parse(foreground, &new_type->foreground) ;
  gdk_color_parse(background, &new_type->background) ;

  new_type->width = width ;
  new_type->showUpStrand = show_up_strand ;
  new_type->min_mag = min_mag ;

  return new_type ;
}


/* Copy an existing type. */
ZMapFeatureTypeStyle zMapFeatureTypeCopy(ZMapFeatureTypeStyle type)
{
  ZMapFeatureTypeStyle new_type = NULL ;

  zMapAssert(type) ;

  new_type = g_new0(ZMapFeatureTypeStyleStruct, 1) ;
  *new_type = *type ;					    /* n.b. struct copy. */
  new_type->name = g_strdup(type->name) ;

  return new_type ;
}


/* Destroy the type, freeing all resources. */
void zMapFeatureTypeDestroy(ZMapFeatureTypeStyle type)
{
  zMapAssert(type && type->name) ;

  g_free(type->name) ;

  g_free(type) ;

  return ;
}







gboolean zMapFeatureTypeSetAugment(GData **current, GData **new)
{
  gboolean result = FALSE ;

  zMapAssert(new && *new) ;

  if (!current || !*current)
    g_datalist_init(current) ;
  
  g_datalist_foreach(new, doTypeSets, (void *)current) ;

  result = TRUE ;					    /* currently shouldn't fail.... */

  return result ;
}



/* Read the type/method/source (call it what you will) information from the given file
 * which currently must reside in the users $HOME/.ZMap directory. */
GData *zMapFeatureTypeGetFromFile(char *types_file_name)
{
  GData *types = NULL ;
  gboolean result = FALSE ;
  ZMapConfigStanzaSet types_list = NULL ;
  ZMapConfig config ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  char *types_file_name = "ZMapTypes" ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if ((config = zMapConfigCreateFromFile(NULL, types_file_name)))
    {
      ZMapConfigStanza types_stanza ;

      /* Set up default values for variables in the stanza.  Elements here must match
       * those being loaded below or you might segfault.
       * IF YOU ADD ANY ELEMENTS TO THIS ARRAY THEN MAKE SURE YOU UPDATE THE INIT STATEMENTS
       * FOLLOWING THIS ARRAY SO THEY POINT AT THE RIGHT ELEMENTS....!!!!!!!! */
      ZMapConfigStanzaElementStruct types_elements[] = {{"name"        , ZMAPCONFIG_STRING, {NULL}},
							{"outline"     , ZMAPCONFIG_STRING, {"black"}},
							{"foreground"  , ZMAPCONFIG_STRING, {"white"}},
							{"background"  , ZMAPCONFIG_STRING, {"black"}},
							{"width"       , ZMAPCONFIG_FLOAT , {NULL}},
							{"showUpStrand", ZMAPCONFIG_BOOL  , {NULL}},
							{"minmag"      , ZMAPCONFIG_INT   , {NULL}},
							{NULL, -1, {NULL}}} ;

      /* minmag should be a float !!! */


      /* Must init separately as compiler cannot statically init different union types....sigh.... */
      types_elements[4].data.f = ZMAPFEATURE_DEFAULT_WIDTH ;


      types_stanza = zMapConfigMakeStanza("Type", types_elements) ;

      if (!zMapConfigFindStanzas(config, types_stanza, &types_list))
	result = FALSE ;
      else
	result = TRUE ;
    }

  /* Set up connections to the named types. */
  if (result)
    {
      int num_types = 0 ;
      ZMapConfigStanza next_types ;

      g_datalist_init(&types) ;

      /* Current error handling policy is to connect to servers that we can and
       * report errors for those where we fail but to carry on and set up the ZMap
       * as long as at least one connection succeeds. */
      next_types = NULL ;
      while (result
	     && ((next_types = zMapConfigGetNextStanza(types_list, next_types)) != NULL))
	{
	  char *name ;

	  /* Name must be set so if its not found then don't make a struct.... */
	  if ((name = zMapConfigGetElementString(next_types, "name")))
	    {
	      ZMapFeatureTypeStyle new_type ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      ZMapFeatureTypeStyle new_type = g_new0(ZMapFeatureTypeStyleStruct, 1) ;

	      /* NB Elements here must match those pre-initialised above or you might segfault. */

	      gdk_color_parse(zMapConfigGetElementString(next_types, "outline"   ), &new_type->outline   ) ;
	      gdk_color_parse(zMapConfigGetElementString(next_types, "foreground"), &new_type->foreground) ;
	      gdk_color_parse(zMapConfigGetElementString(next_types, "background"), &new_type->background) ;
	      new_type->width = zMapConfigGetElementFloat(next_types, "width") ;
	      new_type->showUpStrand = zMapConfigGetElementBool(next_types, "showUpStrand") ;
	      new_type->min_mag = zMapConfigGetElementInt(next_types, "minmag") ;

	      /* lowercase the name (aka type).....and FREE the memory....sigh...... */
	      name_canonical = g_string_new(name) ;
	      name_canonical = g_string_ascii_down(name_canonical) ;
	      new_type->name = g_strdup(name_canonical->str) ;
	      g_string_free(name_canonical, TRUE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      new_type = zMapFeatureTypeCreate(name,
					       zMapConfigGetElementString(next_types, "outline"),
					       zMapConfigGetElementString(next_types, "foreground"),
					       zMapConfigGetElementString(next_types, "background"),
					       zMapConfigGetElementFloat(next_types, "width"),
					       zMapConfigGetElementBool(next_types, "showUpStrand"),
					       zMapConfigGetElementInt(next_types, "minmag")) ;

	      g_datalist_set_data(&types, new_type->name, new_type) ;
	      num_types++ ;
	    }
	  else
	    {
	      zMapLogWarning("config file \"%s\" has a \"Type\" stanza which has no \"name\" element, "
			     "the stanza has been ignored.", types_file_name) ;
	    }
	}

      /* Found no valid types.... */
      if (!num_types)
	result = FALSE ;
    }

  /* clean up. */
  if (types_list)
    zMapConfigDeleteStanzaSet(types_list) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* debug.... */
  printAllTypes(types, "getTypesFromFile()") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return types ;
}


void zMapFeatureTypePrintAll(GData *type_set, char *user_string)
{
  printf("\nTypes at %s\n", user_string) ;

  g_datalist_foreach(&type_set, typePrintFunc, NULL) ;

  return ;
}





/* 
 *                  Internal routines
 */



static void doTypeSets(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle new_type = (ZMapFeatureTypeStyle)data ;
  GData **current_out = (GData **)user_data ;
  GData *current = *current_out ;

  if (!g_datalist_id_get_data(&current, key_id))
    {
      /* copy the struct and then add it to the current set. */
      ZMapFeatureTypeStyle type ;

      type = zMapFeatureTypeCopy(new_type) ;

      g_datalist_id_set_data(&current, key_id, type) ;
    }

  *current_out = current ;

  return ;
}


static void typePrintFunc(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;
  char *style_name = (char *)g_quark_to_string(key_id) ;
  
  printf("\t%s: \t%f \t%d\n", style_name, style->width, style->showUpStrand) ;

  return ;
}


