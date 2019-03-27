

/*
 * On every iteration client should send to every
 * known node:   1; "my_name:my_ip_address:port:file1,file2,...";
 * Server checks if he has this name, add it if no. Then hi might
 * request a transfer(one or more?) like this:    0; file1;
 */


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include "structs.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP_ADDRESS   "127.0.0.1"
#define PORT     8839
#define NAME "Daniel"


char data_buffer[2048];
struct host_struct hosts[N_HOSTS];
int host_head = -1;
bool syn = true; // initially server waits for syn
char files[N_FILES][N_FILE_NAME];


bool is_file_synced(char filename[N_FILE_NAME]) {
    for (int i=0; i<N_FILES; i++) {
        if (!strcmp(files[i], filename)) {
            return true;
        }
    }
    return false;
}

void add_new_host(struct sockaddr_in client_addr, char name[]) {
    int found = 0;
    for (int i=0; i<N_HOSTS; i++) {
        if (hosts[i].host.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
            found = 1;
            break;
        }
    }
    if (!found) { // new node is found!
        host_head++;
        hosts[host_head].host = client_addr;
        memcpy(hosts[host_head].name, name, 100);
        printf("Got new known node! %s:%s:%u\n", name, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }
}

void add_new_host_from_char(char host[100]) {
    char delim = ':';
    char name[40];
    char ip[10];
    char port[6];

    int i=0; int j=0;
    while (host[i] != delim) {
        name[i] = host[i];
        i++;
    }
    i++;
    while (host[i] != delim) {
        ip[j] = host[i];
        i++;
        j++;
    }
    i++;
    j = 0;
    while (host[i] != delim) {
        port[j] = host[i];
        i++;
        j++;
    }

    int prt = atoi(port);
    struct sockaddr_in new_host;
    new_host.sin_family = AF_INET;
    new_host.sin_port = prt;
    struct hostent *adr = (struct hostent *)gethostbyname(ip);
    new_host.sin_addr = *((struct in_addr *)adr->h_addr);
    add_new_host(new_host, name);

}

char* sync_files(char client_info[]) {
    char delim = ':';
    char file_delim = ',';
    int found_delim = 0;
    int i = 0; int j = 0;
    char filename[N_FILE_NAME];
    for (; i<100; i++) {
        if (found_delim == 3) {
            filename[j] = client_info[i];
            j++;
        }
        else if (client_info[i] == delim)
            found_delim++;
    }
    return filename;
}

void setup_tcp_communication() {

    int host_id = 0;
    bool send_me = true;
    printf("[Client]: Starting to ping every node...\n");

    while(1) {
        char client_data[100];
        strcpy(client_data, NAME);
        strcat(client_data, ":");

        int sockfd = 0,
                sent_recv_bytes = 0;

        int addr_len = 0;

        addr_len = sizeof(struct sockaddr);

        /*pick next host in host list*/
        struct sockaddr_in dest = hosts[host_id].host;

        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        connect(sockfd, (struct sockaddr *) &dest, sizeof(struct sockaddr));

        char result[N_FILE_NAME];

        strcat(client_data, SERVER_IP_ADDRESS);
        strcat(client_data, ":");
        char str_port[10];
        sprintf(str_port, "%d", PORT);
        strcat(client_data, str_port);
        for (int i=0; i<N_FILES; i++) {
            if (strcmp(files[i], "")) {  // if not empty
                strcat(client_data, ":");
                strcat(client_data, files[i]);
            }
        }
        char syn_flag[] = "1";
        char host_n[2];
        sprintf(host_n, "%d", host_head + 1);

        sendto(sockfd, // SYN
               &syn_flag,
               1, 0,
               (struct sockaddr *) &dest,
               sizeof(struct sockaddr));

        sendto(sockfd,  // name:address:port:files..
                                 &client_data,
                                 100, 0,
                                 (struct sockaddr *) &dest,
                                 sizeof(struct sockaddr));

        sendto(sockfd, // number of my hosts
               &host_n,
               2, 0,
               (struct sockaddr *) &dest,
               sizeof(struct sockaddr));

        for(int j=0; j<host_head+1; j++) {  // send all my known hosts
            char h[100];
            strcpy(h, hosts[j].name);
            strcat(h, ":");
            strcat(h, inet_ntoa(hosts[j].host.sin_addr));
            strcat(h, ":");
            char str_port[10];
            sprintf(str_port, "%d", ntohs(hosts[j].host.sin_port));
            strcat(h, str_port);
            sendto(sockfd, // number of my hosts
                   &h,
                   100, 0,
                   (struct sockaddr *) &dest,
                   sizeof(struct sockaddr));
        }

        sent_recv_bytes = recvfrom(sockfd, &result, N_FILE_NAME, 0,
                                   (struct sockaddr *) &dest, &addr_len);
        if (sent_recv_bytes >= 100) {
            char filename[N_FILE_NAME];
            for (int i=0; i<100; i++)
                filename[i] = result[i+1];
            printf("[Client]: rcvd %d bytes. rcvd %s\n", sent_recv_bytes, result);
            FILE *fp;
            fp = fopen(filename, "r");
            if (fp == NULL) {
                perror("Error while opening the file.\n");
                exit(EXIT_FAILURE);
            }
            int n_words = 0;
            char word[40];
            char words[30][40];

            while(fscanf(fp, "%s", word) != EOF) {
                memcpy(words[n_words], word, 40);
                n_words ++;
            }
            fclose(fp);
            char s_n_words[2];
            sprintf(s_n_words, "%d", n_words);

            sendto(sockfd, // sending number of words in the file
                   &s_n_words,
                   2, 0,
                   (struct sockaddr *) &dest,
                   sizeof(struct sockaddr));

            for (int i=0; i<n_words; i++) { // sending word at a time to the requester
                sendto(sockfd,
                       &words[i],
                       40, 0,
                       (struct sockaddr *) &dest,
                       sizeof(struct sockaddr));
            }
        }

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
        printf("[Server]: socket creation failed\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = PORT;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    addr_len = sizeof(struct sockaddr);

    if (bind(master_sock_tcp_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1) {
        printf("[Server]: socket bind failed\n");
        return;
    }

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(master_sock_tcp_fd, (struct sockaddr *)&sin, &len) == -1)
        perror("[Server]: getsockname");
    else
        printf("[Server]: port number %d\n", sin.sin_port);

    if (listen(master_sock_tcp_fd, 5) < 0) {
        printf("[Server]: listen failed\n");
        return;
    }

    while (1) {

        FD_ZERO(&readfds);
        FD_SET(master_sock_tcp_fd, &readfds);

        select(master_sock_tcp_fd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(master_sock_tcp_fd, &readfds)) {

            comm_socket_fd = accept(master_sock_tcp_fd, (struct sockaddr *) &client_addr, &addr_len);
            if (comm_socket_fd < 0) {

                printf("[Server]: accept error : errno = %d\n", errno);
                continue;
            }

            memset(data_buffer, 0, sizeof(data_buffer));

            sent_recv_bytes = recvfrom(comm_socket_fd, (char *) data_buffer, sizeof(data_buffer), 0,
                                       (struct sockaddr *) &client_addr, &addr_len);

            printf("%d/%s\n", sent_recv_bytes, data_buffer);

            if (sent_recv_bytes >= 203) {

                printf("[Server]: recvd %d bytes from client %s:%u\n", sent_recv_bytes,
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                char client[100]; char num_hosts;

                for (int i=0; i<100; i++) { // reading client's info
                    client[i] = data_buffer[i+1];
                }

                num_hosts = data_buffer[101]; // reading number of hosts client knows
                int n_hosts = num_hosts - '0';

                for (int j=0; j<n_hosts; j++) { // reading received hosts client has
                    int adr = 103 + j*100;
                    if (adr >= sent_recv_bytes)
                        break;
                    char h[100];
                    for (int k=0; k<100; k++) {
                        h[k] = data_buffer[adr+k];
                    }
                    printf("this is h: %s\n", h);
                    add_new_host_from_char(h); // synchronise given hosts: add them if you don't have 'em already

                }

                char* filename = sync_files(client); // reading received client's file names
                char filename_[100];
                strcpy(filename_, filename);

                char req_flag[] = "0";

                sendto(comm_socket_fd,  // send request flag
                       &req_flag,
                       1, 0,
                       (struct sockaddr *) &client_addr,
                       sizeof(struct sockaddr));

                sendto(comm_socket_fd,  // send filename
                       &filename_,
                       100, 0,
                       (struct sockaddr *) &client_addr,
                       sizeof(struct sockaddr));


                printf("[Server] filename:   %s\n", filename);
            }

//            else if (sent_recv_bytes > 2 && sent_recv_bytes < 100) { // receiving file
//                printf("[Server] got file: %s\n", data_buffer);
//
//                char ok_msg[] = "okay";
//                sendto(comm_socket_fd,
//                       &ok_msg,
//                       4, 0,
//                       (struct sockaddr *) &client_addr,
//                       sizeof(struct sockaddr));
//            }

            else {
                char ok_msg[] = "okay";
                sendto(comm_socket_fd,
                       &ok_msg,
                       4, 0,
                       (struct sockaddr *) &client_addr,
                       sizeof(struct sockaddr));
            }

        }
    }
}


int main()
{

    // hard code one host
    struct sockaddr_in host;
    host.sin_family = AF_INET;
    host.sin_port = PORT;
    struct hostent *adr = (struct hostent *)gethostbyname(SERVER_IP_ADDRESS);
    host.sin_addr = *((struct in_addr *)adr->h_addr);
    add_new_host(host, NAME);
    memcpy(files[0], "tim.txt", 7);

    pthread_t server_thread_id; // server thread goes here
    pthread_create(&server_thread_id, NULL, setup_tcp_server_communication, NULL);
    setup_tcp_communication(); // client goes here
    exit(0);

}







