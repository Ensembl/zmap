/*  File: zmapConfigStrings.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Repository for all the configuration file stanza
 *              keywords. We don't have to do this but its good for
 *              documentation and also good for discipline.
 *
 *              READ THIS BEFORE ADDING NEW KEYWORDS:
 *
 *              This file produces an important user documentation
 *              page: it tells the user what they can put in the
 *              configuration file to control zmap.
 *
 *              Therefore...if you add a new configuration stanza or
 *              keyword, please be sure to add _ALL_ the relevant
 *              text to the html table that procedes each set of
 *              defines !
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONFIG_STRINGS_H
#define ZMAP_CONFIG_STRINGS_H





/*! @defgroup config_stanzas   ZMap configuration Files
 *
 * @section Overview
 *
 * ZMap uses configuration files to find its servers, files and generally
 * to configure aspects of its interface. Currently these files must
 * reside in the users $HOME/.ZMap directory.
 *
 * The main configuration file is "ZMap" and this may contain references to
 * to other configuration files.
 *
 * If the $HOME/.ZMap directory does not exist when ZMap is run, then ZMap
 * will try to create the directory, the program cannot be run if the directory
 * cannot be created.
 *
 * If the $HOME/.ZMap/ZMap file does not exist when ZMap is run, then ZMap
 * will try to create the file, the program cannot be run if the file
 * cannot be created.
 *
 *  */

/*! @addtogroup config_stanzas
 *
 * @section  format Configuration File Format.
 *
 * All configuration files for ZMap are a series of stanzas of the form:
 *
 * <PRE>
 * [stanza_name]                e.g.   [ZMap]
 * resource1 = value1                  show_mainwindow = false
 * resource2 = value2                  default_printer = "colour900"
 *
 * </PRE>
 *
 * Resources may take boolean, integer, float or string values which must be given
 * in the following formats:
 *
 * String values are spefied literally, without quotes, e.g. host = griffin
 * Some string resources will accept a list of strings, in which case these
 * should be separated by semicolons e.g. styles = align;dna_align;pep_align
 *
 * Boolean values must be specified as either true or false, e.g. logging = false
 *
 * Float values must be given in floating point format, e.g. width = 10.0
 *
 * Integer values must be given in integer format, e.g. port = 999
 *
 * White space is not important.
 *
 * Any text following (and including) a "#" is interpreted as a comment and
 * ignored.
 *
 * Some keywords are mandatory within a stanza and hence have no default value. If
 * are not specified the stanza is ignored.
 *
 *  */



/*! @addtogroup config_stanzas
 *
 * @section app   ZMap Application Options
 *
 * These are options that control the fundamental way ZMap behaves.
 *
 * <table>
 *  <tr>
 *  <th>Stanza</th>
 *  <th colspan=3 align=center>"ZMap"</th>
 *  </tr>
 *  <tr>
 *  <th>Keyword</th>
 *  <th>Datatype</th>
 *  <th>Default</th>
 *  <th>Description</th>
 *  </tr>
 *  <tr>
 *  <th>"show_mainwindow"</th>
 *  <td>Boolean</td>
 *  <td>true</td>
 *  <td>If false then the intial main zmap window is not shown, this is useful when zmap is being
 *      controlled by another application and its not necessary for the user to directly open
 *      feature windows themselves.</td>
 *  </tr>
 *  <tr>
 *  <th>"exit_timeout"</th>
 *  <td>Int</td>
 *  <td>5</td>
 *  <td>Time in seconds to wait for zmap to finish clearing up server connections before exiting.
 *      After this zmap will exit and some connections may not have been clearly terminated.</td>
 *  </tr>
 *  <tr>
 *  <th>"default_sequence"</th>
 *  <td>String</td>
 *  <td>""</td>
 *  <td>if a particular sequence has its own unique configuration file, then the sequence name
 *      can be specified in the file.
 *  </tr>
 *  <tr>
 *  <th>"default_printer"</th>
 *  <td>String</td>
 *  <td>""</td>
 *  <td>Specify a printer which will be the default printer for screen shots from ZMap.</td>
 *  </tr>
 *  <tr>
 *  <th>"script-dir"</th>
 *  <td>String</td>
 *  <td>"ZMap run-time directory"</td>
 *  <td>Specify the directory where data retrieval scripts are stored by default.</td>
 *  </tr>
 *  <tr>
 *  <th>"data-dir"</th>
 *  <td>String</td>
 *  <td>"ZMap run-time directory"</td>
 *  <td>Specify the directory where GFF data files are stored by default.</td>
 *  </tr>
 *  <tr>
 *  <th>"stylesfile"</th>
 *  <td>string</td>
 *  <td>""</td>
 *  <td>Styles specify how sets of features are displayed and processed. By default this
 *  information is fetched from the server for each feature set. As an alternative the
 *  styles can be specified in a file in $HOME/.ZMap. (see "style" stanza description)
 *  </tr>
 * </table>
 *
 *  */
#define ZMAPSTANZA_APP_CONFIG            "ZMap"
#define ZMAPSTANZA_APP_MAINWINDOW        "show-mainwindow"
#define ZMAPSTANZA_APP_EXIT_TIMEOUT      "exit-timeout"
#define ZMAPSTANZA_APP_HELP_URL          "help-url"

#define ZMAPSTANZA_APP_DATASET           "dataset"
#define ZMAPSTANZA_APP_SEQUENCE          "sequence"
#define ZMAPSTANZA_APP_START             "start"
#define ZMAPSTANZA_APP_END               "end"
#define ZMAPSTANZA_APP_CSNAME            "csname"
#define ZMAPSTANZA_APP_CSVER             "csver"

#define ZMAPSTANZA_APP_PRINTER           "default-printer"
#define ZMAPSTANZA_APP_SEQUENCE_SERVERS  "sequence-server"
#define ZMAPSTANZA_APP_PFETCH_LOCATION   "pfetch"
#define ZMAPSTANZA_APP_COOKIE_JAR        "cookie-jar"
#define ZMAPSTANZA_APP_PFETCH_MODE       "pfetch-mode"
#define ZMAPSTANZA_APP_LOCALE            "locale"
//#define ZMAPSTANZA_APP_SCRIPTS           "script-dir"
#define ZMAPSTANZA_APP_DATA              "data-dir"
//#define ZMAPSTANZA_APP_STYLESFILE        "stylesfile"
#define ZMAPSTANZA_APP_LEGACY_STYLES     "legacy-styles"
#define ZMAPSTANZA_APP_STYLE_FROM_METHOD "stylename-from-methodname"
#define ZMAPSTANZA_APP_XREMOTE_DEBUG     "xremote-debug"
#define ZMAPSTANZA_APP_COLUMNS           "columns"
#define ZMAPSTANZA_APP_REPORT_THREAD     "thread-fail-silent"
#define ZMAPSTANZA_APP_NAVIGATOR_SETS    "navigatorsets"

#define ZMAPSTANZA_APP_SEQ_DATA          "seq-data"


/*! @addtogroup config_stanzas
 *
 * @section logging     Logging Options
 *
 * ZMap writes messages to the logfile $HOME/.ZMap/zmap.log by default, but the log filepath
 * and other parameters of logging can be specified in the "logging" stanza.
 *
 * <table>
 *  <tr>
 *  <th>Stanza</th>
 *  <th colspan=3 align=center>"logging"</th>
 *  </tr>
 *  <tr>
 *  <th>Keyword</th>
 *  <th>Datatype</th>
 *  <th>Default</th>
 *  <th>Description</th>
 *  </tr>
 *  <tr>
 *  <th>"logging"</th>
 *  <td>Boolean</td>
 *  <td>true</td>
 *  <td>Turn logging on or off.</td>
 *  </tr>
 *  <tr>
 *  <th>"file"</th>
 *  <td>Boolean</td>
 *  <td>true</td>
 *  <td>true: messages to logfile, false: messages to stdout or stderr.</td>
 *  </tr>
 *  <tr>
 *  <th>"directory"</th>
 *  <td>String</td>
 *  <td>"$HOME/.ZMap"</td>
 *  <td>Absolute or relative directory to hold logfiles (relative is relative to $HOME/.ZMap).</td>
 *  </tr>
 *  <tr>
 *  <th>"filename"</th>
 *  <td>String</td>
 *  <td>"zmap.log"</td>
 *  <td>Name of logfile, this is combined with "directory" to give the logfile path.</td>
 *  </tr>
 * </table>
 *
 *  */
#define ZMAPSTANZA_LOG_CONFIG    "logging"
#define ZMAPSTANZA_LOG_LOGGING   "logging"
#define ZMAPSTANZA_LOG_FILE      "file"
#define ZMAPSTANZA_LOG_DIRECTORY "directory"
#define ZMAPSTANZA_LOG_FILENAME  "filename"
#define ZMAPSTANZA_LOG_SHOW_CODE "show-code"
#define ZMAPSTANZA_LOG_SHOW_TIME "show-time"    // mgh: didn't complete this so didn't put it in the help
#define ZMAPSTANZA_LOG_CATCH_GLIB "catch-glib"
#define ZMAPSTANZA_LOG_ECHO_GLIB  "echo-glib"


/*! @addtogroup config_stanzas
 *
 * @section source    Data Source Options
 *
 * ZMap can obtain sequence data from files and servers of various kinds, source stanzas
 * specify information about the source.  Each source stanza should must have a unique name,
 * and must be referenced in the 'sources' resource in the [ZMap] stanza.
 *
 * Data sources are identified using urls in the following supported variants:
 *
 * <PRE>
 *
 *    url = "<url_identifier>"
 *
 *    where <url_identifier> should match ([] = optional)
 *
 *    <protocol>://[[<username>:<password>@]<hostname>[:<port>]]/<location>#<format>
 *
 *    <protocol> may be one of acedb, file, pipe or http
 *    <username> should be a username string
 *    <password> should be a password string
 *    <hostname> should be a hostname string
 *    <port>     should be a port number
 *    <location> should identify the location on a particular server
 *    <format>   may be one of gff, das, das2 (default gff)
 *
 *    examples
 *
 *    file:///var/tmp/my_gff_file.gff#gff
 *    pipe:////software/anacode/bin/get_genes?dataset=human&name=1&analysis=ccds_gene&end=161655109...
 *    http://das1.sanger.ac.uk:8080/das/h_sapiens#das
 *    acedb://any:any@deskpro110:23100
 *
 *    N.B.  <location> might include a query string too. e.g.
 *
 *                 http://www.sanger.ac.uk/das/h_sapiens?chromosome=1#das
 *    Note that for file: and pipe: sources file:///file is a relative file name and file://// is absolute.
 *
 * </PRE>
 *
 * <table>
 *  <tr>
 *  <th>Stanza</th>
 *  <th colspan=3 align=center>"source"</th>
 *  </tr>
 *  <tr>
 *  <th>Keyword</th>
 *  <th>Datatype</th>
 *  <th>Default</th>
 *  <th>Description</th>
 *  </tr>
 *  <tr>
 *  <th>"url"</th>
 *  <td>String</td>
 *  <td>Mandatory</td>
 *  <td>The url specifying where to find the source.
 *  </tr>
 *  <tr>
 *  <th>"featuresets"</th>
 *  <td>String</td>
 *  <td>""</td>
 *  <td>A list of feature sets to be retrieved from the source(s). The list also gives the order
 *      in which the feature sets will be displayed in the zmap window. If not specified, all
 *      available feature sets will be retrieved and they will be displayed in whichever order
 *      the list of available sets was returned by the server.
 *  </tr>
 *  <tr>
 *  <th>"navigatorsets"</th>
 *  <td>String</td>
 *  <td>""</td>
 *  <td>A list of feature sets to use in a navigator window.
 *  </tr>
 *  <tr>
 *  <th>"timeout"</th>
 *  <td>Int</td>
 *  <td>120</td>
 *  <td>Timeout in seconds to wait for a server to reply before aborting a request.</td>
 *  </tr>
 *  <tr>
 *  <th>"version"</th>
 *  <td>String</td>
 *  <td>""</td>
 *  <td>The minimum version of the server supported, must be a standard version string in the
 *      form "version.release.update", e.g. "4.9.28". If the server is an earlier version than
 *      this the connection will not be made.
 *  </tr>
 *  <tr>
 *  <th>"use_acedb_methods"</th>
 *  <td>Boolean</td>
 *  <td>false</td>
 *  <td><b>Only read for acedb databases.</b> If true, then zmap reads style information from
 *      Method class objects instead of the newer and more flexible ZMap_style classes.
 *  </tr>
 *  <tr>
 *  <th>"sequence"</th>s
 *  <td>Boolean</td>
 *  <td>false</td>
 *  <td>If true, this is the server that the sequence for the feature region should be fetched from,
 *      only one such server can be specified.
 *  </tr>
 *  <tr>
 *  <th>"format"</th>
 *  <td>string</td>
 *  <td>""</td>
 *  <td>Defunct, use the "url" keyword/value to specify the format.
 *  </tr>
 *  <tr>
 *  <th>"styles"</th>
 *  <td>string</td>
 *  <td>""</td>
 *  <td>List of all styles to be retrieved from styles file.  If not specified then all styles will be read
 *  if a file has been specified in the [ZMap] stanza, otherwsie the source should provide the styles itself.
 *  By default a featureset will use a style of the same name.
 *
 *  </tr>
 *  <tr>
 *  <th>"delayed"</th>
 *  <td>Boolean</td>
 *  <td>true/false</td>
 *  <td>If true then the server will not request features automatically on ZMap startup. The default for pipeServers is true and for others false.
 *  </tr>

 * </table>
 *
 *  */
#define ZMAPSTANZA_SOURCE_CONFIG         "source"
#define ZMAPSTANZA_SOURCE_URL            "url"
#define ZMAPSTANZA_SOURCE_TIMEOUT        "timeout"
#define ZMAPSTANZA_SOURCE_VERSION        "version"
#define ZMAPSTANZA_SOURCE_FEATURESETS    "featuresets"
#define ZMAPSTANZA_SOURCE_STYLES         "styles"
#define ZMAPSTANZA_SOURCE_STYLESFILE     "stylesfile"
#define ZMAPSTANZA_SOURCE_MAPPING        "provide-mapping"
#define ZMAPSTANZA_SOURCE_NAVIGATORSETS  "navigatorsets"
//#define ZMAPSTANZA_SOURCE_SEQUENCE       "sequence"
#define ZMAPSTANZA_SOURCE_FORMAT         "format"
#define ZMAPSTANZA_SOURCE_DELAYED        "delayed"
#define ZMAPSTANZA_SOURCE_GROUP          "group"

#define ZMAPSTANZA_SOURCE_GROUP_NEVER      "never"
#define ZMAPSTANZA_SOURCE_GROUP_START      "start"
#define ZMAPSTANZA_SOURCE_GROUP_DELAYED    "delayed"
#define ZMAPSTANZA_SOURCE_GROUP_ALWAYS     "always"



/*! @addtogroup config_stanzas
 *
 * @section types   Style Options
 *
 * ZMap processes/displays/formats features based on a description of the characteristics of the
 * features given in a "style".
 *
 * <table>
 *  <tr>
 *  <th>Stanza</th>
 *  <th colspan=3 align=center>"Type"</th>
 *  </tr>
 *  <tr>
 *  <th>Keyword</th>
 *  <th>Datatype</th>
 *  <th>Default</th>
 *  <th>Description</th>
 *  </tr>
 *  <tr>
 *  <th>"name"</th>
 *  <td>String</td>
 *  <td>Mandatory value</td>
 *  <td>The name of the style, this must be specified. Any feature that references this style
 *      will be processed using the information given in this style.</td>
 *  </tr>
 *  <tr>
 *  <th>"outline"</th>
 *  <td>String</td>
 *  <td>"black"</td>
 *  <td>An outline colour in the standard X colours format, e.g. "DarkSlateGray"
 *      or a hex specification such as "305050". The colour is used for the border
 *      of a feature.
 *  </tr>
 *  <tr>
 *  <th>"background"</th>
 *  <td>String</td>
 *  <td>"black"</td>
 *  <td>The background colour in the standard X colours format, e.g. "DarkSlateGray"
 *      or a hex specification such as "305050". The colour is used to give the fill
 *      colour of a feature.
 *  </tr>
 *  <tr>
 *  <th>"foreground"</th>
 *  <td>String</td>
 *  <td>"white"</td>
 *  <td>The foreground colour in the standard X colours format, e.g. "DarkSlateGray"
 *      or a hex specification such as "305050". The colour is used to give the fill
 *      colour of a feature.
 *  </tr>
 *  <tr>
 *  <th>"width"</th>
 *  <td>Float</td>
 *  <td>10.0</td>
 *  <td>The total width of the column in which the features of this type will be displayed
 *      in the ZMap feature window.
 *  </tr>
 *  <tr>
 *  <th>"strand_specific"</th>
 *  <td>Boolean</td>
 *  <td>false</td>
 *  <td>If false then strand is irrelevant for this set of features and "showUpStrand" and
 *      "frame_specific" values will be ignored.
 *  </tr>
 *  <tr>
 *  <th>"show_reverse"</th>
 *  <td>Boolean</td>
 *  <td>false</td>
 *  <td>If true then features of this type are displayed on both the forward (down) strand
 *      and reverse (up) strands, the default is for only the forward strand features to be displayed.
 *  </tr>
 *  <tr>
 *  <th>"frame_specific"</th>
 *  <td>Boolean</td>
 *  <td>false</td>
 *  <td>If true then frame is essential to the correct processing of features in this set,
 *      e.g. for transcripts that must be translated.
 *  </tr>
 *  <tr>
 *  <th>"minmag"</th>
 *  <td>Int</td>
 *  <td>0</td>
 *  <td>The minimum magnification at which these features should be shown, when zoomed out more
 *      than this the features are not displayed which can considerably speed up display of long
 *      sequence regions.
 *  </tr>
 *  <tr>
 *  <th>"maxmag"</th>
 *  <td>Int</td>
 *  <td>0</td>
 *  <td>The maximum magnification at which these features should be shown, when zoomed in more
 *      than this the features are not displayed which can considerably speed up and simplify
 *      display at high magnifications.
 *  </tr>
 *  <tr>
 *  <th>"bump"</th>
 *  <td>string</td>
 *  <td>"overlap"</td>
 *  <td>Specifies how features in a set should be displayed when their positions overlap each
 *      other. Valid bump types are:
 *      <ul>
 *      <li>"overlap" - features just overlap and so may partially or wholly obscure each other.
 *      <li>"position" - features are displayed alongside each other where their positions overlap.
 *      <li>"name" - features are displayed in vertical columns according to their name (good for
 *          homologies.
 *      <li>"simple" - every feature gets its own column, leads to a very wide window but is good
 *           for debugging !
 *      </ul>
 *  </tr>
 *  <tr>
 *  <th>"gapped_align"</th>
 *  <td>Boolean</td>
 *  <td>false</td>
 *  <td>If true then homology matches that contain gaps will be shown as separate blocks
 *      connected by lines, otherwise the match is shown as a single block.
 *  </tr>
 * </table>
 *
 *  */

#define ZMAPSTANZA_STYLE_CONFIG      "Style"
/*
 * See ZMap/zmapStyle.h for declarations of property strings.
 */



/*! @addtogroup config_stanzas
 *
 * @section  window      ZMap Feature Window Options
 *
 * The ZMap feature window is where sequence features are displayed, various aspects of it
 * can be configured from colours to performance factors.
 *
 * <table>
 *  <tr>opaquely in the view
 *  <th>Stanza</th>
 *  <th colspan=3 align=center>"ZMapWindow"</th>
 *  </tr>
 *  <tr>
 *  <th>Keyword</th>
 *  <th>Datatype</th>
 *  <th>Default</th>
 *  <th>Description</th>
 *  </tr>
 *  <tr>
 *  <th>"canvas_maxsize"</th>
 *  <td>Integer</td>
 *  <td>30000</td>
 *  <td>Set the maximum vertical extent of the feature window. Setting a smaller size will result in
 *      faster display as less data gets mapped as you zoom in but will mean you have less
 *      data to scroll over. The default is to create close to the maximum window size possible,
 *      this is 32k for X Windows.</td>
 *  </tr>
 *  <tr>
 *  <th>"canvas_maxbases"</th>
 *  <td>Integer</td>
 *  <td>0</td>
 *  <td>Set the vertical extent of the feature window using dna bases. Is just an alternative
 *      method of setting "canvas_maxsize".
 *  </tr>
 *  <tr>
 *  <th>"keep_empty_columns"</th>
 *  <td>Boolean</td>
 *  <td>false</td>
 *  <td>Some requested feature sets may not contain any features, by default these will not be
 *      displayed. For multiple blocks/alignment display it may be clearer to get zmap to display
 *      empty "placeholder" columns for these sets so that columns in different blocks line up
 *      better.
 *  </tr>
 *  <tr>
 *  <th>"colour_root"</th>
 *  <td>string</td>
 *  <td>white"</td>
 *  <td>Colour for window background, specified as in the "Type" stanza.
 *  </tr>
 *  <tr>
 *  <th>"colour_alignment"</th>
 *  <td>string</td>
 *  <td>"white"</td>
 *  <td>Colour for each alignment background, specified as in the "Type" stanza.
 *  </tr>
 *  <tr>
 *  <th>"colour_block"</th>
 *  <td>string</td>
 *  <td>"white"</td>
 *  <td>Colour for each block background, specified as in the "Type" stanza.
 *  </tr>
 *  <tr>
 *  <th>"colour_m_forward"</th>
 *  <td>string</td>
 *  <td>"white"</td>
 *  <td>Colour for forward strand column group background in master alignment,
 *      specified as in the "Type" stanza.
 *  </tr>
 *  <tr>
 *  <th>"colour_m_reverse"</th>
 *  <td>string</td>
 *  <td>"white"</td>
 *  <td>Colour for reverse strand column group background in master alignment,
 *      specified as in the "Type" stanza.
 *  </tr>
 *  <tr>
 *  <th>"colour_q_forward"</th>
 *  <td>string</td>
 *  <td>"light pink"</td>
 *  <td>Colour for forward strand column group background in matched alignment,
 *      specified as in the "Type" stanza.
 *  </tr>
 *  <tr>
 *  <th>"colour_q_reverse"</th>
 *  <td>string</td>
 *  <td>"pink"</td>
 *  <td>Colour for reverse strand column group background in matched alignment,
 *      specified as in the "Type" stanza.
 *  </tr>
 *  <tr>
 *  <th>"colour_m_forwardcol"</th>
 *  <td>string</td>
 *  <td>"white"</td>
 *  <td>Colour for forward strand column background in master alignment,
 *      specified as in the "Type" stanza.
 *  </tr>
 *  <tr>
 *  <th>"colour_m_reversecol"</th>
 *  <td>string</td>
 *  <td>"white"</td>
 *  <td>Colour for reverse strand column background in master alignment,
 *      specified as in the "Type" stanza.
 *  </tr>
 *  <tr>
 *  <th>"colour_q_forwardcol"</th>
 *  <td>string</td>
 *  <td>"light pink"</td>
 *  <td>Colour for forward strand column background in matched alignment,
 *      specified as in the "Type" stanza.
 *  </tr>
 *  <tr>
 *  <th>"colour_q_reversecol"</th>
 *  <td>string</td>
 *  <td>"pink"</td>
 *  <td>Colour for reverse strand column background in matched alignment,
 *      specified as in the "Type" stanza.
 *  </tr>
 * </table>
 *
 *  */
#define ZMAPSTANZA_WINDOW_CONFIG       "ZMapWindow"
#define ZMAPSTANZA_WINDOW_CURSOR       "cursor"
#define ZMAPSTANZA_WINDOW_MAXSIZE      "canvas-maxsize"
#define ZMAPSTANZA_WINDOW_MAXBASES     "canvas-maxbases"
#define ZMAPSTANZA_WINDOW_FWD_COORDS   "display-forward-coords"
#define ZMAPSTANZA_WINDOW_SHOW_3F_REV  "show-3-frame-reverse"
#define ZMAPSTANZA_WINDOW_A_SPACING    "align-spacing"
#define ZMAPSTANZA_WINDOW_B_SPACING    "block-spacing"
#define ZMAPSTANZA_WINDOW_S_SPACING    "strand-spacing"
#define ZMAPSTANZA_WINDOW_C_SPACING    "column-spacing"
#define ZMAPSTANZA_WINDOW_F_SPACING    "feature-spacing"
#define ZMAPSTANZA_WINDOW_LINE_WIDTH   "feature-line-width"
#define ZMAPSTANZA_WINDOW_COLUMNS      "keep-empty-columns"
#define ZMAPSTANZA_WINDOW_ROOT         "colour-root"
#define ZMAPSTANZA_WINDOW_ALIGNMENT    "colour-alignment"
#define ZMAPSTANZA_WINDOW_BLOCK        "colour-block"
#define ZMAPSTANZA_WINDOW_M_FORWARD    "colour-m-forward"
#define ZMAPSTANZA_WINDOW_M_REVERSE    "colour-m-reverse"
#define ZMAPSTANZA_WINDOW_Q_FORWARD    "colour-q-forward"
#define ZMAPSTANZA_WINDOW_Q_REVERSE    "colour-q-reverse"
#define ZMAPSTANZA_WINDOW_M_FORWARDCOL "colour-m-forwardcol"
#define ZMAPSTANZA_WINDOW_M_REVERSECOL "colour-m-reversecol"
#define ZMAPSTANZA_WINDOW_Q_FORWARDCOL "colour-q-forwardcol"
#define ZMAPSTANZA_WINDOW_Q_REVERSECOL "colour-q-reversecol"
#define ZMAPSTANZA_WINDOW_SEPARATOR    "colour-separator"
#define ZMAPSTANZA_WINDOW_COL_HIGH     "colour-column-highlight"
#define ZMAPSTANZA_WINDOW_ITEM_MARK    "colour-item-mark"
#define ZMAPSTANZA_WINDOW_ITEM_HIGH    "colour-item-highlight"
#define ZMAPSTANZA_WINDOW_FRAME_0      "colour-frame-0"
#define ZMAPSTANZA_WINDOW_FRAME_1      "colour-frame-1"
#define ZMAPSTANZA_WINDOW_FRAME_2      "colour-frame-2"
#define ZMAPSTANZA_WINDOW_ITEM_EVIDENCE_BORDER "colour-evidence-border"
#define ZMAPSTANZA_WINDOW_ITEM_EVIDENCE_FILL   "colour-evidence-fill"
#define ZMAPSTANZA_WINDOW_MASKED_FEATURE_FILL   "colour-masked-feature-fill"
#define ZMAPSTANZA_WINDOW_MASKED_FEATURE_BORDER   "colour-masked-feature-border"
#define ZMAPSTANZA_WINDOW_FILTERED_COLUMN   "colour-filtered-column"


/*! @addtogroup config_stanzas
 *
 * @section debug     Debugging Options
 *
 * You should only use these if you are a developer as they can impact the performance of
 * ZMap significantly or cause it to write large amounts of data to stdout.
 *
 * <table>
 *  <tr>
 *  <th>Stanza</th>
 *  <th colspan=3 align=center>"debug"</th>
 *  </tr>
 *  <tr>
 *  <th>Keyword</th>
 *  <th>Datatype</th>
 *  <th>Default</th>
 *  <th>Description</th>
 *  </tr>
 *  <tr>
 *  <th>"threads"</th>
 *  <td>Boolean</td>
 *  <td>false</td>
 *  <td>Turns on/off debug output for the threading sections of ZMap.</td>
 *  </tr>
 *  <tr>
 *  <th>"feature2style"</th>
 *  <td>Boolean</td>
 *  <td>false</td>
 *  <td>Turns on/off debug output for mapping featuresets to styles.</td>
 *  </tr>
 *  <tr>
 *  <th>"styles"</th>
 *  <td>Boolean</td>
 *  <td>false</td>
 *  <td>Turns on/off debug output for style definitions.</td>
 *  </tr>
 * </table>
 *
 *  */
#define ZMAPSTANZA_DEBUG_CONFIG      "debug"
#define ZMAPSTANZA_DEBUG_APP_THREADS "threads"
#define ZMAPSTANZA_DEBUG_APP_FEATURE2STYLE      "feature2style"
#define ZMAPSTANZA_DEBUG_APP_STYLES  "styles"
#define ZMAPSTANZA_DEBUG_APP_TIMING            "timing"     // output timing of various things to STDOUT
#define ZMAPSTANZA_DEBUG_APP_DEVELOP  "development"


/*! @addtogroup config_stanzas
 *
 * @section align     Multiple Alignment Options
 *
 * ZMap can display multiple alignments, each alignment can contain multiple blocks. This
 * stanza specifies which blocks in which alignment match each other. Note that all parameters
 * are mandatory otherwise zmap will not know how to display the blocks.
 *
 * <table>
 *  <tr>
 *  <th>Stanza</th>
 *  <th colspan=3 align=center>"align"</th>
 *  </tr>
 *  <tr>
 *  <th>Keyword</th>
 *  <th>Datatype</th>
 *  <th>Default</th>
 *  <th>Description</th>
 *  </tr>
 *  <tr>
 *  <th>"reference_seq"</th>
 *  <td>String</td>
 *  <td>Mandatory</td>
 *  <td>Name of the "master" or reference sequence, this sequence is shown on the right of the
 *      window and blocks in other alignments are aligned to it.</td>
 *  </tr>
 *  <tr>
 *  <th>"reference_start"</th>
 *  <td>int</td>
 *  <td>Mandatory</td>
 *  <td>Start position of a block in the reference sequence.
 *  </tr>
 *  <tr>
 *  <th>"reference_end"</th>
 *  <td>int</td>
 *  <td>Mandatory</td>
 *  <td>Start position of a block in the reference sequence.
 *  </tr>
 *  <tr>
 *  <th>"reference_strand"</th>
 *  <td>int</td>
 *  <td>Mandatory</td>
 *  <td>Which strand of the reference sequence is being aligned, 1 means forward strand,
 *      -1 means reverse.</td>
 *  </tr>
 *  <tr>
 *  <th>"non_reference_seq"</th>
 *  <td>String</td>
 *  <td>Mandatory</td>
 *  <td>Name of the "master" or non reference sequence, this sequence is shown on the right of the
 *      window and blocks in other alignments are aligned to it.</td>
 *  </tr>
 *  <tr>
 *  <th>"non_reference_start"</th>
 *  <td>int</td>
 *  <td>Mandatory</td>
 *  <td>Start position of a block in the non reference sequence.
 *  </tr>
 *  <tr>
 *  <th>"non_reference_end"</th>
 *  <td>int</td>
 *  <td>Mandatory</td>
 *  <td>Start position of a block in the non reference sequence.
 *  </tr>
 *  <tr>
 *  <th>"non_reference_strand"</th>
 *  <td>int</td>
 *  <td>Mandatory</td>
 *  <td>Which strand of the non reference sequence is being aligned, 1 means forward strand,
 *      -1 means reverse.</td>
 *  </tr>
 * </table>
 *
 *  */
#define ZMAPSTANZA_ALIGN_CONFIG        "align"
#define ZMAPSTANZA_ALIGN_SEQ           "reference-seq"
#define ZMAPSTANZA_ALIGN_START         "reference-start"
#define ZMAPSTANZA_ALIGN_END           "reference-end"
#define ZMAPSTANZA_ALIGN_STRAND        "reference-strand"
#define ZMAPSTANZA_ALIGN_NONREF_SEQ    "non-reference-seq"
#define ZMAPSTANZA_ALIGN_NONREF_START  "non-reference-start"
#define ZMAPSTANZA_ALIGN_NONREF_END    "non-reference-end"
#define ZMAPSTANZA_ALIGN_NONREF_STRAND "non-reference-strand"



/*! @addtogroup config_stanzas
 *
 * @section blixem     Blixem Options
 *
 * ZMap can show multiple alignments using the blixem program, this stanza tells
 * zmap how to find and run blixem.
 *
 * <table>
 *  <tr>
 *  <th>Stanza</th>
 *  <th colspan=3 align=center> ZMAPSTANZA_ALIGN_CONFIG </th>
 *  </tr>
 *  <tr>
 *  <th>Keyword</th>
 *  <th>Datatype</th>
 *  <th>Default</th>
 *  <th>Description</th>
 *  </tr>
 *  <tr>
 *  <th>"netid"</th>
 *  <td>String</td>
 *  <td>Mandatory</td>
 *  <td>The network id of the machine running pfetch from which blixem will retrieve the sequences
 *      to be shown in the multiple alignement.</td>
 *  </tr>
 *  <tr>
 *  <th>"port"</th>
 *  <td>int</td>
 *  <td>Mandatory</td>
 *  <td>The port number on the "netid" machine for the pfetch server.
 *  </tr>
 *  <tr>
 *  <th>"script"</th>
 *  <td>String</td>
 *  <td>Mandatory</td>
 *  <td>The name of the blixem program, this could be a shell script which sets up some
 *      environment and then starts blixem or indeed another multiple alignment display program.
 *  </tr>
 *  <tr>
 *  <th>"scope"</th>
 *  <td>Int</td>
 *  <td>40000</td>
 *  <td>The span around the selected feature about which blixem will get and display data
 *      for the multiple alignment.
 *  </tr>
 *  <tr>
 *  <th>"homol_max"</th>
 *  <td>Int</td>
 *  <td>????</td>
 *  <td>The maximum number of homologies displayed ?? CHECK THIS....
 *  </tr>
 * </table>
 *
 *  */
#define ZMAPSTANZA_BLIXEM_CONFIG           "blixem"
#define ZMAPSTANZA_BLIXEM_NETID            "netid"
#define ZMAPSTANZA_BLIXEM_PORT             "port"
#define ZMAPSTANZA_BLIXEM_SCRIPT           "script"
#define ZMAPSTANZA_BLIXEM_CONF_FILE        "config-file"
#define ZMAPSTANZA_BLIXEM_SCOPE            "scope"
#define ZMAPSTANZA_BLIXEM_SCOPE_MARK       "scope-from-mark"
#define ZMAPSTANZA_BLIXEM_FEATURES_MARK    "features-from-mark"
#define ZMAPSTANZA_BLIXEM_MAX              "homol-max"
#define ZMAPSTANZA_BLIXEM_FILE_FORMAT      "file-format"
#define ZMAPSTANZA_BLIXEM_KEEP_TEMP        "keep-tempfiles"
#define ZMAPSTANZA_BLIXEM_KILL_EXIT        "kill-on-exit"
#define ZMAPSTANZA_BLIXEM_DNA_FS           "dna-featuresets"
#define ZMAPSTANZA_BLIXEM_PROT_FS          "protein-featuresets"
#define ZMAPSTANZA_BLIXEM_FS               "featuresets"

#define ZMAPSTANZA_BLIXEM_FILE_OPT_GFF   "gffv3"
#define ZMAPSTANZA_BLIXEM_FILE_OPT_EXBLX "exblx"



#define ZMAPSTANZA_GLYPH_CONFIG           "glyphs"

#define ZMAPSTANZA_HEATMAP_CONFIG           "heatmaps"
#define MAX_HEAT_COLOUR				16	/* more than conceivable, only 8 colours in a rainbow */


#define ZMAPSTANZA_COLUMN_CONFIG              "columns"
#define ZMAPSTANZA_COLUMN_STYLE_CONFIG        "column-style"
#define ZMAPSTANZA_COLUMN_DESCRIPTION_CONFIG  "column-description"
#define ZMAPSTANZA_FEATURESET_STYLE_CONFIG    "featureset-style"
#define ZMAPSTANZA_GFF_SOURCE_CONFIG          "featureset-source"
#define ZMAPSTANZA_GFF_DESCRIPTION_CONFIG     "featureset-description"
#define ZMAPSTANZA_GFF_RELATED_CONFIG         "featureset-related"
#define ZMAPSTANZA_FEATURESETS_CONFIG         "featuresets"





#endif /* !ZMAP_CONFIG_STRINGS_H */
