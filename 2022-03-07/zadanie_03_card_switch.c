/*
 * Copyright (C) 2022 Michal Kalewski <mkalewski at cs.put.poznan.pl>
 *
 * Compilation:  gcc -Wall ./ifsetup.c -o ./ifsetup
 * Usage:        ./ifsetup IFNAME IP
 * NOTE:         This program requires root privileges.
 *
 * Bug reports:  https://gitlab.cs.put.poznan.pl/mkalewski/ps-2022/issues
 *
 */

#include <arpa/inet.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(int argc, char** argv) {
  int sfd;
  struct ifreq ifr;

  sfd = socket(PF_INET, SOCK_DGRAM, 0);
  strncpy(ifr.ifr_name, argv[1], strlen(argv[1]) + 1);
  ioctl(sfd, SIOCGIFFLAGS, &ifr);
  if(atoi(argv[2]) == 0){
    ifr.ifr_flags &= ~IFF_UP;
  }
  if(atoi(argv[2]) == 1){
    ifr.ifr_flags |= IFF_UP;
}
  ioctl(sfd, SIOCSIFFLAGS, &ifr);
  close(sfd);
  return EXIT_SUCCESS;
}
