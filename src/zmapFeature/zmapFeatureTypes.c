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
 * Last edited: May 27 13:23 2005 (edgrif)
 * Created: Tue Dec 14 13:15:11 2004 (edgrif)
 * CVS info:   $Id: zmapFeatureTypes.c,v 1.6 2005-05-27 15:14:31 edgrif Exp $
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
					   double width, double min_mag)
{
  ZMapFeatureTypeStyle new_type = NULL ;
  char *name_lower ;

  /* I am unsure if this is a good thing to do here, I'm attempting to make sure the type is
   * "sane" but no more than that. */
  zMapAssert(name && *name && width >= 0.0 && min_mag >= 0) ;


  if (width == 0)
    width = 5 ;

  /* Set some default colours.... */
  if (!outline)
    outline = "black" ;
  if (!foreground)
    foreground = "white" ;
  if (!background)
    background = "white" ;

  new_type = g_new0(ZMapFeatureTypeStyleStruct, 1) ;

  name_lower = g_ascii_strdown(name, -1) ;
  new_type->original_id = g_quark_from_string(name) ;
  new_type->unique_id = zMapStyleCreateID(name_lower) ;
  g_free(name_lower) ;

  gdk_color_parse(outline, &new_type->outline) ;
  gdk_color_parse(foreground, &new_type->foreground) ;
  gdk_color_parse(background, &new_type->background) ;

  new_type->width = width ;
  new_type->min_mag = min_mag ;

  return new_type ;
}



/* These attributes are not needed for many features and are not independent,
 * hence we set them in a special routine, none of this is very good as we don't have
 * a good way of enforcing stuff...so its all a bit heuristic. */
void zMapStyleSetStrandAttrs(ZMapFeatureTypeStyle type,
			     gboolean strand_specific, gboolean frame_specific,
			     gboolean show_rev_strand)
{
  zMapAssert(type) ;

  if (frame_specific && !strand_specific)
    strand_specific = TRUE ;

  if (show_rev_strand && !strand_specific)
    strand_specific = TRUE ;

  type->strand_specific = strand_specific ;
  type->frame_specific = frame_specific ;
  type->show_rev_strand = show_rev_strand ;

  return ;
}


/* Pretty brain dead but we need some way to deal with the situation where a style may differ in
 * case only betweens servers...perhaps we should not be merging....???
 * Caller must free returned string.
 *  */
char *zMapStyleCreateName(char *style)
{
  char *style_name ;
  GString *unique_style_name ;

  unique_style_name = g_string_new(style) ;
  unique_style_name = g_string_ascii_down(unique_style_name) ; /* does it in place */

  style_name = unique_style_name->str ;

  g_string_free(unique_style_name, FALSE) ;		    /* Do not free returned string. */

  return style_name ;
}


/* Like zMapStyleCreateName() but returns a quark representing the style name. */
GQuark zMapStyleCreateID(char *style)
{
  GQuark style_id = 0 ;
  char *style_name ;

  style_name = zMapStyleCreateName(style) ;

  style_id = g_quark_from_string(style_name) ;

  g_free(style_name) ;

  return style_id ;
}


/* Copy an existing type. */
ZMapFeatureTypeStyle zMapFeatureTypeCopy(ZMapFeatureTypeStyle type)
{
  ZMapFeatureTypeStyle new_type = NULL ;

  zMapAssert(type) ;

  new_type = g_new0(ZMapFeatureTypeStyleStruct, 1) ;
  *new_type = *type ;					    /* n.b. struct copy. */

  return new_type ;
}


char *zMapStyleGetName(ZMapFeatureTypeStyle style)
{
  char *style_name ;

  style_name = (char *)g_quark_to_string(style->original_id) ;

  return style_name ;
}




/* Destroy the type, freeing all resources. */
void zMapFeatureTypeDestroy(ZMapFeatureTypeStyle type)
{
  zMapAssert(type) ;

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


  if ((config = zMapConfigCreateFromFile(types_file_name)))
    {
      ZMapConfigStanza types_stanza ;
      ZMapConfigStanzaElementStruct types_elements[]
	= {{"name"        , ZMAPCONFIG_STRING, {NULL}},
	   {"outline"     , ZMAPCONFIG_STRING, {"black"}},
	   {"foreground"  , ZMAPCONFIG_STRING, {"white"}},
	   {"background"  , ZMAPCONFIG_STRING, {"black"}},
	   {"width"       , ZMAPCONFIG_FLOAT , {NULL}},
	   {"show_reverse", ZMAPCONFIG_BOOL  , {NULL}},
	   {"strand_specific", ZMAPCONFIG_BOOL  , {NULL}},
	   {"frame_specific", ZMAPCONFIG_BOOL  , {NULL}},
	   {"minmag"      , ZMAPCONFIG_INT   , {NULL}},
	   {NULL, -1, {NULL}}} ;

      /* Init fields that cannot default to string NULL. */
      zMapConfigGetStructFloat(types_elements, "width") = ZMAPFEATURE_DEFAULT_WIDTH ;

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
	      gboolean strand_specific, frame_specific, show_rev_strand ;

	      new_type = zMapFeatureTypeCreate(name,
					       zMapConfigGetElementString(next_types, "outline"),
					       zMapConfigGetElementString(next_types, "foreground"),
					       zMapConfigGetElementString(next_types, "background"),
					       zMapConfigGetElementFloat(next_types, "width"),
					       zMapConfigGetElementInt(next_types, "minmag")) ;

	      zMapStyleSetStrandAttrs(new_type,
				      zMapConfigGetElementBool(next_types, "strand_specific"),
				      zMapConfigGetElementBool(next_types, "frame_specific"),
				      zMapConfigGetElementBool(next_types, "show_reverse")) ;

	      g_datalist_id_set_data(&types, new_type->unique_id, new_type) ;
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
  
  printf("\t%s: \t%f \t%s\n", style_name, style->width,
	 (style->show_rev_strand ? "show_rev_strand" : "!show_rev_strand")) ;

  return ;
}


