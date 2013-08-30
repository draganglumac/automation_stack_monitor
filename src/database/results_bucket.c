/*
 *
 * =====================================================================================
 *
 *       Filename:  results_bucket.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/28/13 18:27:14
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "results_bucket.h"
#include <stdio.h>
#include <string.h>
void add_mysql_result_bucket_entry(mysql_result_bucket **result_bucket,MYSQL_RES *result)
{
	int field_count = mysql_num_fields(result);
	int row_count = mysql_num_rows(result);
	MYSQL_FIELD *fields = mysql_fetch_fields(result); 

	MYSQL_ROW current_row;
	MYSQL_ROW *rows = calloc(row_count,sizeof(MYSQL_ROW));
	int counter = 0;
	while((current_row = mysql_fetch_row(result)))
	{
		rows[counter] = current_row;
		++counter;
	}
	mysql_result_bucket *bucket = malloc(sizeof(mysql_result_bucket));
	bucket->field_count = field_count;
	bucket->row_count = row_count;
	bucket->rows = rows;
	bucket->fields = fields;	
	bucket->result = result;
	(*result_bucket) = bucket;
}
void remove_mysql_result_bucket(mysql_result_bucket **bucket)
{
	mysql_free_result((*bucket)->result);
	free((*bucket)->rows);
	free((*bucket));
}
int get_mysql_result_bucket_field_position(mysql_result_bucket **result_bucket, char *field_name)
{
	if(*result_bucket == NULL || (*result_bucket)->fields == NULL)
	{
		return -1;
	}
	int x; 
	for(x = 0; x < (*result_bucket)->field_count; ++x)
	{
		char *current_field = (*result_bucket)->fields[x].name;
		if(strcmp(current_field,field_name) == 0)
		{
			return x;
		}
	}	
	return -1;
}
