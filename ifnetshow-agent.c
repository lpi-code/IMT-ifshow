#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdbool.h>
#include <linux/if_link.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <unistd.h>
#include <sys/types.h>
// Hashtable

// Include error() function
#define IFNETSHOW_PORT 5050

// Usage :
// Server daemon

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int netmask_to_cidr(struct sockaddr *netmask, int family)
{
    unsigned int cidr = 0;
    unsigned int mask = 0;
    if (family == AF_INET) {
        mask = ((struct sockaddr_in *)netmask)->sin_addr.s_addr;
    while (mask) {
        cidr += mask & 1;
        mask >>= 1;
    }
    } else if (family == AF_INET6) {
        struct in6_addr *addr = &((struct sockaddr_in6 *)netmask)->sin6_addr;
        for (int i = 0; i < 16; i++) {
            unsigned char byte = addr->s6_addr[i];
            while (byte) {
                cidr += byte & 1;
                byte >>= 1;
            }
        }
    }
    return cidr;

}

bool is_str_in_list(char * str, char * list[], int list_len){
    int i = 0;
    printf("Checking %s ", str);
    while (i < list_len && list[i] != NULL && !(strcmp(str, list[i]) == 0)){
       i++;
    }
    return i < list_len && list[i] != NULL && (strcmp(str, list[i]) == 0);
}

char * get_ifname_info(char * ifname){
    char * ifinfo = malloc(2048);
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    ifa = ifaddr;
    bool alreadyPrinted = false;
    while(ifa != NULL){
        if (strcmp(ifa->ifa_name, ifname) == 0) {
            family = ifa->ifa_addr->sa_family;
            if (family == AF_INET || family == AF_INET6) {
                s = getnameinfo(ifa->ifa_addr,
                                (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                      sizeof(struct sockaddr_in6),
                                host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                if (s != 0) {
                    printf("getnameinfo() failed: %s\n", gai_strerror(s));
                    exit(EXIT_FAILURE);
                }
                if (!alreadyPrinted) {
                    sprintf(ifinfo, "Interface : %s\n", ifa->ifa_name);
                } 
                if (family == AF_INET) {
                    sprintf(ifinfo, "%s\tIpV4 : %s/%d\n",ifinfo, host, netmask_to_cidr(ifa->ifa_netmask, family));
                    
                } else {
                 sprintf(ifinfo, "%s\tIpV6 : %s/%d\n",ifinfo, host, netmask_to_cidr(ifa->ifa_netmask, family));
                }
            alreadyPrinted = true;
            }
        }
        ifa = ifa->ifa_next;
    }
    freeifaddrs(ifaddr);
    if (!alreadyPrinted) {
        sprintf(ifinfo, "Interface : %s not found", ifname);
    }
    return ifinfo;
}

char * get_all_ifinfo(){
    char * ifinfo = malloc(2048);
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    ifa = ifaddr;
    // Array of string
    char * alreadyPrintedIf[100];
    // Fill array with NULL
    memset(alreadyPrintedIf, 0, 100);
    int i = 0;
    while(ifa != NULL && i < 100){
        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                  sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
        if (!is_str_in_list(ifa->ifa_name, alreadyPrintedIf, i)){
            sprintf(ifinfo, "%s%s\n",ifinfo, get_ifname_info(ifa->ifa_name));
            alreadyPrintedIf[i++] = ifa->ifa_name;
        }
        }
        ifa = ifa->ifa_next;
    }
    freeifaddrs(ifaddr);
    return ifinfo;
}

void server_listen_loop(){
    int sockfd, newsockfd, portno, clilen;
    char buffer[2048];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    portno = IFNETSHOW_PORT;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    int opt = 1;
    setsockopt(sockfd,SOL_SOCKET, SOL_SOCKET | SO_REUSEADDR, &opt, sizeof(opt));
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 5);
    while (1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd,
                           (struct sockaddr *) &cli_addr,
                           &clilen);
        
        printf("New connection from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        if (newsockfd < 0)
            error("ERROR on accept");
        bzero(buffer,2048);
        n = read(newsockfd,buffer,2048);
        if (n < 0) error("ERROR reading from socket");
        // if buffer is empty, send all interfaces info
        // else send only the interface info
        char * ifinfo;
        if (strcmp(buffer, "\n")) {
            ifinfo = get_all_ifinfo();
        } else {
            ifinfo = get_ifname_info(buffer);
        }
        n = send(newsockfd, ifinfo, strlen(ifinfo), 0);
        if (n < 0) error("ERROR writing to socket");
        close(newsockfd);
    }
    close(sockfd);
    shutdown(sockfd, SHUT_RDWR);
}

int main (int argc, char *argv[])
{
    while (1) {
        server_listen_loop();
    }
}