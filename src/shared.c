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

jnx_hashmap *ips_to_macs = NULL;
pthread_mutex_t mux = PTHREAD_MUTEX_INITIALIZER;

void
received_mac_for_ip(char *ip, char *mac)
{
pthread_mutex_lock(&mux);
	if (ips_to_macs == NULL)
		ips_to_macs = jnx_hash_init(1024);

	jnx_hash_put(ips_to_macs, ip, mac);
pthread_mutex_unlock(&mux);
}

char**
get_ips_that_responded(int *size)
{
	const char **keys;
	char **retval;
	int i;

pthread_mutex_lock(&mux);
	*size = jnx_hash_get_keys(ips_to_macs, &keys);
	retval = malloc((*size) * sizeof(char**));
	for (i = 0; i < *size; i++)
	{
		retval[i] = malloc(strlen(keys[i]));
		strcpy(retval[i], keys[i]);
	}

	free(keys);
	jnx_hash_delete(ips_to_macs);
	ips_to_macs = NULL;
pthread_mutex_unlock(&mux);

	return retval;
}
