/*
 * =====================================================================================
 *
 *       Filename:  shared.c
 *
 *    Description
 *
 *        Version:  1.0
 *        Created:  03/09/2013 19:28:19
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
#include <jnxc_headers/jnxhash.h>
#include <pthread.h>
#include <string.h>

#include "monitor_db_api.h"

jnx_hashmap *ips_to_macs = NULL;
pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;

void
pretty_print(jnx_hashmap *hm)
{
	const char **keys;
	int size = jnx_hash_get_keys(hm, &keys);
	int i;

	for (i = 0; i < size; i++)
	{
		printf("%s=%s\n", keys[i], (char *)jnx_hash_get(hm, keys[i]));
	}
}

void
clear_ips_to_mac()
{
	const char **keys;
	int i, size;

	if (ips_to_macs == NULL)
		return;

	size = jnx_hash_get_keys(ips_to_macs, &keys);
	for (i = 0; i < size; i++)
	{
		free(jnx_hash_delete_value(ips_to_macs,(char*) keys[i]));
		free((char*) keys[i]);
	}
	free(keys);
	jnx_hash_delete(ips_to_macs);

	ips_to_macs = NULL;
}

void
reset_probe()
{
pthread_mutex_lock(&mux);
	if (ips_to_macs != NULL)
	{
		clear_ips_to_mac();
	}

	ips_to_macs = jnx_hash_init(1024);
pthread_mutex_unlock(&mux);
}

void
received_mac_for_ip(char *ip, char *mac)
{
pthread_mutex_lock(&mux);
	if (ips_to_macs == NULL)
		ips_to_macs = jnx_hash_init(1024);

	void *prev_val = jnx_hash_get(ips_to_macs, ip);
	if (prev_val != NULL)
		free(prev_val);
	jnx_hash_put(ips_to_macs, ip, mac);
pthread_mutex_unlock(&mux);
}

char**
get_ips_that_responded(int *size)
{
	const char **keys;
	char **retval;
	int i;
	void *val;

pthread_mutex_lock(&mux);
	if (ips_to_macs == NULL)
	{
pthread_mutex_unlock(&mux);
		*size = 0;
		return NULL;
	}

	*size = jnx_hash_get_keys(ips_to_macs, &keys);
	retval = malloc((*size) * sizeof(char**));
	for (i = 0; i < *size; i++)
	{
		retval[i] = malloc(strlen(keys[i]));
		strcpy(retval[i], keys[i]);
	}
pthread_mutex_unlock(&mux);

	free(keys);
	return retval;
}

void
update_db_after_probing(time_t poll_time, char **unresponsive, int num_unresponsive)
{
pthread_mutex_lock(&mux);
	update_device_stats(poll_time, ips_to_macs, unresponsive, num_unresponsive);
pthread_mutex_unlock(&mux);
}
