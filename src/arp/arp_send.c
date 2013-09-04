/*
 * =====================================================================================
 *
 *       Filename:  send_arp.c
 *    
 *    Description:  Modeled on the example code by P. David Buchan sourced from
 *	                http://www.pdbuchan.com/rawsock/rawsock.html 
 *
 *        Version:  1.0
 *        Created:  08/30/2013 02:18:37 PM
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
#include <unistd.h>           // close()
#include <string.h>           // strcpy, memset(), and memcpy()

#include <netdb.h>            // struct addrinfo
#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_RAW, INET_ADDRSTRLEN
#include <netinet/ip.h>       // IP_MAXPACKET (which is 65535)
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl.
#include <net/if.h>           // struct ifreq
#include <linux/if_ether.h>   // ETH_P_ARP = 0x0806
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <net/ethernet.h>
#include <jnxc_headers/jnxhash.h>

#include <errno.h>            // errno, perror()

#include "arp.h"

extern jnx_hashmap *config;

void get_local_mac_address(char *interface, struct ifreq *ifr, uint8_t **src_mac);
void resolve_local_ip_address_for_interface(char *interface, struct in_addr *ip_addr);
void resolve_addrinfo_for_ip(char* ip, struct in_addr *ip_addr);
void set_arp_header(uint8_t *src_mac, arp_hdr *arphdr);
int pack_ethernet_frame(arp_hdr *arphdr, uint8_t *dst_mac, uint8_t *src_mac, uint8_t *ether_frame);

int
arp_send (char *target_ip)
{
	int i, status, frame_length, sd, bytes;
	char *interface, *target;
	arp_hdr arphdr;
	uint8_t *src_mac, *dst_mac, *ether_frame;
	struct sockaddr_ll device; // ToDo: Standardise!
	struct ifreq ifr;

	// Allocate memory for various arrays.
	src_mac = allocate_ustrmem (6);
	dst_mac = allocate_ustrmem (6);
	ether_frame = allocate_ustrmem (IP_MAXPACKET);
	interface = allocate_strmem (40);
	target = allocate_strmem (40);

	get_local_mac_address(interface, &ifr, &src_mac);

	// Set destination MAC address: broadcast address
	memset (dst_mac, 0xff, 6 * sizeof (uint8_t));

	resolve_local_ip_address_for_interface(interface, (struct in_addr *) &arphdr.sender_ip);

	// Resolve target using getaddrinfo().
	resolve_addrinfo_for_ip(target_ip, (struct in_addr *) &arphdr.target_ip);

	set_arp_header(src_mac, &arphdr);

	frame_length = pack_ethernet_frame(&arphdr, dst_mac, src_mac, ether_frame);

	// Submit request for a raw socket descriptor.
	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
		perror ("socket() failed ");
		exit (EXIT_FAILURE);
	}

	// Fill out sockaddr_ll.
	// Find interface index from interface name and store index in
	// struct sockaddr_ll device, which will be used as an argument of sendto().
	if ((device.sll_ifindex = if_nametoindex (interface)) == 0) {
		perror ("if_nametoindex() failed to obtain interface index ");
		exit (EXIT_FAILURE);
	}

	device.sll_family = AF_PACKET;
	memcpy (device.sll_addr, src_mac, 6 * sizeof (uint8_t));
	device.sll_halen = htons (6);


	// Send ethernet frame to socket.
	if ((bytes = sendto (sd, ether_frame, frame_length, 0, (struct sockaddr *) &device, sizeof (device))) <= 0) {
		perror ("sendto() failed");
		exit (EXIT_FAILURE);
	}

	// Close socket descriptor.
	close (sd);

	// Free allocated memory.
	free (src_mac);
	free (dst_mac);
	free (ether_frame);
	free (interface);
	free (target);

	return (EXIT_SUCCESS);
}

void
get_local_mac_address(char *interface, struct ifreq *ifr, uint8_t **src_mac)
{
	int i, sd;

	// Interface to send packet through.
	strcpy (interface, jnx_hash_get(config, "INTERFACE"));  

	// Submit request for a socket descriptor to look up interface.
	if ((sd = socket (AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror ("socket() failed to get socket descriptor for using ioctl() ");
		exit (EXIT_FAILURE);
	}

	// Use ioctl() to look up interface name and get its MAC address.
	memset (ifr, 0, sizeof (ifr));
	sprintf (ifr->ifr_name, "%s", interface);
	if (ioctl (sd, SIOCGIFHWADDR, ifr) < 0) {
		perror ("ioctl() failed to get source MAC address\n");
		exit (EXIT_FAILURE);
	}
	close (sd);

	// Copy source MAC address.
	memcpy (*src_mac, ifr->ifr_hwaddr.sa_data, 6 * sizeof (uint8_t));
}

void
resolve_local_ip_address_for_interface(char *interface, struct in_addr *ip_addr)
{
	int sd;
	struct ifreq ifr;
	struct sockaddr_in *ipv4;

	sd = socket(AF_INET, SOCK_DGRAM, 0);

	// We want to get an IPv4 IP address
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);
	// Resolve using ioctl for interface
	ioctl(sd, SIOCGIFADDR, &ifr);

	close(sd);

	ipv4 = (struct sockaddr_in *) (&ifr.ifr_addr);
	memcpy (ip_addr, &ipv4->sin_addr, 4 * sizeof (uint8_t));
}

void
resolve_addrinfo_for_ip(char* ip, struct in_addr *ip_addr)
{
	int status;
	struct addrinfo hints, *res;
	struct sockaddr_in *ipv4;

	// Fill out hints for getaddrinfo().
	memset (&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = hints.ai_flags | AI_CANONNAME;

	// Resolve source using getaddrinfo().
	if ((status = getaddrinfo (ip, NULL, &hints, &res)) != 0) {
		fprintf (stderr, "getaddrinfo() failed: %s\n", gai_strerror (status));
		exit (EXIT_FAILURE);
	}
	ipv4 = (struct sockaddr_in *) res->ai_addr;
	
	memcpy (ip_addr, &ipv4->sin_addr, 4 * sizeof (uint8_t));
	freeaddrinfo (res);
}

void
set_arp_header(uint8_t *src_mac, arp_hdr *arphdr)
{
	int frame_length;

	// Hardware type (16 bits): 1 for ethernet
	arphdr->htype = htons (1);

	// Protocol type (16 bits): 2048 for IP
	arphdr->ptype = htons (ETH_P_IP);

	// Hardware address length (8 bits): 6 bytes for MAC address
	arphdr->hlen = 6;

	// Protocol address length (8 bits): 4 bytes for IPv4 address
	arphdr->plen = 4;

	// OpCode: 1 for ARP request
	arphdr->opcode = htons (ARPOP_REQUEST);

	// Sender hardware address (48 bits): MAC address
	memcpy (&(arphdr->sender_mac), src_mac, 6 * sizeof (uint8_t));

	// Sender protocol address (32 bits)
	// See getaddrinfo() resolution of src_ip.

	// Target hardware address (48 bits): zero, since we don't know it yet.
	memset (&arphdr->target_mac, 0, 6 * sizeof (uint8_t));
}

int
pack_ethernet_frame(arp_hdr *arphdr, uint8_t *dst_mac, uint8_t *src_mac, uint8_t *ether_frame)
{
	// Fill out ethernet frame header.
	int frame_length;

	// Ethernet frame length = ethernet header (MAC + MAC + ethernet type) + ethernet data (ARP header)
	frame_length = 6 + 6 + 2 + ARP_HDRLEN;

	// Destination and Source MAC addresses
	memcpy (ether_frame, dst_mac, 6 * sizeof (uint8_t));
	memcpy (ether_frame + 6, src_mac, 6 * sizeof (uint8_t));

	// Next is ethernet type code (ETH_P_ARP for ARP).
	// http://www.iana.org/assignments/ethernet-numbers
	ether_frame[12] = ETH_P_ARP / 256;
	ether_frame[13] = ETH_P_ARP % 256;

	// Next is ethernet frame data (ARP header).

	// ARP header
	memcpy (ether_frame + ETH_HDRLEN, arphdr, ARP_HDRLEN * sizeof (uint8_t));

	return frame_length;
}
