/*  Last edited: Jun 18 10:50 2004 (edgrif) */
/* This is a temporary file only to help with testing....it will go once GFF code is combined
 * into the threads etc. code proper.... */

#include <stdio.h>
#include <glib.h>
#include <ZMap/zmapGFF.h>
#include <ZMap/zmapConfig.h>


static GArray *parseGFF(char *filename) ;


GArray *testGetGFF(void)
{
  GArray *features = NULL ;
  ZMapConfig config ;
  char *GFF_file = NULL ;

  if ((config = zMapConfigCreate()))
    {
      ZMapConfigStanzaSet GFF_list = NULL ;
      ZMapConfigStanza GFF_stanza ;
      char *GFF_stanza_name = "GFF" ;
      ZMapConfigStanzaElementStruct GFF_elements[] = {{"filename", ZMAPCONFIG_STRING, {NULL}},
						      {NULL, -1, {NULL}}} ;

      GFF_stanza = zMapConfigMakeStanza(GFF_stanza_name, GFF_elements) ;

      if (zMapConfigFindStanzas(config, GFF_stanza, &GFF_list))
	{
	  ZMapConfigStanza next_log ;
	  
	  /* Get the first GFF stanza found, we will ignore any others. */
	  next_log = zMapConfigGetNextStanza(GFF_list, NULL) ;
	  
	  if ((GFF_file = zMapConfigGetElementString(next_log, "filename")))
	    GFF_file = g_strdup(GFF_file) ;
	  
	  zMapConfigDeleteStanzaSet(GFF_list) ;		    /* Not needed anymore. */
	}
      
      zMapConfigDestroyStanza(GFF_stanza) ;
      
      zMapConfigDestroy(config) ;
    }


  if (GFF_file)
    {
      features = parseGFF(GFF_file) ;
      g_free(GFF_file) ;
    }

  return features ;
}



static GArray *parseGFF(char *filename)
{
  GArray *features = NULL ;
  GIOChannel* gff_file ;
  GError *gff_file_err = NULL ;


  if ((gff_file = g_io_channel_new_file(filename, "r", &gff_file_err)))
    {
      ZMapGFFParser parser ;
      GString* gff_line ;
      GIOStatus status ;
      gsize terminator_pos = 0 ;
      gboolean free_arrays = FALSE ;


      gff_line = g_string_sized_new(2000) ;		    /* Probably not many lines will be >
							       2k chars. */

      parser = zMapGFFCreateParser() ;

      while ((status = g_io_channel_read_line_string(gff_file, gff_line, &terminator_pos,
						     &gff_file_err)) == G_IO_STATUS_NORMAL)
	{

	  *(gff_line->str + terminator_pos) = '\0' ;	    /* Remove terminating newline. */


	  if (!zMapGFFParseLine(parser, gff_line->str))
	    {
	      GError *error = zMapGFFGetError(parser) ;

	      if (!error)
		{
		  printf("WARNING: zMapGFFParseLine() failed with no GError for line %d: %s\n",
			 zMapGFFGetLineNumber(parser), gff_line->str) ;
		}
	      else
		printf("%s\n", (zMapGFFGetError(parser))->message) ;
	    }
	  
	  
	  gff_line = g_string_truncate(gff_line, 0) ;   /* Reset line to empty. */

	}


      /* Try getting the features. */
      features = zmapGFFGetFeatures(parser) ;


      zMapGFFSetFreeOnDestroy(parser, free_arrays) ;
      zMapGFFDestroyParser(parser) ;
      
      
      if (status == G_IO_STATUS_EOF)
	{
	  if (g_io_channel_shutdown(gff_file, FALSE, &gff_file_err) != G_IO_STATUS_NORMAL)
	    {
	      printf("Could not close gff file\n") ;
	      exit(-1) ;
	    }
	}
      else
	{
	  printf("Error reading lines from gff file\n") ;
	  exit(-1) ;
	}

    }


  return features ;
}


