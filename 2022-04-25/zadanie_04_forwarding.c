/*
 * Copyright (C) 2022 Michal Kalewski <mkalewski at cs.put.poznan.pl>
 *
 * Compilation:  gcc -Wall ./iprecv.c -o ./iprecv
 * Usage:        ./iprecv
 * NOTE:         This program requires root privileges.
 *
 * Bug reports:  https://gitlab.cs.put.poznan.pl/mkalewski/ps-2022/issues
 *
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define IPPROTO_CUSTOM 222

int main(int argc, char **argv) {
  int sfd, sfd2, rc;
  char buf[65536], saddr[16], daddr[16];
  char *data;
  socklen_t sl;
  struct sockaddr_in addr;
  struct sockaddr_in addr2;
  struct iphdr *ip;


  sfd = socket(PF_INET, SOCK_RAW, IPPROTO_CUSTOM);

  while(1) {
    memset(&addr, 0, sizeof(addr));
    memset(&addr2, 0, sizeof(addr2));
    sl = sizeof(addr);
    rc = recvfrom(sfd, buf, sizeof(buf), 0, (struct sockaddr*) &addr, &sl);
    ip = (struct iphdr*) &buf;
    if (ip->protocol == IPPROTO_CUSTOM) {
      inet_ntop(AF_INET, &ip->saddr, (char*) &saddr, 16);
      inet_ntop(AF_INET, &ip->daddr, (char*) &daddr, 16);
      data = (char*) ip + (ip->ihl * 4);
      printf("[%dB] %s -> %s | %s\n", rc - (ip->ihl * 4), saddr, daddr, data);

        sfd2 = socket(PF_INET, SOCK_RAW, IPPROTO_CUSTOM);
        //send 
        addr2.sin_family = AF_INET;
        addr2.sin_port = 0;
        addr2.sin_addr.s_addr = inet_addr(argv[1]);
        sendto(sfd2, data, strlen(data) + 1, 0, (struct sockaddr*) &addr2,
            sizeof(addr2));
        close(sfd2);

    }


  }
  close(sfd);
  return EXIT_SUCCESS;
}
