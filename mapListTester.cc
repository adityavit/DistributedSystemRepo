/*
 * =====================================================================================
 *
 *       Filename:  mapListTester.cc
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/16/2012 05:45:38 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Aditya Bhatia (@tuubow), adityavit@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <map>
#include <string>
#include <iostream>
using namespace std;
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int
main ( int argc, char *argv[] )
{
	map<int,list<std::string> > stringMap;
 	stringMap[1] = std::list<std::string> (2);
	stringMap[1].insert(1,"hello");
	stringMap[1].insert(5,"world");
	stringMap[2] = std::list<std::string> (3,"world");	
	list<string> strings = stringMap[1];
	list<string>::iterator string_it = strings.begin();
//	while(string_it != strings.end()){
//		cout<<(*string_it);
//		if(*string_it == "hello"){
//			string_it = strings.erase(string_it);
//		}else{
//			string_it++;
//		}
//	}
	string_it = strings.begin();
	while(string_it != strings.end()){
		cout<<(*string_it);
		string_it++;
	}

        cout<< strings.size();
	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */

