/*
 * =====================================================================================
 *
 *       Filename:  arp_util.c
 *
 *    Description:  Modeled on the example code by P. David Buchan sourced from
 *	                http://www.pdbuchan.com/rawsock/rawsock.html 
 *
 *        Version:  1.0
 *        Created:  08/30/2013 02:27:27 PM
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
#include <string.h>           // strcpy, memset(), and memcpy()

#include "arp.h"

// Allocate memory for an array of chars.
char *
allocate_strmem (int len)
{
  void *tmp;
  
  tmp = (char *) malloc (len * sizeof (char));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (char));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_strmem().\n");
    exit (EXIT_FAILURE);
  }
}

// Allocate memory for an array of unsigned chars.
uint8_t *
allocate_ustrmem (int len)
{
  void *tmp;

  tmp = (uint8_t *) malloc (len * sizeof (uint8_t));
  if (tmp != NULL) {
    memset (tmp, 0, len * sizeof (uint8_t));
    return (tmp);
  } else {
    fprintf (stderr, "ERROR: Cannot allocate memory for array allocate_ustrmem().\n");
    exit (EXIT_FAILURE);
  }
}
