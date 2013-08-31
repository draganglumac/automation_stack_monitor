/*
 * =====================================================================================
 *
 *       Filename:  arp.h
 *
 *    Description:  Modeled on the example code by P. David Buchan sourced from
 *	                http://www.pdbuchan.com/rawsock/rawsock.html 
 *
 *        Version:  1.0
 *        Created:  08/30/2013 02:20:08 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dragan Glumac (), dragan.glumac@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __ARP_H__
#define __ARP_H__

#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t
#include <arpa/inet.h>        // inet_pton() and inet_ntop()

// Define a struct for ARP header
typedef struct _arp_hdr arp_hdr;
struct _arp_hdr {
  uint16_t htype;
  uint16_t ptype;
  uint8_t hlen;
  uint8_t plen;
  uint16_t opcode;
  uint8_t sender_mac[6];
  uint8_t sender_ip[4];
  uint8_t target_mac[6];
  uint8_t target_ip[4];
};

// Define some constants.
#define ETH_HDRLEN 14      // Ethernet header length
#define IP4_HDRLEN 20      // IPv4 header length
#define ARP_HDRLEN 28      // ARP header length
#define ARPOP_REQUEST 1    // Taken from <linux/if_arp.h>

// Function prototypes
char *allocate_strmem (int);
uint8_t *allocate_ustrmem (int);

// Function arp_send
//     Sends an ARP packet to an IP address via interface specified in
//     confing['INTERFACE'].
//
// Parameters
//     target_ip - string containing the IPv4 address of the target host
//
// Return value
//     Returns an integer, 0 if success and non-zero otherwise.
//
int arp_send (char *target_ip);

#endif // __ARP_H__
