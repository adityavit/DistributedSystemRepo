// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include <vector>
#include "extent_protocol.h"

using namespace std;

class extent_server {

 private:
 
	 struct filedata {
		string file_content;
		int content_size;
		map<string,extent_protocol::extentid_t> dir_list;
		extent_protocol::attr file_attr;
	 };				/* ----------  end of struct filedata  ---------- */

	 typedef struct filedata Filedata;
 	
	map<extent_protocol::extentid_t,Filedata> fileDataMap;
	void get_file_name_inum(string,vector<string> & );
 	extent_protocol::extentid_t n2i(string);
	string filename(extent_protocol::extentid_t );
	void print_directory_list(); 
	bool is_inum_present(extent_protocol::extentid_t);
 public:
  extent_server();
  bool isfile(extent_protocol::extentid_t);
  bool isdir(extent_protocol::extentid_t);
  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
};

#endif 







