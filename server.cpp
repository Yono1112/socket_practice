#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <unistd.h>
#include <poll.h>
#include <vector>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUF_SIZE 1024

void	exit_error(const std::string& msg) {
	std::cerr << "ERROR: " << msg << std::endl;
	std::exit(1);
}

int main () {
	int server_sockfd;

	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sockfd == -1) {
		exit_error("socket");
	}
	std::cout << "SUCCESS: socket: " << server_sockfd << std::endl;

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	if (bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		close(server_sockfd);
		exit_error("bind");
	}
	std::cout << "SUCCESS: bind" << std::endl;

	if (listen(server_sockfd, SOMAXCONN)) {
		close(server_sockfd);
		exit_error("listen");
	}
	std::cout << "SUCCESS: listen" << std::endl;

	std::vector<struct pollfd> vec;
	struct pollfd server_pollfd;
	server_pollfd.fd = server_sockfd;
	server_pollfd.events = POLLIN;
	server_pollfd.revents = 0;
	vec.push_back(server_pollfd);

	while (1) {
		int ret = poll(vec.data(), static_cast<nfds_t>(vec.size()), -1);
		if (ret == -1) {
			close(server_sockfd);
			exit_error("poll");
		}

		// std::cout << "poll ret: " << ret << std::endl;
		for (std::size_t i = 0; i < vec.size(); ++i) {
			if (vec[i].revents & POLLIN) {
				if (vec[i].fd == server_sockfd) {
					struct sockaddr_in client_addr;
					struct pollfd client_pollfd;
					socklen_t client_addr_len = sizeof(client_addr);
					client_pollfd.fd = accept(server_sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
					if (client_pollfd.fd == -1) {
						close(server_sockfd);
						exit_error("accept");
					}
					std::cout << "SUCCESS: connection from client[" << client_pollfd.fd << "]" << std::endl;
					client_pollfd.events = POLLIN;
					client_pollfd.revents = 0;
					vec.push_back(client_pollfd);
				} else {
					while (1) {
						char recv_msg[BUF_SIZE] = {0};
						ssize_t recv_size = recv(vec[i].fd, &recv_msg, BUF_SIZE, MSG_DONTWAIT);
						if (recv_size == -1) {
								if (errno == EAGAIN) {
									break ;
								}
							close(vec[i].fd);
							close(server_sockfd);
							exit_error("recv");
						} else if (recv_size == 0) {
							std::cout << "finish connection from client[" << vec[i].fd << "]" << std::endl;
							close(vec[i].fd);
							vec.erase(vec.begin() + i);
							break ;
						}

						std::cout << "client_fd[" << vec[i].fd << "]: \"" << recv_msg << "\"" << std::endl;
						int send_size = send(vec[i].fd, &recv_msg, std::strlen(recv_msg), MSG_DONTWAIT);
						if (send_size == -1) {
								if (errno == EAGAIN) {
									break ;
								}
							std::cerr << "ERROR: send" << std::endl;
							break ;
						}
					}
				}
			}
		}
	}
	close(server_sockfd);
	return 0;
}
