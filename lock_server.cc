// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

using namespace std;

lock_server::lock_server():
  nacquire (0)
{
  pthread_mutex_init(&statusMutex,NULL);
  pthread_cond_init(&statusCheckCond,NULL);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  lock_server::acquire
 *  Description:  
 * =====================================================================================
 */
lock_protocol::status	
lock_server::acquire (int clt,lock_protocol::lockid_t lid,int &r)
{
  lock_protocol::status returnValue = lock_protocol::NOENT;
  pthread_mutex_lock(&statusMutex);
	if(threadStatusMap.find(lid) != threadStatusMap.end()){
    if(threadStatusMap[lid]==0){
//      printf("lock_server::acquire::Lock Already present for the %llu with value %d\n",lid,threadStatusMap[lid]);
	  	threadStatusMap[lid] = 1;
		  returnValue = lock_protocol::OK;
    }else{
      while(threadStatusMap[lid]==1){
//        printf("acquire::thread %llu waiting\n",lid);   
        pthread_cond_wait(&statusCheckCond,&statusMutex);
        }
//      printf("acquire::thread for lid %llu wokenup\n",lid);
      threadStatusMap[lid] = 1;
      returnValue = lock_protocol::OK;
    }
	}else{
//    printf("acquire::No lock present already for %llu\n",lid);
    threadStatusMap[lid] = 1;
    returnValue=lock_protocol::OK;
  }
  pthread_mutex_unlock(&statusMutex);
//  for(map<int,int>::iterator it=threadStatusMap.begin();it != threadStatusMap.end(); ++it){
//     printf("acquire:: %d ==> %d\n",(*it).first,(*it).second);
//  }
	return returnValue;
}		/* -----  end of function lock_server::acquire  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  lock_server::release
 *  Description:  
 * =====================================================================================
 */
	lock_protocol::status
lock_server::release ( int clt,lock_protocol::lockid_t lid,int &r )
{
  lock_protocol::status returnValue = lock_protocol::NOENT;
  pthread_mutex_lock(&statusMutex);
	if(threadStatusMap.find(lid) != threadStatusMap.end() && threadStatusMap[lid] == 1){
//    printf("lock_server::release:: lock already present %llu with value %d\n", lid, threadStatusMap[lid]);
		threadStatusMap[lid] = 0;
		returnValue = lock_protocol::OK;
//    printf("lock_server::release:: lock thread for lid = %llu signalled\n",lid);
    pthread_cond_signal(&statusCheckCond);
	}
  pthread_mutex_unlock(&statusMutex);
//  for(map<int,int>::iterator it=threadStatusMap.begin();it != threadStatusMap.end(); ++it){
//     printf("release::%d ==> %d\n",(*it).first,(*it).second);
//  }
	return returnValue; 
}		/* -----  end of function lock_server::release  ----- */
