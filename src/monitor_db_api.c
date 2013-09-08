/*
 * =====================================================================================
 *
 *       Filename:  db_api.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/09/13 10:20:40
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <jnxc_headers/jnxhash.h>

#include "monitor_db_api.h"

#define GET_DEVICE_IPS "select id, ip from devices where ip is not null;"
#define GET_MACHINE_IPS "select id, ip from machines where ip is not null;"

static jnx_hashmap *ip_ids = NULL;

void free_ip_ids()
{
	int i, num_entries;
	const char **keys;

	if (ip_ids != NULL)
	{
		num_entries = jnx_hash_get_keys(ip_ids, &keys);

		for (i = 0; i < num_entries; i++)
		{
			free(jnx_hash_get(ip_ids, keys[i]));
			free((char *) keys[i]);
		}

		free(keys);
		jnx_hash_delete(ip_ids);
		ip_ids = NULL;
	}
}

void init_ip_ids()
{
	free_ip_ids();
	ip_ids = jnx_hash_init(1024);
}

char *copy_string(char *in)
{
	int len = strlen(in);
	char *out = calloc(len, sizeof(char));
	strncpy(out, in, len);

	return out;
}

void get_ip_ids_for_query(const char *query)
{
	char *ip, *id;
	int i, row_count;
	mysql_result_bucket *results;
	MYSQL_ROW *rows;

	sql_send_query(&results, GET_DEVICE_IPS);
	row_count = results->row_count;

	if (row_count > 0)
	{
		init_ip_ids();
		rows = results->rows;

		for(i = 0; i < row_count; i++)
		{
			id = copy_string(rows[i][0]);
			ip = copy_string(rows[i][1]);
			printf("id = %s; ip = %s\n", id, ip);
			jnx_hash_put(ip_ids, ip, (void *) id);	
		}
	}

	remove_mysql_result_bucket(&results);
}

char **get_devices_to_probe(int *num_devices)
{
	char **ips = NULL;
	const char **keys;
	int i;
	get_ip_ids_for_query(GET_DEVICE_IPS);

	*num_devices = jnx_hash_get_keys(ip_ids, &keys);
	ips = calloc(*num_devices,  sizeof(char**));
	
	for (i = 0; i < *num_devices; i++)
	{
		ips[i] = malloc(strlen(keys[i]));
		strcpy(ips[i], keys[i]);
	}

	free(keys);

	return ips;
}

char **get_machines_to_probe(int *num_machines)
{
	char **ips = NULL;
	const char **keys;
	int i;
	get_ip_ids_for_query(GET_MACHINE_IPS);

	*num_machines = jnx_hash_get_keys(ip_ids, &keys);
	ips = calloc(*num_machines,  sizeof(char**));
	
	for (i = 0; i < *num_machines; i++)
	{
		ips[i] = malloc(strlen(keys[i]));
		strcpy(ips[i], keys[i]);
	}

	free(keys);

	return ips;
}

void
update_non_responsive_devices(char **devices, int num_devices)
{
	int i;

	for (i = 0; i < num_devices; i++)
	{
		printf("IP %s is not responding.\n", devices[i]);
	}
}

