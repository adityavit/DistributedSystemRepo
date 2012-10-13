#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>

#include "lock_protocol.h"
#include "lock_client.h"

class yfs_client {
  extent_client *ec;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  static std::string filename(inum);
  static inum generate_inum();
  static inum generate_file_inum();
  static inum generate_dir_inum();
 public:

  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &); 
  int create(inum, std::string,inum*);
  int lookup(inum,std::string,inum*);
  int setattr(inum,unsigned int);
  void readdir(inum,std::string&);
  void read(inum,size_t,size_t,std::string&);
  void write(inum,const char*,size_t,size_t);
  void get_file(inum,std::string&);
  static inum n2i(std::string);
};

#endif 
