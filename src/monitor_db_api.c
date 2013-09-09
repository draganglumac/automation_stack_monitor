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

#define GET_STATS_FOR_DEVICE "select * from device_stats where device_id = %s;"
#define GET_STATS_FOR_MACHINE "select * from machine_stats where machine_id = %s;"

#define INSERT_FIRST_SUCCESSFUL_DEVICE_STAT "insert into device_stats(device_id,polltime,ping_success,mac_address,firstpoll) values (%s,%s,true,'%s',%s);"
#define INSERT_FIRST_FAILED_DEVICE_STAT "insert into device_stats(device_id,polltime,ping_success,firstpoll) values (%s,%s,false,%s);"
#define INSERT_NEW_SUCCESSFUL_DEVICE_STAT "insert into device_stats(device_id,polltime,ping_success,mac_address) values (%s,%s,true,'%s');"
#define INSERT_NEW_FAILED_DEVICE_STAT "insert into device_stats(device_id,polltime,ping_success) values (%s,%s,false);"
#define UPDATE_DEVICE_STAT "update device_stats set polltime = %s where id = %s;"

#define INSERT_FIRST_SUCCESSFUL_MACHINE_STAT "insert into machine_stats(machine_id,polltime,ping_success,mac_address,firstpoll) values (%s,%s,true,'%s',%s);"
#define INSERT_FIRST_FAILED_MACHINE_STAT "insert into machine_stats(machine_id,polltime,ping_success,firstpoll) values (%s,%s,false,%s);"
#define INSERT_NEW_SUCCESSFUL_MACHINE_STAT "insert into machine_stats(machine_id,polltime,ping_success,mac_address) values (%s,%s,true,'%s');"
#define INSERT_NEW_FAILED_MACHINE_STAT "insert into machine_stats(machine_id,polltime,ping_success) values (%s,%s,false);"
#define UPDATE_MACHINE_STAT "update machine_stats set polltime = %s where id = %s;"

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
update_device_stats(time_t poll_time, jnx_hashmap *ip_macs, char **unresponsive, int num_unersponsive)
{
	const char **ips;
	int i, size;
	
	size = jnx_hash_get_keys(ip_macs, &ips);
	printf("Devices that responded with mac addresses:\n");
	for (i = 0; i < size; i++)
	{
		if (jnx_hash_get(ip_ids, ips[i]))
			printf("%02s -> MAC[%s]=%s\n", jnx_hash_get(ip_ids, ips[i]), ips[i], jnx_hash_get(ip_macs, ips[i]));
	}

	printf("\n");
	printf("Devices that did not respond during probing cycle:\n");
	for (i = 0; i < num_unersponsive; i++)
		printf("%02s -> %s\n", jnx_hash_get(ip_ids, unresponsive[i]), unresponsive[i]);
}
