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

#include "arp/arp.h"
#include "database/sql_interface_layer.h"
#include "shared.h"

#define RETRIES 5
#define TIMEOUT 10
#define AGGR_RETRIES 5
#define AGGR_TIMEOUT 5

jnx_hashmap *config = NULL;

typedef struct 
{
	char **array;
	int size;
}
array_size;

void usage()
{
	printf("Please provide -c [configuration]\n");
	exit(0);
}
int parse_conf_file(char *path, jnx_hashmap **config)
{
	jnx_file_kvp_node *head = jnx_file_read_keyvaluepairs(path,"=");
	if(!head) { return 1; }
	(*config) = jnx_hash_init(1024);
	if(!(*config)) { return 1; }
	while(head)
	{
		jnx_hash_put((*config),head->key,head->value);
		printf("Inserted %s %s into configuration\n",head->key,head->value);
		free(head);	
		head = head->next;
	}
	return 0;
}

void *start_recv_loop(void *data);
void *start_send_loop(void *data);
char **update_devices_to_probe(char **devices, int *num_devices);
void poll_normally(char ***devices, int *num_devices);
void poll_aggressively(char ***devices, int *num_devices);

int main(int argc, char** argv)
{
	int c, i, num_devices, num_machines;
	char *configuration = NULL;
	pthread_t recv_thread;
	char **devices, **machines;

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
	if(parse_conf_file(configuration,&config) != 0)
	{
		usage();
	}

	pthread_create(&recv_thread, NULL, start_recv_loop, NULL);
	sleep(1);

	devices = get_devices_to_probe(&num_devices);

	printf("Starting normal polling cycle.\n");	
	poll_normally(&devices, &num_devices);

	if (num_devices > 0)
	{
		printf("Starting aggressive polling cycle to try to wake up sleepy devices.\n");
		poll_aggressively(&devices, &num_devices);
	}

	if (num_devices > 0)
	{
		update_non_responsive_devices(devices, num_devices);
	}
	else
	{
		printf("All devices responded with MAC addresses.\n");
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
		printf("Probing %s\n", ips[i]);
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
				break;
			}
		}

		if (!found)
		{
			temp[new_size] = devices[i];
			printf("Retrying %s\n", temp[new_size]);
			new_size++;
		}
	}

	free(devices);

	*num_devices = new_size;
	return temp;
}

void
poll(char ***devices, int *num_devices, int retries, int timeout)
{
	int i;
	array_size data;
	pthread_t recv_thread, send_thread;
	
	for (i = 0; i < retries; i++)
	{
		if (*num_devices <= 0)
		{
			printf("All devices responded.\n");
			break;
		}

		data.array = *devices;
		data.size = *num_devices;

		pthread_create(&send_thread, NULL, start_send_loop, (void*) &data);
		pthread_join(send_thread, NULL);
		
		sleep(timeout);

		*devices = update_devices_to_probe(*devices, num_devices);
		printf("\nAt the end of iteration <%d> new num_devices to probe = %d\n", i, *num_devices);
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

