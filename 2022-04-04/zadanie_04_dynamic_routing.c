/*
 * Copyright (C) 2022 Michal Kalewski <mkalewski at cs.put.poznan.pl>
 *
 * Compilation:  gcc -Wall ./ethrecv.c -o ./ethrecv
 * Usage:        ./ethrecv INTERFACE
 * NOTE:         This program requires root privileges.
 *
 * Bug reports:  https://gitlab.cs.put.poznan.pl/mkalewski/ps-2022/issues
 *
 */

#include <arpa/inet.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/route.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define ETH_P_CUSTOM 0x8888
#define IRI_T_ADRESS 0
#define IRI_T_ROUTE 1

struct ifrtinfo {
    int iri_type;
    char iri_iname[16];
    struct sockaddr_in iri_iaddr;
    struct sockaddr_in iri_rtdst;
    struct sockaddr_in iri_rtmsk;
    struct sockaddr_in iri_rtgip;
};


void ifsetup(struct ifrtinfo *ifr_inf){
  int sfd;
  struct ifreq ifr;
  struct sockaddr_in* sin;

  sfd = socket(PF_INET, SOCK_DGRAM, 0);
  strncpy(ifr.ifr_name, ifr_inf->iri_iname, strlen(ifr_inf->iri_iname) + 1);
  sin = (struct sockaddr_in*) &ifr.ifr_addr;
  memset(sin, 0, sizeof(struct sockaddr_in));
  sin->sin_family = ifr_inf->iri_iaddr.sin_family;
  sin->sin_port = ifr_inf->iri_iaddr.sin_port;
  sin->sin_addr.s_addr = ifr_inf->iri_iaddr.sin_addr.s_addr;
  ioctl(sfd, SIOCSIFADDR, &ifr);
  ioctl(sfd, SIOCGIFFLAGS, &ifr);
  ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
  ioctl(sfd, SIOCSIFFLAGS, &ifr);
  close(sfd);
  return;
}

void setgw(struct ifrtinfo *ifr_inf){
  int sfd;
  struct rtentry route;
  struct sockaddr_in* addr;

  sfd = socket(PF_INET, SOCK_DGRAM, 0);
  memset(&route, 0, sizeof(route));
  addr = (struct sockaddr_in*) &route.rt_gateway;


  addr->sin_family = ifr_inf->iri_rtgip.sin_family;
  addr->sin_addr.s_addr = ifr_inf->iri_rtgip.sin_addr.s_addr;
  addr = (struct sockaddr_in*) &route.rt_dst;
  addr->sin_family = ifr_inf->iri_rtdst.sin_family;
  addr->sin_addr.s_addr = ifr_inf->iri_rtdst.sin_addr.s_addr;
  addr = (struct sockaddr_in*) &route.rt_genmask;
  addr->sin_family = ifr_inf->iri_rtmsk.sin_family;
  addr->sin_addr.s_addr = ifr_inf->iri_rtmsk.sin_addr.s_addr;
  route.rt_flags = RTF_UP | RTF_GATEWAY;
  route.rt_metric = 0;
  int rc = ioctl(sfd, SIOCADDRT, &route);
  printf("%d\n", rc);
  close(sfd);
  return;
}

int main(int argc, char** argv) {
  int sfd, i;
  ssize_t len;
  char* frame;
  char* fdata;
  struct ethhdr* fhead;
  struct ifreq ifr;
  struct sockaddr_ll sall;

  struct ifrtinfo *ifr_inf;
  
  sfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_CUSTOM));
  strncpy(ifr.ifr_name, argv[1], IFNAMSIZ);
  ioctl(sfd, SIOCGIFINDEX, &ifr);
  memset(&sall, 0, sizeof(struct sockaddr_ll));
  sall.sll_family = AF_PACKET;
  sall.sll_protocol = htons(ETH_P_CUSTOM);
  sall.sll_ifindex = ifr.ifr_ifindex;
  sall.sll_hatype = ARPHRD_ETHER;
  sall.sll_pkttype = PACKET_HOST;
  sall.sll_halen = ETH_ALEN;
  bind(sfd, (struct sockaddr*) &sall, sizeof(struct sockaddr_ll));
  while(1) {
    frame = malloc(ETH_FRAME_LEN);
    memset(frame, 0, ETH_FRAME_LEN);
    fhead = (struct ethhdr*) frame;
    fdata = frame + ETH_HLEN;
    len = recvfrom(sfd, frame, ETH_FRAME_LEN, 0, NULL, NULL);
    printf("Otrzymano wiadomosc\n");

    ifr_inf = (struct ifrtinfo*) fdata;
    if(ifr_inf->iri_type == IRI_T_ADRESS){
        printf("%d\n",IRI_T_ADRESS);
        ifsetup(ifr_inf);
    }
    else if(ifr_inf->iri_type == IRI_T_ROUTE){
        printf("%d\n",IRI_T_ROUTE);
        setgw(ifr_inf);
    }



    /*printf("[%dB] %02x:%02x:%02x:%02x:%02x:%02x -> ", (int)len,
           fhead->h_source[0], fhead->h_source[1], fhead->h_source[2],
           fhead->h_source[3], fhead->h_source[4], fhead->h_source[5]);
    printf("%02x:%02x:%02x:%02x:%02x:%02x | ",
           fhead->h_dest[0], fhead->h_dest[1], fhead->h_dest[2],
           fhead->h_dest[3], fhead->h_dest[4], fhead->h_dest[5]);
    printf("%s\n", fdata);
    for (i = 0; i < len ; i++) {
      printf("%02x ", (unsigned char) frame[i]);
      if ((i + 1) % 16 == 0)
        printf("\n");
    }
    printf("\n\n");*/
    free(frame);
  }
  close(sfd);
  return EXIT_SUCCESS;
}

