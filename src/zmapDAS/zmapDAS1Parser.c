/*  File: das1Parser.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <ZMap/zmapDAS.h>
#include <ZMap/zmapUtils.h>

/* same as GDestroyNotify... nearly */
typedef void (*ZMapDAS1CallbackDestroyNotify)(gpointer temp_data);

typedef struct _ZMapDAS1ParserStruct
{
  ZMapXMLParser xml;
  gboolean callback_set, caching;
  gpointer callback_data;

  ZMapDAS1QueryType primed_for; /* for sanity only, when looking @ union */

  /* If only need one of these at a time it should be a union! */
  union{
    ZMapDAS1DSNCB         dsn;
    ZMapDAS1DNACB         dna;
    ZMapDAS1EntryPointsCB entry_points;
    ZMapDAS1TypesCB       types;
    ZMapDAS1FeaturesCB    features;
    ZMapDAS1StylesheetCB  stylesheet;
  }callbacks;

  gpointer handler_data;        /* To hold our various data types */
  ZMapDAS1CallbackDestroyNotify destroy_notify;
  
}ZMapDAS1ParserStruct;

typedef struct
{
  ZMapDAS1Parser        das;
  GQuark style_version, category_id, type_id;
#ifdef RDS_DONT_INCLUDE
  ZMapDAS1Stylesheet    stylesheet;
  ZMapDAS1StyleCategory category;
  ZMapDAS1StyleType     type;
#endif
}styleSheetHandlerStruct, *styleSheetHandler;

static void styleSheetHandlerDestroy(gpointer data);
static void prePrepareDASParser(ZMapDAS1Parser das, ZMapDAS1QueryType query);

/* This parser should be clever enough that the type can be opaque. */
/* This parser represents a neat layer to understand the xml creating
 * useful objects for the user. */
/* All errors will be propogated to the xml parser so we don't have an
 * error function */
/* The user shouldn't care about small changes in the xml. Only this 
 * parser need be concerned with those details. The user will still be 
 * passed objects of the same type.  Extending these objects to 
 * accomodate changes, will obviously mean the user needs to consider
 * changing code, but only the removal/renaming of object properties
 * will hit the user hard. */

static gboolean dsnStart(void *userData,
                         ZMapXMLElement element,
                         ZMapXMLParser parser);
static gboolean dsnEnd(void *userData,
                       ZMapXMLElement element,
                       ZMapXMLParser parser);
static gboolean entryPointStart(void *userData,
                                ZMapXMLElement element,
                                ZMapXMLParser parser);
static gboolean entryPointSegEnd(void *userData,
                                 ZMapXMLElement element,
                                 ZMapXMLParser parser);
static gboolean typeEnd(void *userData,
                        ZMapXMLElement element,
                        ZMapXMLParser parser);
static gboolean typeSegEnd(void *userData,
                           ZMapXMLElement element,
                           ZMapXMLParser parser);
static gboolean stylesheetCategoryStart(void *userData,
                                        ZMapXMLElement element,
                                        ZMapXMLParser parser);
static gboolean stylesheetTypeStart(void *userData,
                                    ZMapXMLElement element,
                                    ZMapXMLParser parser);
static gboolean glyphEnd(void *userData, 
                         ZMapXMLElement element,
                         ZMapXMLParser parser);
static gboolean featureEnd(void *userData,
                           ZMapXMLElement element,
                           ZMapXMLParser parser);

static gboolean rootElementEnd(void *userData,
                               ZMapXMLElement element,
                               ZMapXMLParser parser);

/*! 
 * +--------------------------------------------+
 * |  Anything you want to use DAS Parser layer |
 * +--------------------------------------------+
 * |     DAS Parser - exposes das objects ...   |
 * +--------------------------------------------+
 * |  XML Parser - exposes events, Elements...  |
 * +--------------------------------------------+
 * |   Expat - exposes events, simple chars...  |
 * +--------------------------------------------+
 */

/*! 
 * \brief create a das 1 parser which will use the xml parser provided
 * \param xml parser to use
 * \return the das 1 parser
 *
 * Useful to handle all the xml handling.  Pass in a xml parser created 
 * as you see fit, debugging turned on, etc.  Before you call parse buffer
 * just call the correct prepare method.  When you call parse buffer you 
 * will get called back every time this parser has created a object you're
 * interested in... What you can specify filters???  I'm thinking about it.

 * N.B. Please be aware that the data contained in the pointers passed 
 * to users' callbacks will CEASE to exist at the end of their callback.
 * This is mainly to make filtering faster, but also the das objects 
 * passed are likely to be temporary objects used for creating objects of
 * the user's own type.  If the objects are required to be used verbatim
 * then PLEASE memcpy or otherwise copy the objects to your own data 
 * structures!
 *
 */

ZMapDAS1Parser zMapDAS1ParserCreate(ZMapXMLParser parser)
{
  ZMapDAS1Parser das = NULL;

  if((das = g_new0(ZMapDAS1ParserStruct, 1)) != NULL)
    das->xml = parser;

  return das;
}

gboolean zMapDAS1ParserDSNPrepareXMLParser(ZMapDAS1Parser das, 
                                           ZMapDAS1DSNCB dsn_callback, 
                                           gpointer user_data)
{
  gboolean good = FALSE;
  ZMapXMLObjTagFunctionsStruct starts[] = {
    { "dsn",        dsnStart },
    { NULL, NULL }
  };
  ZMapXMLObjTagFunctionsStruct ends[] = {
    { "dsn",        dsnEnd  },
    { "dasdsn",     rootElementEnd },
    { NULL, NULL }
  };

  /* set our state flag */
  prePrepareDASParser(das, ZMAP_DAS1_DSN);
  
  if((good = zMapXMLParserReset(das->xml)))
    {
      zMapXMLParserSetUserData(das->xml, (void *)das);
      zMapXMLParserSetMarkupObjectTagHandlers(das->xml, &starts[0], &ends[0]);
    }

  if(good && dsn_callback)
    {
      das->callback_set  = good;
      das->callbacks.dsn = dsn_callback;
      das->callback_data = user_data;
    }

  return good;
}

gboolean zMapDAS1ParserTypesPrepareXMLParser(ZMapDAS1Parser das, 
                                             ZMapDAS1TypesCB type_callback, 
                                             gpointer user_data)
{
  gboolean good = FALSE;
  ZMapXMLObjTagFunctionsStruct starts[] = {
    { NULL, NULL }
  };
  ZMapXMLObjTagFunctionsStruct ends[] = {
    { "type",       typeEnd        },
    { "segment",    typeSegEnd     },
    //    { "dastypes",   rootElementEnd },
    { NULL, NULL }
  };

  /* set our state flag */
  prePrepareDASParser(das, ZMAP_DAS1_TYPES);

  if((good = zMapXMLParserReset(das->xml)))
    {
      zMapXMLParserSetUserData(das->xml, (void *)das);
      zMapXMLParserSetMarkupObjectTagHandlers(das->xml, &starts[0], &ends[0]);
    }

  if(good && type_callback)
    {
      das->callback_set  = good;
      das->callback_data = user_data;
      das->callbacks.types = type_callback;
    }

  return good;
}

gboolean zMapDAS1ParserDNAPrepareXMLParser(ZMapDAS1Parser das, 
                                           ZMapDAS1DNACB dna_callback, 
                                           gpointer user_data)
{
  gboolean good = FALSE;

  /* set our state flag */
  prePrepareDASParser(das, ZMAP_DAS1_DNA);

  return good;
}
gboolean zMapDAS1ParserEntryPointsPrepareXMLParser(ZMapDAS1Parser das, 
                                                   ZMapDAS1EntryPointsCB entry_point_callback, 
                                                   gpointer user_data)
{
  gboolean good = FALSE;
  ZMapXMLObjTagFunctionsStruct starts[] = {
    { "entry_points", entryPointStart },
    { NULL, NULL }
  };
  ZMapXMLObjTagFunctionsStruct ends[] = {
    { "segment", entryPointSegEnd },
    //    { "dasep",   rootElementEnd },
    { NULL, NULL }
  };

  prePrepareDASParser(das, ZMAP_DAS1_ENTRY_POINTS);
  
  if((good = zMapXMLParserReset(das->xml)))
    {
      zMapXMLParserSetUserData(das->xml, (void *)das);
      zMapXMLParserSetMarkupObjectTagHandlers(das->xml, &starts[0], &ends[0]);
    }

  if(good && entry_point_callback)
    {
      das->callback_set  = good;
      das->callback_data = user_data;
      das->callbacks.entry_points = entry_point_callback;
    }

  return good;
}
gboolean zMapDAS1ParserFeaturesPrepareXMLParser(ZMapDAS1Parser das, 
                                                ZMapDAS1FeaturesCB feature_callback, 
                                                gpointer user_data)
{
  gboolean good = FALSE;
  ZMapXMLObjTagFunctionsStruct starts[] = {
    { NULL, NULL }
  };
  ZMapXMLObjTagFunctionsStruct ends[] = {
    { "feature", featureEnd     },
    { "dasgff",  rootElementEnd },
    { NULL, NULL }
  };

  prePrepareDASParser(das, ZMAP_DAS1_FEATURES);

  if((good = zMapXMLParserReset(das->xml)))
    {
      zMapXMLParserSetUserData(das->xml, (void *)das);
      zMapXMLParserSetMarkupObjectTagHandlers(das->xml, &starts[0], &ends[0]);
    }

  if(good && feature_callback)
    {
      das->callback_set  = good;
      das->callback_data = user_data;
      das->callbacks.features = feature_callback;
    }

  return good;
}
gboolean zMapDAS1ParserStylesheetPrepareXMLParser(ZMapDAS1Parser das, 
                                                  ZMapDAS1StylesheetCB style_callback, 
                                                  gpointer user_data)
{
  gboolean good = FALSE;
  styleSheetHandler handler = NULL;
  ZMapXMLObjTagFunctionsStruct starts[] = {
    { "category", stylesheetCategoryStart },
    { "type",     stylesheetTypeStart     },
    { NULL, NULL }
  };
  ZMapXMLObjTagFunctionsStruct ends[] = {
    { "glyph",    glyphEnd       },
    { "dasstyle", rootElementEnd },
    { NULL, NULL }
  };

  prePrepareDASParser(das, ZMAP_DAS1_STYLESHEET);

  das->handler_data = handler =  g_new0(styleSheetHandlerStruct,1);
  handler->das = das;

  if((good = zMapXMLParserReset(das->xml)))
    {
      zMapXMLParserSetUserData(das->xml, (void *)handler);
      zMapXMLParserSetMarkupObjectTagHandlers(das->xml, &starts[0], &ends[0]);
      das->destroy_notify = styleSheetHandlerDestroy;
    }

  if(good && style_callback)
    {
      das->callback_set  = good;
      das->callback_data = user_data;
      das->callbacks.stylesheet = style_callback;
    }

  return good;
}

void zMapDAS1ParserDestroy(ZMapDAS1Parser das)
{
  /* This makes sure the destroy_notify gets called*/
  prePrepareDASParser(das, ZMAP_DAS_UNKNOWN);

  das->xml = NULL;              /* Not ours to destroy */

  das->callback_data = NULL;    /* Not ours to destroy */
  
  g_free(das);                  /* free our data */

  return ;
}

/* ---------------------------------------------------------
 * |                      INTERNAL                         |
 * ---------------------------------------------------------
 */
/*
 * This  could  possibly be  a  macro.   All  the Prepare  XML  Parser
 * functions do the same to set state for this DAS parser.
 */
static void prePrepareDASParser(ZMapDAS1Parser das, ZMapDAS1QueryType query)
{
  zMapAssert(das);

  das->callback_set  = FALSE;
  das->primed_for    = ZMAP_DAS_UNKNOWN;
  das->callbacks.dsn = NULL;
  das->callback_data = NULL;

  if(das->destroy_notify) 
    (das->destroy_notify)(das->handler_data); 

  /* Make sure they don't get referred 2 again */
  das->destroy_notify = 
    das->handler_data = NULL;

  das->primed_for     = query;

  return ;
}

static void styleSheetHandlerDestroy(gpointer data)
{
  styleSheetHandler handler = (styleSheetHandler)data;
  handler->das = NULL;

#ifdef RDS_DONT_INCLUDE
  /* The way this has to work means it really needs a zMapDAS1StylesheetDestroy */
  /* The GLists aren't cleaned up here! */
  /* Should this struct just contain the ids as quarks?? */
  if(handler->stylesheet)
    g_free(handler->stylesheet);
  if(handler->category)
    g_free(handler->category);
  if(handler->type)
    g_free(handler->type);
#endif

  g_free(handler);

  return ;
}

/* ---------------------------------------------------------
 * |                      XML HANDLERS                     |
 * ---------------------------------------------------------
 *
 * All are static and have prototype of:
 * gboolean (*ZMapXMLMarkupObjectHandler)(void *userData, ZMapXMLElement, ZMapXMLParser);
 *
 * The parser gets setup so that the userData is of type ZMapDAS1Parser.
 * This gives us access to all we need of the das parser and xml parser.
 * The caller will get callback to their registered callbacks at the
 * appropriate times.
 *
 */
static gboolean dsnStart(void *userData,
                         ZMapXMLElement element,
                         ZMapXMLParser parser)
{
  return TRUE;
}

static gboolean dsnEnd(void *userData,
                ZMapXMLElement element,
                ZMapXMLParser parser)
{
  ZMapDAS1Parser das    = (ZMapDAS1Parser)userData;
  ZMapDAS1DSNStruct dsn = {0};
  ZMapDAS1DSN dsn_ptr   = NULL;
  ZMapXMLElement sub_element = NULL;
  ZMapXMLAttribute attribute = NULL;
  gboolean handled = TRUE;

  dsn_ptr = &dsn;
  if(das->caching)
    dsn_ptr = g_new0(ZMapDAS1DSNStruct, 1);

  if((sub_element = zMapXMLElementGetChildByName(element, "source")) != NULL)
    {
      if((attribute = zMapXMLElementGetAttributeByName(sub_element, "id")) != NULL)
        dsn_ptr->source_id = zMapXMLAttributeGetValue(attribute);
      if((attribute = zMapXMLElementGetAttributeByName(sub_element, "version")) != NULL)
        dsn_ptr->version = zMapXMLAttributeGetValue(attribute);
      dsn_ptr->name = g_quark_from_string(sub_element->contents->str);
    }
  
  if((sub_element = zMapXMLElementGetChildByName(element, "mapmaster")) != NULL)
    dsn_ptr->map = g_quark_from_string(sub_element->contents->str);
  
  if((sub_element = zMapXMLElementGetChildByName(sub_element, "description")) != NULL)
    dsn_ptr->desc = g_quark_from_string(sub_element->contents->str);
  
  if(dsn_ptr->source_id != 0)
    {
      /* quick sanity check */
      if(das->callback_set && das->primed_for == ZMAP_DAS1_DSN)
        (das->callbacks.dsn)(dsn_ptr, das->callback_data);
    }
  else
    zMapXMLParserRaiseParsingError(parser, "Failed to get an id for dsn.");
  
  return handled;
}

static gboolean entryPointStart(void *userData,
                                ZMapXMLElement element,
                                ZMapXMLParser parser)
{
  ZMapDAS1Parser das = (ZMapDAS1Parser)userData;
  ZMapDAS1EntryPointStruct entry_point = {0};
  ZMapXMLAttribute attr;

  if((attr = zMapXMLElementGetAttributeByName(element, "href")))
    entry_point.href = zMapXMLAttributeGetValue(attr);
  if((attr = zMapXMLElementGetAttributeByName(element, "version")))
    entry_point.version = zMapXMLAttributeGetValue(attr);

  /* quick sanity check */
  if(das->callback_set && das->primed_for == ZMAP_DAS1_ENTRY_POINTS)
    (das->callbacks.entry_points)(&entry_point, das->callback_data);

  return TRUE;
}

static gboolean entryPointSegEnd(void *userData,
                                 ZMapXMLElement element,
                                 ZMapXMLParser parser)
{
  ZMapDAS1Parser das = (ZMapDAS1Parser)userData;
  gboolean good = TRUE;
  ZMapDAS1EntryPointStruct entry_point = {0};
  ZMapDAS1SegmentStruct segment = {0};
  ZMapXMLAttribute attr;
  GList list = {NULL};
  int start = 0 , stop = 0, size = 0;

  segment.orientation = g_quark_from_string("+"); /* optional and assumed positive */
  if((attr = zMapXMLElementGetAttributeByName(element, "id")))
    segment.id = zMapXMLAttributeGetValue(attr);
  if((attr = zMapXMLElementGetAttributeByName(element, "orientation")))
    segment.orientation = zMapXMLAttributeGetValue(attr);

  if((attr = zMapXMLElementGetAttributeByName(element, "start")))
    start = zMapXMLAttributeValueToInt(attr);

  if((attr = zMapXMLElementGetAttributeByName(element, "stop")))
    stop = zMapXMLAttributeValueToInt(attr);

  if((attr = zMapXMLElementGetAttributeByName(element, "size")))
    size = zMapXMLAttributeValueToInt(attr);
  /* Some have class others have type.......... */
  if((attr = zMapXMLElementGetAttributeByName(element, "class")))
    segment.type = zMapXMLAttributeGetValue(attr);
  else if((attr = zMapXMLElementGetAttributeByName(element, "type")))
    segment.type = zMapXMLAttributeGetValue(attr);

#ifdef RDS_DONT_INCLUDE  
  if((attr = zMapXMLElementGetAttributeByName(element, "reference")))
    segment.ref = (char *)g_quark_to_string( zMapXMLAttributeGetValue(attr) );
  if((attr = zMapXMLElementGetAttributeByName(element, "subparts")))
    segment.sub = (char *)g_quark_to_string( zMapXMLAttributeGetValue(attr) );

  if((seg = dasOneSegment_create1(id, start, stop, type, orientation)) != NULL)
    {
      dasOneSegment_refProperties(seg, ref, sub, TRUE);
    }
#endif

  segment.desc = g_quark_from_string(element->contents->str);

  /* For Old skool */
  if((!start || !stop) && size)
    { start = 1; stop = size; }

  segment.start = start;
  segment.stop  = stop;

  entry_point.segments = &list;
  list.data = &segment;

  /* quick sanity check */
  if(das->callback_set && das->primed_for == ZMAP_DAS1_ENTRY_POINTS)
    (das->callbacks.entry_points)(&entry_point, das->callback_data);

  return good;
}

static gboolean typeEnd(void *userData,
                        ZMapXMLElement element,
                        ZMapXMLParser parser)
{
  ZMapDAS1Parser das = (ZMapDAS1Parser)userData;
  ZMapDAS1TypeStruct type = {0};
  ZMapXMLAttribute attr;

  if((attr = zMapXMLElementGetAttributeByName(element, "id")))
    type.type_id  = zMapXMLAttributeGetValue(attr);
  if((attr = zMapXMLElementGetAttributeByName(element, "method")))
    type.method   = zMapXMLAttributeGetValue(attr);
  if((attr = zMapXMLElementGetAttributeByName(element, "category")))
    type.category = zMapXMLAttributeGetValue(attr);
  if((attr = zMapXMLElementGetAttributeByName(element, "source")))
    type.source   = zMapXMLAttributeGetValue(attr);

  if(das->callback_set && das->primed_for == ZMAP_DAS1_TYPES)
    (das->callbacks.types)(&type, das->callback_data);
  
  return TRUE;
}

static gboolean typeSegEnd(void *userData,
                           ZMapXMLElement element,
                           ZMapXMLParser parser)
{
  gboolean handled = TRUE;


  return handled;
}

static gboolean stylesheetCategoryStart(void *userData,
                                        ZMapXMLElement element,
                                        ZMapXMLParser parser)
{
  styleSheetHandler data = (styleSheetHandler)userData;
  ZMapXMLAttribute attr  = NULL;
  gboolean handled = TRUE;
#ifdef RDS_DONT_INCLUDE
  if(data->category)
    g_free(data->category);

  data->category = g_new0(ZMapDAS1StyleCategoryStruct, 1);
#endif
  if((attr = zMapXMLElementGetAttributeByName(element, "id")) != NULL)
    data->category_id = zMapXMLAttributeGetValue(attr);
  else
    zMapXMLParserRaiseParsingError(parser, "id is a required attribute for category");

  return handled;
}

static gboolean stylesheetTypeStart(void *userData,
                                    ZMapXMLElement element,
                                    ZMapXMLParser parser)
{
  styleSheetHandler data = (styleSheetHandler)userData;
  ZMapXMLAttribute attr  = NULL;
  gboolean handled = TRUE;
#ifdef RDS_DONT_INCLUDE
  if(data->type)
    g_free(data->type);

  data->type = g_new0(ZMapDAS1StyleTypeStruct, 1);
#endif
  if((attr = zMapXMLElementGetAttributeByName(element, "id")) != NULL)
    data->type_id = zMapXMLAttributeGetValue(attr);
  else
    zMapXMLParserRaiseParsingError(parser, "id is a required attribute for type");

  return handled;
}

static gboolean glyphEnd(void *userData, 
                         ZMapXMLElement element,
                         ZMapXMLParser parser)
{
  styleSheetHandler data = (styleSheetHandler)userData;
  ZMapXMLElement sub_element = NULL, common_ele = NULL, specific_ele = NULL;
  ZMapDAS1GlyphStruct glyph = {0};
  ZMapDAS1StylesheetStruct style = {0};
  ZMapDAS1StyleTypeStruct type = {0};
  ZMapDAS1StyleCategoryStruct category = {0};
  GList categories = {0}, types = {0}, glyphs = {0};

  zMapXMLParserCheckIfTrueErrorReturn(data->category_id == 0, parser, "glyph must be part of a category");
  zMapXMLParserCheckIfTrueErrorReturn(data->type_id     == 0, parser, "glyph must be part of a type"); 

  style.categories = &categories; /* GList */
  categories.data  = &category; /* Contents of GList */
  category.id      = data->category_id;
  category.types   = &types;   /* GList */
  types.data       = &type;    /* Contents of GList */
  type.id          = data->type_id;
  type.glyphs      = &glyphs;  /* GList */
  glyphs.data      = &glyph;   /* Contents of GList */


  if((sub_element = zMapXMLElementGetChildByName(element, "arrow")) != NULL)
    glyph.type = ZMAP_DAS1_ARROW_GLYPH;
  else if((sub_element = zMapXMLElementGetChildByName(element, "anchored_arrow")) != NULL)
    glyph.type = ZMAP_DAS1_ANCHORED_ARROW_GLYPH;
  else if((sub_element = zMapXMLElementGetChildByName(element, "box")) != NULL)
    glyph.type = ZMAP_DAS1_BOX_GLYPH;
  else if((sub_element = zMapXMLElementGetChildByName(element, "cross")) != NULL)
    glyph.type = ZMAP_DAS1_CROSS_GLYPH;
  else if((sub_element = zMapXMLElementGetChildByName(element, "dot")) != NULL)
    glyph.type = ZMAP_DAS1_DOT_GLYPH;
  else if((sub_element = zMapXMLElementGetChildByName(element, "ex")) != NULL)
    glyph.type = ZMAP_DAS1_EX_GLYPH;
  else if((sub_element = zMapXMLElementGetChildByName(element, "hidden")) != NULL)
    glyph.type = ZMAP_DAS1_HIDDEN_GLYPH;
  else if((sub_element = zMapXMLElementGetChildByName(element, "line")) != NULL)
    glyph.type = ZMAP_DAS1_LINE_GLYPH;
  else if((sub_element = zMapXMLElementGetChildByName(element, "span")) != NULL)
    glyph.type = ZMAP_DAS1_SPAN_GLYPH;
  else if((sub_element = zMapXMLElementGetChildByName(element, "text")) != NULL)
    glyph.type = ZMAP_DAS1_TEXT_GLYPH;
  else if((sub_element = zMapXMLElementGetChildByName(element, "toomany")) != NULL)
    glyph.type = ZMAP_DAS1_TOOMANY_GLYPH;
  else if((sub_element = zMapXMLElementGetChildByName(element, "triangle")) != NULL)
    glyph.type = ZMAP_DAS1_TRIANGLE_GLYPH;
  else if((sub_element = zMapXMLElementGetChildByName(element, "primers")) != NULL)
    glyph.type = ZMAP_DAS1_PRIMERS_GLYPH;
  else
    glyph.type = ZMAP_DAS1_NONE_GLYPH;

  if(sub_element)
    {
      /* height, fgcolor, bgcolor, label and bump are common. unfortunately they are all elements! */
      if(((common_ele = zMapXMLElementGetChildByName(sub_element, "height")) != NULL) &&
         common_ele->contents->str)       /* int */
        glyph.height  = zMapXMLElementContentsToInt(common_ele);

      if(((common_ele = zMapXMLElementGetChildByName(sub_element, "fgcolor")) != NULL) &&
         common_ele->contents->str) /* colour */
        glyph.fg      = g_quark_from_string(common_ele->contents->str);

      if(((common_ele = zMapXMLElementGetChildByName(sub_element, "bgcolor")) != NULL) &&
         common_ele->contents->str) /* colour */
        glyph.bg      = g_quark_from_string(common_ele->contents->str);

      if((common_ele = zMapXMLElementGetChildByName(sub_element, "label")) != NULL)
        glyph.label  = zMapXMLElementContentsToBool(common_ele);    /* Boolean */

      if((common_ele = zMapXMLElementGetChildByName(sub_element, "bump")) != NULL)
        glyph.bump   = zMapXMLElementContentsToBool(common_ele);   /* Boolean */
    }

  /* handle the specific attributes */
  switch(glyph.type)
    {
    case ZMAP_DAS1_ARROW_GLYPH:
    case ZMAP_DAS1_ANCHORED_ARROW_GLYPH:
      if((specific_ele = zMapXMLElementGetChildByName(sub_element, "parallel")) != NULL)
        glyph.shape.arrow.parallel = zMapXMLElementContentsToBool(common_ele);
      break;
    case ZMAP_DAS1_BOX_GLYPH:
      if((specific_ele = zMapXMLElementGetChildByName(sub_element, "linewidth")) != NULL)
        glyph.shape.box.linewidth = zMapXMLElementContentsToInt(specific_ele);
      break;
    case ZMAP_DAS1_LINE_GLYPH:
      if((specific_ele = zMapXMLElementGetChildByName(sub_element, "style")) != NULL)
        {
          if(g_ascii_strncasecmp(common_ele->contents->str, "hat", 3) == 0)
            glyph.shape.line.style = ZMAP_DAS1_LINE_HAT_STYLE;
          else if(g_ascii_strncasecmp(common_ele->contents->str, "dashed", 6) == 0)
            glyph.shape.line.style = ZMAP_DAS1_LINE_DASHED_STYLE;
        }
      break;
    case ZMAP_DAS1_TEXT_GLYPH:
      if((specific_ele = zMapXMLElementGetChildByName(sub_element, "font")) != NULL)
        glyph.shape.text.font      = specific_ele->contents->str;
      if((specific_ele = zMapXMLElementGetChildByName(sub_element, "fontsize")) != NULL)
        glyph.shape.text.font_size = zMapXMLElementContentsToInt(specific_ele);
      if((specific_ele = zMapXMLElementGetChildByName(sub_element, "string")) != NULL)
        glyph.shape.text.string    = specific_ele->contents->str;
      /* multiple style elements possible! */
      if((specific_ele = zMapXMLElementGetChildByName(sub_element, "style")) != NULL)
        glyph.shape.text.style     = zMapXMLElementContentsToInt(specific_ele);
      break;
    case ZMAP_DAS1_TOOMANY_GLYPH:
      if((specific_ele = zMapXMLElementGetChildByName(sub_element, "linewidth")) != NULL)
        glyph.shape.toomany.linewidth = zMapXMLElementContentsToInt(specific_ele);
      break;
    case ZMAP_DAS1_TRIANGLE_GLYPH:
      if((specific_ele = zMapXMLElementGetChildByName(sub_element, "linewidth")) != NULL)
        glyph.shape.triangle.linewidth = zMapXMLElementContentsToInt(specific_ele);
      if((specific_ele = zMapXMLElementGetChildByName(sub_element, "direction")) != NULL)
        glyph.shape.triangle.direction = "N|S|E|W";
      break;
      /* The following have no specific attributes... */
    case ZMAP_DAS1_CROSS_GLYPH:
    case ZMAP_DAS1_DOT_GLYPH:
    case ZMAP_DAS1_EX_GLYPH:
    case ZMAP_DAS1_HIDDEN_GLYPH:
    case ZMAP_DAS1_SPAN_GLYPH:
    case ZMAP_DAS1_PRIMERS_GLYPH:
    default:
      break;
    }

  if(glyph.type != ZMAP_DAS1_NONE_GLYPH)
    {
      /* quick sanity check */
      if(data->das->callback_set && data->das->primed_for == ZMAP_DAS1_STYLESHEET)
        (data->das->callbacks.stylesheet)(&style, data->das->callback_data);
    }

  return TRUE;
}
static gboolean makeGroupsFromFeature(ZMapXMLParser parser, 
                                      ZMapXMLElement feature_element, 
                                      GList *stack_list, 
                                      ZMapDAS1Group groups,
                                      int stack_list_length)
{
  GList *list_ptr = stack_list, *prev = NULL;
  GList *group_elements      = NULL;
  ZMapXMLElement sub_element = NULL;
  ZMapXMLAttribute attr      = NULL;
  ZMapDAS1Group group_ptr    = NULL;
  int groups_found = 0, 
    current_group  = 0;

  /* Check this works, and we don't get one too many! */
  group_elements = zMapXMLElementGetChildrenByName(feature_element, 
                                                   g_quark_from_string("group"), 
                                                   stack_list_length);
  groups_found = g_list_length(group_elements);
  group_ptr    = groups;

  while((group_elements != NULL))
    {
      sub_element = (ZMapXMLElement)(group_elements->data);
      if((attr = zMapXMLElementGetAttributeByName(sub_element, "id")) != NULL)
        group_ptr->group_id = zMapXMLAttributeGetValue(attr);
      if((attr = zMapXMLElementGetAttributeByName(sub_element, "type")) != NULL)
        group_ptr->type     = zMapXMLAttributeGetValue(attr);

      list_ptr->data = (gpointer)group_ptr;
      list_ptr->prev = prev;

      if(prev)
        prev->next   = list_ptr;
      
      prev = list_ptr;

      current_group++;
      group_elements = group_elements->next;
      group_ptr++;
    }

  return TRUE;
}
static void printGroupInfo(gpointer data, gpointer user_data)
{
  ZMapDAS1Group group = (ZMapDAS1Group)data;
  
  printf("[printGroupInfo] group id=%s, type=%s\n", 
         (char *)g_quark_to_string(group->group_id),
         (char *)g_quark_to_string(group->type));
  
  return ;
}

#define MAX_GROUP_NO 5
static gboolean featureEnd(void *userData,
                           ZMapXMLElement element,
                           ZMapXMLParser parser)
{
  ZMapDAS1Parser das = (ZMapDAS1Parser)userData;
  ZMapDAS1FeatureStruct feature = {0};
  ZMapDAS1TypeStruct type = {0};
  ZMapXMLElement sub_element = NULL;
  ZMapXMLAttribute attr = NULL;
  GList          groups_list[MAX_GROUP_NO] = {{0}}; /* Is this right? */
  ZMapDAS1GroupStruct groups[MAX_GROUP_NO] = {{0}};
  gboolean debug_groups = FALSE;

  feature.type  = &type;

  /* Feature element attributes */
  if((attr = zMapXMLElementGetAttributeByName(element, "id")) != NULL)
    feature.feature_id = zMapXMLAttributeGetValue(attr);
  if((attr = zMapXMLElementGetAttributeByName(element, "label")) != NULL)
    feature.label = zMapXMLAttributeGetValue(attr);

  /* Sub elements */
  if((sub_element = zMapXMLElementGetChildByName(element, "type")) != NULL)
    {
      if((attr = zMapXMLElementGetAttributeByName(sub_element, "id")) != NULL)
        feature.type_id = zMapXMLAttributeGetValue(attr);
    }
  if((sub_element = zMapXMLElementGetChildByName(element, "method")) != NULL)
    {
      if((attr = zMapXMLElementGetAttributeByName(sub_element, "id")) != NULL)
        feature.method_id = zMapXMLAttributeGetValue(attr);
    }
  
  if((sub_element = zMapXMLElementGetChildByName(element, "start")) != NULL)
    feature.start = zMapXMLElementContentsToInt(sub_element);
  if((sub_element = zMapXMLElementGetChildByName(element, "end")) != NULL)
    feature.end   = zMapXMLElementContentsToInt(sub_element);
  if((sub_element = zMapXMLElementGetChildByName(element, "score")) != NULL)
    feature.score = zMapXMLElementContentsToDouble(sub_element);
  if((sub_element = zMapXMLElementGetChildByName(element, "orientation")) != NULL)
    feature.orientation = g_quark_from_string(sub_element->contents->str);
  if((sub_element = zMapXMLElementGetChildByName(element, "phase")) != NULL)
    feature.phase = zMapXMLElementContentsToInt(sub_element);
    
  /* This needs to cope with multiple, make into a function.
   * ptototype will be:
   * possibly doing
   */
  makeGroupsFromFeature(parser, element, 
                        groups_list, groups,
                        MAX_GROUP_NO);

  if(debug_groups)
    g_list_foreach(&groups_list[0], printGroupInfo, NULL);

  feature.groups = &groups_list[0];

  if(feature.start != 0 && feature.start != feature.end)
    {
      /* quick sanity check */

      if(das->callback_set && das->primed_for == ZMAP_DAS1_FEATURES)
        (das->callbacks.features)(&feature, das->callback_data);
    }

  return TRUE;
}


/* This could probably make it into the zmapXML package... */
static gboolean rootElementEnd(void *userData,
                               ZMapXMLElement element,
                               ZMapXMLParser parser)
{
  /* We should always return TRUE! */
  return TRUE;
}



