/*
 * =====================================================================================
 *
 *       Filename:  shared.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/09/2013 19:13:42
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __SHARED_H__
#define __SHARED_H__

void received_mac_for_ip(char *ip, char *mac);
char** get_ips_that_responded(int *size);

#endif // __SHARED_H__
