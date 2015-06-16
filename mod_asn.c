
/*
 * Copyright (c) 2008-2010 Peter Poeml <poeml@mirrorbrain.org> / Novell Inc.
 * Copyright (c) 2008-2015 Peter Poeml <poeml@mirrorbrain.org>
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 *
 *     mod_asn
 *
 * look up the autonomous system, and network prefix, that a client's IP
 * address is belonging to
 *
 * see http://mirrorbrain.org/mod_asn/
 *
 */

#include "ap_config.h"
#include "httpd.h"
#include "http_request.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"

#include "apr_version.h"
#include "apu_version.h"
#include "apr_strings.h"
#include "apr_lib.h"
#include "apr_dbd.h"
#include "mod_dbd.h"


#ifndef UNSET
#define UNSET (-1)
#endif

#define MOD_ASN_VER "1.7"
#define VERSION_COMPONENT "mod_asn/"MOD_ASN_VER

/* from ssl/ssl_engine_config.c */
#define cfgMerge(el,unset)  mrg->el = (add->el == (unset)) ? base->el : add->el
#define cfgMergeArray(el)   mrg->el = apr_array_append(p, add->el, base->el)
#define cfgMergeString(el)  cfgMerge(el, NULL)
#define cfgMergeBool(el)    cfgMerge(el, UNSET)
#define cfgMergeInt(el)     cfgMerge(el, UNSET)

#define DEFAULT_QUERY "SELECT pfx, asn FROM pfx2asn WHERE pfx >>= ipaddress(%s) ORDER BY @ pfx LIMIT 1"

module AP_MODULE_DECLARE_DATA asn_module;


/* per-dir configuration */
typedef struct
{
    int asn_enabled;
    int set_headers;
    int debug;
} asn_dir_conf;

/* per-server configuration */
typedef struct
{
    const char *query;
    const char *query_prep;
    const char *ip_header;
    const char *ip_envvar;
} asn_server_conf;



/* optional functions - look them up once in post_config */
static ap_dbd_t *(*asn_dbd_open_fn)(apr_pool_t*, server_rec*) = NULL;
static void (*asn_dbd_close_fn)(server_rec*, ap_dbd_t*) = NULL;
static void (*asn_dbd_prepare_fn)(server_rec*, const char*, const char*) = NULL;

static apr_version_t vsn;
static int dbd_first_row;


static void debugLog(const request_rec *r, const asn_dir_conf *cfg,
                     const char *fmt, ...)
{
    if (cfg->debug == 1) {
        char buf[512];
        va_list ap;
        va_start(ap, fmt);
        apr_vsnprintf(buf, sizeof (buf), fmt, ap);
        va_end(ap);
        /* we use warn loglevel to be able to debug without 
         * setting the entire server into debug logging mode.
	 * (Apache 2.4 got per-module loglevel configuration; so, in case that 
	 * Apache 2.2 should no longer be supported by us in the future, we
	 * could remove this function */
        ap_log_rerror(APLOG_MARK,
                      APLOG_WARNING, 
                      APR_SUCCESS,
                      r, "[mod_asn] %s", buf);
    }
}


static int asn_post_config(apr_pool_t *pconf, apr_pool_t *plog, 
                               apr_pool_t *ptemp, server_rec *s)
{
    apr_version(&vsn);
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s,
                 "[mod_asn] compiled with APR/APR-Util %s/%s",
                 APR_VERSION_STRING, APU_VERSION_STRING);

    if ((vsn.major == 1) && (vsn.minor == 2)) {
        dbd_first_row = 0;
    } else {
        dbd_first_row = 1;
    }

    ap_add_version_component(pconf, VERSION_COMPONENT);

    /* make sure that mod_dbd is loaded */
    if (asn_dbd_prepare_fn == NULL) {
        asn_dbd_prepare_fn = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_prepare);
        if (asn_dbd_prepare_fn == NULL) {
            ap_log_error(APLOG_MARK, APLOG_ERR, 0, s,
                         "[mod_asn] You must load mod_dbd to enable mod_asn to work");
        return HTTP_INTERNAL_SERVER_ERROR;
        }
        asn_dbd_open_fn = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_open);
        asn_dbd_close_fn = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_close);
    }

    /* prepare DBD SQL statements */
    static unsigned int label_num = 0;
    server_rec *sp;
    for (sp = s; sp; sp = sp->next) {
        asn_server_conf *cfg = ap_get_module_config(sp->module_config, 
                                                        &asn_module);
        /* make a label */
        cfg->query_prep = apr_psprintf(pconf, "asn_dbd_%d", ++label_num);
        asn_dbd_prepare_fn(sp, cfg->query, cfg->query_prep);
    }

    return OK;
}

static void *create_asn_dir_config(apr_pool_t *p, char *dirspec)
{
    asn_dir_conf *new =
      (asn_dir_conf *) apr_pcalloc(p, sizeof(asn_dir_conf));

    new->asn_enabled  = UNSET;
    new->set_headers  = UNSET;
    new->debug          = UNSET;
    return (void *) new;
}

static void *merge_asn_dir_config(apr_pool_t *p, void *basev, void *addv)
{
    asn_dir_conf *mrg  = (asn_dir_conf *) apr_pcalloc(p, sizeof(asn_dir_conf));
    asn_dir_conf *base = (asn_dir_conf *) basev;
    asn_dir_conf *add  = (asn_dir_conf *) addv;

    cfgMergeInt(asn_enabled);
    cfgMergeInt(set_headers);
    cfgMergeInt(debug);
    return (void *) mrg;
}

static void *create_asn_server_config(apr_pool_t *p, server_rec *s)
{
    asn_server_conf *new =
      (asn_server_conf *) apr_pcalloc(p, sizeof(asn_server_conf));

    new->query = DEFAULT_QUERY;
    new->query_prep = NULL;
    new->ip_header = NULL;
    new->ip_envvar = NULL;
    return (void *) new;
}

static void *merge_asn_server_config(apr_pool_t *p, void *basev, void *addv)
{
    asn_server_conf *base = (asn_server_conf *) basev;
    asn_server_conf *add = (asn_server_conf *) addv;
    asn_server_conf *mrg = apr_pcalloc(p, sizeof(asn_server_conf));

    mrg->query = (add->query != (char *) DEFAULT_QUERY) ? add->query : base->query;
    cfgMergeString(query_prep);
    cfgMergeString(ip_header);
    cfgMergeString(ip_envvar);
    return (void *) mrg;
}

static const char *asn_cmd_asn(cmd_parms *cmd, void *config, int flag)
{
    asn_dir_conf *cfg = (asn_dir_conf *) config;
    cfg->asn_enabled = flag;
    return NULL;
}

static const char *asn_cmd_debug(cmd_parms *cmd, void *config, int flag)
{
    asn_dir_conf *cfg = (asn_dir_conf *) config;
    cfg->debug = flag;
    return NULL;
}

static const char *asn_cmd_setheaders(cmd_parms *cmd, void *config, int flag)
{
    asn_dir_conf *cfg = (asn_dir_conf *) config;
    cfg->set_headers = flag;
    return NULL;
}

static const char *asn_cmd_dbdquery(cmd_parms *cmd, 
                                void *config, const char *arg1)
{
    server_rec *s = cmd->server;
    asn_server_conf *cfg = 
        ap_get_module_config(s->module_config, &asn_module);

    cfg->query = arg1;
    return NULL;
}

static const char *asn_cmd_ip_header(cmd_parms *cmd, 
                                void *config, const char *arg1)
{
    server_rec *s = cmd->server;
    asn_server_conf *cfg = 
        ap_get_module_config(s->module_config, &asn_module);

    cfg->ip_header = arg1;
    return NULL;
}

static const char *asn_cmd_ip_envvar(cmd_parms *cmd, 
                                void *config, const char *arg1)
{
    server_rec *s = cmd->server;
    asn_server_conf *cfg = 
        ap_get_module_config(s->module_config, &asn_module);

    cfg->ip_envvar = arg1;
    return NULL;
}

static int asn_header_parser(request_rec *r)
{
    asn_dir_conf *cfg = NULL;
    asn_server_conf *scfg = NULL;
    const char *clientip = NULL;
    const char *pfx = NULL;
    const char *as = NULL;
    apr_status_t rv;
    apr_dbd_prepared_t *statement;
    apr_dbd_results_t *res = NULL;
    apr_dbd_row_t *row = NULL;

    cfg = (asn_dir_conf *)     ap_get_module_config(r->per_dir_config, 
                                                        &asn_module);
    scfg = (asn_server_conf *) ap_get_module_config(r->server->module_config, 
                                                        &asn_module);

    /* are ASLookups disabled for this directory? */
    if (cfg->asn_enabled != 1) {
        return DECLINED;
    }
    debugLog(r, cfg, "ASLookups On");


    if (scfg->query == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "[mod_asn] No ASLookupQuery configured!");
        return DECLINED;
    }
    if (scfg->query_prep == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "[mod_asn] No database query prepared!");
        return DECLINED;
    }
    ap_dbd_t *dbd = asn_dbd_open_fn(r->pool, r->server);
    if (dbd == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                "[mod_asn] Error acquiring database connection");
        return DECLINED;
    }
    debugLog(r, cfg, "Successfully acquired database connection.");

    statement = apr_hash_get(dbd->prepared, scfg->query_prep, APR_HASH_KEY_STRING);
    if (statement == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "[mod_asn] Could not get prepared statement!");
        asn_dbd_close_fn(r->server, dbd);
        return DECLINED;
    }

    if (scfg->ip_header) {
        clientip = apr_table_get(r->headers_in, scfg->ip_header);
        debugLog(r, cfg, "client ip from %s header: %s", scfg->ip_header, clientip);
    } else if (scfg->ip_envvar) {
        clientip = apr_table_get(r->subprocess_env, scfg->ip_envvar);
        debugLog(r, cfg, "client ip from %s envvar: %s", scfg->ip_envvar, clientip);
    } else {
#if AP_MODULE_MAGIC_AT_LEAST(20111130, 0)
        clientip = apr_pstrdup(r->pool, r->useragent_ip);
#else
        clientip = apr_pstrdup(r->pool, r->connection->remote_ip);
#endif
    }

    if (!clientip) {
        debugLog(r, cfg, "empty client ip... not doing a lookup");
        asn_dbd_close_fn(r->server, dbd);
        return DECLINED;
    }

    
    /* 0: sequential. must loop over all rows.
     * 1: random. accessing invalid row (-1) will clear the cursor. */
    if (apr_dbd_pvselect(dbd->driver, r->pool, dbd->handle, &res, statement, 0,
                         clientip, NULL) != 0) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                "[mod_asn] Error looking up %s in database", clientip);
        asn_dbd_close_fn(r->server, dbd);
        return DECLINED;
    }

    /* we care only about the 1st row, because our query uses 'limit 1' */
    rv = apr_dbd_get_row(dbd->driver, r->pool, res, &row, dbd_first_row);
    if (rv != APR_SUCCESS) {
        if (rv == -1) {
            /* not an error - might be a private IP, for instance */
            ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r,
                          "[mod_asn] IP %s not found", clientip);
        } else {
            const char *errmsg = apr_dbd_error(dbd->driver, dbd->handle, rv);
            ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                          "[mod_asn] Error retrieving row from database for %s: %s", 
                          clientip, (errmsg ? errmsg : "[???]"));
        }
        asn_dbd_close_fn(r->server, dbd);
        return DECLINED;
    }

    if ((pfx = apr_dbd_get_entry(dbd->driver, row, 0)) == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                      "[mod_asn] apr_dbd_get_entry found NULL for pfx");
    }
    if ((as = apr_dbd_get_entry(dbd->driver, row, 1)) == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                      "[mod_asn] apr_dbd_get_entry found NULL for as");
    }

    /* clear the cursor by accessing invalid row */
    rv = apr_dbd_get_row(dbd->driver, r->pool, res, &row, dbd_first_row + 1);
    if (rv != -1) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                      "[mod_asn] found one row too much looking up %s", 
                      clientip);
        asn_dbd_close_fn(r->server, dbd);
        return DECLINED;
    }

    debugLog(r, cfg, "data: pfx=%s, as=%s", pfx, as);

    /* save details for logging via a CustomLog */
    apr_table_setn(r->subprocess_env, "PFX", apr_pstrdup(r->pool, pfx));
    apr_table_setn(r->subprocess_env, "ASN", apr_pstrdup(r->pool, as));
    if (cfg->set_headers == 1) {
        apr_table_setn(r->err_headers_out, "X-Prefix", apr_pstrdup(r->pool, pfx));
        apr_table_setn(r->err_headers_out, "X-AS", apr_pstrdup(r->pool, as));
    }

    asn_dbd_close_fn(r->server, dbd);
    return OK;
}


static const command_rec asn_cmds[] =
{
    /* to be used only in Directory et al. */
    AP_INIT_FLAG("ASLookup", asn_cmd_asn, NULL, 
                 ACCESS_CONF,
                 "Set to On or Off to enable or disable AS number lookups"),
    AP_INIT_FLAG("ASLookupDebug", asn_cmd_debug, NULL, 
                 ACCESS_CONF,
                 "Set to On or Off to enable or disable debug logging to error log"),
    AP_INIT_FLAG("ASSetHeaders", asn_cmd_setheaders, NULL, 
                 ACCESS_CONF,
                 "Set to On or Off to enable or disable adding the lookup result to "
                 "the response headers"),

    /* to be used only in server context */
    AP_INIT_TAKE1("ASLookupQuery", asn_cmd_dbdquery, NULL,
                  RSRC_CONF,
                  "the SQL query string to use"),
    AP_INIT_TAKE1("ASIPHeader", asn_cmd_ip_header, NULL,
                  RSRC_CONF,
                  "Take the IP address from this HTTP request header, instead "
                  "of the IP address that the request originates from"),
    AP_INIT_TAKE1("ASIPEnvvar", asn_cmd_ip_envvar, NULL,
                  RSRC_CONF,
                  "Use this string to look up the IP address in the subprocess "
                  "environment, instead of using the IP address that the request "
                  " originates from"),

    { NULL }
};

static void asn_register_hooks(apr_pool_t *p)
{
    ap_hook_post_config    (asn_post_config, NULL, NULL, APR_HOOK_MIDDLE);

    static const char * const aszSucc[]={ "mod_setenvif.c", "mod_rewrite.c", NULL };
    ap_hook_header_parser  (asn_header_parser, NULL, aszSucc, APR_HOOK_FIRST );
}


#ifdef AP_DECLARE_MODULE
AP_DECLARE_MODULE(asn) =
#else 
/* pre-2.4 */
module AP_MODULE_DECLARE_DATA asn_module =
#endif
{
    STANDARD20_MODULE_STUFF,
    create_asn_dir_config,      /* create per-directory config structures */
    merge_asn_dir_config,       /* merge per-directory config structures  */
    create_asn_server_config,   /* create per-server config structures    */
    merge_asn_server_config,    /* merge per-server config structures     */
    asn_cmds,                   /* command handlers */
    asn_register_hooks          /* register hooks */
};


/* vim: set ts=4 sw=4 expandtab smarttab: */
