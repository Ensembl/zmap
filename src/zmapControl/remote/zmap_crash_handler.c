/*  File: zmap_crash_handler.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * Last edited: Jun 15 12:17 2007 (rds)
 * Created: Fri Jun 15 11:15:28 2007 (rds)
 * CVS info:   $Id: zmap_crash_handler.c,v 1.2 2007-07-16 17:24:55 rds Exp $
 *-------------------------------------------------------------------
 */

/*

This was one thought on how to let as external program know that we'd crashed.
It relies on a shell cover script like the following and zmap to return the
right thing when crashing/going away...

#!/bin/bash

CONFDIR=$1
WINDOWID=$2

zmap --conf_dir=$CONFDIR --wind_id=$WINDOWID || zmap_crash_handler $WINDOWID

  */

#include <ZMap/zmapXRemote.h>            /* Public header for Perl Stuff */
#include <stdlib.h>

#define ZMAP_XREMOTE_ZMAP_CRASHED "<zmap action=\"crashed\" />"

int main(int argc, char *argv[]){
  ZMapXRemoteObj obj;
  char *response;

  if(argc != 2)
    printf("Usage: %s <windowId>\n", argv[0]); /* e.g. windowId = 0x2200210 */

  if((obj = zMapXRemoteNew()))
    {
      zMapXRemoteInitClient(obj, strtoul(argv[1], (char **)NULL, 16));
      
      zMapXRemoteSetRequestAtomName (obj, ZMAP_CLIENT_REQUEST_ATOM_NAME);
      zMapXRemoteSetResponseAtomName(obj, ZMAP_CLIENT_RESPONSE_ATOM_NAME);
      zMapXRemoteSendRemoteCommand  (obj, ZMAP_XREMOTE_ZMAP_CRASHED, &response);
    }

  return 0;
}
