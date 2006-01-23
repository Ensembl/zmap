/*  File: zmapFeature_P.h
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
 * Description: Internals for zmapFeature routines.
 *
 * HISTORY:
 * Last edited: Jan 23 13:48 2006 (edgrif)
 * Created: Wed Nov 24 11:01:24 2004 (edgrif)
 * CVS info:   $Id: zmapFeature_P.h,v 1.2 2006-01-23 14:10:55 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_FEATURE_P_H
#define ZMAP_FEATURE_P_H

#include <ZMap/zmapFeature.h>


#define zmapFeatureSwop(TYPE, FIRST, SECOND)   \
  { TYPE tmp = (FIRST) ; (FIRST) = (SECOND) ; (SECOND) = tmp ; }


#define zmapFeatureInvert(COORD, SEQ_END)	\
  (COORD) = (SEQ_END) - (COORD) + 1


#define zmapFeatureRevComp(TYPE, SEQ_END, COORD_1, COORD_2)  \
  {                                                        \
    zmapFeatureSwop(TYPE, COORD_1, COORD_2) ;	           \
    zmapFeatureInvert(COORD_1, SEQ_END) ;                    \
    zmapFeatureInvert(COORD_2, SEQ_END) ;                    \
  }


void zmapPrintFeatureContext(ZMapFeatureContext context) ;



#endif /* !ZMAP_FEATURE_P_H */
