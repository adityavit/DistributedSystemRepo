// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"
#include <pthread.h>
using namespace std;

class lock_server {

 protected:
  int nacquire;
  int lock;
  pthread_mutex_t statusMutex;
  pthread_cond_t statusCheckCond;
  map<int,int> threadStatusMap;
 public:
  lock_server();
  ~lock_server() {};
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt,lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt,lock_protocol::lockid_t lid, int &);
};

#endif 







