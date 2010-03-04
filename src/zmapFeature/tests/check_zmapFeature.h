/*  File: check_zmapFeature.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Mar 31 10:02 2009 (rds)
 * Created: Mon Mar 30 20:45:32 2009 (rds)
 * CVS info:   $Id: check_zmapFeature.h,v 1.2 2010-03-04 15:10:12 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef CHECK_ZMAP_FEATURE_H
#define CHECK_ZMAP_FEATURE_H

#include <check.h>
#include <ZMap/zmapFeature.h>


#define ZMAP_CHECK_FEATURE_SUITE_NAME "Feature Suite"

#define ZMAP_CHECK_FEATURE_NAME     "test feature name"
#define ZMAP_CHECK_FEATURE_SEQUENCE "test-sequence"
#define ZMAP_CHECK_FEATURE_ONTOLOGY "exon"
#define ZMAP_CHECK_FEATURE_MODE     ZMAPSTYLE_MODE_BASIC
#define ZMAP_CHECK_FEATURE_START    123456
#define ZMAP_CHECK_FEATURE_END      234567
#define ZMAP_CHECK_FEATURE_HASSCORE FALSE
#define ZMAP_CHECK_FEATURE_SCORE    (0.0)
#define ZMAP_CHECK_FEATURE_STRAND   ZMAPSTRAND_REVERSE
#define ZMAP_CHECK_FEATURE_PHASE    ZMAPPHASE_NONE


Suite *zMapCheckFeatureSuite(void);


Suite *zMapCheckFeaturePrivateSuite(void);


#endif /* CHECK_ZMAP_FEATURE_H */

