/*  File: check_zmapStyle.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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

#include <ZMap/zmap.h>






#include <check_zmapStyle.h>

/* Unfortunately it looks like we rely on globals here... */


/* The START_TEST declares them as static! */
START_TEST (check_zMapStyleCreate)
{
  ZMapFeatureTypeStyle style = NULL;
  char *name        = "MyStyleName"; /* Mixed case for test below */
  char *description = "My description is this";
  GQuark test_quark;

  style = zMapStyleCreate(name, description);

  test_quark = g_quark_from_string(name);

  fail_unless(style != NULL,
	      "Object creation failed");

  /* Check the style has an id which matches original id */
  fail_unless(zMapStyleGetID(style) == test_quark,
	      "Style Name id incorrect");

  /* Check the style has a canonicalised unique id */
  fail_unless(zMapStyleGetUniqueID(style) != test_quark,
	      "Style Unique id incorrect");

  zMapStyleDestroy(style);
}
END_TEST







/* Create a suite with access to _all_ the static tests */
/* We can then have an array of suites to run... */
Suite *zMapCheckStyleSuite(void)
{
  Suite *suite = NULL;
  TCase *test_case = NULL;

  /* Create the test suite for zmap styles */
  suite = suite_create(ZMAP_CHECK_STYLE_SUITE_NAME);

  /* Create a test case to test instantiation... */
  test_case = tcase_create(ZMAP_CHECK_STYLE_SUITE_NAME " Tests");

  tcase_add_test(test_case, check_zMapStyleCreate);

  suite_add_tcase(suite, test_case);


  return suite;
}


/* 
 * Things I want to do...
 * 
 * - Some unit testing of the gobject code, object creation etc...
 * - Some unit testing of the limits in get/set property code
 * - Assess test coverage.
 * - Integrate this with autotools.
 *
 */


#ifdef TEST_COVERAGE_EXAMPLE_DOC

    -:   41:     * object */
   18:   42:    if (ht->table[p] != NULL) {
    -:   43:        /* replaces the current entry */
#####:   44:        ht->count--;
#####:   45:        ht->size -= ht->table[p]->size +
#####:   46:          sizeof(struct hashtable_entry);   

      /*
	First column is the only real interest and has the following key:
	-        = unexecutable code (comment, ifdef, etc...)
	[number] = count of execution
	#####    = Executable, but never executed! 

      */

      /* Perl code to assess coverage... */

      /*
	#!/usr/local/bin/perl

	use strict;
	use warnings;
	
	# hash to hold onto line status
	my $coverage = {
	'lines_total'      => 0, 
	'lines_executed'   => 0,
	'lines_unexecuted' => 0,
	'lines_empty'      => 0,
	};


	# loop through file start
	my @columns = split($line, /:/, 3);

	my $execution_state = $columns[0];

	$coverage{'lines_total'}++;

	if($execution_state =~ m/(\d+)/)
	{
	  $coverage{'lines_executed'}++;
	}
	else if($execution_state =~ m/\-/)
	{
	  $coverage{'lines_empty'}++;
	}
	else if($execution_state =~ m/\#\#\#\#\#/)
	{
	  $coverage{'lines_unexecuted'}++;
	}
	else
	{
	  $coverage{'lines_unknown'}++;
	}
	# end of loop

	
	# calulate coverage
	printf("Coverage statistics for %s\n", $0);
	printf("Test Coverage [Lines executed/Lines not executed] = %f%%\n", 
	       $coverage{'lines_executed'} / $lines{'lines_unexecuted'} * 100);

	printf("Comment coverage = %f%%\n",
	       $coverage{'lines_empty'} / $lines{'lines_total'} * 100);

	
      */

#endif /* TEST_COVERAGE_EXAMPLE_DOC */


