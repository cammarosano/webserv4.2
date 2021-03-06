#include "CgiPostRH.hpp"

CgiPostRH::CgiPostRH(HttpRequest *request, FdManager &table,
					 std::string &script_path)
	: ACgiRH(request, table, script_path), bd(*request)
{
	if (setup() == -1)
		throw(std::exception());

	// resolve body size limit
	if (!request->route->body_size_limit) // 0 means no limit.
		limit_body = false;
	else
	{
		limit_body = true;
		max_body_size = request->route->body_size_limit;
	}

	// check if 100-continue is expected
	state = s_start;
	if (response100_expected())
	{
		response.assemble_100_continue_str();
		state = s_send_100_response;
	}
}

CgiPostRH::~CgiPostRH()
{
	if (state < s_recv_cgi_header)
		close(cgi_input_fd);
	if (state > s_start && state < s_recv_cgi_header)
		table.remove_fd(cgi_input_fd);
	if (state > s_sending_body2cgi && state < s_done)
		table.remove_fd(cgi_output_fd);
	close(cgi_output_fd);
	if (waitpid(cgi_process, NULL, WNOHANG) == 0)
		child_processes.push_back(cgi_process);
}

int CgiPostRH::setup()
{
	int pipe_in[2];
	int pipe_out[2];

	// open pipe for cgi input
	if (pipe(pipe_in) == -1)
	{
		perror("pipe");
		return (-1);
	}

	// open pipe for cgi output
	if (pipe(pipe_out) == -1)
	{
		perror("pipe");
		close(pipe_in[0]);
		close(pipe_in[1]);
		return (-1);
	}

	// make pipe_out read-end and pipe_in write-end non-blocking
	if (fcntl(pipe_out[0], F_SETFL, O_NONBLOCK) == -1 ||
		fcntl(pipe_in[1], F_SETFL, O_NONBLOCK) == -1)
	{
		perror("fcntl");
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		return (-1);
	}

	// fork
	cgi_process = fork();
	if (cgi_process == -1)
	{
		perror("fork");
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		return (-1);
	}

	if (cgi_process == 0) // child process
	{
		// setup argv and envp for execve
		char **argv = setup_cgi_argv();
		char **envp = setup_cgi_env();

		// redirect stdin to the read-end of pipe_in
		if (dup2(pipe_in[0], 0) == -1)
		{
			perror("dup2");
			exit(1);
		}

		// redirect stdout to the write-end of pipe_out
		if (dup2(pipe_out[1], 1) == -1)
		{
			perror("dup2");
			exit(1);
		}
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		close(2);

		// chdir to cgi root
		chdir(request->route->root.c_str());

		execve(argv[0], argv, envp);
		perror("exec"); // if exec returns, that's an error
		exit(1);
	}

	// parent process
	close(pipe_out[1]); // close pipe out write end
	close(pipe_in[0]);	// close pipe in read end
	cgi_output_fd = pipe_out[0];
	cgi_input_fd = pipe_in[1];
	return (0);
}

int CgiPostRH::respond()
{
	int ret_bd;

	switch (state)
	{
	case s_send_100_response:
		if (send_str(response.header_str) == 0) // incomplete
			return (0);
		state = s_start;

	case s_start:
		table.add_fd_write(cgi_input_fd, client);
		state = s_recv_req_body;

	case s_recv_req_body:
		ret_bd = bd.decode_body();
		if (ret_bd == -1) // error
			return (400);
		if (bd.getLengthDecoded() > max_body_size)
			return (413);
		if (!client.decoded_body.empty()) // there's data to be sent to CGI
			table.set_pollout(cgi_input_fd);
		if (ret_bd == 0) // not finished
			return (0);
		state = s_sending_body2cgi;

	case s_sending_body2cgi:
		if (!client.decoded_body.empty()) // not finished
			return (0);
		close(cgi_input_fd); // sends EOF
		table.remove_fd(cgi_input_fd);
		table.add_fd_read(cgi_output_fd, client);
		table.unset_pollout(client.socket); // make sure nothing's gonna be sent
		state = s_recv_cgi_header;

	case s_recv_cgi_header:
		if (client.unsent_data.empty())
		{
			if (table[cgi_output_fd].is_EOF) // CGI failed
				return (502);
			return (0);
		}
		response.status_code = 200;
		response.assemble_partial_header_str();
		// prepend header upper-part to content to be sent
		client.unsent_data.insert(0, response.header_str);
		state = s_recving_cgi_output;

	case s_recving_cgi_output:
		if (!table[cgi_output_fd].is_EOF) // not finished
			return (0);
		table.remove_fd(cgi_output_fd);
		state = s_done;

	default: // case s_done
		return (1);
	}
}

int CgiPostRH::time_out_code()
{
	if (state <= s_recv_req_body)
		return (408);
	return (504);
}
