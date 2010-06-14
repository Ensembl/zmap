/*  Last edited: Mar  8 15:46 2005 (rds) */
/* Various functions of utilitarian nature.
   Copyright (C) 1995, 1996, 1997, 1998, 2000, 2001
   Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

In addition, as a special exception, the Free Software Foundation
gives permission to link the code of its release of Wget with the
OpenSSL project''s "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.  */

#include <ZMap/zmap.h>






#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else  /* not HAVE_STRING_H */
# include <strings.h>
#endif /* not HAVE_STRING_H */
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_MMAP
# include <sys/mman.h>
#endif
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
#include <limits.h>
#ifdef HAVE_UTIME_H
# include <utime.h>
#endif
#ifdef HAVE_SYS_UTIME_H
# include <sys/utime.h>
#endif
#include <errno.h>
#ifdef NeXT
# include <libc.h>		/* for access() */
#endif
#include <fcntl.h>
#include <assert.h>

/* For TIOCGWINSZ and friends: */
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif

#include <ZMap/zmapUrlUtils.h>
#include <glib.h>
#ifndef errno
extern int errno;
#endif

/* This section implements several wrappers around the basic
   allocation routines.  This is done for two reasons: first, so that
   the callers of these functions need not consistently check for
   errors.  If there is not enough virtual memory for running Wget,
   something is seriously wrong, and Wget exits with an appropriate
   error message.

   The second reason why these are useful is that, if DEBUG_MALLOC is
   defined, they also provide a handy (if crude) malloc debugging
   interface that checks memory leaks.  */

/* Croak the fatal memory error and bail out with non-zero exit
   status.  */
static void
memfatal (const char *what)
{
  /* Make sure we don't try to store part of the log line, and thus
     call malloc.  */
  //  log_set_save_context (0);
  //logprintf (LOG_ALWAYS, _(": %s: Not enough memory.\n"), what);
  g_log("ZMAP",G_LOG_FLAG_FATAL, "%s: Not enough memory.\n",what);
  exit (1);
}

/* These functions end with _real because they need to be
   distinguished from the debugging functions, and from the macros.
   Explanation follows:

   If memory debugging is not turned on, wget.h defines these:

     #define xmalloc xmalloc_real
     #define xrealloc xrealloc_real
     #define xstrdup xstrdup_real
     #define xfree free

   In case of memory debugging, the definitions are a bit more
   complex, because we want to provide more information, *and* we want
   to call the debugging code.  (The former is the reason why xmalloc
   and friends need to be macros in the first place.)  Then it looks
   like this:

     #define xmalloc(a) xmalloc_debug (a, __FILE__, __LINE__)
     #define xfree(a)   xfree_debug (a, __FILE__, __LINE__)
     #define xrealloc(a, b) xrealloc_debug (a, b, __FILE__, __LINE__)
     #define xstrdup(a) xstrdup_debug (a, __FILE__, __LINE__)

   Each of the *_debug function does its magic and calls the real one.  */

#ifdef DEBUG_MALLOC
# define STATIC_IF_DEBUG static
#else
# define STATIC_IF_DEBUG
#endif

STATIC_IF_DEBUG void *
xmalloc_real (size_t size)
{
  void *ptr = malloc (size);
  if (!ptr)
    memfatal ("malloc");
  return ptr;
}

STATIC_IF_DEBUG void *
xrealloc_real (void *ptr, size_t newsize)
{
  void *newptr;

  /* Not all Un*xes have the feature of realloc() that calling it with
     a NULL-pointer is the same as malloc(), but it is easy to
     simulate.  */
  if (ptr)
    newptr = realloc (ptr, newsize);
  else
    newptr = malloc (newsize);
  if (!newptr)
    memfatal ("realloc");
  return newptr;
}

STATIC_IF_DEBUG char *
xstrdup_real (const char *s)
{
  char *copy;

#ifndef HAVE_STRDUP
  int l = strlen (s);
  copy = malloc (l + 1);
  if (!copy)
    memfatal ("strdup");
  memcpy (copy, s, l + 1);
#else  /* HAVE_STRDUP */
  copy = strdup (s);
  if (!copy)
    memfatal ("strdup");
#endif /* HAVE_STRDUP */

  return copy;
}

/* Utility function: like xstrdup(), but also lowercases S.  */

char *
xstrdup_lower (const char *s)
{
  char *copy = xstrdup (s);
  char *p = copy;
  for (; *p; p++)
    *p = TOLOWER (*p);
  return copy;
}

/* Copy the string formed by two pointers (one on the beginning, other
   on the char after the last char) to a new, malloc-ed location.
   0-terminate it.  */
char *
strdupdelim (const char *beg, const char *end)
{
  char *res = (char *)xmalloc (end - beg + 1);
  memcpy (res, beg, end - beg);
  res[end - beg] = '\0';
  return res;
}

/* Parse a string containing comma-separated elements, and return a
   vector of char pointers with the elements.  Spaces following the
   commas are ignored.  */
char **
sepstring (const char *s)
{
  char **res;
  const char *p;
  int i = 0;

  if (!s || !*s)
    return NULL;
  res = NULL;
  p = s;
  while (*s)
    {
      if (*s == ',')
	{
	  res = (char **)xrealloc (res, (i + 2) * sizeof (char *));
	  res[i] = strdupdelim (p, s);
	  res[++i] = NULL;
	  ++s;
	  /* Skip the blanks following the ','.  */
	  while (ISSPACE (*s))
	    ++s;
	  p = s;
	}
      else
	++s;
    }
  res = (char **)xrealloc (res, (i + 2) * sizeof (char *));
  res[i] = strdupdelim (p, s);
  res[i + 1] = NULL;
  return res;
}

/* Return the size of file named by FILENAME, or -1 if it cannot be
   opened or seeked into. */
long
file_size (const char *filename)
{
  long size;
  /* We use fseek rather than stat to determine the file size because
     that way we can also verify whether the file is readable.
     Inspired by the POST patch by Arnaud Wylie.  */
  FILE *fp = fopen (filename, "rb");
  if (!fp)
    return -1;
  fseek (fp, 0, SEEK_END);
  size = ftell (fp);
  fclose (fp);
  return size;
}

/* Merge BASE with FILE.  BASE can be a directory or a file name, FILE
   should be a file name.

   file_merge("/foo/bar", "baz")  => "/foo/baz"
   file_merge("/foo/bar/", "baz") => "/foo/bar/baz"
   file_merge("foo", "bar")       => "bar"

   In other words, it's a simpler and gentler version of uri_merge_1.  */

char *
file_merge (const char *base, const char *file)
{
  char *result;
  const char *cut = (const char *)strrchr (base, '/');

  if (!cut)
    return xstrdup (file);

  result = (char *)xmalloc (cut - base + 1 + strlen (file) + 1);
  memcpy (result, base, cut - base);
  result[cut - base] = '/';
  strcpy (result + (cut - base) + 1, file);

  return result;
}


/* Compare S1 and S2 frontally; S2 must begin with S1.  E.g. if S1 is
   `/something', frontcmp() will return 1 only if S2 begins with
   `/something'.  Otherwise, 0 is returned.  */
int
frontcmp (const char *s1, const char *s2)
{
  for (; *s1 && *s2 && (*s1 == *s2); ++s1, ++s2);
  return !*s1;
}

/* Return non-zero if STRING ends with TAIL.  For instance:

   match_tail ("abc", "bc", 0)  -> 1
   match_tail ("abc", "ab", 0)  -> 0
   match_tail ("abc", "abc", 0) -> 1

   If FOLD_CASE_P is non-zero, the comparison will be
   case-insensitive.  */

int
match_tail (const char *string, const char *tail, int fold_case_p)
{
  int i, j;

  /* We want this to be fast, so we code two loops, one with
     case-folding, one without. */

  if (!fold_case_p)
    {
      for (i = strlen (string), j = strlen (tail); i >= 0 && j >= 0; i--, j--)
	if (string[i] != tail[j])
	  break;
    }
  else
    {
      for (i = strlen (string), j = strlen (tail); i >= 0 && j >= 0; i--, j--)
	if (TOLOWER (string[i]) != TOLOWER (tail[j]))
	  break;
    }

  /* If the tail was exhausted, the match was succesful.  */
  if (j == -1)
    return 1;
  else
    return 0;
}

/* Count the digits in a (long) integer.  */
int
numdigit (long number)
{
  int cnt = 1;
  if (number < 0)
    {
      number = -number;
      ++cnt;
    }
  while ((number /= 10) > 0)
    ++cnt;
  return cnt;
}

/* A half-assed implementation of INT_MAX on machines that don't
   bother to define one. */
#ifndef INT_MAX
# define INT_MAX ((int) ~((unsigned)1 << 8 * sizeof (int) - 1))
#endif

#define ONE_DIGIT(figure) *p++ = n / (figure) + '0'
#define ONE_DIGIT_ADVANCE(figure) (ONE_DIGIT (figure), n %= (figure))

#define DIGITS_1(figure) ONE_DIGIT (figure)
#define DIGITS_2(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_1 ((figure) / 10)
#define DIGITS_3(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_2 ((figure) / 10)
#define DIGITS_4(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_3 ((figure) / 10)
#define DIGITS_5(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_4 ((figure) / 10)
#define DIGITS_6(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_5 ((figure) / 10)
#define DIGITS_7(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_6 ((figure) / 10)
#define DIGITS_8(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_7 ((figure) / 10)
#define DIGITS_9(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_8 ((figure) / 10)
#define DIGITS_10(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_9 ((figure) / 10)

/* DIGITS_<11-20> are only used on machines with 64-bit longs. */

#define DIGITS_11(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_10 ((figure) / 10)
#define DIGITS_12(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_11 ((figure) / 10)
#define DIGITS_13(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_12 ((figure) / 10)
#define DIGITS_14(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_13 ((figure) / 10)
#define DIGITS_15(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_14 ((figure) / 10)
#define DIGITS_16(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_15 ((figure) / 10)
#define DIGITS_17(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_16 ((figure) / 10)
#define DIGITS_18(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_17 ((figure) / 10)
#define DIGITS_19(figure) ONE_DIGIT_ADVANCE (figure); DIGITS_18 ((figure) / 10)

/* Print NUMBER to BUFFER in base 10.  This should be completely
   equivalent to `sprintf(buffer, "%ld", number)', only much faster.

   The speedup may make a difference in programs that frequently
   convert numbers to strings.  Some implementations of sprintf,
   particularly the one in GNU libc, have been known to be extremely
   slow compared to this function.

   Return the pointer to the location where the terminating zero was
   printed.  (Equivalent to calling buffer+strlen(buffer) after the
   function is done.)

   BUFFER should be big enough to accept as many bytes as you expect
   the number to take up.  On machines with 64-bit longs the maximum
   needed size is 24 bytes.  That includes the digits needed for the
   largest 64-bit number, the `-' sign in case it's negative, and the
   terminating '\0'.  */

char *
number_to_string (char *buffer, long number)
{
  char *p = buffer;
  long n = number;

#if (SIZEOF_LONG != 4) && (SIZEOF_LONG != 8)
  /* We are running in a strange or misconfigured environment.  Let
     sprintf cope with it.  */
  sprintf (buffer, "%ld", n);
  p += strlen (buffer);
#else  /* (SIZEOF_LONG == 4) || (SIZEOF_LONG == 8) */

  if (n < 0)
    {
      if (n < -INT_MAX)
	{
	  /* We cannot print a '-' and assign -n to n because -n would
	     overflow.  Let sprintf deal with this border case.  */
	  sprintf (buffer, "%ld", n);
	  p += strlen (buffer);
	  return p;
	}

      *p++ = '-';
      n = -n;
    }

  if      (n < 10)                   { DIGITS_1 (1); }
  else if (n < 100)                  { DIGITS_2 (10); }
  else if (n < 1000)                 { DIGITS_3 (100); }
  else if (n < 10000)                { DIGITS_4 (1000); }
  else if (n < 100000)               { DIGITS_5 (10000); }
  else if (n < 1000000)              { DIGITS_6 (100000); }
  else if (n < 10000000)             { DIGITS_7 (1000000); }
  else if (n < 100000000)            { DIGITS_8 (10000000); }
  else if (n < 1000000000)           { DIGITS_9 (100000000); }
#if SIZEOF_LONG == 4
  /* ``if (1)'' serves only to preserve editor indentation. */
  else if (1)                        { DIGITS_10 (1000000000); }
#else  /* SIZEOF_LONG != 4 */
  else if (n < 10000000000L)         { DIGITS_10 (1000000000L); }
  else if (n < 100000000000L)        { DIGITS_11 (10000000000L); }
  else if (n < 1000000000000L)       { DIGITS_12 (100000000000L); }
  else if (n < 10000000000000L)      { DIGITS_13 (1000000000000L); }
  else if (n < 100000000000000L)     { DIGITS_14 (10000000000000L); }
  else if (n < 1000000000000000L)    { DIGITS_15 (100000000000000L); }
  else if (n < 10000000000000000L)   { DIGITS_16 (1000000000000000L); }
  else if (n < 100000000000000000L)  { DIGITS_17 (10000000000000000L); }
  else if (n < 1000000000000000000L) { DIGITS_18 (100000000000000000L); }
  else                               { DIGITS_19 (1000000000000000000L); }
#endif /* SIZEOF_LONG != 4 */

  *p = '\0';
#endif /* (SIZEOF_LONG == 4) || (SIZEOF_LONG == 8) */

  return p;
}

#undef ONE_DIGIT
#undef ONE_DIGIT_ADVANCE

#undef DIGITS_1
#undef DIGITS_2
#undef DIGITS_3
#undef DIGITS_4
#undef DIGITS_5
#undef DIGITS_6
#undef DIGITS_7
#undef DIGITS_8
#undef DIGITS_9
#undef DIGITS_10
#undef DIGITS_11
#undef DIGITS_12
#undef DIGITS_13
#undef DIGITS_14
#undef DIGITS_15
#undef DIGITS_16
#undef DIGITS_17
#undef DIGITS_18
#undef DIGITS_19

/* Support for timers. */

#undef TIMER_WINDOWS
#undef TIMER_GETTIMEOFDAY
#undef TIMER_TIME

/* Depending on the OS and availability of gettimeofday(), one and
   only one of the above constants will be defined.  Virtually all
   modern Unix systems will define TIMER_GETTIMEOFDAY; Windows will
   use TIMER_WINDOWS.  TIMER_TIME is a catch-all method for
   non-Windows systems without gettimeofday.

   #### Perhaps we should also support ftime(), which exists on old
   BSD 4.2-influenced systems?  (It also existed under MS DOS Borland
   C, if memory serves me.)  */

#ifdef WINDOWS
# define TIMER_WINDOWS
#else  /* not WINDOWS */
# ifdef HAVE_GETTIMEOFDAY
#  define TIMER_GETTIMEOFDAY
# else
#  define TIMER_TIME
# endif
#endif /* not WINDOWS */

#ifdef TIMER_GETTIMEOFDAY
typedef struct timeval wget_sys_time;
#endif

#ifdef TIMER_TIME
typedef time_t wget_sys_time;
#endif

#ifdef TIMER_WINDOWS
typedef ULARGE_INTEGER wget_sys_time;
#endif

struct wget_timer {
  /* The starting point in time which, subtracted from the current
     time, yields elapsed time. */
  wget_sys_time start;

  /* The most recent elapsed time, calculated by wtimer_elapsed().
     Measured in milliseconds.  */
  double elapsed_last;

  /* Approximately, the time elapsed between the true start of the
     measurement and the time represented by START.  */
  double elapsed_pre_start;
};
/* This should probably be at a better place, but it doesn't really
   fit into html-parse.c.  */

/* The function returns the pointer to the malloc-ed quoted version of
   string s.  It will recognize and quote numeric and special graphic
   entities, as per RFC1866:

   `&' -> `&amp;'
   `<' -> `&lt;'
   `>' -> `&gt;'
   `"' -> `&quot;'
   SP  -> `&#32;'

   No other entities are recognized or replaced.  */
char *
html_quote_string (const char *s)
{
  const char *b = s;
  char *p, *res;
  int i;

  /* Pass through the string, and count the new size.  */
  for (i = 0; *s; s++, i++)
    {
      if (*s == '&')
	i += 4;			/* `amp;' */
      else if (*s == '<' || *s == '>')
	i += 3;			/* `lt;' and `gt;' */
      else if (*s == '\"')
	i += 5;			/* `quot;' */
      else if (*s == ' ')
	i += 4;			/* #32; */
    }
  res = (char *)xmalloc (i + 1);
  s = b;
  for (p = res; *s; s++)
    {
      switch (*s)
	{
	case '&':
	  *p++ = '&';
	  *p++ = 'a';
	  *p++ = 'm';
	  *p++ = 'p';
	  *p++ = ';';
	  break;
	case '<': case '>':
	  *p++ = '&';
	  *p++ = (*s == '<' ? 'l' : 'g');
	  *p++ = 't';
	  *p++ = ';';
	  break;
	case '\"':
	  *p++ = '&';
	  *p++ = 'q';
	  *p++ = 'u';
	  *p++ = 'o';
	  *p++ = 't';
	  *p++ = ';';
	  break;
	case ' ':
	  *p++ = '&';
	  *p++ = '#';
	  *p++ = '3';
	  *p++ = '2';
	  *p++ = ';';
	  break;
	default:
	  *p++ = *s;
	}
    }
  *p = '\0';
  return res;
}

