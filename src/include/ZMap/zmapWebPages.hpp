/*  File: zmapWebPages.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Contains references to zmap web pages for current
 *              code release.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WEBPAGES_H
#define ZMAP_WEBPAGES_H


/* The docs should be installed alongside the executable in the share directory.
 * The share directory lives in the same parent directory as the bin directory
 * where the executable is, so we need to find the bin directory and then use
 * this relative path */
#define ZMAP_LOCAL_DOC_PATH "../share/doc/zmap"

/* Reorganising the zmap_update_web.sh and the directories, together
 * with inertia of the way the last release code works, means that the
 * Release_notes dir is no longer under the DOC_URL dir. */


#define ZMAPWEB_RELEASE_NOTES "release_notes.shtml"

#define ZMAPWEB_HELP_DOC  "index.shtml"
#define ZMAPWEB_HELP_QUICK_START  "zmap_quick_start.shtml"
#define ZMAPWEB_HELP_ALIGNMENT_SECTION "user_interface.shtml"
/* gb10: for some reason the open command fails with this: user_interface.shtml#alignment_display */

#define ZMAPWEB_KEYBOARD_DOC  "keyboard_mouse.shtml"
#define ZMAPWEB_FILTER_DOC  "filter_window.shtml"
#define ZMAPWEB_MANUAL  "ZMap_User_Manual.pdf"


#endif /* ZMAP_WEBPAGES_H */
