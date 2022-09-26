#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char** argv) {
  int sfd, sfd2, rc;
  socklen_t sl;
  char buf[128];
  struct sockaddr_in saddr, caddr, saddr2, caddr2;
  struct hostent* addrent;

  memset(&saddr, 0, sizeof(saddr));
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;
  saddr.sin_port = htons(1234);
  sfd = socket(PF_INET, SOCK_DGRAM, 0);
  bind(sfd, (struct sockaddr*) &saddr, sizeof(saddr));

  addrent = gethostbyname(argv[1]);
  memset(&caddr2, 0, sizeof(caddr2));
  caddr2.sin_family = AF_INET;
  caddr2.sin_addr.s_addr = INADDR_ANY;
  caddr2.sin_port = 0;
  memset(&saddr2, 0, sizeof(saddr2));
  saddr2.sin_family = AF_INET;
  memcpy(&saddr2.sin_addr.s_addr, addrent->h_addr, addrent->h_length);
  saddr2.sin_port = htons(atoi(argv[2]));
  sfd2 = socket(PF_INET, SOCK_DGRAM, 0);
  bind(sfd2, (struct sockaddr*) &caddr2, sizeof(caddr2));



  while(1) {
    memset(&caddr, 0, sizeof(caddr));
    memset(&buf, 0, sizeof(buf));
    sl = sizeof(caddr);
    rc = recvfrom(sfd, buf, 128, 0, (struct sockaddr*) &caddr, &sl);
    write(1,buf,rc);
    sendto(sfd2, buf, rc, 0, (struct sockaddr*) &saddr2, sizeof(saddr2));
  }

  close(sfd2);
  close(sfd);
  return EXIT_SUCCESS;
}