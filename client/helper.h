#ifndef HELPER_H
#define HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>
#include <arpa/inet.h>

#include <bits/stdc++.h>
using namespace std;

#define POST 1
#define GET 0
#define MAXDATASIZE 1000 // max number of bytes we can get at once 
#define timeout 5000

struct request{
	char type;
	string path;
	string host_name;
	string port_number;	
};

vector<request> read_requests(char*);
void print_requests(vector<request>);
void process_get_request(int, request);
void process_post_request(int, request);
void send_to_socket(int, char *, int);
char* read_from_socket(int, char*, bool, const std::string&, int);

#endif