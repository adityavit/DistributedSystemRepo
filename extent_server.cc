// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>

extent_server::extent_server() {}


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  // You fill this in for Lab 2.
//  printf("extent_server:: put \n");
  if(is_inum_present(id)){
//    printf("extent_server:: put -- file found\n");
    //Found the entry in the map.
    fileDataMap[id].file_content = buf;
    fileDataMap[id].file_attr.size = fileDataMap[id].file_content.size();
    fileDataMap[id].file_attr.ctime = time(NULL);
    fileDataMap[id].file_attr.mtime = time(NULL);
    }else{
//      printf("extent_server:: put -- file not found\n");
      Filedata* file_data = new filedata;
      extent_protocol::attr* file_attrib = new extent_protocol::attr;
      file_attrib->ctime = time(NULL);
      file_attrib->mtime = time(NULL);
      file_attrib->size = buf.size();
      file_data->file_attr = *file_attrib;
      file_data->file_content = buf;
      //file_data->content_size = buf.size();
      fileDataMap[id] = *file_data; 

    }
/*     if(isfile(id)){
 *       printf("extent_server:: put -- creating a file\n");
 *       fileDataMap[id].file_content = buf;
 *       fileDataMap[id].file_attr.size = fileDataMap[id].file_content.size();
 *       fileDataMap[id].file_attr.ctime = time(NULL);
 *       fileDataMap[id].file_attr.mtime = time(NULL);
 *     }else{
 *       printf("extent_server :: put -- not creating a directory\n");
 */
/*        if(buf.compare("")!=0){
 *           std::vector<std::string> file_name_file_id;
 *           get_file_name_inum(buf,file_name_file_id);
 *           fileDataMap[id].dir_list[n2i(file_name_file_id[0])] = file_name_file_id[1];
 *         }
 *         fileDataMap[id].file_attr.ctime = time(NULL);
 *         fileDataMap[id].file_attr.mtime = time(NULL);
 *  
 */
/*     }
 *   }else{
 *     if(isfile(id)){
 *       printf("extent_server:: put -- file not found\n");
 *       Filedata* file_data = new filedata;
 *       extent_protocol::attr* file_attrib = new extent_protocol::attr;
 *       file_attrib->ctime = time(NULL);
 *       file_attrib->mtime = time(NULL);
 *       file_attrib->size = buf.size();
 *       file_data->file_attr = *file_attrib;
 *       file_data->file_content = buf;
 *       //file_data->content_size = buf.size();
 *       fileDataMap[id] = *file_data; 
 *     }else{
 *       printf("extent_server :: put -- creating a directory\n");
 *       if(buf.compare("")!=0){
 *         std::vector<std::string> file_name_file_id  = new std::vector<std::string>();
 *         get_file_name_inum(buf,file_name_file_id);
 *         fileDataMap[id].dir_list[file_name_file_id[1]] = n2i(file_name_file_id[0]);
 *       }
 *       fileDataMap[id].file_attr.ctime = time(NULL);
 *       fileDataMap[id].file_attr.mtime = time(NULL);
 *     }
 *   }
 */
//  print_directory_list();
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  // You fill this in for Lab 2.
//  printf("extent_server :: get \n");
  if(fileDataMap.find(id) != fileDataMap.end()){
//	printf("extent_server :: get -- file found\n");
//  if(isfile(id)){
    buf = fileDataMap[id].file_content;
	  fileDataMap[id].file_attr.atime = time(NULL);  
/*   }else{
 *       buf = "";
 *       map<string,extent_protocol::extentid_t>::iterator dir_it;
 *       for(dir_it = fileDataMap[id].dir_list.begin();dir_it != fileDataMap[id].dir_list.end();dir_it++)
 *         {
 *           buf += (*dir_it).first + ":" + filename((*dir_it).second) + ";";
 *       }
 *   }
 */
  return extent_protocol::OK;
  }
  return extent_protocol::IOERR;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
//  printf("extent_server:: getattr \n");
  // You fill this in for Lab 2.
  // You replace this with a real implementation. We send a phony response
  // for now because it's difficult to get FUSE to do anything (including
  // unmount) if getattr fails.
  if(fileDataMap.find(id) != fileDataMap.end()){
//	printf("extent_server:: getattr file found\n");
	 a = fileDataMap[id].file_attr;
	 return extent_protocol::OK;
	}
//  a.size = 0;
//  a.atime = 0;
//  a.mtime = 0;
//  a.ctime = 0;
  return extent_protocol::IOERR;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  // You fill this in for Lab 2.
  if(fileDataMap.find(id) != fileDataMap.end()){
//		printf("extent_server:: remove file from available in the map\n");
	}
  return extent_protocol::IOERR;
}
bool
extent_server::isfile(extent_protocol::extentid_t inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool
extent_server::isdir(extent_protocol::extentid_t inum)
{
  return ! isfile(inum);
}

void
extent_server::get_file_name_inum(string file,vector<string> &file_name_file_inum){
  size_t delim_pos = file.find(":");
  file_name_file_inum.push_back(file.substr(0,delim_pos));
  file_name_file_inum.push_back(file.substr(delim_pos));
}
extent_protocol::extentid_t 
extent_server::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string
extent_server::filename(extent_protocol::extentid_t inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

/* 
* ===  FUNCTION  ======================================================================
*         Name:  print_directory_list
*  Description:  
* =====================================================================================
*/
void
extent_server::print_directory_list()
{
  map<extent_protocol::extentid_t,Filedata>::iterator it;
  for(it = fileDataMap.begin();it!= fileDataMap.end();it++){
//      if(isfile((*it).first)){
        cout<<"extent_server :: print_directory_list file---"<<(*it).first<<"\n";
        cout<<"extent_server :: print_directory_list atime---"<<(*it).second.file_attr.atime<<"\n";
        cout<<"extent_server :: print_directory_list ctime---"<<(*it).second.file_attr.ctime<<"\n";
        cout<<"extent_server :: print_directory_list mtime---"<<(*it).second.file_attr.mtime<<"\n";
        cout<<"extent_server :: print_directory_list size---"<<(*it).second.file_attr.size<<"\n";
        cout<<"extent_server :: print_directory_list file_content--\n"<<(*it).second.file_content<<"\n";

/*       }else{
 *         cout<<"extent_server :: print_directory_list directory---"<<(*it).first<<"\n";
 *         cout<<"extent_server :: print_directory_list atime---"<<(*it).second.file_attr.atime<<"\n";
 *         cout<<"extent_server :: print_directory_list ctime---"<<(*it).second.file_attr.ctime<<"\n";
 *         cout<<"extent_server :: print_directory_list mtime---"<<(*it).second.file_attr.mtime<<"\n";
 *         map<string,extent_protocol::extentid_t>::iterator dir_it;
 *         for(dir_it = (*it).second.dir_list.begin();dir_it !=(*it).second.dir_list.end();dir_it++){
 *           cout<<"extent_server :: print_directory_list file inum---"<<filename((*dir_it).second);
 *           cout<<"extent_server :: print_directory_list file name---"<<(*dir_it).first;
 * 
 *         }
 *       }
 */
  }
}		/* -----  end of function print_directory_list  ----- */

/* 
* ===  FUNCTION  ======================================================================
*         Name:  is_inum_present
*  Description: returns true if present else returns false. 
* =====================================================================================
*/
bool
extent_server::is_inum_present (extent_protocol::extentid_t inum)
{
  return (fileDataMap.find(inum) != fileDataMap.end());
}		/* -----  end of function is_inam_present  ----- */
