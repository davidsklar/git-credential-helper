
Collection of Git credential helpers
====================================

This repository contains the set of Git credential helpers (`gitcredentials(7)`) that are part of `git` (or meant to be contributed in the future).

The latest discussion about some refactoring can be found [here](http://thread.gmane.org/gmane.comp.version-control.git/204154).

Currently supported git credential backends are

 * *GnomeKeyring*
 * *Mac OS X Keychain*
 * *Windows Credential API*

Compared to the current upstream version, the implementations in this repository are based on a generic helper implementation that provides the basic common infrastructure for new credential helpers.

Installation & Usage
--------------------

```sh
$ git clone git://github.com/pah/git-credential-helper.git
$ BACKEND=gnome-keyring      # or any other backend
$ cd git-credential-helper/$BACKEND
$ make
$ cp git-credential-$BACKEND /some/dir/in/path
```

To use this backend, you can add it to your (global) Git configuration by setting

```sh
$ git config [--global] credential.helper $BACKEND
```

See `gitcredentials(7)` for details.

Adding a new backend using the generic helper
---------------------------------------------


To implement a new backend, one needs to implement functions for each credential operation to support.

```c
/* include generic helper */
#include <credential_helper.h>

/* ... include backend specific stuff */

/*
 * implement credential operation functions
 * e.g. for get, store, erase
 */

int my_backend_get(struct credential *cred); 
```

To complete the implementation, add all functions to the table `credential_helper_ops`:

```c
struct credential_operation const credential_helper_ops[] =
{
	{ "get",   my_backend_get   },
	{ "store", my_backend_store },
	{ "erase", my_backend_erase },
	CREDENTIAL_OP_END
};
```

The `Makefile` in the individual backend directories can be used as a template to build new backends.  Usually, it is sufficient to change the name of the +MAIN+ component and to add backend-specific compiler flags to the `INCS` and `LIBS` variables.


TODO
----

 * Add global `init` and `cleanup` functions to the generic helper
 * Add support for command-line options via the generic helper


License
-------

These files are licensed under the GPL v2, or a later version
at the discretion of Linus.
