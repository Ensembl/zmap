/*  File: zmapControlImportFile.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2012: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Posts a dialog for user to enter sequence, start/end
 *              and optionally a config file for the sequence. Once
 *              user has chosen then this code calls the function
 *              provided by the caller to get the sequence displayed.
 *
 * Exported functions: See ZMap/zmapControlImportFile.h
 *
 * NOTE this file was initially copied from zmapAppSequenceView.c
 * and then tweaked. There may be some common code & functions
 * but maybe this file will be volatile for a while
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapString.h>				    /* testing only. */

#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStanzaStructs.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapControlImportFile.h>
#include <zmapControl_P.h>




#define N_FILE_TYPE (FILE_BIGWIG + 1)

/* number of optional dialog entries for FILE_NONE (is really 8 so i allowed a few spare) */
#define N_ARGS 16 


typedef enum { FILE_NONE, FILE_GFF, FILE_BAM, FILE_BIGWIG } fileType;

typedef struct
{
  fileType file_type;
  char * script;
  gchar ** args;   /* an allocated null terminated array os arg strings */
  gchar ** allocd; /* args for freeing on destroy */
} ZMapImportScriptStruct, *ZMapImportScript;


/* Data we need in callbacks. */
typedef struct MainFrameStructName
{
  GtkWidget *toplevel ;
  GtkWidget *sequence_widg ;
  GtkWidget *start_widg ;
  GtkWidget *end_widg ;
  GtkWidget *whole_widg ;
  GtkWidget *mark_widg ;

  GtkWidget *file_widg ;
  GtkWidget *script_widg;
  GtkWidget *args_widg;
  GtkWidget *req_sequence_widg ;
  GtkWidget *req_start_widg ;
  GtkWidget *req_end_widg ;
  GtkWidget *strand_widg;
  GtkWidget *source_widg;
  GtkWidget *style_widg;

  GtkWidget *map_widg ;
  GtkWidget *offset_widg ;
  GtkWidget *assembly_widg;


  fileType file_type;
  ZMapImportScriptStruct scripts[N_FILE_TYPE];

  gboolean is_otter;
  char *chr;

  ZMapFeatureSequenceMap sequence_map;

  ZMapControlImportFileCB user_func ;
  gpointer user_data ;
} MainFrameStruct, *MainFrame ;




static GtkWidget *makePanel(GtkWidget *toplevel, gpointer *seqdata_out,
                            ZMapControlImportFileCB user_func, gpointer user_data,
                            ZMapFeatureSequenceMap sequence_map, int req_start, int req_end) ;
static GtkWidget *makeMainFrame(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map) ;
static GtkWidget *makeOptionsBox(MainFrame main_frame, char *seq, int start, int end);
static GtkWidget *makeButtonBox(MainFrame main_frame) ;

static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;
static void importFileCB(GtkWidget *widget, gpointer cb_data) ;
static void chooseConfigCB(GtkFileChooserButton *widget, gpointer user_data) ;
static void closeCB(GtkWidget *widget, gpointer cb_data) ;

static void sequenceCB(GtkWidget *widget, gpointer cb_data) ;

static void fileChangedCB(GtkWidget *widget, gpointer user_data);
static void scriptChangedCB(GtkWidget *widget, gpointer user_data);




/*
 *                   External interface routines.
 */



/* Display a dialog to get from the reader a file to be displayed
 * with a optional start/end and various mapping parameters
 */
void zMapControlImportFile(ZMapControlImportFileCB user_func, gpointer user_data,
			    ZMapFeatureSequenceMap sequence_map, int req_start, int req_end)
{
  GtkWidget *toplevel, *container ;
  gpointer seq_data = NULL ;

  if (!user_func) 
    return ;

  ZMap zmap = (ZMap)user_data;

  toplevel = zMapGUIToplevelNew(NULL, "Please choose a file to import.") ;
  
  /* Make sure the dialog is destroyed if the zmap window is closed */
  /*! \todo For some reason this doesn't work and the dialog hangs around
   * after zmap->toplevel is destroyed */
  if (zmap && zmap->toplevel && GTK_IS_WINDOW(zmap->toplevel))
    gtk_window_set_transient_for(GTK_WINDOW(toplevel), GTK_WINDOW(zmap->toplevel)) ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  container = makePanel(toplevel, &seq_data, user_func, user_data, sequence_map, req_start, req_end) ;

  gtk_container_add(GTK_CONTAINER(toplevel), container) ;
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
		     GTK_SIGNAL_FUNC(toplevelDestroyCB), seq_data) ;

  gtk_widget_show_all(toplevel) ;

  return ;
}




/*
 *                   Internal routines.
 */



/* To fit in w/ ZMap practice we have to handle multiple views and config files
 * so this all has to be allocated */
static void importGetConfig(MainFrame main_frame, char *config_file)
{
  ZMapImportScript s;
  ZMapConfigIniContext context;
  fileType file_type = FILE_NONE;
  char * default_scripts[] = { "", "zmap_get_gff", "zmap_get_bam", "zmap_get_bigwig" };	
							    /* in parallel with the filetype enum */
  int i;
  char *tmp_string;
  ZMapImportScript scripts = main_frame->scripts;

  /* make some defaults for the file types we know to allow running unconfigured */
  for( i = 0;  i < N_FILE_TYPE; i++)
    {
      s = scripts + i;
      s->file_type = (fileType) i;
      s->script = NULL;
      s->args = NULL;
      s->allocd = NULL;
    }
  //	scripts[2].args = g_strsplit("--fruit=apple --car=jeep --weather=sunny", " ", 0);

  if ((context = zMapConfigIniContextProvide(config_file)))
    {
      GKeyFile *gkf;
      gchar ** keys,**freethis;
      gchar **args, **argp;
      char *arg_str;
      char *ft;
      gsize len;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
				       ZMAPSTANZA_APP_CSVER, &tmp_string))
	{
	  if(!g_ascii_strcasecmp(tmp_string,"Otter"))
	    {
	      char *chr;
	      main_frame->is_otter = TRUE;
	      if(zMapConfigIniContextGetString(context,
					       ZMAPSTANZA_APP_CONFIG,
					       ZMAPSTANZA_APP_CONFIG,
					       ZMAPSTANZA_APP_CHR,
					       &chr))
		{
		  main_frame->chr = chr;
		}
	      else
		{
		  main_frame->chr = NULL;
		}
	    }
	}

      if(zMapConfigIniHasStanza(context->config,ZMAPSTANZA_IMPORT_CONFIG,&gkf))
	{
	  freethis = keys = g_key_file_get_keys(gkf,ZMAPSTANZA_IMPORT_CONFIG,&len,NULL);

	  for(;len--;keys++)
	    {
	      arg_str = g_key_file_get_string(gkf,ZMAPSTANZA_IMPORT_CONFIG,*keys,NULL);

	      if(!arg_str || !*arg_str)
		continue;

	      argp = args = g_strsplit(arg_str, ";" , 0);
	      /* NB we strip leading & trailing whitespace from args later */

	      ft = *argp++;
	      while(*ft && *ft <= ' ')
		ft++;

	      if(!g_ascii_strncasecmp(ft,"GFF",3))
		file_type = FILE_GFF;
	      else if(!g_ascii_strncasecmp(ft,"BAM",3))
		file_type = FILE_BAM;
	      else if(!g_ascii_strncasecmp(ft,"BIGWIG",6))
		file_type = FILE_BIGWIG;
	      else
		continue;

	      s = scripts + file_type;
	      s->script = g_strdup(*keys);
	      s->args = argp;
	      s->allocd = args;
	    }
	}
    }

  for( i = 0; i < N_FILE_TYPE; i++)
    {
      s = scripts + i;
      if(!s->script)
	s->script = g_strdup(default_scripts[i]);
    }
  //	scripts[2].args = g_strsplit("--fruit=apple --car=jeep --weather=sunny", " ", 0);

  return ;
}


/* Make the whole panel returning the top container of the panel. */
static GtkWidget *makePanel(GtkWidget *toplevel, gpointer *our_data,
			    ZMapControlImportFileCB user_func, gpointer user_data,
			    ZMapFeatureSequenceMap sequence_map, int req_start, int req_end)
{
  GtkWidget *frame = NULL ;
  GtkWidget *vbox, *main_frame, *button_box, *options_box;
  MainFrame main_data ;
  char *sequence = "";

  main_data = g_new0(MainFrameStruct, 1) ;

  main_data->user_func = user_func ;
  main_data->user_data = user_data ;

  importGetConfig(main_data, sequence_map->config_file );

  if (toplevel)
    {
      main_data->toplevel = toplevel ;
      *our_data = main_data ;
    }

  frame = gtk_frame_new(NULL) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(frame), vbox) ;

  main_frame = makeMainFrame(main_data, sequence_map) ;
  gtk_box_pack_start(GTK_BOX(vbox), main_frame, TRUE, TRUE, 0) ;

  main_data->sequence_map = sequence_map;
  if(sequence_map)
    sequence = sequence_map->sequence;		/* request defaults to original */

  options_box = makeOptionsBox(main_data, sequence, req_start, req_end) ;
  gtk_box_pack_start(GTK_BOX(vbox), options_box, TRUE, TRUE, 0) ;


  button_box = makeButtonBox(main_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), button_box, TRUE, TRUE, 0) ;

  return frame ;
}




/* Make the label/entry fields frame. */
static GtkWidget *makeMainFrame(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label, *button ;
  char *sequence = "", *start = "", *end = "" ;

  if (sequence_map)
    {
      if (sequence_map->sequence)
	sequence = sequence_map->sequence ;
      if (sequence_map->start)
	start = g_strdup_printf("%d", sequence_map->start) ;
      if (sequence_map->end)
	end = g_strdup_printf("%d", sequence_map->end) ;
    }

  frame = gtk_frame_new( "ZMap Sequence: " );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);

  topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;


  /* Labels..... */
  labelbox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), labelbox, FALSE, FALSE, 0) ;

  label = gtk_label_new( "Sequence " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Start " ) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);

  label = gtk_label_new( "End " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;


  /* Entries.... */
  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  main_frame->sequence_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), sequence) ;
  //  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;
  gtk_widget_set_sensitive(GTK_WIDGET(entry),FALSE);

  main_frame->start_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), start) ;
  //  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;
  gtk_widget_set_sensitive(GTK_WIDGET(entry),FALSE);

  main_frame->end_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), end) ;
  //  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;
  gtk_widget_set_sensitive(GTK_WIDGET(entry),FALSE);


  {
    ZMap zmap = (ZMap) main_frame->user_data;
    ZMapWindow window = zMapViewGetWindow(zmap->focus_viewwindow);

    if(zMapWindowMarkIsSet(window))
      {
	hbox = gtk_hbox_new(FALSE, 0) ;
	gtk_container_border_width(GTK_CONTAINER(hbox), 0);
	gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;

	main_frame->whole_widg = button = gtk_button_new_with_label("Whole Sequence") ;
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(sequenceCB), (gpointer)main_frame) ;

	main_frame->mark_widg = button = gtk_button_new_with_label("Use Mark") ;
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(sequenceCB), (gpointer)main_frame) ;
      }
  }

  /* Free resources. */
  if (sequence_map)
    {
      if (*start)
	g_free(start) ;
      if (*end)
	g_free(end) ;
    }


  return frame ;
}


/* Make the option buttons frame. */
static GtkWidget *makeOptionsBox(MainFrame main_frame, char *req_sequence, int req_start, int req_end)
{
  GtkWidget *frame ;
  GtkWidget *map_seq_button, *config_button;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label ;
  char *sequence = "", *start = "", *end = "", *file = "" ;
  char *home_dir ;

  if (req_sequence)
    {
      sequence = req_sequence ;
      if (req_start)
	start = g_strdup_printf("%d", req_start) ;
      if (req_end)
	end = g_strdup_printf("%d", req_end) ;
    }

  frame = gtk_frame_new("Request Sequence") ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;

  topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;


  /* Labels..... */
  labelbox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), labelbox, FALSE, FALSE, 0) ;

  /* N.B. we use the gtk "built-in" file chooser stuff. */
  config_button = gtk_file_chooser_button_new("Choose a File to Import", GTK_FILE_CHOOSER_ACTION_OPEN) ;
  gtk_signal_connect(GTK_OBJECT(config_button), "file-set",
		     GTK_SIGNAL_FUNC(chooseConfigCB), (gpointer)main_frame) ;
  gtk_box_pack_start(GTK_BOX(labelbox), config_button, FALSE, TRUE, 0) ;
  home_dir = (char *)g_get_home_dir() ;
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(config_button), home_dir) ;
  gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(config_button), TRUE) ;

  label = gtk_label_new( "Script " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Extra Parameters " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Sequence " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Start " ) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);

  label = gtk_label_new( "End " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Strand " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Source " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Style " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Map Sequence " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Sequence Offset " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Assembly " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;


  /* Entries.... */
  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  main_frame->file_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), file) ;
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     GTK_SIGNAL_FUNC(fileChangedCB), (gpointer)main_frame) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  main_frame->script_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     GTK_SIGNAL_FUNC(scriptChangedCB), (gpointer)main_frame) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  main_frame->args_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  main_frame->req_sequence_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), sequence) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  main_frame->req_start_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), start) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  main_frame->req_end_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), end) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  main_frame->strand_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  main_frame->source_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  main_frame->style_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;


  main_frame->map_widg = map_seq_button = gtk_check_button_new () ;
  gtk_box_pack_start(GTK_BOX(entrybox), map_seq_button, FALSE, TRUE, 0) ;

  main_frame->offset_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  main_frame->assembly_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;


  if (*start)
    g_free(start) ;
  if (*end)
    g_free(end) ;


  return frame ;
}



/* Make the action buttons frame. */
static GtkWidget *makeButtonBox(MainFrame main_frame)
{
  GtkWidget *frame ;
  GtkWidget *button_box, *create_button, *close_button ;

  frame = gtk_frame_new(NULL) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;

  button_box = gtk_hbutton_box_new() ;
  gtk_container_border_width(GTK_CONTAINER(button_box), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), button_box) ;

  create_button = gtk_button_new_with_label("Import File") ;
  gtk_signal_connect(GTK_OBJECT(create_button), "clicked",
		     GTK_SIGNAL_FUNC(importFileCB), (gpointer)main_frame) ;
  gtk_box_pack_start(GTK_BOX(button_box), create_button, FALSE, TRUE, 0) ;


  /* Only add a close button and set a default button if this is a standalone dialog. */
  if (main_frame->toplevel)
    {
      close_button = gtk_button_new_with_label("Close") ;
      gtk_signal_connect(GTK_OBJECT(close_button), "clicked",
			 GTK_SIGNAL_FUNC(closeCB), (gpointer)main_frame) ;
      gtk_box_pack_start(GTK_BOX(button_box), close_button, FALSE, TRUE, 0) ;

      /* set create button as default. */
      GTK_WIDGET_SET_FLAGS(create_button, GTK_CAN_DEFAULT) ;
      gtk_window_set_default(GTK_WINDOW(main_frame->toplevel), create_button) ;
    }

  return frame ;
}



/* This function gets called whenever there is a gtk_widget_destroy() to the top level
 * widget. Sometimes this is because of window manager action, sometimes one of our exit
 * routines does a gtk_widget_destroy() on the top level widget.
 */
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_frame = (MainFrame)cb_data ;
  int i;

  for(i = 0; i < N_FILE_TYPE;i++)
    {
      if(main_frame->scripts[i].script)
	g_free(main_frame->scripts[i].script);

      if(main_frame->scripts[i].allocd)
	g_strfreev(main_frame->scripts[i].allocd);
    }

  return ;
}


/* Kill the dialog. */
static void closeCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_frame = (MainFrame)cb_data ;

  gtk_widget_destroy(main_frame->toplevel) ;

  return ;
}


/* set start, end as requested */
static void sequenceCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_frame = (MainFrame)cb_data ;
  int start, end;
  char buf[32];

  if(widget == main_frame->mark_widg)
    {
      ZMap zmap = (ZMap) main_frame->user_data;
      ZMapWindow window = zMapViewGetWindow(zmap->focus_viewwindow);

      zMapWindowGetMark(window, &start, &end);	/* NOTE we get -fsd coords from this function if revcomped */

      if(start < 0)
	start = -start;
      if(end < 0)
	end = -end;

      start += main_frame->sequence_map->start;
      end   += main_frame->sequence_map->start;
    }
  else if(widget == main_frame->whole_widg)
    {
      start = main_frame->sequence_map->start;
      end   = main_frame->sequence_map->end;
    }
  else
    {
      return;
    }

  sprintf(buf,"%d",start);
  gtk_entry_set_text(GTK_ENTRY( main_frame->req_start_widg), buf) ;
  sprintf(buf,"%d",end);
  gtk_entry_set_text(GTK_ENTRY( main_frame->req_end_widg),   buf) ;
}


/* Called when user chooses a file via the file dialog. */
static void chooseConfigCB(GtkFileChooserButton *widget, gpointer user_data)
{
  MainFrame main_frame = (MainFrame) user_data ;
  char *filename ;

  filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget)) ;

  gtk_entry_set_text(GTK_ENTRY(main_frame->file_widg), g_strdup(filename)) ;

  fileChangedCB ( main_frame->file_widg, user_data);
}


static void enable_widgets(MainFrame main_frame)
{
  gboolean is_gff = main_frame->file_type == FILE_GFF;
  gboolean is_bam = main_frame->file_type == FILE_BAM || main_frame->file_type == FILE_BIGWIG;

  gtk_widget_set_sensitive(main_frame->strand_widg, main_frame->file_type == FILE_NONE || main_frame->file_type == FILE_BIGWIG );
  gtk_widget_set_sensitive(main_frame->source_widg, !is_gff);
  gtk_widget_set_sensitive(main_frame->style_widg,  !is_gff);

  gtk_widget_set_sensitive(main_frame->args_widg, main_frame->file_type == FILE_NONE );

  gtk_widget_set_sensitive(main_frame->map_widg, !(is_bam && main_frame->is_otter));
  gtk_widget_set_sensitive(main_frame->offset_widg, !(is_bam && main_frame->is_otter));
  gtk_widget_set_sensitive(main_frame->req_sequence_widg, !(main_frame->is_otter));
  gtk_widget_set_sensitive(main_frame->assembly_widg, (main_frame->is_otter));
}

static void scriptChangedCB(GtkWidget *widget, gpointer user_data)
{
  MainFrame main_frame = (MainFrame) user_data ;

  main_frame->file_type = FILE_NONE;		/* user defined script */

  enable_widgets(main_frame);
}

static void fileChangedCB(GtkWidget *widget, gpointer user_data)
{
  MainFrame main_frame = (MainFrame) user_data ;
  char *filename ;
  char *extent;
  fileType file_type = FILE_GFF;
  char *source_txt = NULL;
  char *style_txt = NULL;
  ZMapFeatureSource src;
  ZMapView view;
  ZMap zmap = (ZMap) main_frame->user_data;

  char *args_txt = "";
  ZMapImportScript scripts;

  /* Check that the zmap window has not been closed (currently
   * the import dialog isn't closed with it, which will cause
   * problems if we don't exit early here ) */
  if (!zmap->toplevel)
    return;
  
  view = zMapViewGetView(zmap->focus_viewwindow);

  filename = (char *) gtk_entry_get_text(GTK_ENTRY(widget)) ;

  extent = filename + strlen(filename);
  while(*extent != '.' && extent > filename)
    extent--;
  if(!g_ascii_strcasecmp(extent,".bam"))
    file_type = FILE_BAM;
  else if(!g_ascii_strcasecmp(extent,".bigwig"))
    file_type = FILE_BIGWIG;

  main_frame->file_type = file_type;

  // look up script and args and display themn
  for (scripts = main_frame->scripts; scripts ; scripts++)
    {
      if(scripts->file_type == file_type)
	{
	  gtk_entry_set_text(GTK_ENTRY(main_frame->script_widg), scripts->script) ;

	  if(scripts->args)
	    args_txt = g_strjoinv(" ", scripts->args);

	  gtk_entry_set_text(GTK_ENTRY(main_frame->args_widg), args_txt) ;
	  break;
	}
    }

  if(file_type != FILE_GFF)
    {
      char *name = extent;

      while(name > filename && *name != '/')
	name--;
      if(*name == '/')
	name++;
      source_txt = g_strdup_printf("%.*s", (int)(extent - name), name) ;

      gtk_entry_set_text(GTK_ENTRY(main_frame->source_widg), source_txt) ;

    }

  if(file_type == FILE_BAM)
    {
      /* add featureset_2_style entry to the view */
      GQuark f_id;

      f_id = zMapFeatureSetCreateID("BAM");
      src = zMapViewGetFeatureSetSource(view, f_id);
      if(src)
	{
	  style_txt = g_strdup_printf("%s",g_quark_to_string(src->style_id));
	  gtk_entry_set_text(GTK_ENTRY(main_frame->style_widg), style_txt) ;
	  //			src->is_seq = TRUE;
	}
    }

  if(file_type == FILE_BIGWIG)
    {
      /* add featureset_2_style entry to the view */
      GQuark f_id;
      char * strand_txt;

      f_id = zMapFeatureSetCreateID("bigWig");
      src = zMapViewGetFeatureSetSource(view, f_id);
      if(src)
	{
	  style_txt = g_strdup_printf("%s",g_quark_to_string(src->style_id));
	  gtk_entry_set_text(GTK_ENTRY(main_frame->style_widg), style_txt) ;
	  //			src->is_seq = TRUE;
	}

      strand_txt = g_strstr_len(filename,-1,"inus") ? "-" : "+";		/* encode has 'Minus' */
      gtk_entry_set_text(GTK_ENTRY(main_frame->strand_widg), strand_txt) ;

    }

  enable_widgets(main_frame);

  return ;
}


/* Ok...check the users entries and then call the callback function provided.
 *
 * Note that valid entries are:
 *
 *       sequence & start & end with optional config file
 *
 *       config file (which contains sequence, start, end)
 *
 *  */
static void importFileCB(GtkWidget *widget, gpointer cb_data)
{
  gboolean status = TRUE ;
  MainFrame main_frame = (MainFrame)cb_data ;
  char *err_msg = NULL ;
  char *sequence = "", *start_txt, *end_txt, *file_txt, *script_txt, *args_txt,
    *req_start_txt, *req_end_txt, *offset_txt, *source_txt, *style_txt, *strand_txt, *assembly_txt ;
  int start = 1, end = 0 ;
  gboolean map_seq = FALSE;
  int seq_offset = 0;
  char *req_sequence;
  int req_start, req_end;
  fileType file_type = main_frame->file_type;
  ZMapView view;
  ZMap zmap = (ZMap) main_frame->user_data;
  int strand = 0 ;

  /* Check that the zmap window hasn't been closed (currently the dialog
   * isn't closed automatically with it, so we'll have problems if we don't 
   * exit early here) */
  if (!zmap->toplevel)
    return;

  view = zMapViewGetView(zmap->focus_viewwindow);

  /* Note gtk_entry returns the empty string "" _not_ NULL when there is no text. */
  sequence = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->sequence_widg)) ;
  start_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->start_widg)) ;
  end_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->end_widg)) ;

  file_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->file_widg)) ;
  script_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->script_widg)) ;
  args_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->args_widg)) ;
  req_sequence = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->req_sequence_widg)) ;
  req_start_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->req_start_widg)) ;
  req_end_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->req_end_widg)) ;
  source_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->source_widg)) ;
  style_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->style_widg)) ;

  offset_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->offset_widg)) ;
  assembly_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->assembly_widg)) ;

  if (status)
    {
      if (!(*file_txt))
	{
	  status = FALSE ;
	  err_msg = "Please choose a file to import." ;
	}
    }

  if (status)
    {
      if (*sequence && *start_txt && *end_txt)
        {
	  if (!zMapStr2Int(start_txt, &start) || start < 1)
	    {
	      status = FALSE ;
	      err_msg = "Invalid start specified." ;
	    }
	  else if (!zMapStr2Int(end_txt, &end) || end <= start)
	    {
	      status = FALSE ;
	      err_msg = "Invalid end specified." ;
	    }
	}
    }

  if (status)
    {
      if (*req_sequence && *req_start_txt && *req_end_txt)
        {
	  if (!zMapStr2Int(req_start_txt, &req_start) || req_start < 1)
	    {
	      status = FALSE ;
	      err_msg = "Invalid request start specified." ;
	    }
	  else if (!zMapStr2Int(req_end_txt, &req_end) || req_end <= start)
	    {
	      status = FALSE ;
	      err_msg = "Invalid request end specified." ;
	    }
	}
      else
        {
          status = FALSE;
          err_msg = "Please specify request sequence start and end";
        }
    }

  if (status)
    {
      if ((*offset_txt) && !zMapStr2Int(offset_txt, &seq_offset))
        {
          status = FALSE ;
          err_msg = "Invalid offset specified." ;
        }
    }

  if (status)
    {
      map_seq = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(main_frame->map_widg));
    }

  if (status)
    {
      if ((strand_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->strand_widg)))
	  && *strand_txt)
        {
	  /* NOTE this is not +/- as presented to the user but 1 or -1. */
	  if ((strchr(strand_txt, '+')))
	    strand = 1 ;
	  else if ((strchr(strand_txt, '-')))
	    strand = -1 ;
	  else
            {
              status = FALSE ;
              err_msg = "Strand must be + or -";
            }
        }
    }


  if (!status)
    {
      zMapWarning("Error: \"%s\"", err_msg) ;
    }
  else
    {
      char *config_str;
      GList * servers;
      ZMapConfigSource server;
      GList *req_featuresets = NULL;
      char *and = "";
      gchar *args[N_ARGS];
      gchar **argp = args;
      char * opt_args_txt = "";

      if (main_frame->sequence_map && (seq_offset || map_seq))
        seq_offset += main_frame->sequence_map->start;

      if (file_type != FILE_GFF)
        {
          /* add featureset_2_style entry to the view */
          ZMapFeatureSource src;
          GQuark f_id;

          f_id = zMapFeatureSetCreateID(source_txt);
          src = zMapViewGetFeatureSetSource(view, f_id);
          if (!src)
            {
              src = g_new0(ZMapFeatureSourceStruct,1);
              src->source_text = g_quark_from_string(source_txt);
              src->source_id = f_id;
              zMapViewSetFeatureSetSource(view, f_id, src);
            }

          src->style_id = zMapStyleCreateID(style_txt);
          src->is_seq = TRUE;
        }

      /* prep user defined args */
      if (*args_txt)
        {
          gchar **vector ;

	  args_txt = zMapStringCompress(args_txt) ;

          vector = g_strsplit(args_txt, " ", 0) ;

          args_txt = g_strjoinv("&", vector) ;

          g_strfreev(vector) ;

          and = "&" ;
        }

      /* prep dialog defined args according to file type and session */
      /* some are mandatory: */
      *argp++ = g_strdup_printf("--file=%s", file_txt);
      *argp++ = g_strdup_printf("--gff_seqname=%s", sequence);
      *argp++ = g_strdup_printf("--start=%d", req_start);
      *argp++ = g_strdup_printf("--end=%d", req_end);

      if ((seq_offset || map_seq) && !main_frame->is_otter)
        *argp++ = g_strdup_printf("--mapto=%d", seq_offset);

      if (main_frame->is_otter)
        {
          *argp++ = g_strdup_printf("--csver=%s", assembly_txt);
          if (main_frame->chr)
            *argp++ = g_strdup_printf("--chr=%s", main_frame->chr);
        }

      if (req_sequence && !main_frame->is_otter)
        *argp++ = g_strdup_printf("--seq_id=%s", req_sequence);

      /* some depend on file type */
      switch(file_type)
        {
        case FILE_NONE:
          /* add in any that have data */
          if (source_txt)
            *argp++ = g_strdup_printf("--gff_feature_source=%s", source_txt) ;
          if (strand)
            *argp++ = g_strdup_printf("--strand=%d", strand) ;
          break;

        case FILE_GFF:
          break;

        case FILE_BIGWIG:
          *argp++ = g_strdup_printf("--strand=%d", strand) ;
          /* fall through */

        case FILE_BAM:
          *argp++ = g_strdup_printf("--gff_feature_source=%s", source_txt);
          break;
        }

      *argp = NULL;
      opt_args_txt = g_strjoinv("&", args);
      while(argp > args)
        g_free(*--argp);

      config_str = g_strdup_printf("[ZMap]\nsources = temp\n\n[temp]\nfeaturesets=\nurl=pipe://%s/%s?%s%s%s",
                                   *script_txt == '/' ? "/" :"", script_txt, args_txt, and, opt_args_txt);

      servers = zmapViewGetIniSources(NULL, config_str, NULL) ;

      server = (ZMapConfigSource) servers->data ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* SEEMS LIKE A BUG TO ME...THE req_start/end ARE IGNORED HERE ! */
      /* Leave this here until it's tested..... */

      if (zMapViewRequestServer(view, NULL, req_featuresets, (gpointer) server, start, end, FALSE, TRUE, TRUE))
        zMapViewShowLoadStatus(view);
      else
        zMapWarning("could not request %s",file_txt);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      if (zMapViewRequestServer(view, NULL, req_featuresets, (gpointer)server, req_start, req_end, FALSE, TRUE, TRUE))
        zMapViewShowLoadStatus(view);
      else
        zMapWarning("could not request %s",file_txt);



      zMapConfigSourcesFreeList(servers);

      g_free(config_str);


      /* I'VE NO IDEA WHAT THIS IS SUPPOSED TO MEAN....EG */
      /* call user func if you like.... */
    }

  return ;
}


