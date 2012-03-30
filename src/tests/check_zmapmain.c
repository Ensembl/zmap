/*  File: check_zmapmain.c
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
/* -I$(top_srcdir) */
#include <tests/zmapCheck.h>	/* check specific stuff */
#include <tests/zmapSuites.h>	/* prototypes for the ZMapCheckSuiteCreateFunc list */

#ifndef CHECK_ZMAP_LOG
#define CHECK_ZMAP_LOG "check_zmap.log"
#endif /* CHECK_ZMAP_LOG */

static Suite *zmapCheckDefaultSuite(void);

/* They will be run in this order. See tests/zmapSuites.h for available functions */
static ZMapCheckSuiteCreateFunc zmap_suite_funcs_G[] = {
  zmapCheckDefaultSuite,
  zMapCheckStyleSuite,
  zMapCheckFeatureSuite,
  zMapCheckPfetchSuite,
  zMapCheckConfigLoader,
  NULL
};


/* Local test to check we've initialised the GType system... */

START_TEST(check_initialised)
{
  char *g_object_name = NULL;
  const char *expect = "GObject";

  g_object_name = g_type_name(G_TYPE_OBJECT);

  if(g_ascii_strcasecmp(g_object_name, expect) != 0)
    {
      fail("g_type_name() failed. GType initialisation failure?");
    }
  
  return ;
}
END_TEST




/* The GType system needs initialising before anything else we do. */
static void do_global_type_init(void)
{
  printf("Initialising GType System\n");

  g_type_init_with_debug_flags(G_TYPE_DEBUG_OBJECTS | G_TYPE_DEBUG_SIGNALS);

  return ;
}

/* Nothing to do here. */
static void do_global_destroy(void)
{
  return ;
}


/* The master suite */
static Suite *check_master_suite(void)
{
  Suite *suite     = NULL;
  TCase *test_case = NULL;

  suite = suite_create("Master Suite");

  test_case = tcase_create("GLib Type Initialisation");

  /* This needs to be unchecked */
  tcase_add_unchecked_fixture(test_case, do_global_type_init, do_global_destroy);

  /* Check initialisation succeeded. */
  tcase_add_test(test_case, check_initialised);

  suite_add_tcase(suite, test_case);

  return suite;
}

/* Dull simple test. */
START_TEST(check_false)
{
  if(FALSE)
    {
      fail("FALSE is true!");
    }
}
END_TEST

static Suite *zmapCheckDefaultSuite(void)
{
  Suite *suite = NULL;
  TCase *test_case = NULL;

  suite = suite_create("Default Suite");

  test_case = tcase_create("True or False");
  tcase_add_test(test_case, check_false);

  suite_add_tcase(suite, test_case);

  return suite;
}


/*
 * And to the main...
 */

int main(int argc, char *argv[])
{
  Suite *zmap_suite    = NULL;
  SRunner *zmap_runner = NULL;
  ZMapCheckSuiteCreateFunc *all_suites_ptr;
  int failed_test_count = 0;
  int test_timeout = ZMAP_DEFAULT_TIMEOUT;

  printf("~~~~~~~~~~~~~~~~~~~~~\n");
  printf("| ZMap Test Harness |\n");
  printf("~~~~~~~~~~~~~~~~~~~~~\n");

  zmap_suite  = check_master_suite();
  zmap_runner = srunner_create (zmap_suite);

  all_suites_ptr = &zmap_suite_funcs_G[0];

  while(all_suites_ptr && all_suites_ptr[0])
    {
      Suite *suite;

      /*
       * I was going to pass the timeout through, but realised
       * that export CK_DEFAULT_TIMEOUT=0 is better.
       * if((suite = (all_suites_ptr[0])(test_timeout)))
       */

      if((suite = (all_suites_ptr[0])()))
	srunner_add_suite(zmap_runner, suite);

      all_suites_ptr++;
    }

#ifdef CHECK_ZMAP_LOG_TO_FILE
  srunner_set_log(zmap_runner, CHECK_ZMAP_LOG);
#endif /* CHECK_ZMAP_LOG_TO_FILE */

  srunner_run_all (zmap_runner, ZMAP_CHECK_OUTPUT_LEVEL);

  failed_test_count = srunner_ntests_failed (zmap_runner);

  srunner_free (zmap_runner);

  return (failed_test_count == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
