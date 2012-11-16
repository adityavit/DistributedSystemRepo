#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>

#include <map>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"
using namespace std;

class lock_server_cache {
 private:
  int nacquire;

  struct lock_status {
      int status;
      list <string> waiting_client;
      string lock_client;
      map<string,bool> revoke_map;
  };				/* ----------  end of struct lock_status  ---------- */

  typedef struct lock_status Lock_status;

  struct lock_client {
    string client_id;
    lock_protocol::lockid_t lid;

  };				/* ----------  end of struct lock_client  ---------- */

  typedef struct lock_client Lock_client;
  pthread_mutex_t statusMutex;
  pthread_mutex_t retryMutex;
  pthread_mutex_t revokeMutex;
  pthread_cond_t retryCheckCond;
  pthread_cond_t revokeCheckCond;
  pthread_cond_t statusCheckCond;
  map <lock_protocol::lockid_t,Lock_status*> lock_status_map;
  list <Lock_client*> retry_list;
  list <Lock_client*> revoke_list;
  map <lock_protocol::lockid_t,string> next_lock_client;
  map <string,rpcc*> client_map;
  //functions
  public:
  lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  void retryer();
  void revoker();
  int acquire(lock_protocol::lockid_t, std::string id, int &);
  int release(lock_protocol::lockid_t, std::string id, int &);
  lock_protocol::status attach(std::string id, int &);
  lock_protocol::status unattach(std::string id, int &);
};
void* retryThread(void*);
void* revokeThread(void*);
 
#endif
