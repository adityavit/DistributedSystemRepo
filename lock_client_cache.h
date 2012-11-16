// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"
#include <map>
#include <list>
using namespace std;
// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 5.
class lock_release_user {
 public:
  virtual void dorelease(lock_protocol::lockid_t) = 0;
  virtual ~lock_release_user() {};
};

class lock_client_cache : public lock_client {
 private:
  class lock_release_user *lu;
  int rlock_port;
  std::string hostname;
  std::string id;

  enum lock_st{NONE,FREE,LOCKED,ACQUIRING,RELEASING};

  struct lock_status {
      enum lock_st lock_status;
  };				/* ----------  end of struct lock_status  ---------- */

  typedef struct lock_status Lock_status;
  map<lock_protocol::lockid_t,Lock_status*> lock_map;
  map<lock_protocol::lockid_t,bool> retry_map;
  map<lock_protocol::lockid_t,bool> revoke_map;
  list<lock_protocol::lockid_t> revoke_list;
  pthread_mutex_t lockMap;
  pthread_mutex_t retryLock;
  pthread_mutex_t revokeMap;
  pthread_cond_t revoke_map_cond;
  pthread_cond_t retry_check_cond;
  pthread_cond_t lock_check_cond; 
  lock_protocol::status call_to_server(lock_protocol::lockid_t);
  int revokerThreadStatus;
 public:
  lock_client_cache(std::string xdst, class lock_release_user *l = 0);
 virtual  ~lock_client_cache(); 
  void releaser(void);
  lock_protocol::status acquire(lock_protocol::lockid_t);
  lock_protocol::status release(lock_protocol::lockid_t);
  rlock_protocol::status revoke_handler(lock_protocol::lockid_t, 
                                        int &);
  rlock_protocol::status retry_handler(lock_protocol::lockid_t, 
                                       int &);
};

void* releaserThread(void*);
#endif
