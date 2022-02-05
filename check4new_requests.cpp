#include "includes.hpp"
#include "FdManager.hpp"
#include "StaticRH.hpp"
#include "HttpRequest.hpp"

// extracts data from the client's received_data buffer into a HttpRequest object
// return NULL if buffer does not contain a complete request header
// TODO: watch out for request bodies that end with an empty line
// eventual trailing CRLF must be removed!!
HttpRequest * new_HttpRequest(Client &client)
{
	// look for end-of-header delimiter: 2CRLF
	size_t pos = client.received_data.find("\r\n\r\n");

	if (pos == std::string::npos) // not found: header is incomplete
		return (NULL);

	// header is complete: consume data
	std::string header_str = client.received_data.substr(0, pos);
	client.received_data.erase(0, pos + 4);

	// debug
	std::cout << "---------\nThe following request header was received:\n"
				<< header_str << "\n" << std::endl;
	
	// create HttpRequest object
	return new HttpRequest(client, header_str);
}

// request.route cannot be NULL
std::string assemble_ressource_path(HttpRequest &request)
{
	std::string route_root = request.route->root;
	std::string route_prefix = request.route->prefix;
	std::string path = route_root + '/' + 
						request.target.substr(route_prefix.length());
	// debug
	std::cout << "ressource path: " << path << std::endl;

	return (path);

}

// resolve type of response: static_file, CGI, directory, error...
ARequestHandler *init_response(HttpRequest &request, FdManager &table)
{
	std::string resource_path;
	struct stat sb;

	// assemble ressource path
	if (!request.route)
		return (NULL); // todo new ErrorRH(404);
	resource_path = assemble_ressource_path(request);

	// check if ressource is available
	if (stat(resource_path.c_str(), &sb) == -1) // not found
		return (NULL); // todo new ErrorRH(404);
	
	// check if it is a directory
	if (S_ISDIR(sb.st_mode)) // is a directory
		return (NULL); // todo new DirectoryRH
	
	// TODO: check if CGI response (match extension)

	return (new StaticRH(&request, table, resource_path));
}

int check4new_requests(FdManager &table,
						std::list<ARequestHandler*> &req_handlers_lst)
{
	// iterate over clients in recv_header state
	for (int fd = 3; fd < table.len(); ++fd)
	{
		if (table[fd].type != fd_client_socket)
			continue;

		Client &client = *table[fd].client;

		if (client.state != recv_header || client.received_data.empty())
			continue;
		HttpRequest *request = new_HttpRequest(client);
		if (!request)
			continue;
		ARequestHandler *req_handler = init_response(*request, table);
		req_handlers_lst.push_back(req_handler);
		client.state = handling_response;
		client.ongoing_response = req_handler;
	}
	return (0);
}
