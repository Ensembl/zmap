
#include <ZMap/zmap.hpp>





/* URL handling.
   Copyright (C) 1995, 1996, 1997, 2000, 2001, 2003, 2003
   Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

In addition, as a special exception, the Free Software Foundation
gives permission to link the code of its release of Wget with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#include <assert.h>

#include <ZMap/zmapUrlUtils.hpp>
#include <ZMap/zmapUrlOptions.hpp>
#include <ZMap/zmapUrl.hpp>
#include <ZMap/zmapUtils.hpp>

#ifndef errno
extern int errno;
#endif

struct scheme_data
{
  const char *leading_string;
  int default_port;
  int enabled;
};

/* Supported schemes: */
static struct scheme_data supported_schemes[] =
{
  { "http://",  DEFAULT_HTTP_PORT,  1 },
#ifdef HAVE_SSL
  { "https://", DEFAULT_HTTPS_PORT, 1 },
#endif
  { "ftp://",   DEFAULT_FTP_PORT,   1 },

  { "acedb://", DEFAULT_ACEDB_PORT, 1 },

  { "file://",  DEFAULT_FILE_PORT,  1 },

  { "pipe://",  DEFAULT_PIPE_PORT,  1 },

  { "mysql://", DEFAULT_MYSQL_PORT, 1 },

  { "ensembl://", DEFAULT_ENSEMBL_PORT, 1 },

  { "trackhub://", DEFAULT_TRACKHUB_PORT, 1 },

  /* SCHEME_INVALID */
  { NULL,       -1,                 0 }
};

/* Forward declarations: */

static int path_simplify(char *) ;

/* Support for encoding and decoding of URL strings.  We determine
   whether a character is unsafe through static table lookup.  This
   code assumes ASCII character set and 8-bit chars.  */

enum {
  /* rfc1738 reserved chars, preserved from encoding.  */
  urlchr_reserved = 1,

  /* rfc1738 unsafe chars, plus some more.  */
  urlchr_unsafe   = 2
};

#define urlchr_test(c, mask) (urlchr_table[(unsigned char)(c)] & (mask))
#define URL_RESERVED_CHAR(c) urlchr_test(c, urlchr_reserved)
#define URL_UNSAFE_CHAR(c) urlchr_test(c, urlchr_unsafe)

/* Shorthands for the table: */
#define R  urlchr_reserved
#define U  urlchr_unsafe
#define RU R|U

static const unsigned char urlchr_table[256] =
{
  U,  U,  U,  U,   U,  U,  U,  U,   /* NUL SOH STX ETX  EOT ENQ ACK BEL */
  U,  U,  U,  U,   U,  U,  U,  U,   /* BS  HT  LF  VT   FF  CR  SO  SI  */
  U,  U,  U,  U,   U,  U,  U,  U,   /* DLE DC1 DC2 DC3  DC4 NAK SYN ETB */
  U,  U,  U,  U,   U,  U,  U,  U,   /* CAN EM  SUB ESC  FS  GS  RS  US  */
  U,  0,  U, RU,   0,  U,  R,  0,   /* SP  !   "   #    $   %   &   '   */
  0,  0,  0,  R,   0,  0,  0,  R,   /* (   )   *   +    ,   -   .   /   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* 0   1   2   3    4   5   6   7   */
  0,  0, RU,  R,   U,  R,  U,  R,   /* 8   9   :   ;    <   =   >   ?   */
 RU,  0,  0,  0,   0,  0,  0,  0,   /* @   A   B   C    D   E   F   G   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* H   I   J   K    L   M   N   O   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* P   Q   R   S    T   U   V   W   */
  0,  0,  0, RU,   U, RU,  U,  0,   /* X   Y   Z   [    \   ]   ^   _   */
  U,  0,  0,  0,   0,  0,  0,  0,   /* `   a   b   c    d   e   f   g   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* h   i   j   k    l   m   n   o   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* p   q   r   s    t   u   v   w   */
  0,  0,  0,  U,   U,  U,  U,  U,   /* x   y   z   {    |   }   ~   DEL */

  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,

  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
};
#undef R
#undef U
#undef RU

static char* protocol_from_scheme(ZMapURLScheme scheme);

static options opt_G = {0};

/* URL-unescape the string S.

   This is done by transforming the sequences "%HH" to the character
   represented by the hexadecimal digits HH.  If % is not followed by
   two hexadecimal digits, it is inserted literally.

   The transformation is done in place.  If you need the original
   string intact, make a copy before calling this function.  */

static void
url_unescape (char *s)
{
  char *t = s;/* t - tortoise */
  char *h = s;/* h - hare     */

  for (; *h; h++, t++)
    {
      if (*h != '%')
        {
copychar:
          *t = *h;
        }
      else
        {
          /* Do nothing if '%' is not followed by two hex digits. */
          if (!h[1] || !h[2] || !(ISXDIGIT (h[1]) && ISXDIGIT (h[2])))
            goto copychar;
          *t = X2DIGITS_TO_NUM (h[1], h[2]);
          h += 2;
        }
    }
  *t = '\0';
}

/* The core of url_escape_* functions.  Escapes the characters that
   match the provided mask in urlchr_table.

   If ALLOW_PASSTHROUGH is non-zero, a string with no unsafe chars
   will be returned unchanged.  If ALLOW_PASSTHROUGH is zero, a
   freshly allocated string will be returned in all cases.  */

static char *
url_escape_1 (const char *s, unsigned char mask, int allow_passthrough)
{
  const char *p1;
  char *p2, *newstr;
  int newlen;
  int addition = 0;

  for (p1 = s; *p1; p1++)
    if (urlchr_test (*p1, mask))
      addition += 2;/* Two more characters (hex digits) */

  if (!addition)
    return allow_passthrough ? (char *)s : xstrdup (s);

  newlen = (p1 - s) + addition;
  newstr = (char *)xmalloc (newlen + 1);

  p1 = s;
  p2 = newstr;
  while (*p1)
    {
      /* Quote the characters that match the test mask. */
      if (urlchr_test (*p1, mask))
        {
          unsigned char c = *p1++;
          *p2++ = '%';
          *p2++ = XNUM_TO_DIGIT (c >> 4);
          *p2++ = XNUM_TO_DIGIT (c & 0xf);
        }
      else
        *p2++ = *p1++;
    }
  assert (p2 - newstr == newlen);
  *p2 = '\0';

  return newstr;
}

/* URL-escape the unsafe characters (see urlchr_table) in a given
   string, returning a freshly allocated string.  */

char *
url_escape (const char *s)
{
  return url_escape_1 (s, urlchr_unsafe, 0);
}

/* URL-escape the unsafe characters (see urlchr_table) in a given
   string.  If no characters are unsafe, S is returned.  */

static char *
url_escape_allow_passthrough (const char *s)
{
  return url_escape_1 (s, urlchr_unsafe, 1);
}


enum copy_method { CM_DECODE, CM_ENCODE, CM_PASSTHROUGH };

/* Decide whether to encode, decode, or pass through the char at P.
   This used to be a macro, but it got a little too convoluted.  */
static inline enum copy_method
decide_copy_method (const char *p)
{
  if (*p == '%')
    {
      if (ISXDIGIT (*(p + 1)) && ISXDIGIT (*(p + 2)))
        {
          /* %xx sequence: decode it, unless it would decode to an
             unsafe or a reserved char; in that case, leave it as
             is. */
          char preempt = X2DIGITS_TO_NUM (*(p + 1), *(p + 2));
          if (URL_UNSAFE_CHAR (preempt) || URL_RESERVED_CHAR (preempt))
            return CM_PASSTHROUGH;
          else
            return CM_DECODE;
        }
      else
/* Garbled %.. sequence: encode `%'. */
        return CM_ENCODE;
    }
  else if (URL_UNSAFE_CHAR (*p) && !URL_RESERVED_CHAR (*p))
    return CM_ENCODE;
  else
    return CM_PASSTHROUGH;
}

/* Translate a %-escaped (but possibly non-conformant) input string S
   into a %-escaped (and conformant) output string.  If no characters
   are encoded or decoded, return a duplicate of the same string S; i.e 
   always returns a freshly allocated string which must be free'd by the
   caller with xfree.

   After a URL has been run through this function, the protocols that
   use `%' as the quote character can use the resulting string as-is,
   while those that don't call url_unescape() to get to the intended
   data.  This function is also stable: after an input string is
   transformed the first time, all further transformations of the
   result yield the same result string.

   Let's discuss why this function is needed.

   Imagine Wget is to retrieve `http://abc.xyz/abc def'.  Since a raw
   space character would mess up the HTTP request, it needs to be
   quoted, like this:

       GET /abc%20def HTTP/1.0

   It appears that the unsafe chars need to be quoted, for example
   with url_escape.  But what if we're requested to download
   `abc%20def'?  url_escape transforms "%" to "%25", which would leave
   us with `abc%2520def'.  This is incorrect -- since %-escapes are
   part of URL syntax, "%20" is the correct way to denote a literal
   space on the Wget command line.  This leaves us in the conclusion
   that in that case Wget should not call url_escape, but leave the
   `%20' as is.

   And what if the requested URI is `abc%20 def'?  If we call
   url_escape, we end up with `/abc%2520%20def', which is almost
   certainly not intended.  If we don't call url_escape, we are left
   with the embedded space and cannot complete the request.  What the
   user meant was for Wget to request `/abc%20%20def', and this is
   where reencode_escapes kicks in.

   Wget used to solve this by first decoding %-quotes, and then
   encoding all the "unsafe" characters found in the resulting string.
   This was wrong because it didn't preserve certain URL special
   (reserved) characters.  For instance, URI containing "a%2B+b" (0x2b
   == '+') would get translated to "a%2B%2Bb" or "a++b" depending on
   whether we considered `+' reserved (it is).  One of these results
   is inevitable because by the second step we would lose information
   on whether the `+' was originally encoded or not.  Both results
   were wrong because in CGI parameters + means space, while %2B means
   literal plus.  reencode_escapes correctly translates the above to
   "a%2B+b", i.e. returns the original string.

   This function uses an algorithm proposed by Anon Sricharoenchai:

   1. Encode all URL_UNSAFE and the "%" that are not followed by 2
      hexdigits.

   2. Decode all "%XX" except URL_UNSAFE, URL_RESERVED (";/?:@=&") and
      "+".

   ...except that this code conflates the two steps, and decides
   whether to encode, decode, or pass through each character in turn.
   The function still uses two passes, but their logic is the same --
   the first pass exists merely for the sake of allocation.  Another
   small difference is that we include `+' to URL_RESERVED.

   Anon's test case:

   "http://abc.xyz/%20%3F%%36%31%25aa% a?a=%61+a%2Ba&b=b%26c%3Dc"
   ->
   "http://abc.xyz/%20%3F%2561%25aa%25%20a?a=a+a%2Ba&b=b%26c%3Dc"

   Simpler test cases:

   "foo bar"         -> "foo%20bar"
   "foo%20bar"       -> "foo%20bar"
   "foo %20bar"      -> "foo%20%20bar"
   "foo%%20bar"      -> "foo%25%20bar"       (0x25 == '%')
   "foo%25%20bar"    -> "foo%25%20bar"
   "foo%2%20bar"     -> "foo%252%20bar"
   "foo+bar"         -> "foo+bar"            (plus is reserved!)
   "foo%2b+bar"      -> "foo%2b+bar"  */

static char *
reencode_escapes (const char *s)
{
  const char *p1 = NULL;
  char *newstr = NULL, *p2 = NULL;
  int oldlen, newlen;

  int encode_count = 0;
  int decode_count = 0;

  /* First, pass through the string to see if there's anything to do,
     and to calculate the new length.  */
  for (p1 = s; *p1; p1++)
    {
      switch (decide_copy_method (p1))
        {
        case CM_ENCODE:
          ++encode_count;
          break;
        case CM_DECODE:
          ++decode_count;
          break;
        case CM_PASSTHROUGH:
          break;
        }
    }

  if (!encode_count && !decode_count)
    {
      /* The string is good as it is. */
      newstr = xstrdup(s);
    }
  else
    {
      oldlen = p1 - s;
      /* Each encoding adds two characters (hex digits), while each
         decoding removes two characters.  */
      newlen = oldlen + 2 * (encode_count - decode_count);
      newstr = (char *)xmalloc(newlen + 1) ;

      p1 = s;
      p2 = newstr;

      while (*p1)
        {
          switch (decide_copy_method (p1))
            {
            case CM_ENCODE:
              {
                unsigned char c = *p1++;
                *p2++ = '%';
                *p2++ = XNUM_TO_DIGIT (c >> 4);
                *p2++ = XNUM_TO_DIGIT (c & 0xf);
              }
              break;
            case CM_DECODE:
              *p2++ = X2DIGITS_TO_NUM (p1[1], p1[2]);
              p1 += 3;/* skip %xx */
              break;
            case CM_PASSTHROUGH:
              *p2++ = *p1++;
            }
        }
      *p2 = '\0';
      assert (p2 - newstr == newlen);
    }

  return newstr;
}

/* Returns the scheme type if the scheme is supported, or
   SCHEME_INVALID if not.  */

ZMapURLScheme
url_scheme (const char *url)
{
  int i;

  for (i = 0; supported_schemes[i].leading_string; i++)
    if (0 == strncasecmp (url, supported_schemes[i].leading_string,
  strlen (supported_schemes[i].leading_string)))
      {
        if (supported_schemes[i].enabled)
          return (ZMapURLScheme) i;
        else
          return SCHEME_INVALID;
      }

  return SCHEME_INVALID;
}

/* Perform tilde expansion of filenames (only applicable for filename url schemes - 
 * for other schemes just returns a copy of url).
 * The result is a newly-allocated string which must be free'd with g_free. */
char *
url_tilde_expansion (const char *url, ZMapURLScheme scheme)
{
  char *result = NULL ;

  if (url && scheme == SCHEME_FILE)
    {
      /* Remove the scheme prefix */
      const char *leading_string = supported_schemes[scheme].leading_string;
      char *p = (char *)url + strlen (leading_string);

      /* Perform tilde expansion */
      char *tmp = zMapExpandFilePath(p) ;

      if (tmp)
        {
          /* Re-add scheme prefix */
          result = g_strdup_printf("%s%s", leading_string, tmp) ;
          g_free(tmp);
        }
      else
        {
          result = g_strdup(url);
        }
    }
  else if (url)
    {
      result = g_strdup(url);
    }

  return result ;
}

#define SCHEME_CHAR(ch) (ISALNUM (ch) || (ch) == '-' || (ch) == '+')

/* Return 1 if the URL begins with any "scheme", 0 otherwise.  As
   currently implemented, it returns true if URL begins with
   [-+a-zA-Z0-9]+: .  */

int
url_has_scheme (const char *url)
{
  const char *p = url;

  /* The first char must be a scheme char. */
  if (!*p || !SCHEME_CHAR (*p))
    return 0;
  ++p;
  /* Followed by 0 or more scheme chars. */
  while (*p && SCHEME_CHAR (*p))
    ++p;
  /* Terminated by ':'. */
  return *p == ':';
}

int
scheme_default_port (ZMapURLScheme scheme)
{
  return supported_schemes[scheme].default_port;
}

void
scheme_disable (ZMapURLScheme scheme)
{
  supported_schemes[scheme].enabled = 0;
  return ;
}

/* Skip the username and password, if present here.  The function
   should *not* be called with the complete URL, but with the part
   right after the scheme.

   If no username and password are found, return 0.  */

static int
url_skip_credentials (const char *url)
{
  /* Look for '@' that comes before terminators, such as '/', '?',
     '#', or ';'.  */
  const char *p = (const char *)strpbrk (url, "@/?#;");
  if (!p || *p != '@')
    return 0;
  return p + 1 - url;
}

/* Parse credentials contained in [BEG, END).  The region is expected
   to have come from a URL and is unescaped.  */

static int
parse_credentials (const char *beg, const char *end, char **user, char **passwd)
{
  char *colon;
  const char *userend;

  if (beg == end)
    return 0;/* empty user name */

  colon = (char *)memchr(beg, ':', end - beg);
  if (colon == beg)
    return 0;/* again empty user name */

  if (colon)
    {
      *passwd = strdupdelim (colon + 1, end);
      userend = colon;
      url_unescape (*passwd);
    }
  else
    {
      *passwd = NULL;
      userend = end;
    }
  *user = strdupdelim (beg, userend);
  url_unescape (*user);
  return 1;
}

/* Used by main.c: detect URLs written using the "shorthand" URL forms
   popularized by Netscape and NcFTP.  HTTP shorthands look like this:

   www.foo.com[:port]/dir/file   -> http://www.foo.com[:port]/dir/file
   www.foo.com[:port]            -> http://www.foo.com[:port]

   FTP shorthands look like this:

   foo.bar.com:dir/file          -> ftp://foo.bar.com/dir/file
   foo.bar.com:/absdir/file      -> ftp://foo.bar.com//absdir/file

   If the URL needs not or cannot be rewritten, return NULL.  */

char *
rewrite_shorthand_url (const char *url)
{
  const char *p;

  if (url_has_scheme (url))
    return NULL;

  /* Look for a ':' or '/'.  The former signifies NcFTP syntax, the
     latter Netscape.  */
  for (p = url; *p && *p != ':' && *p != '/'; p++)
    ;

  if (p == url)
    return NULL;

  if (*p == ':')
    {
      const char *pp;
      char *res;
      /* If the characters after the colon and before the next slash
 or end of string are all digits, it's HTTP.  */
      int digits = 0;
      for (pp = p + 1; ISDIGIT (*pp); pp++)
        ++digits;
      if (digits > 0 && (*pp == '/' || *pp == '\0'))
        goto http;

      /* Prepend "ftp://" to the entire URL... */
      res = (char *)xmalloc (6 + strlen (url) + 1);
      sprintf (res, "ftp://%s", url);
      /* ...and replace ':' with '/'. */
      res[6 + (p - url)] = '/';
      return res;
    }
  else
    {
      char *res;
    http:
      /* Just prepend "http://" to what we have. */
      res = (char *)xmalloc (7 + strlen (url) + 1);
      sprintf (res, "http://%s", url);
      return res;
    }
}

static void split_path(const char *, char **, char **) ;

/* Like strpbrk, with the exception that it returns the pointer to the
   terminating zero (end-of-string aka "eos") if no matching character
   is found.

   Although I normally balk at Gcc-specific optimizations, it probably
   makes sense here: glibc has optimizations that detect strpbrk being
   called with literal string as ACCEPT and inline the search.  That
   optimization is defeated if strpbrk is hidden within the call to
   another function.  (And no, making strpbrk_or_eos inline doesn't
   help because the check for literal accept is in the
   preprocessor.)  */

#ifdef __GNUC__

#define strpbrk_or_eos(s, accept) ({\
  char *SOE_p = strpbrk (s, accept);\
  if (!SOE_p)\
    SOE_p = (char *)s + strlen (s);\
  SOE_p;\
})

#else  /* not __GNUC__ */

static char *
strpbrk_or_eos (const char *s, const char *accept)
{
  char *p = strpbrk (s, accept);
  if (!p)
    p = (char *)s + strlen (s);
  return p;
}
#endif

/* Turn STR into lowercase; return non-zero if a character was
   actually changed. */

static int
lowercase_str (char *str)
{
  int change = 0;
  for (; *str; str++)
    if (ISUPPER (*str))
      {
        change = 1;
        *str = TOLOWER (*str);
      }
  return change;
}

static const char *parse_errors[] = {
#define PE_NO_ERROR 0
  N_("No error"),
#define PE_UNSUPPORTED_SCHEME 1
  N_("Unsupported scheme"),
#define PE_EMPTY_HOST 2
  N_("Empty host"),
#define PE_BAD_PORT_NUMBER 3
  N_("Bad port number"),
#define PE_INVALID_USER_NAME 4
  N_("Invalid user name"),
#define PE_UNTERMINATED_IPV6_ADDRESS 5
  N_("Unterminated IPv6 numeric address"),
#define PE_IPV6_NOT_SUPPORTED 6
  N_("IPv6 addresses not supported"),
#define PE_INVALID_IPV6_ADDRESS 7
  N_("Invalid IPv6 numeric address")
};

#ifdef ENABLE_IPV6
/* The following two functions were adapted from glibc. */

static int
is_valid_ipv4_address (const char *str, const char *end)
{
  int saw_digit, octets;
  int val;

  saw_digit = 0;
  octets = 0;
  val = 0;

  while (str < end) {
    int ch = *str++;

    if (ch >= '0' && ch <= '9') {
      val = val * 10 + (ch - '0');

      if (val > 255)
        return 0;
      if (saw_digit == 0) {
        if (++octets > 4)
          return 0;
        saw_digit = 1;
      }
    } else if (ch == '.' && saw_digit == 1) {
      if (octets == 4)
        return 0;
      val = 0;
      saw_digit = 0;
    } else
      return 0;
  }
  if (octets < 4)
    return 0;

  return 1;
}

static const int NS_INADDRSZ  = 4;
static const int NS_IN6ADDRSZ = 16;
static const int NS_INT16SZ   = 2;

static int
is_valid_ipv6_address (const char *str, const char *end)
{
  static const char xdigits[] = "0123456789abcdef";
  const char *curtok;
  int tp;
  const char *colonp;
  int saw_xdigit;
  unsigned int val;

  tp = 0;
  colonp = NULL;

  if (str == end)
    return 0;

  /* Leading :: requires some special handling. */
  if (*str == ':')
    {
      ++str;
      if (str == end || *str != ':')
        return 0;
    }

  curtok = str;
  saw_xdigit = 0;
  val = 0;

  while (str < end) {
    int ch = *str++;
    const char *pch;

    /* if ch is a number, add it to val. */
    pch = strchr(xdigits, ch);
    if (pch != NULL) {
      val <<= 4;
      val |= (pch - xdigits);
      if (val > 0xffff)
        return 0;
      saw_xdigit = 1;
      continue;
    }

    /* if ch is a colon ... */
    if (ch == ':') {
      curtok = str;
      if (saw_xdigit == 0) {
        if (colonp != NULL)
          return 0;
        colonp = str + tp;
        continue;
      } else if (str == end) {
        return 0;
      }
      if (tp > NS_IN6ADDRSZ - NS_INT16SZ)
        return 0;
      tp += NS_INT16SZ;
      saw_xdigit = 0;
      val = 0;
      continue;
    }

    /* if ch is a dot ... */
    if (ch == '.' && (tp <= NS_IN6ADDRSZ - NS_INADDRSZ) &&
is_valid_ipv4_address(curtok, end) == 1) {
      tp += NS_INADDRSZ;
      saw_xdigit = 0;
      break;
    }

    return 0;
  }

  if (saw_xdigit == 1) {
    if (tp > NS_IN6ADDRSZ - NS_INT16SZ)
      return 0;
    tp += NS_INT16SZ;
  }

  if (colonp != NULL) {
    if (tp == NS_IN6ADDRSZ)
      return 0;
    tp = NS_IN6ADDRSZ;
  }

  if (tp != NS_IN6ADDRSZ)
    return 0;

  return 1;
}
#endif

/* Parse a URL.
 *******************************************************************************
 * Return a new ZMapURL if successful, NULL on error.  In case of
 * error, and if ERROR is not NULL, also set *ERROR to the appropriate
 * error code.
 *
 * This follows the following RFC ...
 * http://rfc.net/rfc1738.html
 *
 * Generally
 * <scheme>://[user][:password]@<host>[:port]/[url-path][;typecode][?query][#fragment]
 *
 * Some Notes:
 * 1) [] square brackets are optional, <> angle brackets compulsory, with the
 *    exception of host when scheme == file.
 * 2) The original source of this code is wget, which did nothing to support
 *    the file scheme.  According to the rfc above and in order to support both
 *    relative and absolute url-paths for the file scheme the leading / is NOT
 *    part of the url-path and IS a separator.
 *    e.g. file:///tmp/file.tmp            is relative
 *         file:////tmp/file.tmp           is absolute
 *         file://localhost/tmp/file.tmp   is relative
 *         file://localhost//tmp/file.tmp  is absolute
 * 3) this documentation should be elsewhere.
 *******************************************************************************
 */

ZMapURL url_parse (const char *url, int *error)
{
  ZMapURL u;
  char *p;
  int path_modified, host_modified;

  ZMapURLScheme scheme;

  const char *uname_b,     *uname_e;
  const char *host_b,      *host_e;
  const char *path_b,      *path_e;
  const char *params_b,    *params_e;
  const char *query_b,     *query_e;
  const char *fragment_b,  *fragment_e;

  int port = 0;
  char *user = NULL, *passwd = NULL;

  //char *url_expanded = NULL;
  char *url_encoded = NULL;

  int error_code;

  scheme = url_scheme (url);
  if (scheme == SCHEME_INVALID)
    {
      error_code = PE_UNSUPPORTED_SCHEME;
      goto error;
    }

  /* for file paths, perform tilde expansion */
  /* gb10: comment out for now as this doesn't work properly because we
   * get called with a url which has already had the ~ escaped. */
  //url_expanded = url_tilde_expansion (url, scheme);

  /* escape special characters */
  url_encoded = reencode_escapes (url);

  p = url_encoded;

  p += strlen (supported_schemes[scheme].leading_string);
  uname_b = p;
  p += url_skip_credentials (p);
  uname_e = p;

  /* scheme://user:pass@host[:port]... */
  /*                    ^              */

  /* We attempt to break down the URL into the components path,
     params, query, and fragment.  They are ordered like this:

       scheme://host[:port][/path][;params][?query][#fragment]  */

  params_b   = params_e   = NULL;
  query_b    = query_e    = NULL;
  fragment_b = fragment_e = NULL;

  host_b = p;

  if (*p == '[')
    {
      /* Handle IPv6 address inside square brackets.  Ideally we'd
 just look for the terminating ']', but rfc2732 mandates
 rejecting invalid IPv6 addresses.  */

      /* The address begins after '['. */
      host_b = p + 1;
      host_e = strchr (host_b, ']');

      if (!host_e)
        {
          error_code = PE_UNTERMINATED_IPV6_ADDRESS;
          goto error;
        }

#ifdef ENABLE_IPV6
      /* Check if the IPv6 address is valid. */
      if (!is_valid_ipv6_address(host_b, host_e))
        {
          error_code = PE_INVALID_IPV6_ADDRESS;
          goto error;
        }

      /* Continue parsing after the closing ']'. */
      p = host_e + 1;
#else
      error_code = PE_IPV6_NOT_SUPPORTED;
      goto error;
#endif
    }
  else
    {
      p = strpbrk_or_eos (p, ":/;?#");
      host_e = p;
    }



  /* ROY HERE IS THE CODE I THINK MAY NOT BE WORKING.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (host_b == host_e)
    {
      error_code = PE_EMPTY_HOST;
      goto error;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* Check that the host is provided (unless not required, e.g. for file) */
  if ((scheme != SCHEME_FILE) && (scheme != SCHEME_PIPE) && (scheme != SCHEME_TRACKHUB) &&
      host_b == host_e)
    {
      error_code = PE_EMPTY_HOST;
      goto error;
    }





  port = scheme_default_port (scheme);
  if (*p == ':')
    {
      const char *port_b, *port_e, *pp;

      /* scheme://host:port/tralala */
      /*              ^             */
      ++p;
      port_b = p;
      p = strpbrk_or_eos (p, "/;?#");
      port_e = p;

      if (port_b == port_e)
        {
          /* http://host:/whatever */
          /*             ^         */
          error_code = PE_BAD_PORT_NUMBER;
          goto error;
        }

      for (port = 0, pp = port_b; pp < port_e; pp++)
        {
          if (!ISDIGIT (*pp))
            {
              /* http://host:12randomgarbage/blah */
              /*               ^                  */
                      error_code = PE_BAD_PORT_NUMBER;
                      goto error;
            }

          port = 10 * port + (*pp - '0');
        }
    }

  if (*p == '/')
    {
      ++p;
      path_b = p;
      p = strpbrk_or_eos (p, ";?#");
      path_e = p;
    }
  else
    {
      /* Path is not allowed not to exist. */
      path_b = path_e = p;
    }

  if (*p == ';')
    {
      ++p;
      params_b = p;
      p = strpbrk_or_eos (p, "?#");
      params_e = p;
    }
  if (*p == '?')
    {
      ++p;
      query_b = p;
      p = strpbrk_or_eos (p, "#");
      query_e = p;

      /* Hack that allows users to use '?' (a wildcard character) in
 FTP URLs without it being interpreted as a query string
 delimiter.  */
      if (scheme == SCHEME_FTP)
        {
          query_b = query_e = NULL;
          path_e = p;
        }
    }
  if (*p == '#')
    {
      ++p;
      fragment_b = p;
      p += strlen (p);
      fragment_e = p;
    }
  assert (*p == 0);

  if (uname_b != uname_e)
    {
      /* http://user:pass@host */
      /*        ^         ^    */
      /*     uname_b   uname_e */
      if (!parse_credentials (uname_b, uname_e - 1, &user, &passwd))
        {
          error_code = PE_INVALID_USER_NAME;
          goto error;
        }
    }

  u = (ZMapURL)xmalloc(sizeof(ZMapURLStruct));
  memset (u, 0, sizeof (*u));

  u->scheme = scheme;
  u->protocol = protocol_from_scheme(scheme);
  u->host   = strdupdelim (host_b, host_e);
  u->port   = port;
  u->user   = user;
  u->passwd = passwd;

  u->path = strdupdelim (path_b, path_e);
  path_modified = path_simplify (u->path);
  split_path (u->path, &u->dir, &u->file);

  host_modified = lowercase_str (u->host);

  if (params_b)
    u->params = strdupdelim (params_b, params_e);
  if (query_b)
    u->query = strdupdelim (query_b, query_e);
  if (fragment_b)
    u->fragment = strdupdelim (fragment_b, fragment_e);

  if (path_modified || u->fragment || host_modified || path_b == path_e)
    {
      /* If we suspect that a transformation has rendered what
         url_string might return different from URL_ENCODED, rebuild
         u->url using url_string.  */
      u->url = url_string (u, 0);
      
      xfree ((char *) url_encoded);
    }
  else
    {
      u->url = url_encoded;
    }
  url_encoded = NULL;

  //if (url_expanded)
  //  g_free(url_expanded);

  return u;

 error:
  /* Cleanup in case of error (note that we must use the correct free function): */
  if (url_encoded)
    xfree (url_encoded);

  //if (url_expanded)
  //  g_free (url_expanded);

  /* Transmit the error code to the caller, if the caller wants to
     know.  */
  if (error)
    *error = error_code;
  return NULL;
}

/* Return the error message string from ERROR_CODE, which should have
   been retrieved from url_parse.  The error message is translated.  */

const char *url_error (guint error_code)
{
  assert (error_code >= 0 && error_code < countof (parse_errors));

  return _(parse_errors[error_code]);
}

/* Split PATH into DIR and FILE.  PATH comes from the URL and is
   expected to be URL-escaped.

   The path is split into directory (the part up to the last slash)
   and file (the part after the last slash), which are subsequently
   unescaped.  Examples:

   PATH                 DIR           FILE
   "foo/bar/baz"        "foo/bar"     "baz"
   "foo/bar/"           "foo/bar"     ""
   "foo"                ""            "foo"
   "foo/bar/baz%2fqux"  "foo/bar"     "baz/qux" (!)

   DIR and FILE are freshly allocated.  */

static void
split_path (const char *path, char **dir, char **file)
{
  char *last_slash = strrchr ((char *)path, '/');

  if (!last_slash)
    {
      *dir = xstrdup ("");
      *file = xstrdup (path);
    }
  else
    {
      *dir = strdupdelim (path, last_slash);
      *file = xstrdup (last_slash + 1);
    }
  url_unescape (*dir);
  url_unescape (*file);
}

/* Note: URL's "full path" is the path with the query string and
   params appended.  The "fragment" (#foo) is intentionally ignored,
   but that might be changed.  For example, if the original URL was
   "http://host:port/foo/bar/baz;bullshit?querystring#uselessfragment",
   the full path will be "/foo/bar/baz;bullshit?querystring".  */

/* Return the length of the full path, without the terminating
   zero.  */

static int
full_path_length (const ZMapURL url)
{
  int len = 0;

#define FROB(el) if (url->el) len += 1 + strlen (url->el)

  FROB (path);
  FROB (params);
  FROB (query);

#undef FROB

  return len;
}

/* Write out the full path. */

static void
full_path_write (const ZMapURL url, char *where)
{
#define FROB(el, chr) do {\
  char *f_el = url->el;\
  if (f_el) {\
    int l = strlen (f_el);\
    *where++ = chr;\
    memcpy (where, f_el, l);\
    where += l;\
  }\
} while (0)

  FROB (path, '/');
  FROB (params, ';');
  FROB (query, '?');

#undef FROB
}

/* Public function for getting the "full path".  E.g. if u->path is
   "foo/bar" and u->query is "param=value", full_path will be
   "/foo/bar?param=value". */

char *
url_full_path (const ZMapURL url)
{
  int length = full_path_length (url);
  char *full_path = (char *)xmalloc(length + 1);

  full_path_write (url, full_path);
  full_path[length] = '\0';

  return full_path;
}

/* Escape unsafe and reserved characters, except for the slash
   characters.  */

static char *
url_escape_dir (const char *dir)
{
  char *newdir = url_escape_1 (dir, urlchr_unsafe | urlchr_reserved, 1);
  char *h, *t;
  if (newdir == dir)
    return (char *)dir;

  /* Unescape slashes in NEWDIR. */

  h = newdir;/* hare */
  t = newdir;/* tortoise */

  for (; *h; h++, t++)
    {
      /* url_escape_1 having converted '/' to "%2F" exactly. */
      if (*h == '%' && h[1] == '2' && h[2] == 'F')
        {
          *t = '/';
          h += 2;
        }
      else
        *t = *h;
    }
  *t = '\0';

  return newdir;
}

/* Sync u->path and u->url with u->dir and u->file.  Called after
   u->file or u->dir have been changed, typically by the FTP code.  */

static void
sync_path (ZMapURL u)
{
  char *newpath, *efile, *edir;

  xfree (u->path);

  /* u->dir and u->file are not escaped.  URL-escape them before
     reassembling them into u->path.  That way, if they contain
     separators like '?' or even if u->file contains slashes, the
     path will be correctly assembled.  (u->file can contain slashes
     if the URL specifies it with %2f, or if an FTP server returns
     it.)  */
  edir = url_escape_dir (u->dir);
  efile = url_escape_1 (u->file, urlchr_unsafe | urlchr_reserved, 1);

  if (!*edir)
    newpath = xstrdup (efile);
  else
    {
      int dirlen = strlen (edir);
      int filelen = strlen (efile);

      /* Copy "DIR/FILE" to newpath. */
      char *p = newpath = (char *)xmalloc (dirlen + 1 + filelen + 1);
      memcpy (p, edir, dirlen);
      p += dirlen;
      *p++ = '/';
      memcpy (p, efile, filelen);
      p += filelen;
      *p++ = '\0';
    }

  u->path = newpath;

  if (edir != u->dir)
    xfree (edir);
  if (efile != u->file)
    xfree (efile);

  /* Regenerate u->url as well.  */
  xfree (u->url);
  u->url = url_string (u, 0);
}

/* Mutators.  Code in ftp.c insists on changing u->dir and u->file.
   This way we can sync u->path and u->url when they get changed.  */

void
url_set_dir (ZMapURL url, const char *newdir)
{
  xfree (url->dir);
  url->dir = xstrdup (newdir);
  sync_path (url);
}

void
url_set_file (ZMapURL url, const char *newfile)
{
  xfree (url->file);
  url->file = xstrdup (newfile);
  sync_path (url);
}

void
url_free (ZMapURL url)
{
  xfree (url->host);
  xfree (url->path);
  xfree (url->url);

  FREE_MAYBE (url->params);
  FREE_MAYBE ((char *)(url->query));
  FREE_MAYBE (url->fragment);
  FREE_MAYBE (url->user);
  FREE_MAYBE (url->passwd);

  xfree (url->dir);
  xfree (url->file);

  xfree (url);
}


/* Functions for constructing the file name out of URL components.  */

/* A growable string structure, used by url_file_name and friends.
   This should perhaps be moved to utils.c.

   The idea is to have a convenient and efficient way to construct a
   string by having various functions append data to it.  Instead of
   passing the obligatory BASEVAR, SIZEVAR and TAILPOS to all the
   functions in questions, we pass the pointer to this struct.  */

struct growable {
  char *base;
  int size;
  int tail;
};

/* Ensure that the string can accept APPEND_COUNT more characters past
   the current TAIL position.  If necessary, this will grow the string
   and update its allocated size.  If the string is already large
   enough to take TAIL+APPEND_COUNT characters, this does nothing.  */
#define GROW(g, append_size) do {\
  struct growable *G_ = g;\
  DO_REALLOC (G_->base, G_->size, G_->tail + append_size, char);\
} while (0)

/* Return the tail position of the string. */
#define TAIL(r) ((r)->base + (r)->tail)

/* Move the tail position by APPEND_COUNT characters. */
#define TAIL_INCR(r, append_count) ((r)->tail += append_count)

/* Append the string STR to DEST.  NOTICE: the string in DEST is not
   terminated.  */

static void
append_string (const char *str, struct growable *dest)
{
  int l = strlen (str);
  GROW (dest, l);
  memcpy (TAIL (dest), str, l);
  TAIL_INCR (dest, l);
}

/* Append CH to DEST.  For example, append_char (0, DEST)
   zero-terminates DEST.  */

static void
append_char (char ch, struct growable *dest)
{
  GROW (dest, 1);
  *TAIL (dest) = ch;
  TAIL_INCR (dest, 1);
}

enum {
  filechr_not_unix    = 1,/* unusable on Unix, / and \0 */
  filechr_not_windows = 2,/* unusable on Windows, one of \|/<>?:*" */
  filechr_control     = 4/* a control character, e.g. 0-31 */
};

#define FILE_CHAR_TEST(c, mask) (filechr_table[(unsigned char)(c)] & (mask))

/* Shorthands for the table: */
#define U filechr_not_unix
#define W filechr_not_windows
#define C filechr_control

#define UW U|W
#define UWC U|W|C

/* Table of characters unsafe under various conditions (see above).

   Arguably we could also claim `%' to be unsafe, since we use it as
   the escape character.  If we ever want to be able to reliably
   translate file name back to URL, this would become important
   crucial.  Right now, it's better to be minimal in escaping.  */

static const unsigned char filechr_table[256] =
{
UWC,  C,  C,  C,   C,  C,  C,  C,   /* NUL SOH STX ETX  EOT ENQ ACK BEL */
  C,  C,  C,  C,   C,  C,  C,  C,   /* BS  HT  LF  VT   FF  CR  SO  SI  */
  C,  C,  C,  C,   C,  C,  C,  C,   /* DLE DC1 DC2 DC3  DC4 NAK SYN ETB */
  C,  C,  C,  C,   C,  C,  C,  C,   /* CAN EM  SUB ESC  FS  GS  RS  US  */
  0,  0,  W,  0,   0,  0,  0,  0,   /* SP  !   "   #    $   %   &   '   */
  0,  0,  W,  0,   0,  0,  0, UW,   /* (   )   *   +    ,   -   .   /   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* 0   1   2   3    4   5   6   7   */
  0,  0,  W,  0,   W,  0,  W,  W,   /* 8   9   :   ;    <   =   >   ?   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* @   A   B   C    D   E   F   G   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* H   I   J   K    L   M   N   O   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* P   Q   R   S    T   U   V   W   */
  0,  0,  0,  0,   W,  0,  0,  0,   /* X   Y   Z   [    \   ]   ^   _   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* `   a   b   c    d   e   f   g   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* h   i   j   k    l   m   n   o   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* p   q   r   s    t   u   v   w   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* x   y   z   {    |   }   ~   DEL */

  C, C, C, C,  C, C, C, C,  C, C, C, C,  C, C, C, C, /* 128-143 */
  C, C, C, C,  C, C, C, C,  C, C, C, C,  C, C, C, C, /* 144-159 */
  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,

  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
};
#undef U
#undef W
#undef C
#undef UW
#undef UWC

/* FN_PORT_SEP is the separator between host and port in file names
   for non-standard port numbers.  On Unix this is normally ':', as in
   "www.xemacs.org:4001/index.html".  Under Windows, we set it to +
   because Windows can't handle ':' in file names.  */
#define FN_PORT_SEP  (opt_G.restrict_files_os != options::restrict_windows ? ':' : '+')

/* FN_QUERY_SEP is the separator between the file name and the URL
   query, normally '?'.  Since Windows cannot handle '?' as part of
   file name, we use '@' instead there.  */
#define FN_QUERY_SEP (opt_G.restrict_files_os != options::restrict_windows ? '?' : '@')

/* Quote path element, characters in [b, e), as file name, and append
   the quoted string to DEST.  Each character is quoted as per
   file_unsafe_char and the corresponding table.

   If ESCAPED_P is non-zero, the path element is considered to be
   URL-escaped and will be unescaped prior to inspection.  */

static void
append_uri_pathel (const char *b, const char *e, int escaped_p,
   struct growable *dest)
{
  const char *p;
  int quoted, outlen;

  int mask;
  if (opt_G.restrict_files_os == options::restrict_unix)
    mask = filechr_not_unix;
  else
    mask = filechr_not_windows;
  if (opt_G.restrict_files_ctrl)
    mask |= filechr_control;

  /* Copy [b, e) to PATHEL and URL-unescape it. */
  if (escaped_p)
    {
      char *unescaped;

      BOUNDED_TO_ALLOCA (b, e, unescaped);
      url_unescape (unescaped);
      b = unescaped;
      e = unescaped + strlen (unescaped);
    }

  /* Walk the PATHEL string and check how many characters we'll need
     to add for file quoting.  */
  quoted = 0;
  for (p = b; p < e; p++)
    if (FILE_CHAR_TEST (*p, mask))
      ++quoted;

  /* e-b is the string length.  Each quoted char means two additional
     characters in the string, hence 2*quoted.  */
  outlen = (e - b) + (2 * quoted);
  GROW (dest, outlen);

  if (!quoted)
    {
      /* If there's nothing to quote, we don't need to go through the
 string the second time.  */
      memcpy (TAIL (dest), b, outlen);
    }
  else
    {
      char *q = TAIL (dest);
      for (p = b; p < e; p++)
        {
          if (!FILE_CHAR_TEST (*p, mask))
            *q++ = *p;
          else
            {
              unsigned char ch = *p;
              *q++ = '%';
              *q++ = XNUM_TO_DIGIT (ch >> 4);
              *q++ = XNUM_TO_DIGIT (ch & 0xf);
            }
        }
      assert (q - TAIL (dest) == outlen);
    }
  TAIL_INCR (dest, outlen);
}

/* Append to DEST the directory structure that corresponds the
   directory part of URL's path.  For example, if the URL is
   http://server/dir1/dir2/file, this appends "/dir1/dir2".

   Each path element ("dir1" and "dir2" in the above example) is
   examined, url-unescaped, and re-escaped as file name element.

   Additionally, it cuts as many directories from the path as
   specified by opt_G.cut_dirs.  For example, if opt_G.cut_dirs is 1, it
   will produce "bar" for the above example.  For 2 or more, it will
   produce "".

   Each component of the path is quoted for use as file name.  */

static void
append_dir_structure (const ZMapURL u, struct growable *dest)
{
  char *pathel, *next;
  int cut = opt_G.cut_dirs;

  /* Go through the path components, de-URL-quote them, and quote them
     (if necessary) as file names.  */

  pathel = u->path;
  for (; (next = strchr (pathel, '/')) != NULL; pathel = next + 1)
    {
      if (cut-- > 0)
        continue;
      if (pathel == next)
/* Ignore empty pathels.  */
continue;

      if (dest->tail)
        append_char ('/', dest);
      append_uri_pathel (pathel, next, 1, dest);
    }
}

/* Return a file name that matches the given URL as good as
   possible.  Does not create directories on the file system.  */

char *
url_file_name (const ZMapURL u)
{
  struct growable fnres;

  const char *u_file, *u_query;
  char *fname;

  fnres.base = NULL;
  fnres.size = 0;
  fnres.tail = 0;

  /* Start with the directory prefix, if specified. */
  if (opt_G.dir_prefix)
    append_string (opt_G.dir_prefix, &fnres);

  /* If "dirstruct" is turned on (typically the case with -r), add
     the host and port (unless those have been turned off) and
     directory structure.  */
  if (opt_G.dirstruct)
    {
      if (opt_G.add_hostdir)
        {
          if (fnres.tail)
            append_char ('/', &fnres);
          append_string (u->host, &fnres);
          if (u->port != scheme_default_port (u->scheme))
            {
              char portstr[24];
              number_to_string (portstr, u->port);
              append_char (FN_PORT_SEP, &fnres);
              append_string (portstr, &fnres);
            }
        }

      append_dir_structure (u, &fnres);
    }

  /* Add the file name. */
  if (fnres.tail)
    append_char ('/', &fnres);
  u_file = *u->file ? u->file : "index.html";
  append_uri_pathel (u_file, u_file + strlen (u_file), 0, &fnres);

  /* Append "?query" to the file name. */
  u_query = u->query && *u->query ? u->query : NULL;
  if (u_query)
    {
      append_char (FN_QUERY_SEP, &fnres);
      append_uri_pathel (u_query, u_query + strlen (u_query), 1, &fnres);
    }

  /* Zero-terminate the file name. */
  append_char ('\0', &fnres);

  fname = fnres.base;

  return fname;

}

/* Return the length of URL's path.  Path is considered to be
   terminated by one of '?', ';', '#', or by the end of the
   string.  */
static int
path_length (char *url)
{
  char *q = strpbrk_or_eos (url, "?;#");
  return q - url;
}

/* Find the last occurrence of character C in the range [b, e), or
   NULL, if none are present.  This is equivalent to strrchr(b, c),
   except that it accepts an END argument instead of requiring the
   string to be zero-terminated.  Why is there no memrchr()?  */
static const char *
find_last_char (const char *b, const char *e, char c)
{
  for (; e > b; e--)
    if (*e == c)
      return e;
  return NULL;
}

/* Resolve "." and ".." elements of PATH by destructively modifying
   PATH and return non-zero if PATH has been modified, zero otherwise.

   The algorithm is in spirit similar to the one described in rfc1808,
   although implemented differently, in one pass.  To recap, path
   elements containing only "." are removed, and ".." is taken to mean
   "back up one element".  Single leading and trailing slashes are
   preserved.

   This function does not handle URL escapes explicitly.  If you're
   passing paths from URLs, make sure to unquote "%2e" and "%2E" to
   ".", so that this function can find the dots.  (Wget's URL parser
   calls reencode_escapes, which see.)

   For example, "a/b/c/./../d/.." will yield "a/b/".  More exhaustive
   test examples are provided below.  If you change anything in this
   function, run test_path_simplify to make sure you haven't broken a
   test case.  */

static int
path_simplify (char *path)
{
  char *h, *t, *end;

  /* Preserve the leading '/'. */
  if (path[0] == '/')
    ++path;

  h = path;/* hare */
  t = path;/* tortoise */
  end = path + strlen (path);

  while (h < end)
    {
      /* Hare should be at the beginning of a path element. */

      if (h[0] == '.' && (h[1] == '/' || h[1] == '\0'))
        {
          /* Ignore "./". */
          h += 2;
        }
      else if (h[0] == '.' && h[1] == '.' && (h[2] == '/' || h[2] == '\0'))
        {
          /* Handle "../" by retreating the tortoise by one path
             element -- but not past beggining of PATH.  */
          if (t > path)
            {
              /* Move backwards until T hits the beginning of the
         previous path element or the beginning of path. */
              for (--t; t > path && t[-1] != '/'; t--)
                ;
            }
          h += 3;
        }
      else if (*h == '/')
        {
          /* Ignore empty path elements.  Supporting them well is hard
             (where do you save "http://x.com///y.html"?), and they
             don't bring any practical gain.  Plus, they break our
             filesystem-influenced assumptions: allowing them would
             make "x/y//../z" simplify to "x/y/z", whereas most people
             would expect "x/z".  */
          ++h;
        }
      else
        {
          /* A regular path element.  If H hasn't advanced past T,
             simply skip to the next path element.  Otherwise, copy
             the path element until the next slash.  */
          if (t == h)
            {
              /* Skip the path element, including the slash.  */
              while (h < end && *h != '/')
                t++, h++;
              if (h < end)
                t++, h++;
            }
          else
            {
              /* Copy the path element, including the final slash.  */
              while (h < end && *h != '/')
                *t++ = *h++;
              if (h < end)
                *t++ = *h++;
            }
        }
    }

  if (t != h)
    *t = '\0';

  return t != h;
}

/* Merge BASE with LINK and return the resulting URI.

   Either of the URIs may be absolute or relative, complete with the
   host name, or path only.  This tries to reasonably handle all
   foreseeable cases.  It only employs minimal URL parsing, without
   knowledge of the specifics of schemes.

   Perhaps this function should call path_simplify so that the callers
   don't have to call url_parse unconditionally.  */

char *
uri_merge (const char *base, const char *link)
{
  int linklength;
  char *end;
  char *merge;

  if (url_has_scheme (link))
    return xstrdup (link);

  /* We may not examine BASE past END. */
  end = (char *)(base + path_length ((char *)base)) ;
  linklength = strlen (link);

  if (!*link)
    {
      /* Empty LINK points back to BASE, query string and all. */
      return xstrdup (base);
    }
  else if (*link == '?')
    {
      /* LINK points to the same location, but changes the query
         string.  Examples: */
      /* uri_merge("path",         "?new") -> "path?new"     */
      /* uri_merge("path?foo",     "?new") -> "path?new"     */
      /* uri_merge("path?foo#bar", "?new") -> "path?new"     */
      /* uri_merge("path#foo",     "?new") -> "path?new"     */
      int baselength = end - base;
      merge = (char *)xmalloc (baselength + linklength + 1);
      memcpy (merge, base, baselength);
      memcpy (merge + baselength, link, linklength);
      merge[baselength + linklength] = '\0';
    }
  else if (*link == '#')
    {
      /* uri_merge("path",         "#new") -> "path#new"     */
      /* uri_merge("path#foo",     "#new") -> "path#new"     */
      /* uri_merge("path?foo",     "#new") -> "path?foo#new" */
      /* uri_merge("path?foo#bar", "#new") -> "path?foo#new" */
      int baselength;
      const char *end1 = strchr (base, '#');
      if (!end1)
        end1 = base + strlen (base);
      baselength = end1 - base;
      merge = (char *)xmalloc (baselength + linklength + 1);
      memcpy (merge, base, baselength);
      memcpy (merge + baselength, link, linklength);
      merge[baselength + linklength] = '\0';
    }
  else if (*link == '/' && *(link + 1) == '/')
    {
      /* LINK begins with "//" and so is a net path: we need to
 replace everything after (and including) the double slash
 with LINK. */

      /* uri_merge("foo", "//new/bar")            -> "//new/bar"      */
      /* uri_merge("//old/foo", "//new/bar")      -> "//new/bar"      */
      /* uri_merge("http://old/foo", "//new/bar") -> "http://new/bar" */

      int span;
      const char *slash;
      const char *start_insert;

      /* Look for first slash. */
      slash = (char *)memchr (base, '/', end - base);
      /* If found slash and it is a double slash, then replace
 from this point, else default to replacing from the
 beginning.  */
      if (slash && *(slash + 1) == '/')
        start_insert = slash;
      else
        start_insert = base;

      span = start_insert - base;
      merge = (char *)xmalloc (span + linklength + 1);
      if (span)
        memcpy (merge, base, span);
      memcpy (merge + span, link, linklength);
      merge[span + linklength] = '\0';
    }
  else if (*link == '/')
    {
      /* LINK is an absolute path: we need to replace everything
         after (and including) the FIRST slash with LINK.

         So, if BASE is "http://host/whatever/foo/bar", and LINK is
         "/qux/xyzzy", our result should be
         "http://host/qux/xyzzy".  */
      int span;
      const char *slash;
      const char *start_insert = NULL; /* for gcc to shut up. */
      const char *pos = base;
      int seen_slash_slash = 0;
      /* We're looking for the first slash, but want to ignore
         double slash. */
    again:
      slash = (char *)memchr (pos, '/', end - pos);
      if (slash && !seen_slash_slash)
        if (*(slash + 1) == '/')
          {
            pos = slash + 2;
            seen_slash_slash = 1;
            goto again;
          }

      /* At this point, SLASH is the location of the first / after
         "//", or the first slash altogether.  START_INSERT is the
         pointer to the location where LINK will be inserted.  When
         examining the last two examples, keep in mind that LINK
         begins with '/'. */

      if (!slash && !seen_slash_slash)
        /* example: "foo" */
        /*           ^    */
        start_insert = base;
      else if (!slash && seen_slash_slash)
        /* example: "http://foo" */
        /*                     ^ */
        start_insert = end;
      else if (slash && !seen_slash_slash)
        /* example: "foo/bar" */
        /*           ^        */
        start_insert = base;
      else if (slash && seen_slash_slash)
        /* example: "http://something/" */
        /*                           ^  */
        start_insert = slash;

      span = start_insert - base;
      merge = (char *)xmalloc (span + linklength + 1);
      if (span)
        memcpy (merge, base, span);
      memcpy (merge + span, link, linklength);
      merge[span + linklength] = '\0';
    }
  else
    {
      /* LINK is a relative URL: we need to replace everything
 after last slash (possibly empty) with LINK.

 So, if BASE is "whatever/foo/bar", and LINK is "qux/xyzzy",
 our result should be "whatever/foo/qux/xyzzy".  */
      int need_explicit_slash = 0;
      int span;
      const char *start_insert;
      const char *last_slash = find_last_char (base, end, '/');
      if (!last_slash)
        {
          /* No slash found at all.  Append LINK to what we have,
             but we'll need a slash as a separator.

             Example: if base == "foo" and link == "qux/xyzzy", then
             we cannot just append link to base, because we'd get
             "fooqux/xyzzy", whereas what we want is
             "foo/qux/xyzzy".

             To make sure the / gets inserted, we set
             need_explicit_slash to 1.  We also set start_insert
             to end + 1, so that the length calculations work out
             correctly for one more (slash) character.  Accessing
             that character is fine, since it will be the
             delimiter, '\0' or '?'.  */
          /* example: "foo?..." */
          /*               ^    ('?' gets changed to '/') */
          start_insert = end + 1;
          need_explicit_slash = 1;
        }
      else if (last_slash && last_slash >= base + 2
       && last_slash[-2] == ':' && last_slash[-1] == '/')
        {
          /* example: http://host"  */
          /*                      ^ */
          start_insert = end + 1;
          need_explicit_slash = 1;
        }
      else
        {
          /* example: "whatever/foo/bar" */
          /*                        ^    */
          start_insert = last_slash + 1;
        }

      span = start_insert - base;
      merge = (char *)xmalloc (span + linklength + 1);
      if (span)
        memcpy (merge, base, span);
      if (need_explicit_slash)
        merge[span - 1] = '/';
      memcpy (merge + span, link, linklength);
      merge[span + linklength] = '\0';
    }

  return merge;
}

#define APPEND(p, s) do {\
  int len = strlen (s);\
  memcpy (p, s, len);\
  p += len;\
} while (0)

/* Use this instead of password when the actual password is supposed
   to be hidden.  We intentionally use a generic string without giving
   away the number of characters in the password, like previous
   versions did.  */
#define HIDDEN_PASSWORD "*password*"

/* Recreate the URL string from the data in URL.

   If HIDE is non-zero (as it is when we're calling this on a URL we
   plan to print, but not when calling it to canonicalize a URL for
   use within the program), password will be hidden.  Unsafe
   characters in the URL will be quoted.  */

char *
url_string (const ZMapURL url, int hide_password)
{
  int size;
  char *result, *p;
  const char *quoted_user = NULL, *quoted_passwd = NULL;

  int scheme_port  = supported_schemes[url->scheme].default_port;
  char *scheme_str = (char *)supported_schemes[url->scheme].leading_string;
  int fplen = full_path_length (url);

  int brackets_around_host = 0;

  assert (scheme_str != NULL);

  /* Make sure the user name and password are quoted. */
  if (url->user)
    {
      quoted_user = url_escape_allow_passthrough (url->user);
      if (url->passwd)
        {
          if (hide_password)
            quoted_passwd = HIDDEN_PASSWORD;
          else
            quoted_passwd = url_escape_allow_passthrough (url->passwd);
        }
    }

  if (strchr (url->host, ':'))
    brackets_around_host = 1;

  size = (strlen (scheme_str)
  + strlen (url->host)
  + (brackets_around_host ? 2 : 0)
  + fplen
  + 1);
  if (url->port != scheme_port)
    size += 1 + numdigit (url->port);
  if (quoted_user)
    {
      size += 1 + strlen (quoted_user);
      if (quoted_passwd)
        size += 1 + strlen (quoted_passwd);
    }

  p = result = (char *)xmalloc (size);

  APPEND (p, scheme_str);
  if (quoted_user)
    {
      APPEND (p, quoted_user);
      if (quoted_passwd)
        {
          *p++ = ':';
          APPEND (p, quoted_passwd);
        }
      *p++ = '@';
    }

  if (brackets_around_host)
    *p++ = '[';
  APPEND (p, url->host);
  if (brackets_around_host)
    *p++ = ']';
  if (url->port != scheme_port)
    {
      *p++ = ':';
      p = number_to_string (p, url->port);
    }

  full_path_write (url, p);
  p += fplen;
  *p++ = '\0';

  assert (p - result == size);

  if (quoted_user && quoted_user != url->user)
    xfree ((void *)quoted_user);
  if (quoted_passwd && !hide_password
      && quoted_passwd != url->passwd)
    xfree ((void *)quoted_passwd);

  return result;
}

/* Return non-zero if scheme a is similar to scheme b.

   Schemes are similar if they are equal.  If SSL is supported, schemes
   are also similar if one is http (SCHEME_HTTP) and the other is https
   (SCHEME_HTTPS).  */
int
schemes_are_similar_p (ZMapURLScheme a, ZMapURLScheme b)
{
  if (a == b)
    return 1;
#ifdef HAVE_SSL
  if ((a == SCHEME_HTTP && b == SCHEME_HTTPS)
      || (a == SCHEME_HTTPS && b == SCHEME_HTTP))
    return 1;
#endif
  return 0;
}

static char *
protocol_from_scheme(ZMapURLScheme scheme)
{
  char *tmp = NULL;
  tmp = (char *)supported_schemes[scheme].leading_string;
  if(tmp)
    return strdupdelim(tmp, tmp + strlen(tmp) - 3);
  return tmp;
}


char *zMapURLGetQueryValue(const char *full_query, const char *key)
{
  char *value = NULL, **split   = NULL, **ptr     = NULL ;

  if ((full_query && *full_query) && (key && *key))
    {
      split = ptr = g_strsplit(full_query, "&", 0) ;

      while(ptr && *ptr != NULL && **ptr != '\0')
        {
          char **key_value = NULL, **kv_ptr, *real_key ;

          key_value = kv_ptr = g_strsplit(*ptr, "=", 0);

          /* Can this actually happen ? */
          if (key_value[0])
            {
              real_key = *key_value ;
              if (*real_key == '?')
                real_key++ ;

              if (g_ascii_strcasecmp(real_key, key) == 0)
                value = g_strdup(key_value[1]) ;
            }

          g_strfreev(kv_ptr) ;

          ptr++ ;
        }

      g_strfreev(split) ;
    }

  return value;
}

gboolean zMapURLGetQueryBoolean(const char *full_query, const char *key)
{
  gboolean result = FALSE;
  char *value = NULL;

  if((value = zMapURLGetQueryValue(full_query, key)))
    {
      if(g_ascii_strcasecmp("true", value) == 0)
        result = TRUE;
      g_free(value);
    }

  return result;
}

int zMapURLGetQueryInt(const char *full_query, const char *key)
{
  int result = 0 ;
  char *value = NULL ;

  if ((value = zMapURLGetQueryValue(full_query, key)))
    {
      result = atoi(value) ;

      g_free(value) ;
    }

  return result ;
}

#if 0
/* Debugging and testing support for path_simplify. */

/* Debug: run path_simplify on PATH and return the result in a new
   string.  Useful for calling from the debugger.  */
static char *
ps (char *path)
{
  char *copy_string = xstrdup (path);
  path_simplify (copy_string);
  return copy_string;
}

static void
run_test (char *test, char *expected_result, int expected_change)
{
  char *test_copy = xstrdup (test);
  int modified = path_simplify (test_copy);

  if (0 != strcmp (test_copy, expected_result))
    {
      printf ("Failed path_simplify(\"%s\"): expected \"%s\", got \"%s\".\n",
      test, expected_result, test_copy);
    }
  if (modified != expected_change)
    {
      if (expected_change == 1)
printf ("Expected no modification with path_simplify(\"%s\").\n",
test);
      else
printf ("Expected modification with path_simplify(\"%s\").\n",
test);
    }
  xfree (test_copy);
}

static void
test_path_simplify (void)
{
  static struct {
    char *test, *result;
    int should_modify;
  } tests[] = {
    { "","",0 },
    { ".","",1 },
    { "..","",1 },
    { "foo","foo",0 },
    { "foo/bar","foo/bar",0 },
    { "foo///bar","foo/bar",1 },
    { "foo/.","foo/",1 },
    { "foo/./","foo/",1 },
    { "foo./","foo./",0 },
    { "foo/../bar","bar",1 },
    { "foo/../bar/","bar/",1 },
    { "foo/bar/..","foo/",1 },
    { "foo/bar/../x","foo/x",1 },
    { "foo/bar/../x/","foo/x/",1 },
    { "foo/..","",1 },
    { "foo/../..","",1 },
    { "a/b/../../c","c",1 },
    { "./a/../b","b",1 }
  };
  int i;

  for (i = 0; i < countof (tests); i++)
    {
      char *test = tests[i].test;
      char *expected_result = tests[i].result;
      int   expected_change = tests[i].should_modify;
      run_test (test, expected_result, expected_change);
    }

  /* Now run all the tests with a leading slash before the test case,
     to prove that the slash is being preserved.  */
  for (i = 0; i < countof (tests); i++)
    {
      char *test, *expected_result;
      int expected_change = tests[i].should_modify;

      test = xmalloc (1 + strlen (tests[i].test) + 1);
      sprintf (test, "/%s", tests[i].test);

      expected_result = xmalloc (1 + strlen (tests[i].result) + 1);
      sprintf (expected_result, "/%s", tests[i].result);

      run_test (test, expected_result, expected_change);

      xfree (test);
      xfree (expected_result);
    }
}
#endif
