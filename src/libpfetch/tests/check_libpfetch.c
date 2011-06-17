/*  File: check_libpfetch.c
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <check_libpfetch.h>

typedef struct
{
  PFetchHandle pfetch;
  GMainLoop *main_loop;
  gboolean reply_not_empty;
  gboolean reply_was_no_match;
} CheckPfetchDataStruct, *CheckPfetchData;


static PFetchHandle pfetch_create(GType pfetch_type);
static PFetchHandle pfetch_create_setup(GType pfetch_type);
static PFetchStatus reader_func(PFetchHandle handle,
				char        *text,
				guint       *actual_read,
				GError      *error,
				gpointer     user_data);
static PFetchStatus error_func(PFetchHandle handle,
			       char        *text,
			       guint       *actual_read,
			       GError      *error,
			       gpointer     user_data);
static PFetchStatus closed_func(PFetchHandle pfetch, gpointer user_data);
static gboolean check_pfetch_fetch(CheckPfetchData check_data);

/* TESTS */

START_TEST(check_pfetch_create)
{
  PFetchHandle pfetch = NULL;

  pfetch = pfetch_create(PFETCH_TYPE_PIPE_HANDLE);

  PFetchHandleDestroy(pfetch);

  return ;
}
END_TEST


START_TEST(check_pfetch_entry)
{
  CheckPfetchData check_data;
  PFetchHandle pfetch = NULL;
  GObject *object = NULL;

  pfetch = pfetch_create_setup(PFETCH_TYPE_PIPE_HANDLE);

  object = G_OBJECT(pfetch);

  check_data = g_new0(CheckPfetchDataStruct, 1);

  check_data->pfetch = pfetch;

  g_signal_connect(object, "reader", G_CALLBACK(reader_func), check_data);
  g_signal_connect(object, "error",  G_CALLBACK(error_func),  check_data);
  g_signal_connect(object, "closed", G_CALLBACK(closed_func), check_data);

  check_data->main_loop = g_main_loop_new(NULL, FALSE);

  g_idle_add((GSourceFunc)check_pfetch_fetch, check_data);

  g_main_loop_run(check_data->main_loop);

  return ;
}
END_TEST



/* INTERNALS */

static PFetchHandle pfetch_create(GType pfetch_type)
{
  PFetchHandle pfetch = NULL;

  pfetch = PFetchHandleNew(pfetch_type);

  if(!pfetch)
    {
      fail("Object creation failed");
    }

  return pfetch;
}

static PFetchHandle pfetch_create_setup(GType pfetch_type)
{
  PFetchHandle pfetch = NULL;

  pfetch = pfetch_create(pfetch_type);

  PFetchHandleSettings(pfetch,
		       "debug",       CHECK_PFETCH_DEBUG,
		       "full",        CHECK_PFETCH_FULL,
		       "pfetch",      CHECK_PFETCH_LOCATION,
		       "isoform-seq", CHECK_PFETCH_ISOFORM_SEQ,
		       NULL);

  if(PFETCH_IS_HTTP_HANDLE(pfetch))
    {
      PFetchHandleSettings(pfetch,
			   "port",       CHECK_PFETCH_PORT,
			   "cookie-jar", CHECK_PFETCH_COOKIE_JAR,
			   NULL);
    }

  return pfetch;
}

static PFetchStatus reader_func(PFetchHandle handle,
				char        *text,
				guint       *actual_read,
				GError      *error,
				gpointer     user_data)
{
  PFetchStatus status = PFETCH_STATUS_OK;
  CheckPfetchData check_data = (CheckPfetchData)user_data;

  if(actual_read && *actual_read > 0)
    {
      check_data->reply_not_empty = TRUE;

      if(g_ascii_strncasecmp(text, CHECK_PFETCH_NO_MATCH, strlen(CHECK_PFETCH_NO_MATCH)) == 0)
	{
	  check_data->reply_was_no_match = TRUE;
	}

      printf("%s", text);
    }

  return status;
}

static PFetchStatus error_func(PFetchHandle handle,
			       char        *text,
			       guint       *actual_read,
			       GError      *error,
			       gpointer     user_data)
{
  PFetchStatus status = PFETCH_STATUS_OK;

  printf("%s\n", text);

  PFetchHandleDestroy(handle);

  fail("Error from " CHECK_PFETCH_LOCATION);

  return status;
}

static PFetchStatus closed_func(PFetchHandle pfetch, gpointer user_data)
{
  PFetchStatus status = PFETCH_STATUS_OK;
  CheckPfetchData check_data = (CheckPfetchData)user_data;

  if(check_data->main_loop)
    g_main_loop_quit(check_data->main_loop);

  if(check_data->pfetch)
    {
      PFetchHandleDestroy(pfetch);
      check_data->pfetch = NULL;
    }

  if(!check_data->reply_not_empty)
    fail("Received no response from " CHECK_PFETCH_LOCATION);

  if(check_data->reply_was_no_match)
    fail("Received '" CHECK_PFETCH_NO_MATCH "' response from " CHECK_PFETCH_LOCATION);

  g_free(check_data);

  return status;
}

static gboolean check_pfetch_fetch(CheckPfetchData check_data)
{
  PFetchHandle pfetch;

  pfetch = check_data->pfetch;

  if(PFetchHandleFetch(pfetch, CHECK_PFETCH_SEQUENCE) != PFETCH_STATUS_OK)
    {
      g_main_loop_quit(check_data->main_loop);

      fail("PFetchHandleFetch Failed");
    }

  return FALSE;
}


/* SUITE CREATOR */

Suite *zMapCheckPfetchSuite(void)
{
  Suite *suite = NULL;
  TCase *test_case = NULL;

  suite = suite_create(PFETCH_SUITE_NAME);

  test_case = tcase_create(PFETCH_SUITE_NAME " Tests");

  tcase_add_test(test_case, check_pfetch_create);

  tcase_add_test(test_case, check_pfetch_entry);

  tcase_set_timeout(test_case, 60);

  suite_add_tcase(suite, test_case);

  return suite;
}

