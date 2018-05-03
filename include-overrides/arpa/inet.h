#ifndef __ARPA_INET_H
#define __ARPA_INET_H

// arpa/inet.h is used by our indirect dependency, cn-cbor. It needs it for the following functions:
//   ntohs, ntohl, htons, htonl.
//
// arm-none-eabi toolchain provides this function in machine/endian.h so we define them using these.

#include <machine/endian.h>

#define ntohs(x)  __ntohs((x))
#define ntohl(x)  __ntohl((x))
#define htons(x)  __htons((x))
#define htonl(x)  __htonl((x))

#endif
