/*  File: zmapFeatureXML.c
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
 * originally written by:
 *
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






#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapXML.h>
#include <zmapFeature_P.h>


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


void generateContextXMLEvents   (ZMapFeatureContext feature_context, XMLContextDump xml_data);
void generateAlignXMLEvents     (ZMapFeatureAlignment feature_align, XMLContextDump xml_data);
void generateBlockXMLEvents     (ZMapFeatureBlock feature_block,     XMLContextDump xml_data);
void generateFeatureSetXMLEvents(ZMapFeatureSet feature_set,         XMLContextDump xml_data);
void generateFeatureXMLEvents   (ZMapFeature feature,                XMLContextDump xml_data);
void generateClosingEvents      (ZMapFeatureAny feature_any,         XMLContextDump xml_data);
void generateFeatureSpanEventsXremote(ZMapFeature feature,           XMLContextDump xml_data);
void generateContextXMLEndEvent   (XMLContextDump xml_data);
void generateAlignXMLEndEvent     (XMLContextDump xml_data);
void generateBlockXMLEndEvent     (XMLContextDump xml_data);
void generateFeatureSetXMLEndEvent(XMLContextDump xml_data);
void generateFeatureXMLEndEvent   (XMLContextDump xml_data);

ZMapFeatureContextExecuteStatus xmlDumpCB(GQuark key, 
                                          gpointer data, 
                                          gpointer user_data, 
                                          char **error);


GArray *zMapFeatureAnyAsXMLEvents(ZMapFeatureAny feature_any, 
                                  /* ZMapFeatureXMLType xml_type */
                                  int xml_type)
{
  xml_type = ZMAPFEATURE_XML_XREMOTE;
  GArray *array = NULL;
  
  zMapFeatureAnyAsXML(feature_any, NULL, &array, xml_type);

  return array;
}

/* don't like this interface of either or with xml_writer and xml_events_out!!!
 * 
 * Yes...there's quite a lot that needs changing to make things easier in the
 * future....
 * 
 *  */
gboolean zMapFeatureAnyAsXML(ZMapFeatureAny feature_any, 
                             ZMapXMLWriter xml_writer,
                             GArray **xml_events_out,
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
          zMapAssertNotReached();
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

void generateClosingEvents(ZMapFeatureAny feature_any, 
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
              zMapAssertNotReached();
              break;
            }
        }
    }
  return ;
}


void generateContextXMLEvents(ZMapFeatureContext feature_context,
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

void generateAlignXMLEvents(ZMapFeatureAlignment feature_align,
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

void generateBlockXMLEvents(ZMapFeatureBlock feature_block,
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

void generateFeatureSetXMLEvents(ZMapFeatureSet feature_set, 
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

void generateFeatureXMLEvents(ZMapFeature feature,
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

        if(feature->style_id)
          {
            event.data.comp.name = g_quark_from_string("style");
            event.data.comp.data = ZMAPXML_EVENT_DATA_QUARK;
#warning STYLE_ID wrong, needs to be Original id instead
            event.data.comp.value.quark = feature->style_id ;
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
        if(feature->type == ZMAPSTYLE_MODE_ALIGNMENT)
          generateFeatureAlignmentEventsXremote();
#endif
        if(feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
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

void generateFeatureSpanEventsXremote(ZMapFeature feature, 
                                      XMLContextDump xml_data)
{
  GArray *span_array = NULL;
  static ZMapXMLUtilsEventStackStruct elements[] = 
    {
      {ZMAPXML_START_ELEMENT_EVENT, "subfeature", ZMAPXML_EVENT_DATA_NONE,    {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "start",      ZMAPXML_EVENT_DATA_INTEGER, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "end",        ZMAPXML_EVENT_DATA_INTEGER, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "ontology",   ZMAPXML_EVENT_DATA_QUARK,   {NULL}},
      {ZMAPXML_END_ELEMENT_EVENT,   "subfeature", ZMAPXML_EVENT_DATA_NONE,    {0}},
      {0}
    };
  ZMapXMLUtilsEventStack event;
  int i;

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
          event->value.s = "exon";
          
          xml_data->xml_events_out = zMapXMLUtilsAddStackToEventsArray(&elements[0], xml_data->xml_events_out);
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
          event->value.s = "intron";
          
          xml_data->xml_events_out = zMapXMLUtilsAddStackToEventsArray(&elements[0], xml_data->xml_events_out);
        }
    }

  return ;
}

void generateContextXMLEndEvent(XMLContextDump xml_data)
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

void generateAlignXMLEndEvent(XMLContextDump xml_data)
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

void generateBlockXMLEndEvent(XMLContextDump xml_data)
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

void generateFeatureSetXMLEndEvent(XMLContextDump xml_data)
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

void generateFeatureXMLEndEvent(XMLContextDump xml_data)
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

