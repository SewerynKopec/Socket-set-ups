/*
 * Copyright (C) 2022 Michal Kalewski <mkalewski at cs.put.poznan.pl>
 *
 * Compilation:  gcc -Wall ./sctpms-server.c -o ./sctpms-server -lsctp
 * Usage:        ./sctpms-server
 *
 * Bug reports:  https://gitlab.cs.put.poznan.pl/mkalewski/ps-2022/issues
 *
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define LOCAL_TIME_STREAM 0
#define GREENWICH_MEAN_TIME_STREAM 1

int main(int argc, char **argv) {
  int sfd, cfd, on = 1, no, i;
  socklen_t sl;
  struct sockaddr_in saddr, caddr;
  struct sctp_event_subscribe events;
  struct sctp_paddrparams heartbeat;
  struct sctp_rtoinfo rtoinfo;
  struct sockaddr_in *laddrs[5];
  char buf[1024];
  time_t now;

  sfd = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP);
  setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on));
  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons(1234);
  ///
  memset(&heartbeat, 0, sizeof(heartbeat));
  heartbeat.spp_flags = SPP_HB_ENABLE;
  heartbeat.spp_hbinterval = 2000;
  heartbeat.spp_pathmaxrxt = 1;
  setsockopt(sfd, SOL_SCTP, SCTP_PEER_ADDR_PARAMS , &heartbeat,
             sizeof(heartbeat));
  memset(&rtoinfo, 0, sizeof(rtoinfo));
  rtoinfo.srto_max = 2000;
  setsockopt(sfd, SOL_SCTP, SCTP_RTOINFO , &rtoinfo, sizeof(rtoinfo));
  memset((void*) &events, 0, sizeof(events));
  events.sctp_data_io_event = 1;
  setsockopt(sfd, SOL_SCTP, SCTP_EVENTS, (void*) &events, sizeof(events));
  ///
  bind(sfd, (struct sockaddr*) &saddr, sizeof(saddr));
  listen(sfd, 5);
  ///
  no = sctp_getladdrs(sfd, 0, (struct sockaddr**) laddrs);
  for(i = 0; i < no; i++)
    printf("ADDR %d: %s:%d\n", i + 1, inet_ntoa((*laddrs)[i].sin_addr),
           ntohs((*laddrs)[i].sin_port));
  sctp_freeladdrs((struct sockaddr*) *laddrs);

  while(1) {
    memset(&caddr, 0, sizeof(caddr));
    sl = sizeof(caddr);
    cfd = accept(sfd, (struct sockaddr*) &caddr, &sl);
    printf("new connection: %s:%d\n", inet_ntoa(caddr.sin_addr),
           caddr.sin_port);
    now = time(NULL);
    snprintf(buf, 1024, "%s", ctime(&now));
    sctp_sendmsg(cfd, (void*) buf, (size_t) strlen(buf) + 1, NULL, 0, 0, 0,
                 LOCAL_TIME_STREAM, 0, 0);
    snprintf(buf, 1024, "%s", asctime(gmtime(&now)));
    sctp_sendmsg(cfd, (void*) buf, (size_t) strlen(buf) + 1, NULL, 0, 0, 0,
                 GREENWICH_MEAN_TIME_STREAM, 0, 0);
    close(cfd);
  }
  close(sfd);

  return EXIT_SUCCESS;
}
