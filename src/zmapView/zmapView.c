/*  File: zmapView.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Handles the getting of the feature context from sources
 *              and their subsequent processing.
 *
 * Exported functions: See ZMap/zmapView.h
 * HISTORY:
 * Last edited: May  5 17:39 2010 (edgrif)
 * Created: Thu May 13 15:28:26 2004 (edgrif)
 * CVS info:   $Id: zmapView.c,v 1.199 2010-05-19 13:15:31 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <sys/types.h>
#include <signal.h>             /* kill() */
#include <glib.h>
#include <gtk/gtk.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapGFF.h>
#include <ZMap/zmapUtilsXRemote.h>
#include <ZMap/zmapXRemote.h>
#include <ZMap/zmapCmdLineArgs.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapConfigStanzaStructs.h>

#include <zmapView_P.h>

typedef struct
{
  /* Processing options. */

  gboolean dynamic_loading ;


  /* Context data. */

  /* database data. */
  char *database_name ;
  char *database_title ;
  char *database_path ;

  char *err_msg;        // from the server mainly

  GList *feature_sets ;

  GList *required_styles ;
  gboolean server_styles_have_mode ;
  gboolean status;      // load sucessful?

  GHashTable *featureset_2_stylelist ;                    /* Mapping of each feature_set to all
                                                 the styles it requires. */

  GHashTable *featureset_2_column;        // needed by pipeServers
  GHashTable *source_2_sourcedata;

  GData *curr_styles ;                              /* Styles for this context. */
  ZMapFeatureContext curr_context ;

  ZMapServerReqType display_after ;
} ConnectionDataStruct, *ConnectionData ;


typedef struct
{
  gboolean found_style ;
  GString *missing_styles ;
} DrawableDataStruct, *DrawableData ;


typedef struct
{
  GData *styles ;
  gboolean error ;
} UnsetDeferredLoadStylesStruct, *UnsetDeferredLoadStyles ;


static GList *zmapViewGetIniSources(char *config_str,char **stylesfile);

static ZMapView createZMapView(GtkWidget *xremote_widget, char *view_name,
			       GList *sequences, void *app_data) ;
static void destroyZMapView(ZMapView *zmap) ;

static gint zmapIdleCB(gpointer cb_data) ;
static void enterCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void leaveCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void scrollCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void viewFocusCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void viewSelectCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void viewVisibilityChangeCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void setZoomStatusCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void commandCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void loaded_dataCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void viewSplitToPatternCB(ZMapWindow window, void *caller_data, void *window_data);

static void setZoomStatus(gpointer data, gpointer user_data);
static void splitMagic(gpointer data, gpointer user_data);

static void unsetDeferredLoadStylesCB(GQuark key_id, gpointer data, gpointer user_data) ;

static void startStateConnectionChecking(ZMapView zmap_view) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void stopStateConnectionChecking(ZMapView zmap_view) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static gboolean checkStateConnections(ZMapView zmap_view) ;

static gboolean dispatchContextRequests(ZMapViewConnection view_con, ZMapServerReqAny req_any) ;
static gboolean processDataRequests(ZMapViewConnection view_con, ZMapServerReqAny req_any) ;
static void freeDataRequest(ZMapServerReqAny req_any) ;

static gboolean processGetSeqRequests(ZMapViewConnection view_con, ZMapServerReqAny req_any) ;

static ZMapViewConnection createConnection(ZMapView zmap_view,
                                 ZMapViewConnection view_con,
                                 ZMapFeatureContext context,
					   char *url, char *format,
					   int timeout, char *version,
					   char *styles, char *styles_file,
					   GList *req_featuresets,
					   gboolean dna_requested,
                                 gboolean terminate);
static void destroyConnection(ZMapView view, ZMapViewConnection view_conn) ;
static void killGUI(ZMapView zmap_view) ;
static void killConnections(ZMapView zmap_view) ;

static void resetWindows(ZMapView zmap_view) ;
static void displayDataWindows(ZMapView zmap_view,
			       ZMapFeatureContext all_features,
                               ZMapFeatureContext new_features, GData *new_styles,
                               gboolean undisplay) ;
static void killAllWindows(ZMapView zmap_view) ;

static ZMapViewWindow createWindow(ZMapView zmap_view, ZMapWindow window) ;
static void destroyWindow(ZMapView zmap_view, ZMapViewWindow view_window) ;

static void getFeatures(ZMapView zmap_view, ZMapServerReqGetFeatures feature_req, GData *styles) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static GList *string2StyleQuarks(char *feature_sets) ;
static gboolean nextIsQuoted(char **text) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static gboolean justMergeContext(ZMapView view, ZMapFeatureContext *context_inout, GData *styles);
static void justDrawContext(ZMapView view, ZMapFeatureContext diff_context, GData *styles);

static ZMapFeatureContext createContext(char *sequence, int start, int end, GList *feature_set_names) ;

static ZMapViewWindow addWindow(ZMapView zmap_view, GtkWidget *parent_widget) ;

static void addAlignments(ZMapFeatureContext context) ;

static gboolean mergeAndDrawContext(ZMapView view, ZMapFeatureContext context_inout, GData *styles);
static void eraseAndUndrawContext(ZMapView view, ZMapFeatureContext context_inout);

#ifdef NOT_REQUIRED_ATM
static gboolean getSequenceServers(ZMapView zmap_view, char *config_str) ;
static void destroySeq2ServerCB(gpointer data, gpointer user_data_unused) ;
static ZMapViewSequence2Server createSeq2Server(char *sequence, char *server) ;
static void destroySeq2Server(ZMapViewSequence2Server seq_2_server) ;
static gboolean checkSequenceToServerMatch(GList *seq_2_server, ZMapViewSequence2Server target_seq_server) ;
static gint findSequence(gconstpointer a, gconstpointer b) ;
#endif /* NOT_REQUIRED_ATM */

static void threadDebugMsg(ZMapThread thread, char *format_str, char *msg) ;

static void killAllSpawned(ZMapView zmap_view);

static gboolean checkContinue(ZMapView zmap_view) ;


static gboolean makeStylesDrawable(GData *styles, char **missing_styles_out) ;
static void drawableCB(GQuark key_id, gpointer data, gpointer user_data) ;

static void addPredefined(GData **styles_inout, GHashTable **featureset_2_stylelist_inout) ;
static void styleCB(GQuark key_id, gpointer data, gpointer user_data) ;

static void invoke_merge_in_names(gpointer list_data, gpointer user_data);

static gboolean mapEventCB(GtkWidget *widget, GdkEvent *event, gpointer user_data) ;



/* These callback routines are global because they are set just once for the lifetime of the
 * process. */

/* Callbacks we make back to the level above us. */
static ZMapViewCallbacks view_cbs_G = NULL ;


/* Callbacks back we set in the level below us, i.e. zMapWindow. */
ZMapWindowCallbacksStruct window_cbs_G =
{
  enterCB, leaveCB,
  scrollCB,
  viewFocusCB,
  viewSelectCB,
  viewSplitToPatternCB,
  setZoomStatusCB,
  viewVisibilityChangeCB,
  commandCB,
  loaded_dataCB
} ;




/*
 *  ------------------- External functions -------------------
 */



/*! @defgroup zmapview   zMapView: feature context display/processing
 * @{
 *
 * \brief  Feature Context View Handling.
 *
 * zMapView routines receive requests to load, display and process
 * feature contexts. Each ZMapView corresponds to a single feature context.
 *
 *
 *  */



/*!
 * This routine must be called just once before any other views routine,
 * the caller must supply all of the callback routines.
 *
 * @param callbacks   Caller registers callback functions that view will call
 *                    from the appropriate actions.
 * @return <nothing>
 *  */
void zMapViewInit(ZMapViewCallbacks callbacks)
{
  zMapAssert(!view_cbs_G) ;
  /* We have to assert alot here... */
  zMapAssert(callbacks);
  zMapAssert(callbacks->enter);
  zMapAssert(callbacks->leave);
  zMapAssert(callbacks->load_data);
  zMapAssert(callbacks->focus);
  zMapAssert(callbacks->select);
  zMapAssert(callbacks->visibility_change);
  zMapAssert(callbacks->state_change && callbacks->destroy) ;

  /* Now we can get on. */
  view_cbs_G = g_new0(ZMapViewCallbacksStruct, 1) ;

  view_cbs_G->enter = callbacks->enter ;
  view_cbs_G->leave = callbacks->leave ;
  view_cbs_G->load_data = callbacks->load_data ;
  view_cbs_G->focus = callbacks->focus ;
  view_cbs_G->select = callbacks->select ;
  view_cbs_G->split_to_pattern = callbacks->split_to_pattern;
  view_cbs_G->visibility_change = callbacks->visibility_change ;
  view_cbs_G->state_change = callbacks->state_change ;
  view_cbs_G->destroy = callbacks->destroy ;


  /* Init windows.... */
  zMapWindowInit(&window_cbs_G) ;

  return ;
}



/*!
 * Create a "view", this is the holder for a single feature context. The view may use
 * several threads to get this context and may display it in several windows.
 * A view _always_ has at least one window, this window may be blank but as long as
 * there is a view, there is a window. This makes the coding somewhat simpler and is
 * intuitively sensible.
 *
 * @param xremote_widget   Widget that xremote commands for this view will be delivered to.
 * @param view_container   Parent widget of the view window(s)
 * @param sequence         Name of virtual sequence of context to be created.
 * @param start            Start coord of virtual sequence.
 * @param end              End coord of virtual sequence.
 * @param app_data         data that will be passed to the callers callback routines.
 * @return a new ZMapViewWindow (= view + a window)
 *  */
ZMapViewWindow zMapViewCreate(GtkWidget *xremote_widget, GtkWidget *view_container,
			      char *sequence, int start, int end,
			      void *app_data)
{
  ZMapViewWindow view_window = NULL ;
  ZMapView zmap_view = NULL ;
  ZMapViewSequenceMap sequence_fetch ;
  GList *sequences_list = NULL ;
  char *view_name ;

  /* No callbacks, then no view creation. */
  zMapAssert(view_cbs_G);
  zMapAssert(GTK_IS_WIDGET(view_container));
  zMapAssert(sequence);
//  zMapAssert(start > 0);
  zMapAssert((end == 0 || end >= start)) ;

  /* Set up debugging for threads and servers, we do it here so that user can change setting
   * in config file and next time they create a view the debugging will go on/off. */
  zMapUtilsConfigDebug();


  /* Set up sequence to be fetched, in this case server defaults to whatever is set in config. file. */
  sequence_fetch = g_new0(ZMapViewSequenceMapStruct, 1) ;
  sequence_fetch->sequence = g_strdup(sequence) ;
  sequence_fetch->start = start ;
  sequence_fetch->end = end ;
  sequences_list = g_list_append(sequences_list, sequence_fetch) ;

  view_name = sequence ;

  zmap_view = createZMapView(xremote_widget, view_name, sequences_list, app_data) ; /* N.B. this step can't fail. */

  zmap_view->state = ZMAPVIEW_INIT ;

  view_window = addWindow(zmap_view, view_container) ;

  zmap_view->cwh_hash = zmapViewCWHHashCreate();

  zmap_view->featureset_2_stylelist = zMap_g_hashlist_create() ;


  return view_window ;
}



void zMapViewSetupNavigator(ZMapViewWindow view_window, GtkWidget *canvas_widget)
{
  ZMapView zmap_view ;

  zMapAssert(view_window) ;

  zmap_view = view_window->parent_view ;

  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      zmap_view->navigator_window = zMapWindowNavigatorCreate(canvas_widget);
      zMapWindowNavigatorSetCurrentWindow(zmap_view->navigator_window, view_window->window);
    }

  return ;
}



/* read in rather a lot of stanzas and add the data to a few hash tables and lists
 * This sets up:
 *    view->columns
 *    view->featureset_2_column
 *    view->source_2_sourcedata
 *    view->featureset_2_stylelist
 *
 * This 'replaces' ACEDB data but if absent we should still use the ACE stuff, it all gets merged
 */

void print_src2src(char * str,GHashTable *data)
{
      GList *iter;
      ZMapGFFSource gff_source;
      gpointer key, value;

      printf("\nsrc2src %s:\n",str);

      zMap_g_hash_table_iter_init (&iter, data);
      while (zMap_g_hash_table_iter_next (&iter, &key, &value))
      {
            gff_source = (ZMapGFFSource) value;
            printf("%s = %s %s\n",g_quark_to_string(GPOINTER_TO_UINT(key)),
                  g_quark_to_string(gff_source->source_id),
                  g_quark_to_string(gff_source->style_id));
      }
}
void print_fset2col(char * str,GHashTable *data)
{
      GList *iter;
      ZMapGFFSet gff_set;
      gpointer key, value;

      printf("\nfset2col %s:\n",str);

      zMap_g_hash_table_iter_init (&iter, data);
      while (zMap_g_hash_table_iter_next (&iter, &key, &value))
      {
            gff_set = (ZMapGFFSet) value;
            printf("%s = %s %s %s \"%s\"\n",g_quark_to_string(GPOINTER_TO_UINT(key)),
                  g_quark_to_string(gff_set->feature_set_id),
                  g_quark_to_string(gff_set->feature_set_ID),
                  g_quark_to_string(gff_set->feature_src_ID),
                  gff_set->feature_set_text ? gff_set->feature_set_text : "");
      }
}


void zmapViewGetIniData(ZMapView view, char *config_str, GList *sources)
{
  ZMapConfigIniContext context ;
  // 11 structs to juggle, count them! It's GLibalicious!!
  GHashTable *fset_col;
  GHashTable *fset_styles;
  GHashTable *gff_src;
  GHashTable *col_styles;
  GHashTable *gff_desc;
  GHashTable *col_desc;
  GHashTable *src2src;
  ZMapGFFSource gff_source;
  ZMapGFFSet gffset;
  GList *iter;

  gpointer key, value;
  GList *columns;


  /*
   * This gets very fiddly
   * - lots of the names may have whitespace, which we normalise
   * - most of the rest of the code lowercases the names so we have to
   *   yet we still need to display the text as capitlaised if that's what people put in
   * See zMapConfigIniGetQQHash()...
   *
   */

  if ((context = zMapConfigIniContextProvide()))
    {
      if(config_str)
        zMapConfigIniContextIncludeBuffer(context, config_str);

            /*-------------------------------------
             * the display columns in L -> R order
             *-------------------------------------
             */
      view->columns = zMapConfigIniGetColumns(context);

      if(view->columns)       // else we rely on defaults and/or ACEDB
      {
        view->columns_set = view->columns ? TRUE : FALSE;


            /*-------------------------------------------------------------------------------
             * featureset to column mapping, with column descriptions added to GFFSet struct
             *-------------------------------------------------------------------------------
             */
        // default 1-1 mapping : add for pipe servers, ACE and DAS provide this
        // it's an input and output for servers, must provide for pipes

        fset_col = g_hash_table_new(NULL,NULL);

        for(; sources ; sources = sources->next)
          {
            GList *featuresets;
            ZMapConfigSource src;
            src = (ZMapConfigSource) sources->data;

            if(g_ascii_strncasecmp(src->url,"pipe",4) && g_ascii_strncasecmp(src->url,"file",4))
                  continue;

            featuresets = zMapConfigString2QuarkList(src->featuresets) ;

            while(featuresets)
              {
                 GQuark fset,col;

                 gffset = g_new0(ZMapGFFSetStruct,1);

                 col = GPOINTER_TO_UINT(featuresets->data);
                 fset = zMapFeatureSetCreateID((char *) g_quark_to_string(col));
                 gffset->feature_set_id = fset;
                 gffset->feature_set_ID = col;
                 gffset->feature_src_ID = col;
                 g_hash_table_insert(fset_col,GUINT_TO_POINTER(fset),gffset);

                 featuresets = g_list_delete_link(featuresets,featuresets);
              }
          }


        // nb there is a small memory leak if we config description for non-existant columns
        col_desc   = zMapConfigIniGetQQHash(context,ZMAPSTANZA_COLUMN_DESCRIPTION_CONFIG,QQ_STRING);
        fset_col   = zMapConfigIniGetFeatureset2Column(context,fset_col,col_desc);

        if(g_hash_table_size(fset_col))
            view->featureset_2_column = fset_col;
        else
            g_hash_table_destroy(fset_col);
        if(col_desc)
            g_hash_table_destroy(col_desc);




            /*-----------------------------------------------------------------------
             * source_2_sourcedata:featureset -> (sourceid, styleid, description)
             *-----------------------------------------------------------------------
             */

        src2src = g_hash_table_new(NULL,NULL);

            // source id may not be a style but it's name still gets normalised
        gff_src   = zMapConfigIniGetQQHash(context,ZMAPSTANZA_GFF_SOURCE_CONFIG,QQ_QUARK);
        fset_styles = zMapConfigIniGetQQHash(context,ZMAPSTANZA_FEATURESET_STYLE_CONFIG,QQ_STYLE);
        gff_desc  = zMapConfigIniGetQQHash(context,ZMAPSTANZA_GFF_DESCRIPTION_CONFIG,QQ_QUARK);
        gff_source = NULL;

        // it's an input and output for servers, must provide for pipes
        // see featureset_2_column as above
        zMap_g_hash_table_iter_init(&iter,view->featureset_2_column);
        while(zMap_g_hash_table_iter_next(&iter,&key,&value))
          {
            GQuark q;
            gboolean set;

            set = FALSE;

            if(!gff_source)
                  gff_source = g_new0(ZMapGFFSourceStruct,1);

                  // start with a 1-1 default mapping
            gffset = (ZMapGFFSet) value;

            gff_source->source_id = gffset->feature_src_ID;   // upper case wanted
            gff_source->style_id = zMapStyleCreateID((char *) g_quark_to_string(GPOINTER_TO_UINT(key)));
            gff_source->source_text = gff_source->source_id;

                  // then overlay this with the config file

                  // get the GFFsource name
                  // this is displayed at status bar top right
            if(gff_src)
            {
                  q = GPOINTER_TO_UINT(g_hash_table_lookup(gff_src,key));
                  if(q)
                        gff_source->source_id = q;
                  set = TRUE;
            }
                  // get style defined by featureset name
            if(fset_styles)
            {
                  q = GPOINTER_TO_UINT(g_hash_table_lookup(fset_styles,key));
                  if(q)
                        gff_source->style_id = q;
                  set = TRUE;
            }
                  // get description defined by featureset name
            if(gff_desc)
            {
                  q = GPOINTER_TO_UINT(g_hash_table_lookup(gff_desc,key));
                  if(q)
                        gff_source->source_text = q;
                  set = TRUE;
            }

            //if(set)   // must do all for pipe servers
            {
                  // source_2_source data defaults are hard coded in GFF2parser
                  // but if we set one field then we set them all
                  g_hash_table_insert(src2src,
                        GUINT_TO_POINTER(zMapFeatureSetCreateID(
                              (char *)g_quark_to_string(GPOINTER_TO_UINT(key)))),
                        gff_source);
                  gff_source = NULL;
            }
          }

        if(gff_source)
            g_free(gff_source);

        if(gff_src)
            g_hash_table_destroy(gff_src);
        if(fset_styles)
            g_hash_table_destroy(fset_styles);
        if(gff_desc)
            g_hash_table_destroy(gff_desc);

        view->source_2_sourcedata = src2src;

//        print_src2src("view ini",view->source_2_sourcedata);
//        print_fset2col("view ini",view->featureset_2_column);

            /*---------------------------------------------
             * featureset_2_stylelist: hash of Glist of quarks
             *---------------------------------------------
             * contains all the styles needed by a column
             * NB: style for each featureset is also held in source_2_sourcedata above
             * column specifc style (not featureset style included in column)
             * is optional and we only include it if configured
             */

        // NB we have to use zMap_g_hashlist_thing() as this is used all over
        // view->featureset_2_stylelist is pre-allocated


        // get list of featuresets for each column
        // add column style if it exists to glist
        // add corresponding featureset styles to glist
        // add glist to view->featuresets_2_styles

        // or rather, given that our data is upside down:
        // add each featureset's style to the hashlist
        // then add the column styles if configured
        // these are keyed by the column quark


            // add featureset styles
        zMap_g_hash_table_iter_init (&iter, view->featureset_2_column);
        while (zMap_g_hash_table_iter_next (&iter, &key, &value))
        {
            GQuark style_id,fset_id;

            gffset = (ZMapGFFSet) value;

            // key is featureset quark, value is column GFFSet struct

            gff_source = g_hash_table_lookup(src2src,key);
            if(gff_source)
            {
                  style_id = gff_source->style_id;
//                  fset_id = zMapFeatureSetCreateID((char *)g_quark_to_string(gffset->feature_set_id));
                  fset_id = gffset->feature_set_id;

                  zMap_g_hashlist_insert(view->featureset_2_stylelist,
                                   fset_id,     // the column
                                   GUINT_TO_POINTER(style_id)) ;  // the style
            }
        }

            // add col specific styles

        col_styles  = zMapConfigIniGetQQHash(context,ZMAPSTANZA_COLUMN_STYLE_CONFIG,QQ_STYLE);

        for(columns = view->columns;columns;columns = columns->next)
        {
            GQuark style_id,fset_id;

            style_id = 0;
            fset_id = zMapFeatureSetCreateID((char *)g_quark_to_string(GPOINTER_TO_UINT(columns->data)));

            if(col_styles)
                  style_id = GPOINTER_TO_UINT(g_hash_table_lookup(col_styles,GUINT_TO_POINTER(fset_id)));
                              // set the column style if there
            if(style_id)
                  zMap_g_hashlist_insert(view->featureset_2_stylelist,
                                   fset_id,
                                   GUINT_TO_POINTER(style_id)) ;
        }

//        printf("\nfset2style\n");
//        zMap_g_hashlist_print(view->featureset_2_stylelist);
      }

    zMapConfigIniContextDestroy(context);
  }
}



/* Connect a View to its databases via threads, at this point the View is blank and waiting
 * to be called to load some data. */
gboolean zMapViewConnect(ZMapView zmap_view, char *config_str)
{
  gboolean result = TRUE ;
  char *stylesfile = NULL;

  if (zmap_view->state != ZMAPVIEW_INIT)        // && zmap_view->state != ZMAPVIEW_LOADED)

    {
      /* Probably we should indicate to caller what the problem was here....
       * e.g. if we are resetting then say we are resetting etc.....again we need g_error here. */
      zMapLogCritical("GUI: %s", "cannot connect because not in init state.") ;

      result = FALSE ;
    }
  else
    {
      GList *settings_list = NULL, *free_this_list = NULL;

      zMapStartTimer(ZMAP_GLOBAL_TIMER) ;
      zMapPrintTimer(NULL, "Open connection") ;

      zmapViewBusy(zmap_view, TRUE) ;

      /* There is some redundancy of state here as the below code actually does a connect
       * and load in one call but we will almost certainly need the extra states later... */
      zmap_view->state = ZMAPVIEW_CONNECTING ;

      settings_list = zmapViewGetIniSources(config_str,&stylesfile);    // get the stanza structs from ZMap config


        /* There are a number of predefined methods that we require so add these in as well
         * as the mapping for "feature set" -> style for these. */
      addPredefined(&(zmap_view->orig_styles), &(zmap_view->featureset_2_stylelist)) ;

      // read in a few ZMap stanzas
      zmapViewGetIniData(zmap_view, config_str, settings_list);

      if(zmap_view->columns_set)
      {
           g_list_foreach(zmap_view->window_list, invoke_merge_in_names, zmap_view->columns);
      }

      /* Set up connections to the named servers. */
      if (settings_list)
	{
	  ZMapConfigSource current_server = NULL;
	  int connections = 0 ;

	  free_this_list = settings_list ;



	  /* Current error handling policy is to connect to servers that we can and
	   * report errors for those where we fail but to carry on and set up the ZMap
	   * as long as at least one connection succeeds. */
	  do
	    {

	      ZMapViewConnection view_con ;

	      current_server = (ZMapConfigSource)settings_list->data ;
// if global            current_server->stylesfile = g_strdup(stylesfile);

            if(current_server->delayed)   // only request data when asked by otterlace
                  continue;


	      /* Check for required fields from config, if not there then we can't connect. */
              if (!current_server->url)
		{
		  /* url is absolutely required. Go on to next stanza if there isn't one. */
		  zMapWarning("%s", "No url specified in configuration file so cannot connect to server.") ;

		  zMapLogWarning("GUI: %s", "No url specified in config source group.") ;

		  continue ;

		}
	      else if (!(current_server->featuresets))
		{
		  /* featuresets are absolutely required, go on to next stanza if there aren't
		   * any. */
		  zMapWarning("Server \"%s\": no featuresets specified in configuration file so cannot connect.",
			      current_server->url) ;

		  zMapLogWarning("GUI: %s", "No featuresets specified in config source group.") ;

		  continue ;
		}



#ifdef NOT_REQUIRED_ATM
	      /* This will become redundant with step stuff..... */

	      else if (!checkSequenceToServerMatch(zmap_view->sequence_2_server, &tmp_seq))
		{
		  /* If certain sequences must only be fetched from certain servers then make sure
		   * we only make those connections. */
		  zMapLogMessage("server %s no sequence: ignored",current_server->url);
		  continue ;
		}
#endif /* NOT_REQUIRED_ATM */


		{
               GList *req_featuresets = NULL, *tmp_navigator_sets = NULL ;
               ZMapFeatureContext context;
               gboolean dna_requested = FALSE;

              /* req all featuresets  as a list of their quark names. */
              req_featuresets = zMapConfigString2QuarkList(current_server->featuresets) ;

              if(!zmap_view->columns_set)
              {
                  g_list_foreach(zmap_view->window_list, invoke_merge_in_names, req_featuresets);
              }

              /* Navigator styles are all predefined so no need to merge into main feature sets to get the styles. */
              if (current_server->navigatorsets)
              {
                tmp_navigator_sets = zmap_view->navigator_set_names = zMapConfigString2QuarkList(current_server->navigatorsets);
                    /* We _must_ merge the set names into the navigator though. */
                    /* The navigator knows nothing of view, so saving them there isn't really useful for getting them drawn.
                     * N.B. This is zMapWindowNavigatorMergeInFeatureSetNames _not_ zMapWindowMergeInFeatureSetNames. */
                if(zmap_view->navigator_window)
                    zMapWindowNavigatorMergeInFeatureSetNames(zmap_view->navigator_window, tmp_navigator_sets);
              }

              while(req_featuresets)
                {
                  // depending on config make one request for all or one for each
                  GList *con_featuresets = NULL;

                  if((current_server->group & SOURCE_GROUP_START))
                    {
                      con_featuresets = req_featuresets;          // all together
                      req_featuresets = NULL;                     // then finished
                    }
                  else
                    {
                       // one at a time
                       con_featuresets = g_list_append(con_featuresets,req_featuresets->data);
                       // until cooked
                       req_featuresets = req_featuresets->next;
                    }

                  /* Create data specific to this step list...and set it in the connection. */
                  context = createContext(zmap_view->sequence, zmap_view->start, zmap_view->end, con_featuresets) ;

                  /* Check whether DNA was requested, see comments below about setting up sequence req. */
                  dna_requested = (gboolean) zMap_g_list_find_quark(con_featuresets,
                         zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME));

	    	      if ((view_con = createConnection(zmap_view, NULL, context,
                                       current_server->url,
						   (char *)current_server->format,
						   current_server->timeout,
						   (char *)current_server->version,
						   (char *)current_server->styles_list,
						   current_server->stylesfile,
						   con_featuresets,
						   dna_requested,             // current_server->sequence,
                                       current_server->delayed )))  // has to be false or else we are not here
		        {
                      connections++ ;

                      /* If at least one connection succeeded then we are up and running, if not then the zmap
                       * returns to the init state. */
                      zmap_view->state = ZMAPVIEW_LOADING ;

                      /* THESE NEED TO GO WHEN STEP LIST STUFF IS DONE PROPERLY.... */
                      // this is an optimisation: the server supports DNA so no point in searching for it
                      // if we implement multiple sources then we can remove this
		          if (dna_requested)
			      zmap_view->sequence_server  = view_con ;
		        }
                }
		}


	    }
	  while ((settings_list = g_list_next(settings_list)));


	  /* Ought to return a gerror here........ */
	  if (!connections)
	    result = FALSE ;

	  zMapConfigSourcesFreeList(free_this_list);
	}
      else
	{
	  result = FALSE;
	}


      if (!result)
	{
	  zmap_view->state = ZMAPVIEW_INIT ;
	}


      /* Start polling function that checks state of this view and its connections,
       * this will wait until the connections reply, process their replies and call
       * zmapViewStepListIter() again.
       */
      startStateConnectionChecking(zmap_view) ;
    }
//  if(stylesfile)
//    g_free(stylesfile);

  return result ;
}



/* Copies an existing window in a view.
 * Returns the window on success, NULL on failure. */
ZMapViewWindow zMapViewCopyWindow(ZMapView zmap_view, GtkWidget *parent_widget,
				  ZMapWindow copy_window, ZMapWindowLockType window_locking)
{
  ZMapViewWindow view_window = NULL ;


  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      GData *copy_styles ;


      /* the view _must_ already have a window _and_ data. */
      zMapAssert(zmap_view);
      zMapAssert(parent_widget);
      zMapAssert(zmap_view->window_list);
      zMapAssert(zmap_view->state == ZMAPVIEW_LOADED) ;

      copy_styles = zmap_view->orig_styles ;

      view_window = createWindow(zmap_view, NULL) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zmapViewBusy(zmap_view, TRUE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      if (!(view_window->window = zMapWindowCopy(parent_widget, zmap_view->sequence,
						 view_window, copy_window,
						 zmap_view->features,
						 zmap_view->orig_styles, copy_styles,
						 window_locking)))
	{
	  /* should glog and/or gerror at this stage....really need g_errors.... */
	  /* should free view_window.... */

	  view_window = NULL ;
	}
      else
	{
	  /* add to list of windows.... */
	  zmap_view->window_list = g_list_append(zmap_view->window_list, view_window) ;

	}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zmapViewBusy(zmap_view, FALSE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  return view_window ;
}


/* Returns number of windows for current view. */
int zMapViewNumWindows(ZMapViewWindow view_window)
{
  int num_windows = 0 ;
  ZMapView zmap_view ;

  zMapAssert(view_window) ;

  zmap_view = view_window->parent_view ;

  if (zmap_view->state != ZMAPVIEW_DYING && zmap_view->window_list)
    {
      num_windows = g_list_length(zmap_view->window_list) ;
    }

  return num_windows ;
}


/* Removes a window from a view, the last window cannot be removed, the view must
 * always have at least one window. The function does nothing if there is only one
 * window. */
void zMapViewRemoveWindow(ZMapViewWindow view_window)
{
  ZMapView zmap_view ;

  zMapAssert(view_window) ;

  zmap_view = view_window->parent_view ;

  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      if (g_list_length(zmap_view->window_list) > 1)
	{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* I'm removing busy calls from view as it's messing things up.
	   * e.g. this should be a call to zmapviewbusy() !!! */

	  /* We should check the window is in the list of windows for that view and abort if
	   * its not........ */
	  zMapWindowBusy(view_window->window, TRUE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	  destroyWindow(zmap_view, view_window) ;

	  /* no need to reset cursor here...the window is gone... */
	}
    }

  return ;
}



/*!
 * Get the views "xremote" widget, returns NULL if view is dying.
 *
 * @param                The ZMap View
 * @return               NULL or views xremote widget.
 *  */
GtkWidget *zMapViewGetXremote(ZMapView view)
{
  GtkWidget *xremote_widget = NULL ;

  if (view->state != ZMAPVIEW_DYING)
    xremote_widget = view->xremote_widget ;

  return xremote_widget ;
}



/*!
 * Erases the supplied context from the view's context and instructs
 * the window to delete the old features.
 *
 * @param                The ZMap View
 * @param                The Context to erase.  Those features which
 *                       match will be removed from this context and
 *                       the view's own context. They will also be
 *                       removed from the display windows. Those that
 *                       don't match will be left in this context.
 * @return               void
 *************************************************** */
void zmapViewEraseFromContext(ZMapView replace_me, ZMapFeatureContext context_inout)
{
  if (replace_me->state != ZMAPVIEW_DYING)
    {
      /* should replace_me be a view or a view_window???? */
      eraseAndUndrawContext(replace_me, context_inout);
    }

  return;
}

/*!
 * Merges the supplied context with the view's context.
 *
 * @param                The ZMap View
 * @param                The Context to merge in.  This will be emptied
 *                       but needs destroying...
 * @return               The diff context.  This needs destroying.
 *************************************************** */
ZMapFeatureContext zmapViewMergeInContext(ZMapView view, ZMapFeatureContext context)
{

  /* called from zmapViewRemoteReceive.c, no styles are available. */

  if (view->state != ZMAPVIEW_DYING)
    justMergeContext(view, &context, NULL);

  return context;
}

/*!
 * Instructs the view to draw the diff context. The drawing will
 * happen sometime in the future, so we NULL the diff_context!
 *
 * @param               The ZMap View
 * @param               The Context to draw...
 *
 * @return              Boolean to notify whether the context was
 *                      free'd and now == NULL, FALSE only if
 *                      diff_context is the same context as view->features
 *************************************************** */
gboolean zmapViewDrawDiffContext(ZMapView view, ZMapFeatureContext *diff_context)
{
  gboolean context_freed = TRUE;

  /* called from zmapViewRemoteReceive.c, no styles are available. */

  if (view->state != ZMAPVIEW_DYING)
    {
      if(view->features == *diff_context)
        context_freed = FALSE;

      justDrawContext(view, *diff_context, NULL);
    }
  else
    {
      context_freed = TRUE;
      zMapFeatureContextDestroy(*diff_context, context_freed);
    }

  if(context_freed)
    *diff_context = NULL;

  return context_freed;
}

/* Force a redraw of all the windows in a view, may be reuqired if it looks like
 * drawing has got out of whack due to an overloaded network etc etc. */
void zMapViewRedraw(ZMapViewWindow view_window)
{
  ZMapView view ;
  GList* list_item ;

  view = zMapViewGetView(view_window) ;
  zMapAssert(view) ;

  if (view->state == ZMAPVIEW_LOADED)
    {
      list_item = g_list_first(view->window_list) ;
      do
	{
	  ZMapViewWindow view_window ;

	  view_window = list_item->data ;

	  zMapWindowRedraw(view_window->window) ;
	}
      while ((list_item = g_list_next(list_item))) ;
    }

  return ;
}


/* Show stats for this view. */
void zMapViewStats(ZMapViewWindow view_window,GString *text)
{
  ZMapView view ;
  GList* list_item ;

  view = zMapViewGetView(view_window) ;
  zMapAssert(view) ;

  if (view->state == ZMAPVIEW_LOADED)
    {
      ZMapViewWindow view_window ;

      list_item = g_list_first(view->window_list) ;
      view_window = list_item->data ;

      zMapWindowStats(view_window->window,text) ;
    }

  return ;
}


/* Reverse complement a view, this call will:
 *
 *    - leave the View window(s) displayed and hold onto user information such as machine/port
 *      sequence etc so user does not have to add the data again.
 *    - reverse complement the sequence context.
 *    - display the reversed context.
 *
 *  */
gboolean zMapViewReverseComplement(ZMapView zmap_view)
{
  gboolean result = FALSE ;

  if (zmap_view->state == ZMAPVIEW_LOADED)
    {
      GList* list_item ;

      zmapViewBusy(zmap_view, TRUE) ;

      /* Call the feature code that will do the revcomp. */
      zMapFeatureReverseComplement(zmap_view->features, zmap_view->orig_styles) ;

      /* Set our record of reverse complementing. */
      zmap_view->revcomped_features = !(zmap_view->revcomped_features) ;

      zMapWindowNavigatorReset(zmap_view->navigator_window);
      zMapWindowNavigatorSetStrand(zmap_view->navigator_window, zmap_view->revcomped_features);
      zMapWindowNavigatorDrawFeatures(zmap_view->navigator_window, zmap_view->features, zmap_view->orig_styles);

      if((list_item = g_list_first(zmap_view->window_list)))
	{
	  do
	    {
	      ZMapViewWindow view_window ;
	      GData *copy_styles = NULL;

	      view_window = list_item->data ;

	      copy_styles = zmap_view->orig_styles ;

	      zMapWindowFeatureRedraw(view_window->window, zmap_view->features,
				      zmap_view->orig_styles, copy_styles, TRUE) ;
	    }
	  while ((list_item = g_list_next(list_item))) ;
	}

      /* Not sure if we need to do this or not.... */
      /* signal our caller that we have data. */
      (*(view_cbs_G->load_data))(zmap_view, zmap_view->app_data, NULL) ;

      zmapViewBusy(zmap_view, FALSE);

      result = TRUE ;
    }

  return result ;
}

/* Return which strand we are showing viz-a-viz reverse complementing. */
gboolean zMapViewGetRevCompStatus(ZMapView zmap_view)
{
  return zmap_view->revcomped_features ;
}



/* Reset an existing view, this call will:
 *
 *    - leave the View window(s) displayed and hold onto user information such as machine/port
 *      sequence etc so user does not have to add the data again.
 *    - Free all ZMap window data that was specific to the view being loaded.
 *    - Kill the existing server thread(s).
 *
 * After this call the ZMap will be ready for the user to try again with the existing
 * machine/port/sequence or to enter data for a new sequence etc.
 *
 *  */
gboolean zMapViewReset(ZMapView zmap_view)
{
  gboolean result = FALSE ;

  if (zmap_view->state == ZMAPVIEW_CONNECTING || zmap_view->state == ZMAPVIEW_CONNECTED
      || zmap_view->state == ZMAPVIEW_LOADING || zmap_view->state == ZMAPVIEW_LOADED)
    {
      zmapViewBusy(zmap_view, TRUE) ;

      zmap_view->state = ZMAPVIEW_RESETTING ;

      /* Reset all the windows to blank. */
      resetWindows(zmap_view) ;

      /* We need to destroy the existing thread connection and wait until user loads a new
	 sequence. */
      killConnections(zmap_view) ;

      result = TRUE ;
    }

  return result ;
}


/*
 * If view_window is NULL all windows are zoomed.
 *
 *
 *  */
void zMapViewZoom(ZMapView zmap_view, ZMapViewWindow view_window, double zoom)
{
  if (zmap_view->state == ZMAPVIEW_LOADED)
    {
      if (view_window)
	zMapWindowZoom(zMapViewGetWindow(view_window), zoom) ;
      else
	{
	  GList* list_item ;

	  list_item = g_list_first(zmap_view->window_list) ;

	  do
	    {
	      ZMapViewWindow view_window ;

	      view_window = list_item->data ;

	      zMapWindowZoom(view_window->window, zoom) ;
	    }
	  while ((list_item = g_list_next(list_item))) ;
	}
    }

  return ;
}

void zMapViewHighlightFeatures(ZMapView view, ZMapViewWindow view_window, ZMapFeatureContext context, gboolean multiple)
{
  GList *list;

  if (view->state == ZMAPVIEW_LOADED)
    {
      if (view_window)
	{
	  zMapLogWarning("%s", "What were you thinking");
	}
      else
	{
	  list = g_list_first(view->window_list);
	  do
	    {
	      view_window = list->data;
	      zMapWindowHighlightObjects(view_window->window, context, multiple);
	    }
	  while((list = g_list_next(list)));
	}
    }

  return ;
}


/*
 *    A set of accessor functions.
 */
char *zMapViewGetSequence(ZMapView zmap_view)
{
  char *sequence = NULL ;

  if (zmap_view->state != ZMAPVIEW_DYING)
    sequence = zmap_view->sequence ;

  return sequence ;
}


void zMapViewGetSourceNameTitle(ZMapView zmap_view, char **name, char **title)
{

  if (zmap_view->view_db_name)
    *name = zmap_view->view_db_name ;

  if (zmap_view->view_db_title)
    *title = zmap_view->view_db_title ;

  return ;
}


ZMapFeatureContext zMapViewGetFeatures(ZMapView zmap_view)
{
  ZMapFeatureContext features = NULL ;

  if (zmap_view->state != ZMAPVIEW_DYING && zmap_view->features)
    features = zmap_view->features ;

  return features ;
}

GData *zMapViewGetStyles(ZMapViewWindow view_window)
{
  GData *styles = NULL ;
  ZMapView view = zMapViewGetView(view_window);

  if (view->state != ZMAPVIEW_DYING)
    styles = view->orig_styles ;

  return styles;
}

gboolean zMapViewGetFeaturesSpan(ZMapView zmap_view, int *start, int *end)
{
  gboolean result = FALSE ;

  if (zmap_view->state != ZMAPVIEW_DYING && zmap_view->features)
    {
      *start = zmap_view->features->sequence_to_parent.c1 ;
      *end = zmap_view->features->sequence_to_parent.c2 ;

      result = TRUE ;
    }

  return result ;
}


/* N.B. we don't exclude ZMAPVIEW_DYING because caller may want to know that ! */
ZMapViewState zMapViewGetStatus(ZMapView zmap_view)
{
  return zmap_view->state ;
}

char *zMapViewGetStatusStr(ZMapViewState state)
{
  /* Array must be kept in synch with ZmapState enum in zmapView.h */
  static char *zmapStates[] = {"",
			       "Connecting", "Connected",
			       "Data loading", "Data loaded","Columns loading",
			       "Resetting", "Dying"} ;
  char *state_str ;

  zMapAssert(state >= ZMAPVIEW_INIT);
  zMapAssert(state <= ZMAPVIEW_DYING) ;

  state_str = zmapStates[state] ;

  return state_str ;
}


ZMapWindow zMapViewGetWindow(ZMapViewWindow view_window)
{
  ZMapWindow window = NULL ;

  zMapAssert(view_window) ;

  if (view_window->parent_view->state != ZMAPVIEW_DYING)
    window = view_window->window ;

  return window ;
}

ZMapWindowNavigator zMapViewGetNavigator(ZMapView view)
{
  ZMapWindowNavigator navigator = NULL ;

  zMapAssert(view) ;

  if (view->state != ZMAPVIEW_DYING)
    navigator = view->navigator_window ;

  return navigator ;
}


GList *zMapViewGetWindowList(ZMapViewWindow view_window)
{
  zMapAssert(view_window);

  return view_window->parent_view->window_list;
}


void zMapViewSetWindowList(ZMapViewWindow view_window, GList *list)
{
  zMapAssert(view_window);
  zMapAssert(list);

  view_window->parent_view->window_list = list;

  return;
}


ZMapView zMapViewGetView(ZMapViewWindow view_window)
{
  ZMapView view = NULL ;

  zMapAssert(view_window) ;

  if (view_window->parent_view->state != ZMAPVIEW_DYING)
    view = view_window->parent_view ;

  return view ;
}

void zMapViewReadConfigBuffer(ZMapView zmap_view, char *buffer)
{
  /* designed to add extra config bits to an already created view... */
  /* This is probably a bit of a hack, why can't we make the zMapViewConnect do it?  */
#ifdef NOT_REQUIRED_ATM
  getSequenceServers(zmap_view, buffer);
#endif /* NOT_REQUIRED_ATM */
  return ;
}

void zmapViewFeatureDump(ZMapViewWindow view_window, char *file)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMapFeatureDump(view_window->parent_view->features, file) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  printf("reimplement.......\n") ;

  return;
}


/* Called to kill a view and get the associated threads killed, note that
 * this call just signals everything to die, its the checkConnections() routine
 * that really clears up and when everything has died signals the caller via the
 * callback routine that they supplied when the view was created.
 *
 * NOTE: if the function returns FALSE it means the view has signalled its threads
 * and is waiting for them to die, the caller should thus wait until view signals
 * via the killedcallback that the view has really died before doing final clear up.
 *
 * If the function returns TRUE it means that the view has been killed immediately
 * because it had no threads so the caller can clear up immediately.
 */
void zMapViewDestroy(ZMapView zmap_view)
{

  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      zmapViewBusy(zmap_view, TRUE) ;

      /* All states have GUI components which need to be destroyed. */
      killGUI(zmap_view) ;

      if (zmap_view->state == ZMAPVIEW_INIT)
	{
	  /* For init we simply need to signal to our parent layer that we have died,
	   * we will then be cleaned up immediately. */

	  zmap_view->state = ZMAPVIEW_DYING ;
	}
      else
	{
	  /* for other states there are threads to kill so they must be cleaned
	   * up asynchronously. */

	  if (zmap_view->state != ZMAPVIEW_RESETTING)
	    {
	      /* If we are resetting then the connections have already being killed. */
	      killConnections(zmap_view) ;
	    }

	  /* Must set this as this will prevent any further interaction with the ZMap as
	   * a result of both the ZMap window and the threads dying asynchronously.  */
	  zmap_view->state = ZMAPVIEW_DYING ;
	}
    }

  return ;
}


/*! @} end of zmapview docs. */




/*
 *             Functions internal to zmapView package.
 */



char *zmapViewGetStatusAsStr(ZMapViewState state)
{
  /* Array must be kept in synch with ZmapState enum in ZMap.h */
  static char *zmapStates[] = {"ZMAPVIEW_INIT",
//			       "ZMAPVIEW_NOT_CONNECTED", "ZMAPVIEW_NO_WINDOW",
			       "ZMAPVIEW_CONNECTING", "ZMAPVIEW_CONNECTED",
			       "ZMAPVIEW_LOADING", "ZMAPVIEW_LOADED", "ZMAPVIEW_UPDATING",
			       "ZMAPVIEW_RESETTING", "ZMAPVIEW_DYING"} ;
  char *state_str ;

  zMapAssert(state >= ZMAPVIEW_INIT);
  zMapAssert(state <= ZMAPVIEW_DYING) ;

  state_str = zmapStates[state] ;

  return state_str ;
}




static GList *zmapViewGetIniSources(char *config_str,char ** stylesfile)
{
     ZMapConfigIniContext context ;
      GList *settings_list = NULL;

      if ((context = zMapConfigIniContextProvide()))
      {

        if(config_str)
            zMapConfigIniContextIncludeBuffer(context, config_str);

        settings_list = zMapConfigIniContextGetSources(context);
#if MH17_NOT_NEEDED
// now specified per server
        if(stylesfile)
        {
            zMapConfigIniContextGetString(context,
                              ZMAPSTANZA_APP_CONFIG,ZMAPSTANZA_APP_CONFIG,
                              ZMAPSTANZA_APP_STYLESFILE,stylesfile);
        }
#endif
        zMapConfigIniContextDestroy(context);

      }

      return(settings_list);
}


// create a hash table of feature set names and thier sources
static GHashTable *zmapViewGetFeatureSourceHash(GList *sources)
{
  GHashTable *hash = NULL;
  ZMapConfigSource src;
  gchar **features,**feats;

  hash = g_hash_table_new(NULL,NULL);

  // for each source extract featuresets and add a hash to the source
  for(;sources; sources = g_list_next(sources))
    {
      src = sources->data;
      if(!src->featuresets)
            continue;
      features = g_strsplit(src->featuresets,";",0); // this will give null entries eg 'aaa ; bbbb' -> 5 strings
      if(!features)
            continue;
      for(feats = features;*feats;feats++)
        {
          GQuark q;
          // the data we want to lookup happens to have been quarked
          if(**feats)
          {
            gchar *feature;

            feature = zMapConfigNormaliseWhitespace(*feats);
            if(!feature)
                  continue;
            q =  zMapFeatureSetCreateID(feature);
            g_hash_table_insert(hash,GUINT_TO_POINTER(q), (gpointer) src);
          }
        }

      g_strfreev(features);
    }

  return(hash);
}

#define zmapViewGetSourceFromFeatureset(hash,featurequark)   (ZMapConfigSource) g_hash_table_lookup(hash,(gpointer) featurequark)






/* Loads features within block from the sets req_featuresets that lie within features_start
 * to features_end. The features are fetched from the data sources and added to the existing
 * view. N.B. this is asynchronous because the sources are separate threads and once
 * retrieved the features are added via a gtk event. */
 /* mh17:
  * unlike zmapViewConnect() we request one feature set at a time from the relevant server
  * we scan the config file and create a hash table linking feature set to source
  */
void zmapViewLoadFeatures(ZMapView view, ZMapFeatureBlock block_orig, GList *req_sources,
			  int features_start, int features_end)
{
  ZMapFeatureContext context ;
  ZMapFeatureBlock block ;
  GHashTable *hash = NULL;
  GList * sources = NULL;
  ZMapConfigSource server;
  char *stylesfile = NULL;

  gboolean requested = FALSE;

  sources = zmapViewGetIniSources(NULL,&stylesfile);
  hash = zmapViewGetFeatureSourceHash(sources);

  for(;req_sources;req_sources = g_list_next(req_sources))
    {
      GQuark featureset = GPOINTER_TO_UINT(req_sources->data);

      if(view->featureset_2_column)
	{
	  ZMapGFFSet GFFset = NULL;

	  GFFset = g_hash_table_lookup(view->featureset_2_column, GUINT_TO_POINTER(featureset)) ;
	  if(GFFset)
	    featureset = GFFset->feature_set_id;
	}
      server = zmapViewGetSourceFromFeatureset(hash,featureset);
// if global            server->stylesfile = g_strdup(stylesfile);
//zMapLogMessage("Load features %s from %s, group = %d\n", g_quark_to_string(featureset),server->url,server->group);
      if(server)
	{
	  GList *req_featuresets = NULL;
	  int existing = FALSE;
	  ZMapViewConnection view_conn = NULL;

            // make a list of one feature only
        req_featuresets = g_list_append(req_featuresets,GUINT_TO_POINTER(featureset));

        if((server->group & SOURCE_GROUP_DELAYED))
          {
            // get all featuresets from this source and remove from req_sources
            GList *req_src;
            GQuark fset;
            ZMapConfigSource fset_server;

            for(req_src = req_sources->next;req_src;)
              {
                fset = GPOINTER_TO_UINT(req_src->data);
//zMapLogMessage("add %s\n",g_quark_to_string(fset));
                if(view->featureset_2_column)
                  {
                    ZMapGFFSet GFFset = NULL;

                    GFFset = g_hash_table_lookup(view->featureset_2_column, GUINT_TO_POINTER(fset)) ;
                    if(GFFset)
                      fset = GFFset->feature_set_id;
                  }
                fset_server = zmapViewGetSourceFromFeatureset(hash,fset);

//if(fset_server) zMapLogMessage("Try %s\n",fset_server->url);
                if(fset_server == server)
                  {
                     GList *del;

                        // prepend faster than append...we don't care about the order
                     req_featuresets = g_list_prepend(req_featuresets,GUINT_TO_POINTER(fset));

                     // avoid getting ->next from deleted item
                     del = req_src;
                     req_src = req_src->next;
//zMapLogMessage("use %s\n",g_quark_to_string(fset));

                     // as req_src is ->next of req_sources we know req_sources is still valid
                     // even though we are removing an item from it
                     // However we still get a retval from g_list remove()
                     req_sources = g_list_remove_link(req_sources,del);
// where else is this held: crashes
//NB could use delete link if that was ok
//                     g_free(del); // free the link; no data to free
                  }
                else
                  {
                     req_src = req_src->next;
                  }
              }
          }

           // look for server in view->connections list

        GList *view_con_list;
	  for(view_con_list = view->connection_list;view_con_list;view_con_list = g_list_next(view_con_list))
          {
	      view_conn = (ZMapViewConnection) view_con_list->data;
	      if(!strcmp(view_conn->url,server->url))
		{
		  existing = TRUE;
		  break;
		}
          }


	  /* Copy the original context from the target block upwards setting feature set names
	   * and the range of features to be copied.
	   * We need one for each featureset/ request
	   */
#if 1
	  // using this as it may be necessary for Blixem ?
	  context = zMapFeatureContextCopyWithParents((ZMapFeatureAny)block_orig) ;
	  context->feature_set_names = req_featuresets ;

	  block = zMapFeatureAlignmentGetBlockByID(context->master_align, block_orig->unique_id) ;
	  zMapFeatureBlockSetFeaturesCoords(block, features_start, features_end) ;

#else
	  /*
	    try the exact stuff fron startup that works
	    it's a lot simpler and fails in exactly the same way
	  */
	  context = createContext(view->sequence, view->start, view->end, req_featuresets) ;
#endif
	  // make the windows have the same list of featuresets so that they display
        if(!view->columns_set)
            g_list_foreach(view->window_list, invoke_merge_in_names, req_featuresets);


	  //printf("request featureset %s from %s\n",g_quark_to_string(GPOINTER_TO_UINT(req_featuresets->data)),server->url);

	  // start a new server connection
	  // can optionally use an existing one -> pass in second arg
	  if(createConnection(view, existing ? view_conn : NULL,
			      context, server->url,
			      (char *)server->format,
			      server->timeout,
			      (char *)server->version,
			      (char *)server->styles_list,
			      server->stylesfile,
			      req_featuresets,
			      FALSE,
			      !existing ))
	    {
	      requested = TRUE;
	    }
	  // g_list_free(req_featuresets); no! this list gets used by threads
	  req_featuresets = NULL;
	}
    }

  if(requested && view->state != ZMAPVIEW_UPDATING)
    {
      zmapViewBusy(view, TRUE) ;     // gets unset when all step lists finish
      view->state = ZMAPVIEW_UPDATING;
      (*(view_cbs_G->state_change))(view, view->app_data, NULL) ;
    }

//  if(stylesfile)
//    g_free(stylesfile);

  if(sources)
    zMapConfigSourcesFreeList(sources);
  if(hash)
    g_hash_table_destroy(hash);

  return ;
}



/*
 *  ------------------- Internal functions -------------------
 */



/* We could provide an "executive" kill call which just left the threads dangling, a kind
 * of "kill -9" style thing, it would actually kill/delete stuff without waiting for threads
 * to die....or we could allow a "force" flag to zmapViewKill/Destroy  */


/* This is really the guts of the code to check what a connection thread is up
 * to. Every time the GUI thread has stopped doing things GTK calls this routine
 * which then checks our connections for responses from the threads...... */
static gint zmapIdleCB(gpointer cb_data)
{
  gint call_again = 0 ;
  ZMapView zmap_view = (ZMapView)cb_data ;


  /* Returning a value > 0 tells gtk to call zmapIdleCB again, so if checkConnections() returns
   * TRUE we ask to be called again. */
  if (checkStateConnections(zmap_view))
    call_again = 1 ;
  else
    call_again = 0 ;

  return call_again ;
}



static void enterCB(ZMapWindow window, void *caller_data, void *window_data)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Not currently used because we are doing "click to focus" at the moment. */

  /* Pass back a ZMapViewWindow as it has both the View and the window. */
  (*(view_cbs_G->enter))(view_window, view_window->parent_view->app_data, NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}


static void leaveCB(ZMapWindow window, void *caller_data, void *window_data)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Not currently used because we are doing "click to focus" at the moment. */

  /* Pass back a ZMapViewWindow as it has both the View and the window. */
  (*(view_cbs_G->leave))(view_window, view_window->parent_view->app_data, NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}


static void scrollCB(ZMapWindow window, void *caller_data, void *window_data)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  printf("In View, in window scroll callback\n") ;

//
  return ;
}


/* Called when a sequence window has been focussed, usually by user actions. */
static void viewFocusCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;

  /* Pass back a ZMapViewWindow as it has both the View and the window. */
  (*(view_cbs_G->focus))(view_window, view_window->parent_view->app_data, NULL) ;

  {
    zMapWindowNavigatorFocus(view_window->parent_view->navigator_window, TRUE);
  }

  return ;
}


/* Called when some sequence window feature (e.g. column, actual feature etc.)
 * has been selected. */
static void viewSelectCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
  ZMapWindowSelect window_select = (ZMapWindowSelect)window_data ;
  ZMapViewSelectStruct view_select = {0} ;

  /* Check we've got a window_select! */
  if(!window_select)
    return ;                    /* !!! RETURN !!! */

  if((view_select.type = window_select->type) == ZMAPWINDOW_SELECT_SINGLE)
    {
      if (window_select->highlight_item)
	{
	  GList* list_item ;

	  /* Highlight the feature in all windows, BUT note that we may not find it in some
	   * windows, e.g. if a window has been reverse complemented the item may be hidden. */
	  list_item = g_list_first(view_window->parent_view->window_list) ;

	  do
	    {
	      ZMapViewWindow view_window ;
	      FooCanvasItem *item ;

	      view_window = list_item->data ;

	      if ((item = zMapWindowFindFeatureItemByItem(view_window->window, window_select->highlight_item)))
		zMapWindowHighlightObject(view_window->window, item,
					  window_select->replace_highlight_item,
					  window_select->highlight_same_names) ;
	    }
	  while ((list_item = g_list_next(list_item))) ;
	}


      view_select.feature_desc   = window_select->feature_desc ;
      view_select.secondary_text = window_select->secondary_text ;

    }


  view_select.xml_handler = window_select->xml_handler ;    /* n.b. struct copy. */

  if(window_select->xml_handler.zmap_action)
    {
      view_select.xml_handler.handled =
        window_select->xml_handler.handled = zmapViewRemoteSendCommand(view_window->parent_view,
                                                                       window_select->xml_handler.zmap_action,
                                                                       window_select->xml_handler.xml_events,
                                                                       window_select->xml_handler.start_handlers,
                                                                       window_select->xml_handler.end_handlers,
                                                                       window_select->xml_handler.handler_data);
    }


  /* Pass back a ZMapViewWindow as it has both the View and the window to our caller. */
  (*(view_cbs_G->select))(view_window, view_window->parent_view->app_data, &view_select) ;


  window_select->xml_handler.handled = view_select.xml_handler.handled;

  return ;
}



static void viewSplitToPatternCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data;
  ZMapWindowSplitting split  = (ZMapWindowSplitting)window_data;
  ZMapViewSplittingStruct view_split = {0};

  view_split.split_patterns      = split->split_patterns;
  view_split.touched_window_list = NULL;

  view_split.touched_window_list = g_list_append(view_split.touched_window_list, view_window);

  (*(view_cbs_G->split_to_pattern))(view_window, view_window->parent_view->app_data, &view_split);

  /* foreach window find feature and Do something according to pattern */
  split->window_index = 0;
  g_list_foreach(view_split.touched_window_list, splitMagic, window_data);

  /* clean up the list */
  g_list_free(view_split.touched_window_list);

  return ;
}

static void splitMagic(gpointer data, gpointer user_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)data;
  ZMapWindowSplitting  split = (ZMapWindowSplitting)user_data;
  ZMapSplitPattern   pattern = NULL;

  if(view_window->window == split->original_window)
    {
      printf("Ignoring original window for now."
             "  I may well revisit this, but I"
             " think we should leave it alone ATM.\n");
      return ;                    /* really return from this */
    }

  if((pattern = &(g_array_index(split->split_patterns, ZMapSplitPatternStruct, split->window_index))))
    {
      printf("Trying pattern %d\n", split->window_index);
      /* Do it here! */
    }

  (split->window_index)++;

  return ;
}

static ZMapView createZMapView(GtkWidget *xremote_widget, char *view_name, GList *sequences, void *app_data)
{
  ZMapView zmap_view = NULL ;
  GList *first ;
  ZMapViewSequenceMap master_seq ;

  first = g_list_first(sequences) ;
  master_seq = (ZMapViewSequenceMap)(first->data) ;

  zmap_view = g_new0(ZMapViewStruct, 1) ;

  zmap_view->state = ZMAPVIEW_INIT ;
  zmap_view->busy = FALSE ;

  zmap_view->xremote_widget = xremote_widget ;

  /* Only after map-event are we guaranteed that there's a window for us to work with. */
  zmap_view->map_event_handler = g_signal_connect(G_OBJECT(zmap_view->xremote_widget), "map-event",
						  G_CALLBACK(mapEventCB), (gpointer)zmap_view) ;

  zmapViewSetupXRemote(zmap_view, xremote_widget);

  zmap_view->view_name = g_strdup(view_name) ;

  /* TEMP CODE...UNTIL I GET THE MULTIPLE SEQUENCES IN ONE VIEW SORTED OUT..... */
  /* TOTAL HACK UP MESS.... */
  zmap_view->sequence = g_strdup(master_seq->sequence) ;
  zmap_view->start = master_seq->start ;
  zmap_view->end = master_seq->end ;

#ifdef NOT_REQUIRED_ATM
  /* TOTAL LASH UP FOR NOW..... */
  if (!(zmap_view->sequence_2_server))
    {
      getSequenceServers(zmap_view, NULL) ;
    }
#endif

  /* Set the regions we want to display. */
  zmap_view->sequence_mapping = sequences ;

  zmap_view->window_list = zmap_view->connection_list = NULL ;

  zmap_view->app_data = app_data ;

  zmap_view->revcomped_features = FALSE ;

  zmap_view->kill_blixems = TRUE ;

  zmap_view->session_data = g_new0(ZMapViewSessionStruct, 1) ;

  zmap_view->session_data->sequence = zmap_view->sequence ;


  return zmap_view ;
}


/* Adds a window to a view. */
static ZMapViewWindow addWindow(ZMapView zmap_view, GtkWidget *parent_widget)
{
  ZMapViewWindow view_window = NULL ;
  ZMapWindow window ;

  view_window = createWindow(zmap_view, NULL) ;

  /* There are no steps where this can fail at the moment. */
  window = zMapWindowCreate(parent_widget, zmap_view->sequence, view_window, NULL) ;
  zMapAssert(window) ;

  view_window->window = window ;

  /* add to list of windows.... */
  zmap_view->window_list = g_list_append(zmap_view->window_list, view_window) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* This seems redundant now....old code ???? */

  /* There is a bit of a hole in the "busy" state here in that we do the windowcreate
   * but have not signalled that we are busy, I suppose we could do the busy stuff twice,
   * once before the windowCreate and once here to make sure we set cursors everywhere.. */
  zmapViewBusy(zmap_view, TRUE) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Start polling function that checks state of this view and its connections, note
   * that at this stage there is no data to display. */
  startStateConnectionChecking(zmap_view) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  zmapViewBusy(zmap_view, FALSE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return view_window ;
}

static void killAllSpawned(ZMapView zmap_view)
{
  GPid pid;
  GList *processes = zmap_view->spawned_processes;

  if (zmap_view->kill_blixems)
    {
      while (processes)
	{
	  pid = GPOINTER_TO_INT(processes->data);
	  g_spawn_close_pid(pid);
	  kill(pid, 9);
	  processes = processes->next;
	}
    }

  if (zmap_view->spawned_processes)
    {
      g_list_free(zmap_view->spawned_processes);
      zmap_view->spawned_processes = NULL ;
    }

  return ;
}

/* Should only do this after the window and all threads have gone as this is our only handle
 * to these resources. The lists of windows and thread connections are dealt with somewhat
 * asynchronously by killAllWindows() & checkConnections() */
static void destroyZMapView(ZMapView *zmap_view_out)
{
  ZMapView zmap_view = *zmap_view_out ;

  g_free(zmap_view->sequence) ;

#ifdef NOT_REQUIRED_ATM
  if (zmap_view->sequence_2_server)
    {
      g_list_foreach(zmap_view->sequence_2_server, destroySeq2ServerCB, NULL) ;
      g_list_free(zmap_view->sequence_2_server) ;
      zmap_view->sequence_2_server = NULL ;
    }
#endif /* NOT_REQUIRED_ATM */

  if (zmap_view->cwh_hash)
    zmapViewCWHDestroy(&(zmap_view->cwh_hash));

  if (zmap_view->featureset_2_stylelist)
    zMap_g_hashlist_destroy(zmap_view->featureset_2_stylelist) ;

  if (zmap_view->session_data)
    {
      if (zmap_view->session_data->servers)
	{
	  g_list_foreach(zmap_view->session_data->servers, zmapViewSessionFreeServer, NULL) ;
	  g_list_free(zmap_view->session_data->servers) ;
	}

      g_free(zmap_view->session_data) ;
    }


  killAllSpawned(zmap_view);

  g_free(zmap_view) ;

  *zmap_view_out = NULL ;

  return ;
}




/*
 *       Connection control functions, interface to the data fetching threads.
 */


/* Start the ZMapView GTK idle function (gets run when the GUI is doing nothing).
 */
static void startStateConnectionChecking(ZMapView zmap_view)
{

#ifdef UTILISE_ALL_CPU_ON_DESKPRO203
  zmap_view->idle_handle = gtk_idle_add(zmapIdleCB, (gpointer)zmap_view) ;
#endif /* UTILISE_ALL_CPU_ON_DESKPRO203 */

  zmap_view->idle_handle = gtk_timeout_add(100, zmapIdleCB, (gpointer)zmap_view) ;
  // WARNING: gtk_timeout_add is deprecated and should not be used in newly-written code. Use g_timeout_add() instead.

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* I think that probably I won't need this most of the time, it could be used to remove the idle
 * function in a kind of unilateral way as a last resort, otherwise the idle function needs
 * to cancel itself.... */
static void stopStateConnectionChecking(ZMapView zmap_view)
{
  gtk_timeout_remove(zmap_view->idle_handle) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





/* This function checks the status of the connection and checks for any reply and
 * then acts on it, it gets called from the ZMap idle function.
 * If all threads are ok and zmap has not been killed then routine returns TRUE
 * meaning it wants to be called again, otherwise FALSE.
 *
 * The function monitors the View state so that when the last connection has disappeared
 * and the View is dying then the View is cleaned up and the caller gets called to say
 * the View is now dead.
 *
 * NOTE that you cannot use a condvar here, if the connection thread signals us using a
 * condvar we will probably miss it, that just doesn't work, we have to pole for changes
 * and this is possible because this routine is called from the idle function of the GUI.
 *
 *  */
static gboolean checkStateConnections(ZMapView zmap_view)
{
  gboolean call_again = TRUE ;	      /* Normally we want to be called continuously. */
  gboolean state_change = TRUE ;          /* Has view state changed ?. */
  gboolean reqs_finished = FALSE;         // at least one thread just finished
  int thread_status = 0;                  // current thread: -ve for fail 0 for pending  +ve for ok
  int has_step_list = 0;                  // any requests still active?

  if (zmap_view->connection_list)
    {
      GList *list_item ;

      list_item = g_list_first(zmap_view->connection_list) ;

      do
	{
	  ZMapViewConnection view_con ;
	  ZMapThread thread ;
	  ZMapThreadReply reply ;
	  void *data = NULL ;
	  char *err_msg = NULL ;
	  gboolean thread_has_died = FALSE ;
        ConnectionData cd;
        LoadFeaturesDataStruct lfd;

	  view_con = list_item->data ;
	  thread = view_con->thread ;
        cd = (ConnectionData) view_con->request_data;

	  /* NOTE HOW THE FACT THAT WE KNOW NOTHING ABOUT WHERE THIS DATA CAME FROM
	   * MEANS THAT WE SHOULD BE PASSING A HEADER WITH THE DATA SO WE CAN SAY WHERE
	   * THE INFORMATION CAME FROM AND WHAT SORT OF REQUEST IT WAS.....ACTUALLY WE
	   * GET A LOT OF INFO FROM THE CONNECTION ITSELF, E.G. SERVER NAME ETC. */

	  data = NULL ;
	  err_msg = NULL ;
        thread_status = 0;

        // need to copy this info in case of thread death which clears it up

        if(cd)
          {
            lfd.feature_sets = cd->feature_sets;
            lfd.xwid = zmap_view->xwid;
          }

	  if (!(zMapThreadGetReplyWithData(thread, &reply, &data, &err_msg)))
	    {
	      /* We assume that something bad has happened to the connection and remove it
	       * if we can't read the reply. */

	      threadDebugMsg(thread, "GUI: thread %s, cannot access reply from server thread - %s\n", err_msg) ;

	      /* Warn the user ! */
	      zMapWarning("Source \"%s\" is being removed, check log for details.", view_con->url) ;

	      zMapLogCritical("Source \"%s\", cannot access reply from server thread,"
			      " error was: %s", view_con->url, err_msg) ;

	      thread_has_died = TRUE ;
	    }
	  else
	    {
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
            { char fred[32]; sprintf(fred,"%d",reply);
	      threadDebugMsg(thread, "GUI: thread %s, thread reply = %d\n",fred) ;
            }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      switch (reply)
		{
		case ZMAPTHREAD_REPLY_WAIT:
		  {
		    state_change = FALSE ;

		    break ;
		  }
		case ZMAPTHREAD_REPLY_GOTDATA:
		case ZMAPTHREAD_REPLY_REQERROR:
		  {
		    ZMapServerReqAny req_any ;
		    ZMapViewConnectionRequest request ;
		    ZMapViewConnectionStep step ;
		    gboolean kill_connection = FALSE ;

		    view_con->curr_request = ZMAPTHREAD_REQUEST_WAIT ;

		    /* Recover the request from the thread data. */
		    req_any = (ZMapServerReqAny)data ;

		    /* Recover the stepRequest from the view connection and process the data from
		     * the request. */
		    if (!(request = zmapViewStepListFindRequest(view_con->step_list, req_any->type, view_con)))
		      {
			zMapLogCritical("Request of type %s for connection %s not found in view %s step list !",
					zMapServerReqType2ExactStr(req_any->type),
					view_con->url,
					zmap_view->view_name) ;

			kill_connection = TRUE ;
		      }
		    else
		      {
			step = (ZMapViewConnectionStep) view_con->step_list->current->data;      //request->step ;

			if (reply == ZMAPTHREAD_REPLY_REQERROR)
			  {
			    /* This means the request failed for some reason. */
			    threadDebugMsg(thread, "GUI: thread %s, request to server failed....\n", NULL) ;

			    if (err_msg)
			      {
				char *format_str =
				  "Source \"%s\" has returned an error, request that failed was: %s" ;

				zMapWarning(format_str, view_con->url, err_msg) ;

				zMapLogCritical(format_str, view_con->url, err_msg) ;
			      }

			  }
			else
			  {
			    // threadDebugMsg(thread, "GUI: thread %s, got data\n", NULL) ;

			    if (zmap_view->state != ZMAPVIEW_LOADING && zmap_view->state != ZMAPVIEW_UPDATING)
			      {
				threadDebugMsg(thread, "GUI: thread %s, got data but ZMap state is - %s\n",
					       zmapViewGetStatusAsStr(zMapViewGetStatus(zmap_view))) ;
			      }

			    zmapViewStepListStepProcessRequest(view_con, request) ;
			    if(request->state != STEPLIST_FINISHED)    // ie there was an error
			      reply = ZMAPTHREAD_REPLY_REQERROR;
 			  }
		      }

		    if (reply == ZMAPTHREAD_REPLY_REQERROR)
		      {
			if (step->on_fail == REQUEST_ONFAIL_CANCEL_THREAD
			    || step->on_fail == REQUEST_ONFAIL_CANCEL_STEPLIST)
			  {
			    /* Remove request from all steps.... */
			    zmapViewStepListDestroy(view_con) ;
			  }
			if (step->on_fail == REQUEST_ONFAIL_CANCEL_THREAD)
			  kill_connection = TRUE ;
		      }

		    if (kill_connection)
		      {
			/* Warn the user ! */
			zMapWarning("Source \"%s\" is being cancelled, check log for details.", view_con->url) ;

			zMapLogCritical("Source \"%s\" is being cancelled"//
					" because a request to it has failed,"
					" error was: %s", view_con->url, err_msg) ;

			/* Signal thread to die. */
			zMapThreadKill(thread) ;
		      }

		    /* Reset the reply from the slave. */
		    zMapThreadSetReply(thread, ZMAPTHREAD_REPLY_WAIT) ;

		    break ;
		  }
		case ZMAPTHREAD_REPLY_DIED:
		  {
		    /* Thread has failed for some reason and we should clean up. */
		    if (err_msg)
		      zMapWarning("%s", err_msg) ;

		    thread_has_died = TRUE ;

		    threadDebugMsg(thread, "GUI: thread %s has died so cleaning up....\n", NULL) ;

		    break ;
		  }
		case ZMAPTHREAD_REPLY_CANCELLED:
		  {
		    /* This happens when we have signalled the threads to die and they are
		     * replying to say that they have now died. */
		    thread_has_died = TRUE ;

		    /* This means the thread was cancelled so we should clean up..... */
		    threadDebugMsg(thread, "GUI: thread %s has been cancelled so cleaning up....\n", NULL) ;

		    break ;
		  }
            case ZMAPTHREAD_REPLY_QUIT:
              {
                thread_has_died = TRUE;

                threadDebugMsg(thread, "GUI: thread %s has quit so cleaning up....\n", NULL) ;

                break;
              }

		default:
		  {
		    zMapLogFatalLogicErr("switch(), unknown value: %d", reply) ;

		    break ;
		  }
		}

	    }

        if(!thread_has_died && !zMapThreadExists(thread))
          {
            thread_has_died = TRUE;
            // message to differ from REPLY_DIED above
            // it really is sudden death, thread is just not there
            threadDebugMsg(thread, "GUI: thread %s has died suddenly so cleaning up....\n", NULL) ;
          }

	  /* If the thread has died then remove it's connection. */
	  // do this before counting up the number of step lists
	  if (thread_has_died)
	    {
	      /* We are going to remove an item from the list so better move on from
	       * this item. */

	      list_item = g_list_next(list_item) ;
	      zmap_view->connection_list = g_list_remove(zmap_view->connection_list, view_con) ;

	      if(view_con->step_list)
		reqs_finished = TRUE;
            thread_status = -1;
	      destroyConnection(zmap_view,view_con) ;  //NB frees up what cd points to  (view_com->request_data)

	    }

            /* Check for more connection steps and dispatch them or clear up if finished. */
	  if ((view_con->step_list))
	    {
	      /* If there were errors then all connections may have been removed from
	       * step list or if we have finished then destroy step_list. */
	      if (zmapViewStepListIsNext(view_con->step_list))
		{

		  zmapViewStepListIter(view_con) ;
		  has_step_list++;
		}
	      else
		{
                  zmapViewStepListDestroy(view_con) ;
                  reqs_finished = TRUE;
                  thread_status = 1;
		      //zMapLogMessage("step list %s finished\n",view_con->url);
		}
	    }

        if(thread_status)     // tell otterlace
          {
            if(cd)      // ie was valid at the start of the loop
            {
              if(thread_status < 0 && !err_msg)
                err_msg = "Failed";

              lfd.status = thread_status > 0 ? TRUE : FALSE;
              lfd.err_msg = err_msg;

             (*(view_cbs_G->load_data))(zmap_view, zmap_view->app_data,&lfd);
            }
          }

	  if (err_msg)
	    g_free(err_msg) ;
	}
      while (list_item && (list_item = g_list_next(list_item))) ;
    }


  if (!has_step_list && reqs_finished)
    {
      zmapViewBusy(zmap_view, FALSE) ;

      /*
       * rather than count up the number loaded we say 'LOADED' if there's no LOADING active
       * This accounts for failures as well as completed loads
       */
      zmap_view->state = ZMAPVIEW_LOADED ;
      state_change = TRUE;
    }

  if (state_change)
    (*(view_cbs_G->state_change))(zmap_view, zmap_view->app_data, NULL) ;

  /* At this point if we have connections then we carry on looping looking for
   * replies from the views. If there are no threads left then we need to examine
   * our state and take action depending on whether we are dying or threads
   * have died or whatever.... */
  if (!zmap_view->connection_list)
    {
      /* Decide if we need to be called again or if everythings dead. */
      call_again = checkContinue(zmap_view) ;
    }


  return call_again ;
}




/* This is _not_ a generalised dispatch function, it handles a sequence of requests that
 * will end up fetching a feature context from a source. The steps are interdependent
 * and data from one step must be available to the next. */
static gboolean dispatchContextRequests(ZMapViewConnection connection, ZMapServerReqAny req_any)
{
  gboolean result = TRUE ;
  ConnectionData connect_data = (ConnectionData)(connection->request_data) ;

//printf("%s: dispatch %d\n",connection->url,req_any->type);
  switch (req_any->type)
    {
    case ZMAP_SERVERREQ_CREATE:
    case ZMAP_SERVERREQ_OPEN:
    case ZMAP_SERVERREQ_GETSERVERINFO:
      {

	break ;
      }
    case ZMAP_SERVERREQ_FEATURESETS:
      {
	ZMapServerReqFeatureSets feature_sets = (ZMapServerReqFeatureSets)req_any ;

	feature_sets->featureset_2_stylelist_out = connect_data->featureset_2_stylelist ;
// MH17: if this is an output parameter why do we set it on dispatch?
// beacuse it's a preallocated hash table

      // next 2 are outputs from ACE and inputs to pipeServers
      feature_sets->featureset_2_column_inout = connect_data->featureset_2_column;
      feature_sets->source_2_sourcedata_inout = connect_data->source_2_sourcedata;
//        print_fset2col("dispatch",feature_sets->featureset_2_column_inout);

	break ;
      }
    case ZMAP_SERVERREQ_STYLES:
      {
	ZMapServerReqStyles get_styles = (ZMapServerReqStyles)req_any ;

	/* required styles comes from featuresets call. */
	get_styles->required_styles_in = connect_data->required_styles ;

	break ;
      }
    case ZMAP_SERVERREQ_NEWCONTEXT:
      {
	ZMapServerReqNewContext new_context = (ZMapServerReqNewContext)req_any ;

	new_context->context = connect_data->curr_context ;

	break ;
      }
    case ZMAP_SERVERREQ_FEATURES:
      {
	ZMapServerReqGetFeatures get_features = (ZMapServerReqGetFeatures)req_any ;

	get_features->context = connect_data->curr_context ;
	get_features->styles = connect_data->curr_styles ;

	break ;
      }
    case ZMAP_SERVERREQ_SEQUENCE:
      // mh17: this was a cut and paste of _FEATURES
      // it turns out that the code that fields this expects it, so what is ZMapServerReqGetSequence about?
      {
      ZMapServerReqGetFeatures get_features = (ZMapServerReqGetFeatures)req_any ;

      get_features->context = connect_data->curr_context ;
      get_features->styles = connect_data->curr_styles ;

//	ZMapServerReqGetSequence get_sequence = (ZMapServerReqGetSequence)req_any ;
/*
      get_sequence->orig_feature = va_arg(args, ZMapFeature) ;
      get_sequence->sequences = va_arg(args, GList *) ;
      get_sequence->flags = va_arg(args, int) ;
*/
	break ;
      }
    case ZMAP_SERVERREQ_TERMINATE:
      {
      //ZMapServerReqTerminate terminate = (ZMapServerReqTerminate) req_any ;

      break ;
      }
    default:
      {
	zMapLogFatalLogicErr("switch(), unknown value: %d", req_any->type) ;

	break ;
      }
    }

  result = TRUE ;

  return result ;
}

void printStyle(GQuark style_id, gpointer data, gpointer user_data)
{
      char *x = (char *) user_data;
      printf("%s: style %s\n",x,g_quark_to_string(style_id));
}

void mergeHashTableCB(gpointer key, gpointer value, gpointer user)
{
      GHashTable *old = (GHashTable *) user;

      if(!g_hash_table_lookup(old,key))
            g_hash_table_insert(old,key,value);
}



// get 1-1 mapping of featureset names to style id except when configured differently
GList *get_required_styles_list(GHashTable *srchash,GList *fsets)
{
      GList *iter;
      GList *styles = NULL;
      ZMapGFFSource src;
      gpointer key,value;

      zMap_g_hash_table_iter_init(&iter,srchash);
      while(zMap_g_hash_table_iter_next(&iter,&key,&value))
      {
            src = g_hash_table_lookup(srchash,key);
            if(src)
                  value = GUINT_TO_POINTER(src->style_id);
            styles = g_list_prepend(styles,value);
      }

      return(styles);
}

/* This is _not_ a generalised processing function, it handles a sequence of replies from
 * a thread that build up a feature context from a source. The steps are interdependent
 * and data from one step must be available to the next. */
static gboolean processDataRequests(ZMapViewConnection view_con, ZMapServerReqAny req_any)
{
  gboolean result = TRUE ;
  ConnectionData connect_data = (ConnectionData)(view_con->request_data) ;
  ZMapView zmap_view = view_con->parent_view ;

  /* Process the different types of data coming back. */
//printf("%s: response to %d was %d\n",view_con->url,req_any->type,req_any->response);
//zMapLogWarning("%s: response to %d was %d\n",view_con->url,req_any->type,req_any->response);

  switch (req_any->type)
    {
    case ZMAP_SERVERREQ_CREATE:
    case ZMAP_SERVERREQ_OPEN:
      {
	break ;
      }
    case ZMAP_SERVERREQ_GETSERVERINFO:
      {
	ZMapServerReqGetServerInfo get_info = (ZMapServerReqGetServerInfo)req_any ;

	connect_data->database_name = get_info->database_name_out ;
	connect_data->database_title = get_info->database_title_out ;
	connect_data->database_path = get_info->database_path_out ;

	/* Hacky....what if there are several source names etc...we need a flag to say "master" source... */
	if (!(zmap_view->view_db_name))
	  zmap_view->view_db_name = connect_data->database_name ;

	if (!(zmap_view->view_db_title))
	  zmap_view->view_db_title = connect_data->database_title ;

	break ;
      }

    case ZMAP_SERVERREQ_FEATURESETS:
      {
	ZMapServerReqFeatureSets feature_sets = (ZMapServerReqFeatureSets)req_any ;


	if (req_any->response == ZMAP_SERVERRESPONSE_OK)
	  {
	    /* Got the feature sets so record them in the server context. */
	    connect_data->curr_context->feature_set_names = feature_sets->feature_sets_inout ;
	  }
	else if (req_any->response == ZMAP_SERVERRESPONSE_UNSUPPORTED && feature_sets->feature_sets_inout)
	  {
	    /* If server doesn't support checking feature sets then just use those supplied. */
	    connect_data->curr_context->feature_set_names = feature_sets->feature_sets_inout ;

	    feature_sets->required_styles_out =
                  get_required_styles_list(zmap_view->source_2_sourcedata,feature_sets->feature_sets_inout);
	  }

	/* Not all servers will provide a mapping between feature sets and styles so we add
	 * one now which is a straight mapping from feature set name to a style of the same name. */
	if (!(g_hash_table_size(feature_sets->featureset_2_stylelist_out)))
	  {
	    GList *sets ;

	    sets = feature_sets->feature_sets_inout ;

	    do
	      {
		GQuark feature_set_id, feature_set_name_id;

		/* We _must_ canonicalise here. */
		feature_set_name_id = GPOINTER_TO_UINT(sets->data) ;

		feature_set_id = zMapFeatureSetCreateID((char *)g_quark_to_string(feature_set_name_id)) ;

		zMap_g_hashlist_insert(feature_sets->featureset_2_stylelist_out,
				       feature_set_id,
				       GUINT_TO_POINTER(feature_set_id)) ;
	      }
	    while((sets = g_list_next(sets))) ;
	  }


	/* I don't know if we need these, can get from context. */
	connect_data->feature_sets = feature_sets->feature_sets_inout ;
	connect_data->required_styles = feature_sets->required_styles_out ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
printf("\nadding stylelists:\n");
	zMap_g_hashlist_print(feature_sets->featureset_2_stylelist_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	/* Merge the featureset to style hashses. */
	zMap_g_hashlist_merge(zmap_view->featureset_2_stylelist, feature_sets->featureset_2_stylelist_out) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
printf("\nview styles lists after merge:\n");
      zMap_g_hashlist_print(zmap_view->featureset_2_stylelist) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	/* If the hashes aren't equal, we had to do a merge.  Need to free the server
	 * created hash that will otherwise be left dangling... */
	if (zmap_view->featureset_2_stylelist != feature_sets->featureset_2_stylelist_out)
	  {
	    zMap_g_hashlist_destroy(feature_sets->featureset_2_stylelist_out);
	    feature_sets->featureset_2_stylelist_out = NULL;
	  }

	/* merge these in, the base mapping is defined in zMapViewIniGetData() */
      // NB these are not supplied by pipeServers and we assume a 1-1 mapping
      // (see above) of source to display featureset and source to style.
      // See also zmapViewRemoteReceive.c/xml_featureset_start_cb()

	if (!(zmap_view->featureset_2_column))
	  {
           zmap_view->featureset_2_column = feature_sets->featureset_2_column_inout ;
        }
      else if (feature_sets->featureset_2_column_inout &&
                  zmap_view->featureset_2_column != feature_sets->featureset_2_column_inout)
        {
//print_fset2col("merge view",zmap_view->featureset_2_column);
//print_fset2col("merge inout",feature_sets->featureset_2_column_inout);
          g_hash_table_foreach(feature_sets->featureset_2_column_inout,
            mergeHashTableCB,zmap_view->featureset_2_column);
        }

	if (!(zmap_view->source_2_sourcedata))
	  {
          zmap_view->source_2_sourcedata = feature_sets->source_2_sourcedata_inout ;
        }
      else if(feature_sets->source_2_sourcedata_inout &&
                  zmap_view->source_2_sourcedata !=  feature_sets->source_2_sourcedata_inout)
        {
          g_hash_table_foreach(feature_sets->source_2_sourcedata_inout,
            mergeHashTableCB,zmap_view->source_2_sourcedata);
        }
//print_src2src("got featuresets",zmap_view->source_2_sourcedata);
//print_fset2col("got featuresets",zmap_view->featureset_2_column);

// MH17: need to think about freeing _inout tables if in != out

	break ;
      }
    case ZMAP_SERVERREQ_STYLES:
      {
	ZMapServerReqStyles get_styles = (ZMapServerReqStyles)req_any ;

      //printf("\nmerging...old\n");
      //g_datalist_foreach(&(zmap_view->orig_styles), printStyle, "got styles") ;
      //printf("\nmerging...new\n");
      //g_datalist_foreach(&(get_styles->styles_out), printStyle, "got styles") ;

	/* Merge the retrieved styles into the views canonical style list. */
	zmap_view->orig_styles = zMapStyleMergeStyles(zmap_view->orig_styles, get_styles->styles_out,
						      ZMAPSTYLE_MERGE_PRESERVE) ;
	/* For dynamic loading the styles need to be set to load the features.*/
	if (connect_data->dynamic_loading)
	  {
	    gboolean is_complete_sequence = FALSE;

	    if (is_complete_sequence)
	      g_datalist_foreach(&(zmap_view->orig_styles), unsetDeferredLoadStylesCB, NULL) ;

	    g_datalist_foreach(&(get_styles->styles_out), unsetDeferredLoadStylesCB, NULL) ;
	  }


	/* Store the curr styles for use in creating the context and drawing features. */
	connect_data->curr_styles = get_styles->styles_out ;

	break ;
      }
    case ZMAP_SERVERREQ_NEWCONTEXT:
      {
	break ;
      }
    case ZMAP_SERVERREQ_FEATURES:
    case ZMAP_SERVERREQ_SEQUENCE:
      {
	ZMapServerReqGetFeatures get_features = (ZMapServerReqGetFeatures)req_any ;

	if (req_any->type == ZMAP_SERVERREQ_FEATURES)
	  {
	    char *missing_styles = NULL ;


	    if (!(connect_data->server_styles_have_mode)
		&& !zMapFeatureAnyAddModesToStyles((ZMapFeatureAny)(connect_data->curr_context),
						   connect_data->curr_styles))
	      {
		zMapLogWarning("Source %s, inferring Style modes from Features failed.",
			       view_con->url) ;

		result = FALSE ;
	      }

	    if (!(connect_data->server_styles_have_mode)
		&& !zMapFeatureAnyAddModesToStyles((ZMapFeatureAny)(connect_data->curr_context),
						   zmap_view->orig_styles))
	      {
		zMapLogWarning("Source %s, inferring Style modes from Features failed.",
			       view_con->url) ;

		result = FALSE ;
	      }


	    /* I'm not sure if this couldn't come much earlier actually....something
	     * to investigate.... */

	    if (result && !makeStylesDrawable(connect_data->curr_styles, &missing_styles))
	      {
		zMapLogWarning("Failed to make following styles drawable: %s", missing_styles) ;

		result = FALSE ;
	      }

	    if (result && !makeStylesDrawable(zmap_view->orig_styles, &missing_styles))
	      {
		zMapLogWarning("Failed to make following styles drawable: %s", missing_styles) ;

		result = FALSE ;
	      }
	  }

	/* ok...once we are here we can display stuff.... */
	if (result && req_any->type == connect_data->display_after)
	  {
	    /* Isn't there a problem here...which bit of info goes with which server ???? */
	    zmapViewSessionAddServerInfo(zmap_view->session_data, connect_data->database_path) ;

	    getFeatures(zmap_view, get_features, connect_data->curr_styles) ;

	  }

	break ;
      }
//    case ZMAP_SERVERREQ_GETSEQUENCE:
      // never appears?? - see commandCB() and processGetSeqRequests() in this file

    case ZMAP_SERVERREQ_TERMINATE:
      {
      break ;
      }

    default:
      {
	zMapLogFatalLogicErr("switch(), unknown value: %d", req_any->type) ;
	result = FALSE ;

	break ;
      }
    }

  return result ;
}

static void freeDataRequest(ZMapServerReqAny req_any)
{

  zMapServerCreateRequestDestroy(req_any) ;

  return ;
}



/*
 *      Callbacks for getting local sequences for passing to blixem.
 */
static gboolean processGetSeqRequests(ZMapViewConnection view_con, ZMapServerReqAny req_any)
{
  gboolean result = FALSE ;
  ZMapView zmap_view = view_con->parent_view ;

  if (req_any->type == ZMAP_SERVERREQ_GETSEQUENCE)
    {
      ZMapServerReqGetSequence get_sequence = (ZMapServerReqGetSequence)req_any ;
      GPid blixem_pid ;
      gboolean status ;

      /* Got the sequences so launch blixem. */
      if ((status = zmapViewCallBlixem(zmap_view,
				       get_sequence->orig_feature,
				       get_sequence->sequences,
				       get_sequence->flags,
				       &blixem_pid,

				       &(zmap_view->kill_blixems))))
	zmap_view->spawned_processes = g_list_append(zmap_view->spawned_processes,
						     GINT_TO_POINTER(blixem_pid)) ;

      result = TRUE ;
    }
  else
    {
      zMapLogFatalLogicErr("wrong request type: %d", req_any->type) ;

      result = FALSE ;
    }


  return result ;
}







/* Calls the control window callback to remove any reference to the zmap and then destroys
 * the actual zmap itself.
 *  */
static void killGUI(ZMapView zmap_view)
{
  killAllWindows(zmap_view) ;

  return ;
}


static void killConnections(ZMapView zmap_view)
{
  GList* list_item ;

  if(!(zmap_view->connection_list))
      return;

  list_item = g_list_first(zmap_view->connection_list) ;
  do
    {
      ZMapViewConnection view_con ;
      ZMapThread thread ;

      view_con = list_item->data ;
      thread = view_con->thread ;

      /* NOTE, we do a _kill_ here, not a destroy. This just signals the thread to die, it
       * will actually die sometime later. */
      zMapThreadKill(thread) ;
    }
  while ((list_item = g_list_next(list_item))) ;

  return ;
}





static void invoke_merge_in_names(gpointer list_data, gpointer user_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)list_data;
  GList *feature_set_names = (GList *)user_data;

  /* This relies on hokey code... with a number of issues...
   * 1) the window function only concats the lists.
   * 2) this view code might well want to do the merge?
   * 3) how do we order all these columns?
   */
   /* mh17: column ordering is as in ZMap config
    * either by 'columns' command or by server and then featureset order
    * concat is ok as featuresets are not duplicated
    */
  zMapWindowMergeInFeatureSetNames(view_window->window, feature_set_names);

  return ;
}


/* Allocate a connection and send over the request to get the sequence displayed. */
/* NB: this is called from zmapViewConnect() and also zmapViewLoadFeatures() and commandCB (for DNA only) */
static ZMapViewConnection createConnection(ZMapView zmap_view,
                                 ZMapViewConnection view_con,
                                 ZMapFeatureContext context,
					   char *server_url, char *format,
					   int timeout, char *version,
					   char *styles, char *styles_file,
                                 GList *req_featuresets,
					   gboolean dna_requested,
                                 gboolean terminate)
{

  ZMapThread thread ;
  ConnectionData connect_data ;
  gboolean existing = FALSE;
  int url_parse_error ;
  ZMapURL urlObj;

     /* Parse the url and create connection. */
  if (!(urlObj = url_parse(server_url, &url_parse_error)))
   {
      zMapLogWarning("GUI: url %s did not parse. Parse error < %s >\n",
            server_url, url_error(url_parse_error)) ;
      return(NULL);
   }

  if(view_con)
    {
          // use existing connection if not busy
        if(view_con->step_list)
          view_con = NULL;
        else
          existing = TRUE;
//if(existing) printf("using existing connection %s\n",view_con->url);
    }

  if(!view_con)
    {
      /* Create the thread to service the connection requests, we give it a function that it will call
      * to decode the requests we send it and a terminate function. */
      if ((thread = zMapThreadCreate(zMapServerRequestHandler,
			        zMapServerTerminateHandler, zMapServerDestroyHandler)))
        {
                  /* Create the connection struct. */
            view_con = g_new0(ZMapViewConnectionStruct, 1) ;
            view_con->parent_view = zmap_view ;
            view_con->thread = thread ;
            view_con->url = g_strdup(server_url) ;
//printf("create thread for %s\n",view_con->url);
        }
      else
        {
            return(NULL);
        }
    }

  if(view_con)
    {
      ZMapServerReqAny req_any;

      view_con->curr_request = ZMAPTHREAD_REQUEST_EXECUTE ;

      connect_data = g_new0(ConnectionDataStruct, 1) ;
      connect_data->curr_context = context ;
      if(terminate)           // ie server->delayed -> called after startup
           connect_data->dynamic_loading = TRUE ;

      connect_data->featureset_2_stylelist = zMap_g_hashlist_create() ;
      connect_data->featureset_2_column = zmap_view->featureset_2_column;
      connect_data->source_2_sourcedata = zmap_view->source_2_sourcedata;

      // we need to save this to tell otterlace when we've finished
      // it also gets given to threads: when can we free it?
      connect_data->feature_sets = req_featuresets;



      view_con->request_data = connect_data ;


      view_con->step_list = zmapViewConnectionStepListCreate(dispatchContextRequests,
                                 processDataRequests,
                                 freeDataRequest);

      /* CHECK WHAT THIS IS ABOUT.... */
      /* Record info. for this session. */
      zmapViewSessionAddServer(zmap_view->session_data,urlObj,format) ;

      connect_data->display_after = ZMAP_SERVERREQ_FEATURES;

      /* Set up this connection in the step list. */
      if(!existing)
      {
            req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_CREATE, urlObj, format, timeout, version) ;
            zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_CREATE, req_any) ;
            req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_OPEN) ;
            zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_OPEN, req_any) ;
            req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_GETSERVERINFO) ;
            zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_GETSERVERINFO, req_any) ;
      }

      if(req_featuresets)
      {
        req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_FEATURESETS, req_featuresets, NULL) ;
        zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_FEATURESETS, req_any) ;
        req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_STYLES, styles, styles_file) ;
        zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_STYLES, req_any) ;
        req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_NEWCONTEXT, context) ;
        zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_NEWCONTEXT, req_any) ;
        req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_FEATURES) ;
        zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_FEATURES, req_any) ;
      }

      if (dna_requested)
	{
	  req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_SEQUENCE) ;
	  zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_SEQUENCE, req_any) ;
        connect_data->display_after = ZMAP_SERVERREQ_SEQUENCE ;
      }

      if(terminate)
      {
        req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_TERMINATE) ;
        zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_TERMINATE, req_any) ;
      }

      if(!existing)
          zmap_view->connection_list = g_list_append(zmap_view->connection_list, view_con) ;

      /* Start the connection to the source. */
      zmapViewStepListIter(view_con) ;
    }
  else
    {
      zMapLogWarning("GUI: url %s looks ok (host: %s\tport: %d)"
                  "but could not connect to server.",
                  urlObj->url,
                  urlObj->host,
                  urlObj->port) ;
    }

  return view_con ;
}



static void destroyConnection(ZMapView view, ZMapViewConnection view_conn)
{

  if(view->sequence_server == view_conn)
      view->sequence_server = NULL;

  zMapThreadDestroy(view_conn->thread) ;

  g_free(view_conn->url) ;

  if(view_conn->step_list)
      zmapViewStepListDestroy(view_conn);

  /* Need to destroy the types array here....... */

  g_free(view_conn) ;

  return ;
}


/* set all the windows attached to this view so that they contain nothing. */
static void resetWindows(ZMapView zmap_view)
{
  GList* list_item ;

  list_item = g_list_first(zmap_view->window_list) ;

  do
    {
      ZMapViewWindow view_window ;

      view_window = list_item->data ;

      zMapWindowReset(view_window->window) ;
    }
  while ((list_item = g_list_next(list_item))) ;

  return ;
}



/* Signal all windows there is data to draw. */
static void displayDataWindows(ZMapView zmap_view,
			       ZMapFeatureContext all_features,
                               ZMapFeatureContext new_features, GData *new_styles,
                               gboolean undisplay)
{
  GList *list_item, *window_list  = NULL;
  gboolean clean_required = FALSE;

  list_item = g_list_first(zmap_view->window_list) ;

  /* when the new features aren't the stored features i.e. not the first draw */
  /* un-drawing the features doesn't work the same way as drawing */
  if(all_features != new_features && !undisplay)
    {
      clean_required = TRUE;
    }

  do
    {
      ZMapViewWindow view_window ;

      view_window = list_item->data ;

      if (!undisplay)
      {
        zMapWindowDisplayData(view_window->window, NULL,
			      all_features, new_features,
			      view_window->parent_view->orig_styles, new_styles,
			      zmap_view->featureset_2_stylelist) ;
      }
      else
        zMapWindowUnDisplayData(view_window->window, all_features, new_features);

      if (clean_required)
        window_list = g_list_append(window_list, view_window->window);
    }
  while ((list_item = g_list_next(list_item))) ;

  if(clean_required)
    zmapViewCWHSetList(zmap_view->cwh_hash, new_features, window_list);

  return ;
}


static void loaded_dataCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapFeatureContext context = (ZMapFeatureContext)window_data;
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data;
  ZMapView view;
  gboolean removed, debug = FALSE, unique_context;

  view = zMapViewGetView(view_window);

  if(debug)
    zMapLogWarning("%s", "Attempting to Destroy Diff Context");

  removed = zmapViewCWHRemoveContextWindow(view->cwh_hash, &context, window, &unique_context);

  if(debug)
    {
      if(removed)
        zMapLogWarning("%s", "Context was destroyed");
      else if(unique_context)
        zMapLogWarning("%s", "Context is the _only_ context");
      else
        zMapLogWarning("%s", "Another window still needs the context memory");
    }

  if(removed || unique_context)
    (*(view_cbs_G->load_data))(view, view->app_data, NULL) ;

  return ;
}


/* Kill all the windows... */
static void killAllWindows(ZMapView zmap_view)
{

  while (zmap_view->window_list)
    {
      ZMapViewWindow view_window ;

      view_window = zmap_view->window_list->data ;

      destroyWindow(zmap_view, view_window) ;
    }


  return ;
}


static ZMapViewWindow createWindow(ZMapView zmap_view, ZMapWindow window)
{
  ZMapViewWindow view_window ;

  view_window = g_new0(ZMapViewWindowStruct, 1) ;
  view_window->parent_view = zmap_view ;		    /* back pointer. */
  view_window->window = window ;

  return view_window ;
}


static void destroyWindow(ZMapView zmap_view, ZMapViewWindow view_window)
{
  zmap_view->window_list = g_list_remove(zmap_view->window_list, view_window) ;

  zMapWindowDestroy(view_window->window) ;

  g_free(view_window) ;

  return ;
}



/* We have far too many function calls here...it's all confusing..... */
static void getFeatures(ZMapView zmap_view, ZMapServerReqGetFeatures feature_req, GData *styles)
{
  ZMapFeatureContext new_features = NULL ;

  zMapPrintTimer(NULL, "Got Features from Thread") ;

  /* May be a new context or a merge with an existing one. */
  if (feature_req->context)
    {
      new_features = feature_req->context ;

      if (!mergeAndDrawContext(zmap_view, new_features, styles))
	feature_req->context = NULL ;
    }

  return ;
}



/* Error handling is rubbish here...stuff needs to be free whether there is an error or not.
 *
 * new_features should be freed (but not the data...ahhhh actually the merge should
 * free any replicated data...yes, that is what should happen. Then when it comes to
 * the diff we should not free the data but should free all our structs...
 *
 * We should free the context_inout context here....actually better
 * would to have a "free" flag............
 *  */
static gboolean mergeAndDrawContext(ZMapView view, ZMapFeatureContext context_in, GData *styles)
{
  gboolean merge_results = FALSE ;
  ZMapFeatureContext diff_context = NULL ;

  if ((merge_results = justMergeContext(view, &context_in, styles)))
    {
      diff_context = context_in;

      justDrawContext(view, diff_context, styles);
    }

  return merge_results ;
}

static gboolean justMergeContext(ZMapView view, ZMapFeatureContext *context_inout, GData *styles)
{
  gboolean merge_result = FALSE ;
  ZMapFeatureContext new_features, diff_context = NULL ;
  ZMapFeatureContextMergeCode merge = ZMAPFEATURE_CONTEXT_ERROR ;

  new_features = *context_inout ;

//  printf("just Merge new = %s\n",zMapFeatureContextGetDNAStatus(new_features) ? "yes" : "non");

  zMapPrintTimer(NULL, "Merge Context starting...") ;

  if (view->revcomped_features)
    {
      zMapPrintTimer(NULL, "Merge Context has to rev comp first, starting") ;

      zMapFeatureReverseComplement(new_features, view->orig_styles);

      zMapPrintTimer(NULL, "Merge Context has to rev comp first, finished") ;
    }

  merge = zMapFeatureContextMerge(&(view->features), &new_features, &diff_context) ;

//  printf("just Merge view = %s\n",zMapFeatureContextGetDNAStatus(view->features) ? "yes" : "non");
//  printf("just Merge diff = %s\n",zMapFeatureContextGetDNAStatus(diff_context) ? "yes" : "non");

#ifdef MH17_NEVER
      {
            GError *err = NULL;

            zMapFeatureDumpToFileName(diff_context,"features.txt","(justMerge) diff context:\n", NULL, &err) ;
            zMapFeatureDumpToFileName(view->features,"features.txt","(justMerge) view->Features:\n", NULL, &err) ;

      }
#endif

  if (merge == ZMAPFEATURE_CONTEXT_OK)
    {
      zMapLogMessage("%s", "Context merge succeeded.") ;
      merge_result = TRUE ;
    }
  else if (merge == ZMAPFEATURE_CONTEXT_NONE)
    zMapLogWarning("%s", "Context merge failed because no new features found in new context.") ;
  else
    zMapLogCritical("%s", "Context merge failed, serious error.") ;


  zMapPrintTimer(NULL, "Merge Context Finished.") ;

  /* Return the diff_context which is the just the new features (NULL if merge fails). */
  *context_inout = diff_context ;

  return merge_result ;
}

static void justDrawContext(ZMapView view, ZMapFeatureContext diff_context, GData *new_styles)
{
   /* Signal the ZMap that there is work to be done. */
  displayDataWindows(view, view->features, diff_context, new_styles, FALSE) ;

  /* Not sure about the timing of the next bit. */

  /* We have to redraw the whole navigator here.  This is a bit of
   * a pain, but it's due to the scaling we do to make the rest of
   * the navigator work.  If the length of the sequence changes the
   * all the previously drawn features need to move.  It also
   * negates the need to keep state as to the length of the sequence,
   * the number of times the scale bar has been drawn, etc... */
  zMapWindowNavigatorReset(view->navigator_window); /* So reset */
  zMapWindowNavigatorSetStrand(view->navigator_window, view->revcomped_features);
  /* and draw with _all_ the view's features. */
  zMapWindowNavigatorDrawFeatures(view->navigator_window, view->features, view->orig_styles);

  /* signal our caller that we have data. */
  (*(view_cbs_G->load_data))(view, view->app_data, NULL) ;

  return ;
}

static void eraseAndUndrawContext(ZMapView view, ZMapFeatureContext context_inout)
{
  ZMapFeatureContext diff_context = NULL;

  if(!zMapFeatureContextErase(&(view->features), context_inout, &diff_context))
    zMapLogCritical("%s", "Cannot erase feature data from...");
  else
    {
      displayDataWindows(view, view->features, diff_context, NULL, TRUE);

      zMapFeatureContextDestroy(diff_context, TRUE);
    }

  return ;
}



/* Layer below wants us to execute a command. */
static void commandCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
  ZMapView view = view_window->parent_view ;
  ZMapWindowCallbackCommandAny cmd_any = (ZMapWindowCallbackCommandAny)window_data ;

  switch(cmd_any->cmd)
    {
    case ZMAPWINDOW_CMD_SHOWALIGN:
      {
	ZMapWindowCallbackCommandAlign align_cmd = (ZMapWindowCallbackCommandAlign)cmd_any ;
	gboolean status ;
	GList *local_sequences = NULL ;
	ZMapViewBlixemAlignSet align_type = BLIXEM_NO_MATCHES ;

	/* GHASTLY....ALL CHOICE BIT SHOULD BE IN WINDOW.... */
	if (align_cmd->blix_type.single_match)
	  align_type = BLIXEM_FEATURE_SINGLE_MATCH ;
	else if (align_cmd->blix_type.single_feature)
	  align_type = BLIXEM_FEATURE_ALL_MATCHES ;
	else if (align_cmd->blix_type.feature_set)
	  align_type = BLIXEM_FEATURESET_MATCHES ;
	else if (align_cmd->blix_type.multi_sets)
	  align_type = BLIXEM_MULTI_FEATURESET_MATCHES ;
	else if (align_cmd->blix_type.all_sets)
	  align_type = BLIXEM_ALL_FEATURESET_MATCHES ;

	if ((status = zmapViewBlixemLocalSequences(view, align_cmd->feature, &local_sequences)))
	  {
	    if (!view->sequence_server)
	      {
		zMapWarning("%s", "No sequence server was specified so cannot fetch raw sequences for blixem.") ;
	      }
	    else
	      {
		ZMapViewConnection view_con ;
		ZMapViewConnectionRequest request ;
		ZMapServerReqAny req_any ;

		view_con = view->sequence_server ;
            // assumed to be acedb

		if (!view_con) // || !view_con->sequence_server)
		  {
		    zMapWarning("%s", "Sequence server incorrectly specified in config file"
				" so cannot fetch local sequences for blixem.") ;
		  }
		else if(view_con->step_list)
              {
                zMapWarning("%s", "Sequence server is currently active"
                        " so cannot fetch local sequences for blixem.") ;
              }
            else
		  {
                zmapViewBusy(view, TRUE) ;

		    /* Create the step list that will be used to fetch the sequences. */
		    view_con->step_list = zmapViewStepListCreate(NULL, processGetSeqRequests, NULL) ;
		    zmapViewStepListAddStep(view_con->step_list, ZMAP_SERVERREQ_GETSEQUENCE,
					    REQUEST_ONFAIL_CANCEL_STEPLIST) ;

		    /* Add the request to the step list. */
		    req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_GETSEQUENCE,
						      align_cmd->feature, local_sequences, align_type) ;
		    request = zmapViewStepListAddServerReq(view_con->step_list,
							   view_con, ZMAP_SERVERREQ_GETSEQUENCE, req_any) ;

		    /* Start the step list. */
		    zmapViewStepListIter(view_con) ;
		  }
	      }
	  }
	else
	  {
	    GPid blixem_pid ;

	    if ((status = zmapViewCallBlixem(view, align_cmd->feature, NULL, align_type,
					     &blixem_pid, &(view->kill_blixems))))
	      view->spawned_processes = g_list_append(view->spawned_processes, GINT_TO_POINTER(blixem_pid)) ;
	  }

	break ;
      }
    case ZMAPWINDOW_CMD_GETFEATURES:
      {
	ZMapWindowCallbackGetFeatures get_data = (ZMapWindowCallbackGetFeatures)cmd_any ;

	zmapViewLoadFeatures(view, get_data->block, get_data->feature_set_ids, get_data->start, get_data->end) ;

	break ;
      }


    case ZMAPWINDOW_CMD_REVERSECOMPLEMENT:
      {
	gboolean status ;

	/* NOTE, there is no need to signal the layer above that things are changing,
	 * the layer above does the complement and handles all that. */
	if (!(status = zMapViewReverseComplement(view)))
	  {
	    zMapLogCritical("%s", "View Reverse Complement failed.") ;

	    zMapWarning("%s", "View Reverse Complement failed.") ;
	  }

	break ;
      }
    default:
      {
	zMapAssertNotReached() ;
	break ;
      }
    }


  return ;
}


static void viewVisibilityChangeCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
  ZMapWindowVisibilityChange vis = (ZMapWindowVisibilityChange)window_data;

  /* signal our caller that something has changed. */
  (*(view_cbs_G->visibility_change))(view_window, view_window->parent_view->app_data, window_data) ;

  /* view_window->window can be NULL (when window copying) so we use the passed in window... */
  /* Yes it's a bit messy, but it's stopping it crashing. */
  zMapWindowNavigatorSetCurrentWindow(view_window->parent_view->navigator_window, window);

  zMapWindowNavigatorDrawLocator(view_window->parent_view->navigator_window, vis->scrollable_top, vis->scrollable_bot);

  return;
}

/* When a window is split, we set the zoom status of all windows */
static void setZoomStatusCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;

  g_list_foreach(view_window->parent_view->window_list, setZoomStatus, NULL) ;

  return;
}

static void setZoomStatus(gpointer data, gpointer user_data)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapViewWindow view_window = (ZMapViewWindow)data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  return;
}



/* Trial code to get alignments from a file and create a context...... */
static ZMapFeatureContext createContext(char *sequence, int start, int end, GList *feature_set_names)
{
  ZMapFeatureContext context = NULL ;
  gboolean master = TRUE ;
  ZMapFeatureAlignment alignment ;
  ZMapFeatureBlock block ;

  context = zMapFeatureContextCreate(sequence, start, end, feature_set_names) ;

  /* Add the master alignment and block. */
  alignment = zMapFeatureAlignmentCreate(sequence, master) ; /* TRUE => master alignment. */

  zMapFeatureContextAddAlignment(context, alignment, master) ;

  block = zMapFeatureBlockCreate(sequence,
				 start, end, ZMAPSTRAND_FORWARD,
				 start, end, ZMAPSTRAND_FORWARD) ;  // mh17: this looks like it should be reversed, but v difficult to tell

  zMapFeatureAlignmentAddBlock(alignment, block) ;


  /* Add other alignments if any were specified in a config file. */
  addAlignments(context) ;


  return context ;
}


/* Add other alignments if any specified in a config file. */
static void addAlignments(ZMapFeatureContext context)
{
#ifdef THIS_NEEDS_REDOING
  ZMapConfigStanzaSet blocks_list = NULL ;
  ZMapConfig config ;
  char *config_file ;
  char *stanza_name = "align" ;
  gboolean result = FALSE ;
  char *ref_seq ;
  int start, end ;
  ZMapFeatureAlignment alignment = NULL ;


  ref_seq = (char *)g_quark_to_string(context->sequence_name) ;
  start = context->sequence_to_parent.c1 ;
  end = context->sequence_to_parent.c2 ;


  config_file = zMapConfigDirGetFile() ;
  if ((config = zMapConfigCreateFromFile(config_file)))
    {
      ZMapConfigStanza block_stanza ;
      ZMapConfigStanzaElementStruct block_elements[]
	= {{"reference_seq"        , ZMAPCONFIG_STRING , {NULL}},
	   {"reference_start"      , ZMAPCONFIG_INT    , {NULL}},
	   {"reference_end"        , ZMAPCONFIG_INT    , {NULL}},
	   {"reference_strand"     , ZMAPCONFIG_INT    , {NULL}},
	   {"non_reference_seq"    , ZMAPCONFIG_STRING , {NULL}},
	   {"non_reference_start"  , ZMAPCONFIG_INT    , {NULL}},
	   {"non_reference_end"    , ZMAPCONFIG_INT    , {NULL}},
	   {"non_reference_strand" , ZMAPCONFIG_INT    , {NULL}},
	   {NULL, -1, {NULL}}} ;

      /* init non string elements. */
      zMapConfigGetStructInt(block_elements, "reference_start")      = 0 ;
      zMapConfigGetStructInt(block_elements, "reference_end")        = 0 ;
      zMapConfigGetStructInt(block_elements, "reference_strand")     = 0 ;
      zMapConfigGetStructInt(block_elements, "non_reference_start")  = 0 ;
      zMapConfigGetStructInt(block_elements, "non_reference_end")    = 0 ;
      zMapConfigGetStructInt(block_elements, "non_reference_strand") = 0 ;

      block_stanza = zMapConfigMakeStanza(stanza_name, block_elements) ;

      result = zMapConfigFindStanzas(config, block_stanza, &blocks_list) ;
    }

  if (result)
    {
      ZMapConfigStanza next_block = NULL;

      while ((next_block = zMapConfigGetNextStanza(blocks_list, next_block)) != NULL)
        {
          ZMapFeatureBlock data_block = NULL ;
	  char *reference_seq, *non_reference_seq ;
	  int reference_start, reference_end, non_reference_start, non_reference_end ;
	  ZMapStrand ref_strand = ZMAPSTRAND_REVERSE, non_strand = ZMAPSTRAND_REVERSE ;
	  int diff ;

	  reference_seq = zMapConfigGetElementString(next_block, "reference_seq") ;
	  reference_start = zMapConfigGetElementInt(next_block, "reference_start") ;
	  reference_end = zMapConfigGetElementInt(next_block, "reference_end") ;
	  non_reference_seq = zMapConfigGetElementString(next_block, "non_reference_seq") ;
	  non_reference_start = zMapConfigGetElementInt(next_block, "non_reference_start") ;
	  non_reference_end = zMapConfigGetElementInt(next_block, "non_reference_end") ;
	  if (zMapConfigGetElementInt(next_block, "reference_strand"))
	    ref_strand = ZMAPSTRAND_FORWARD ;
	  if (zMapConfigGetElementInt(next_block, "non_reference_strand"))
	    non_strand = ZMAPSTRAND_FORWARD ;


	  /* We only add aligns/blocks that are for the relevant reference sequence and within
	   * the start/end for that sequence. */
	  if (g_ascii_strcasecmp(ref_seq, reference_seq) == 0
	      && !(reference_start > end || reference_end < start))
	    {

	      if (!alignment)
		{
		  /* Add the other alignment, note that we do this dumbly at the moment assuming
		   * that there is only one other alignment */
		  alignment = zMapFeatureAlignmentCreate(non_reference_seq, FALSE) ;

		  zMapFeatureContextAddAlignment(context, alignment, FALSE) ;
		}


	      /* Add the block for this set of data. */

	      /* clamp coords...SHOULD WE DO THIS, PERHAPS WE JUST REJECT THINGS THAT DON'T FIT ? */
	      if ((diff = start - reference_start) > 0)
		{
		  reference_start += diff ;
		  non_reference_start += diff ;
		}
	      if ((diff = reference_end - end) > 0)
		{
		  reference_end -= diff ;
		  non_reference_end -= diff ;
		}

	      data_block = zMapFeatureBlockCreate(non_reference_seq,
						  reference_start, reference_end, ref_strand,
						  non_reference_start, non_reference_end, non_strand) ;

	      zMapFeatureAlignmentAddBlock(alignment, data_block) ;
	    }
        }
    }
#endif /* THIS_NEEDS_REDOING */
  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Creates a copy of a context from the given block upwards and sets the start/end
 * of features within the block. */
static ZMapFeatureContext createContextCopyFromBlock(ZMapFeatureBlock block, GList *feature_set_names,
						     int features_start, int features_end)
{
  ZMapFeatureContext context = NULL ;
  gboolean master = TRUE ;
  ZMapFeatureAlignment alignment ;


  context = zMapFeatureContextCreate(sequence, start, end, feature_set_names) ;

  /* Add the master alignment and block. */
  alignment = zMapFeatureAlignmentCreate(sequence, master) ; /* TRUE => master alignment. */

  zMapFeatureContextAddAlignment(context, alignment, master) ;

  block = zMapFeatureBlockCreate(sequence,
				 start, end, ZMAPSTRAND_FORWARD,
				 start, end, ZMAPSTRAND_FORWARD) ;

  zMapFeatureBlockSetFeaturesCoords(block, features_start, features_end) ;

  zMapFeatureAlignmentAddBlock(alignment, block) ;


  return context ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */







#ifdef NOT_REQUIRED_ATM

/* Read list of sequence to server mappings (i.e. which sequences must be fetched from which
 * servers) from the zmap config file. */
static gboolean getSequenceServers(ZMapView zmap_view, char *config_str)
{
  gboolean result = FALSE ;
  ZMapConfig config ;
  ZMapConfigStanzaSet zmap_list = NULL ;
  ZMapConfigStanza zmap_stanza ;
  char *zmap_stanza_name = ZMAPSTANZA_APP_CONFIG ;
  ZMapConfigStanzaElementStruct zmap_elements[] = {{ZMAPSTANZA_APP_SEQUENCE_SERVERS, ZMAPCONFIG_STRING, {NULL}},
						   {NULL, -1, {NULL}}} ;

  if ((config = getConfigFromBufferOrFile(config_str)))
    {
      zmap_stanza = zMapConfigMakeStanza(zmap_stanza_name, zmap_elements) ;

      if (zMapConfigFindStanzas(config, zmap_stanza, &zmap_list))
	{
	  ZMapConfigStanza next_zmap = NULL ;

	  /* We only read the first of these stanzas.... */
	  if ((next_zmap = zMapConfigGetNextStanza(zmap_list, next_zmap)) != NULL)
	    {
	      char *server_seq_str ;

	      if ((server_seq_str
		   = zMapConfigGetElementString(next_zmap, ZMAPSTANZA_APP_SEQUENCE_SERVERS)))
		{
		  char *sequence, *server ;
		  ZMapViewSequence2Server seq_2_server ;
		  char *search_str ;

		  search_str = server_seq_str ;
		  while ((sequence = strtok(search_str, " ")))
		    {
		      search_str = NULL ;
		      server = strtok(NULL, " ") ;

		      seq_2_server = createSeq2Server(sequence, server) ;

		      zmap_view->sequence_2_server = g_list_append(zmap_view->sequence_2_server, seq_2_server) ;
		    }
		}
	    }

	  zMapConfigDeleteStanzaSet(zmap_list) ;		    /* Not needed anymore. */
	}

      zMapConfigDestroyStanza(zmap_stanza) ;

      zMapConfigIniDestroy(config) ;

      result = TRUE ;
    }

  return result ;
}


static void destroySeq2ServerCB(gpointer data, gpointer user_data_unused)
{
  ZMapViewSequence2Server seq_2_server = (ZMapViewSequence2Server)data ;

  destroySeq2Server(seq_2_server) ;

  return ;
}

static ZMapViewSequence2Server createSeq2Server(char *sequence, char *server)
{
  ZMapViewSequence2Server seq_2_server = NULL ;

  seq_2_server = g_new0(ZMapViewSequence2ServerStruct, 1) ;
  seq_2_server->sequence = g_strdup(sequence) ;
  seq_2_server->server = g_strdup(server) ;

  return seq_2_server ;
}


static void destroySeq2Server(ZMapViewSequence2Server seq_2_server)
{
  g_free(seq_2_server->sequence) ;
  g_free(seq_2_server->server) ;
  g_free(seq_2_server) ;

  return ;
}

/* Check to see if a sequence/server pair match any in the list mapping sequences to servers held
 * in the view. NOTE that if the sequence is not in the list at all then this function returns
 * TRUE as it is assumed by default that sequences can be fetched from any servers.
 * */
static gboolean checkSequenceToServerMatch(GList *seq_2_server, ZMapViewSequence2Server target_seq_server)
{
  gboolean result = FALSE ;
  GList *server_item ;

  /* If the sequence is in the list then check the server names. */
  if ((server_item = g_list_find_custom(seq_2_server, target_seq_server, findSequence)))
    {
      ZMapViewSequence2Server curr_seq_2_server = (ZMapViewSequence2Server)server_item->data ;

      if ((g_ascii_strcasecmp(curr_seq_2_server->server, target_seq_server->server) == 0))
	result = TRUE ;
    }
  else
    {
      /* Return TRUE if sequence not in list. */
      result = TRUE ;
    }

  return result ;
}


/* A GCompareFunc to check whether servers match in the sequence to server mapping. */
static gint findSequence(gconstpointer a, gconstpointer b)
{
  gint result = -1 ;
  ZMapViewSequence2Server seq_2_server = (ZMapViewSequence2Server)a ;
  ZMapViewSequence2Server target_fetch = (ZMapViewSequence2Server)b ;

  if ((g_ascii_strcasecmp(seq_2_server->sequence, target_fetch->sequence) == 0))
    result = 0 ;

  return result ;
}
#endif /* NOT_REQUIRED_ATM */

/* Hacky...sorry.... */
static void threadDebugMsg(ZMapThread thread, char *format_str, char *msg)
{
  char *thread_id ;
  char *full_msg ;

  thread_id = zMapThreadGetThreadID(thread) ;
  full_msg = g_strdup_printf(format_str, thread_id, msg ? msg : "") ;

  zMapDebug("%s", full_msg) ;
//  zMapLogWarning("%s",full_msg);

  g_free(full_msg) ;
  g_free(thread_id) ;

  return ;
}



/* check whether there are live connections or not, return TRUE if there are, FALSE otherwise. */
static gboolean checkContinue(ZMapView zmap_view)
{
  gboolean connections = FALSE ;

  switch (zmap_view->state)
    {
    case ZMAPVIEW_INIT:
    case ZMAPVIEW_LOADED:           // delayed servers get cleared up, we do not need connections to run

      {
	/* Nothing to do here I think.... */
	connections = TRUE ;
	break ;
      }
    case ZMAPVIEW_CONNECTING:
    case ZMAPVIEW_CONNECTED:
    case ZMAPVIEW_LOADING:
//    case ZMAPVIEW_LOADED:
      {
	/* Kill the view as all connections have died. */
	/* Threads have died because of their own errors....but the ZMap is not dying so
	 * reset state to init and we should return an error here....(but how ?), we
	 * should not be outputting messages....I think..... */
	/* need to reset here..... */

	/* Tricky coding here: we need to destroy the view _before_ we notify the layer above us
	 * that the zmap_view has gone but we need to pass information from the view back, so we
	 * keep a temporary record of certain parts of the view. */
	ZMapView zmap_view_ref = zmap_view ;
	void *app_data = zmap_view->app_data ;
	ZMapViewCallbackDestroyData destroy_data ;

	zMapWarning("%s", "Cannot show ZMap because server connections have all died or been cancelled.") ;
	zMapLogWarning("%s", "Cannot show ZMap because server connections have all died or been cancelled.") ;

	zmap_view->state = ZMAPVIEW_DYING ;

	killGUI(zmap_view) ;

	destroy_data = g_new(ZMapViewCallbackDestroyDataStruct, 1) ; /* Caller must free. */
	destroy_data->xwid = zmap_view->xwid ;

	destroyZMapView(&zmap_view) ;

	/* Signal layer above us that view has died. */
	(*(view_cbs_G->destroy))(zmap_view_ref, app_data, destroy_data) ;

	connections = FALSE ;

	break ;
      }
    case ZMAPVIEW_UPDATING:
      {
      /* zmap is ok but a  new column didn't come through */
      zmap_view->state = ZMAPVIEW_LOADED;

      /* Signal layer above us because the view has stopped loading. */
      (*(view_cbs_G->state_change))(zmap_view, zmap_view->app_data, NULL) ;
//printf("check continue thread died while updating\n");
//zMapLogWarning("check continue thread died while updating\n",NULL);
      connections = TRUE ;

      break;
      }
    case ZMAPVIEW_RESETTING:
      {
	/* zmap is ok but user has reset it and all threads have died so we need to stop
	 * checking.... */
	zmap_view->state = ZMAPVIEW_INIT ;

	/* Signal layer above us because the view has reset. */
	(*(view_cbs_G->state_change))(zmap_view, zmap_view->app_data, NULL) ;

	connections = FALSE ;

	break ;
      }
    case ZMAPVIEW_DYING:
      {
	/* Tricky coding here: we need to destroy the view _before_ we notify the layer above us
	 * that the zmap_view has gone but we need to pass information from the view back, so we
	 * keep a temporary record of certain parts of the view. */
	ZMapView zmap_view_ref = zmap_view ;
	void *app_data = zmap_view->app_data ;
	ZMapViewCallbackDestroyData destroy_data ;

	destroy_data = g_new(ZMapViewCallbackDestroyDataStruct, 1) ; /* Caller must free. */
	destroy_data->xwid = zmap_view->xwid ;

	/* view was waiting for threads to die, now they have we can free everything. */
	destroyZMapView(&zmap_view) ;

	/* Signal layer above us that view has died. */
	(*(view_cbs_G->destroy))(zmap_view_ref, app_data, destroy_data) ;

	connections = FALSE ;

	break ;
      }
    default:
      {
	zMapLogFatalLogicErr("switch(), unknown value: %d", zmap_view->state) ;

	break ;
      }
    }

//if(!connections) printf("checkContinue returns FALSE\n");
  return connections ;
}







static gboolean makeStylesDrawable(GData *styles, char **missing_styles_out)
{
  gboolean result = FALSE ;
  DrawableDataStruct drawable_data = {FALSE} ;

  g_datalist_foreach(&styles, drawableCB, &drawable_data) ;

  if (drawable_data.missing_styles)
    *missing_styles_out = g_string_free(drawable_data.missing_styles, FALSE) ;

  result = drawable_data.found_style ;

  return result ;
}



/* A GDataForeachFunc() to make the given style drawable. */
static void drawableCB(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;
  DrawableData drawable_data = (DrawableData)user_data ;

  if (zMapStyleIsDisplayable(style))
    {
      if (zMapStyleMakeDrawable(style))
	{
	  drawable_data->found_style = TRUE ;
	}
      else
	{
	  if (!(drawable_data->missing_styles))
	    drawable_data->missing_styles = g_string_sized_new(1000) ;

	  g_string_append_printf(drawable_data->missing_styles, "%s ", g_quark_to_string(key_id)) ;
	}
    }

  return ;
}



/* A GDataForeachFunc() func that unsets DeferredLoads for styles in the target. */
static void unsetDeferredLoadStylesCB(GQuark key_id, gpointer data, gpointer user_data_unused)
{
  ZMapFeatureTypeStyle orig_style = (ZMapFeatureTypeStyle)data ;

  zMapStyleSetDeferred(orig_style, FALSE) ;

  return ;
}





static void addPredefined(GData **styles_out, GHashTable **featureset_2_stylelist_inout)
{
  GData *styles ;
  GHashTable *f2s = *featureset_2_stylelist_inout ;

  styles = zMapStyleGetAllPredefined() ;

  g_datalist_foreach(&styles, styleCB, f2s) ;

  *styles_out = styles ;
  *featureset_2_stylelist_inout = f2s ;

  return ;
}

/* GDataForeachFunc() to set up a feature_set/style hash. */
static void styleCB(GQuark key_id, gpointer data, gpointer user_data)
{
  GHashTable *hash = (GHashTable *)user_data ;
  GQuark feature_set_id, feature_set_name_id;

  /* We _must_ canonicalise here. */
  feature_set_name_id = key_id ;

  feature_set_id = zMapStyleCreateID((char *)g_quark_to_string(feature_set_name_id)) ;

  zMap_g_hashlist_insert(hash,
			 feature_set_id,
			 GUINT_TO_POINTER(feature_set_id)) ;

  return ;
}



/* Called when the xremote widget for View is mapped, we can then set the X Window id
 * for the xremote widget and then remove the handler so we don't get called anymore. */
static gboolean mapEventCB(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  gboolean handled = FALSE ;				    /* Allow others to handle. */
  ZMapView zmap_view = (ZMapView)user_data ;

  zmap_view->xwid = zMapXRemoteWidgetGetXID(zmap_view->xremote_widget) ;

  g_signal_handler_disconnect(zmap_view->xremote_widget, zmap_view->map_event_handler) ;

  return handled ;
}
