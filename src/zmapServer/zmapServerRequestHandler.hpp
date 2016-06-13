/*  File: zmapServerRequestHandler.hpp
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2016: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *      Gemma Barson (Sanger Institute, UK) gb10@sanger.ac.uk
 *
 * Description: Contains the interface the slaveHandler code calls
 *              to forward server requests to the right place and
 *              return the replies. It's separated out from the public
 *              server header because it's only needed by the slave
 *              code.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_REQUESTHANDLER_H
#define ZMAP_REQUESTHANDLER_H


ZMapServerResponseType zMapServerRequest(ZMapServer *server_inout, ZMapServerReqAny request, char **err_msg_out) ;


#endif /* !ZMAP_REQUESTHANDLER_H */

