// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
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
  srand ( time(NULL) );
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
  if(isfile(file_inum)){
//	printf("yfs_client:: create -- file inum %16llx \n",file_inum);		
	}else{
//	printf("yfs_client:: error generating file inum\n");
   }
  string buffer;
  ec->get(inum,buffer);
		if(buffer.find(name) != string::npos){
			//file already present in the directory
			//Do nothing return EXISTS
			return EXIST; 
		}else{
			//file doesn't exist then create file.
						
  			extent_protocol::status status= ec->put(file_inum,"");
//			printf("yfs_client :: create --- file added\n");
  			string dir_content = buffer + filename(file_inum)+":"+name +";";
//			cout<<"yfs_client :: create -- dir buf"<<dir_content<<"\n";
			status = ec->put(inum,dir_content);
			printf("yfs_client:: file created--- %d\n",status);
			return OK;

		}				
	return IOERR; 
 }		/* -----  end of function yfs_client::create  ----- */
 
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
	size_t file_name_start = buffer.find(file_name);
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
	get_file(file_inum,file_data);
        buffer = "";
	if(offset < file_data.size()){
		buffer = file_data.substr(offset,buffer_size);
	}	
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
