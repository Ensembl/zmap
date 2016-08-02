/*  File: zmapDataSource_P.hpp
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk,
 *      Gemma Barson (Sanger Institute, UK) gb10@sanger.ac.uk
 *
 * Description: Private types etc for DataSource package.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAPDATASOURCE_P_H
#define ZMAPDATASOURCE_P_H

#include <ZMap/zmapDataSource.hpp>

ZMapThreadReturnCode zmapDataSourceThreadRequestHandler(void **slave_data, void *request_in, char **err_msg_out) ;
ZMapThreadReturnCode zmapDataSourceThreadTerminateHandler(void **slave_data, char **err_msg_out) ;
ZMapThreadReturnCode zmapDataSourceThreadDestroyHandler(void **slave_data) ;

#endif /* ZMAPDATASOURCE_P_H */
