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
void update_non_responsive_devices(char **devices, int num_devices);

#endif // __DB_API_H__
