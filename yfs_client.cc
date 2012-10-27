// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include "lock_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
using namespace std;

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
  lc = new lock_client(lock_dst);
  srand ( getpid() );
//  printf("yfs_client:: adding root directory\n");
  inum root_dir_inum = 0x00000001;
  extent_protocol::status status= ec->put(root_dir_inum,"");
  printf("yfs_client:: root directory created--- %d\n",status);
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string
yfs_client::filename(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool
yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the file lock

//  printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
//  printf("getfile %016llx -> sz %llu\n", inum, fin.size);

 release:

  return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the directory lock

//  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  return r;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  yfs_client::create
 *  Description:  
 * =====================================================================================
 */
int
yfs_client::create ( inum inum, std::string  name , yfs_client::inum* finum)
{
//  printf("yfs_client:: create \n");
//  printf("yfs_client::create -- creating a file %16llx \n",inum);
//  cout << "yfs_client:: create -- file or directory" << isfile(inum) <<"\n";
//  printf("yfs_client::create -- adding file to extent server\n");
//  cout<<"yfs_client:: create -- parent file name"<< filename(inum)<<"\n";
  yfs_client::inum file_inum = generate_file_inum();
  *finum = file_inum;
  yfs_client::status return_status;
  string buffer;
  lc->acquire(inum);
  ec->get(inum,buffer);
		if(buffer.find(name) != string::npos){
			//file already present in the directory
			//Do nothing return EXISTS
			return_status = EXIST; 
		}else{
			//file doesn't exist then create file.
						
  			extent_protocol::status status= ec->put(file_inum,"");
//			printf("yfs_client :: create --- file added\n");
  			string dir_content = buffer + filename(file_inum)+":"+name +";";
//			cout<<"yfs_client :: create -- dir buf"<<dir_content<<"\n";
			status = ec->put(inum,dir_content);
			printf("yfs_client::create file created--- %d\n",status);
            cout<<"yfs_client::create dir content --- "<<dir_content<<"\n";
		    return_status = OK;

		}				
    lc->release(inum);
    return return_status;
 }		/* -----  end of function yfs_client::create  ----- */

 /* 
 * ===  FUNCTION  ======================================================================
 *         Name:  yfs_client::create_dir
 *  Description:  Creates a directory inside a parent directory.
 * =====================================================================================
 */
int
yfs_client::create_dir (inum parent_inum,std::string dir_name,inum* dirinum )
{
    yfs_client::inum dir_inum = generate_dir_inum();
    *dirinum = dir_inum;
    yfs_client::status return_status;
    string buffer;
    lc->acquire(parent_inum);
    ec->get(parent_inum,buffer);
        if(buffer.find(dir_name) != string::npos){
            //file already present in the directory
            //Do nothing return EXISTS
            return_status =  EXIST;
        }else{
            //dir doesn't exist then create a dir.
            extent_protocol::status status = ec->put(dir_inum,"");
            string parent_dir_content = buffer + filename(dir_inum)+":"+dir_name +";";
            status = ec->put(parent_inum,parent_dir_content);
            printf("yfs_client::mkdir directory created --- %d\n",status);
            cout<<"yfs_client::mkdir parent_dir_content \n"<<parent_dir_content<<"\n";
            return_status = OK;
        }
        lc->release(parent_inum);
        return return_status;
}		/* -----  end of function yfs_client::mkdir  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  yfs_client::lookup
 *  Description:  
 * =====================================================================================
 */
int
yfs_client::lookup ( inum parent_inum, string file_name,inum* finum )
{
	string buffer;
	string file_inum;
	ec->get(parent_inum,buffer);
	size_t file_name_start = buffer.find(file_name+";");
	if(file_name_start != string::npos){
		//file exits now find the inum in the list of the files in the parent.
//		cout<<"yfs_client::lookup file found --"<<file_name <<"\n";
//		printf("yfs_client::file_start_position ---- %d\n",file_name_start);
//		size_t file_entry_end = buffer.find(";",file_name_start);
	//	size_t file_inum_token_start = buffer.find(":",file_name_start);
		int indexer = file_name_start - 1;
		while(indexer != -1 && buffer.at(indexer) != ';'){
			indexer--;
		}
//		printf("yfs_client::lookup file_inum_token_start %d --- file_inum_end_token --- %d\n",file_inum_token_start,file_entry_end);
		if(indexer != -1){
                	file_inum = buffer.substr(indexer+1,file_name_start-1-indexer);
		}else{
			file_inum = buffer.substr(indexer+1,file_name_start-1);
		}
//		cout<<"yfs_client::lookup file_inum-----"<<file_inum<<"\n";
		*finum = n2i(file_inum);
		return OK;
	}
//	cout<<"yfs_client::lookup file not found---"<<file_name<<"\n";
	return IOERR;
}
		/* -----  end of function yfs_client::lookup  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  yfs_client::unlink
 *  Description:  
 * =====================================================================================
 */
int
yfs_client::unlink (inum parent_inum,string file_name)
{
    cout<<"yfs_client::unlink---parent_inum"<<parent_inum<<"\n";
    cout<<"yfs_client::unlink---file_name"<<file_name<<"\n";
    inum file_inum;
    yfs_client::status status = lookup(parent_inum,file_name,&file_inum);
    yfs_client::status return_status;
    cout<<"yfs_client::unlink --- file_inum "<<file_inum<<"\n";
    if(status == yfs_client::OK){
        lc->acquire(parent_inum);
        string parent_buffer;
        ec->get(parent_inum,parent_buffer);
        cout<<"yfs_client::unlink---parent_dir_name\n "<<parent_buffer<<"\n";
        string file_inum_str = filename(file_inum);
        string::iterator file_name_position = parent_buffer.begin() + parent_buffer.find(file_name+";");
        string::iterator file_inum_position = parent_buffer.begin() + parent_buffer.find(file_inum_str);
        string::iterator file_token_end_position;
        for(string::iterator it = file_name_position;it!= parent_buffer.end();it++){
            if(*it == ';'){
                file_token_end_position = it;
                break;
            }
        }
        string dir_list = parent_buffer.replace(file_inum_position,file_token_end_position+1,"");
        cout<<"yfs_client::unlink--parent dir list\n "<<dir_list;
        ec->put(parent_inum,dir_list);
        removeFiles(file_inum);
        lc->release(parent_inum);
        return_status = OK;
    }else{
        return_status =  NOENT;
    }
    return return_status;
}		/* -----  end of function yfs_client::unlink  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  yfs_client::removeFiles
 *  Description:  
 * =====================================================================================
 */
void
yfs_client::removeFiles (inum file_inum)
{
    if(isfile(file_inum)){
        ec->remove(file_inum);
    }else{
//        string buffer;
//        ec->get(file_inum,buffer);
//        string::iterator it;
//        size_t delim_starting_pos = 0;
//        string dir_file_inum = "";
//        for(it = buffer.begin();it!= buffer.end();it++){
//            if(*it == ':'){
//                dir_file_inum = buffer.substr(delim_starting_pos,it-delim_starting_pos);
//                delim_starting_pos = it+1;
//                removeFiles(n2i(dir_file_inum);
//            }
//        }
//        ec->remove(file_inum);
    }
    return;
}		/* -----  end of function yfs_client::removeFiles  ----- */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  yfs_client::readdir
 *  Description:  
 * =====================================================================================
 */
void
yfs_client::readdir ( inum file_inum,string &buffer)
{
	ec->get(file_inum,buffer);	
}		/* -----  end of function yfs_client::readdir  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  yfs_client::get_file
 *  Description:  
 * =====================================================================================
 */
void
yfs_client::get_file (inum file_inum,string& buffer)
{
	ec->get(file_inum,buffer);
}		/* -----  end of function yfs_client::get_file  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  yfs_client::setattr
 *  Description:  
 * =====================================================================================
 */
yfs_client::status
yfs_client::setattr (inum file_inum,unsigned int file_size)
{
//	cout<<"yfs::client:: Inside setattr method\n";
	char paddingCharacter = '\0';
	string buffer;
        string final_buffer;
        get_file(file_inum,buffer);
	if(buffer.size() > file_size){
//		cout<<"yfs::client when buffer size is greater than setattr size\n";
		final_buffer = string(buffer,0,file_size);
	}else if(buffer.size() < file_size){
//		cout<<"yfs::client when buffer size is less than setattr size\n";
		final_buffer = buffer + string(file_size-buffer.size(),paddingCharacter);
	}else{
//		cout<<"yfs::client when buffer size is same as setattr size\n";
		final_buffer = buffer;
	}
        yfs_client::status status = ec->put(file_inum,final_buffer);
	return status;
}		/* -----  end of function yfs_client::setattr  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  yfs_client::read
 *  Description:  
 * =====================================================================================
 */
	void
yfs_client::read (inum file_inum,size_t offset,size_t buffer_size,string& buffer )
{
//	cout<<"yfs_client::read file read -- at offset--"<<offset<<"--buffer_size--"<<buffer_size<<"\n";
	string file_data;
    lc->acquire(file_inum);
	get_file(file_inum,file_data);
        buffer = "";
	if(offset < file_data.size()){
		buffer = file_data.substr(offset,buffer_size);
	}	
    lc->release(file_inum);
}		/* -----  end of function yfs_client::read  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  yfs_client::write
 *  Description:  
 * =====================================================================================
 */
void
yfs_client::write (inum file_inum,const char* buffer,size_t buffer_size,size_t offset)
{
//	char paddingChar = '0';
	string file_data;
	string write_string;
	string buffer_str = buffer;
    lc->acquire(file_inum);
	ec->get(file_inum,file_data);
//	cout<<"yfs_client::write -- offset--"<<offset<<"--buffer--"<<buffer<<"--buffer_size--"<<buffer_size<<"file-data---size---"<<file_data.size()<<"\n";
	if(buffer_str.size() >= buffer_size){
		write_string = buffer_str.substr(0,buffer_size);
	}else{
		write_string = buffer_str.append(buffer_size - buffer_str.size(),'\0');
	}
	if(file_data.size() < offset){
		file_data.append(offset - file_data.size(),'\0');
		file_data.append(write_string);
	}
	else if(file_data.size() <=offset+buffer_size){
		file_data = file_data.substr(0,offset);
		file_data.append(write_string);
	}else{
		file_data.replace(offset,buffer_size,write_string);
	}
	ec->put(file_inum,file_data);
    lc->release(file_inum);
}		/* -----  end of function yfs_client::write  ----- */

yfs_client::inum 
yfs_client::generate_inum(){
  int random_inum = rand();
  inum generated_value = 0LL;
  generated_value = generated_value | random_inum;
  return generated_value;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  generate_file_inum
 *  Description:  
 * =====================================================================================
 */
yfs_client::inum 
yfs_client::generate_file_inum ()
{
	return generate_inum() | 0x80000000;
}		/* -----  end of function generate_file_inum  ----- */
 
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  generate_dir_inum
 *  Description:  
 * =====================================================================================
 */
yfs_client::inum 
yfs_client::generate_dir_inum ()
{
	return generate_inum() & 0x7FFFFFFF;
}		/* -----  end of function generate_dir_inum  ----- */
