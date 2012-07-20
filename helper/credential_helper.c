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

/*
 * This credential struct and API simplified from git's
 * credential.{h,c} to be used within credential helper
 * implementations.
 */

#include <credential_helper.h>

void credential_init(struct credential* c)
{
	memset(c, 0, sizeof(*c));
}

void credential_clear(struct credential* c)
{
	free(c->protocol);
	free(c->host);
	free(c->path);
	free(c->username);
	free(c->password);

	credential_init(c);
}

int credential_read(struct credential* c)
{
	char*   buf      = NULL;
	size_t  buf_len  = 0;
	ssize_t line_len = 0;

	while( -1 != (line_len = getline(&buf, &buf_len, stdin)))
  {
		char *key   = buf;
		char *value = strchr(buf, '=');

		if(buf[line_len-1]=='\n')
			buf[--line_len]='\0';

		if(!line_len)
			break;

		if(!value) {
			warning("invalid credential line: %s", key);
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

void credential_write_item(FILE *fp, const char *key, const char *value)
{
	if (!value)
		return;
	fprintf(fp, "%s=%s\n", key, value);
}

void credential_write(const struct credential *c)
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

static void usage(const char *name)
{
	struct credential_operation const *try_op = credential_helper_ops;
	const char *basename = strrchr(name,'/');

	basename = (basename) ? basename + 1 : name;
	fprintf(stderr, "Usage: %s <", basename);
	while(try_op->name) {
		fprintf(stderr,"%s",(try_op++)->name);
		if(try_op->name)
			fprintf(stderr,"%s","|");
	}
	fprintf(stderr,"%s",">\n");
}

/* 
 * generic main function for credential helpers
 */
int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;

	struct credential_operation const *try_op = credential_helper_ops;
	struct credential                  cred   = CREDENTIAL_INIT;

	if (!argv[1]) {
		usage(argv[0]);
		goto out;
  }

	/* lookup operation callback */
	while(try_op->name && strcmp(argv[1], try_op->name))
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
