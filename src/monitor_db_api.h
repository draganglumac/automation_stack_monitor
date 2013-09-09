/*
 * =====================================================================================
 *
 *       Filename:  db_api.h
 *
 *    Description
 *
 *        Version:  1.0
 *        Created:  08/09/13 10:18:17
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dragan Glumac (), dragan.glumac@gmail.com 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __DB_API_H__
#define __DB_API_H__

#include "database/sql_interface_layer.h"

char **get_devices_to_probe(int *num_devices);
char **get_machines_to_probe(int *num_machines);

void update_device_stats(time_t poll_time, jnx_hashmap *ip_macs, char **unresponsive, int num_unersponsive);
void update_machine_stats(time_t poll_time, jnx_hashmap *ip_macs, char **unresponsive, int num_unersponsive);

#endif // __DB_API_H__
