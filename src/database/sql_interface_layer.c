/*
 * =====================================================================================
 *
 *       Filename:  sql_interface_layer.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/27/13 20:49:47
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  AlexsJones (), alexsimonjones@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "sql_interface_layer.h"
#include "sql_connector.h"
#include <jnxc_headers/jnxhash.h>
#include <stdarg.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

int sql_setup_credentials(void)
{

	char *host = jnx_hash_get(config,"SQLHOST");	
	printf("host %s\n",host);
	char *username = jnx_hash_get(config,"SQLUSER");
	char *password = jnx_hash_get(config,"SQLPASS");
	assert(host);
	assert(username);
	assert(password);
	perform_store_sql_credentials(host,username,password);
	return 0;
}
int sql_send_query(mysql_result_bucket **results_bucket, const char *querytemplate, ...)
{
	char constructed_query[1024];
	va_list ap;
	va_start(ap,querytemplate);
	vsprintf(constructed_query,querytemplate,ap);
	va_end(ap);
#ifdef DEBUG
	printf("Sending query -> %s\n",constructed_query);	
#endif
	return sql_query(constructed_query,results_bucket);
}
char **get_devices_to_probe(int *num_devices)
{
	char *ips[] = {
		"10.65.82.61",
		"10.65.82.62",
		"10.65.82.63",
		"10.65.82.64",
		"10.65.82.65",
		"10.65.82.66",
		"10.65.82.67",
		"10.65.82.68",
		"10.65.82.69",
		"10.65.82.70",
		"10.65.82.71",
		"10.65.82.72",
		"10.65.82.73",
		"10.65.82.74",
		"10.65.82.75",
		"10.65.82.76",
		"10.65.82.77",
		"10.65.82.78",
		"10.65.82.79",
		"10.65.82.80"
	};

	char **temp = malloc(20 * sizeof(char**));
	int i;
	*num_devices = 20;

	for (i = 0; i < *num_devices; i++)
	{
		temp[i] = malloc(strlen(ips[i]));
		strcpy(temp[i], ips[i]);
	}

	return temp;
}
