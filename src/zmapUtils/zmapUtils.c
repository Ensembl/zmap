/*  File: zmapUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * Description: Utility functions for the ZMap code.
 *              
 * Exported functions: See ZMap/zmapUtils.h
 * HISTORY:
 * Last edited: Jul  1 16:50 2004 (edgrif)
 * Created: Fri Mar 12 08:16:24 2004 (edgrif)
 * CVS info:   $Id: zmapUtils.c,v 1.3 2004-07-02 18:24:11 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapUtils_P.h>



/*! @defgroup zmaputils   zMapUtils: utilities for ZMap
 * @{
 * 
 * \brief  Utilities for ZMap.
 * 
 * zMapUtils routines provide services such as debugging, testing and logging,
 * string handling, file utilities and GUI functions. They are general routines
 * used by all of ZMap.
 *
 *  */




/*!
 * Returns a single number representing the Version/Release/Update of the ZMap code.
 *
 * @param   void  None.
 * @return        The version as an integer.
 *  */
int zMapGetVersion(void)
{
  return ZMAP_MAKE_VERSION_NUMBER(ZMAP_VERSION, ZMAP_RELEASE, ZMAP_UPDATE) ;
}


/*!
 * Returns a string representing the Version/Release/Update of the ZMap code.
 *
 * @param   void  None.
 * @return        The version number as a string.
 *  */
char *zMapGetVersionString(void)
{
  return ZMAP_MAKE_VERSION_STRING(ZMAP_VERSION, ZMAP_RELEASE, ZMAP_UPDATE) ;
}


/*!
 * Returns a string which is the name of the ZMap application.
 *
 * @param   void  None.
 * @return        The applications name as a string.
 *  */
char *zMapGetAppName(void)
{
  return ZMAP_TITLE ;
}


/*!
 * Returns a string which is a brief decription of the ZMap application.
 *
 * @param   void  None.
 * @return        The applications description as a string.
 *  */
char *zMapGetAppTitle(void)
{
  return ZMAP_MAKE_TITLE_STRING(ZMAP_TITLE, ZMAP_VERSION, ZMAP_RELEASE, ZMAP_UPDATE) ;
}


/*!
 * Returns a copyright string for the ZMap application.
 *
 * @param void  None.
 * @return      The copyright as a string.
 *  */
char *zMapGetCopyrightString(void)
{
  return ZMAP_COPYRIGHT_STRING(ZMAP_TITLE, ZMAP_VERSION, ZMAP_RELEASE, ZMAP_UPDATE, ZMAP_DESCRIPTION) ;
}



/*!
 * Formats and displays a message, note that the message may be displayed in a window
 * or at the terminal depending on how the message system has been initialised.
 *
 * @param msg_type      The type of message: information, warning etc.
 * @param format        A printf() style format string.
 * @param ...           The parameters matching the format string.
 * @return              nothing
 *  */
void zMapShowMsg(ZMapMsgType msg_type, char *format, ...)
{
  va_list args ;
  char *msg_string ;

  va_start(args, format) ;
  msg_string = g_strdup_vprintf(format, args) ;
  va_end(args) ;


  zmapGUIShowMsg(msg_type, msg_string) ;
  
  g_free(msg_string) ;

  return ;
}

/*! @} end of zmaputils docs. */

