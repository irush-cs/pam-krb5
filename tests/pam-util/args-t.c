/*
 * PAM utility argument initialization test suite.
 *
 * The canonical version of this file is maintained in the rra-c-util package,
 * which can be found at <http://www.eyrie.org/~eagle/software/rra-c-util/>.
 *
 * Written by Russ Allbery <rra@stanford.edu>
 * Copyright 2010
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <config.h>
#include <portable/pam.h>
#include <portable/system.h>

#include <pam-util/args.h>
#include <tests/fakepam/pam.h>
#include <tests/tap/basic.h>


int
main(void)
{
    pam_handle_t *pamh;
    struct pam_conv conv = { NULL, NULL };
    struct pam_args *args;

    plan(11);

    if (pam_start("test", NULL, &conv, &pamh) != PAM_SUCCESS)
        sysbail("Fake PAM initialization failed");
    args = putil_args_new(pamh, 0);
    ok(args != NULL, "New args struct is not NULL");
    ok(args->pamh == pamh, "...and pamh is correct");
    ok(args->config == NULL, "...and config is NULL");
    ok(args->user == NULL, "...and user is NULL");
    is_int(args->debug, false, "...and debug is false");
    is_int(args->silent, false, "...and silent is false");
#ifdef HAVE_KERBEROS
    ok(args->ctx != NULL, "...and the Kerberos context is initialized");
    ok(args->realm == NULL, "...and realm is NULL");
#else
    skip_block(2, "Kerberos support not configured");
#endif
    putil_args_free(args);
    ok(1, "Freeing the args struct works");

    args = putil_args_new(pamh, PAM_SILENT);
    ok(args != NULL, "New args struct with PAM_SILENT is not NULL");
    is_int(args->silent, true, "...and silent is true");
    putil_args_free(args);

    pam_end(pamh, 0);

    return 0;
}
