/*  File: zmapControlWindowInfoPanel.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 * Description: Makes the information panel at the top of a zmap which
 *              shows details of features that the user selects.
 *
 * Exported functions: See zmapControl_P.h
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmap.hpp>
#include <ZMap/zmapSOParser.hpp>

#include <zmapControl_P.hpp>


/* Keep in step with number of feature details widgets. NOTE that _not_ every text field in
 * the feature description is displayed in a label. */
enum
  {
    INFO_PANEL_LABELS = 10
  } ;


/* Used for naming the info panel widgets so we can set their background colour with a style. */
#define PANEL_WIDG_STYLE_NAME "zmap-control-infopanel"


/*
 * Make the info. panel.
 *
 * (sm23) This function doesn't use the zmap argument, so this could be removed.
 * As far as I can see, it's only called in zmapControlViews.c, however I will leave
 * this for the time being.
 */
GtkWidget *zmapControlWindowMakeInfoPanel(ZMap zmap, ZMapInfoPanelLabels labels)
{
  GtkWidget *hbox = NULL,
   *frame,
    *event_box,
    **label[INFO_PANEL_LABELS] = {NULL} ;
  int i ;

  zMapReturnValIfFail(labels, hbox) ;

  label[0] = &(labels->feature_name) ;
  label[1] = &(labels->feature_strand) ;
  label[2] = &(labels->feature_coords) ;
  label[3] = &(labels->sub_feature_coords) ;
  label[4] = &(labels->feature_frame) ;
  label[5] = &(labels->feature_population) ;
  label[6] = &(labels->feature_score) ;
  label[7] = &(labels->feature_type) ;
  label[8] = &(labels->feature_set) ;
  label[9] = &(labels->feature_source) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 5);

  /* Note that label widgets do not have windows so in order for them to have tooltips and
   * do colouring via a widget name/associated style we must enclose them in an event box...sigh..... */
  for (i = 0 ; i < INFO_PANEL_LABELS ; i++)
    {
      frame = gtk_frame_new(NULL) ;
      gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0) ;

      event_box = gtk_event_box_new() ;
      gtk_widget_set_name(event_box, PANEL_WIDG_STYLE_NAME) ;
      gtk_container_add(GTK_CONTAINER(frame), event_box) ;

      *(label[i]) = gtk_label_new(NULL) ;
      gtk_label_set_selectable(GTK_LABEL(*(label[i])), TRUE);

      gtk_container_add(GTK_CONTAINER(event_box), *(label[i])) ;
    }


  return hbox ;
}


/* Add text/tooltips to info panel labels. If feature_desc == NULL then info panel
 * is reset.
 *
 * Current panel for a feature is:
 *
 *  name [strand] [start end] [subpart_start subpart_end] [frame] [score/percent_id] [type] [feature_set] [feature_style]
 *
 * or for a column is:
 *
 *                                     column name
 *
 * THE APPROACH HERE IS NOT CORRECT, SHOULD BE REWRITTEN TO KNOW NOTHING ABOUT FEATURES
 * AND SIMPLY DISPLAY IN ORDER A NUMBER OF FIELDS WITH TOOLTIP TEXT PROVIDED BY THE
 * CALLER, THIS SHOULD BE A DUMB DISPLAY FACILITY.
 *
 */
void zmapControlInfoPanelSetText(ZMap zmap, ZMapInfoPanelLabels labels, ZMapFeatureDesc feature_desc)
{
  static const int text_length_max = 20 ;
  static const char *format_name_max = "%s %s" ;
  GtkWidget *label[INFO_PANEL_LABELS] = {NULL} ;
  char *text[INFO_PANEL_LABELS] = {NULL} ;
  char *tooltip[INFO_PANEL_LABELS] = {NULL} ;
  char *temp_string = NULL, *temp_string_part = NULL ;
  int i = 0, name_length = 0 ;
  GString *desc_str = NULL ;

  zMapReturnIfFail(zmap && labels) ;

  label[0] = labels->feature_name ;
  label[1] = labels->feature_strand ;
  label[2] = labels->feature_coords ;
  label[3] = labels->sub_feature_coords ;
  label[4] = labels->feature_frame ;
  label[5] = labels->feature_population ;
  label[6] = labels->feature_score ;
  label[7] = labels->feature_type ;
  label[8] = labels->feature_set ;
  label[9] = labels->feature_source ;


  /* If no feature description then blank the info panel. */
  if (!feature_desc)
    {
      for (i = 0 ; i < INFO_PANEL_LABELS ; i++)
        {
          if (i == 0)
            text[i] = g_strdup("") ;    /* placeholder stops panel disappearing. */
          else
            text[i] = tooltip[i] = NULL ;
        }
    }
  else
    {
      if (feature_desc->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
        {
          text[0] = g_strdup(feature_desc->feature_set) ;
          if (feature_desc->feature_set_description)
            {
              tooltip[0] = g_strdup_printf("Description  -  \"%s\"",
                feature_desc->feature_set_description) ;
            }
          else
            tooltip[0] = g_strdup_printf("Description  -  \"%s\"",text[0]);
        }
      else
        {
          /*
           * Name label; if it's longer than a certain number of characters then
           * what goes to the display should be truncated.
           */
          if (feature_desc->feature_name)
            {
              name_length = strlen(feature_desc->feature_name) ;
              if (name_length > text_length_max)
                {
                  temp_string_part = g_strndup(feature_desc->feature_name, text_length_max) ;
                  temp_string = g_strdup_printf(format_name_max, temp_string_part, "[...]") ;
                }
              else
                {
                  temp_string = g_strdup(feature_desc->feature_name) ;
                }
              text[0] = g_strdup_printf("%s%s%s%s%s%s%s",
                                        temp_string,
                                        (feature_desc->feature_known_name ? "  (" : ""),
                                        (feature_desc->feature_known_name ? feature_desc->feature_known_name : ""),
                                        (feature_desc->feature_known_name ? ")" : ""),
                                        (feature_desc->feature_total_length ? "  (" : ""),
                                        (feature_desc->feature_total_length ? feature_desc->feature_total_length : ""),
                                        (feature_desc->feature_total_length ? ")" : "")) ;
              if (temp_string)
                {
                  g_free(temp_string) ;
                  temp_string = NULL ;
                }
              if (temp_string_part)
                {
                  g_free(temp_string_part) ;
                  temp_string_part = NULL ;
                }
            }

          /*
           * Strand label
           */
          if (feature_desc->feature_strand)
            {
              if (feature_desc->type == ZMAPSTYLE_MODE_ALIGNMENT && feature_desc->feature_query_strand)
                text[1] = g_strdup_printf("%s / %s", feature_desc->feature_strand, feature_desc->feature_query_strand) ;
              else
                text[1] = g_strdup_printf("%s", feature_desc->feature_strand) ;
            }

          /*
           * Feature start and end data.
           */
          if (feature_desc->feature_start)
            {
              if (!(feature_desc->feature_query_start))
                text[2] = g_strdup_printf("%s, %s%s%s%s",
                                          feature_desc->feature_start,
                                          feature_desc->feature_end,
                                          (feature_desc->feature_length ? "  (" : ""),
                                          (feature_desc->feature_length ? feature_desc->feature_length : ""),
                                          (feature_desc->feature_length ? ")" : "")) ;
              else
                text[2] = g_strdup_printf("%s, %s%s%s%s%s%s%s%s",
                                          feature_desc->feature_start,
                                          feature_desc->feature_end,
                                          (feature_desc->feature_query_start ? "  <-  " : ""),
                                          (feature_desc->feature_query_start ? feature_desc->feature_query_start : ""),
                                          (feature_desc->feature_query_start ? ", " : ""),
                                          (feature_desc->feature_query_end ? feature_desc->feature_query_end : ""),
                                          (feature_desc->feature_query_length ? "  (" : ""),
                                          (feature_desc->feature_query_length ? feature_desc->feature_query_length : ""),
                                          (feature_desc->feature_query_length ? ")" : "")) ;
            }

          /*
           * Subpart start and end data
           */
          if (feature_desc->sub_feature_start)
            text[3] = g_strdup_printf("%s %s: %s, %s%s%s%s%s%s%s%s",
                                      feature_desc->sub_feature_term,
                                      feature_desc->sub_feature_index,
                                      feature_desc->sub_feature_start,
                                      feature_desc->sub_feature_end,
                                      (feature_desc->sub_feature_query_start ? "  <-  " : ""),
                                      (feature_desc->sub_feature_query_start ? feature_desc->sub_feature_query_start : ""),
                                      (feature_desc->sub_feature_query_start ? ", " : ""),
                                      (feature_desc->sub_feature_query_end ? feature_desc->sub_feature_query_end : ""),
                                      (feature_desc->sub_feature_length ? "  (" : ""),
                                      (feature_desc->sub_feature_length ? feature_desc->sub_feature_length : ""),
                                      (feature_desc->sub_feature_length ? ")" : "")) ;
          else if (feature_desc->type == ZMAPSTYLE_MODE_ALIGNMENT && feature_desc->sub_feature_none_txt)
            text[3] = g_strdup(feature_desc->sub_feature_none_txt) ;
          else if (feature_desc->type == ZMAPSTYLE_MODE_TRANSCRIPT && feature_desc->sub_feature_none_txt)
            text[3] = g_strdup(feature_desc->sub_feature_none_txt) ;

          /* Frame */
          if (feature_desc->feature_frame)
            {
              text[4] = g_strdup(feature_desc->feature_frame) ;
            }

          if(feature_desc->feature_population)
            {
              text[5] = g_strdup_printf("%s",feature_desc->feature_population);
            }

          /* Score or percent_id attribute */
          if (feature_desc->type == ZMAPSTYLE_MODE_ALIGNMENT)
            {
              text[6] = g_strdup_printf("%s%s%s",
                                        feature_desc->feature_score ? feature_desc->feature_score : "",
                                        ((feature_desc->feature_percent_id && feature_desc->feature_score) ? " / " : ""),
                                        (feature_desc->feature_percent_id ? feature_desc->feature_percent_id : "")) ;
            }
          else if (feature_desc->feature_score)
            {
              text[6] = g_strdup_printf("%s", feature_desc->feature_score) ;
            }

          /* Style type */
          if (feature_desc->feature_term)
            {
              text[7] = g_strdup(feature_desc->feature_term) ;
            }

          /* Feature set */
          if (feature_desc->feature_set)
            {
              name_length = strlen(feature_desc->feature_set) ;
              if (name_length > text_length_max)
                {
                  temp_string_part = g_strndup(feature_desc->feature_set, text_length_max) ;
                  temp_string = g_strdup_printf(format_name_max, temp_string_part, "[...]") ;
                }
              else
                {
                  temp_string = g_strdup(feature_desc->feature_set) ;
                }
              text[8] = g_strdup(temp_string) ;
              if (temp_string)
                {
                  g_free(temp_string) ;
                  temp_string = NULL ;
                }
              if (temp_string_part)
                {
                  g_free(temp_string_part) ;
                  temp_string_part = NULL ;
                }
            }

          /* Source */
          if (feature_desc->feature_source)
            {
              name_length = strlen(feature_desc->feature_source) ;
              if (name_length > text_length_max)
                {
                  temp_string_part = g_strndup(feature_desc->feature_source, text_length_max) ;
                  temp_string = g_strdup_printf(format_name_max, temp_string_part, "[...]") ;
                }
              else
                {
                  temp_string = g_strdup(feature_desc->feature_source) ;
                }
              text[9] = g_strdup(temp_string) ;
              if (temp_string)
                {
                  g_free(temp_string) ;
                  temp_string = NULL ;
                }
              if (temp_string_part)
                {
                  g_free(temp_string_part) ;
                  temp_string_part = NULL ;
                }
            }

          /* Now to the tooltips... */

          /* The first one needs building, as it contains quite a wealth of information */
          desc_str = g_string_new("") ;

          g_string_append(desc_str, "FeatureName = '") ;
          g_string_append(desc_str, feature_desc->feature_name) ;
          g_string_append(desc_str, "'\n") ;

          if (feature_desc->feature_known_name)
            {
              g_string_append(desc_str, " (feature known name)") ;
            }

           if (feature_desc->feature_variation_string)
             {
               g_string_append(desc_str, g_strdup_printf("allele =\n%s", feature_desc->feature_variation_string )) ;
             }

          if (feature_desc->feature_total_length)
            {
              g_string_append(desc_str, " (total feature length)") ;
            }

          if (feature_desc->feature_known_name || feature_desc->feature_description
              || feature_desc->feature_locus)

            {
              g_string_append(desc_str, "\n\nExtra Feature Information:") ;

              if (feature_desc->feature_known_name)
                {
                  g_string_append(desc_str, "\n\n") ;

                  g_string_append_printf(desc_str, "Feature Known Name  -  \"%s\"",
                                         feature_desc->feature_known_name) ;
                }

              if (feature_desc->feature_description)
                {
                  g_string_append(desc_str, "\n\n") ;

                  g_string_append_printf(desc_str, "Notes  -  \"%s\"",
                                         feature_desc->feature_description) ;
                }

              if (feature_desc->feature_locus)
                {
                  g_string_append(desc_str, "\n\n") ;

                  g_string_append_printf(desc_str, "Locus  -  \"%s\"",
                                         feature_desc->feature_locus) ;
                }
            }

          /*
           * Text for the tooltips themselves.
           */
          tooltip[0] = g_string_free(desc_str, FALSE) ;

          if (feature_desc->type == ZMAPSTYLE_MODE_ALIGNMENT)
            tooltip[1] = g_strdup("Strand match is aligned to / Strand match is aligned from") ;
          else
            tooltip[1] = g_strdup("Strand feature is located on") ;

          if (feature_desc->type == ZMAPSTYLE_MODE_ALIGNMENT)
            tooltip[2] = g_strdup("sequence start, end  <-  match start, end (match length)") ;
          else
            tooltip[2] = g_strdup("Feature start, end (length)") ;

          if (feature_desc->sub_feature_term)
            tooltip[3] = g_strdup("sub_feature_type  index:   start, end  (length)") ;

          tooltip[4] = g_strdup("Frame") ;

          tooltip[5] = g_strdup("Number of clustered features") ;

          if (feature_desc->type == ZMAPSTYLE_MODE_ALIGNMENT && feature_desc->feature_percent_id)
            tooltip[6] = g_strdup("Score / percent ID") ;
          else if (feature_desc->feature_score)
            tooltip[6] = g_strdup("Score") ;

          tooltip[7] = g_strdup("Feature Term") ;
          tooltip[8] = g_strdup_printf("Feature Set =  '%s'", feature_desc->feature_set) ;
          tooltip[9] = g_strdup_printf("Feature Source = '%s'",  feature_desc->feature_source) ;
        }
    }


  for (i = 0 ; i < INFO_PANEL_LABELS ; i++)
    {
      GtkWidget *frame_parent = gtk_widget_get_parent(gtk_widget_get_parent(label[i])) ;

      if (text[i])
        {
          if (!GTK_WIDGET_MAPPED(frame_parent))
            gtk_widget_show_all(frame_parent) ;

          gtk_label_set_text(GTK_LABEL(label[i]), text[i]) ;

          if (tooltip[i])
            gtk_tooltips_set_tip(zmap->feature_tooltips,
                                 gtk_widget_get_parent(label[i]),
                                 tooltip[i], "") ;
        }
      else
        {
          gtk_widget_hide_all(frame_parent) ;
        }
    }

  /* Do some clearing up.... */
  for (i=0; i<INFO_PANEL_LABELS; ++i)
    {
      if (text[i])
        g_free(text[i]) ;
      if (tooltip[i])
        g_free(tooltip[i]) ;
    }

  /* I don't know if we need to do this each time....or if it does any harm.... */
  if (feature_desc)
    gtk_tooltips_enable(zmap->feature_tooltips) ;
  else
    gtk_tooltips_disable(zmap->feature_tooltips) ;

  return ;
}

