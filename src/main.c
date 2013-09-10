/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/29/2013 12:38:32 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dragan Glumac (), dragan.glumac@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <jnxc_headers/jnxhash.h>
#include <jnxc_headers/jnxfile.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "arp/arp.h"
#include "monitor_db_api.h"
#include "shared.h"

#define RETRIES 5
#define TIMEOUT 10
#define AGGR_RETRIES 5
#define AGGR_TIMEOUT 5

//#define PROBE_TIMEOUT 300
#define PROBE_TIMEOUT 120

jnx_hashmap *config = NULL;

typedef struct 
{
	char **array;
	int size;
}
array_size;

void
usage()
{
	printf("Please provide -c [configuration]\n");
	exit(0);
}

int
parse_conf_file(char *path, jnx_hashmap **config)
{
	jnx_file_kvp_node *temp;
	jnx_file_kvp_node *head = jnx_file_read_keyvaluepairs(path,"=");
	if(!head) { return 1; }
	(*config) = jnx_hash_init(1024);
	if(!(*config)) { return 1; }
	while(head)
	{
		jnx_hash_put((*config),head->key,head->value);
		printf("Inserted %s %s into configuration\n",head->key,head->value);
		temp = head;
		head = head->next;
		free(temp);	
	}
	return 0;
}

char* check_program_arguments(int argc, char **argv)
{
	int c;
	char *configuration = NULL;

	while((c = getopt(argc, argv,"c:")) != -1)
	{
		switch(c)
		{
			case 'c':
				configuration = optarg;
				break;
			case '?':
				usage();
				break;
		}
	}

	if(!configuration)
	{
		usage();
	}

	return configuration;
}

void *start_recv_loop(void *data);
void *start_send_loop(void *data);
char **update_devices_to_probe(char **devices, int *num_devices);
void poll_normally(char ***devices, int *num_devices);
void poll_aggressively(char ***devices, int *num_devices);

int
main(int argc, char** argv)
{
	int c, i, j, num_devices, num_machines, probe_sleep;
	time_t start_time;
	char *configuration = NULL;
	pthread_t recv_thread;
	char **devices, **machines;

	configuration = check_program_arguments(argc, argv);
	if(parse_conf_file(configuration,&config) != 0)
	{
		usage();
	}

	// Reset the probe data structures in preparation for the first poll cycle
	reset_probe();

	pthread_create(&recv_thread, NULL, start_recv_loop, NULL);
	sleep(1);

	while(1)
	{
		start_time = time(0);

		devices = get_devices_to_probe(&num_devices);

		printf("Starting normal probe cycle.\n");	
		poll_normally(&devices, &num_devices);

		if (num_devices > 0)
		{
			printf("Starting aggressive probe cycle to try to wake up sleepy devices.\n");
			poll_aggressively(&devices, &num_devices);
		}

		update_db_after_probing(start_time, devices, num_devices);

		for (i = 0; i < num_devices; i++)
			free(devices[i]);
		free(devices);

		// Reset the probe data structures for the next polling cycle
		reset_probe();

		probe_sleep = PROBE_TIMEOUT - (int)(time(0) - start_time);
		printf("Sleeping for %um %us until the next probe cycle.\n", probe_sleep / 60, probe_sleep % 60);
		sleep(probe_sleep);	
	}

	return 0;
}

void *
start_recv_loop(void *data)
{
	printf("Starting receive thread.\n");

	arp_recv();

	printf("Receive thread completed.\n");

	return NULL;
}

void *
start_send_loop(void *data)
{
	array_size *ips_to_send = (array_size*) data;
	char **ips = ips_to_send->array;
	int size = ips_to_send->size;
	int i;

	for (i = 0; i < size; i++)
	{
		arp_send(ips[i]);
	}

	return NULL;
}

char **
update_devices_to_probe(char **devices, int *num_devices)
{
	char **temp, **seen;
	int i, j, seen_size, new_size = 0, found;

	seen = get_ips_that_responded(&seen_size);
	if (seen == NULL)
	{
		// No responses received
		return devices;
	}

	temp = malloc((*num_devices) * sizeof(char**));
	for (i = 0; i < *num_devices; i++)
	{
		found = 0;
		for (j = 0; j < seen_size; j++)
		{
			if (strcmp(devices[i], seen[j]) == 0)
			{
				found = 1;
				free(devices[i]);
				break;
			}
		}

		if (!found)
		{
			temp[new_size] = devices[i];
			new_size++;
		}
	}

	free(devices);
	for (i = 0; i < seen_size; i++)
		free(seen[i]);
	free(seen);

	*num_devices = new_size;
	return temp;
}

void
poll(char ***devices, int *num_devices, int retries, int timeout)
{
	int i, sleep_period;
	array_size data;
	pthread_t recv_thread, send_thread;
	time_t start_time;

	for (i = 0; i < retries; i++)
	{
		if (*num_devices <= 0)
		{
			break;
		}

		start_time = time(0);

		data.array = *devices;
		data.size = *num_devices;

		pthread_create(&send_thread, NULL, start_send_loop, (void*) &data);
		pthread_join(send_thread, NULL);

		sleep_period = timeout - (int)(time(0) - start_time);
		sleep(sleep_period);

		*devices = update_devices_to_probe(*devices, num_devices);
	}
}

void
poll_normally(char ***devices, int *num_devices)
{
	poll(devices, num_devices, RETRIES, TIMEOUT);
}

void
poll_aggressively(char ***devices, int *num_devices)
{
	poll(devices, num_devices, AGGR_RETRIES, AGGR_TIMEOUT);
}

