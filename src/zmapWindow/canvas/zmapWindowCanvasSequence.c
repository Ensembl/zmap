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



#if 0
/* font finding code derived from some by Gemma Barson */
/* erm... there is an almost identical fucntion in zmapGUIUtils.c which we should use */

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
	  ZMAP_ZOOM_FONT_FAMILY,
//	"fixed",
//	"Monospace",
//	"Courier",
//	"Courier new",
//	"Courier 10 pitch",
//	"Lucida console",
//	"monaco",
//	"Bitstream vera sans mono",
//	"deja vu sans mono",
//	"Lucida sans typewriter",
//	"Andale mono",

// Gemm has these commented out
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

#endif


static void zmapWindowCanvasSequenceGetPango(GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasSequence seq)
{
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

//		font = findFixedWidthFontFamily(seq->pango_context);
		desc = pango_font_description_from_string (ZMAP_ZOOM_FONT_FAMILY);
#warning mod this function and call from both places
#if 0
		/* this must be identical to the one get by the ZoomControl */
		if(zMapGUIGetFixedWidthFont(view,
				fixed_font_list, ZMAP_ZOOM_FONT_SIZE, PANGO_WEIGHT_NORMAL,
				NULL,desc))
		{
		}
		else
		{
			/* if this fails then we get a proportional font and someone will notice */
			zmapLogWarning("Paint sequence cannot get fixed font","");
		}
#endif

		pango_font_description_set_size (desc,ZMAP_ZOOM_FONT_SIZE * PANGO_SCALE);
		pango_layout_set_font_description (seq->pango_layout, desc);
		pango_font_description_free (desc);

		pango_layout_set_text (seq->pango_layout, "a", 1);		/* we need to get the size of one character */
		pango_layout_get_size (seq->pango_layout, &width, &height);
		seq->text_height = height / PANGO_SCALE;
		seq->text_width = width / PANGO_SCALE;

		seq->feature.width = zMapStyleGetWidth(featureset->style)  * width;

		zMapStyleGetColours(featureset->style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, NULL, &draw, NULL);

		gdk_pango_renderer_set_override_color (GDK_PANGO_RENDERER (seq->pango_renderer), PANGO_RENDER_PART_FOREGROUND, draw );

//printf("text size: %d,%d\n",seq->text_height, seq->text_width);
	}
}


static void zmapWindowCanvasSequenceSetZoom(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasSequence seq)
{

	if(!seq->row_size)			/* have just zoomed */
	{
		seq->start = featureset->start;
		seq->end = featureset->end;

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

		/* integer overflow? 20MB x 20pixels = 400M px which is ok in a long */
		long n_bases = seq->end - seq->start + 1;
		long pixels = n_bases * featureset->zoom + seq->text_height - 1;	/* zoom is pixels per unit y; + fiddle factor for integer divide */
		int width = (int) zMapStyleGetWidth(featureset->style);	/* characters not pixels */
		int n_disp = width; 		// if pixels, divide by seq->text_width;
		long n_rows;


		seq->spacing = seq->text_height + ZMAP_WINDOW_TEXT_BORDER;	/* NB: BORDER is 0 */
		n_rows = pixels / seq->spacing;
		seq->row_size = n_bases / n_rows;
		if(n_bases % n_rows)	/* avoid dangling bases at the end */
			seq->row_size++;

		n_rows = n_bases / seq->row_size;
		seq->spacing = pixels / n_rows;

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

//printf("n_bases: %ld %.5f %ld, %d %d %ld = %ld %ld\n",n_bases, featureset->zoom, pixels, width, n_disp, n_rows, seq->row_size, seq->spacing);
		/* allocate space for the text */
		if(seq->n_text <= seq->row_size)
		{
			if(seq->text)
				g_free(seq->text);
			seq->n_text = seq->row_size > MAX_SEQ_TEXT ? seq->row_size : MAX_SEQ_TEXT;	/* avoid reallocation, maybe we first do this zoomned in */
			seq->text = g_malloc(seq->n_text);
		}
	}
}


/* paint highlights for one line of text */
static GList *zmapWindowCanvasSequencePaintHighlight( GdkDrawable *drawable, ZMapWindowFeaturesetItem featureset,
									ZMapWindowCanvasSequence seq, GList *highlight, long y, int cx, int cy)
{
	ZMapSequenceHighlight sh;
	GdkColor c;
	long start,end;
	int n_bases;
	gboolean join_left = FALSE;
	int hcx;

	while(highlight)
	{
		/* each highlight struct has start and end seq coords and a colour */
		/* there may be more than one on a line eg w/ split codon */

		sh = (ZMapSequenceHighlight) highlight->data;
printf("highlight %ld (%ld) @ %ld, %ld\n",y, seq->row_size, sh->start, sh->end);

		if(sh->end < y)
		{
			highlight = highlight->next;	/* this highlight is upstream of this line */
			continue;
		}
		else
		{
			if(sh->start >= y + seq->row_size)	/* this highlight is downstream of this line */
				break;
		}

		/* this highlight is on this line */

		c.pixel = sh->colour;
		gdk_gc_set_foreground (featureset->gc, &c);

		/* paint bases actually visible */
		start = sh->start;
		end = sh->end;
		if(start < y)
			start = y;

		if(start < y + seq->n_bases)
		{
			if(end >= y + seq->n_bases)
			{
				end  = y + seq->n_bases - 1;
				join_left = TRUE;
			}

			n_bases = end - start + 1;
			hcx = cx + (start - y) * seq->text_width;

printf("paint sel @ %ld %d, %d (%d)\n", start, cy, hcx, n_bases);

			gdk_draw_rectangle (drawable, featureset->gc, TRUE, hcx, cy, seq->text_width * n_bases, seq->spacing + 1);
		}

		if(sh->end > y + seq->n_bases)
		{
			/* paint bases in the ... */

			cx += seq->n_bases * seq->text_width;
			n_bases = strlen(seq->truncated);
			if(!join_left)
			{
				n_bases--;
				cx += seq->text_width;
			}
			if(sh->end < y + seq->row_size)
			{
				n_bases--;
			}
			/* NOTE there are some combinations of highlight we will not show in the ... */
//printf("paint ... @ %ld %d, %d (%d)\n", y, cy, cx, n_bases);

			gdk_draw_rectangle (drawable, featureset->gc, TRUE, cx, cy, seq->text_width * n_bases, seq->spacing + 1);
		}

		if(sh->end >= y + seq->row_size)	/* there can be no more on this line */
			return highlight;

		highlight = highlight->next;	/* try the next one */
	}


	return highlight;
}


static void zmapWindowCanvasSequencePaintFeature(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature, GdkDrawable *drawable, GdkEventExpose *expose)
{
	/* calculate strings on demand and run them through pango
	 * for the example code this derived from look at:
	 * http://developer.gnome.org/gdk/stable/gdk-Pango-Interaction.html
	 */

	FooCanvasItem *foo = (FooCanvasItem *) featureset;
	double y1, y2;
	ZMapSequence sequence = &feature->feature->feature.sequence;
	ZMapWindowCanvasSequence seq = (ZMapWindowCanvasSequence) feature;
	int cx,cy;
	long seq_y1, seq_y2, y;
	int nb;
	GList *hl;	/* iterator through highlight */

	zmapWindowCanvasSequenceGetPango(drawable, featureset, seq);
	zmapWindowCanvasSequenceSetZoom(featureset, seq);

	/* get the expose area: copied from calling code, we have one item here and it's normally bigger than the expose area */
	foo_canvas_c2w(foo->canvas,0,floor(expose->area.y - 1),NULL,&y1);
	foo_canvas_c2w(foo->canvas,0,ceil(expose->area.y + expose->area.height + 1),NULL,&y2);

//printf("paint from %.1f to %.1f\n",y1,y2);
//	NOTE sort highlight list here if it's not added in ascending coord order */

	/* find and draw all the sequence data within */
	/* we act dumb and just display each row complete, let X do the clipping */

	/* bias coordinates to stable ones , find the first seq coord in this row */
	seq_y1 = (long) y1 - featureset->dy + 1;
	seq_y2 = (long) y2 - featureset->dy + 1;

	if(seq_y1 < 0)
		seq_y1 = 0;

	y = seq_y1;
	y -= 1;
	y -=  y % seq->row_size;				/* sequence offset from start, 0 based */

	seq->offset = (seq->spacing - seq->text_height) / 2;		/* vertical padding if relevant */

	for(hl = seq->highlight; y < seq_y2 && y < sequence->length; y += seq->row_size)
	{
		char *p,*q;
		int i;

//printf("y,off, seq: %ld %ld (%ld %ld) start,end %ld %ld\n",y, off, seq_y1,seq_y2,seq->start, seq->end);

		p = sequence->sequence + y;
		q = seq->text;

		nb = seq->n_bases;
		if(sequence->length - y < nb)
			nb = sequence->length - y;

		for(i = 0;i < nb; i++)
			*q++ = *p++;	// & 0x5f; original code did this lower cased, peptides are upper

		strcpy(q,seq->truncated);	/* may just be a null */
		while (*q)
			q++;

		pango_layout_set_text (seq->pango_layout, seq->text, q - seq->text);	/* be careful of last short row */
			/* need to get pixel coordinates for pango */
		foo_canvas_w2c (foo->canvas, featureset->dx, y + featureset->dy, &cx, &cy);

		/* NOTE y is 0 base so we have to add 1 to do the highlight properly */
		hl = zmapWindowCanvasSequencePaintHighlight(drawable, featureset, seq, hl, y + 1, cx, cy);

printf("paint dna %s @ %ld = %d (%ld)\n",seq->text, y, cy, seq->offset);
		pango_renderer_draw_layout (seq->pango_renderer, seq->pango_layout,  cx * PANGO_SCALE ,  (cy + seq->offset) * PANGO_SCALE);

//printf("dna %s at %ld, canvas %.1f, %.1f = %d, %d \n",  seq->text, y, featureset->dx, y + featureset->dy, cx, cy);
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


static void zmapWindowCanvasSequenceFreeHighlight(ZMapWindowCanvasSequence seq)
{
	GList *l;

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
}


static void zmapWindowCanvasSequenceFreePango(ZMapWindowCanvasSequence seq)
{
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

		g_object_unref(seq->pango_renderer);
		seq->pango_renderer = NULL;
	}

}

static void zmapWindowCanvasSequenceFreeSet(ZMapWindowFeaturesetItem featureset)
{
	ZMapWindowCanvasSequence seq;
	ZMapSkipList sl;

	/* nominally there is only one feature in a seq column, but we could have 3 franes staggered.
	 * besides, this data is in the feature not the featureset so we have to iterate regardless
	 */
	for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
	{
		seq = (ZMapWindowCanvasSequence) sl->data;

		zmapWindowCanvasSequenceFreeHighlight(seq);
		zmapWindowCanvasSequenceFreePango(seq);
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
	ZMapWindowCanvasSequence seq;
	ZMapWindowFeaturesetItem fi = (ZMapWindowFeaturesetItem) foo;

	/* based on colour_type:
	 * set highlight region from span if SELECTED
	 * set background if NORMAL
	 * remove highlight if INVALID
	 */

	/* NOTE this function always adds a highlight. To replace one, deselect the highlight first.
	 * We don't ever expect to want to change a highlight slightly.
	 * NOTE must add these in seq coord order, if not must sort before paint
	 */

	ZMapSequenceHighlight h;
	gulong colour_pixel;

	ZMapSkipList sl;

	/* nominally there is only one feature in a seq column, but we could have 3 franes staggered.
	 * besides, this data is in the feature not the featureset so we have to iterate regardless
	 */
	for(sl = zMapSkipListFirst(fi->display_index); sl; sl = sl->next)
	{
		seq = (ZMapWindowCanvasSequence) sl->data;

		switch(colour_type)
		{
			case ZMAPSTYLE_COLOURTYPE_INVALID:
				zmapWindowCanvasSequenceFreeHighlight(seq);
				break;

			case ZMAPSTYLE_COLOURTYPE_NORMAL:
				colour_pixel = zMap_gdk_color_to_rgba(default_fill);
				colour_pixel = foo_canvas_get_color_pixel(foo->canvas, colour_pixel);

				seq->background = colour_pixel;
				break;

			case ZMAPSTYLE_COLOURTYPE_SELECTED:
				zMapAssert(sub_feature);

				/* zmapStyleSetColour() according to sub_part->subpart */
				colour_pixel = zMap_gdk_color_to_rgba(default_fill);
				colour_pixel = foo_canvas_get_color_pixel(foo->canvas, colour_pixel);

				h = g_new0(zmapSequenceHighlightStruct, 1);
				h->colour = colour_pixel;
				/* this must match the sequence coords calculated for painting text */
				/* which are zero based relative to seq start coord and featureset offset (dy) */
				h->start = sub_feature->start - fi->dy + 1;
				h->end = sub_feature->end - fi->dy + 1;
printf("set highlight for %d,%d @ %ld, %ld\n",sub_feature->start, sub_feature->end, h->start,h->end);
				seq->highlight = g_list_append(seq->highlight, h);
				break;
		}
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

