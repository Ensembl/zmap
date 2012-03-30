/*  File: zmapWindowTranscriptFeature.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_TRANSCRIPT_FEATURE_H
#define ZMAP_WINDOW_TRANSCRIPT_FEATURE_H


#define ZMAP_WINDOW_TRANSCRIPT_FEATURE_NAME "ZMapWindowTranscriptFeature"

#define ZMAP_TYPE_WINDOW_TRANSCRIPT_FEATURE           (zMapWindowTranscriptFeatureGetType())

#if GOBJ_CAST
#define ZMAP_WINDOW_TRANSCRIPT_FEATURE(obj)              ((ZMapWindowTranscriptFeature) obj)
#define ZMAP_WINDOW_TRANSCRIPT_FEATURE_CONST(obj)     ((ZMapWindowTranscriptFeature const) obj)
#else
#define ZMAP_WINDOW_TRANSCRIPT_FEATURE(obj)	         (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_TRANSCRIPT_FEATURE, zmapWindowTranscriptFeature))
#define ZMAP_WINDOW_TRANSCRIPT_FEATURE_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_TRANSCRIPT_FEATURE, zmapWindowTranscriptFeature const))
#endif

#define ZMAP_WINDOW_TRANSCRIPT_FEATURE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_TRANSCRIPT_FEATURE, zmapWindowTranscriptFeatureClass))
#define ZMAP_IS_WINDOW_TRANSCRIPT_FEATURE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_TRANSCRIPT_FEATURE))
#define ZMAP_WINDOW_TRANSCRIPT_FEATURE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_TRANSCRIPT_FEATURE, zmapWindowTranscriptFeatureClass))


typedef enum
  {
    ZMAP_WINDOW_TRANSCRIPT_0,   /* invalid */
    ZMAP_WINDOW_TRANSCRIPT_INTRON,
    ZMAP_WINDOW_TRANSCRIPT_EXON,
    ZMAP_WINDOW_TRANSCRIPT_EXON_CDS
  } ZMapWindowTranscriptFeatureType ;


/* Instance */
typedef struct _zmapWindowTranscriptFeatureStruct  zmapWindowTranscriptFeature, *ZMapWindowTranscriptFeature ;


/* Class */
typedef struct _zmapWindowTranscriptFeatureClassStruct  zmapWindowTranscriptFeatureClass, *ZMapWindowTranscriptFeatureClass ;


/* Public funcs */
GType zMapWindowTranscriptFeatureGetType(void);


#endif /* ZMAP_WINDOW_TRANSCRIPT_FEATURE_H */
