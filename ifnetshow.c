#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdbool.h>
// optarg
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define IFNETSHOW_PORT 5050
// Ifnetshow client
// Usage :
// ifnetshow -n addr -i ifname : Show interface ifname information of client of ip address addr
// ifnetshow -n addr -a : Show all interfaces information of client of ip address addr
// Quick and dirty code ^^
void usage (void)
{
    printf("Usage: \n\tifnetshow -n addr -i ifname\n\tifnetshow -n addr -a (all interfaces)\n");
}

char * get_ifname_by_addr(char * addr, char * ifname)
{
    // Connect to addr on port IFNETSHOW_PORT
    // Establish connection
    printf("Connecting to %s:%d\n", addr, IFNETSHOW_PORT);
    int connection_socket;

    connection_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (connection_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Define the address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(IFNETSHOW_PORT);
    server_address.sin_addr.s_addr = inet_addr(addr);

    // Connect to the server
    printf("Connecting to %s:%d\n", addr, IFNETSHOW_PORT);
    int connection_status = connect(connection_socket, (struct sockaddr *) &server_address, sizeof(server_address));
    if (connection_status == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    // Send ifname
    printf("Sending ifname: %s\n", ifname);
    write(connection_socket, ifname, strlen(ifname));
    // Receive ifname
    char server_response[2048];
    read(connection_socket, &server_response, sizeof(server_response));
    // Print the server's response
    // Close the socket
    close(connection_socket);
    char * output = malloc(sizeof(server_response));
    strcpy(output, server_response);
    close(connection_socket);
    return output;
}

int main(int argc, char *argv[])
{
    int opt;
    char *addr = NULL;
    char *ifname = NULL;
    bool all = false;
    while ((opt = getopt(argc, argv, "n:i:ah")) != -1) {
        switch (opt) {
        case 'n':
            addr = optarg;
            break;
        case 'i':
            ifname = optarg;
            break;
        case 'a':
            all = true;
            break;
        case 'h':
            usage();
            exit(EXIT_SUCCESS);
        default:
            usage();
            exit(EXIT_FAILURE);
        }
    }
    if (addr == NULL) {
        usage();
        exit(EXIT_FAILURE);
    }
    if (ifname == NULL && all == false) {
        usage();
        exit(EXIT_FAILURE);
    }
    if (ifname != NULL && all == true) {
        usage();
        exit(EXIT_FAILURE);
    }

    if (ifname != NULL) {
        char * output = get_ifname_by_addr(addr, ifname);
        printf("%s", output);
    }
    if (all) {
        printf("All interfaces:\n");
        char * output = get_ifname_by_addr(addr, "_");
        printf("%s", output);
    }

    return 0;
}