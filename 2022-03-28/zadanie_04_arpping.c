/*
 * Copyright (C) 2022 Michal Kalewski <mkalewski at cs.put.poznan.pl>
 *
 * Compilation:  gcc -Wall ./pcapsniff.c -o ./pcapsniff -lpcap
 * Usage:        ./pcapsniff INTERFACE
 * NOTE:         This program requires root privileges.
 *
 * Bug reports:  https://gitlab.cs.put.poznan.pl/mkalewski/ps-2022/issues
 *
 */

#include <pcap.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <arpa/inet.h>
#include <libnet.h>
#include <time.h>

char* errbuf;
pcap_t* handle;
libnet_t *ln;
clock_t startT, stopT;
u_int8_t* local_ip;


struct arphdr {
  u_int16_t ftype;
  u_int16_t ptype;
  u_int8_t flen;
  u_int8_t plen;
  u_int16_t opcode;
  u_int8_t sender_mac_addr[6];
  u_int8_t sender_ip_addr[4];
  u_int8_t target_mac_addr[6];
  u_int8_t target_ip_addr[4];
};

void cleanup() {
  pcap_close(handle);
  free(errbuf);
}

void stop(int signo) {
  libnet_destroy(ln);

  exit(EXIT_SUCCESS);
}

void trap(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes) {
  //printf("[%dB of %dB]\n", h->caplen, h->len);
  struct ethhdr *eth = (struct ethhdr *)bytes;
  if(ntohs(eth -> h_proto) == 0x0806){
    stopT = clock();
    struct arphdr *arp = (struct arphdr *) (bytes + sizeof(struct ethhdr));
    static int n =0;
    //printf("(%d.%d.%d.%d)\n", local_ip[0],local_ip[1],local_ip[2],local_ip[3]);
    //printf("(%d.%d.%d.%d)\n", arp->sender_ip_addr[0],arp->sender_ip_addr[1],
    //arp->sender_ip_addr[2],arp->sender_ip_addr[3]);
    if(arp->sender_ip_addr[0] == local_ip[0] &&
      arp->sender_ip_addr[1] == local_ip[1] &&
      arp->sender_ip_addr[2] == local_ip[2] &&
      arp->sender_ip_addr[3] == local_ip[3] ){
      printf("%d bytes from %02x:%02x:%02x:%02x:%02x:%02x (%d.%d.%d.%d): index=%d time= %ld ms\n",
      h->len,
      arp->sender_mac_addr[0], arp->sender_mac_addr[1],  
      arp->sender_mac_addr[2], arp->sender_mac_addr[3],
      arp->sender_mac_addr[4], arp->sender_mac_addr[5],
      arp->sender_ip_addr[0], arp->sender_ip_addr[1],
      arp->sender_ip_addr[2], arp->sender_ip_addr[3],
      ++n,
      stopT-startT);

      startT = clock();
      libnet_write(ln);
    }
    
  } 

}

int main(int argc, char** argv) {

  u_int32_t target_ip_addr, src_ip_addr;
  u_int8_t bcast_hw_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
           zero_hw_addr[6]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  struct libnet_ether_addr* src_hw_addr;
  char errbuf2[LIBNET_ERRBUF_SIZE];

  ln = libnet_init(LIBNET_LINK, argv[1], errbuf2);
  src_ip_addr = libnet_get_ipaddr4(ln);
  src_hw_addr = libnet_get_hwaddr(ln);
  target_ip_addr = libnet_name2addr4(ln, argv[2], LIBNET_RESOLVE);
  libnet_autobuild_arp(
    ARPOP_REQUEST,                   /* operation type       */
    src_hw_addr->ether_addr_octet,   /* sender hardware addr */
    (u_int8_t*) &src_ip_addr,        /* sender protocol addr */
    zero_hw_addr,                    /* target hardware addr */
    (u_int8_t*) &target_ip_addr,     /* target protocol addr */
    ln);                             /* libnet context       */
  libnet_autobuild_ethernet(
    bcast_hw_addr,                   /* ethernet destination */
    ETHERTYPE_ARP,                   /* ethertype            */
    ln);                             /* libnet context       */

  atexit(cleanup);
  signal(SIGINT, stop);
  errbuf = malloc(PCAP_ERRBUF_SIZE);
  handle = pcap_create(argv[1], errbuf);
  pcap_set_promisc(handle, 1);
  pcap_set_snaplen(handle, 65535);
  pcap_set_timeout(handle, 1000);
  pcap_activate(handle);

  local_ip = (u_int8_t*) &target_ip_addr;
  printf("Arping %s via %s\n",argv[1],argv[2]);
  startT = clock();
  libnet_write(ln);
  pcap_loop(handle, -1, trap, NULL);
}
