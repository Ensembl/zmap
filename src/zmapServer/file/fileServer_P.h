/*  File: fileServer_P.h
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Defines/types etc. for the file accessing version
 *              of the server code.
 *              
 *-------------------------------------------------------------------
 */
#ifndef FILE_SERVER_P_H
#define FILE_SERVER_P_H


#define FILE_PROTOCOL_STR "GFF File"			    /* For error messages. */




/* Holds all the state we need to access the file. */
typedef struct _FileServerStruct
{
  gchar *file_path ;
  GIOChannel* gff_file ;

  char *styles_file ;

  gboolean error ;					    /* TRUE if any error occurred. */
  char *last_err_msg ;

  ZMapFeatureContext req_context ;

} FileServerStruct, *FileServer ;


#endif /* !FILE_SERVER_P_H */
