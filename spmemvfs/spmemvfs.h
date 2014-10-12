/*
 * Copyright 2009 Stephen Liu
 *
 */

#ifndef __spmemvfs_h__
#define __spmemvfs_h__

#ifdef __cplusplus
extern "C" {
#endif

#include "sqlite3.h"

#define SPMEMVFS_NAME "spmemvfs"

typedef struct spmembuffer_t {
	char * data;
	int used;
	int total;
} spmembuffer_t;

typedef struct spmemvfs_db_t {
	sqlite3 * handle;
	spmembuffer_t * mem;
} spmemvfs_db_t;

int spmemvfs_env_init();

void spmemvfs_env_fini();

int spmemvfs_open_db( spmemvfs_db_t * db, const char * path, spmembuffer_t * mem );

int spmemvfs_close_db( spmemvfs_db_t * db );

#ifdef __cplusplus
}
#endif

#endif

