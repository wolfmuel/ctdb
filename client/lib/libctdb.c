/*
 *  CTDB Client API
 *
 * Copyright (C) Wolfgang Mueller-Friedt  2010
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#include "includes.h"
#include "lib/events/events.h"
#include "client/lib/ctdbapi.h"
#include "../include/ctdb_private.h"

#include <sys/time.h>
#include <time.h>
#include <pthread.h>

typedef struct ctdbapi_handle {
    TALLOC_CTX *ctx;
	struct ctdb_context *ctdb;
	struct ctdb_db_context *ctdb_db;
	struct event_context *ev;    
    pthread_mutex_t mutex;        
    FILE *logf;
    int flags;
} ctdb_handle_t;


/*
 * attach to ctdb managed tdb
 * if tdb doesn't exist it will be created
 */
int ctdbapi_attach(CTDB_HANDLE *ch, const char *dbname, int flags, FILE *logf)
{
    int rc;
	TALLOC_CTX *ctx = talloc_new(NULL);
    ctdb_handle_t *h = talloc_zero_size(ctx, sizeof(struct ctdbapi_handle));
    *ch = (CTDB_HANDLE)h;
    h->ctx = ctx;
    h->ev = event_context_init(NULL);
	h->ctdb = ctdb_cmdline_client(h->ev);
    h->flags = flags;

	/* 
     * attach to non-persistent database 
     */
	h->ctdb_db = ctdb_attach(h->ctdb, dbname, false, 
                             TDB_DEFAULT | TDB_CLEAR_IF_FIRST);
	if (!h->ctdb_db) {
        if (h->logf) {
            fprintf(h->logf, __location__
                    "ctdb_attach failed - %s\n", ctdb_errstr(h->ctdb));
        }
        talloc_free(ctx);
        return 1;
	}

    if (h->flags & CTDBAPI_FLAGS_USEMUTEX) {
        rc = pthread_mutex_init(&h->mutex, 0);
        if (rc != 0) {
            if (h->logf) {
                fprintf(h->logf, __location__ 
                        "pthread_mutex_init failed with rc = %d\n", rc);
            }
            talloc_free(ctx);
            return rc;
        }
    }
    return 0;
}    

/*
 * store data associated with key in ctdb
 */
int ctdbapi_store(CTDB_HANDLE ch, const char *key, int keylen,
                  const char *data, size_t datalen)
{
    int ret, rc;
	TDB_DATA tkey, tdata;
 	struct ctdb_record_handle *rec;
    ctdb_handle_t *h = (ctdb_handle_t*)ch;

    if (h == 0 || key == 0 || data == 0) {
        return 1;
    }
    
    if (h->flags & CTDBAPI_FLAGS_USEMUTEX) {
        rc = pthread_mutex_lock(&h->mutex);
        if (rc != 0) {
            if (h->logf) {
                fprintf(h->logf, __location__
                        "pthread_mutex_lock failed with rc = %d\n", rc);
            }
            return rc;
        }
    }
	tkey.dptr = discard_const(key);
	tkey.dsize = keylen;

    rec = ctdb_fetch_lock(h->ctdb_db, h->ctx, tkey, &tdata);
	if (rec == NULL) {
		talloc_free(rec);
        if (h->flags & CTDBAPI_FLAGS_USEMUTEX) {
            pthread_mutex_unlock(&h->mutex);
        }
        if (h->logf) {
            fprintf(h->logf, __location__
                    "ctdb_fetch_lock failed\n");
        }
		return 2;
	}

    talloc_free(tdata.dptr);
    tdata.dptr = discard_const(data);
    tdata.dsize = datalen;
    
    ret = ctdb_record_store(rec, tdata);
    talloc_free(rec);
    if (h->flags & CTDBAPI_FLAGS_USEMUTEX) {
        pthread_mutex_unlock(&h->mutex);
    }
    if (ret != 0) {
        if (h->logf) {
            fprintf(h->logf, __location__
                    "ctdb_record_store failed with rc = %d\n", ret);
        }
        return ret;
    }
    return 0;
}

/*
 * fetch data associated with key from ctdb
 * if record cannot be found datalen will be 0 on return
 */
int ctdbapi_fetch(CTDB_HANDLE ch, const char *key, int keylen,
                  char *data, size_t *datalen)
{
    int rc;
	TDB_DATA tkey, tdata;
    size_t size;
	struct ctdb_record_handle *rec;
    ctdb_handle_t *h = (ctdb_handle_t*)ch;

    if (h == 0 || key == 0 || data == 0) {
        return 1;
    }

    if (h->flags & CTDBAPI_FLAGS_USEMUTEX) {
        rc = pthread_mutex_lock(&h->mutex);
        if (rc != 0) {
            if (h->logf) {
                fprintf(h->logf, __location__
                        "pthread_mutex_lock failed with rc = %d\n", rc);
            }
            return rc;
        }
    }

	tkey.dptr = discard_const(key);
	tkey.dsize = keylen;

    rec = ctdb_fetch_lock(h->ctdb_db, h->ctx, tkey, &tdata);
	if (rec == NULL) {
		talloc_free(rec);
        if (h->flags & CTDBAPI_FLAGS_USEMUTEX) {
            pthread_mutex_unlock(&h->mutex);
        }
        if (h->logf) {
            fprintf(h->logf, __location__
                    "ctdb_fetch_lock failed\n");
        }
		return 1;
	}
    size = tdata.dsize > *datalen ? *datalen: tdata.dsize;
    memcpy(data, tdata.dptr, size);

    *datalen = size;

    talloc_free(tdata.dptr);
    talloc_free(rec);
    pthread_mutex_unlock(&h->mutex);

    return 0;
}

/*
 * delete record associated with key from ctdb
 */
int ctdbapi_delete(CTDB_HANDLE ch, const char *key, int keylen)
{
    int ret, rc;
	TDB_DATA tkey, tdata;
	struct ctdb_record_handle *rec;
    ctdb_handle_t *h = (ctdb_handle_t*)ch;

    if (h == 0 || key == 0) {
        return 1;
    }

    if (h->flags & CTDBAPI_FLAGS_USEMUTEX) {
        rc = pthread_mutex_lock(&h->mutex);
        if (rc != 0) {
            if (h->logf) {
                fprintf(h->logf, __location__
                        "pthread_mutex_lock failed with rc = %d\n", rc);
            }
            return rc;
        }
    }
	tkey.dptr = discard_const(key);
	tkey.dsize = keylen;

    rec = ctdb_fetch_lock(h->ctdb_db, h->ctx, tkey, &tdata);
	if (rec == NULL) {
		talloc_free(rec);
        if (h->flags & CTDBAPI_FLAGS_USEMUTEX) {
            pthread_mutex_unlock(&h->mutex);
        }
        if (h->logf) {
            fprintf(h->logf, __location__
                    "ctdb_fetch_lock failed\n");
        }
		return 2;
	}

    tdata.dsize = 0;
    
    ret = ctdb_record_store(rec, tdata);
    talloc_free(tdata.dptr);
    talloc_free(rec);

    if (h->flags & CTDBAPI_FLAGS_USEMUTEX) {
        pthread_mutex_unlock(&h->mutex);
    }
    if (ret != 0) {
        if (h->logf) {
            fprintf(h->logf, __location__
                    "ctdb_record_store failed with rc = %d\n", ret);
        }
        return ret;
    }
    return 0;
}

struct traverse_data {
    ctdbapi_traverse_func fn;
    void *pdata;
};

static int traverse(struct ctdb_context *ctdb, TDB_DATA key, TDB_DATA data, 
                    void *private_data)
{
    struct traverse_data *td = (struct traverse_data *)private_data;

    return td->fn((const char*)key.dptr, key.dsize, 
                  (const char*)(data.dptr + sizeof(struct ctdb_ltdb_header)), 
                  data.dsize - sizeof(struct ctdb_ltdb_header), td->pdata);
}

/*
 * traverse all records in a tdb and call fn for every record
 */
int ctdbapi_traverse(CTDB_HANDLE ch, ctdbapi_traverse_func fn, void *private_data)
{
    int rc;
    struct traverse_data td;
    ctdb_handle_t *h = (ctdb_handle_t*)ch;

    if (h == 0 || fn == 0) {
        return 1;
    }

    if (h->flags & CTDBAPI_FLAGS_USEMUTEX) {
        rc = pthread_mutex_lock(&h->mutex);
        if (rc != 0) {
            if (h->logf) {
                fprintf(h->logf, __location__ 
                        "pthread_mutex_lock failed with rc = %d\n", rc);
            }
            return 1;
        }
    }

    td.fn = fn;
    td.pdata = private_data;

    rc = ctdb_traverse(h->ctdb_db, traverse, &td);

    if (h->flags & CTDBAPI_FLAGS_USEMUTEX) {
        pthread_mutex_unlock(&h->mutex);
    }
    return rc;
}

