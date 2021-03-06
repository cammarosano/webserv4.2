#include "StaticRH.hpp"

StaticRH::StaticRH(HttpRequest *request, FdManager &table,
				   std::string &resource_path)
	: AReqHandler(request, table), resource_path(resource_path)
{
	if (setup() == -1)
		throw std::exception();
	state = s_sending_header;
}

StaticRH::~StaticRH()
{
	close(fd_file);
	if (state >= s_sending_file)
		table.remove_fd(fd_file);
}

// get fd for file in disk and generate response header
// returns -1 if error
int StaticRH::setup()
{
	struct stat sb;

	fd_file = open(resource_path.c_str(), O_RDONLY | O_NONBLOCK);
	if (fd_file == -1)
		return (-1);
	if (fstat(fd_file, &sb) == -1)
	{
		close(fd_file);
		return (-1);
	}
	response.status_code = 200;
	response.header_fields["Content-Length"] = long_to_str(sb.st_size);
	response.header_fields["Content-Type"] =
		response.get_mime_type(resource_path);
	response.assemble_header_str();
	return (0); // ok
}

// perform the necessary sequence of steps to respond to a request
// returns 1 if response if complete
// 0 if response not yet complete
// -1 if response was aborted
int StaticRH::respond()
{
	switch (state)
	{
	case s_sending_header:
		if (send_str(response.header_str) == 0) // incomplete
			return (0);
		if (request->method == "HEAD")
		{
			state = s_done;
			return (1);
		}
		table.add_fd_read(fd_file, client);
		state = s_sending_file;

	case s_sending_file:
		if (!table[fd_file].is_EOF) // incomplete
			return (0);
		state = s_done;

	default: // case s_done
		return (1);
	}
}
