
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
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
/*Server process is running on this port no. Client has to send data to this port no*/
#define PORT     8824
#define FILE_SUCCESS "Everything's synchronised"


char data_buffer[2048];
struct sockaddr_in hosts[N_HOSTS];
int host_head = -1;
char files[N_FILES][N_FILE_NAME];


bool is_host_in_array(struct sockaddr_in val){
    for (int i=0; i < N_HOSTS; i++) {
        if (hosts[i].sin_addr.s_addr == val.sin_addr.s_addr)
            return true;
    }
    return false;
}

bool is_file_synced(char filename[N_FILE_NAME]) {
    for (int i=0; i<N_FILES; i++) {
        if (!strcmp(files[i], filename)) {
            return true;
        }
    }
    return false;
}

void add_new_host(struct sockaddr_in client_addr) {
    client_addr.sin_port = PORT;
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

bool sync_hosts(struct sockaddr_in new_hosts[]) {
    // synchronises hosts lists
    bool equal = true;
    for (int i=0; i<N_HOSTS; i++) {
        if (is_host_in_array(new_hosts[i]))
            continue;
        else {
            add_new_host(hosts[i]);
            equal = false;
        }
    }
    return equal;
}

void setup_tcp_communication() {

    int host_id = 0;
    printf("[Client]: Starting to ping every node...\n");

    while(1) {
        client_struct_t client_data;

        int sockfd = 0,
                sent_recv_bytes = 0;

        int addr_len = 0;

        addr_len = sizeof(struct sockaddr);

        /*pick next host in host list*/
        struct sockaddr_in dest = hosts[host_id];
        printf("[Client]: Now ping %s\n", inet_ntoa(dest.sin_addr));

        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        connect(sockfd, (struct sockaddr *) &dest, sizeof(struct sockaddr));

        memcpy(&client_data.hosts, &hosts, N_HOSTS * sizeof(hosts[0])); // send the list of known hosts

        memcpy(&client_data.files, &files, N_FILES * N_FILE_NAME);
        memcpy(client_data.files[0], "tim.txt", 7); // hard code one file

        sendto(sockfd,
                                 &client_data,
                                 sizeof(client_struct_t),
                                 0,
                                 (struct sockaddr *) &dest,
                                 sizeof(struct sockaddr));

        char result[N_FILE_NAME];

        recvfrom(sockfd, &result, N_FILE_NAME, 0,
                                   (struct sockaddr *) &dest, &addr_len);

        if (strcmp(result, FILE_SUCCESS)) { // node doesn't have particular file
            FILE *fp;
            fp = fopen(result, "r");
            if (fp == NULL) {
                perror("Error while opening the file.\n");
                exit(EXIT_FAILURE);
            }
            int n_words = 0;
            char word[40];
            file_struct_t file_content;
            memcpy(file_content.filename, result, N_FILE_NAME);

            while(fscanf(fp, "%s", word) != EOF) {
                memcpy(file_content.words[n_words], word, 40);
                n_words ++;
            }
            fclose(fp);
            printf("words: %d\n", n_words);
            file_content.n_words = n_words;


            sent_recv_bytes = sendto(sockfd,
                   &file_content, sizeof(file_struct_t), 0, (struct sockaddr *) &dest, sizeof(struct sockaddr));
            printf("[Client]: sending # of words, sent %d bytes\n", sent_recv_bytes);
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

        printf("[Server]: blocked on select System call...\n");

        select(master_sock_tcp_fd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(master_sock_tcp_fd, &readfds)) {

            comm_socket_fd = accept(master_sock_tcp_fd, (struct sockaddr *) &client_addr, &addr_len);
            if (comm_socket_fd < 0) {

                printf("[Server]: accept error : errno = %d\n", errno);
                exit(0);
            }

            printf("[Server]: Connection accepted from client : %s:%u\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            /* ADDING NEW HOST TO THE LIST OF KNOWN HOSTS (IF IT ISN'T THERE ALREADY) */
            add_new_host(client_addr);

            while (1) {
                memset(data_buffer, 0, sizeof(data_buffer));

                sent_recv_bytes = recvfrom(comm_socket_fd, (char *) data_buffer, sizeof(data_buffer), 0,
                                           (struct sockaddr *) &client_addr, &addr_len);

                printf("[Server]: recvd %d bytes from client %s:%u\n", sent_recv_bytes,
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                if (sent_recv_bytes == sizeof(file_struct_t)) { // got file content!
                    file_struct_t *file_content = (file_struct_t *) data_buffer;

                    FILE *fp;
                    fp = fopen(file_content->filename, "w");
                    if(fp == NULL) {
                        /* File not created hence exit */
                        printf("Unable to create file.\n");
                        exit(EXIT_FAILURE);
                    }
                    printf("[Server]: recieved file: \n");
                    for (int j=0; j<file_content->n_words; j++) {
                        printf("%s ", file_content->words[j]);
                        fprintf(fp, "%s ", file_content->words[j]);
                    }
                    printf("\n");
                    fclose(fp);
                    continue;
                }

                if (sent_recv_bytes == 0 || sent_recv_bytes == -1) {
                    close(comm_socket_fd);
                    break;

                }

                client_struct_t *client_data = (client_struct_t *) data_buffer;
                sync_hosts(client_data->hosts);

                char reply[100] = FILE_SUCCESS;
                bool all_synced = true;
                for (int i=0; i<N_FILES; i++) {
                    if (!is_file_synced(client_data->files[i])) {  // if a file is not found in the list of known files
                        all_synced = false;
                        // acknowledge that server doesn't have such a file
                        memcpy(reply, client_data->files[i], N_FILE_NAME);
                        sendto(comm_socket_fd, &reply, 100, 0, // send a name of not found file
                                         (struct sockaddr *) &client_addr, sizeof(struct sockaddr));

                    }
                }
                if (all_synced) {
                    sendto(comm_socket_fd, &reply, 100, 0,
                                             (struct sockaddr *) &client_addr, sizeof(struct sockaddr));
                }

            }
        }
    }
}


int main()
{

    // hard code one host
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = PORT;
    struct hostent *host = (struct hostent *)gethostbyname(SERVER_IP_ADDRESS);
    addr.sin_addr = *((struct in_addr *)host->h_addr);
    add_new_host(addr);

    pthread_t server_thread_id; // server thread goes here
    pthread_create(&server_thread_id, NULL, setup_tcp_server_communication, NULL);
    setup_tcp_communication(); // client goes here
    exit(0);

}







