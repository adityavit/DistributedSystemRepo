// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"
#include <map>
#include <set>
#include <string>
using namespace std;

#define DISABLED 0;
#define ENABLED 1;
void* releaserThread(void* lock_client){
   lock_client_cache* lock_cl = (lock_client_cache*) lock_client;
   lock_cl->releaser(); 
}
lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  pthread_t threads[1];
  rpcs *rlsrpc = new rpcs(0);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);
  pthread_mutex_init(&lockMap,NULL);
  pthread_mutex_init(&retryLock,NULL);
  pthread_mutex_init(&revokeMap,NULL);
  pthread_cond_init(&revoke_map_cond,NULL);
  pthread_cond_init(&retry_check_cond,NULL);
  pthread_cond_init(&lock_check_cond,NULL);
  int thread = pthread_create(&threads[0],NULL,releaserThread,(void*)this); 
  const char *hname;
  hname = "127.0.0.1";
  std::ostringstream host;
  revokerThreadStatus = DISABLED;
  host << hname << ":" << rlsrpc->port();
  id = host.str();
  int ret;
  lock_protocol::status returnSt =  cl->call(lock_protocol::attach,id,ret);
  assert(returnSt == lock_protocol::OK);
}

lock_client_cache::~lock_client_cache(){
    pthread_mutex_lock(&lockMap);
    cout<<"lock_client_cache="<<id<<" destructor releasing locks"<<endl;
    int status;
    for(map<lock_protocol::lockid_t,Lock_status*>::iterator it=lock_map.begin();
            it!=lock_map.end();it++){
        int r;
        if(it->second->lock_status != NONE){
             status = cl->call(lock_protocol::release,it->first,id,r);
             cout<<"lock_client_cache="<<id<<" destructor released lock for lid="<<it->first<<" status ="<<status<<endl;
        }
        it->second->lock_status = NONE;
    }
    pthread_mutex_unlock(&lockMap);
    int ret;
    lock_protocol::status retStatus = cl->call(lock_protocol::unattach,id,ret);
    assert(retStatus == lock_protocol::OK);
    cout<<"lock_client_cache="<<id<<" destructor done"<<endl;
}


void lock_client_cache::releaser(){
    //the releaser thread used to check the revoke map to check if there is any
    //entry where the lid is true and if so then call the release rpc from the
    //server.
      while(1){ 
        pthread_mutex_lock(&lockMap);
        if(revokerThreadStatus == 0){
            cout<<"lock_client_cache="<<id<<" releaser condition wait on the revoke_map"<<endl;
            pthread_cond_wait(&revoke_map_cond,&lockMap);
        } 
        cout<<"lock_client_cache="<<id<<"  releaser thread woken up"<<endl;
//        map<lock_protocol::lockid_t,bool> temp_revoke_map = revoke_map;
//        pthread_mutex_unlock(&revokeMap);
//        set<lock_protocol::lockid_t> removeSet;
        for(list<lock_protocol::lockid_t>::iterator it = revoke_list.begin();
                it != revoke_list.end();it++){
            lock_protocol::lockid_t lid = *it;
            cout<<"lock_client_cache="<<id<<" releaser check status of the lock with lid "<<lid<<endl;
//            cout<<"lock_client_cache="<<id<<" revoke_map entry lid="<<lid<<endl;
            cout<<"lock_client_cache="<<id<<"releaser lid="<<lid<<" lockstatus="<<lock_map[lid]->lock_status<<endl;
//            pthread_mutex_lock(&lockMap);
            if(lock_map[lid]->lock_status == RELEASING){
                lock_map[lid]->lock_status = NONE;
//                pthread_mutex_unlock(&lockMap);
               lu->dorelease(lid); 
                int r; 
                cout<<"lock_client_cache="<<id<<"  calling to server to release the lid="<<lid<<endl;
                int ret_status = cl->call(lock_protocol::release,lid,id,r);
                cout<<"lock_client_cache="<<id<<"  got the return status as ="<<ret_status<<endl;
//                pthread_mutex_lock(&revokeMap);
//                revoke_map.erase(lid);
//                pthread_mutex_unlock(&revokeMap);
//                pthread_mutex_lock(&lockMap);
//                removeSet.insert(lid);
                cout<<"lock_client_cache="<<id<<" releaser deleting the lid"<<lid<<endl;
                revoke_list.erase(it);
                it--;
                break;
           }
            cout<<"lock_client_cache="<<id<<" releaser after iterating over lid revoke_list size="<<revoke_list.size()<<endl;
        }
        cout<<"lock_client_cache="<<id<<" releaser after map iteration done"<<endl;
          revokerThreadStatus = DISABLED;
//        pthread_mutex_unlock(&lockMap);
//        pthread_mutex_lock(&revokeMap);
//        for(set<lock_protocol::lockid_t>::iterator it = removeSet.begin();
//                it!=removeSet.end();it++){
//
//            revoke_map.erase(*it);
//
//        }
//        pthread_mutex_unlock(&revokeMap);
//          pthread_mutex_lock(&lockMap);
//          revoke_map.clear();
            cout<<"lock_client_cache="<<id<<"  releaser signalling remaing threads for lock"<<endl;
            pthread_cond_signal(&lock_check_cond);
 
          pthread_mutex_unlock(&lockMap);
    }
}

lock_protocol::status lock_client_cache::call_to_server(lock_protocol::lockid_t lid){
    cout<<"lock_client_cache "<<id<<" Calling to server for the acquire"<<endl;
    int r;
    lock_protocol::status ret = cl->call(lock_protocol::acquire,lid,id,r);
    cout<<"lock_client_cache "<<id<<" getting result from server="<<ret<<endl;
    return ret;
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
  Lock_status* lid_status;
  cout<<"lock_client_cache="<<id<<" acquire lid="<<lid<<endl;
  pthread_mutex_lock(&lockMap);
  if(lock_map.find(lid) == lock_map.end()){
      //the lid is not found in the map create a new object and add it to the
      //map and make it as none.As client has no idea about the lock.
      Lock_status* lock_st = new Lock_status;
      lock_st->lock_status = NONE;
//      pthread_mutex_lock(&lockMap);
      lock_map[lid] = lock_st;
      lid_status = lock_st;
//      pthread_mutex_unlock(&lockMap);
//      pthread_mutex_lock(&retryLock);
      retry_map[lid] = false;
//      pthread_mutex_unlock(&retryLock);
      cout<<"lock_client_cache="<<id<<"  the lid not found in map"<<endl;
  }else{
//      pthread_mutex_lock(&lockMap);
      lid_status = lock_map[lid];
//      pthread_mutex_unlock(&lockMap);
      cout<<"lock_client_cache="<<id<<"  the lid found in map"<<endl;
  }
  pthread_mutex_unlock(&lockMap);
  while(1){
      //infinite loop the thread will see the state in which the lock is and
      //either will wait or will return.
      pthread_mutex_lock(&lockMap);
      switch(lock_map[lid]->lock_status){
        case NONE:{
            //The client has no idea about this lock.So request to the server
            //to fetch the status of the lock if the lock is available will get
            //OK else will get retry.
            cout<<"lock_client_cache="<<id<<"  status of the lock is NONE"<<endl;
            lock_protocol::status server_lock_status = call_to_server(lid);
            cout<<"lock_client_cache="<<id<<"  status from the server "<< server_lock_status<<endl;
            if(server_lock_status == lock_protocol::OK){
                //lock is available then return to the OK and change the lock
                //status map to the LOCKED.
//                pthread_mutex_lock(&lockMap);
                lock_map[lid]->lock_status = LOCKED;
//                pthread_mutex_unlock(&lockMap);
                cout<<"lock_cl_cache="<<id<<" lock status received OK"<<endl;
                pthread_mutex_unlock(&lockMap);
                return lock_protocol::OK;
            }else{
                //the server_lock_Status is RETRY in that case the lock is not
                //available and the client will have to wait till the time the
                //retry is received from the lock server for that lock.
                //Once retry is received the client will contact the lock
                //server again to fetch the lock status.
                cout<<"lock_cl_cache="<<id<<" lock status received RETRY"<<endl;
                while(server_lock_status == lock_protocol::RETRY){
//                    pthread_mutex_lock(&lockMap);
                    lock_map[lid]->lock_status = ACQUIRING;
//                    pthread_mutex_unlock(&lockMap);
//                    pthread_mutex_lock(&retryLock);
                    if(retry_map[lid] == false){
                        //retry request has not been received by the client from the
                        //server.The thread will wait for the retry map for lid
                        //to become true.
                        cout<<"lock_client_cache="<<id<<"  retry map no lid "<<lid<<endl;
                        pthread_cond_wait(&retry_check_cond,&lockMap);
                    }else{
                        cout<<"lock_client_cache="<<id<<" checking when the retry is true"<<endl;
                        server_lock_status = call_to_server(lid);
                        if(server_lock_status == lock_protocol::OK){
//                            pthread_mutex_lock(&lockMap);
                            cout<<"lock_client_cache="<<id<<" retry is satisfied"<<endl;
                            lock_map[lid]->lock_status = LOCKED;
                            retry_map[lid] = false;
//                            pthread_mutex_unlock(&lockMap);
                        }else{
                            retry_map[lid] = false;
                        }
                    }
//                    pthread_mutex_unlock(&retryLock);
                }
                pthread_mutex_unlock(&lockMap);
                return lock_protocol::OK;
            }
            break;
        }
        case FREE:{
           //The case when the lock is available with the client in this case
           //the lock can be given to the thread as lock is free no need to
           //request to the server.
           cout<<"lock_client_cache="<<id<<" "<<"lid="<<lid<<"is FREE"<<endl;
//           pthread_mutex_lock(&lockMap);
           lock_map[lid]->lock_status = LOCKED;
//           pthread_mutex_unlock(&lockMap);
           cout<<"lock_client_cache="<<id<<"  acquire FREE for lid ="<<lid<<" return with OK"<<endl;
           pthread_mutex_unlock(&lockMap);
           return lock_protocol::OK;
           break;
        }
        default:{
            //case when the lock is either in acquiring,releasing or locked
            //status in such a case the lock cannot be given to the thread.It
            //will have to wait till the lock is either NONE or FREE.
//            pthread_mutex_lock(&lockMap);
            if(lock_map[lid]->lock_status == LOCKED){
                cout<<"lock_client_cache="<<id<<" lid="<<lid<<" status = LOCKED"<<endl;
            }
            if(lock_map[lid]->lock_status == ACQUIRING){
                cout<<"lock_client_cache="<<id<<" lid="<<lid<<" status = ACQUIRING"<<endl;
            }
            if(lock_map[lid]->lock_status == RELEASING){
                cout<<"lock_client_cache="<<id<<" lid="<<lid<<" status = RELEASING"<<endl;
            }
            cout<<"lock_client_cache="<<id<<" acquire waiting in the default"<<endl;
            pthread_cond_wait(&lock_check_cond,&lockMap);
            cout<<"lock_client_cache="<<id<<" acquire signalled after default"<<endl;
            pthread_mutex_unlock(&lockMap);
            break;
        }
      }
  } 
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
  //The release will check the lock lid in the revoke map if it is true then it
  //will make the status as release so that no other threads can grab the lock
  //and will signal the releaser thread which will check which locks are in
  //releasing state and will release them.
  //In the case the lock is not there in the revoke map it will make the lock
  //status as FREE and will wake up the threads waiting on the condition
  //&lock_check_cond so that they can take the lock.
  cout<<"lock_client_cache="<<id<<" releasing lock with lid="<<lid<<endl;
  pthread_mutex_lock(&lockMap);
  bool found_in_revoke_list = false;
  for(list<lock_protocol::lockid_t>::iterator it= revoke_list.begin();
          it!=revoke_list.end();it++){
      if(*it == lid){
          found_in_revoke_list = true;
          break;
      }
  }
  if(found_in_revoke_list){
        cout<<"lock_client_cache="<<id<<"  releasing lid="<<lid<<"found in revoke map to be revoked"<<endl;
//        pthread_mutex_lock(&lockMap);
        lock_map[lid]->lock_status = RELEASING;
//        pthread_mutex_unlock(&lockMap);
//        pthread_mutex_unlock(&revokeMap);
        revokerThreadStatus = ENABLED;
        pthread_cond_signal(&revoke_map_cond);
  }else{
        cout<<"lock_client_cache="<<id<<"  releasing lock can be freed lid="<<lid<<endl;
//        pthread_mutex_unlock(&revokeMap);
//        pthread_mutex_lock(&lockMap);
        lock_map[lid]->lock_status = FREE;
//        pthread_mutex_unlock(&lockMap);
        cout<<"lock_client_cache="<<id<<"  release signaling waiting thread"<<endl;
        pthread_cond_signal(&lock_check_cond);
  }
  pthread_mutex_unlock(&lockMap);
  return lock_protocol::OK;

}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, 
                                  int &)
{
    cout<<"lock_client_cache "<<id<<" got REVOKE from the server for lid="<<lid<<endl;
//    pthread_mutex_lock(&revokeMap);
    pthread_mutex_lock(&lockMap);
    if(lock_map[lid]->lock_status == FREE){
        //If the lock is free then change the lock status to releasing and
        //add to revoke_map[lid] as true and signal the release thread to
        //release the lock.
        //If the lock is not free then just update the map with the lid as true
        //so that the release method can signal the release thread.
        cout<<"lock_client_cache="<<id<<"  lid="<<lid<<" is free for release"<<endl;
        revokerThreadStatus = ENABLED;
        lock_map[lid]->lock_status = RELEASING;
        revoke_list.push_back(lid);
//        revoke_map[lid] = true;
//        pthread_mutex_unlock(&lockMap);
//        pthread_mutex_unlock(&revokeMap);
        cout<<"lock_client_cache="<<id<<"  signalling the releaser thread for lid="<<lid<<endl;
        pthread_cond_signal(&revoke_map_cond);
    }else{
        cout<<"lock_client_cache="<<id<<"  lid="<<lid<<"is still not yet released"<<endl;
//        pthread_mutex_unlock(&lockMap);
            revoke_list.push_back(lid);
//        revoke_map[lid] = true;
//        pthread_mutex_unlock(&revokeMap);
    }
  pthread_mutex_unlock(&lockMap);
  return rlock_protocol::OK;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, 
                                 int &)
{
  //Will add the lid to the retry map with true and will signal the threads
  //waiting for the retry_check_cond.Which will call the server again to get
  //the lock. 
  cout<<"lock_client_cache "<<id<<" receiving a retry from server for lid="<<lid;
  pthread_mutex_lock(&lockMap);
  retry_map[lid] = true;
  pthread_mutex_unlock(&lockMap);
  pthread_cond_signal(&retry_check_cond);
  return rlock_protocol::OK;
}



