/*
 * Copyright (C) 2011 John Szakmeister <john@szakmeister.net>
 *               2012 Philipp A. Hartmann <pah@qo.cx>
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

/*
 * Credits:
 * - GNOME Keyring API handling originally written by John Szakmeister
 * - credential struct and API simplified from git's credential.{h,c}
 * - ported to new helper protocol by Philipp A. Hartmann
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gnome-keyring.h>

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

static void die_errno(int err)
{
	fprintf(stderr, "fatal: %s\n", strerror(err));
	exit(EXIT_FAILURE);
}

static void die_result(GnomeKeyringResult result)
{
	fprintf(stderr, "fatal: %s\n",gnome_keyring_result_to_message(result));
	exit(EXIT_FAILURE);
}

static void warning( const char* fmt, ... )
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "warning: ");
	vfprintf(stderr, fmt, ap);
	va_end(ap); 
}

static char *xstrdup(const char *str)
{
	char *ret = strdup(str);
	if (!ret)
		die_errno(errno);

	return ret;
}

static void credential_init(struct credential* c)
{
	memset(c, 0, sizeof(*c));
}

static void credential_clear(struct credential* c)
{
	free(c->protocol);
	free(c->host);
	free(c->path);
	free(c->username);
	free(c->password);

	credential_init(c);
}

static int credential_read(struct credential* c)
{
	char*   buf      = NULL;
	size_t  buf_len  = 0;
	ssize_t line_len = 0;

	while( -1 != ( line_len = getline( &buf, &buf_len, stdin ) ) ) {
		char *key   = buf;
		char *value = strchr(buf, '=');

		if(buf[line_len-1]=='\n')
			buf[--line_len]='\0';

		if(!line_len)
			break;

		if(!value) {
			warning("invalid credential line: %s", key );
			free(buf);
			return -1;
		}
		*value++ = '\0';

		if (!strcmp(key, "protocol")) {
			free(c->protocol);
			c->protocol = xstrdup(value);
		} else if (!strcmp(key, "host")) {
			free(c->host);
			c->host = xstrdup(value);
		} else if (!strcmp(key, "path")) {
			free(c->path);
			c->path = xstrdup(value);
		} else if (!strcmp(key, "username")) {
			free(c->username);
			c->username = xstrdup(value);
		} else if (!strcmp(key, "password")) {
			free(c->password);
			c->password = xstrdup(value);
		}
		/*
		 * Ignore other lines; we don't know what they mean, but
		 * this future-proofs us when later versions of git do
		 * learn new lines, and the helpers are updated to match.
		 */
	}

	free(buf);
	return 0;
}

static void credential_write_item(FILE *fp, const char *key, const char *value)
{
	if (!value)
		return;
	fprintf(fp, "%s=%s\n", key, value);
}

static void credential_write(const struct credential *c)
{
	/* only write username/password, if set */
#if 0
	credential_write_item(stdout, "protocol", c->protocol);
	credential_write_item(stdout, "host", c->host);
	credential_write_item(stdout, "path", c->path);
#endif
	credential_write_item(stdout, "username", c->username);
	credential_write_item(stdout, "password", c->password);
}

/*  */
static char* keyring_object(struct credential* c)
{
	char* object = NULL;

	if (!c->path)
		return object;

	object = (char*) malloc(strlen(c->host)+strlen(c->path)+2);
	if(!object)
		die_errno(errno);

	sprintf(object,"%s/%s",c->host,c->path);
	return object;
}

int keyring_get( struct credential* c )
{
	int ret = EXIT_SUCCESS;

	char* object = NULL;
	GList *entries;
	GnomeKeyringNetworkPasswordData *password_data;
	GnomeKeyringResult result;

	/*
	 * sanity check that what we're being asked for something sensible
	 */
	if (!c->protocol || !(c->host || c->path))
		return ret;

	object = keyring_object(c);

	result = gnome_keyring_find_network_password_sync(
				c->username,
				NULL /* domain */,
				c->host,
				object,
				c->protocol,
				NULL /* authtype */,
				0    /* port */,
				&entries);

	free(object);

	if (result == GNOME_KEYRING_RESULT_NO_MATCH)
		return ret;

	if (result == GNOME_KEYRING_RESULT_CANCELLED)
		return ret;

	if (result != GNOME_KEYRING_RESULT_OK)
		die_result(result);

	/* pick the first one from the list */
	password_data = (GnomeKeyringNetworkPasswordData *) entries->data;

	free(c->password);
	c->password = xstrdup(password_data->password);

	if (!c->username)
		c->username = xstrdup(password_data->user);

	gnome_keyring_network_password_list_free(entries);

  return ret;
}


int keyring_store(struct credential* c)
{
	int ret = EXIT_SUCCESS;
	guint32 item_id;
	char  *object = NULL;

	/*
	 * Sanity check that what we are storing is actually sensible.
	 * In particular, we can't make a URL without a protocol field.
	 * Without either a host or pathname (depending on the scheme),
	 * we have no primary key. And without a username and password,
	 * we are not actually storing a credential.
	 */
	if (!c->protocol || !(c->host || c->path) ||
	    !c->username || !c->password)
		return ret;

	object = keyring_object(c);

	gnome_keyring_set_network_password_sync(
				GNOME_KEYRING_DEFAULT,
				c->username,
				NULL /* domain */,
				c->host,
				object,
				c->protocol,
				NULL /* authtype */,
				0 /* port */,
				c->password,
				&item_id);

	free(object);
	return ret;
}

int keyring_erase( struct credential* c )
{
	int ret = EXIT_SUCCESS;

	char  *object = NULL;
	GList *entries;
	GnomeKeyringNetworkPasswordData *password_data;
	GnomeKeyringResult result;

	/*
	 * Sanity check that we actually have something to match
	 * against. The input we get is a restrictive pattern,
	 * so technically a blank credential means "erase everything".
	 * But it is too easy to accidentally send this, since it is equivalent
	 * to empty input. So explicitly disallow it, and require that the
	 * pattern have some actual content to match.
	 */
	if (!c->protocol && !c->host && !c->path && !c->username)
		return ret;

	object = keyring_object(c);

	result = gnome_keyring_find_network_password_sync(
				c->username,
				NULL /* domain */,
				c->host,
				object,
				c->protocol,
				NULL /* authtype */,
				0 /* port */,
				&entries);

	free(object);

	if (result == GNOME_KEYRING_RESULT_NO_MATCH)
		return ret;

	if (result == GNOME_KEYRING_RESULT_CANCELLED)
		return ret;

	if (result != GNOME_KEYRING_RESULT_OK)
		die_result(result);

	/* pick the first one from the list (delete all matches?) */
	password_data = (GnomeKeyringNetworkPasswordData *) entries->data;

	result = gnome_keyring_item_delete_sync(
		password_data->keyring, password_data->item_id);

	gnome_keyring_network_password_list_free(entries);

	if (result != GNOME_KEYRING_RESULT_OK)
		die_result(result);

	return ret;
}

typedef int (*operation_cb)(struct credential *);

static
struct helper_operation
{
	char         *name;
	operation_cb op;
}
const helper_ops[] =
{
		{ "get",   keyring_get   }
	, { "store", keyring_store }
	, { "erase", keyring_erase }
	, { NULL, NULL }
};

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;

	struct helper_operation const *try_op = helper_ops;
	struct credential              cred   = CREDENTIAL_INIT;

	/* TODO: add options support (e.g. select keyring) */
	if (argc!=2)
		goto out;

	/* lookup operation callback */
	while(try_op->name && strcmp(argv[1],try_op->name))
		try_op++;

	/* unsupported operation given -- ignore silently */
	if(!try_op->name || !try_op->op)
		goto out;

	credential_read(&cred);

	/* perform credential operation */
	ret = (*try_op->op)(&cred);

	credential_write(&cred);
	credential_clear(&cred);

out:
	return ret;
}
