#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "ARequestHandler.hpp"
#include "config.hpp"
#include "macros.h"
#include <arpa/inet.h>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <queue>
#include <set>
#include <sys/socket.h>
#include <unistd.h>

// forward declarations
class AReqHandler;
struct HttpRequest;

struct Client
{
	enum e_state
	{
		idle,
		incoming_request,
		ongoing_response
	};

	int socket;
	std::list<Vserver> &vservers;
	std::string ipv4_addr;
	std::string host_name;

	bool disconnect_after_send;

	// buffers
	std::string received_data;
	std::string unsent_data;
	std::string decoded_body;

	// ongoing response
	HttpRequest *request;
	AReqHandler *request_handler;

	// time-outs
	time_t last_state_change;

	// state checks and changes
	void update_state();
	void update_state(e_state new_state);
	bool is_idle();
	bool is_incoming_request();
	bool is_ongoing_response();
	std::list<Client *>::iterator list_node;

	Client(int socket, sockaddr sa, std::list<Vserver> &vservers);
	~Client();

	// static lists
	static std::list<Client *> idle_clients;
	static std::list<Client *> incoming_req_clients;
	static std::list<Client *> ongoing_resp_clients;

	// instances counter
	static int counter;

  private:
	e_state state;

	void get_client_info(sockaddr &sa);
};

#endif