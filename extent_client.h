// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "rpc.h"
#include <map>;

using namespace std;

class extent_client {
 private:
  rpcc *cl;

  struct filedata {
      string file_content;
      extent_protocol::attr file_attr;
      bool is_dirty;
      bool is_removed;
  };				/* ----------  end of struct filedata  ---------- */

  typedef struct filedata Filedata;
  map<extent_protocol::extentid_t,Filedata> file_data_map;
  pthread_mutex_t file_data_lock;

 public:
  extent_client(std::string dst);

  extent_protocol::status get(extent_protocol::extentid_t eid, 
			      std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				  extent_protocol::attr &a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
  extent_protocol::status flush(extent_protocol::extentid_t eid);
};

#endif 

