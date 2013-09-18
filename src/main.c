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
#include <sys/types.h>
#include <sys/stat.h>

#include "arp/arp.h"
#include "monitor_db_api.h"
#include "shared.h"

#define DEFAULT_CONFIG "/etc/automation_stack_monitor.conf"

#define RETRIES 5
int retries = RETRIES;
#define TIMEOUT 10
int timeout = TIMEOUT;
#define AGGR_RETRIES 5
int aggr_retries = AGGR_RETRIES;
#define AGGR_TIMEOUT 5
int aggr_timeout = AGGR_TIMEOUT;

#define PROBE_TIMEOUT 300
int probe_timeout = PROBE_TIMEOUT;

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
	printf("Please provide /etc/automation_stack_monitor.conf or pass -c [configuration] argument.\n");
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

char*
check_program_arguments(int argc, char **argv)
{
	int c;
	char *configuration = NULL;
	struct stat confstat;

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
		if (stat(DEFAULT_CONFIG, &confstat) != 0)			
			usage();
		else
			if (S_ISREG(confstat.st_mode))
				configuration = DEFAULT_CONFIG; 
			else
				usage();

	return configuration;
}

int
get_int_from_config(char *key)
{
	char *val = jnx_hash_get(config, key);
	if (val)
	{
		int ival = atoi(val);
		free(val);
		return ival;
	}

	return -1;
}

void
set_global_constants()
{
	int val;

	val = get_int_from_config("RETRIES");
	if (val > 0)
		retries = val;

	val = get_int_from_config("TIMEOUT");
	if (val > 0)
		timeout = val;

	val = get_int_from_config("AGGR_RETRIES");
	if (val > 0)
		aggr_retries = val;

	val = get_int_from_config("AGGR_TIMEOUT");
	if (val > 0)
		aggr_timeout = val;

	val = get_int_from_config("PROBE_TIMEOUT");
	if (val > 0)
		probe_timeout = val;
}

void *start_recv_loop(void *data);
void *start_send_loop(void *data);
char **update_devices_to_probe(char **devices, int *num_devices);
void poll_normally(char ***devices, int *num_devices);
void poll_aggressively(char ***devices, int *num_devices);

int
main(int argc, char** argv)
{
	int i, num_devices, num_machines, probe_sleep;
	time_t start_time;
	char *configuration = NULL;
	pthread_t recv_thread;
	char **devices, **machines;
	configuration = check_program_arguments(argc, argv);
	if(parse_conf_file(configuration,&config) != 0)
	{
		usage();
	}

	if (sql_setup_credentials() != 0)
	{
		printf("Couldn't set up SQL credentials. Exiting monitor.\n");
		exit(1);
	}

	set_global_constants();

	// Reset the probe data structures in preparation for the first poll cycle
	reset_probe();

	pthread_create(&recv_thread, NULL, start_recv_loop, NULL);
	sleep(1);

//	while(1)
//	{
		start_time = time(0);

		devices = get_devices_to_probe(&num_devices);

		printf("%s\n", ctime(&start_time));
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

		probe_sleep = probe_timeout - (int)(time(0) - start_time);
		sleep(probe_sleep);	
//	}

	return 0;
}

void *
start_recv_loop(void *data)
{
	printf("Starting receive thread - listening for ARP responses.\n");

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
	poll(devices, num_devices, retries, timeout);
}

void
poll_aggressively(char ***devices, int *num_devices)
{
	poll(devices, num_devices, aggr_retries, aggr_timeout);
}
