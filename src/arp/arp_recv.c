/*
 * =====================================================================================
 *
 *       Filename:  arp_recv.c
 *
 *    Description:  Modeled on the example code by P. David Buchan sourced from
 *	                http://www.pdbuchan.com/rawsock/rawsock.html 
 *
 *        Version:  1.0
 *        Created:  09/01/2013 07:49:12 PM
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
#include <string.h>           // strcpy, memset()

#include <netinet/ip.h>       // IP_MAXPACKET (65535)
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t
#include <sys/socket.h>       // needed for socket()
#include <net/ethernet.h>

#include <errno.h>            // errno, perror()

#include "arp.h"
#include "../shared.h"

void pretty_print_packet(uint8_t *ether_frame, arp_hdr *arphdr);
char* get_remote_ip_from_arphdr(arp_hdr *arphdr);
char* get_remote_mac_from_arphdr(arp_hdr *arphdr);

int
arp_recv ()
{
	int i, sd, status;
	uint8_t *ether_frame;
	arp_hdr *arphdr;
	char *ip, *mac;

	// Submit request for a raw socket descriptor.
	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
		perror ("socket() failed ");
		exit (EXIT_FAILURE);
	}

	while (1)
	{
		// Allocate memory for various arrays.
		ether_frame = allocate_ustrmem (IP_MAXPACKET);

		// Listen for incoming ethernet frame from socket sd.
		// We expect an ARP ethernet frame of the form:
		//     MAC (6 bytes) + MAC (6 bytes) + ethernet type (2 bytes)
		//     + ethernet data (ARP header) (28 bytes)
		// Keep at it until we get an ARP reply.
		arphdr = (arp_hdr *) (ether_frame + 6 + 6 + 2);
		while (((((ether_frame[12]) << 8) + ether_frame[13]) != ETH_P_ARP) || (ntohs (arphdr->opcode) != ARPOP_REPLY)) {
			if ((status = recv (sd, ether_frame, IP_MAXPACKET, 0)) < 0) {
				if (errno == EINTR) {
					memset (ether_frame, 0, IP_MAXPACKET * sizeof (uint8_t));
					continue;  // Something weird happened, but let's try again.
				} else {
					perror ("recv() failed:");
					exit (EXIT_FAILURE);
				}
			}
		}
	
		ip = get_remote_ip_from_arphdr(arphdr);
		mac = get_remote_mac_from_arphdr(arphdr);

		received_mac_for_ip(ip, mac);
		
		free(ip);
		free (ether_frame);
	}

	close (sd);

	return (EXIT_SUCCESS);
}

char*
get_remote_ip_from_arphdr(arp_hdr *arphdr)
{
	// IP address format -> nnn.nnn.nnn.nnn, so at most 15 chars plus NULL byte
	char *ip = malloc(16 * sizeof(char));
	sprintf(ip, "%u.%u.%u.%u",
			arphdr->sender_ip[0], 
			arphdr->sender_ip[1], 
			arphdr->sender_ip[2], 
			arphdr->sender_ip[3]);

	return ip;
}

char*
get_remote_mac_from_arphdr(arp_hdr *arphdr)
{	
	// MAC address format -> xx:xx:xx:xx:xx:xx, so at most 17 chars plus NULL byte
	char *mac = malloc(18 * sizeof(char));
	sprintf (mac, "%02x:%02x:%02x:%02x:%02x:%02x",
			arphdr->sender_mac[0],
			arphdr->sender_mac[1],
			arphdr->sender_mac[2],
			arphdr->sender_mac[3],
			arphdr->sender_mac[4],
			arphdr->sender_mac[5]);

	return mac;
}

void pretty_print_packet(uint8_t *ether_frame, arp_hdr *arphdr)
{
	int i;

	// Print out contents of received ethernet frame.
	printf ("\nEthernet frame header:\n");
	printf ("Destination MAC (this node): ");
	for (i=0; i<5; i++) {
		printf ("%02x:", ether_frame[i]);
	}
	printf ("%02x\n", ether_frame[5]);
	printf ("Source MAC: ");
	for (i=0; i<5; i++) {
		printf ("%02x:", ether_frame[i+6]);
	}
	printf ("%02x\n", ether_frame[11]);
	// Next is ethernet type code (ETH_P_ARP for ARP).
	// http://www.iana.org/assignments/ethernet-numbers
	printf ("Ethernet type code (2054 = ARP): %u\n", ((ether_frame[12]) << 8) + ether_frame[13]);
	printf ("\nEthernet data (ARP header):\n");
	printf ("Hardware type (1 = ethernet (10 Mb)): %u\n", ntohs (arphdr->htype));
	printf ("Protocol type (2048 for IPv4 addresses): %u\n", ntohs (arphdr->ptype));
	printf ("Hardware (MAC) address length (bytes): %u\n", arphdr->hlen);
	printf ("Protocol (IPv4) address length (bytes): %u\n", arphdr->plen);
	printf ("Opcode (2 = ARP reply): %u\n", ntohs (arphdr->opcode));
	printf ("Sender hardware (MAC) address: ");
	for (i=0; i<5; i++) {
		printf ("%02x:", arphdr->sender_mac[i]);
	}
	printf ("%02x\n", arphdr->sender_mac[5]);
	printf ("Sender protocol (IPv4) address: %u.%u.%u.%u\n",
			arphdr->sender_ip[0], arphdr->sender_ip[1], arphdr->sender_ip[2], arphdr->sender_ip[3]);
	printf ("Target (this node) hardware (MAC) address: ");
	for (i=0; i<5; i++) {
		printf ("%02x:", arphdr->target_mac[i]);
	}
	printf ("%02x\n", arphdr->target_mac[5]);
	printf ("Target (this node) protocol (IPv4) address: %u.%u.%u.%u\n",
			arphdr->target_ip[0], arphdr->target_ip[1], arphdr->target_ip[2], arphdr->target_ip[3]);

}
