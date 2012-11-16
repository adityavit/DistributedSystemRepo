// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"
#include "rpc.h"
#include <pthread.h>

#define OPEN 0
#define LOCKED 1

void* retryThread(void* lock_server){
    lock_server_cache* server = (lock_server_cache*) lock_server;
    server->retryer();
}


void* revokerThread(void* lock_server){
    lock_server_cache* server = (lock_server_cache*) lock_server;
    server->revoker();
    }


lock_server_cache::lock_server_cache()
{
    pthread_t threads[2];
    pthread_mutex_init(&statusMutex,NULL);
    pthread_mutex_init(&retryMutex,NULL);
    pthread_mutex_init(&revokeMutex,NULL);
    pthread_cond_init(&statusCheckCond,NULL);
    pthread_cond_init(&retryCheckCond,NULL);
    pthread_cond_init(&revokeCheckCond,NULL);
    int thread = pthread_create(&threads[0],NULL,retryThread,(void*) this);
    thread = pthread_create(&threads[1],NULL,revokerThread,(void*) this);
}


void lock_server_cache::retryer(){
    while(1){
        pthread_mutex_lock(&retryMutex);
        if(retry_list.size()==0){
            pthread_cond_wait(&retryCheckCond,&retryMutex);
        }
            //pop the element from the retry_list and call the rpc
            //to the client.
            cout<<"lock_server_cache retry thread"<<endl;
            Lock_client* lock_cl = retry_list.front();
            pthread_mutex_unlock(&retryMutex);
            string clientHostName = lock_cl->client_id;
            rpcc *cl = client_map[clientHostName];
            int status;
            cout<<"lock_server_cache retry sending retry call clt="<<clientHostName<<" lid ="<<lock_cl->lid<<endl;
            int retStatus = cl->call(rlock_protocol::retry,lock_cl->lid,status);
            cout<<"lock_server_cache retry got response "<<retStatus<<endl; 
            pthread_mutex_lock(&retryMutex);
            retry_list.pop_front();
             pthread_mutex_unlock(&retryMutex);
    }
}

void lock_server_cache::revoker(){
    while(1){
        pthread_mutex_lock(&revokeMutex);
        if(revoke_list.size() == 0){
            pthread_cond_wait(&revokeCheckCond,&revokeMutex);
        }
            //pop the front element of the retry_list call rpc revoke for the
            //thread.
            cout<<"lock_server_cache revoker thread "<<endl;
            Lock_client* lock_cl = revoke_list.front();
            pthread_mutex_unlock(&revokeMutex);
            string clientHostName = lock_cl->client_id;
            rpcc *cl = client_map[clientHostName];
            int status;
            cout<<"lock_server_cache revoker sending revoke to clt="<<clientHostName<<endl;
            int retStatus = cl->call(rlock_protocol::revoke,lock_cl->lid,status);
            cout<<"lock_server_cache revoke response received="<<retStatus<<endl;
            pthread_mutex_lock(&revokeMutex);
            revoke_list.pop_front();
            pthread_mutex_unlock(&revokeMutex);
    }

}
int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, int &)
{
  cout<<"lock_server_cache acquire for lid="<<lid<<"from client="<<id<<endl;
  lock_protocol::status ret;
  pthread_mutex_lock(&statusMutex);
  int revoke_to_send = false;
  if(lock_status_map.find(lid) != lock_status_map.end()){
      //the lid is present in the lock_status_map.
      //Check the lid status is free or locked if open return Ok.
      //If locked return retry and call the retry method of the client which
      //has the lock.
      //In the case of retry rpc send to a client will make sure that next lock
      //for that lid is given to that client only when it sends acquire all
      //other clients will be in waiting.This will help to make sure that
      //revoke is send correctly and no other client doesn't cut in between
      //when  retry client has to send acquire.
//      bool next_client = true;//true for the first case when no retry rpc has been send.
//      if(next_lock_client.find(lid) != next_lock_client.end()){
//            //next client is available for that lock.
//            //Check the id of the client if it matches then make the lock open
//            //else make it false.
//            if(next_lock_client[lid].compare(id) == 0){
//                next_client = true;
//            }else{
//                next_client = false;
//            }
//      }
      cout<<"lock_server_cache acquire lid="<<lid<<"found in map"<<endl;
      if(lock_status_map[lid]->status == OPEN){
          cout<<"lock_server_cache acquire lid="<<lid<<"is OPEN"<<endl;
          ret = lock_protocol::OK;
          lock_status_map[lid]->status = LOCKED;
          lock_status_map[lid]->lock_client = id;
          if(lock_status_map[lid]->waiting_client.size() != 0){
              //In the case when the waiting_clients are still there and a
              //revoke has been send to the first waiting client and has to
              //send the acquire.
              //In such a case the lock client the revoke should be send to
              //that lock client as soon as the acquire is done.
              lock_status_map[lid]->revoke_map[id] = true;
              Lock_client* lock_clnt = new Lock_client;
              lock_clnt->lid = lid;
              lock_clnt->client_id = id;
              revoke_list.push_back(lock_clnt);
              revoke_to_send = true;
              cout<<"lock_server_cache acquire sending revoke send to as other waiting clients "<<lock_clnt->client_id<<endl;
          }else{
              //If no waiting client not need to send revoke.
              lock_status_map[lid]->revoke_map[id] = false;
          }
      }else{
          cout<<"lock_server_cache acquire lid="<<lid<<"is not OPEN sending retry to client="<<id<<endl;
          ret = lock_protocol::RETRY;
          if(lock_status_map[lid]->revoke_map[lock_status_map[lid]->lock_client] == false){
             //if the revoke has not been already sent to the lock_client then
             //send it else don't send the revoke to the lock_client. 
            Lock_client* lock_cl_obj = new Lock_client;
            lock_cl_obj->lid = lid;
            lock_cl_obj->client_id = lock_status_map[lid]->lock_client;
            cout<<"lock_server_cache acquire when lock is not opened list revoke send to "<<lock_cl_obj->client_id<<endl;
            revoke_list.push_back(lock_cl_obj);
            lock_status_map[lid]->revoke_map[lock_status_map[lid]->lock_client] = true;
            revoke_to_send = true;
          } 
          lock_status_map[lid]->waiting_client.push_back(id);
//          if(lock_status_map[lid]->waiting_client.size() == 0){
//            //if the waiting clients are not there then revoke will be send to
//            //the lock client.Else the revoke will be send to the last waiting
//            //client.
//            lock_status_map[lid]->waiting_client.push_back(id);
//            Lock_client* lock_cl_obj = new Lock_client;
//            lock_cl_obj->lid = lid;
//            lock_cl_obj->client_id = lock_status_map[lid]->lock_client;
//            cout<<"lock_server_cache acquire when no waiting list revoke send to "<<lock_cl_obj->client_id<<endl;
//            revoke_list.push_back(lock_cl_obj);
//          }else{
//              //Sending the revoke to the last acquire request client.
//              Lock_client* lock_cl_obj = new Lock_client;
//              lock_cl_obj->lid = lid;
//              lock_cl_obj->client_id = lock_status_map[lid]->waiting_client.back();
//              cout<<"lock_server_cache acquire revoke send to "<<lock_cl_obj->client_id<<endl;
//              revoke_list.push_back(lock_cl_obj);
//              lock_status_map[lid]->waiting_client.push_back(id);
//          }
          //signal the revoke_thread to call the revoke method of the
          //lock_client.
          //call the the rpc revoke for the lock_client.
      }
  }else{
      //Case when the lid is not present in the map in this case just create a
      //new structure for the lid store the lock_client and return the Ok.
      cout<<"lock_server_cache acquire lid="<<lid<<" is not seen in the map"<<endl;
      Lock_status* lock_status_obj = new Lock_status;
      lock_status_obj->status = LOCKED;
      lock_status_obj->lock_client = id;
      lock_status_map[lid] = lock_status_obj;
      lock_status_map[lid]->revoke_map[id] = false;
      ret = lock_protocol::OK;
  }
  if(revoke_to_send){
      //signal the revoke thread.
      cout<<"lock_server_cache acquire signalling the revoke thread to revoke the client"<<endl;
      cout<<"lock_server_cache acquire revoke_list size"<<revoke_list.size()<<endl;
      pthread_cond_signal(&revokeCheckCond);
  } 
  pthread_mutex_unlock(&statusMutex);
  cout<<"lock_server_cache acquire sending response="<<ret<<" to id="<<id<<endl;
  return ret;
}

int 
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, 
         int &r)
{
    cout<<"lock_server_cache  release for the lid="<<lid<<" from client="<<id<<endl;
    lock_protocol::status ret;
    if(lock_status_map.find(lid) != lock_status_map.end() && lock_status_map[lid]->status == LOCKED){
       //if the number of the waiting clients is zero then make status as open
       //and return OK.Else if there are clients are waiting then send retry to
       //the first one in the list.
       if(lock_status_map[lid]->waiting_client.size() == 0){
           cout<<"lock_server_cache release no client waiting for the lock"<<endl;
           pthread_mutex_lock(&statusMutex);
           ret = lock_protocol::OK;
           lock_status_map[lid]->status = OPEN;
           lock_status_map[lid]->lock_client = "";
//           if(next_lock_client.find(lid)!=next_lock_client.end()){
//                next_lock_client.erase(lid);
//           }
           lock_status_map[lid]->revoke_map[id]=false;
       }else{
           cout<<"lock_server_cache release clients waiting for the lock lid="<<lid<<endl;
           pthread_mutex_lock(&statusMutex);
           ret = lock_protocol::OK;
           lock_status_map[lid]->status = OPEN;
           lock_status_map[lid]->lock_client = "";
           lock_status_map[lid]->revoke_map[id] = false;
           string waiting_clnt = lock_status_map[lid]->waiting_client.front();
           cout<<"lock_server_cache release client waiting="<<waiting_clnt<<endl;
           lock_status_map[lid]->waiting_client.pop_front();

           //creating a new Lock_client obj
           Lock_client* lock_cl_obj = new Lock_client;
           lock_cl_obj->client_id = waiting_clnt;
           lock_cl_obj->lid = lid;
           retry_list.push_back(lock_cl_obj);
           pthread_mutex_unlock(&statusMutex);
           //signal the retry thread to call the retry method on the first
           //element of the retry_list 
           cout<<"lock_server_cache release signalling the retry thread"<<endl;
           pthread_cond_signal(&retryCheckCond);
       }
    }
    return ret;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  printf("lock_server_cache stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}

lock_protocol::status
lock_server_cache::attach(std::string id, int &r){
        pthread_mutex_lock(&statusMutex);
        sockaddr_in dstsock;
        make_sockaddr(id.c_str(),&dstsock);
        rpcc *cl = new rpcc(dstsock);
        cl->bind();
        client_map[id] = cl; 
        r =0;
        pthread_mutex_unlock(&statusMutex);
        return lock_protocol::OK;
}

lock_protocol::status
lock_server_cache::unattach(std::string id,int &r){
    pthread_mutex_lock(&statusMutex);
    delete client_map[id];
    client_map.erase(id);
    r =0;
    pthread_mutex_lock(&statusMutex);
    return lock_protocol::OK;
}
