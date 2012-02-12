/*
 * Option handling for pam-krb5.
 *
 * Responsible for initializing the args struct that's passed to nearly all
 * internal functions.  Retrieves configuration information from krb5.conf and
 * parses the PAM configuration.
 *
 * Copyright 2011
 *     The Board of Trustees of the Leland Stanford Junior University
 * Copyright 2005, 2006, 2007, 2008, 2009, 2010
 *     Russ Allbery <rra@stanford.edu>
 * Copyright 2005 Andres Salomon <dilinger@debian.org>
 * Copyright 1999, 2000 Frank Cusack <fcusack@fcusack.com>
 *
 * See LICENSE for licensing terms.
 */

#include <config.h>
#include <portable/krb5.h>
#include <portable/system.h>

#include <errno.h>

#include <internal.h>
#include <pam-util/args.h>
#include <pam-util/logging.h>
#include <pam-util/options.h>
#include <pam-util/vector.h>

/* Our option definition.  Must be sorted. */
#define K(name) (#name), offsetof(struct pam_config, name)
static const struct option options[] = {
    { K(alt_auth_map),       true,  STRING (NULL)  },
    { K(banner),             true,  STRING ("Kerberos") },
    { K(ccache),             true,  STRING (NULL)  },
    { K(ccache_dir),         true,  STRING ("FILE:/tmp") },
    { K(clear_on_fail),      true,  BOOL   (false) },
    { K(debug),              true,  BOOL   (false) },
    { K(defer_pwchange),     true,  BOOL   (false) },
    { K(expose_account),     true,  BOOL   (false) },
    { K(fail_pwchange),      true,  BOOL   (false) },
    { K(fast_ccache),        true,  STRING (NULL)  },
    { K(force_alt_auth),     true,  BOOL   (false) },
    { K(force_first_pass),   false, BOOL   (false) },
    { K(force_pwchange),     true,  BOOL   (false) },
    { K(forwardable),        true,  BOOL   (false) },
    { K(ignore_k5login),     true,  BOOL   (false) },
    { K(ignore_root),        true,  BOOL   (false) },
    { K(keytab),             true,  STRING (NULL)  },
    { K(minimum_uid),        true,  NUMBER (0)     },
    { K(no_ccache),          false, BOOL   (false) },
    { K(no_password),        true,  BOOL   (false) },
    { K(only_alt_auth),      true,  BOOL   (false) },
    { K(pkinit_anchors),     true,  STRING (NULL)  },
    { K(pkinit_prompt),      true,  BOOL   (false) },
    { K(pkinit_user),        true,  STRING (NULL)  },
    { K(preauth_opt),        true,  LIST   (NULL)  },
    { K(prompt_principal),   true,  BOOL   (false) },
    { K(renew_lifetime),     true,  TIME   (0)     },
    { K(retain_after_close), true,  BOOL   (false) },
    { K(search_k5login),     true,  BOOL   (false) },
    { K(ticket_lifetime),    true,  TIME   (0)     },
    { K(try_first_pass),     false, BOOL   (false) },
    { K(try_pkinit),         true,  BOOL   (false) },
    { K(use_authtok),        false, BOOL   (false) },
    { K(use_first_pass),     false, BOOL   (false) },
    { K(use_pkinit),         true,  BOOL   (false) },
};
static const size_t optlen = sizeof(options) / sizeof(options[0]);


/*
 * Allocate a new struct pam_args and initialize its data members, including
 * parsing the arguments and getting settings from krb5.conf.  Check the
 * resulting options for consistency.
 */
struct pam_args *
pamk5_init(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    int i;
    struct pam_args *args;
    struct pam_config *config = NULL;

    args = putil_args_new(pamh, flags);
    if (args == NULL)
        return NULL;
    config = calloc(1, sizeof(struct pam_config));
    if (config == NULL)
        goto nomem;
    args->config = config;

    /*
     * Do an initial scan to see if the realm is already set in our options.
     * If so, make sure that's set before we start loading option values,
     * since it affects what comes out of krb5.conf.
     */
    for (i = 0; i < argc; i++) {
        if (strncmp(argv[i], "realm=", 6) != 0)
            continue;
        if (args->realm != NULL)
            free(args->realm);
        args->realm = strdup(&argv[i][strlen("realm=")]);
        if (args->realm == NULL)
            goto nomem;
    }

    if (!putil_args_defaults(args, options, optlen)) {
        free(config);
        putil_args_free(args);
        return NULL;
    }
    if (!putil_args_krb5(args, "pam", options, optlen))
        goto fail;
    if (!putil_args_parse(args, argc, argv, options, optlen))
        goto fail;
    if (config->debug)
        args->debug = true;

    /* An empty banner should be treated the same as not having one. */
    if (config->banner != NULL && config->banner[0] == '\0') {
        free(config->banner);
        config->banner = NULL;
    }

    /* Sanity-check try_first_pass, use_first_pass, and force_first_pass. */
    if (config->force_first_pass && config->try_first_pass) {
        putil_err(args, "force_first_pass set, ignoring try_first_pass");
        config->try_first_pass = 0;
    }
    if (config->force_first_pass && config->use_first_pass) {
        putil_err(args, "force_first_pass set, ignoring use_first_pass");
        config->use_first_pass = 0;
    }
    if (config->use_first_pass && config->try_first_pass) {
        putil_err(args, "use_first_pass set, ignoring try_first_pass");
        config->try_first_pass = 0;
    }

    /*
     * Don't set expose_account if we're using search_k5login.  The user will
     * get a principal formed from the account into which they're logging in,
     * which isn't the password they'll use (that's the whole point of
     * search_k5login).
     */
    if (config->search_k5login)
        config->expose_account = 0;

    /* UIDs are unsigned on some systems. */
    if (config->minimum_uid < 0)
        config->minimum_uid = 0;

    /*
     * Warn if PKINIT options were set and PKINIT isn't supported.  The MIT
     * method (krb5_get_init_creds_opt_set_pa) can't support use_pkinit.
     */
#ifndef HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PKINIT
# ifndef HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PA
    if (config->try_pkinit)
        putil_err(args, "try_pkinit requested but PKINIT not available");
# endif
    if (config->use_pkinit)
        putil_err(args, "use_pkinit requested but PKINIT not available or"
                  " cannot be enforced");
#endif

    /* Warn if the FAST option was set and FAST isn't supported. */
#ifndef HAVE_KRB5_GET_INIT_CREDS_OPT_SET_FAST_CCACHE_NAME
    if (config->fast_ccache)
        putil_err(args, "fast_ccache requested but FAST not supported by"
                  " Kerberos libraries");
#endif

    return args;

nomem:
    putil_crit(args, "cannot allocate memory: %s", strerror(errno));
    if (config != NULL)
        free(config);
    putil_args_free(args);
    return NULL;

fail:
    pamk5_free(args);
    return NULL;
}


/*
 * Free the allocated args struct and any memory it points to.
 */
void
pamk5_free(struct pam_args *args)
{
    struct pam_config *config;

    if (args == NULL)
        return;
    config = args->config;
    if (config != NULL) {
        if (config->alt_auth_map != NULL)
            free(config->alt_auth_map);
        if (config->banner != NULL)
            free(config->banner);
        if (config->ccache != NULL)
            free(config->ccache);
        if (config->ccache_dir != NULL)
            free(config->ccache_dir);
        if (config->fast_ccache != NULL)
            free(config->fast_ccache);
        if (config->keytab != NULL)
            free(config->keytab);
        if (config->pkinit_anchors != NULL)
            free(config->pkinit_anchors);
        if (config->pkinit_user != NULL)
            free(config->pkinit_user);
        if (config->preauth_opt != NULL)
            vector_free(config->preauth_opt);
        free(args->config);
        args->config = NULL;
    }
    putil_args_free(args);
}
