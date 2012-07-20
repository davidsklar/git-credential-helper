/*
 * Copyright (C) 2012 Philipp A. Hartmann <pah@qo.cx>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CREDENTIAL_HELPER_H_INCLUDED_
#define CREDENTIAL_HELPER_H_INCLUDED_

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

/*
 * This credential struct and API simplified from git's
 * credential.{h,c} to be used within credential helper
 * implementations.
 */

struct credential
{
	char *protocol;
	char *host;
	char *path;
	char *username;
	char *password;
};

#define CREDENTIAL_INIT \
  { 0,0,0,0,0 }

void credential_init(struct credential* c);
void credential_clear(struct credential* c);
int  credential_read(struct credential* c);
void credential_write(const struct credential *c);

typedef int (*credential_op_cb)(struct credential*);

struct credential_operation
{
	char             *name;
	credential_op_cb op;
};

#define CREDENTIAL_OP_END \
  { 0, 0 }

/*
 * Table with operation callbacks is defined in concrete
 * credential helper implementation.
 *
 */
extern struct credential_operation const credential_helper_ops[];

/* ---------------- helper functions ---------------- */

static inline void die_errno(int err)
{
	fprintf(stderr, "fatal: %s\n", strerror(err));
	exit(EXIT_FAILURE);
}

static inline void warning( const char* fmt, ... )
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "warning: ");
	vfprintf(stderr, fmt, ap);
	va_end(ap); 
}

static inline char *xstrdup(const char *str)
{
	char *ret = strdup(str);
	if (!ret)
		die_errno(errno);

	return ret;
}

#endif /* CREDENTIAL_HELPER_H_INCLUDED_ */
