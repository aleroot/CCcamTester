//
//  network.c
//  cccamtest
//

#include "network.h"

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int set_nonblock(int32_t fd, bool nonblock)
{
    int32_t flags = fcntl(fd, F_GETFL);
    if (flags == -1)
        return -1;
    if (nonblock)
        flags |= O_NONBLOCK;
    else
        flags &= (~O_NONBLOCK);
    return fcntl(fd, F_SETFL, flags);
}

int32_t network_tcp_connection_open(char* ip, int port) {
    int32_t fd;

    if((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        fd = 0;
        return -1;
    }
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    
    int32_t keep_alive = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keep_alive, sizeof(keep_alive));

    int32_t flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(flag));

    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&flag, sizeof(flag)) < 0)
    {
        fd = 0;
        return -1;
    }
    
    set_nonblock(fd,true);
    
    int32_t res = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if(res == -1)
    {
        int32_t r = -1;
        if(errno == EINPROGRESS || errno == EALREADY)
        {
            struct pollfd pfd;
            pfd.fd = fd;
            pfd.events = POLLOUT;
            int32_t rc = poll(&pfd, 1, 3000);
            if(rc > 0)
            {
                uint32_t l = sizeof(r);
                if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &r, (socklen_t *)&l) != 0)
                    { r = -1; }
                else
                    { errno = r; }
            }
            else
            {
                errno = ETIMEDOUT;
            }
        }
        if(r != 0)
        {
            // connect has failed.
            close(fd);
            fd = 0;
            return -1;
        }
    }
    
    set_nonblock(fd, false); // restore blocking mode
    return fd;
}

int32_t network_tcp_connection_close(int32_t fd) {
    if(fd)
    {
        close(fd);
        fd = 0;
    }
    return fd;
}

char* lookup_host (const char *host)
{
  struct addrinfo hints, *res, *result;
  int errcode;
  char *addrstr;
 
  if ((addrstr = malloc(sizeof(char) * 100)) == NULL)
    return (NULL);

  memset (&hints, 0, sizeof (hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags |= AI_CANONNAME;

  errcode = getaddrinfo (host, NULL, &hints, &result);
  if (errcode != 0)
    {
      perror ("getaddrinfo");
      return (NULL);
    }
  
  res = result;

  while (res)
    {
      inet_ntop (res->ai_family, res->ai_addr->sa_data, addrstr, 100);
      void *ptr;
      switch (res->ai_family)
        {
        case AF_INET6:
          ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
          break;
        default:
          ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
          break;
        }
      inet_ntop (res->ai_family, ptr, addrstr, 100);
      res = res->ai_next;
    }
  
  freeaddrinfo(result);

  return addrstr;
}