
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  //Header file for sleep(). man 3 sleep for details.
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
// #include <zconf.h>
#include "structs.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP_ADDRESS   "127.0.0.1"

/*Server process is running on this port no. Client has to send data to this port no*/
#define SERVER_PORT     8822
#define N_HOSTS

test_struct_t test_struct;
result_struct_t res_struct;
char data_buffer[1024];
struct sockaddr_in hosts[N_HOSTS];
int host_head = -1;

test_struct_t client_data;
result_struct_t result;


void add_new_host(struct sockaddr_in client_addr) {
    client_addr.sin_port = SERVER_PORT;
    int found = 0;
    for (int i=0; i<N_HOSTS; i++) {
        if (hosts[i].sin_addr.s_addr == client_addr.sin_addr.s_addr) {
            found = 1;
            break;
        }
    }
    if (!found) { // new node is found!
        host_head++;
        hosts[host_head] = client_addr;
        printf("Got new known node! %s:%u\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }
}


bool equal_host_lists(struct sockaddr_in hosts1, struct sockaddr_in hosts2) {
    // check if lists are the same, otherwise return false and add nodes that don't have
    bool equal = true;
    for (int i=0; i<N_HOSTS; i++) {
        if (hosts1[i].sin_addr.s_addr == hosts2[i].sin_addr.s_addr)
            continue;
        else {
            add_new_host(hosts2[i]);
            equal = false;
        }
    }
    return equal;
}


void setup_tcp_communication() {

    int host_id = 0;
    printf("I am client and I am starting to ping every node I know\n");

    while(1) {
        int sockfd = 0,
                sent_recv_bytes = 0;

        int addr_len = 0;

        addr_len = sizeof(struct sockaddr);

        /*pick next host in host list*/
        struct sockaddr_in dest = hosts[host_id];

        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


        connect(sockfd, (struct sockaddr *) &dest, sizeof(struct sockaddr));

        client_data.hosts = hosts; // synchronise list of known nodes

        sendto(sockfd,
                                 &client_data,
                                 sizeof(test_struct_t),
                                 0,
                                 (struct sockaddr *) &dest,
                                 sizeof(struct sockaddr));

        recvfrom(sockfd, (char *) &result, sizeof(result_struct_t), 0,
                                   (struct sockaddr *) &dest, &addr_len);

        int alive = 0; // is pinged host alive or not
        if (result.value == client_data.value + 1) {
            alive = 1;
        }
        printf("alive = %d\n",alive);
        close(sockfd);

        if (host_id < host_head) { // go through the list of hosts and ping them
            host_id ++;
        }
        else {
            host_id = 0;
        }
    }

}


void
setup_tcp_server_communication() {

    int master_sock_tcp_fd = 0,
            sent_recv_bytes = 0,
            addr_len = 0,
            opt = 1;

    int comm_socket_fd = 0;
    fd_set readfds;
    struct sockaddr_in server_addr,
            client_addr;

    if ((master_sock_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("socket creation failed\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = SERVER_PORT;


    server_addr.sin_addr.s_addr = INADDR_ANY;

    addr_len = sizeof(struct sockaddr);

    if (bind(master_sock_tcp_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1) {
        printf("socket bind failed\n");
        return;
    }

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(master_sock_tcp_fd, (struct sockaddr *)&sin, &len) == -1)
        perror("getsockname");
    else
        printf("port number %d\n", sin.sin_port);


    if (listen(master_sock_tcp_fd, 5) < 0) {
        printf("listen failed\n");
        return;
    }

    while (1) {

        FD_ZERO(&readfds);
        FD_SET(master_sock_tcp_fd, &readfds);

        printf("blocked on select System call...\n");

        select(master_sock_tcp_fd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(master_sock_tcp_fd, &readfds)) {

            comm_socket_fd = accept(master_sock_tcp_fd, (struct sockaddr *) &client_addr, &addr_len);
            if (comm_socket_fd < 0) {

                printf("accept error : errno = %d\n", errno);
                exit(0);
            }

            printf("Connection accepted from client : %s:%u\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            /* ADDING NEW HOST TO THE LIST OF KNOWN HOSTS (IF IT ISN'T THERE ALREADY) */
            add_new_host(client_addr);

            while (1) {
                printf("Server ready to service client msgs.\n");
                memset(data_buffer, 0, sizeof(data_buffer));

                sent_recv_bytes = recvfrom(comm_socket_fd, (char *) data_buffer, sizeof(data_buffer), 0,
                                           (struct sockaddr *) &client_addr, &addr_len);

                printf("Server recvd %d bytes from client %s:%u\n", sent_recv_bytes,
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                if (sent_recv_bytes == 0) {
                    close(comm_socket_fd);
                    break;

                }

                test_struct_t *client_data = (test_struct_t *) data_buffer;

                result_struct_t result;

                int eq = equal_host_lists(hosts, client_data->hosts));

                sent_recv_bytes = sendto(comm_socket_fd, (char *) &result, sizeof(result_struct_t), 0,
                                         (struct sockaddr *) &client_addr, sizeof(struct sockaddr));

                printf("Server sent %d bytes in reply to client\n", sent_recv_bytes);
            }
        }
    }
}
 s

int main()
{

    // hard code one host
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = SERVER_PORT;
    struct hostent *host = (struct hostent *)gethostbyname(SERVER_IP_ADDRESS);
    addr.sin_addr = *((struct in_addr *)host->h_addr);
    add_new_host(addr);

    pthread_t server_thread_id; // server thread goes here
    pthread_create(&server_thread_id, NULL, setup_tcp_server_communication, NULL);
    setup_tcp_communication(); // client goes here
    exit(0);

}







