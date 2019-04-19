#include "config.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include "cache/cache.h"

#include <vcl.h>

#ifndef VRT_H_INCLUDED
#  include <vrt.h>
#endif

#ifndef VDEF_H_INCLUDED
#  include <vdef.h>
#endif

#include "vcc_mongodb_if.h"

const size_t infosz = 64;
char	     *info;

#define POOL_MAX_CONN_STR	"40"
#define POOL_MAX_CONN_PREFIX	"--POOL-MAX="
#define POOL_MAX_CONN_PARAM	" " POOL_MAX_CONN_PREFIX POOL_MAX_CONN_STR
#define POOL_TIMEOUT_MSEC	3000
#define POOL_ERROR_INT		-1
#define POOL_ERROR_STRING	NULL

struct vmod_mongodb_vcl_settings {
	unsigned		magic;
#define VMOD_MDB_SETTINGS_MAGIC 0xe9537153
	mongoc_client_pool_t *pool;
	mongoc_uri_t *uri;
	const char *dbname;
};

static void
free_mdb_vcl_settings(void *data)
{
	struct vmod_mongodb_vcl_settings *settings;
	CAST_OBJ_NOTNULL(settings, data, VMOD_MDB_SETTINGS_MAGIC);

	mongoc_client_pool_destroy (settings->pool);
	mongoc_uri_destroy (settings->uri);
	mongoc_cleanup ();
	FREE_OBJ(settings);
}

int
init_function(const struct vrt_ctx *ctx, struct vmod_priv *priv,
    enum vcl_event_e e)
{
	struct vmod_mongodb_vcl_settings *settings;

	if (e == VCL_EVENT_LOAD) {
		ALLOC_OBJ(settings, VMOD_MDB_SETTINGS_MAGIC);
		AN(settings);
		priv->priv = settings;
		priv->free = free_mdb_vcl_settings;
		mongoc_init ();
	}

	return (0);
}

static mongoc_client_t *
get_client(const struct vrt_ctx *ctx, struct vmod_mongodb_vcl_settings *settings)
{

	mongoc_client_t *mc = NULL;
	CHECK_OBJ_NOTNULL(settings, VMOD_MDB_SETTINGS_MAGIC);
	
	if (!settings->pool) {
		VSL(SLT_VCL_Log, 0, "Could not connect");
		// VRT_handling(ctx, VCL_RET_FAIL);
		return (NULL);
	}
	VSL(SLT_VCL_Log, 0, "Could not connect");

	mc = mongoc_client_pool_pop (settings->pool);
	if (!mc) {
		VSL(SLT_VCL_Log, 0, "Could not connect");
		// VRT_handling(ctx, VCL_RET_FAIL);
		return (NULL);
	}
	return (mc);
}

static void
free_client(const struct vrt_ctx *ctx,
    struct vmod_mongodb_vcl_settings *settings, mongoc_client_t *mc)
{
	CHECK_OBJ_NOTNULL(settings, VMOD_MDB_SETTINGS_MAGIC);
	AN(mc);
	mongoc_client_pool_push (settings->pool, mc);
}



/*
 * handle vmod internal state, vmod init/fini and/or varnish callback
 * (un)registration here.
 *
 * malloc'ing the info buffer is only indended as a demonstration, for any
 * real-world vmod, a fixed-sized buffer should be a global variable
 */

VCL_VOID
vmod_db(VRT_CTX, struct vmod_priv *priv, VCL_STRING server_url)
{
	struct vmod_mongodb_vcl_settings *settings;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CAST_OBJ_NOTNULL(settings, priv->priv, VMOD_MDB_SETTINGS_MAGIC);
	AZ(settings->pool);

	// size_t pool_len = strlen(server_url);
	// char *pool_str = malloc(pool_len + 1);

	// strcpy(pool_str, server_url);
	settings->uri = mongoc_uri_new(server_url);
	settings->pool = mongoc_client_pool_new(settings->uri);
	mongoc_client_pool_set_error_api(settings->pool, 2);
	settings->dbname = mongoc_uri_get_database(settings->uri);

	VSL(SLT_VCL_Log, 0, "Could not connect");
	// free(pool_str);
	return;
}

VCL_STRING
vmod_find(VRT_CTX, struct vmod_priv *priv, VCL_STRING collection_name, VCL_STRING query_string)
{

	mongoc_client_t *mc;
	struct vmod_mongodb_vcl_settings *settings;
	bson_t *query;
	char *p, *result;

	VSL(SLT_VCL_Log, 0, "Could not connect");

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CAST_OBJ_NOTNULL(settings, priv->priv, VMOD_MDB_SETTINGS_MAGIC);
	mc = get_client(ctx, settings);
	if (!mc) {
		VSL(SLT_VCL_Log, 0, "Could not get DB Conection");
		return (NULL);
	}

	query = bson_new_from_json((const uint8_t *)query_string, -1, NULL);
	if (!query) {
		VSL(SLT_VCL_Log, 0, "Could not connect not query");
		free_client(ctx, settings, mc);
		return (NULL);

	}

	mongoc_collection_t *collection = mongoc_client_get_collection (mc, settings->dbname, collection_name);
	if (!collection) {
			VSL(SLT_VCL_Log, 0, "Could not get collection");
			bson_destroy (query);
			free_client(ctx, settings, mc);
			 return (NULL);
	}
	mongoc_cursor_t *cursor = mongoc_collection_find_with_opts (collection, query, NULL, NULL);

	const bson_t *doc;
	if (mongoc_cursor_next (cursor, &doc)) {
		result = bson_as_canonical_extended_json (doc, NULL);
		bson_destroy (query);
		mongoc_cursor_destroy(cursor);
		mongoc_collection_destroy (collection);
		free_client(ctx, settings, mc);
		p = WS_Copy(ctx->ws, result, -1);
		bson_free (result);
		return (p);
	}

	VSL(SLT_VCL_Log, 0, "Could not get result");
	bson_destroy (query);
	mongoc_cursor_destroy(cursor);
	mongoc_collection_destroy (collection);
	free_client(ctx, settings, mc);

	return NULL;
}

VCL_BOOL
vmod_find_and_update(VRT_CTX, struct vmod_priv *priv, VCL_STRING collection_name, VCL_STRING query_string, VCL_STRING update_string)
{

	mongoc_client_t *mc;
	struct vmod_mongodb_vcl_settings *settings;
	bson_t *query;
	bson_t *update;

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CAST_OBJ_NOTNULL(settings, priv->priv, VMOD_MDB_SETTINGS_MAGIC);
	mc = get_client(ctx, settings);
	if (!mc) {
		VSL(SLT_VCL_Log, 0, "Could not get DB Conection");
		return 0;
	}

	query = bson_new_from_json((const uint8_t *)query_string, -1, NULL);
	if (!query) {
		VSL(SLT_VCL_Log, 0, "Could not make query");
		free_client(ctx, settings, mc);
		return 0;

	}

	update = bson_new_from_json((const uint8_t *)update_string, -1, NULL);
	if (!update) {
		bson_destroy (query);
		VSL(SLT_VCL_Log, 0, "Could not make update");
		free_client(ctx, settings, mc);
		return 0;

	}

	mongoc_collection_t *collection = mongoc_client_get_collection (mc, settings->dbname, collection_name);
	if (!collection) {
			VSL(SLT_VCL_Log, 0, "Could not get collection");
			bson_destroy (query);
			bson_destroy (update);
			free_client(ctx, settings, mc);
			return 0;
	}

	unsigned result = mongoc_collection_update_one (collection, query, update, NULL, NULL, NULL);

	bson_destroy (query);
	bson_destroy (update);
	mongoc_collection_destroy (collection);
	free_client(ctx, settings, mc);
	return result;
}

VCL_STRING
vmod_aggregate(VRT_CTX, struct vmod_priv *priv, VCL_STRING collection_name, VCL_STRING query_string)
{

	mongoc_client_t *mc;
	struct vmod_mongodb_vcl_settings *settings;
	bson_t *query;
	char *p, *result;

	VSL(SLT_VCL_Log, 0, "Could not connect");

	CHECK_OBJ_NOTNULL(ctx, VRT_CTX_MAGIC);
	CAST_OBJ_NOTNULL(settings, priv->priv, VMOD_MDB_SETTINGS_MAGIC);
	mc = get_client(ctx, settings);
	if (!mc) {
		VSL(SLT_VCL_Log, 0, "Could not get DB Conection");
		return (NULL);
	}

	query = bson_new_from_json((const uint8_t *)query_string, -1, NULL);
	if (!query) {
		VSL(SLT_VCL_Log, 0, "Could not connect to query");
		free_client(ctx, settings, mc);
		return (NULL);

	}

	mongoc_collection_t *collection = mongoc_client_get_collection (mc, settings->dbname, collection_name);
	if (!collection) {
			VSL(SLT_VCL_Log, 0, "Could not get collection");
			bson_destroy (query);
			free_client(ctx, settings, mc);
			 return (NULL);
	}

	mongoc_cursor_t *cursor = mongoc_collection_aggregate (collection, MONGOC_QUERY_NONE, query, NULL, NULL);

	const bson_t *doc;
	if (mongoc_cursor_next (cursor, &doc)) {
		result = bson_as_canonical_extended_json (doc, NULL);
		bson_destroy (query);
		mongoc_cursor_destroy(cursor);
		mongoc_collection_destroy (collection);
		free_client(ctx, settings, mc);
		p = WS_Copy(ctx->ws, result, -1);
		bson_free (result);
		return (p);
	}

	VSL(SLT_VCL_Log, 0, "Could not get result");
	bson_destroy (query);
	mongoc_cursor_destroy(cursor);
	mongoc_collection_destroy (collection);
	free_client(ctx, settings, mc);

	return NULL;
}