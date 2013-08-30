/*
 * =====================================================================================
 *
 *       Filename:  results_bucket.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/28/13 18:22:28
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __RESULTS_BUCKET_H__
#define __RESULTS_BUCKET_H__
#ifdef __APPLE__
#include <mysql.h>
#else 
#include <mysql/mysql.h>
#endif
#include <jnxc_headers/jnxhash.h>
typedef struct mysql_result_bucket
{
	int field_count;
	MYSQL_FIELD *fields;
	int row_count;
	MYSQL_ROW *rows;
	MYSQL_RES *result;
}mysql_result_bucket;

int get_mysql_result_bucket_field_position(mysql_result_bucket **result_bucket, char *field_name);
void add_mysql_result_bucket_entry(mysql_result_bucket **result_bucket,MYSQL_RES *result);
void remove_mysql_result_bucket(mysql_result_bucket **bucket);
#endif
