/*
* BSD 2-Clause License
*
* Copyright 2018 Joel Stienlet
* Copyright 2018 Stephen Liu
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

/*===========================================================================*/

/*
 
 The aim is to test the response of the lib when opening multiple dbs at the same time.
 Checks that the linked-list in g_spmemvfs_env is working as expected under "heavy load".
 
 We also check for memory leaks: valgrind -v --leak-check=yes ./testmemvfsmulti
 ( set NBER_OUTER_LOOP to 1 to speed-up the test with valgring )
 No memory leaks found.
 
 */

#include <assert.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <malloc.h>

#include "spmemvfs.h"

/*===========================================================================*/

/*    Test Parameters    */

/* max nber of dbs opened at the same time */
#define N_DBS 1000

/* number of iterations in outer loop */
#define NBER_OUTER_LOOP 10
/* number of iterations in inner loop. Should be >> N_DBS */
#define NBER_INNER_LOOP 10000


/*===========================================================================*/

/* max size of some strings */
#define MAX_NAME_LEN 32
#define MAX_SQL_LEN 100

/* doing some paranoid checks in the test */
#define CANARY 541088524

/*===========================================================================*/

struct one_db_data_str
{
  uint32_t start_canary;
  int is_opened;
  char db_name[MAX_NAME_LEN];
  spmemvfs_db_t db;
  spmembuffer_t * mem;
};

struct test_data_str
{
    struct one_db_data_str *tab_dbs;
    int nber_dbs;
};

/* to track memery leaks */
struct mallinfo malloc_info_start; 

/*===========================================================================*/

int init_test(struct test_data_str *p_data)
{
    int i;
    
    if(NBER_INNER_LOOP < 10 * N_DBS){
        printf("WARNING: not enough iterations in inner loop to make many changes in each db...\n");
    }
    
    p_data -> nber_dbs = N_DBS;
    p_data -> tab_dbs = calloc(p_data -> nber_dbs, sizeof(struct one_db_data_str) );
    assert( NULL != p_data -> tab_dbs );
    
    for(i=0; i< p_data -> nber_dbs; i++){
       p_data -> tab_dbs[i].start_canary = CANARY;
        
       snprintf(p_data -> tab_dbs[i].db_name, MAX_NAME_LEN, "éàü€_%i", i); /* add some UTF-8 characters */
       
      }
    
    return 0;
}

int close_test(struct test_data_str *p_data)
{
    int i;
    
    for(i=0; i< p_data -> nber_dbs; i++){
        assert(NULL == p_data -> tab_dbs[i].mem);
    }
    
    free(p_data -> tab_dbs);
    p_data -> tab_dbs = NULL;
    p_data -> nber_dbs = 0;
    
    return 0;
}

/* clone one db of the many opened. */
int close_db(struct one_db_data_str *p_db)
{
    int ret;
    
    assert(p_db -> start_canary == CANARY);
    assert( 0 != p_db -> is_opened );

    ret = spmemvfs_close_db( &(p_db -> db) );
    if(ret != SQLITE_OK){fprintf(stderr, "ERROR: F:%s L:%d\n", __FILE__, __LINE__); return 1;}
    
    assert( NULL == p_db -> db.mem );
    assert( NULL == p_db -> db.handle );
    assert( NULL == p_db -> mem );
    
    p_db -> is_opened = 0;
    
    return 0;
}

int make_table(struct one_db_data_str *p_db)
{
    int errcode;
    const char * sql;
    
    sql = "CREATE TABLE user ( name, age )";
 	errcode = sqlite3_exec( p_db -> db.handle, sql, NULL, NULL, NULL );
    if(SQLITE_OK != errcode) {fprintf(stderr, "ERROR: code=%d F:%s L:%d\n", errcode, __FILE__, __LINE__); return 1;}
    
    return 0;
}

int insert_in_table(struct one_db_data_str *p_db)
{
    int errcode;
    char sql[MAX_SQL_LEN];
    
	snprintf(sql, MAX_SQL_LEN, "insert into user values ( 'abc', %d );", (int)(100. * drand48()) );
    /* printf("%s\n", sql); */
	errcode = sqlite3_exec( p_db -> db.handle, sql, 0, 0, 0 );
    if(SQLITE_OK != errcode) {fprintf(stderr, "ERROR: code=%d F:%s L:%d\n", errcode, __FILE__, __LINE__); return 1;}
    
    return 0;
}

int open_db(struct one_db_data_str *p_db)
{
    int ret;
    
    assert(p_db -> start_canary == CANARY);

    assert( 0 == p_db -> is_opened );
    assert( NULL == p_db -> mem );
    
    p_db ->mem = calloc( 1, sizeof( spmembuffer_t ) );
    assert(NULL != p_db ->mem);
    
    ret = spmemvfs_open_db( &(p_db -> db), p_db -> db_name, p_db -> mem );
    if(ret != SQLITE_OK){fprintf(stderr, "ERROR: F:%s L:%d\n", __FILE__, __LINE__); return 1;}
    
    p_db -> mem = NULL; /* memory owned by spmemvfs now, but remains accessible until we close the db. 
                           here we set it to NULL to indicate that we don't care about the data. */
    
    ret = make_table(p_db);
    if(ret != 0){fprintf(stderr, "ERROR: F:%s L:%d\n", __FILE__, __LINE__); return 1;}
    
    p_db -> is_opened = 1;
    
    return 0;
}


int check_canaries(struct test_data_str *p_data)
{
    int m;
    
    for(m=0; m< p_data -> nber_dbs; m++){
        struct one_db_data_str *p_db = p_data -> tab_dbs + m;
        assert(p_db -> start_canary == CANARY);
    }
    
    return 0;
}


int test_random_insert_del(struct test_data_str *p_data)
{
int ind;
int ret;
int i,k,m;
struct mallinfo malloc_info;

for(i=0; i< NBER_OUTER_LOOP; i++){
    printf("iteration %d.\n", i);
    /* make changes: create or delete dbs, insert/remove elements... */
    for(k=0; k< NBER_INNER_LOOP; k++){
        /* check_canaries(p_data); */ /* paranoid mode */
        
        ind = (p_data -> nber_dbs) * drand48();
        assert(ind >= 0);
        assert(ind < p_data -> nber_dbs);
        
        if(p_data -> tab_dbs[ind].is_opened){
            /* already opened: either close it or remove / insert items */
      
           if(drand48() > 0.2){
                 ret = insert_in_table(p_data -> tab_dbs + ind);
                 if(ret != 0) {fprintf(stderr, "ERROR: F:%s L:%d\n", __FILE__, __LINE__); return 1;}
           } else {
                 ret = close_db(p_data -> tab_dbs + ind);
                 if(ret != 0) {fprintf(stderr, "ERROR: F:%s L:%d\n", __FILE__, __LINE__); return 1;}
           }
            
        } else {
            /* not open yet: open it! */
	         ret = open_db( p_data -> tab_dbs +ind );
             if(ret != SQLITE_OK){fprintf(stderr, "ERROR: F:%s L:%d\n", __FILE__, __LINE__); return 1;}
        }
    }
    
    /* delete all remaining dbs: test the empty linked-list case. */
    for(m=0; m< p_data -> nber_dbs; m++){
        if( 0 != p_data -> tab_dbs[m].is_opened ){
            ret = close_db(p_data -> tab_dbs + m);
            if(ret != 0) {fprintf(stderr, "ERROR: F:%s L:%d\n", __FILE__, __LINE__); return 1;}
        }
        
    }
    
    malloc_info = mallinfo();
    printf("%d delta-allocated bytes\n", malloc_info.uordblks - malloc_info_start.uordblks);
}
    
return 0;
}

/*===========================================================================*/

int main( int argc, char * argv[] )
{
    int ret;
    struct test_data_str test_data;
    struct mallinfo malloc_info;
    
    malloc_info_start = mallinfo();
    
    ret = init_test(&test_data);
    if(ret != 0) {fprintf(stderr, "ERROR: F:%s L:%d\n", __FILE__, __LINE__); return 1;}
    
    spmemvfs_env_init();
    
    printf("now perform random insertions/deletions...\n");
    
    ret = test_random_insert_del(&test_data);
    if(ret != 0) {fprintf(stderr, "ERROR: F:%s L:%d\n", __FILE__, __LINE__); return 1;}
    
    spmemvfs_env_fini();
    
    ret = close_test(&test_data);
    if(ret != 0) {fprintf(stderr, "ERROR: F:%s L:%d\n", __FILE__, __LINE__); return 1;}
    
    malloc_info = mallinfo();
    printf("%d delta-allocated bytes final.\n", malloc_info.uordblks - malloc_info_start.uordblks);
    
    printf("test OK.\n");
    
 return 0;   
}

/*===========================================================================*/

