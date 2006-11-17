/* 
   ctdb database library

   Copyright (C) Andrew Tridgell  2006

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


/*
  structure passed to a ctdb call function
*/
struct ctdb_call {
	TDB_DATA key;          /* record key */
	TDB_DATA record_data;  /* current data in the record */
	TDB_DATA *new_data;    /* optionally updated record data */
	TDB_DATA *call_data;   /* optionally passed from caller */
	TDB_DATA *reply_data;  /* optionally returned by function */
};

#define CTDB_ERR_INVALID 1
#define CTDB_ERR_NOMEM 2


/*
  initialise ctdb subsystem
*/
struct ctdb_context *ctdb_init(TALLOC_CTX *mem_ctx);

/*
  tell ctdb what nodes are available. This takes a filename, which will contain
  1 node address per line, in a transport specific format
*/
int ctdb_set_nlist(struct ctdb_context *ctdb, const char *nlist);


/*
  error string for last ctdb error
*/
const char *ctdb_errstr(struct ctdb_context *);

/* a ctdb call function */
typedef int (*ctdb_fn_t)(struct ctdb_call *);

/*
  setup a ctdb call function
*/
int ctdb_set_call(struct ctdb_context *ctdb, ctdb_fn_t fn, int id);

/*
  attach to a ctdb database
*/
int ctdb_attach(struct ctdb_context *ctdb, const char *name, int tdb_flags, 
		int open_flags, mode_t mode);


/*
  make a ctdb call. The associated ctdb call function will be called on the DMASTER
  for the given record
*/
int ctdb_call(struct ctdb_context *ctdb, TDB_DATA key, int call_id, 
	      TDB_DATA *call_data, TDB_DATA *reply_data);
