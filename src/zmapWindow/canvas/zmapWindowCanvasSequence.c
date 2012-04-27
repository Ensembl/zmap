/*  File: zmapWindowCanvasSequence.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can refeaturesetstribute it and/or
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
 * implements callback functions for FeaturesetItem Sequence features
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>





#include <math.h>
#include <string.h>
#include <ZMap/zmapFeature.h>
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasSequence_I.h>


/* font finding code derived from some by Gemma Barson */

/* Tries to return a fixed font from the list given in pref_families, returns
 * the font family name if it succeeded in finding a matching font, NULL otherwise.
 * The list of preferred fonts is treated with most preferred first and least
 * preferred last.  The function will attempt to return the most preferred font
 * it finds.
 *
 * @param widget         Needed to get a context, ideally should be the widget you want to
 *                       either set a font in or find out about a font for.
 * @param pref_families  List of font families (as text strings).
 * @return               font family name if font found, NULL otherwise.
 */
static const char *findFixedWidthFontFamily(PangoContext *context)
{
  /* Find all the font families available */
  PangoFontFamily **families;
  gint n_families;
  pango_context_list_families(context, &families, &n_families) ;

  /* Loop through the available font families looking for one in our preferred list */
  gboolean found_most_preferred = FALSE;
  gint most_preferred = 0x7fff;	/* we can't possibly have that many fonts in our preferred list!" */
  char * preferred_fonts[] = 		/* in order of preference */
  {
	"fixed",
	"Monospace",
	"Courier",
	"Courier new",
	"Courier 10 pitch",
	"Lucida console",
	"monaco",
	"Bitstream vera sans mono",
	"deja vu sans mono",
	"Lucida sans typewriter",
	"Andale mono",
//	"profont",
//	"proggy",
//	"droid sans mono",
//	"consolas",
//	"inconsolata",
	NULL
  };
  static const char *fixed_font = NULL;

  PangoFontFamily *match_family = NULL;
  gint family;

  if(!fixed_font)
  {
	for (family = 0 ; (family < n_families && !found_most_preferred) ; family++)
	{
		const gchar *name = pango_font_family_get_name(families[family]) ;
		gint current = 1;
		char **pref;
		/* Look for this font family in our list of preferred families */

		for(pref = preferred_fonts, current = 0; *pref; pref++, current++)
		{
			char *pref_font = *pref;

			if (g_ascii_strncasecmp(name, pref_font, strlen(pref_font)) == 0
#if GLIB_MAJOR_VERSION >= 1 && GLIB_MINOR_VERSION >= 4
				&& pango_font_family_is_monospace(families[family])
#endif
				)
			{
				/* We prefer ones nearer the start of the list */
				if(current <= most_preferred)
				{
					most_preferred = current;
					match_family = families[family];

					if(!most_preferred)
					found_most_preferred = TRUE;
				}

				break;
			}
		}
	}

	if (match_family)
		fixed_font = pango_font_family_get_name(match_family);
  }

  return fixed_font;
}





static void zmapWindowCanvasSequencePaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)
{
	/* calculate strings on demand ans run them through pango
	 * for the example code this drived from look at:
	 * http://developer.gnome.org/gdk/stable/gdk-Pango-Interaction.html
	 */

	FooCanvasItem *foo = (FooCanvasItem *) featureset;
	double y1, y2;
	ZMapSequence sequence = &feature->feature->feature.sequence;
	ZMapWindowCanvasSequence seq = (ZMapWindowCanvasSequence) feature;
	int cx,cy;
	long seq_y1, seq_y2, y;
	const char *font;

	/* lazy evaluation of pango renderer */
	if(!seq->pango_renderer)
	{
		GdkScreen *screen = gdk_drawable_get_screen (drawable);
		PangoFontDescription *desc;
		int width, height;
		GdkColor *draw;

		seq->pango_renderer = gdk_pango_renderer_new (screen);

		gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (seq->pango_renderer), drawable);

		zMapAssert(featureset->gc);	/* should have been set by caller */
		gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (seq->pango_renderer), featureset->gc);

		/* Create a PangoLayout, set the font and text */
		seq->pango_context = gdk_pango_context_get_for_screen (screen);
		seq->pango_layout = pango_layout_new (seq->pango_context);

		font = findFixedWidthFontFamily(seq->pango_context);
		desc = pango_font_description_from_string (ZMAP_ZOOM_FONT_FAMILY);
		pango_font_description_set_size (desc,ZMAP_ZOOM_FONT_SIZE * PANGO_SCALE);
		pango_layout_set_font_description (seq->pango_layout, desc);
		pango_font_description_free (desc);

		pango_layout_set_text (seq->pango_layout, "A", 1);		/* we need to get the size of one character */
		pango_layout_get_size (seq->pango_layout, &width, &height);
		seq->text_height = height / PANGO_SCALE;
		seq->text_width = width / PANGO_SCALE;

		zMapStyleGetColours(featureset->style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, NULL, &draw, NULL);

		gdk_pango_renderer_set_override_color (GDK_PANGO_RENDERER (seq->pango_renderer), PANGO_RENDER_PART_FOREGROUND, draw );

printf("text size: %d,%d\n",seq->text_height, seq->text_width);
	}


	if(!seq->row_size)			/* have just zoomed */
	{
		/* we need seq start and end to get the range, and the text height and the zoom level
		 * calculate how many bases we need to display per row -> gives us row_size
		 * if there are not enough for the last row then we only display what's there,
		 * and these can dangle off the end if there's not enough pixels vertically
		 * from max width of the column calculate if we can display all the bases and set a flag
		 * set the featureset width
		 */

		/* NOTE may need to move this into zoom and find the feature if we have to do a reposition
		 * or set the feature on add
		 */

		long n_bases = featureset->end - featureset->start + 1;
		long pixels = n_bases * featureset->zoom;
		int width = (int) zMapStyleGetWidth(featureset->style);	/* characters not pixels */
		int n_disp = width; 	// if pixels divide by seq->text_width;
		long n_rows = pixels / seq->text_height;

		seq->row_size = n_bases / n_rows;		/* zoom is pixels per unit y */

		if(seq->row_size <= n_disp)
		{
			seq->row_disp = seq->row_size;
			seq->n_bases  = seq->row_size;
			seq->truncated = "";
		}
		else
		{
			seq->row_disp = n_disp;
			seq->n_bases  = n_disp - 3;
			seq->truncated = "...";
		}
		featureset->width = n_disp * seq->text_width;
		/* do we need a reposition? */

printf("n_bases: %ld %ld, %d %d %ld = %ld\n",n_bases, pixels, width, n_disp, n_rows, seq->row_size);
	}

		/* allocate space for the text */
	if(seq->n_text <= seq->row_size)
	{
		if(seq->text)
			g_free(seq->text);
		seq->n_text = seq->row_size > MAX_SEQ_TEXT ? seq->row_size : MAX_SEQ_TEXT;	/* avoid reallocation, maybe we first do this zoomned in */
		seq->text = g_malloc(seq->n_text);
	}

	/* get the expose area: copied from calling code, we have one item here and it's normally bigger than the expose area */
	foo_canvas_c2w(foo->canvas,0,floor(expose->area.y - 1),NULL,&y1);
	foo_canvas_c2w(foo->canvas,0,ceil(expose->area.y + expose->area.height + 1),NULL,&y2);

	/* find and draw all the sequence data within */
	/* we act dumb and just display each row complete, let X do the clipping */

printf("paint from %.1f to %.1f\n",y1,y2);

	/* bias coordinates to stable ones */
	seq_y1 = y1;
	seq_y2 = y2;

	if(y1 < seq->start)
		y1 = seq->start;

	y = y1 - seq->start;
	y -= ((long) y) % seq->row_size;

	for(; y < y2; y += seq->row_size)
	{
		char *p,*q;
		int i;

		if(y < seq->start || y > y2)
			continue;

		p = sequence->sequence + (int) (y - seq->start);
		q = seq->text;
		for(i = 0;i < seq->n_bases; i++)
			*q++ = *p++;	// & 0x5f; original code did this lower cased, peptide are upper
		strcpy(q,seq->truncated);


		pango_layout_set_text (seq->pango_layout, seq->text, seq->row_disp);

		/* need to get pixel cooprdinates for pamgo */
		foo_canvas_w2c (foo->canvas, featureset->dx, y + featureset->dy, &cx, &cy);

printf("dna %s at %ld, canvas %.1f, %.1f = %d, %d \n",  seq->text, y, featureset->dx, y + featureset->dy, cx, cy);

if(seq->text[0] == 'c')		/* test underlay colour */
{
	GdkColor c;

	c.pixel = 0xff00;
	gdk_gc_set_foreground (featureset->gc, &c);

	gdk_draw_rectangle (drawable, featureset->gc, TRUE, cx, cy, seq->text_width * seq->row_disp, seq->text_height);
}

		pango_renderer_draw_layout (seq->pango_renderer, seq->pango_layout,  cx * PANGO_SCALE ,  cy * PANGO_SCALE);
	}
}



static void zmapWindowCanvasSequenceZoomSet(ZMapWindowFeaturesetItem featureset)
{
	ZMapWindowCanvasSequence seq;
	ZMapSkipList sl;

	/* calculate bases per line */
//	printf("sequence zoom (%.5f)\n", featureset->zoom);	/* is pixels per unit y */

	/* ummmm..... previous implemmentation is quite complex, let-s just do it from scratch */

	for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
	{
		seq = (ZMapWindowCanvasSequence) sl->data;

		seq->row_size = 0;	/* trigger recalc, can't do it here as we don't have the seq feature, or a drawable  */
	}
}


static void zmapWindowCanvasSequenceFreeSet(ZMapWindowFeaturesetItem featureset)
{
	ZMapWindowCanvasSequence seq;
	ZMapSkipList sl;
	GList *l;

	/* nominally there is only one feature in a seq column, but we could have 3 franes staggered.
	 * besides, this data is in the feature not the featureset so we have to iterate regardless
	 */
	for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
	{
		seq = (ZMapWindowCanvasSequence) sl->data;

		/* free any highlight list */

		for(l = seq->highlight; l ; l = l->next)
		{
			ZMapSequenceHighlight sh;

			sh = (ZMapSequenceHighlight) l->data;
			/* expose */

			g_free(sh);
		}
		g_list_free(seq->highlight);
		seq->highlight = NULL;


		if(seq->pango_renderer)		/* free the pango renderer if allocated */
		{
			/* Clean up renderer, possiby this is not necessary */
			gdk_pango_renderer_set_override_color (GDK_PANGO_RENDERER (seq->pango_renderer),
								PANGO_RENDER_PART_FOREGROUND, NULL);
			gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (seq->pango_renderer), NULL);
			gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (seq->pango_renderer), NULL);

			/* free other objects we created */
			if(seq->pango_layout)
			{
				g_object_unref(seq->pango_layout);
				seq->pango_layout = NULL;
			}

			if(seq->pango_context)
			{
				g_object_unref (seq->pango_context);
				seq->pango_context = NULL;
			}
			if(featureset->gc)	/* the featureset code does this but we do it here to have controlover the sequence of events */
			{
				g_object_unref(featureset->gc);
				featureset->gc = NULL;
			}

			g_object_unref(seq->pango_renderer);
			seq->pango_renderer = NULL;
		}
	}
}




#warning SetColour is dummy function only
static void zmapWindowCanvasSequenceSetColour(FooCanvasItem         *foo,
						      ZMapFeature			feature,
						      ZMapFeatureSubPartSpan sub_feature,
						      ZMapStyleColourType    colour_type,
							int colour_flags,
						      GdkColor              *default_fill,
                                          GdkColor              *default_border)
{
	ZMapWindowCanvasSequence seq = (ZMapWindowCanvasSequence) foo;

	/* based on colour_type:
	 * set highlight region from span if SELECTED
	 * set background if NORMAL
	 * remove highlight if INVALID
	 */

	/* NOTE this function always adds a highlight. To replace one, deselect the highlight first.
	 * We don't ever expect to want to change a highlight slightly.
	 */

	ZMapSequenceHighlight h;
	gulong colour_pixel;


	switch(colour_type)
	{
		case ZMAPSTYLE_COLOURTYPE_INVALID:
			zmapWindowCanvasSequenceFreeSet((ZMapWindowFeaturesetItem) seq);
			break;

		case ZMAPSTYLE_COLOURTYPE_NORMAL:
			colour_pixel = zMap_gdk_color_to_rgba(default_fill);
			colour_pixel = foo_canvas_get_color_pixel(foo->canvas, colour_pixel);

			seq->background = colour_pixel;
			/* expose column */
			break;
		case ZMAPSTYLE_COLOURTYPE_SELECTED:

			/* zmapStyleSetColour() according to sub_part->subpart */
			colour_pixel = zMap_gdk_color_to_rgba(default_fill);
			colour_pixel = foo_canvas_get_color_pixel(foo->canvas, colour_pixel);

			h = g_new0(zmapSequenceHighlightStruct, 1);
			h->colour = colour_pixel;
			h->start = sub_feature->start;
			h->end = sub_feature->end;

			seq->highlight = g_list_append(seq->highlight, h);

			/* expose span */
			break;
	}
}



static ZMapFeatureSubPartSpan zmapWindowCanvasSequenceGetSubPartSpan (FooCanvasItem *foo, ZMapFeature feature, double x,double y)
{
	static ZMapFeatureSubPartSpanStruct sub_part;

	/* the interface to this is via zMapWindowCanvasItemGetInterval(), so here we have to look up the feature again */
#warning revisit this when canvas items are simplified
	/* and then we find the transcript data in the feature context which has a list of exaond and introms */
	/* or we could find the exons/introns in the canvas and process those */

	sub_part.start = y;
	sub_part.end = y + 1;
	sub_part.index = 1;
	sub_part.subpart = ZMAPFEATURE_SUBPART_INVALID;

	return &sub_part;
}




void zMapWindowCanvasSequenceInit(void)
{
	gpointer funcs[FUNC_N_FUNC] = { NULL };

	funcs[FUNC_PAINT]   = zmapWindowCanvasSequencePaintFeature;
	funcs[FUNC_ZOOM]    = zmapWindowCanvasSequenceZoomSet;
	funcs[FUNC_COLOUR]  = zmapWindowCanvasSequenceSetColour;
	funcs[FUNC_FREE]    = zmapWindowCanvasSequenceFreeSet;
	funcs[FUNC_SUBPART] = zmapWindowCanvasSequenceGetSubPartSpan;

	zMapWindowCanvasFeatureSetSetFuncs(FEATURE_SEQUENCE, funcs, sizeof(zmapWindowCanvasSequenceStruct));
}

