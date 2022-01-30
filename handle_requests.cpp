#include "includes.hpp"

// creates a HttpResponse object, and adds to the clients response queue 
int handle_get_request(HttpRequest &request, Vserver &vserver, Route &route)
{
	struct stat sb;
	std::string ressource_path;
	int fd;

	(void)vserver; // TODO: use default_403 and 404 for these responses
	(void)route; // TODO: check if this route accepts the GET method

	// assemble ressource_path
	ressource_path = route.root + '/' +
								request.target.substr(route.prefix.length());
	// check if ressource is available
	if (stat(ressource_path.c_str(), &sb) == -1) // not found 
		return (issue_404_response(request));
	if (S_ISDIR(sb.st_mode)) // is a directory
		return (issue_403_response(request)); // TODO: directory listing, default index...
	fd = open(ressource_path.c_str(), O_RDONLY);
	if (fd == -1) // cannot open
		return (issue_403_response(request));

	return (issue_200_response(request, fd, sb));	
}

// transforms HttpRequests into HttpResponses
int handle_requests(std::queue<HttpRequest> &q)
{
	while (!q.empty())
	{
		HttpRequest &request = q.front();
		Vserver &vserver = resolve_vserver(request);
		Route *route = resolve_route(vserver, request.target);

		if (!route)
			issue_404_response(request);
		else if (request.method == "GET")
			handle_get_request(request, vserver, *route);
		
		// else: unsupported method. for now, simply discard it

		q.pop();
	}
	return (0);
}
