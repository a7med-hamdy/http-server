#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <bits/stdc++.h>
#include "helper.h"
using namespace std;


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd;  
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr,"usage: client input_file_path\n");
        exit(1);
    }
    // read requests
    vector<request> requests = read_requests(argv[1]);
    //handle if file not found or error in path
    if(requests.size() == 0)
    {
        cout << "no requests found, possibly file path is incorrect\n";
        return 0;
    }
    
    // process each request
    string old_host = "";
    string old_port = "";
    for(auto &r : requests)
    {
        // if port and host changed create new connection else continue with the old one
        if(r.host_name.compare(old_host) != 0 || r.port_number.compare(old_port) != 0)
        {
            // close the old socket 
            close(sockfd);
            // setup a new connection with the server
            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            if ((rv = getaddrinfo(r.host_name.c_str(), r.port_number.c_str(), &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                return 1;
            }

            // loop through all the results and connect to the first we can
            for(p = servinfo; p != NULL; p = p->ai_next) {
                if ((sockfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1) {
                    perror("client: socket");
                    continue;
                }
                // connect to the server
                if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                    close(sockfd);
                    perror("client: connect");
                    continue;
                }

                break;
            }

            if (p == NULL) {
                fprintf(stderr, "client: failed to connect\n");
                return 2;
            }

            inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
                    s, sizeof s);
            printf("client: connecting to %s\n", s);

            freeaddrinfo(servinfo); // all done with this structure

            old_host = r.host_name;
            old_port = r.port_number;
        }

        if(r.type == GET) 
        {
            process_get_request(sockfd, r);
        }
        else if(r.type == POST)
        {
            process_post_request(sockfd, r);
        }
        else
        {
            cout << "unexpected Error\n";
            return -1;
        }
    }
    close(sockfd);
    printf("Client shutting down\n");
    return 0;
}
