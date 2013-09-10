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
#include <time.h>
#include <jnxc_headers/jnxhash.h>
#include <jnxc_headers/jnxstring.h>
#include <jnxc_headers/jnxterm.h>

#include "monitor_db_api.h"

#define GET_DEVICE_IPS "select id, ip from devices where ip is not null;"
#define GET_MACHINE_IPS "select id, ip from machines where ip is not null;"
#define GET_DEVICE_INFO "select d.name, m.call_sign as machine from devices d, machines m, connected_devices cd where d.id = %s and d.id = cd.device_id and m.id = cd.machine_id;"

#define GET_STATS_FOR_DEVICE "select * from device_stats where device_id = %s;"
#define GET_STATS_FOR_MACHINE "select * from machine_stats where machine_id = %s;"

#define INSERT_FIRST_SUCCESSFUL_DEVICE_STAT "insert into device_stats(device_id,polltime,ping_success,mac_address,firstpoll) values (%s,%s,true,'%s',%s);"
#define INSERT_FIRST_FAILED_DEVICE_STAT "insert into device_stats(device_id,polltime,ping_success,firstpoll) values (%s,%s,false,%s);"
#define INSERT_NEW_SUCCESSFUL_DEVICE_STAT "insert into device_stats(device_id,polltime,ping_success,mac_address) values (%s,%s,true,'%s');"
#define INSERT_NEW_FAILED_DEVICE_STAT "insert into device_stats(device_id,polltime,ping_success) values (%s,%s,false);"
#define UPDATE_DEVICE_STAT "update device_stats set polltime = %s where id = %s;"
#define UPDATE_DEVICE_STAT_MAC "update device_stats set polltime = %s, mac_address = '%s' where id = %s;"

#define INSERT_FIRST_SUCCESSFUL_MACHINE_STAT "insert into machine_stats(machine_id,polltime,ping_success,mac_address,firstpoll) values (%s,%s,true,'%s',%s);"
#define INSERT_FIRST_FAILED_MACHINE_STAT "insert into machine_stats(machine_id,polltime,ping_success,firstpoll) values (%s,%s,false,%s);"
#define INSERT_NEW_SUCCESSFUL_MACHINE_STAT "insert into machine_stats(machine_id,polltime,ping_success,mac_address) values (%s,%s,true,'%s');"
#define INSERT_NEW_FAILED_MACHINE_STAT "insert into machine_stats(machine_id,polltime,ping_success) values (%s,%s,false);"
#define UPDATE_MACHINE_STAT "update machine_stats set polltime = %s where id = %s;"
#define UPDATE_MACHINE_STAT_MAC "update machine_stats set polltime = %s, mac_address = '%s' where id = %s;"

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
			free(jnx_hash_delete_value(ip_ids, (char*) keys[i]));
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
			ip = rows[i][1];
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

int
get_last_stat_for_device(char *device_id, char **id, int *ping_success)
{
	char *ip;
	int i, row_count;
	mysql_result_bucket *results;
	MYSQL_ROW *rows;

	sql_send_query(&results, GET_STATS_FOR_DEVICE, device_id);
	row_count = results->row_count;
	rows = results->rows;

	if (row_count == 0)
		return 0;

	*id = copy_string(rows[row_count - 1][0]);
	*ping_success = atoi(rows[row_count - 1][3]);
	
	remove_mysql_result_bucket(&results);	
	
	return row_count;
}

void
update_device_stats(time_t poll_time, jnx_hashmap *ip_macs, char **unresponsive, int num_unersponsive)
{
	const char **ips;
	int i, hr, size, ping_success;
	char *stat_id, *device_id, *mac_address;
	mysql_result_bucket *results;
	
	size = jnx_hash_get_keys(ip_macs, &ips);
	printf("\033[2J");
	printf("\033[0;0H");
	char *datetime = ctime(&poll_time);
	printf("Last probed on %s\n", datetime);
	printf("Devices that responded with mac addresses:\n");
	printf("\n");
	jnx_term_printf_in_color(JNX_COL_GREEN, "id | device_name                    | node_name  | ip_address  | mac_address       |\n");
	jnx_term_printf_in_color(JNX_COL_GREEN, "---+--------------------------------+------------+-------------+-------------------+\n");
	for (i = 0, hr = 0; i < size; i++, hr++)
	{
		device_id = jnx_hash_get(ip_ids, ips[i]);
		if (device_id)
		{
			char *pt = jnx_string_itos(poll_time);
			mac_address = (char *) jnx_hash_get(ip_macs, ips[i]);
			
			sql_send_query(&results, GET_DEVICE_INFO, device_id);
			char *device_name = results->rows[0][0];
			char *node_name = results->rows[0][1];
			jnx_term_printf_in_color(JNX_COL_GREEN, "%02s | %-30s | %-10s | %s | %s |\n", device_id, device_name, node_name, ips[i], jnx_hash_get(ip_macs, ips[i]));
			remove_mysql_result_bucket(&results);
			if (hr > 0 && (hr % 4 == 0))
				printf("---+--------------------------------+------------+-------------+-------------------+\n");

			if (0 < get_last_stat_for_device(device_id, &stat_id, &ping_success))
			{
				if (ping_success)
					sql_send_query(&results, UPDATE_DEVICE_STAT_MAC, pt, mac_address, stat_id);
				else
					sql_send_query(&results, INSERT_NEW_SUCCESSFUL_DEVICE_STAT, device_id, pt, mac_address);

				free(stat_id);	
			}
			else
			{
				sql_send_query(&results, INSERT_FIRST_SUCCESSFUL_DEVICE_STAT, device_id, pt, mac_address, pt);
			}

			free(pt);
		}
	}
	free(ips);

	printf("\n");
	printf("Devices that did not respond during probing cycle:\n");
	printf("\n");
	jnx_term_printf_in_color(JNX_COL_RED, "id | device_name                    | node_name  | ip_address  |\n");
	jnx_term_printf_in_color(JNX_COL_RED, "---+--------------------------------+------------+-------------+\n");	
	for (i = 0; i < num_unersponsive; i++)
	{
		device_id = (char *) jnx_hash_get(ip_ids, unresponsive[i]);
		if (device_id)
		{
			char *pt = jnx_string_itos(poll_time);
			
			sql_send_query(&results, GET_DEVICE_INFO, device_id);
			char *device_name = results->rows[0][0];
			char *node_name = results->rows[0][1];
			jnx_term_printf_in_color(JNX_COL_RED, "%02s | %-30s | %-10s | %s |\n", device_id, device_name, node_name, unresponsive[i]);
			remove_mysql_result_bucket(&results);

			if (0 < get_last_stat_for_device(device_id, &stat_id, &ping_success))
			{

				if (ping_success)
					sql_send_query(&results, INSERT_NEW_FAILED_DEVICE_STAT, device_id, pt);
				else
					sql_send_query(&results, UPDATE_DEVICE_STAT, pt, stat_id);

				free(stat_id);
			}
			else
			{
				sql_send_query(&results, INSERT_FIRST_FAILED_DEVICE_STAT, device_id, pt, pt);
			}

			free(pt);
		}
	}
	printf("\n");
}
