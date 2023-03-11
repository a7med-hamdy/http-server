#include "helper.h"


/**
 * @brief 
 * 
 * @param path the path of the input file
 * @return vector<request> a vector of request struct 
 */
vector<request> read_requests(char * path)
{
	vector<request> v;
	fstream file;
	file.open(path, ios::in);
	if(file.is_open())
	{
		string buffer;
		while(getline(file, buffer))
		{
			request r;
			stringstream ss(buffer);
			string str;
			int cnt = 0;
			while(ss >> str)
			{
				// parse type
				if(cnt == 0)
				{
					if(str.compare("client_get") == 0)
					{
						r.type = GET;
					}
					else
					{
						r.type = POST;
					}
				}
				// parse path
				else if(cnt == 1)
				{
					r.path = str;
				}
				// parse hostname
				else if(cnt == 2)
				{
					r.host_name = str;
				}
				// parse port nummber
				else if(cnt == 3)
				{
					r.port_number = str;
				}
				cnt++;
			}
			// default port if not given is 80
			if(r.port_number.size() == 0)
				r.port_number = "80";
			v.push_back(r);
		}
	}
	file.close();
	return v;
}


/**
 * @brief 
 * a function that sends post header, then waits for ok from the server
 * then sends the file to be posted
 * @param sockfd : socket file descriptor
 * @param r the request struct
 */
void process_post_request(int sockfd, request r)
{
	ifstream input("files/" + r.path, std::ios::binary);
	if(!input.good())
	{
		printf("Error trying to post non-existent file.\n");
		return;
	}
	string request = "POST /" + r.path + " HTTP/1.1\r\n";
	request += "Host: " + r.host_name + ":" + r.port_number + "\r\n";
	request += "Connection : keep-alive\r\n";
	input.seekg(0, ios::end);
	int size = input.tellg();
	request += "Content-Length: " + to_string(size) + "\r\n";
	string type = r.path.substr(r.path.find(".")+1);
	//html files
	if(type.compare("html") == 0)
	{
		request += "Content-Type: text/html\r\n";
	}
	else if(type.compare("txt") == 0)//txt files
	{
		request += "Content-Type: text/plain\r\n";
	}
	else //images
	{
		request += "Content-Type: image/" + type + "\r\n";
	}
	request += "\r\n";
	input.clear();
	input.seekg(0);
	// send post header
	send_to_socket(sockfd, (char*)request.c_str(), request.size());
	char res[MAXDATASIZE];
	// wait for the ok
	char * rec = read_from_socket(sockfd, res, false, "", -1);
	char * m = "OK";
	if(strcmp(m, rec) == 0)
	{
		// if ok received read the file and send it
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


/**
 * @brief 
 * a function that sends the get request header, then waits for response from the server
 * if 404 end the requests if 200 wait for the GET response
 * @param sockfd : socket file descriptor
 * @param r the request struct
 */
void process_get_request(int sockfd, request r)
{
	string request = "GET /" + r.path + " HTTP/1.1\r\n";
	request += "Host: " + r.host_name + ":" + r.port_number + "\r\n";
	request += "Connection : keep-alive\r\n";
	request += "\r\n";
	send_to_socket(sockfd, (char*)request.c_str(), request.size());

	char buffer[MAXDATASIZE];
	char * buf = read_from_socket(sockfd, buffer, false, "", -1);
	stringstream ss(buf);
	string str;
	ss >> str;
	ss >> str;
	if(str.compare("200") == 0)
	{
		while(str.compare("Content-Length:") != 0)
		{
			ss >> str;
		}
		ss >> str;
		read_from_socket(sockfd, buffer, true, r.path, stoi(str));
	}
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
		string str = "rec/" + s; 
		remove(str.c_str()); // if the file exists delete it
		outfile.open(str, ios::binary | ios::out | ios::app);
	}
	int x = 0;
	while(1)
	{
		int numbytes;
		int nread = MAXDATASIZE;
		if(size > 0)
			nread = size > MAXDATASIZE ? MAXDATASIZE : size;
		if((numbytes = recv(sockfd, buf, nread, 0)) == -1) {
			perror("recv");
			exit(1);
		}
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
	// cout << "sent " << size << " bytes\n";
}


//helper function used for debugging
void print_requests(vector<request> r)
{
	for(auto& req: r)
	{
		cout << (int)req.type << " ," + req.path + ", " + req.host_name+ " ," + req.port_number  + "\n";
	}
}
