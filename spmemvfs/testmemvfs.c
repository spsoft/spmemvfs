/*
* BSD 2-Clause License
*
* Copyright 2009 Stephen Liu
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <assert.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "spmemvfs.h"

void test( spmemvfs_db_t * db )
{
	char errcode = 0;
	int i = 0;
	int count = 0;
	sqlite3_stmt * stmt = NULL;
	const char * sql = NULL;

	errcode = sqlite3_exec( db->handle,
		"CREATE TABLE user ( name, age )", NULL, NULL, NULL );

	printf( "sqlite3_exec %d\n", errcode );

	sql = "insert into user values ( 'abc', 12 );";
	errcode = sqlite3_exec( db->handle, sql, 0, 0, 0 );

	count = sqlite3_changes( db->handle );
	printf( "sqlite3_changes %d\n", count );

	sql = "select * from user;";
	errcode = sqlite3_prepare( db->handle, sql, strlen( sql ), &stmt, NULL );

	printf( "sqlite3_prepare %d, stmt %p\n", errcode, stmt );

	count = sqlite3_column_count( stmt );

	printf( "column.count %d\n", count );

	for( i = 0; i < count; i++ ) {
		const char * name = sqlite3_column_name( stmt, i );
		printf( "\t%s", name );
	}

	printf( "\n" );

	for( ; ; ) {
		errcode = sqlite3_step( stmt );

		if( SQLITE_ROW != errcode ) break;

		for( i = 0; i < count; i++ ) {
			unsigned const char * value = sqlite3_column_text( stmt, i );

			printf( "\t%s", value );
		}

		printf( "\n" );
	}

	errcode = sqlite3_finalize( stmt );
}

int readFile( const char * path, spmembuffer_t * mem )
{
	int ret = -1;

	FILE * fp = fopen( path, "r" );
	if( NULL != fp ) {
		struct stat filestat;
		if( 0 == stat( path, &filestat ) ) {
			ret = 0;

			mem->total = mem->used = filestat.st_size;
			mem->data = (char*)malloc( filestat.st_size + 1 );
			fread( mem->data, filestat.st_size, 1, fp );
			(mem->data)[ filestat.st_size ] = '\0';
		} else {
			printf( "cannot stat file %s\n", path );
		}
		fclose( fp );
	} else {
		printf( "cannot open file %s\n", path );
	}

	return ret;
}

int writeFile( const char * path, spmembuffer_t * mem )
{
	int ret = -1;

	FILE * fp = fopen( path, "w" );
	if( NULL != fp ) {
		ret = 0;
		fwrite( mem->data, mem->used, 1, fp );
		fclose( fp );
	}

	return ret;
}

int main( int argc, char * argv[] )
{
	const char * path = "abc.db";

	spmemvfs_db_t db;

	spmembuffer_t * mem = (spmembuffer_t*)calloc( sizeof( spmembuffer_t ), 1 );

	spmemvfs_env_init();

	readFile( path, mem );
	spmemvfs_open_db( &db, path, mem );

	assert( db.mem == mem );

	test( &db );

	writeFile( path, db.mem );

	spmemvfs_close_db( &db );

	spmemvfs_env_fini();

	return 0;
}

