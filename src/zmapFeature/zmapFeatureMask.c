/*  File: zmapFeatureMask.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:   masks EST features with mRNAs subject to configuration
 *                NOTE see Design_notes/notes/EST_mRNA.shtml
 *                sets flags in the context per feature to say masked or not
 *                that display code can use
 *
 * Created: Fri Jul 23 2010 (mh17)
 * CVS info:   $Id: zmapFeatureMask.c,v 1.1 2010-07-23 10:26:12 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <glib.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtils.h>
#include <zmapFeature_P.h>


/*
 * for masking ESTs by mRNAs we first merge in the new featuresets
 * then for each new featureset we determine if it masks another featureset
 * and store this info somewhere
 * then if there are any featuresets to mask we process each one in turn
 * each featureset masking operation is 1-1 and we only mask with new data
 * masking code just works on the merged context and a list of featuresets
 */



// mask ESTs with mRNAs if configured
void zMapFeatureMaskFeatureSets(view->features, diff_context->feature_set_names);
{
}
