/*  File: zmapGFF.h
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
 * Description: Interface to a GFF parser, the parser works in a
 *              "line at a time" way, the caller must pass complete
 *              GFF lines to the parser which then builds up arrays
 *              of ZMapFeatureStruct's, one for each GFF source.
 *              
 * HISTORY:
 * Last edited: May 20 09:41 2005 (edgrif)
 * Created: Sat May 29 13:18:32 2004 (edgrif)
 * CVS info:   $Id: zmapGFF.h,v 1.7 2005-05-27 15:13:13 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_GFF_H
#define ZMAP_GFF_H

#include <glib.h>
#include <ZMap/zmapFeature.h>

/* An instance of a parser. */
typedef struct ZMapGFFParserStruct_ *ZMapGFFParser ;




ZMapGFFParser zMapGFFCreateParser(GData *sources, gboolean parse_only) ;

gboolean zMapGFFParseLine(ZMapGFFParser parser, char *line) ;

void zMapGFFSetStopOnError(ZMapGFFParser parser, gboolean stop_on_error) ;

void zMapGFFSetParseOnly(ZMapGFFParser parser, gboolean parse_only) ;

void zMapGFFSetSOCompliance(ZMapGFFParser parser, gboolean SO_compliant) ;

gboolean zMapGFFGetFeatures(ZMapGFFParser parser, ZMapFeatureBlock feature_block) ;

int zMapGFFGetVersion(ZMapGFFParser parser) ;

int zMapGFFGetLineNumber(ZMapGFFParser parser) ;

GError *zMapGFFGetError(ZMapGFFParser parser) ;

gboolean zMapGFFTerminated(ZMapGFFParser parser) ;

void zMapGFFSetFreeOnDestroy(ZMapGFFParser parser, gboolean free_on_destroy) ;

void zMapGFFDestroyParser(ZMapGFFParser parser) ;


#endif /* ZMAP_GFF_H */
