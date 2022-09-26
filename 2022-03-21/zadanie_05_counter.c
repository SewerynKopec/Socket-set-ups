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

char* errbuf;
pcap_t* handle;

int arp_count = 0;
int ip_count = 0;
int ip_tcp_count = 0;
int ip_udp_count = 0;
int other_count = 0;

void cleanup() {
  pcap_close(handle);
  free(errbuf);
}

void stop(int signo) {
  printf("ARP: %d \n IP: %d \n IP/TCP: %d \n IP/UDP: %d \n Other: %d \n", arp_count, ip_count, ip_tcp_count, ip_udp_count, other_count);
  exit(EXIT_SUCCESS);
}

void trap(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes) {
  //printf("[%dB of %dB]\n", h->caplen, h->len);

  struct ethhdr *eth = (struct ethhdr *)bytes;
  switch(ntohs(eth -> h_proto)){
    case 0x0806: //ARP
      ++arp_count;
    break;
    case 0x0800: //IP
      ++ip_count;
      struct iphdr *iph = (struct iphdr *)(bytes+sizeof(struct ethhdr));
      switch(iph->protocol){
        case 6: //tcp
          ++ip_tcp_count;
        break;
        case 17: //udp
          ++ip_udp_count;
        break;
        case 0:
          //pseudo protocol number
        break;
        default: //other
          ++other_count;
        break; 
      }

    break;
    default: //other
      ++other_count;
    break;
  } 

}

int main(int argc, char** argv) {
  atexit(cleanup);
  signal(SIGINT, stop);
  errbuf = malloc(PCAP_ERRBUF_SIZE);
  handle = pcap_create(argv[1], errbuf);
  pcap_set_promisc(handle, 1);
  pcap_set_snaplen(handle, 65535);
  pcap_set_timeout(handle, 1000);
  pcap_activate(handle);
  pcap_loop(handle, -1, trap, NULL);
}
