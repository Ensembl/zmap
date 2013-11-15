/*  File: zmapWindowStyle.c
 *  Author: malciolm hinsley (mh17@sanger.ac.uk)
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Code implementing the menus for sequence display.
 *
 * Exported functions: ZMap/zmapWindows.h
 *
 *-------------------------------------------------------------------
 */

/*
called from zmapWindowMenu.c
initially allows style fill and border colours to be set
creating a child style if necessary
then applies the new style to the featureset
using the code in zmapWindowMenus.c

new file started as a) windowMenus.c is quite long already and
b) styles have a lot of other options we could set
*/


#include <ZMap/zmap.h>

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h> /* zMap_g_hash_table_nth */
#include <zmapWindow_P.h>
#include <zmapWindowCanvasItem.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowContainerFeatureSet_I.h>



typedef struct
{
	ItemMenuCBData menu_data;		/* which featureset etc */
	zmapFeatureTypeStyleStruct save;	/* copy of original used for Revert only */
	gboolean changed;				/* if we have applied any changes */
	gboolean override;			/* if we should make a child style */
	gboolean created;				/* if we made a child style */

	gboolean refresh;				/* clicked on another column */

	GtkWidget *toplevel ;
	GtkWidget *featureset_name;

	GtkWidget *fill_widget ;
	GtkWidget *border_widget ;
	char *fill_colour_str ;
	char *border_colour_str ;

	GtkWidget *cds_fill_widget ;
	GtkWidget *cds_border_widget ;
	char *cds_fill_colour_str ;
	char *cds_border_colour_str ;
	GtkWidget *cds;

	GtkWidget *stranded;


} StyleChangeStruct, *StyleChange;


static void destroyCB(GtkWidget *widget, gpointer cb_data);
static void applyCB(GtkWidget *widget, gpointer cb_data);
static void cancelCB(GtkWidget *widget, gpointer cb_data);
static void closeCB(GtkWidget *widget, gpointer cb_data);

static void colourSetCB(GtkColorButton *widget, gpointer user_data);
static void setParametersInStyle(StyleChange my_data, ZMapFeatureTypeStyle style);




void zmapWindowShowStyleDialog( ItemMenuCBData menu_data )
{
  StyleChange my_data = g_new0(StyleChangeStruct,1);
  /* theoretically we could have more than one dialog active. How confusing that would be */
  ZMapFeatureTypeStyle style;
  char *text;
  GtkWidget *toplevel, *top_vbox, *vbox, *hbox, *frame, *button, *label;
  GdkColor colour = {0} ;
  GdkColor *fill = &colour, *border = &colour;

  if(zmapWindowSetStyleFeatureset(menu_data->window, menu_data->item, menu_data->feature))
    return;

  my_data->menu_data = menu_data;
  style = menu_data->feature_set->style;
  memcpy(& my_data->save, style,sizeof (zmapFeatureTypeStyleStruct));

  /* when to override a style? (create a child)
   * - if it has not been overridden already
   * - if it does not have the same name as the featureset
   * - if it is a default style
   * NOTE for otterlace we do not edit styles that are not default/ spec'd in styles.ini
   * for standalone we want to be able to edit the styles file here
   * NOTE it's conceivable that we could have a default style and featureset of the same name
   * so we may have to patch this in applyCB()
   */
  if(style->is_default || (!style->overridden && style->unique_id != menu_data->feature_set->unique_id))
    my_data->override = TRUE;


  /* set up the top level window */
  my_data->toplevel = toplevel = zMapGUIToplevelNew(NULL, "Edit Style") ;

  g_signal_connect(GTK_OBJECT(toplevel), "destroy",
		   GTK_SIGNAL_FUNC(destroyCB), (gpointer)my_data) ;
  gtk_window_set_keep_above((GtkWindow *) toplevel,TRUE);
  gtk_container_set_focus_chain (GTK_CONTAINER(toplevel), NULL);

  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;
  gtk_window_set_default_size(GTK_WINDOW(toplevel), 500, -1) ;

  /* Add ptr so parent knows about us */
  menu_data->window->style_window = (gpointer) my_data ;


  top_vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), top_vbox) ;
  gtk_container_set_focus_chain (GTK_CONTAINER(top_vbox), NULL);

  //      menubar = makeMenuBar(my_data) ;
  //      gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);


  text = "Featureset " ;
  frame = gtk_frame_new(text) ;
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.5);
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(top_vbox), frame, TRUE, TRUE, 0) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_set_focus_chain (GTK_CONTAINER(hbox), NULL);
  gtk_container_add(GTK_CONTAINER(frame), hbox) ;

  my_data->featureset_name = label = gtk_label_new(g_quark_to_string(my_data->menu_data->feature_set->original_id)) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;


  /* Make colour buttons. */
  zMapStyleGetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill, NULL, &border);


  frame = gtk_frame_new("Set Colours:") ;
  gtk_container_set_border_width(GTK_CONTAINER(frame),
				 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);
  gtk_box_pack_start(GTK_BOX(top_vbox), frame, FALSE, FALSE, 0) ;

  vbox = gtk_vbox_new(FALSE, 0) ;		/* vbox for rows of colours */
  gtk_container_add(GTK_CONTAINER(frame), vbox) ;

  hbox = gtk_hbox_new(FALSE, 0) ;		/* hbox for fill / border */
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0) ;
  gtk_box_set_spacing(GTK_BOX(hbox), ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING) ;
  gtk_container_set_border_width(GTK_CONTAINER(hbox), ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  label = gtk_label_new("Fill:") ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;

  my_data->fill_widget = button = gtk_color_button_new() ;
  gtk_color_button_set_color(GTK_COLOR_BUTTON(button), fill) ;
  g_signal_connect(G_OBJECT(button), "color-set",
		   G_CALLBACK(colourSetCB), my_data) ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;

  label = gtk_label_new("Border:") ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;

  my_data->border_widget = button = gtk_color_button_new() ;
  gtk_color_button_set_color(GTK_COLOR_BUTTON(button), border) ;
  g_signal_connect(G_OBJECT(button), "color-set",
		   G_CALLBACK(colourSetCB), my_data) ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;

  if(style->mode == ZMAPSTYLE_MODE_TRANSCRIPT)	/* add CDS colours */
    {
      zMapStyleGetColours(style, STYLE_PROP_TRANSCRIPT_CDS_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill, NULL, &border);
    }

  /* must create these anyway */
  {
    my_data->cds = hbox = gtk_hbox_new(FALSE, 0) ;		/* hbox for fill / border */
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0) ;
    gtk_box_set_spacing(GTK_BOX(hbox), ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING) ;
    gtk_container_set_border_width(GTK_CONTAINER(hbox), ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

    label = gtk_label_new("CDS\nFill:") ;
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;

    my_data->cds_fill_widget = button = gtk_color_button_new() ;
    gtk_color_button_set_color(GTK_COLOR_BUTTON(button), fill) ;
    g_signal_connect(G_OBJECT(button), "color-set",
		     G_CALLBACK(colourSetCB), my_data) ;
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;

    label = gtk_label_new("CDS\nBorder:") ;
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;

    my_data->cds_border_widget = button = gtk_color_button_new() ;
    gtk_color_button_set_color(GTK_COLOR_BUTTON(button), border) ;
    g_signal_connect(G_OBJECT(button), "color-set",
		     G_CALLBACK(colourSetCB), my_data) ;
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  }

  if(style->mode == ZMAPSTYLE_MODE_TRANSCRIPT)	/* add CDS colours */
    {
      gtk_widget_hide_all(my_data->cds);
    }


  /* some major parameters */
  frame = gtk_frame_new("Layout:") ;
  gtk_container_set_border_width(GTK_CONTAINER(frame),
				 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);
  gtk_box_pack_start(GTK_BOX(top_vbox), frame, FALSE, FALSE, 0) ;

  vbox = gtk_vbox_new(FALSE, 0) ;		/* vbox for rows of colours */
  gtk_container_add(GTK_CONTAINER(frame), vbox) ;

  my_data->stranded = button = gtk_check_button_new_with_label("Stranded");
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  // if anything should be toggled
  //      g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(strandedCB), my_data);

#if 0
  /* if alignment... */
  button = gtk_check_button_new_with_label("Always Gapped");
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(strandedCB), my_data);
#endif

  /* Make control buttons along bottom of dialog. */
  frame = gtk_frame_new(NULL) ;
  gtk_container_set_border_width(GTK_CONTAINER(frame),
				 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);
  gtk_box_pack_start(GTK_BOX(top_vbox), frame, FALSE, FALSE, 0) ;


  hbox = gtk_hbutton_box_new();
  gtk_container_add(GTK_CONTAINER(frame), hbox);
  gtk_box_set_spacing(GTK_BOX(hbox), ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING);
  gtk_container_set_border_width(GTK_CONTAINER (hbox), ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(button), "clicked",
		   G_CALLBACK(closeCB), my_data);

  button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(button), "clicked",
		   G_CALLBACK(cancelCB), my_data);

  button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(button), "clicked",
		   G_CALLBACK(applyCB), my_data);

  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT) ; /* set apply button as default. */
  gtk_window_set_default(GTK_WINDOW(toplevel), button) ;

  gtk_widget_show_all(toplevel) ;

  zmapWindowSetStyleFeatureset(menu_data->window, menu_data->item, menu_data->feature);

  return ;
}


static void clear_style(StyleChange my_data)
{
	if(my_data->created)	/* just remove the child and replace w/ the parent */
	{
		GHashTable *styles = my_data->menu_data->window->context_map->styles;
		ZMapFeatureTypeStyle style = my_data->menu_data->feature_set->style;

		/* free the created style: cannot be ref'd anywhere else */
            g_hash_table_remove(styles,GUINT_TO_POINTER(my_data->menu_data->feature_set->style->unique_id));
		zMapStyleDestroy(style);

		my_data->created = FALSE;
		my_data->changed = FALSE;
	}
}


void zmapStyleWindowDestroy(ZMapWindow window)
{
	StyleChange my_data = (StyleChange) window->style_window;

	gtk_widget_destroy(my_data->toplevel);
	window->style_window = NULL;
}

gboolean zmapWindowSetStyleFeatureset(ZMapWindow window, FooCanvasItem *foo, ZMapFeature feature)
{
	StyleChange my_data = (StyleChange) window->style_window;
	ZMapFeatureSet set = (ZMapFeatureSet) feature->parent;
	ZMapFeatureTypeStyle style;
	GdkColor colour = {0} ;
	GdkColor *fill = &colour, *border = &colour;

	if(!my_data)
		return FALSE;

	my_data->menu_data->item = foo;
	my_data->menu_data->feature_set = set;
	my_data->menu_data->feature = feature;
	style = set->style;

	memcpy(& my_data->save, style,sizeof (zmapFeatureTypeStyleStruct));

//	clear_style(my_data);
	my_data->created = FALSE;	/* different style, override only created on apply */

	/* see function above for comment re override rules */
	my_data->override = FALSE;

	if(style->is_default || (!style->overridden && style->unique_id != my_data->menu_data->feature_set->unique_id))
		my_data->override = TRUE;

	gtk_label_set_text((GtkLabel *) my_data->featureset_name, g_quark_to_string(set->original_id));

	/* Update the colour buttons. */
	zMapStyleGetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill, NULL, &border);

      gtk_color_button_set_color(GTK_COLOR_BUTTON(my_data->fill_widget), fill) ;
      gtk_color_button_set_color(GTK_COLOR_BUTTON(my_data->border_widget), border) ;

	if(style->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
	{
		zMapStyleGetColours(style, STYLE_PROP_TRANSCRIPT_CDS_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill, NULL, &border);

		gtk_color_button_set_color(GTK_COLOR_BUTTON(my_data->cds_fill_widget), fill) ;
		gtk_color_button_set_color(GTK_COLOR_BUTTON(my_data->cds_border_widget), border) ;
		gtk_widget_show_all(my_data->cds);
	}
	else
	{
		gtk_widget_hide_all(my_data->cds);
	}

	gtk_toggle_button_set_active((GtkToggleButton *) my_data->stranded, style->strand_specific);

	return TRUE;
}



static void cancelCB(GtkWidget *widget, gpointer cb_data)
{
	StyleChange my_data = (StyleChange) cb_data;
	ItemMenuCBData menu_data = my_data->menu_data;
	ZMapFeatureTypeStyle style = NULL;
	GQuark id;

	if(!my_data->changed)
		return;

	if(my_data->created)	/* just remove the child and replace w/ the parent */
	{
		GHashTable *styles = my_data->menu_data->window->context_map->styles;
		style = my_data->menu_data->feature_set->style;
		id = style->parent_id;

		clear_style(my_data);		/* free the created style: cannot be ref'd anywhere else */

		/* find the parent */
		style = g_hash_table_lookup(styles, GUINT_TO_POINTER(id));
	}
	else				/* restore the existing style */
	{
		style = my_data->menu_data->feature_set->style;
		memcpy((void *) style, (void *) &my_data->save, sizeof (zmapFeatureTypeStyleStruct));
	}

	my_data->changed = FALSE;

	/* update the column */
	zmapWindowMenuSetStyleCB(style->unique_id, my_data->menu_data);

	/* update this dialog */
	zmapWindowSetStyleFeatureset(menu_data->window, menu_data->item, menu_data->feature);
}


static void applyCB(GtkWidget *widget, gpointer cb_data)
{
	StyleChange my_data = (StyleChange) cb_data;
	ZMapFeatureSet feature_set = my_data->menu_data->feature_set;
	ZMapFeatureTypeStyle style = feature_set->style;
	GHashTable *styles = my_data->menu_data->window->context_map->styles;
	char *name, *description;

		/* make a new child style? */
	if(my_data->override && !my_data->created)
	{
		ZMapFeatureTypeStyle parent = style;
		ZMapFeatureTypeStyle tmp_style;

		/* make new style with same name as the featureset and merge in the parent */
		name = (char *) g_quark_to_string(feature_set->original_id);
		name = g_strdup_printf("X_%s",name);

		if(style->unique_id == feature_set->unique_id)
		{
			name = g_strdup_printf("%s_",name);
		}
		description = name;

		style = zMapStyleCreate(name, description);

		/* merge is written upside down
		 * we have to copy the parent and merge the child onto it
		 * then delete the style we just created
		 */

		tmp_style = zMapFeatureStyleCopy(parent) ;

		if (zMapStyleMerge(tmp_style, style))
            {
			g_hash_table_insert(styles,GUINT_TO_POINTER(style->unique_id),tmp_style);
			zMapStyleDestroy(style);
			style = tmp_style;

			g_object_set(G_OBJECT(style), ZMAPSTYLE_PROPERTY_PARENT_STYLE, parent->unique_id , NULL);
            }
            else
		{
			zMapWarning("Cannot create new style %s",name);
			zMapStyleDestroy(tmp_style);
			zMapStyleDestroy(style);
			return;
		}

            my_data->created = TRUE;
	}

		/* apply the chosen colours etc */
	setParametersInStyle(my_data, style);

	/* set the style
	 * if we are only changing colours we could be more efficient
	 * but this will work for all cases and means no need to write more code
	 * we do this _once_ at user/ mouse click speed
	 */
	zmapWindowMenuSetStyleCB(style->unique_id, my_data->menu_data);

	my_data->changed = TRUE;
}




static void closeCB(GtkWidget *widget, gpointer cb_data)
{
	StyleChange my_data = (StyleChange) cb_data ;

	gtk_widget_destroy(my_data->toplevel);
}



/* Slightly obscure GTK interface...this function is called when the user selects a colour
 * from the colour chooser dialog displayed when they click on the colour button,
 * i.e. it is _not_ a callback for the button itself. */
static void colourSetCB(GtkColorButton *widget, gpointer user_data)
{
  StyleChange my_data = (StyleChange) user_data ;
  GdkColor colour = {0} ;
  char **target_string ;

	/* if you add more of these think about using an array or hash */
  if (GTK_WIDGET(widget) == my_data->fill_widget)
	target_string = &(my_data->fill_colour_str) ;
  else if (GTK_WIDGET(widget) == my_data->border_widget)
	target_string = &(my_data->border_colour_str) ;
  else if (GTK_WIDGET(widget) == my_data->cds_fill_widget)
	target_string = &(my_data->fill_colour_str) ;
  else if (GTK_WIDGET(widget) == my_data->cds_border_widget)
	target_string = &(my_data->cds_border_colour_str) ;
  else
	  return;

  if (*target_string)
    g_free(*target_string) ;

  gtk_color_button_get_color(widget, &colour) ;

  *target_string = gdk_color_to_string(&colour) ;
}




static void destroyCB(GtkWidget *widget, gpointer cb_data)
{
  StyleChange my_data = (StyleChange) cb_data;

  my_data->menu_data->window->style_window = NULL;

  g_free(my_data->menu_data);
  g_free(my_data);
}



static void setParametersInStyle(StyleChange my_data, ZMapFeatureTypeStyle style)
{
  char *colour_spec;

  colour_spec = zMapStyleMakeColourString(my_data->fill_colour_str, "black", my_data->border_colour_str,
						  my_data->fill_colour_str, "black", my_data->border_colour_str) ;

  g_object_set(G_OBJECT(style), ZMAPSTYLE_PROPERTY_COLOURS, colour_spec,
			ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC, gtk_toggle_button_get_active((GtkToggleButton *) my_data->stranded),
			NULL);

  if(style->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
  {
	colour_spec = zMapStyleMakeColourString(my_data->cds_fill_colour_str, "black", my_data->cds_border_colour_str,
						  my_data->cds_fill_colour_str, "black", my_data->cds_border_colour_str) ;

	g_object_set(G_OBJECT(style), ZMAPSTYLE_PROPERTY_TRANSCRIPT_CDS_COLOURS, colour_spec, NULL);

  }

}





void zmapWindowMenuSetStyleCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeatureSet feature_set = menu_data->feature_set;
  ZMapFeatureTypeStyle style;
  FooCanvasItem *set_item, *canvas_item;
  ZMapStrand set_strand;
  ZMapFrame set_frame;
  ID2Canvas id2c;
  int ok = FALSE;


  style = g_hash_table_lookup( menu_data->context_map->styles, GUINT_TO_POINTER(menu_item_id));
  if(style)
  {
	  ZMapFeatureColumn column;
	  ZMapFeatureSource s2s;
	  ZMapFeatureSetDesc f2c;
	  GList *c2s;

	  /* now tweak featureset & column styles in various places */
	  s2s = g_hash_table_lookup(menu_data->context_map->source_2_sourcedata,GUINT_TO_POINTER(feature_set->unique_id));
	  f2c = g_hash_table_lookup(menu_data->context_map->featureset_2_column,GUINT_TO_POINTER(feature_set->unique_id));
	  if(s2s && f2c)
	  {
		s2s->style_id = style->unique_id;
		column = g_hash_table_lookup(menu_data->context_map->columns,GUINT_TO_POINTER(f2c->column_id));
		if(column)
		{

			c2s = g_hash_table_lookup(menu_data->context_map->column_2_styles,GUINT_TO_POINTER(f2c->column_id));
			if(c2s)
			{
				g_list_free(column->style_table);
				column->style_table = NULL;
				column->style = NULL;	/* must clear this to trigger style table calculation */
				/* NOTE column->style_id is column specific not related to a featureset, set by config */

				for( ; c2s; c2s = c2s->next)
				{
					if(GPOINTER_TO_UINT(c2s->data) == feature_set->style->unique_id)
					{
						c2s->data = GUINT_TO_POINTER(style->unique_id);
						break;
					}
				}
				ok = TRUE;
			}

			zMapWindowGetSetColumnStyle(menu_data->window, feature_set->unique_id);
		}
	  }
  }

  if(!ok)
  {
	  zMapWarning("cannot set new style","");
	  return;
  }


  /* get current style strand and frame status and operate on 1 or more columns
   * NOTE that the FToIhash has diff hash tables per strand and frame
   * for each one remove the FtoIhash and remove the set from the column
   * if the column is empty the destroy it
   * actaull it's easier just to cycle round all the possible strand and frame combos
   * 3 hash table lookups for each, 8x
   *
   * then set the new style and redisplay the featureset
   * strand and frame will be handled by the display code
   */

	  /* yes really: reverse is bigger than forwards despite appearing on the left */
  for(set_strand = ZMAPSTRAND_NONE; set_strand <= ZMAPSTRAND_REVERSE; set_strand++)
  {
	  /* yes really: frames are numbered 0,1,2 and have the values 1,2,3 */
	for(set_frame = ZMAPSTRAND_NONE; set_frame <= ZMAPFRAME_2; set_frame++)
	{
		/* this is really frustrating:
		 * every operation of the ftoi hash involves
		 * repeating the same nested hash table lookups
		 */

		/* does the set appear in a column ? */
		/* set item is a ContainerFeatureset */
		set_item = zmapWindowFToIFindSetItem(menu_data->window,
					menu_data->window->context_to_item,
     					menu_data->feature_set, set_strand, set_frame);
		if(!set_item)
			continue;

		/* find the canvas item (CanvasFeatureset) containing a feature in this set */
		id2c = zmapWindowFToIFindID2CFull(menu_data->window, menu_data->window->context_to_item,
				    feature_set->parent->parent->unique_id,
				    feature_set->parent->unique_id,
				    feature_set->unique_id,
				    set_strand, set_frame,0);

		canvas_item = NULL;
		if(id2c)
		{
			ID2Canvas feat = zMap_g_hash_table_nth(id2c->hash_table,0);
			if(feat)
				canvas_item = feat->item;
		}


		/* look it up again to delete it :-( */
		zmapWindowFToIRemoveSet(menu_data->window->context_to_item,
				    feature_set->parent->parent->unique_id,
				    feature_set->parent->unique_id,
				    feature_set->unique_id,
				    set_strand, set_frame, TRUE);


		if(canvas_item)
		{
			FooCanvasGroup *group = (FooCanvasGroup *) set_item;

			/* remove this featureset from the CanvasFeatureset */
			/* if it's empty it will perform hari kiri */
			if(ZMAP_IS_WINDOW_FEATURESET_ITEM(canvas_item))
			{
				zMapWindowFeaturesetItemRemoveSet(canvas_item, feature_set, TRUE);
			}

			/* destroy set item if empty ? */
			if(!group->item_list)
				zmapWindowContainerGroupDestroy((ZMapWindowContainerGroup) set_item);
		}

		zmapWindowRemoveIfEmptyCol((FooCanvasGroup **) &set_item) ;

	}
  }


  feature_set->style = style;

  zmapWindowRedrawFeatureSet(menu_data->window, feature_set);	/* does a complex context thing */

  zmapWindowColOrderColumns(menu_data->window) ;	/* put this column (deleted then created) back into the right place */
  zmapWindowFullReposition(menu_data->window->feature_root_group,TRUE, "window style") ;		/* adjust sizing and shuffle left / right */
}

