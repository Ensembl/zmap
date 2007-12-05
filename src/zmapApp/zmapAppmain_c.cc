/*  File: zmapAppmain_C.C
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * Description: C++ main, if the executable is compiled/linked with
 *              main then we can use C++ classes from our code.
 * Exported functions: None.
 * HISTORY:
 * Created: Thu Nov 13 14:38:41 2003 (edgrif)
 * CVS info:   $Id: zmapAppmain_c.cc,v 1.2 2007-12-05 14:19:52 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapApp_P.h>


// dummychange....   

int main(int argc, char *argv[])
{
  int main_rc ;
  
  // cout << "about to call zmap \"main\"" << endl ;

  main_rc = zmapMainMakeAppWindow(argc, argv) ;

  // cout << "I shouldn't have got here..." << endl ;

  return(main_rc) ;
}
