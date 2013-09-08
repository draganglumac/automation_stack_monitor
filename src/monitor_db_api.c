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

#include "monitor_db_api.h"

#define GET_DEVICE_IPS "select id, ip from devices where ip is not null;"
#define GET_MACHINE_IPS "select id, ip from machines where ip is not null;"

static char **ips = NULL;

char **get_devices_to_probe(int *num_devices)
{
	char *ips[] = {
		"192.168.1.85", // Samsung Ultrabook
		"192.168.1.75", // Sky+ STB
		"192.168.1.80", // Apple TV
		"192.168.1.86", // Nexus 7
		"192.168.1.64", // MacBook Pro
		"192.168.1.69", // iPhone
		"192.168.1.79"	// iPad
	};
	int size = 7;
	char **temp = malloc(size * sizeof(char**));
	int i;
	*num_devices = size;

	for (i = 0; i < *num_devices; i++)
	{
		temp[i] = malloc(strlen(ips[i]));
		strcpy(temp[i], ips[i]);
	}

	return temp;
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

