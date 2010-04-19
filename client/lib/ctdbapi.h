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

#include <stdlib.h>
#include <stdio.h>

/*
 * use a mutex to guarantee thread-safeness
 */
#define CTDBAPI_FLAGS_USEMUTEX  0x00000001

typedef struct opaque_type *CTDB_HANDLE;

/*
 * attach to ctdb managed tdb
 * if tdb doesn't exist it will be created
 */
int ctdbapi_attach(CTDB_HANDLE *ch, const char *dbname, 
                   int flags, FILE *f);

/*
 * store data associated with key in ctdb
 */
int ctdbapi_store(CTDB_HANDLE ch, const char *key, int keylen,
                  const char *data, size_t datalen);

/*
 * fetch data associated with key from ctdb
 * if record cannot be found datalen will be 0 on return
 */
int ctdbapi_fetch(CTDB_HANDLE ch, const char *key, int keylen,
                  char *data, size_t *datalen);

/*
 * delete record associated with key from ctdb
 */
int ctdbapi_delete(CTDB_HANDLE ch, const char *key, int keylen);

typedef int (*ctdbapi_traverse_func)(const char *key, size_t keylen,
                                     const char *data, size_t datalen,
                                     void *private_data);

/*
 * traverse all records in a tdb and call fn for every record
 */
int ctdbapi_traverse(CTDB_HANDLE ch, ctdbapi_traverse_func fn, 
                     void *private_data);

