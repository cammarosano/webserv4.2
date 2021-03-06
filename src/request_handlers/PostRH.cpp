#include "PostRH.hpp"

PostRH::PostRH(HttpRequest *request, FdManager &table)
	: AReqHandler(request, table), bd(*request)
{
	Route *r = request->route;
	body = "{ \"success\" : \"true\" }";
	// check if upload_dir exist?
	std::string file_name =
		request->target.substr(r->prefix.length(), std::string::npos);
	file_path = r->root + '/' + r->upload_dir + '/' + file_name;
	state = s_start;
	if (request->header_fields.find("expect") != request->header_fields.end())
	{
		response.assemble_100_continue_str();
		state = s_send_100_continue;
	}
}

PostRH::~PostRH()
{
	close(fd);
	if (state >= s_receiving_body)
		table.remove_fd(fd);
}

int PostRH::respond()
{
	int ret_bd;

	switch (state)
	{
	case s_send_100_continue:
		if (send_str(response.header_str) == 0)
			return 0;
		state = s_start;
	case s_start:
		fd = open(file_path.c_str(), O_CREAT | O_WRONLY | O_NONBLOCK, 0644);
		if (fd < 0)
			return 500;
		table.add_fd_write(fd, client);
		state = s_receiving_body;
	case s_receiving_body:
		ret_bd = bd.decode_body();
		if (ret_bd == -1)
			return 400;
		if (!client.decoded_body.empty())
			table.set_pollout(fd);
		if (ret_bd == 0)
			return 0;
		state = s_assemble_header;
	case s_assemble_header:
		response.http_version = "HTTP/1.1";
		response.status_code = 201;
		response.header_fields["content-length"] = long_to_str(body.length());
		response.assemble_header_str();
		state = s_sending_header;
	case s_sending_header:
		if (send_str(response.header_str) == 0)
			return 0;
		state = s_sending_html_str;
	case s_sending_html_str:
		if (send_str(body) == 0)
			return 0;
		state = s_done;
	default:
		return 1;
	}
}
