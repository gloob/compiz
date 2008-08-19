/*
 * Copyright © 2007 Novell, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#ifndef _COMPIZ_H
#define _COMPIZ_H

#include <libxml/parser.h>

#include <compiz-common.h>

#include <string>
#include <vector>
#include <list>

typedef std::string CompString;


typedef int CompBool;
typedef int CompTimeoutHandle;
typedef int CompWatchFdHandle;

typedef struct _CompMetadata CompMetadata;
typedef struct _CompPlugin   CompPlugin;


class CompCore;
class CompDisplay;
class CompScreen;
class CompWindow;
class CompObject;
class CompOption;

typedef union _CompPrivate {
    void	  *ptr;
    long	  val;
    unsigned long uval;
    void	  *(*fptr) (void);
} CompPrivate;


typedef bool (*CallBackProc) (void *closure);

typedef enum {
    CompLogLevelFatal = 0,
    CompLogLevelError,
    CompLogLevelWarn,
    CompLogLevelInfo,
    CompLogLevelDebug
} CompLogLevel;



CompBool
compInitMetadata (CompMetadata *metadata);

CompBool
compInitPluginMetadata (CompMetadata *metadata,
			const char   *plugin);

void
compFiniMetadata (CompMetadata *metadata);

CompBool
compAddMetadataFromFile (CompMetadata *metadata,
			 const char   *file);

CompBool
compAddMetadataFromString (CompMetadata *metadata,
			   const char	*string);

CompBool
compAddMetadataFromIO (CompMetadata	     *metadata,
		       xmlInputReadCallback  ioread,
		       xmlInputCloseCallback ioclose,
		       void		     *ioctx);

char *
compGetStringFromMetadataPath (CompMetadata *metadata,
			       const char   *path);

int
compReadXmlChunk (const char *src,
		  int	     *offset,
		  char	     *buffer,
		  int	     length);



#endif
