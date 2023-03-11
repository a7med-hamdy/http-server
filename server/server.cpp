#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <bits/stdc++.h>
#include <poll.h>
using namespace std;

#define MAXDATASIZE 1000// max number of bytes we can get at once 
#define BACKLOG 10	 // how many pending connections queue will hold



vector<thread> threads;// vector of the threads running
mutex mtx; // mutex thread for synchronization
void remove_from_list(void);
void handle_connection(int);
void handle_get(int, string);
void handle_post(int, string, int);
void send_to_socket(int, char *, int);
char* read_from_socket(int, char*, bool, const std::string&, int);


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, char * argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	int yes=1;
	int rv;

	 if (argc != 2) {
        fprintf(stderr,"usage: my_server port\n");
        exit(1);
    }

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		// bind
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}
	// listen on the port
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("server: waiting for connections...\n");
	
	while(1) {  // main accept() loop
		sin_size = sizeof(their_addr);
		// accept new connections
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}


		char s[INET6_ADDRSTRLEN];
		inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof s);
		printf("server: got connection from %s\n", s);

		// start a new thread to handle the new file descriptor		
		thread t = thread(handle_connection, new_fd);
		// add the thread to the list
		mtx.lock();
		threads.push_back(move(t));
		mtx.unlock();
	}

	return 0;
}


/**
 * @brief 
 * it is the worker thread starting point
 * @param sockfd socket file descriptor
 */
void handle_connection(int sockfd)
{
	// define structure needed by poll
	struct pollfd ufds[1];
	ufds[0].fd = sockfd;
	ufds[0].events = POLLIN;

	while(1)
	{
		// calculate the time out based on the number of clients in the system
		// determined by the number of currently working threads
		mtx.lock();
		int timeout = 10000 / threads.size();
		mtx.unlock();
		// implement persistent http by using poll
		int rv = poll(ufds, 1, timeout);
		if (rv == -1) {
		perror("poll"); // error occurred in poll()
		} else if (rv == 0) {
			printf("connection timed out !\n");
			break;
		} else {
			int numbytes;
			char buf [MAXDATASIZE];
			if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
				perror("recv");
				exit(1);
			}
			// in case client closed connection
			if(numbytes == 0)
			{
				printf("client closed connection\n");
				break;
			}
			
			// output request
			buf[numbytes] = '\0';
			cout << buf;

			stringstream ss(buf);
			string str;
			// read request type
			ss >> str;
			if(str.compare("GET") == 0)
			{
				ss >> str; // read path
				handle_get(sockfd, str);
			}
			else
			{
				ss>> str;//read path
				string path = str;
				// read content length
				while(str.compare("Content-Length:") != 0)
				{
					ss >> str;
				}
				ss >> str;
				handle_post(sockfd, path, stoi(str));
			}
			ss.clear();

		}
	}
	close(sockfd);
	remove_from_list();
}


/**
 * @brief 
 * a function that handles the get request if the file not found it sends 404 error
 * else it sends 200 OK then reads the file and sends it
 * @param sockfd socket file descriptor
 * @param path path of the file in the GET request
 */
void handle_get(int sockfd, string path)
{
	ifstream input("files" + path, std::ios::binary);
	string response = "";
	bool flag = true;
	if(!input.good())
	{
		flag = false;
		// get response header in case file not found
		response = "HTTP/1.1 404 Not Found\r\n";
		response += "Connection: keep-alive\r\n";
		response += "Content-Length: " + to_string(26) + "\r\n";
		response += "\r\n";
	}
	else
	{
		// get response header in case file found
		response = "HTTP/1.1 200 OK\r\n";
		response += "Connection: keep-alive\r\n";
		input.seekg(0, ios::end);
		int size = input.tellg();
		response += "Content-Length: " + to_string(size) + "\r\n";
		string type = path.substr(path.find_last_of(".")+1);
		//html files
		if(type.compare("html") == 0)
		{
			response += "Content-Type: text/html\r\n";
		}
		else if(type.compare("txt") == 0)//txt files
		{
			response += "Content-Type: text/plain\r\n";
		}
		else //images
		{
			response += "Content-Type: image/" + type + "\r\n";
		}
		response += "\r\n";
		input.clear();
		input.seekg(0);
	}
	// send resopnse header
	send_to_socket(sockfd, (char*)response.c_str(), response.size());
	if(flag)
	{
		// read the file as array of bytes(char)
		std::vector<char> bytes(
         (std::istreambuf_iterator<char>(input)),
         (std::istreambuf_iterator<char>()));
		input.close();

		char buf[bytes.size()];

		for(int i = 0; i < bytes.size(); i++)
		{
			buf[i] = bytes[i];
		}
		send_to_socket(sockfd, buf, bytes.size());
	}
}

/*
a function that handles post request it first sends the ok message then waits for the message body from the client
parameters:
sockfd : socket file descriptor
path : the path of the posted file
size : the size of the posted file
*/
void handle_post(int sockfd, string path, int size)
{
	send_to_socket(sockfd, "OK", 2);
	char buf[MAXDATASIZE];
	read_from_socket(sockfd, buf, true, path, size);
}

/*
a function that reads from socket
parameters:
sockfd : socket descriptor
buf : a byte buffer to read into
write: a boolean that indicates if we want to write in the given path or not
path : a path to write file into
size : the size of data to be read from the socket
returns:
the buffer after writing into it
*/
char* read_from_socket(int sockfd, char* buf, bool write, const std::string& path, int size)
{
	std::ofstream outfile;
	if(write)
	{
		string s = path;
		if(path.find_last_of("/") != string::npos)
		{
			s = path.substr(path.find_last_of("/")+1);
		}	
		string str = "files/" + s;
		remove(str.c_str()); // remove the file if it exists
		outfile.open(str, ios::binary | ios::out | ios::app);
	}
	int x = 0;
	while(1)
	{
		int numbytes, nread;
		if(size > 0)
			nread = size > MAXDATASIZE ? MAXDATASIZE : size;
		if((numbytes = recv(sockfd, buf, nread, 0)) == -1) {
			perror("recv");
			exit(1);
		}
		// if the client closed the connection
		else if(numbytes == 0)
		{
			break;
		}
		else
		{
			x += numbytes;
			buf[numbytes] = '\0';
			if(write)
			{
				// wirte to the file
				outfile.write(buf, numbytes);
			}
			else
			{
				cout << buf << "\n";
			}
			size -= numbytes;
			if(size <= 0)
				break;
		}
	}
	//cout << "Received : " << x << " bytes\n";
	outfile.close();
	return buf;
}


/*
a function that sends data to the socket
parameters:
sockfd : socket descriptor
message : the message to be sent
size : the size of data to be sent to the socket
*/
void send_to_socket(int sockfd, char * message, int size)
{
	int x = send(sockfd, message, size, 0); 
	if(x == -1)
	{
		perror("send");
		exit(0);
	}
	//cout << "sent " << size << " bytes\n";
}


/*
a function that removes the thread from the list and terminates it
*/
void remove_from_list()
{
	// remove the thread from the list
	mtx.lock();
	for(auto itr = threads.begin(); itr != threads.end(); itr++)
	{
		if(itr->get_id() == std::this_thread::get_id())
		{
			itr->detach();
			itr = threads.erase(itr);
			break;
		}
	}
	mtx.unlock();
	//printf("thread destructed\n");
}


