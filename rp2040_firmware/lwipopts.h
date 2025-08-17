#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Common settings for both Wi-Fi and RNDIS
#define NO_SYS                      0
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0

// Enable features for our web server
#define LWIP_HTTPD_CGI              1
#define LWIP_HTTPD_SSI              1
#define LWIP_HTTPD_SSI_INCLUDE_TAG  0 // Use <!--#tag--> syntax

// Required for FreeRTOS
#define TCPIP_THREAD_STACKSIZE      1024
#define DEFAULT_THREAD_STACKSIZE    1024
#define DEFAULT_RAW_RECVMBOX_SIZE   8
#define TCPIP_MBOX_SIZE             8
#define LWIP_TIMEVAL_PRIVATE        0

// Memory options
#define MEM_LIBC_MALLOC             1
#define MEMP_MEM_MALLOC             1

// RNDIS/Ethernet options
#define LWIP_ETHERNET               1
#define LWIP_ARP                    1
#define LWIP_DHCP                   1

#endif
