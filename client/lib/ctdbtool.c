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


#include "ctdbapi.h"
#include <stdio.h>
#include <string.h>

void usage(char *msg)
{
    if (msg) {
        fprintf(stderr, msg);
    }
        
    fprintf(stderr,
            "usage: ctdbtool <database> <op> [<key> [<value>]]\n"
            "       op: fetch, store, delete, dump\n");
}

static int traverse_func(const char *key, size_t keylen,
                         const char *data, size_t datalen,
                         void *private_data)
{
    printf("key(%s) => data(%s)\n", key, data);
    return 0;
}

static struct timeval timeval_current(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv;
}

static double timeval_elapsed(struct timeval *tv)
{
	struct timeval tv2 = timeval_current();
	return (tv2.tv_sec - tv->tv_sec) + 
	       (tv2.tv_usec - tv->tv_usec)*1.0e-6;
}

static int testwrite(CTDB_HANDLE ch, int numrecs)
{
    int i, rc;
    char dbuf[80];
    struct timeval tv;

    printf("Storing %d records\n");
    tv = timeval_current();

    for (i = 0; i < numrecs; i++) {
        sprintf(dbuf, "data%d", i);

        rc = ctdbapi_store(ch, (const char*)&i, sizeof(i),
                           dbuf, strlen(dbuf)+1);
        if (rc != 0) {
            printf("ctdbapi_store(%d) failed: rc = %d\n", i, rc);
            exit(1);
        }
        if (i > 0 && i % 10000 == 0) {
            printf("%d records stored: %lfs\n", i, timeval_elapsed(&tv));
            tv = timeval_current();
        }
    }
    return 0;
}

static int testread(CTDB_HANDLE ch, int numrecs)
{
    int i, rc;
    char dbuf[80];
    struct timeval tv;

    printf("Fetching %d records\n");
    tv = timeval_current();

    for (i = 0; i < numrecs; i++) {
        size_t len = sizeof(dbuf);
        rc = ctdbapi_fetch(ch, (const char*)&i, sizeof(i),
                           dbuf, &len);
        if (rc != 0) {
            printf("ctdbapi_store(%d) failed: rc = %d\n", i, rc);
            exit(1);
        }
        if (i > 0 && i % 10000 == 0) {
            printf("%d records read: %lfs\n", i, timeval_elapsed(&tv));
            tv = timeval_current();
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    CTDB_HANDLE ch;
    char *db;
    char *op;
    char *key;
    char *data;
    int rc;
    size_t len;
    char buf[4096];

	if (argc < 3) {
        usage(0);
		exit(1);
	}

	db = argv[1];
	op = argv[2];

    rc = ctdbapi_attach(&ch, db, CTDBAPI_FLAGS_USEMUTEX, stderr);

    if (rc != 0) {
        printf("ctdbapi_attach failed to attach to db %s: rc = %d\n", 
               db, rc);
        exit(1);
    }

    if (strcasecmp(op, "fetch") == 0) {
        if (argc < 4) {
            usage("key required for fetch operation\n");
            exit(1);
        }
        key = argv[3];
        len = sizeof(buf);
        rc = ctdbapi_fetch(ch, key, strlen(key)+1,
                           buf, &len);
        
        if (rc != 0) {
            printf("ctdbapi_fetch(%s) failed: rc = %d\n", key, rc);
            exit(1);
        }
        printf("key(%s) => value(%s)\n", key, buf);
        exit(0);
    }

    if (strcasecmp(op, "store") == 0) {
        if (argc < 5) {
            usage("key and value required for store operation\n");
            exit(1);
        }
        key = argv[3];
        data = argv[4];
        rc = ctdbapi_store(ch, key, strlen(key)+1,
                           data, strlen(data)+1);
        
        if (rc != 0) {
            printf("ctdbapi_store(%s) failed: rc = %d\n", key, rc);
            exit(1);
        }
        printf("stored key(%s) with value(%s)\n", key, data);
        exit(0);
    }

    if (strcasecmp(op, "delete") == 0) {
        if (argc < 4) {
            usage("key required for delete operation\n");
            exit(1);
        }
        key = argv[3];
        len = 0;
        rc = ctdbapi_store(ch, key, strlen(key)+1,
                           data, len);
        
        if (rc != 0) {
            printf("ctdbapi_delete(%s) failed: rc = %d\n", key, rc);
            exit(1);
        }
        printf("deleted key(%s)\n", key);
        exit(0);
    }

    if (strcasecmp(op, "dump") == 0) {
        rc = ctdbapi_traverse(ch, traverse_func, 0);
        
        printf("%d records processed\n", rc);
        exit(0);
    }

    if (strcasecmp(op, "testwrite") == 0) {
        int numrecs = 0;
        if (argc < 4) {
            usage("testwrite requires numrec parameter\n");
            exit(1);
        }
        numrecs = atoi(argv[3]);
        if (numrecs <= 0) {
            usage("testwrite requires numrec parameter > 0\n");
            exit(1);
        }
            
        testwrite(ch, numrecs);
        exit(0);
    }

    if (strcasecmp(op, "testread") == 0) {
        int numrecs = 0;
        if (argc < 4) {
            usage("testread requires numrec parameter\n");
            exit(1);
        }
        numrecs = atoi(argv[3]);
        if (numrecs <= 0) {
            usage("testread requires numrec parameter > 0\n");
            exit(1);
        }
            
        testread(ch, numrecs);
        exit(0);
    }
    return 0;
}
