#ifndef CLIENT_HPP
# define CLIENT_HPP

# include "config.hpp"
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <iostream>
# include <stdio.h> // perror

// forward declaration
class ARequestHandler;

enum e_client_state { recv_header, handling_response };

struct Client {
    int socket;
    std::list<Vserver> &vservers;
    std::string ipv4_addr;
    std::string host_name;

    e_client_state state;

    // buffers
    std::string received_data;
    std::string unsent_data;
    std::string decoded_body;

    // ongoing response
    ARequestHandler *ongoing_response;

    // constructor
    Client(int socket, sockaddr sa, std::list<Vserver> &vservers);

	private:
    // get ip address and host name
    void get_client_info(sockaddr &sa);
};


#endif