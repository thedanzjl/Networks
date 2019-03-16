#define N_HOSTS 100
#define N_FILES 5
#define N_FILE_NAME 100 // number of bytes per file name

typedef struct client_struct{

    struct sockaddr_in hosts[N_HOSTS];
    char files[N_FILES][N_FILE_NAME];

} client_struct_t;


typedef struct file_struct_{

    char words[30][40];
    char filename[N_FILE_NAME];
    int n_words;

} file_struct_t;




