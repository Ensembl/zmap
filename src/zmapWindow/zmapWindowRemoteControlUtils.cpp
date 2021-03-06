/*  File: zmapWindowRemoteControlUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Takes a feature and returns an xml version of it
 *              that when wrapped in the remote protocol envelope
 *              can be sent to a peer.
 *
 * Exported functions: See ZMap/zmap?????.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <glib.h>
#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapXML.hpp>


/*
 * NOT SURE IF THIS FILE IS THE CORRECT PLACE....perhaps it should
 * just be in zmapwindow.....
 *
 *  */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
typedef struct
{
  struct
  {
    unsigned int context    : 1 ;
    unsigned int align      : 1 ;
    unsigned int block      : 1 ;
    unsigned int featureset : 1 ;
    unsigned int feature    : 1 ;
  } doEndTag;
  GArray *xml_events_out;
  ZMapXMLWriter xml_writer;
  ZMapXMLWriterErrorCode status;
  int xml_type;
} XMLContextDumpStruct, *XMLContextDump;


static void generateContextXMLEvents   (ZMapFeatureContext feature_context, XMLContextDump xml_data);
static void generateAlignXMLEvents     (ZMapFeatureAlignment feature_align, XMLContextDump xml_data);
static void generateBlockXMLEvents     (ZMapFeatureBlock feature_block,     XMLContextDump xml_data);
static void generateFeatureSetXMLEvents(ZMapFeatureSet feature_set,         XMLContextDump xml_data);
static void generateFeatureXMLEvents   (ZMapFeature feature,                XMLContextDump xml_data);
static void generateClosingEvents      (ZMapFeatureAny feature_any,         XMLContextDump xml_data);
static void generateFeatureSpanEventsXremote(ZMapFeature feature,           XMLContextDump xml_data);
static void generateContextXMLEndEvent   (XMLContextDump xml_data);
static void generateAlignXMLEndEvent     (XMLContextDump xml_data);
static void generateBlockXMLEndEvent     (XMLContextDump xml_data);
static void generateFeatureSetXMLEndEvent(XMLContextDump xml_data);
static void generateFeatureXMLEndEvent   (XMLContextDump xml_data);

static ZMapFeatureContextExecuteStatus xmlDumpCB(GQuark key, gpointer data, gpointer user_data, char **error);

static gboolean zMapFeatureAnyAsXML(ZMapFeatureAny feature_any,
                                    ZMapXMLWriter xml_writer, GArray **xml_events_out,
                                    int xml_type) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



GArray *addFeatureSet(GArray *xml_array, ZMapFeature feature) ;
GArray *addFeature(GArray *xml_array, ZMapFeature feature) ;
GArray *addTranscriptSubFeatures(GArray *xml_array, ZMapFeature feature) ;



/*
 *            External interface.
 */



/* New function to xml-ise a feature....it is the responsibility of the caller to free
 * the returned array using g_free() when finished with.
 *
 *  */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
ZMapXMLUtilsEventStack zMapFeatureAnyAsXMLEvents(ZMapFeature feature)
{
  ZMapXMLUtilsEventStack feature_xml = NULL ;
  static ZMapXMLUtilsEventStackStruct
    feature_and_set[] = {{ZMAPXML_START_ELEMENT_EVENT, "featureset", ZMAPXML_EVENT_DATA_NONE,    {0}},
                         {ZMAPXML_ATTRIBUTE_EVENT,     "name",       ZMAPXML_EVENT_DATA_QUARK,   {0}},
                         {ZMAPXML_START_ELEMENT_EVENT, "feature",    ZMAPXML_EVENT_DATA_NONE,    {0}},
                         {ZMAPXML_ATTRIBUTE_EVENT,     "name",       ZMAPXML_EVENT_DATA_QUARK,   {0}},
                         {ZMAPXML_ATTRIBUTE_EVENT,     "start",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
                         {ZMAPXML_ATTRIBUTE_EVENT,     "end",        ZMAPXML_EVENT_DATA_INTEGER, {0}},
                         {ZMAPXML_ATTRIBUTE_EVENT,     "strand",     ZMAPXML_EVENT_DATA_QUARK,   {0}},
                         {ZMAPXML_START_ELEMENT_EVENT, "subfeature", ZMAPXML_EVENT_DATA_NONE,    {0}},
                         {ZMAPXML_ATTRIBUTE_EVENT,     "start",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
                         {ZMAPXML_ATTRIBUTE_EVENT,     "end",        ZMAPXML_EVENT_DATA_INTEGER, {0}},
                         {ZMAPXML_ATTRIBUTE_EVENT,     "ontology",   ZMAPXML_EVENT_DATA_QUARK,   {0}},
                         {ZMAPXML_END_ELEMENT_EVENT,   "subfeature", ZMAPXML_EVENT_DATA_NONE,    {0}},
                         {ZMAPXML_END_ELEMENT_EVENT,   "feature",    ZMAPXML_EVENT_DATA_NONE,    {0}},
                         {ZMAPXML_END_ELEMENT_EVENT,   "featureset", ZMAPXML_EVENT_DATA_NONE,    {0}},
                         {0}} ;
  int index ;
  GQuark featureset_id ;


  index = 1 ;
  featureset_id = feature->parent->original_id ;
  feature_and_set[index].value.q = featureset_id ;

  index = 3 ;
  feature_and_set[index++].value.q = feature->original_id ;
  feature_and_set[index++].value.i = feature->x1 ;
  feature_and_set[index++].value.i = feature->x2 ;
  feature_and_set[index++].value.q = g_quark_from_string(zMapFeatureStrand2Str(feature->strand)) ;


  feature_xml = feature_and_set ;

  return feature_xml ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* This function takes a feature and produces an xml version for sending
 * via the xremote protocol.
 *
 * NOTE, the function does not currently add "align" and "block" xml
 * elements as they seem superfluous. They could easily be added in
 * if necessary.
 *
 *  */
ZMapXMLUtilsEventStack zMapFeatureAnyAsXMLEvents(ZMapFeature feature)
{
  ZMapXMLUtilsEventStack feature_xml = NULL ;
  GArray *xml_array ;

  /* Allocate a zero terminated and zero'd array of eventstacks, 100 is probably much bigger than
   * it will ever need to be. */
  xml_array = g_array_sized_new(TRUE, TRUE, sizeof(ZMapXMLUtilsEventStackStruct), 100) ;

  /* Add featureset */
  xml_array = addFeatureSet(xml_array, feature) ;

  /* return just the xml array. */
  feature_xml = (ZMapXMLUtilsEventStack)g_array_free(xml_array, FALSE) ;

  return feature_xml ;
}




GArray *addFeatureSet(GArray *xml_array, ZMapFeature feature)
{
  static ZMapXMLUtilsEventStackStruct
    featureset[] = {{ZMAPXML_START_ELEMENT_EVENT, "featureset", ZMAPXML_EVENT_DATA_NONE,    {0}},
                    {ZMAPXML_ATTRIBUTE_EVENT,     "name",       ZMAPXML_EVENT_DATA_QUARK,   {0}},
                    {ZMAPXML_END_ELEMENT_EVENT,   "featureset", ZMAPXML_EVENT_DATA_NONE,    {0}},
                    {(ZMapXMLWriterEventType)0}} ;
  int index ;
  GQuark featureset_id ;

  /* Add featureset start */
  index = 0 ;
  g_array_append_val(xml_array, featureset[index++]) ;

  featureset_id = feature->parent->original_id ;
  featureset[index].value.q = featureset_id ;
  g_array_append_val(xml_array, featureset[index++]) ;

  /* Add feature */
  xml_array = addFeature(xml_array, feature) ;

  /* add feature end. */
  g_array_append_val(xml_array, featureset[index]) ;

  return xml_array ;
}



GArray *addFeature(GArray *xml_array, ZMapFeature feature)
{
  static ZMapXMLUtilsEventStackStruct
    feature_element[] = {{ZMAPXML_START_ELEMENT_EVENT, "feature",    ZMAPXML_EVENT_DATA_NONE,    {0}},
                 {ZMAPXML_ATTRIBUTE_EVENT,     "name",       ZMAPXML_EVENT_DATA_QUARK,   {0}},
                 {ZMAPXML_ATTRIBUTE_EVENT,     "start",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
                 {ZMAPXML_ATTRIBUTE_EVENT,     "end",        ZMAPXML_EVENT_DATA_INTEGER, {0}},
                 {ZMAPXML_ATTRIBUTE_EVENT,     "strand",     ZMAPXML_EVENT_DATA_QUARK,   {0}},
                 {ZMAPXML_END_ELEMENT_EVENT,   "feature",    ZMAPXML_EVENT_DATA_NONE,    {0}},
                 {(ZMapXMLWriterEventType)0}} ;
  int index ;

  /* Add feature start */
  index = 0 ;
  g_array_append_val(xml_array, feature_element[index++]) ;

  feature_element[index].value.q = feature->original_id ;
  g_array_append_val(xml_array, feature_element[index++]) ;

  feature_element[index].value.i = feature->x1 ;
  g_array_append_val(xml_array, feature_element[index++]) ;


  feature_element[index].value.i = feature->x2 ;
  g_array_append_val(xml_array, feature_element[index++]) ;

  feature_element[index].value.q = g_quark_from_string(zMapFeatureStrand2Str(feature->strand)) ;
  g_array_append_val(xml_array, feature_element[index++]) ;


  /* Add subfeature stuff. */
  if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    xml_array = addTranscriptSubFeatures(xml_array, feature) ;


  /* Add feature end. */
  g_array_append_val(xml_array, feature_element[index]) ;


  return xml_array ;
}



GArray *addTranscriptSubFeatures(GArray *xml_array, ZMapFeature feature)
{
  static ZMapXMLUtilsEventStackStruct
    subfeature[] = {{ZMAPXML_START_ELEMENT_EVENT, "subfeature", ZMAPXML_EVENT_DATA_NONE,    {0}},
                    {ZMAPXML_ATTRIBUTE_EVENT,     "start",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
                    {ZMAPXML_ATTRIBUTE_EVENT,     "end",        ZMAPXML_EVENT_DATA_INTEGER, {0}},
                    {ZMAPXML_ATTRIBUTE_EVENT,     "ontology",   ZMAPXML_EVENT_DATA_QUARK,   {0}},
                    {ZMAPXML_END_ELEMENT_EVENT,   "subfeature", ZMAPXML_EVENT_DATA_NONE,    {0}},
                    {(ZMapXMLWriterEventType)0}} ;
  enum {OPEN_INDEX, START_INDEX, END_INDEX, ONT_INDEX, CLOSE_INDEX} ; /* keep in step with subfeature array ! */
  GArray *span_array = NULL;
  int i;

  /* CDS */
  if (feature->feature.transcript.flags.cds)
    {
      /* subfeature start. */
      g_array_append_val(xml_array, subfeature[OPEN_INDEX]) ;

      /* start */
      subfeature[START_INDEX].value.i = feature->feature.transcript.cds_start ;
      g_array_append_val(xml_array, subfeature[START_INDEX]) ;

      /* end */
      subfeature[END_INDEX].value.i = feature->feature.transcript.cds_end ;
      g_array_append_val(xml_array, subfeature[END_INDEX]) ;

      /* ontology */
      subfeature[ONT_INDEX].value.q = g_quark_from_string("cds") ;
      g_array_append_val(xml_array, subfeature[ONT_INDEX]) ;

      /* subfeature end. */
      g_array_append_val(xml_array, subfeature[CLOSE_INDEX]) ;
    }

  /* subfeature elements. */
  if ((span_array = feature->feature.transcript.exons))
    {
      for (i = 0; i < (int)span_array->len ; i++)
        {
          ZMapSpan exon ;

          /* subfeature start. */
          g_array_append_val(xml_array, subfeature[OPEN_INDEX]) ;

          exon = &(g_array_index(span_array, ZMapSpanStruct, i)) ;

          /* start */
          subfeature[START_INDEX].value.i = exon->x1 ;
          g_array_append_val(xml_array, subfeature[START_INDEX]) ;

          /* end */
          subfeature[END_INDEX].value.i = exon->x2 ;
          g_array_append_val(xml_array, subfeature[END_INDEX]) ;

          /* ontology */
          subfeature[ONT_INDEX].value.q = g_quark_from_string("exon") ;
          g_array_append_val(xml_array, subfeature[ONT_INDEX]) ;

          /* subfeature end. */
          g_array_append_val(xml_array, subfeature[CLOSE_INDEX]) ;
        }
    }

  if ((span_array = feature->feature.transcript.introns))
    {
      for (i = 0; i < (int)span_array->len; i++)
        {
          ZMapSpan intron ;

          /* subfeature start. */
          g_array_append_val(xml_array, subfeature[OPEN_INDEX]) ;

          intron = &(g_array_index(span_array, ZMapSpanStruct, i)) ;

          /* start */
          subfeature[1].value.i = intron->x1 ;
          g_array_append_val(xml_array, subfeature[START_INDEX]) ;

          /* end */
          subfeature[2].value.i = intron->x2;
          g_array_append_val(xml_array, subfeature[END_INDEX]) ;

          /* ontology */
          subfeature[3].value.q = g_quark_from_string("intron") ;
          g_array_append_val(xml_array, subfeature[ONT_INDEX]) ;

          /* subfeature end. */
          g_array_append_val(xml_array, subfeature[CLOSE_INDEX]) ;
        }
    }

  return xml_array ;
}











/* I DON'T THINK WE NEED ALL THIS REALLY..... */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
GArray *zMapFeatureAnyAsXMLEvents(ZMapFeatureAny feature_any,
                                  /* ZMapFeatureXMLType xml_type */
                                  int xml_type)
{
  xml_type = ZMAPFEATURE_XML_XREMOTE;
  GArray *array = NULL;

  zMapFeatureAnyAsXML(feature_any, NULL, &array, xml_type);

  return array;
}




/*
 *            Internal interface.
 */


/* THIS FUNCTION IS NOT CALLED FROM ANYWHERE EXCEPT IN THIS FILE SO I'M MAKING IT STATIC....
 * THE BELOW COMMENTS IMPLY THAT NONE OF THIS IS VERY WELL THOUGHT OUT........ */

/* don't like this interface of either or with xml_writer and xml_events_out!!!
 *
 * Yes...there's quite a lot that needs changing to make things easier in the
 * future....
 *
 *  */
static gboolean zMapFeatureAnyAsXML(ZMapFeatureAny feature_any,
                                    ZMapXMLWriter xml_writer, GArray **xml_events_out,
                                    int xml_type)
{
  XMLContextDumpStruct xmlDumpData = {{0},0};

  xmlDumpData.doEndTag.context =
    xmlDumpData.doEndTag.align =
    xmlDumpData.doEndTag.block =
    xmlDumpData.doEndTag.featureset =
    xmlDumpData.doEndTag.feature = 0;

  xmlDumpData.status = ZMAPXMLWRITER_OK;

  if(xml_events_out && xml_writer)
    zMapLogWarning("Both xml_events and xml_writer specified."
                   " Will Only use xml_writer, xml_events,"
                   " %s","may turn out rubbish.");

  xmlDumpData.xml_events_out =
    g_array_sized_new(FALSE, FALSE,
                      sizeof(ZMapXMLWriterEventStruct), 512);
  xmlDumpData.xml_writer = xml_writer;
  xmlDumpData.xml_type   = xml_type;

  zMapFeatureContextExecuteSubset(feature_any,
                                  ZMAPFEATURE_STRUCT_FEATURE,
                                  xmlDumpCB,
                                  &xmlDumpData);

  /* We need to close some tags here */
  generateClosingEvents(feature_any, &xmlDumpData);

  if(!xml_writer && xml_events_out)
    *xml_events_out = xmlDumpData.xml_events_out;
  else
    g_array_free(xmlDumpData.xml_events_out, TRUE);

  return xmlDumpData.status;
}



ZMapFeatureContextExecuteStatus xmlDumpCB(GQuark key,
                                          gpointer data,
                                          gpointer user_data,
                                          char **error)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureContext feature_context = NULL;
  ZMapFeatureAlignment feature_align = NULL;
  ZMapFeatureBlock     feature_block = NULL;
  ZMapFeatureSet       feature_set   = NULL;
  ZMapFeature          feature_ft    = NULL;
  ZMapFeatureContextExecuteStatus cs = ZMAP_CONTEXT_EXEC_STATUS_OK;

  XMLContextDump xml_data = (XMLContextDump)user_data;

  if(xml_data->status == ZMAPXMLWRITER_OK)
    {
      switch(feature_any->struct_type)
        {
        case ZMAPFEATURE_STRUCT_CONTEXT:
          feature_context = (ZMapFeatureContext)feature_any;

          if(xml_data->doEndTag.context)
            generateContextXMLEndEvent(xml_data);

          generateContextXMLEvents(feature_context, xml_data);

          xml_data->doEndTag.context = 1;
          xml_data->doEndTag.align = xml_data->doEndTag.block = xml_data->doEndTag.featureset =
            xml_data->doEndTag.feature = 0;
          break;

        case ZMAPFEATURE_STRUCT_ALIGN:
          feature_align = (ZMapFeatureAlignment)feature_any;

          if(xml_data->doEndTag.align)
            generateAlignXMLEndEvent(xml_data);

          generateAlignXMLEvents(feature_align, xml_data);

          xml_data->doEndTag.align = 1;
          xml_data->doEndTag.block = xml_data->doEndTag.featureset = xml_data->doEndTag.feature = 0;
          break;

        case ZMAPFEATURE_STRUCT_BLOCK:
          feature_block = (ZMapFeatureBlock)feature_any;
          if(xml_data->doEndTag.block)
            generateBlockXMLEndEvent(xml_data);

          generateBlockXMLEvents(feature_block, xml_data);

          xml_data->doEndTag.block = 1;
          xml_data->doEndTag.featureset = xml_data->doEndTag.feature = 0;
          break;

        case ZMAPFEATURE_STRUCT_FEATURESET:
          feature_set = (ZMapFeatureSet)feature_any;
          if(xml_data->doEndTag.featureset)
            generateFeatureXMLEndEvent(xml_data);

          generateFeatureSetXMLEvents(feature_set, xml_data);

          xml_data->doEndTag.align = xml_data->doEndTag.block = xml_data->doEndTag.featureset = 1;
          xml_data->doEndTag.feature = 0;
          break;

        case ZMAPFEATURE_STRUCT_FEATURE:
          feature_ft = (ZMapFeature)feature_any;
          if(xml_data->doEndTag.feature)
            generateFeatureXMLEndEvent(xml_data);

          generateFeatureXMLEvents(feature_ft, xml_data);

          xml_data->doEndTag.feature = 1;
          break;

        case ZMAPFEATURE_STRUCT_INVALID:
        default:
          zMapWarnIfReached();
          break;
        }
    }

  if(xml_data->status != ZMAPXMLWRITER_OK)
    {
      cs = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
      if(error && !*error)
        *error = g_strdup_printf("Failed to dump xml from feature '%s'.",
                                 g_quark_to_string(feature_any->unique_id));
    }

  return cs;
}

static void generateClosingEvents(ZMapFeatureAny feature_any,
                           XMLContextDump xml_data)
{
  ZMapFeatureStructType start_point = ZMAPFEATURE_STRUCT_FEATURE,
    end_point = ZMAPFEATURE_STRUCT_CONTEXT;
  int i = 0;

  /* HACK.....SEE COMMENTS IN CALLER OF THIS FUNC.... */
  if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
    end_point = ZMAPFEATURE_STRUCT_ALIGN ;
  else
    end_point = feature_any->struct_type;

  if (start_point >= end_point)
    {
      for(i = start_point; i >= end_point; i--)
        {
          switch(i)
            {
            case ZMAPFEATURE_STRUCT_CONTEXT:
              generateContextXMLEndEvent(xml_data);
              break;
            case ZMAPFEATURE_STRUCT_ALIGN:
              generateAlignXMLEndEvent(xml_data);
              break;
            case ZMAPFEATURE_STRUCT_BLOCK:
              generateBlockXMLEndEvent(xml_data);
              break;
            case ZMAPFEATURE_STRUCT_FEATURESET:
              generateFeatureSetXMLEndEvent(xml_data);
              break;
            case ZMAPFEATURE_STRUCT_FEATURE:
              generateFeatureXMLEndEvent(xml_data);
              break;
            case ZMAPFEATURE_STRUCT_INVALID:
            default:
              zMapWarnIfReached();
              break;
            }
        }
    }
  return ;
}


static void generateContextXMLEvents(ZMapFeatureContext feature_context,
                              XMLContextDump xml_data)
{
#ifdef RDS_KEEP_GCC_QUIET
  ZMapXMLWriterEventStruct event = {0};
#endif /* RDS_KEEP_GCC_QUIET */
  switch(xml_data->xml_type)
    {
    case ZMAPFEATURE_XML_XREMOTE:
      zMapLogWarning("%s","ZMAPFEATURE_XML_XREMOTE type has no equivalent to contexts");
      break;
    default:
      zMapLogWarning("%s", "What type of xml did you want?");
      break;
    }

  return ;
}

static void generateAlignXMLEvents(ZMapFeatureAlignment feature_align,
                            XMLContextDump xml_data)
{
#ifdef RDS_KEEP_GCC_QUIET
  ZMapXMLWriterEventStruct event = {0};
#endif /* RDS_KEEP_GCC_QUIET */

  switch(xml_data->xml_type)
    {
    case ZMAPFEATURE_XML_XREMOTE:
      zMapLogWarning("%s","ZMAPFEATURE_XML_XREMOTE type has no equivalent to alignments");
      break;
    default:
      zMapLogWarning("%s", "What type of xml did you want?");
      break;
    }

  if(xml_data->xml_writer &&
     xml_data->xml_events_out->len & 512)
    {
      zMapXMLWriterProcessEvents(xml_data->xml_writer, xml_data->xml_events_out);
    }

  return ;
}

static void generateBlockXMLEvents(ZMapFeatureBlock feature_block,
                            XMLContextDump xml_data)
{
#ifdef RDS_KEEP_GCC_QUIET
  ZMapXMLWriterEventStruct event = {0};
#endif /* RDS_KEEP_GCC_QUIET */
  switch(xml_data->xml_type)
    {
    case ZMAPFEATURE_XML_XREMOTE:
      zMapLogWarning("%s","ZMAPFEATURE_XML_XREMOTE type has no equivalent to blocks");
      break;
    default:
      zMapLogWarning("%s", "What type of xml did you want?");
      break;
    }

  if(xml_data->xml_writer &&
     xml_data->xml_events_out->len & 512)
    {
      zMapXMLWriterProcessEvents(xml_data->xml_writer, xml_data->xml_events_out);
    }

  return ;
}

static void generateFeatureSetXMLEvents(ZMapFeatureSet feature_set,
                                        XMLContextDump xml_data)
{
  ZMapXMLWriterEventStruct event = {0};

  switch(xml_data->xml_type)
    {
    case ZMAPFEATURE_XML_XREMOTE:
      {
        ZMapFeatureAlignment align ;
        ZMapFeatureBlock block ;


        block = (ZMapFeatureBlock) zMapFeatureGetParentGroup((ZMapFeatureAny)feature_set, ZMAPFEATURE_STRUCT_BLOCK) ;
        align = (ZMapFeatureAlignment) zMapFeatureGetParentGroup((ZMapFeatureAny)block, ZMAPFEATURE_STRUCT_ALIGN) ;

        event.type = ZMAPXML_START_ELEMENT_EVENT ;
        event.data.name = g_quark_from_string("align") ;
        xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event) ;

        event.type = ZMAPXML_ATTRIBUTE_EVENT;
        event.data.comp.name  = g_quark_from_string("name");
        event.data.comp.data  = ZMAPXML_EVENT_DATA_QUARK;
        event.data.comp.value.quark = align->unique_id;
        xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event);


        event.type = ZMAPXML_START_ELEMENT_EVENT ;
        event.data.name = g_quark_from_string("block") ;
        xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event) ;

        event.type = ZMAPXML_ATTRIBUTE_EVENT;
        event.data.comp.name  = g_quark_from_string("name");
        event.data.comp.data  = ZMAPXML_EVENT_DATA_QUARK;
        event.data.comp.value.quark = block->unique_id;
        xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event);


        event.type = ZMAPXML_START_ELEMENT_EVENT;
        event.data.name = g_quark_from_string("featureset");
        xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event);

        break;
      }

    default:
      zMapLogWarning("%s", "What type of xml did you want?");
      break;
    }

  if(xml_data->xml_writer &&
     xml_data->xml_events_out->len & 512)
    {
      zMapXMLWriterProcessEvents(xml_data->xml_writer, xml_data->xml_events_out);
    }

  return ;
}

static void generateFeatureXMLEvents(ZMapFeature feature,
                              XMLContextDump xml_data)
{
  ZMapXMLWriterEventStruct event = {0};

  switch(xml_data->xml_type)
    {
    case ZMAPFEATURE_XML_XREMOTE:
      {
        event.type = ZMAPXML_START_ELEMENT_EVENT;
        event.data.name = g_quark_from_string("feature");
        xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event);

        event.type = ZMAPXML_ATTRIBUTE_EVENT;
        event.data.comp.name = g_quark_from_string("name");
        event.data.comp.data = ZMAPXML_EVENT_DATA_QUARK;
        event.data.comp.value.quark = feature->original_id;
        xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event);

        event.data.comp.name = g_quark_from_string("strand");
        event.data.comp.data = ZMAPXML_EVENT_DATA_QUARK;
        event.data.comp.value.quark = g_quark_from_string(zMapFeatureStrand2Str(feature->strand));
        xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event);

        if (*(feature->style))
          {
            event.data.comp.name = g_quark_from_string("style");
            event.data.comp.data = ZMAPXML_EVENT_DATA_QUARK;
            /*! \todo #warning STYLE_ID wrong, needs to be Original id instead */
//           event.data.comp.value.quark = feature->style_id ;
                {
                        ZMapFeatureTypeStyle style = *feature->style;
                        event.data.comp.value.quark = style->unique_id ;
                        // mh17 left this as unique_id so as not to change anything, warning may be valid
                }
            xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event);
          }

        event.data.comp.name = g_quark_from_string("start");
        event.data.comp.data = ZMAPXML_EVENT_DATA_INTEGER;
        event.data.comp.value.integer = feature->x1;
        xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event);

        event.data.comp.name = g_quark_from_string("end");
        event.data.comp.data = ZMAPXML_EVENT_DATA_INTEGER;
        event.data.comp.value.integer = feature->x2;
        xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event);
#ifdef RDS_DONT_INCLUDE_UNUSED
        if(feature->mode == ZMAPSTYLE_MODE_ALIGNMENT)
          generateFeatureAlignmentEventsXremote();
#endif
        if(feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
          generateFeatureSpanEventsXremote(feature, xml_data);
      }
      break;
    default:
      zMapLogWarning("%s", "What type of xml did you want?");
      break;
    }

  if(xml_data->xml_writer &&
     xml_data->xml_events_out->len & 512)
    {
      zMapXMLWriterProcessEvents(xml_data->xml_writer, xml_data->xml_events_out);
    }

  return ;
}




static void generateFeatureSpanEventsXremote(ZMapFeature feature, XMLContextDump xml_data)
{
  GArray *span_array = NULL;
  static ZMapXMLUtilsEventStackStruct elements[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, "subfeature", ZMAPXML_EVENT_DATA_NONE,    {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "start",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "end",        ZMAPXML_EVENT_DATA_INTEGER, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "ontology",   ZMAPXML_EVENT_DATA_QUARK,   {0}},
      {ZMAPXML_END_ELEMENT_EVENT,   "subfeature", ZMAPXML_EVENT_DATA_NONE,    {0}},
      {0}
    };
  ZMapXMLUtilsEventStack event;
  int i;

  /* CDS */
  if (feature->feature.transcript.flags.cds)
    {
      /* start */
      event = &elements[1];
      event->value.i = feature->feature.transcript.cds_start;

      /* end */
      event = &elements[2];
      event->value.i = feature.feature.transcript.cds_end;

      /* ontology */
      event = &elements[3];
      event->value.q = g_quark_from_string("cds") ;

      xml_data->xml_events_out = zMapXMLUtilsAddStackToEventsArrayEnd(xml_data->xml_events_out, &elements[0]);
    }

  if ((span_array = feature->feature.transcript.exons))
    {
      for(i = 0; i < span_array->len; i++)
        {
          ZMapSpan exon;
          exon = &(g_array_index(span_array, ZMapSpanStruct, i));

          /* start */
          event = &elements[1];
          event->value.i = exon->x1;

          /* end */
          event = &elements[2];
          event->value.i = exon->x2;

          /* ontology */
          event = &elements[3];
          event->value.q = g_quark_from_string("exon") ;

          xml_data->xml_events_out = zMapXMLUtilsAddStackToEventsArrayEnd(xml_data->xml_events_out, &elements[0]);
        }
    }

  if((span_array = feature->feature.transcript.introns))
    {
      for(i = 0; i < span_array->len; i++)
        {
          ZMapSpan intron;
          intron = &(g_array_index(span_array, ZMapSpanStruct, i));

          /* start */
          event = &elements[1];
          event->value.i = intron->x1;

          /* end */
          event = &elements[2];
          event->value.i = intron->x2;

          /* ontology */
          event = &elements[3];
          event->value.q = g_quark_from_string("intron") ;

          xml_data->xml_events_out = zMapXMLUtilsAddStackToEventsArrayEnd(xml_data->xml_events_out, &elements[0]);
        }
    }

  return ;
}



static void generateContextXMLEndEvent(XMLContextDump xml_data)
{
  ZMapXMLWriterEventStruct event = {0};

  event.type = ZMAPXML_END_ELEMENT_EVENT;

  switch(xml_data->xml_type)
    {
    case ZMAPFEATURE_XML_XREMOTE:
      break;
    default:
      zMapLogWarning("%s", "What type of xml did you want?");
      break;
    }

  return ;
}

static void generateAlignXMLEndEvent(XMLContextDump xml_data)
{
  ZMapXMLWriterEventStruct event = {0};

  event.type = ZMAPXML_END_ELEMENT_EVENT;

  switch(xml_data->xml_type)
    {
    case ZMAPFEATURE_XML_XREMOTE:
      event.data.name = g_quark_from_string("align");
      xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event);
      break;
    default:
      zMapLogWarning("%s", "What type of xml did you want?");
      break;
    }

  return ;
}

static void generateBlockXMLEndEvent(XMLContextDump xml_data)
{
  ZMapXMLWriterEventStruct event = {0};

  event.type = ZMAPXML_END_ELEMENT_EVENT;

  switch(xml_data->xml_type)
    {
    case ZMAPFEATURE_XML_XREMOTE:
      event.data.name = g_quark_from_string("block");
      xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event);
      break;
    default:
      zMapLogWarning("%s", "What type of xml did you want?");
      break;
    }

  return ;
}

static void generateFeatureSetXMLEndEvent(XMLContextDump xml_data)
{
  ZMapXMLWriterEventStruct event = {0};

  event.type = ZMAPXML_END_ELEMENT_EVENT;

  switch(xml_data->xml_type)
    {
    case ZMAPFEATURE_XML_XREMOTE:
      event.data.name = g_quark_from_string("featureset");
      xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event);
      break;
    default:
      zMapLogWarning("%s", "What type of xml did you want?");
      break;
    }

  return ;
}

static void generateFeatureXMLEndEvent(XMLContextDump xml_data)
{
  ZMapXMLWriterEventStruct event = {0};

  event.type = ZMAPXML_END_ELEMENT_EVENT;

  switch(xml_data->xml_type)
    {
    case ZMAPFEATURE_XML_XREMOTE:
      event.data.name = g_quark_from_string("feature");
      xml_data->xml_events_out = g_array_append_val(xml_data->xml_events_out, event);
      break;
    default:
      zMapLogWarning("%s", "What type of xml did you want?");
      break;
    }

  return ;
}


#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
