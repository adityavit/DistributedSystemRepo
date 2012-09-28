//
// Lock demo
//

#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"
#include <arpa/inet.h>
#include <vector>
#include <stdlib.h>
#include <stdio.h>

std::string dst;
lock_client *lc;

int
main(int argc, char *argv[])
{
  int r;

  if(argc != 2){
    fprintf(stderr, "Usage: %s [host:]port\n", argv[0]);
    exit(1);
  }

  dst = argv[1];
  printf ( "calling lock_server\n" );
  lc = new lock_client(dst);
//  r = lc->stat(1);
//  printf ("stat returned %d\n", r);
//  r = lc->acquire(1);
//  printf("aquire returned %d\n",r);
//  r = lc->release(1);
//  printf("release returned %d\n",r);
 
  r = lc->acquire(1);
  printf("acquiring lock for 1 client %d\n",r);
  r = lc->acquire(1);
  printf("acquiring again lock for 1 client.Creates are thread in the waiting.%d\n",r);
  r = lc->release(1);
  printf("releasing lock for 1.Which was aquired first.%d\n",r);
  r = lc->acquire(2);
  printf("acquiring lock for 2.%d\n",r);
  r = lc->release(1);
  printf("releasing lock for the second 1.%d\n",r);
  r = lc->release(2);
  printf("releasing lock for 2.%d\n",r);
}
