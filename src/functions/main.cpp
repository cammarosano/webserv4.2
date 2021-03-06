#include "includes.hpp"

// calls poll()
// does all read() and write() operations
void do_io(FdManager &table)
{
	int n_fds;

	n_fds = poll(table.get_poll_array(), table.len(), POLL_TIMEOUT);
	if (n_fds == -1)
		return;
	for (int fd = 3; fd < table.len() && n_fds; fd++)
	{
		short revents = table.get_poll_array()[fd].revents;
		if (!revents)
			continue;
		--n_fds;
		if ((revents & (POLLIN | POLLHUP))) // fd ready for reading
		{
			if (table[fd].type == fd_listen_socket)
				accept_connection(fd, table);
			else if (table[fd].type == fd_client_socket)
				recv_from_client(fd, table);
			else if (table[fd].type == fd_read)
				read_from_fd(fd, table);
		}
		if (revents & POLLOUT) // fd ready for writing
		{
			if (table[fd].type == fd_client_socket)
				send_to_client(fd, table);
			else if (table[fd].type == fd_write)
				write_to_fd(fd, table);
		}
	}
}

void signal_handler(int)
{
	FdManager::stop = true;
}

int main(int argc, char **argv)
{
	FdManager table;

	std::signal(SIGINT, signal_handler);
	if (setup(table, argc, argv) == -1)
		return (1);
	while (!FdManager::stop)
	{
		do_io(table);
		new_requests(table);
		handle_requests(table);
		house_keeper(table);
	}
	clear_resources(table);
	return (0);
}
