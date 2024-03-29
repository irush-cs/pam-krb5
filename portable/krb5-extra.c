/*
 * Portability glue functions for Kerberos.
 *
 * This file provides definitions of the interfaces that portable/krb5.h
 * ensures exist if the function wasn't available in the Kerberos libraries.
 * Everything in this file will be protected by #ifndef.  If the native
 * Kerberos libraries are fully capable, this file will be skipped.
 *
 * The canonical version of this file is maintained in the rra-c-util package,
 * which can be found at <http://www.eyrie.org/~eagle/software/rra-c-util/>.
 *
 * Written by Russ Allbery <rra@stanford.edu>
 *
 * The authors hereby relinquish any claim to any copyright that they may have
 * in this work, whether granted under contract or by operation of law or
 * international treaty, and hereby commit to the public, at large, that they
 * shall not, at any time in the future, seek to enforce any copyright in this
 * work against any person or entity, or prevent any person or entity from
 * copying, publishing, distributing or creating derivative works of this
 * work.
 */

#include <config.h>
#include <portable/krb5.h>
#include <portable/system.h>

#include <errno.h>

/* Figure out what header files to include for error reporting. */
#if !defined(HAVE_KRB5_GET_ERROR_MESSAGE) && !defined(HAVE_KRB5_GET_ERR_TEXT)
# if !defined(HAVE_KRB5_GET_ERROR_STRING)
#  if defined(HAVE_IBM_SVC_KRB5_SVC_H)
#   include <ibm_svc/krb5_svc.h>
#  elif defined(HAVE_ET_COM_ERR_H)
#   include <et/com_err.h>
#  else
#   include <com_err.h>
#  endif
# endif
#endif

/* Used for unused parameters to silence gcc warnings. */
#define UNUSED __attribute__((__unused__))

/*
 * This string is returned for unknown error messages.  We use a static
 * variable so that we can be sure not to free it.
 */
static const char error_unknown[] = "unknown error";


#ifndef HAVE_KRB5_GET_ERROR_MESSAGE
/*
 * Given a Kerberos error code, return the corresponding error.  Prefer the
 * Kerberos interface if available since it will provide context-specific
 * error information, whereas the error_message() call will only provide a
 * fixed message.
 */
const char *
krb5_get_error_message(krb5_context ctx UNUSED, krb5_error_code code UNUSED)
{
    const char *msg = NULL;

# if defined(HAVE_KRB5_GET_ERROR_STRING)
    msg = krb5_get_error_string(ctx);
# elif defined(HAVE_KRB5_GET_ERR_TEXT)
    msg = krb5_get_err_text(ctx, code);
# elif defined(HAVE_KRB5_SVC_GET_MSG)
    krb5_svc_get_msg(code, (char **) &msg);
# else
    msg = error_message(code);
# endif
    if (msg == NULL)
        return error_unknown;
    else
        return msg;
}
#endif /* !HAVE_KRB5_GET_ERROR_MESSAGE */


#ifndef HAVE_KRB5_FREE_ERROR_MESSAGE
/*
 * Free an error string if necessary.  If we returned a static string, make
 * sure we don't free it.
 *
 * This code assumes that the set of implementations that have
 * krb5_free_error_message is a subset of those with krb5_get_error_message.
 * If this assumption ever breaks, we may call the wrong free function.
 */
void
krb5_free_error_message(krb5_context ctx UNUSED, const char *msg)
{
    if (msg == error_unknown)
        return;
# if defined(HAVE_KRB5_GET_ERROR_STRING)
    krb5_free_error_string(ctx, (char *) msg);
# elif defined(HAVE_KRB5_SVC_GET_MSG)
    krb5_free_string(ctx, (char *) msg);
# endif
}
#endif /* !HAVE_KRB5_FREE_ERROR_MESSAGE */


#ifndef HAVE_KRB5_GET_INIT_CREDS_OPT_ALLOC
/*
 * Allocate and initialize a krb5_get_init_creds_opt struct.  This code
 * assumes that an all-zero bit pattern will create a NULL pointer.
 */
krb5_error_code
krb5_get_init_creds_opt_alloc(krb5_context ctx UNUSED,
                              krb5_get_init_creds_opt **opts)
{
    *opts = calloc(1, sizeof(krb5_get_init_creds_opt));
    if (*opts == NULL)
        return errno;
    krb5_get_init_creds_opt_init(*opts);
    return 0;
}
#endif /* !HAVE_KRB5_GET_INIT_CREDS_OPT_ALLOC */


#ifndef HAVE_KRB5_PRINCIPAL_GET_REALM
/*
 * Return the realm of a principal as a const char *.
 */
const char *
krb5_principal_get_realm(krb5_context ctx UNUSED, krb5_const_principal princ)
{
    const krb5_data *data;

    data = krb5_princ_realm(ctx, princ);
    if (data == NULL || data->data == NULL)
        return NULL;
    return data->data;
}
#endif /* !HAVE_KRB5_PRINCIPAL_GET_REALM */


#ifndef HAVE_KRB5_VERIFY_INIT_CREDS_OPT_INIT
/*
 * Initialize the option struct for krb5_verify_init_creds.
 */
void
krb5_verify_init_creds_opt_init(krb5_verify_init_creds_opt *opt)
{
    opt->flags = 0;
    opt->ap_req_nofail = 0;
}
#endif
