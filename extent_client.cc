// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// The calls assume that the caller holds a lock on the extent

extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
  pthread_mutex_init(&file_data_lock,NULL);
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  cout<<"extent_client::get start"<<endl;
  extent_protocol::status ret = extent_protocol::OK;
  pthread_mutex_lock(&file_data_lock);
  if(file_data_map.find(eid) != file_data_map.end()){
      //searches in the file_data_map to check if the file_data is available in
      //the map if yes returns that.
      cout<<"extent_client::get found eid="<<eid<<" in map"<<endl;
      buf = file_data_map[eid].file_content;
      file_data_map[eid].file_attr.atime = time(NULL);
      pthread_mutex_unlock(&file_data_lock);
  }else{
    //makes a call to the extent_server to fetch the data also makes a request
    //to fetch the getattr once it receives the data it copies the data to the
    //map and returns.
    pthread_mutex_unlock(&file_data_lock);
    cout<<"extent_client::get didn't find eid="<<eid<<" in map"<<endl;
    ret = cl->call(extent_protocol::get, eid, buf);
    cout<<"extent_client::get after the rpc get call"<<endl;
    assert(ret == extent_protocol::OK);
    extent_protocol::attr temp_attr;
    extent_protocol::status attr_ret = getattr(eid,temp_attr);
    cout<<"extent_client::get after the rpc call to getattr"<<endl;
    Filedata* file_data = new filedata;
    extent_protocol::attr* file_attrib = new extent_protocol::attr;
    memcpy(file_attrib,&temp_attr,sizeof(extent_protocol::attr));
    file_data->file_attr = *file_attrib;
    file_data->file_content = buf;
    file_data->is_dirty = false;
    file_data->is_removed = false;
    pthread_mutex_lock(&file_data_lock);
    file_data_map[eid] = *file_data;
    pthread_mutex_unlock(&file_data_lock);
    cout<<"extent_client::get added a copy to the map"<<endl;
  }
  pthread_mutex_unlock(&file_data_lock);
  cout<<"extent_client::get end"<<endl;
  return ret;
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  cout<<"extent_client::getattr start"<<endl;
  extent_protocol::status ret = extent_protocol::OK;
  pthread_mutex_lock(&file_data_lock);
  if(file_data_map.find(eid) != file_data_map.end()){
      cout<<"extent_client::getattr found eid="<<eid<<" in map"<<endl;
      //get the attributes from the cache.
      attr = file_data_map[eid].file_attr;
  }else{
      cout<<"extent_client::getattr eid="<<eid<<" not found in map"<<endl;
      //get the data from the server.
      ret = cl->call(extent_protocol::getattr,eid,attr);
      assert(ret == extent_protocol::OK);
      cout<<"extent_client::getattr for the rpc answer"<<endl;
//      attr = file_data_map[eid].file_attr;
  }
  pthread_mutex_unlock(&file_data_lock);
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  pthread_mutex_lock(&file_data_lock);
  if(file_data_map.find(eid) != file_data_map.end()){
      file_data_map[eid].file_content = buf;
      file_data_map[eid].file_attr.size = file_data_map[eid].file_content.size();
      file_data_map[eid].file_attr.ctime = time(NULL);
      file_data_map[eid].file_attr.mtime = time(NULL);
      file_data_map[eid].is_dirty = true;
  }else{
      Filedata* file_data = new filedata;
      file_data->file_content = buf;
      file_data->is_dirty = true;
      file_data->is_removed = false;
      extent_protocol::attr* file_attrib = new extent_protocol::attr;
      file_attrib->ctime = time(NULL);
      file_attrib->mtime = time(NULL);
      file_attrib->size = buf.size();
      file_data->file_attr = *file_attrib;
      file_data_map[eid] = *file_data;
  }
  pthread_mutex_unlock(&file_data_lock);
//  int r;
//  ret = cl->call(extent_protocol::put, eid, buf, r);

  extent_protocol::status ret = extent_protocol::OK;
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;
  map<extent_protocol::extentid_t,Filedata>::iterator file_data_map_it;
  pthread_mutex_lock(&file_data_lock);
  cout<<"extent_client::remove trying to remove eid--"<<eid<<endl;
  if((file_data_map_it = file_data_map.find(eid)) != file_data_map.end()){
//      delete &file_data_map_it->second.file_attr;
//      delete &file_data_map_it->second;
//      file_data_map.erase(file_data_map_it);
        file_data_map[eid].is_removed = true;
  }else{
     ret = extent_protocol::IOERR;
  }
  cout<<"extent_client::remove after the removal of eid--"<<eid<<endl;
  pthread_mutex_unlock(&file_data_lock);
//  int r;
//  ret = cl->call(extent_protocol::remove, eid, r);
  return ret;
}

extent_protocol::status
extent_client::flush(extent_protocol::extentid_t eid){
    cout<<"extent_client::flush start"<<endl;
    map<extent_protocol::extentid_t,Filedata>::iterator it;
//    pthread_mutex_lock(&file_data_lock);
    if((it = file_data_map.find(eid)) != file_data_map.end()){
        //if the is_dirty bit is true then only put the content to the server.
        if(file_data_map[eid].is_removed == true){
            extent_protocol::status ret = cl->call(extent_protocol::remove,eid);
            assert(ret == extent_protocol::OK);
        }else{
        if(file_data_map[eid].is_dirty == true){
            cout<<"extent_client::flush dirty bit true for eid="<<eid<<endl;
            int r;
            extent_protocol::status ret = cl->call(extent_protocol::put,eid,file_data_map[eid].file_content,r);
            assert(ret == extent_protocol::OK);
        }
        }
            file_data_map.erase(it);
    }
//    pthread_mutex_unlock(&file_data_lock);
    return extent_protocol::OK;
}
