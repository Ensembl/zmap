/*  File: ZMapConfigFiles.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Dummy C file, contains config file documentation
 *              used by doxygen tool to document ZMap.
 *              
 * HISTORY:
 * Last edited: Jan  6 11:54 2005 (edgrif)
 * Created: Thu Jan  6 08:15:43 2005 (edgrif)
 * CVS info:   $Id: ZMapConfigFiles.c,v 1.1 2005-01-06 11:55:24 edgrif Exp $
 *-------------------------------------------------------------------
 */

/*! @defgroup config_files   ZMap configuration Files
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

/*! @addtogroup config_files
 *
 * @section  format Configuration File Format.
 * 
 * All configuration files for ZMap are a series of stanzas of the form:
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
 * Resources may take boolean, integer, float or string values.
 *
 * String values must be enclosed in double quotes, e.g. host = "griffin"
 *
 * Boolean values must be specified as either true or false, e.g. logging = false
 *
 * White space is not important.
 *
 * Any text following (and including) a "#" is interpreted as a comment and
 * ignored.
 *
 *  */


/*! @addtogroup config_files
 *
 * @section  logging Logging Options
 * 
 * ZMap writes messages to the logfile $HOME/.ZMap/zmap.log by default, but the log filepath
 * and other parameters of logging can be specified in the "logging" stanza.
 *
 * <table>
 *  <tr>
 *  <th>Stanza name</th>
 *  <th colspan=2 align=center>"logging"</th>
 *  </tr>
 *  <tr>
 *  <th>Resource Name</th>
 *  <th>Resource Type</th>
 *  <th>Resource Description</th>
 *  </tr>
 *  <tr>
 *  <th>"logging"</th>
 *  <td>Boolean</th>
 *  <td>Turn logging on or off.</th>
 *  </tr>
 *  <tr>
 *  <th>"file"</th>
 *  <td>Boolean</th>
 *  <td>true: messages to logfile, false: messages to stdout or stderr.</th>
 *  </tr>
 *  <tr>
 *  <th>"directory"</th>
 *  <td>String</th>
 *  <td>Absolute or relative directory to hold logfiles (relative is relative to $HOME/.ZMap).</th>
 *  </tr>
 *  <tr>
 *  <th>"filename"</th>
 *  <td>String</th>
 *  <td>Name of logfile, this is combined with "directory" to give the logfile path.</th>
 *  </tr>
 * </table>
 *
 *  */


/*! @addtogroup config_files
 *
 * @section  server Server Options
 * 
 * ZMap can obtain sequence data either from servers across a network or locally from
 * files.
 *
 * <table>
 *  <tr>
 *  <th>Stanza name</th>
 *  <th colspan=2 align=center>"server"</th>
 *  </tr>
 *  <tr>
 *  <th>Resource Name</th>
 *  <th>Resource Type</th>
 *  <th>Resource Description</th>
 *  </tr>
 *  <tr>
 *  <th>"host"</th>
 *  <td>String</th>
 *  <td>The name of the network host machine of the server.</th>
 *  </tr>
 *  <tr>
 *  <th>"port"</th>
 *  <td>Int</th>
 *  <td>The port on the host machine used by the server.</th>
 *  </tr>
 *  <tr>
 *  <th>"protocol"</th>
 *  <td>String</th>
 *  <td>The protocol of the server, currently must be one of: "acedb", "das" or "file".</th>
 *  </tr>
 *  <tr>
 *  <th>"timeout"</th>
 *  <td>Int</th>
 *  <td>Timeout in seconds to wait for a server to reply before aborting a request.</th>
 *  </tr>
 *  <tr>
 *  <th>"version"</th>
 *  <td>String</th>
 *  <td>The minimum version of the server supported, must be a standard version string in the
 *      form "version.release.update", e.g. "4.9.28". If the server is an earlier version than
 *      this the connection will not be made.
 *  </tr>
 *  <tr>
 *  <th>"sequence"</th>
 *  <td>Boolean</th>
 *  <td>If true, this is the server that the sequence for the feature region should be fetched from,
 *      only one such server can be specified.
 *  </tr>
 *  <tr>
 *  <th>"writeback"</th>
 *  <td>Boolean</th>
 *  <td>If true, this is the server that user changes/annotations to features should be written
 *      back to. Only one such server can be specified.
 *  </tr>
 *  <tr>
 *  <th>"typesfile"</th>
 *  <td>string</th>
 *  <td>The name of the file in $HOME/.ZMap that specifies the "type" information (type is used in
 *      in the DAS sense) for the server. NEED A XREF TO THE TYPES STUFF HERE...
 *  </tr>
 * </table>
 *
 *  */


/*! @addtogroup config_files
 *
 * @section  types Type Options
 * 
 * ZMap processes/displays/formats features based on a description of the characteristics of the
 * features given in a DAS style "type".
 *
 * <table>
 *  <tr>
 *  <th>Stanza name</th>
 *  <th colspan=2 align=center>"Type"</th>
 *  </tr>
 *  <tr>
 *  <th>Resource Name</th>
 *  <th>Resource Type</th>
 *  <th>Resource Description</th>
 *  </tr>
 *  <tr>
 *  <th>"name"</th>
 *  <td>String</th>
 *  <td>The name of the type.</th>
 *  </tr>
 *  <tr>
 *  <th>"outline"</th>
 *  <td>String</th>
 *  <td>An outline colour in the standard X colours format, e.g. "DarkSlateGray"
 *      or a hex specification such as "305050". The colour is used for the border
 *      of a feature.
 *  </tr>
 *  <tr>
 *  <th>"background"</th>
 *  <td>String</th>
 *  <td>The background colour in the standard X colours format, e.g. "DarkSlateGray"
 *      or a hex specification such as "305050". The colour is used to give the fill
 *      colour of a feature.
 *  </tr>
 *  <tr>
 *  <th>"foreground"</th>
 *  <td>String</th>
 *  <td>The foreground colour in the standard X colours format, e.g. "DarkSlateGray"
 *      or a hex specification such as "305050". The colour is used to give the fill
 *      colour of a feature.
 *  </tr>
 *  <tr>
 *  <th>"width"</th>
 *  <td>Float</th>
 *  <td>The total width of the column in which the features of this type will be displayed
 *      in the ZMap feature window.
 *  </tr>
 *  <tr>
 *  <th>"showUpStrand"</th>
 *  <td>Boolean</th>
 *  <td>If true then features of this type are displayed on both the forward (down) strand
 *      and reverse (up) strands, the default is for only the forward strand features to be displayed.
 *  </tr>
 *  <tr>
 *  <th>"minmag"</th>
 *  <td>Int</th>
 *  <td>The minimum magnification at which these features should be shown, when zoomed out more
 *      than this the features are not displayed which can considerably speed up display of long
 *      sequence regions.
 *  </tr>
 * </table>
 *
 *  */




/*! @addtogroup config_files
 *
 * @section  featureWindow ZMap Feature Window Options
 * 
 * The ZMap feature window can be configured for better performance.
 *
 * <table>
 *  <tr>
 *  <th>Stanza name</th>
 *  <th colspan=2 align=center>"ZMapWindow"</th>
 *  </tr>
 *  <tr>
 *  <th>Resource Name</th>
 *  <th>Resource Type</th>
 *  <th>Resource Description</th>
 *  </tr>
 *  <tr>
 *  <th>"canvas_maxsize"</th>
 *  <td>Integer</th>
 *  <td>Set the maximum vertical extent of the feature window. Setting a smaller size will result in
 *      faster display as less data gets mapped as you zoom in but will mean you have less
 *      data to scroll over. The default is to create the maximum window size possible,
 *      usually 32k long.</th>
 *  </tr>
 * </table>
 *
 *  */





/*! @} end of config file docs. */
