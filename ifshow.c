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
// Usage :
// ifshow -i ifname 
// => Show interface Ipv4 and Ipv6 address
// ifshow -a (all interfaces)
// => Show all interfaces Ipv4 and Ipv6 address
// ifshow -h (help)

void usage (void)
{
    printf("Usage: \n\tifshow -i ifname\n\tifshow -a (all interfaces)\n\tifshow -h (help)\n");
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


void show_interface_addr(char * interfaceName){
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
        if (strcmp(ifa->ifa_name, interfaceName) == 0) {
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
                if (!alreadyPrinted) {
                    printf("Showing interface %s\n", interfaceName);
                }
                // CIDR
                // if (family == AF_INET) {
                //     printf("\tIPv4: %s/%d", host, ifa->ifa_netmask);

                if (family == AF_INET) {
                    printf("\tIPv4: %s/%d\n", host, netmask_to_cidr(ifa->ifa_netmask, family));
                } else if (family == AF_INET6) {
                    // Remove %interfaceName
                    char *p = strchr(host, '%');
                    if (p != NULL) {
                        *p = '\0';
                    }
                    printf("\tIPv6: %s/%d\n", host, netmask_to_cidr(ifa->ifa_netmask, family));
                }
            alreadyPrinted = true;
            }
        }
        ifa = ifa->ifa_next;
    }
    if (!alreadyPrinted) {
        printf("Interface %s no address found\n", interfaceName);
    }
}

int main (int argc, char *argv[])
{
    
    if (argc < 2) {
        usage();
        return 0;
    }
    
    if (strcmp(argv[1], "-i") == 0) {
        if (argc < 3) {
            usage();
            return 0;
        }
        char *ifname = argv[2];
        printf("ifname = %s\n", ifname);
        show_interface_addr(ifname);
    } else if (strcmp(argv[1], "-a") == 0) {
        printf("Show all interfaces\n");
        // Listing all interfaces
        struct ifaddrs *ifaddr, *ifa;
        int family, s;
        char host[NI_MAXHOST];
        if (getifaddrs(&ifaddr) == -1) {
            perror("getifaddrs");
            exit(EXIT_FAILURE);
        }
        ifa = ifaddr;
        while(ifa != NULL){
            show_interface_addr(ifa->ifa_name);
            ifa = ifa->ifa_next;
            printf("\n");
        }

    } else if (strcmp(argv[1], "-h") == 0) {
        usage();
    } else {
        usage();
    }
}