#include "lock_client_cache.h"
#include "extent_client.h"

lock_release_cache_user::lock_release_cache_user(extent_client* ec){
       extent_cl = ec; 
}        

void lock_release_cache_user::dorelease(lock_protocol::lockid_t lid){
        extent_cl->flush(lid);    
}

lock_release_cache_user::~lock_release_cache_user(){
}
