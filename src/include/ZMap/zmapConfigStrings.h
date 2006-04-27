/*  File: zmapConfigStrings.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2006
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
 * Description: Repository for all the configuration file stanza
 *              keywords. We don't have to have this but its good for
 *              documentation and also good for discipline.
 *
 * HISTORY:
 * Last edited: Apr 25 17:15 2006 (edgrif)
 * Created: Tue Apr 25 14:36:16 2006 (edgrif)
 * CVS info:   $Id: zmapConfigStrings.h,v 1.1 2006-04-27 08:08:01 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONFIG_STRINGS_H
#define ZMAP_CONFIG_STRINGS_H


/*! @defgroup zmapstanza   ZMap Configuration Stanzas
 * @{
 * 
 * \brief  ZMap Configuration File Stanzas and their meaning.
 * 
 * ZMap reads stanzas from configuration files, in particular from $HOME/.ZMap/ZMap.
 * These stanzas control various aspects of how ZMap behaves.
 * 
 * The configuration file(s) for ZMap are a series of stanzas of the form:
 * 
 * <PRE>
 * stanza_name                e.g.     server
 * {                                   {
 * resource = value                    host = "griffin"
 * resource = value                    port = 18100
 * }                                   protocol = "acedb"
 *                                     }
 *  </PRE>
 * 
 * Resources may take boolean, integer, float or string values which must be given
 * in the following formats:
 *
 * String values must be enclosed in double quotes, e.g. host = "griffin"
 *
 * Boolean values must be specified as either true or false, e.g. logging = false
 * 
 * Float values must be given in floating point format, e.g. width = 10.0
 *
 * Integer values must be given in integer format, e.g. port = 999
 *
 *  */




/*! @defgroup zmap_stanza   ZMap stanza
 * @{
 * 
 * \brief  Configuration File Stanza for whole ZMap application.
 * 
 *  */

/*!
 * Stanza name.
 */
#define ZMAPSTANZA_APP_CONFIG "ZMap"

/*!
 *  "show_mainwindow", type = BOOL, default value = true
 */
#define ZMAPSTANZA_APP_MAINWINDOW "show_mainwindow"

/*!
 *  "default_sequence", type = STRING, default value = ""
 */
#define ZMAPSTANZA_APP_SEQUENCE "default_sequence"

/*!
 *  "default_printer", type = STRING, default value = ""
 */
#define ZMAPSTANZA_APP_PRINTER "default_printer"


/*! @} end of zmap stanza group. */





/*! @defgroup zmap_stanza   Style stanza
 * @{
 * 
 * \brief  Configuration File Stanza for specifying styles for displaying
 *         sets of features.
 * 
 *  */

/*!
 * Stanza name.
 */
#define ZMAPSTANZA_STYLE_CONFIG "Type"


/*!
 * "name", type = STRING, default value = ""
 */
#define ZMAPSTANZA_STYLE_NAME "name"
	   
/*!
 * "outline", type = STRING, default value = "black"
 */
#define ZMAPSTANZA_STYLE_OUTLINE "outline"
	   
/*!
 * "foreground", type = STRING, default value = "white"
 */
#define ZMAPSTANZA_STYLE_FOREGROUND "foreground"
	   
/*!
 * "background", type = STRING, default value = "black"
 */
#define ZMAPSTANZA_STYLE_BACKGROUND "background"
	   
/*!
 * "width"       , type = FLOAT , default value = 10.0
 */
#define ZMAPSTANZA_STYLE_WIDTH "width"
	   
/*!
 * "show_reverse", type = BOOL  , default value = false
 */
#define ZMAPSTANZA_STYLE_REVERSE "show_reverse"
	   
/*!
 * "strand_specific", type = BOOL  , default value = false
 */
#define ZMAPSTANZA_STYLE_SPECIFIC "strand_specific"
	   
/*!
 * "frame_specific", type = BOOL  , default value = false
 */
#define ZMAPSTANZA_STYLE_SPECIFIC "frame_specific"
	   
/*!
 * "minmag"      , type = INT, default value = 0
 */
#define ZMAPSTANZA_STYLE_MINMAG "minmag"
	   
/*!
 * "maxmag"      , type = INT, default value = 0
 */
#define ZMAPSTANZA_STYLE_MAXMAG "maxmag"
	   
/*!
 * "bump"      , type = STRING, default value = ""
 */
#define ZMAPSTANZA_STYLE_BUMP "bump"
	   
/*!
 * "gapped_align", type = BOOL, default value = false
 */
#define ZMAPSTANZA_STYLE_ALIGN "gapped_align"
	   
/*!
 * "read_gaps"   , type = BOOL, default value = false
 */
#define ZMAPSTANZA_STYLE_GAPS "read_gaps"

/*! @} end of zmap stanza group. */



/*! @defgroup zmap_stanza   ZMap stanza
 * @{
 * 
 * \brief  Configuration File Stanza for whole ZMap application.
 * 
 *  */

/*!
 * Stanza name.
 */
#define ZMAPSTANZA_LOG_CONFIG "logging" 


/*!
 * "logging", type = BOOL, default value = true
 */
#define ZMAPSTANZA_LOG_LOGGING "logging"

/*!
 * "file", type = BOOL, default value = true
 */
#define ZMAPSTANZA_LOG_FILE "file"

/*!
 * "directory", type = STRING, default value = ""
 */
#define ZMAPSTANZA_LOG_DIRECTORY "directory"

/*!
 * "filename", type = STRING, default value = ""
 */
#define ZMAPSTANZA_LOG_FILENAME "filename"


/*! @} end of zmap stanza group. */




/*! @defgroup zmap_stanza   ZMap stanza
 * @{
 * 
 * \brief  Configuration File Stanza for whole ZMap application.
 * 
 *  */

/*!
 * Stanza name.
 */
#define ZMAPSTANZA_DEBUG_CONFIG "debug"


/*!
 *  "threads", type =  BOOL, = FALSE
 */
#define ZMAPSTANZA_DEBUG_APP_THREADS "threads"


/*! @} end of zmap stanza group. */




/*! @defgroup zmap_stanza   Source stanza
 * @{
 * 
 * \brief  Configuration File Stanza for data sources, these are specified as URLs
 *         and hence can be files, http links or other server protocols.
 * 
 *  */

/*!
 * Stanza name.
 */
#define ZMAPSTANZA_SOURCE_CONFIG "source"


/*!
 * "url", type = STRING, default value = ""
 * need to explain url format ???
 */
#define ZMAPSTANZA_SOURCE_URL "url"

 
/*!
 * "timeout", type = INT, default value = 120
 */
#define ZMAPSTANZA_SOURCE_TIMEOUT "timeout"
    
/*!
 * "version", type = STRING, default value = ""
 */
#define ZMAPSTANZA_SOURCE_VERSION "version"
    
/*!
 * "sequence", type = BOOL, default value = false
 */
#define ZMAPSTANZA_SOURCE_SEQUENCE "sequence"

/*!
 * "writeback", type = BOOL, default value = false
 */
#define ZMAPSTANZA_SOURCE_WRITEBACK "writeback"

/*!
 * "stylesfile", type = STRING, default value = ""
 */
#define ZMAPSTANZA_SOURCE_STYLESFILE "stylesfile"
    
/*!
 * "format", type = STRING, default value = ""
 */
#define ZMAPSTANZA_SOURCE_FORMAT "format"
    
/*!
 * "featuresets", type = STRING, default value = ""
 */
#define ZMAPSTANZA_SOURCE_FEATURESETS "featuresets"

/*! @} end of zmap stanza group. */






/*! @defgroup zmap_stanza   Align stanza
 * @{
 * 
 * \brief  Configuration File Stanza for specifying alignments between
 *         several sequences, all of which should be fetched and displayed within
 *         a single zmap.
 * 
 *  */

/*!
 * Stanza name.
 */
#define ZMAPSTANZA_ALIGN_CONFIG "align" 


/*!
 * "reference_seq"        , type = STRING , default value = ""
 */
#define ZMAPSTANZA_ALIGN_SEQ "reference_seq"
/*!
 * "reference_start"      , type = INT    , default value = 0
 */
#define ZMAPSTANZA_ALIGN_START "reference_start"
/*!
 * "reference_end"        , type = INT    , default value = 0
 */
#define ZMAPSTANZA_ALIGN_END "reference_end"
/*!
 * "reference_strand"     , type = INT    , default value = 0
 */
#define ZMAPSTANZA_ALIGN_STRAND "reference_strand"
/*!
 * "non_reference_seq"    , type = STRING , default value = ""
 */
#define ZMAPSTANZA_ALIGN_SEQ "non_reference_seq"
/*!
 * "non_reference_start"  , type = INT    , default value = 0
 */
#define ZMAPSTANZA_ALIGN_START "non_reference_start"
/*!
 * "non_reference_end"    , type = INT    , default value = 0
 */
#define ZMAPSTANZA_ALIGN_END "non_reference_end"
/*!
 * "non_reference_strand" , type = INT    , default value = 0
 */
#define ZMAPSTANZA_ALIGN_STRAND "non_reference_strand"

/*! @} end of zmap stanza group. */



/*! @defgroup zmap_stanza   ZMap stanza
 * @{
 * 
 * \brief  Configuration File Stanza for whole ZMap application.
 * 
 *  */

/*!
 * Stanza name.
 */
#define ZMAPSTANZA_WINDOW_CONFIG "ZMapWindow"

/*!
 * "canvas_maxsize", type = INT, default value = 30000
 */
#define ZMAPSTANZA_WINDOW_MAXSIZE "canvas_maxsize"
/*!
 * "canvas_maxbases", type = INT, default value = 0
 */
#define ZMAPSTANZA_WINDOW_MAXBASES "canvas_maxbases"
/*!
 * "keep_empty_columns", type = BOOL, default value = false
 */
#define ZMAPSTANZA_WINDOW_COLUMNS "keep_empty_columns"

/*!
 * "colour_root", type = STRING, default value = "white"
 */
#define ZMAPSTANZA_WINDOW_"colour_root"

/*!
 * "colour_alignment", type = STRING, default value = "white"
 */
#define ZMAPSTANZA_WINDOW_ALIGNMENT "colour_alignment"

/*!
 * "colour_block", type = STRING, default value = "white"
 */
#define ZMAPSTANZA_WINDOW_BLOCK "colour_block"

/*!
 * "colour_m_forward", type = STRING, default value = "white"
 */
#define ZMAPSTANZA_WINDOW_FORWARD "colour_m_forward"

/*!
 * "colour_m_reverse", type = STRING, default value = "white"
 */
#define ZMAPSTANZA_WINDOW_REVERSE "colour_m_reverse"

/*!
 * "colour_q_forward", type = STRING, default value = "light pink"
 */
#define ZMAPSTANZA_WINDOW_FORWARD "colour_q_forward"

/*!
 * "colour_q_reverse", type = STRING, default value = "pink"
 */
#define ZMAPSTANZA_WINDOW_REVERSE "colour_q_reverse"

/*!
 * "colour_m_forwardcol", type = STRING, default value = "white"
 */
#define ZMAPSTANZA_WINDOW_FORWARDCOL "colour_m_forwardcol"

/*!
 * "colour_m_reversecol", type = STRING, default value = "white"
 */
#define ZMAPSTANZA_WINDOW_REVERSECOL "colour_m_reversecol"

/*!
 * "colour_q_forwardcol", type = STRING, default value = "light pink"
 */
#define ZMAPSTANZA_WINDOW_FORWARDCOL "colour_q_forwardcol"

/*!
 * "colour_q_reversecol", type = STRING, default value = "pink"
 */
#define ZMAPSTANZA_WINDOW_REVERSECOL "colour_q_reversecol"

/*! @} end of zmap stanza group. */



/*! @defgroup zmap_stanza   ZMap stanza
 * @{
 * 
 * \brief  Configuration File Stanza for whole ZMap application.
 *         The following are compulsory: "netid", "port", "script".
 * 
 *  */

/*!
 * Stanza name.
 */
#define ZMAPSTANZA_BLIXEM_CONFIG "blixem"

/*!
 * "netid"     , type = STRING, default value = ""
 */
#define ZMAPSTANZA_BLIXEM_NETID "netid"

/*!
 * "port"      , type = INT   , default value = 0
 */
#define ZMAPSTANZA_BLIXEM_PORT "port"

/*!
 * "script"    , type = STRING, default value = ""
 */
#define ZMAPSTANZA_BLIXEM_SCRIPT "script"

/*!
 * "scope"     , type = INT   , default value = 0
 */
#define ZMAPSTANZA_BLIXEM_SCOPE "scope"

/*!
 * "homol_max" , type = INT   , default value = 0
 */
#define ZMAPSTANZA_BLIXEM_MAX "homol_max"


/*! @} end of zmap stanza group. */



/*! @} end of zmapstanza docs. */



#endif /* !ZMAP_CONFIG_STRINGS_H */
