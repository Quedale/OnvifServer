#include "network_utils.h"
#include <stdlib.h>
#include <netdb.h>
#include "clogger.h"
#include <arpa/inet.h>
#include <linux/if.h>

#define ONVIF_NET_BUFFER_SIZE 81920

struct nlreq {
	struct nlmsghdr hdr;
	struct ifinfomsg gen;
};

#ifdef NET_DEBUG
static const char *oper_states[] = {
	"UNKNOWN", "NOTPRESENT", "DOWN", "LOWERLAYERDOWN",
	"TESTING", "DORMANT",	 "UP"
};

static void
print_ifaddr_flags(__u8 flags){
    C_DEBUG(" \n\tif addr Flags\n");
    C_DEBUG("\t\tSecondary:\t%d", (flags&IFA_F_SECONDARY));
    C_DEBUG("\t\tNopad:\t\t%d", (flags&IFA_F_NODAD));
    C_DEBUG("\t\tOptimistic:\t%d", (flags&IFA_F_OPTIMISTIC));
    C_DEBUG("\t\tDadfailed:\t%d", (flags&IFA_F_DADFAILED));
    C_DEBUG("\t\tHome address:\t%d", (flags&IFA_F_HOMEADDRESS));
    C_DEBUG("\t\tDeprecated:\t%d", (flags&IFA_F_DEPRECATED));
    C_DEBUG("\t\tTentative:\t%d", (flags&IFA_F_TENTATIVE));
    C_DEBUG("\t\tDHCP:\t\t%d", !(flags&IFA_F_PERMANENT));
    C_DEBUG("\t\tManageTempAddr:\t%d", (flags&IFA_F_MANAGETEMPADDR));
    C_DEBUG("\t\tNoprefixroute:\t%d", (flags&IFA_F_NOPREFIXROUTE));
    C_DEBUG("\t\tmacautojoin:\t%d", (flags&IFA_F_MCAUTOJOIN));
    C_DEBUG("\t\tstable privacy:\t%d", (flags&IFA_F_STABLE_PRIVACY));
    C_DEBUG("\t\tifa flags : \t%d", (int)flags);
}

static void 
print_ifaddr(struct ifaddrmsg * addr){
	C_DEBUG("===================== ifaddr =====================");
    C_DEBUG("\tindex : \t%d",addr->ifa_index);
    C_DEBUG("\tprefix length:\t%d", addr->ifa_prefixlen);
	//ifa_scope 

    C_DEBUG(" \n\tAttributes\n");
    int found_attr = 0;
    // int len = ((struct nlmsghdr *)addr)->nlmsg_len - NLMSG_LENGTH( sizeof( *addr ) );
    int len = IFA_PAYLOAD((struct nlmsghdr *)addr);
    for (struct rtattr * attr = IFA_RTA(addr); RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
        found_attr=1;
        switch(attr->rta_type){
            case IFA_LABEL:
                C_DEBUG("\t\tlabel:\t\t%s", (char*)RTA_DATA(attr));
                break;
            case IFA_BROADCAST:
                C_DEBUG("\t\tbroadcast : \t%d.%d.%d.%d",
                    *(int*)RTA_DATA(attr) & 0xFF,
                    (*(int*)RTA_DATA(attr) >> 8) & 0xFF,
                    (*(int*)RTA_DATA(attr) >> 16) & 0xFF,
                    (*(int*)RTA_DATA(attr) >> 24) & 0xFF);
                break;
            case IFA_LOCAL:
                C_DEBUG("\t\tlocal : \t%d.%d.%d.%d",
                    *(int*)RTA_DATA(attr) & 0xFF,
                    (*(int*)RTA_DATA(attr) >> 8) & 0xFF,
                    (*(int*)RTA_DATA(attr) >> 16) & 0xFF,
                    (*(int*)RTA_DATA(attr) >> 24) & 0xFF);
                break;
            case IFA_FLAGS:
                // C_DEBUG("\t\tifa_flags:\t%d", *(int*)RTA_DATA(attr));
				// print_ifaddr_flags(*(int*)RTA_DATA(attr));
                break;
            case IFA_CACHEINFO:
                C_DEBUG("\t\tcacheinfo : \n\t\t\tPrefered: \t%d\n\t\t\tvalid: \t\t%d\n\t\t\tcreated: \t%d\n\t\t\tupdated: \t%d", 
                    ((struct ifa_cacheinfo *)RTA_DATA(attr))->ifa_prefered, 
                    ((struct ifa_cacheinfo *)RTA_DATA(attr))->ifa_valid, 
                    ((struct ifa_cacheinfo *)RTA_DATA(attr))->cstamp, 
                    ((struct ifa_cacheinfo *)RTA_DATA(attr))->tstamp);
                break;
            case IFA_UNSPEC: //Not sure why this would come up other than reaching the end of the actual message body (Extra buffer padding)
                break;
            default:
                C_DEBUG("\t\tUnexpected Address Attribute : %d",attr->rta_type);
                break;
        }
    }

    if(!found_attr){
        C_DEBUG("\t\tNo Attribute");
    }
    print_ifaddr_flags(addr->ifa_flags);
}

static void
print_ifinfo_flags(__u8 flags){
    C_DEBUG(" \n\tif info Flags\n");
    C_DEBUG("\t\tIFF_UP:\t\t%d", flags & IFF_UP);		/* Interface is up.  */
    C_DEBUG("\t\tIFF_BROADCAST:\t\t%d", flags & IFF_BROADCAST);	/* Broadcast address valid.  */
    C_DEBUG("\t\tIFF_DEBUG:\t\t%d", flags & IFF_DEBUG);		/* Turn on debugging.  */
    C_DEBUG("\t\tIFF_LOOPBACK:\t\t%d", flags & IFF_LOOPBACK);		/* Is a loopback net.  */
    C_DEBUG("\t\tIFF_LOWER_UP:\t\t%d", flags & IFF_LOWER_UP);		/* Is a loopback net.  */
    C_DEBUG("\t\tIFF_POINTOPOINT:\t%d", flags & IFF_POINTOPOINT);	/* Interface is point-to-point link.  */
    C_DEBUG("\t\tIFF_NOTRAILERS:\t\t%d", flags & IFF_NOTRAILERS);	/* Avoid use of trailers.  */
    C_DEBUG("\t\tIFF_RUNNING:\t\t%d", flags & IFF_RUNNING);	/* Resources allocated.  */
    C_DEBUG("\t\tIFF_NOARP:\t\t%d", flags & IFF_NOARP);		/* No address resolution protocol.  */
    C_DEBUG("\t\tIFF_PROMISC:\t\t%d", flags & IFF_PROMISC);	/* Receive all packets.  */

    /* Not supported */
    C_DEBUG("\t\tIFF_ALLMULTI:\t\t%d", flags & IFF_ALLMULTI);	/* Receive all multicast packets.  */
    C_DEBUG("\t\tIFF_MASTER:\t\t%d", flags & IFF_MASTER);		/* Master of a load balancer.  */
    C_DEBUG("\t\tIFF_SLAVE:\t\t%d", flags & IFF_SLAVE);		/* Slave of a load balancer.  */
    C_DEBUG("\t\tIFF_MULTICAST:\t\t%d", flags & IFF_MULTICAST);	/* Supports multicast.  */
    C_DEBUG("\t\tIFF_PORTSEL:\t\t%d", flags & IFF_PORTSEL);	/* Can set media type.  */
    C_DEBUG("\t\tIFF_AUTOMEDIA:\t\t%d", flags & IFF_AUTOMEDIA);	/* Auto media select active.  */
    C_DEBUG("\t\tIFF_DYNAMIC:\t\t%d", flags & IFF_DYNAMIC);	/* Dialup device with changing addresses.  */
	C_DEBUG("\t\tifi flags : \t%d", (int)flags);
}

static void
print_ifinfo(struct ifinfomsg * info){
    C_DEBUG("===================== ifinfo =====================");
    C_DEBUG("\tindex : %d",info->ifi_index);
    
    C_DEBUG(" \n\tAttributes\n");
    int found_attr = 0;
    int len = IFLA_PAYLOAD((struct nlmsghdr *) info);
    for (struct rtattr * attr = IFLA_RTA(info); RTA_OK(attr, len); attr = RTA_NEXT(attr, len)){
        switch (attr->rta_type){
			case IFLA_PHYS_PORT_NAME:
				C_DEBUG("Physical port name : %s",(char*)RTA_DATA(attr));
				break;
			case IFLA_ALT_IFNAME:
				C_DEBUG("alt ifname : %s",(char*)RTA_DATA(attr));
				break;
			case IFLA_ADDRESS:
                C_DEBUG("\t\tAddress: %02x:%02x:%02x:%02x:%02x:%02x", 
    	            ((unsigned char*)RTA_DATA(attr))[0], 
                    ((unsigned char*)RTA_DATA(attr))[1], 
                    ((unsigned char*)RTA_DATA(attr))[2], 
                    ((unsigned char*)RTA_DATA(attr))[3], 
                    ((unsigned char*)RTA_DATA(attr))[4], 
                    ((unsigned char*)RTA_DATA(attr))[5]);
				break;
			case IFLA_IFNAME:
                C_DEBUG("\t\tname : %s",(char*)RTA_DATA(attr));
				break;
			case IFLA_MTU:
                C_DEBUG("\t\tMTU: %d",*(int*)RTA_DATA(attr));
				break;
            case IFLA_OPERSTATE:
                C_DEBUG("\t\tstate:\t%s\n", oper_states[*(__u8*)RTA_DATA(attr)]);
				break;
			case IFLA_PERM_ADDRESS:  //unsigned char* perm address (mac)
                C_DEBUG("\t\tPermanent: %02x:%02x:%02x:%02x:%02x:%02x", 
    	            ((unsigned char*)RTA_DATA(attr))[0], 
                    ((unsigned char*)RTA_DATA(attr))[1], 
                    ((unsigned char*)RTA_DATA(attr))[2], 
                    ((unsigned char*)RTA_DATA(attr))[3], 
                    ((unsigned char*)RTA_DATA(attr))[4], 
                    ((unsigned char*)RTA_DATA(attr))[5]);
				break;
            case IFA_LOCAL: //int : IP Address Mask
                C_DEBUG("\t\tMask: %d.%d.%d.%d",
                    *(int*)RTA_DATA(attr) & 0xFF,
                    (*(int*)RTA_DATA(attr) >> 8) & 0xFF,
                    (*(int*)RTA_DATA(attr) >> 16) & 0xFF,
                    (*(int*)RTA_DATA(attr) >> 24) & 0xFF);

                break;
            default:
                // C_TRACE("Ignored IFA Attr %d", attr->rta_type);
                break;
        }
    }
    if(!found_attr){
        C_DEBUG("\t\tNo Attribute");
    }
    
    print_ifinfo_flags(info->ifi_flags);
}
#endif

static int 
NetworkUtils__kernel_cmd(int sock, int * len, struct soap * soap, void * user_data, __u16 type, __u32 seq, struct nlreq * req, struct iovec * io, struct msghdr * msg, char * buf, NetworkUtilsQueryCallback callback){
    req->hdr.nlmsg_type = type;
    req->hdr.nlmsg_seq = seq;
    io->iov_base = req;
    io->iov_len = req->hdr.nlmsg_len;

    //send the message
    if (sendmsg(sock, msg, 0) < 0){
		C_ERROR("Failed to send msg");
        return -1;
    }

	while (1){
        memset(buf, 0, ONVIF_NET_BUFFER_SIZE);
        msg->msg_iov->iov_base = buf;
        msg->msg_iov->iov_len = ONVIF_NET_BUFFER_SIZE;

		if ((*len=recvmsg(sock, msg, 0)) < 0){
			C_ERROR("Failed to read msg");
			return -1;
		}
        //TODO page message in case the response is bigger than the buffer
		for (struct nlmsghdr * msg_ptr = (struct nlmsghdr *)buf;
				NLMSG_OK(msg_ptr, *len); msg_ptr = NLMSG_NEXT(msg_ptr, *len)){
					if(msg_ptr->nlmsg_type == NLMSG_DONE)
						return 1;
#ifdef NET_DEBUG
					struct nlmsghdr * tmp_ptr = msg_ptr;
					switch (tmp_ptr->nlmsg_type){
						case RTM_NEWADDR:
							print_ifaddr(NLMSG_DATA(tmp_ptr));
							break;
						case RTM_NEWLINK:
							print_ifinfo(NLMSG_DATA(tmp_ptr));
							break;
						default:
							C_TRACE("Ignored msg: type=%d, len=%d -- %d\n", tmp_ptr->nlmsg_type, tmp_ptr->nlmsg_len);
							break;
					}
#endif
					if(!callback(soap, user_data,msg_ptr)){
						return -1;
					}
		}
	}

    return 0;
}

int 
NetworkUtils__query(struct soap * soap, void * user_data, NetworkUtilsQueryType qtype,  NetworkUtilsQueryCallback callback){
	//build kernel netlink address
	struct sockaddr_nl kernel;
	memset(&kernel, 0, sizeof(kernel));
	kernel.nl_family = AF_NETLINK;
	kernel.nl_groups = 0;
	
	//create a Netlink socket
	int sock, len;
	if ((sock=socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0){
		C_ERROR("Failed to create kernel socket");
        return -1;
	}
    //TODO RTM_GETNEIGH for LinkLocal
	//build netlink message
	struct nlreq req;
	memset(&req, 0, sizeof(req));
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	// req.hdr.nlmsg_pid = getpid(); //Let the kernel deal with it
	// req.gen.ifi_family = AF_INET; // AF_INET6;

	struct iovec io;
	memset(&io, 0, sizeof(io));

	struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_name = &kernel;
    msg.msg_namelen = sizeof(kernel);

    char buf[ONVIF_NET_BUFFER_SIZE];
	
	if(qtype & NETUTILS_NETLINK){
		req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
		if(!NetworkUtils__kernel_cmd(sock,&len,soap, user_data,RTM_GETLINK,1,&req,&io,&msg,buf,callback))
			return -1;
	}
	if(qtype & NETUTILS_NETADDRS){
		req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
		if(!NetworkUtils__kernel_cmd(sock,&len,soap, user_data,RTM_GETADDR,2,&req,&io,&msg,buf,callback))
			return -1;
	}

    close(sock);
	return 1;
}

int
NetworkUtils__lookup_ip (const char *host, char * retval){
	struct addrinfo hints, *res, *result;
	void *ptr;
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_CANONNAME;

	if (getaddrinfo (host, NULL, &hints, &result) != 0){
		C_ERROR ("Failed to lookup address info");
		return EXIT_FAILURE;
	}

	res = result;
	while (res){
		inet_ntop (res->ai_family, res->ai_addr->sa_data, retval, 100);
		switch (res->ai_family){
			case AF_INET:
				ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
				break;
			case AF_INET6:
				ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
				break;
		}
		inet_ntop (res->ai_family, ptr, retval, 100);
		// C_FATAL ("IPv%d address: %s (%s)\n", res->ai_family == PF_INET6 ? 6 : 4, retval, res->ai_canonname);
		res = res->ai_next;
	}
  
	freeaddrinfo(result);
	return EXIT_SUCCESS;
}

int
NetworkUtils__lookup_hostname (const char *host, char * hostname){
    struct addrinfo *result, *res;
 
    /* resolve the domain name into a list of addresses */
	int error = getaddrinfo(host, NULL, NULL, &result);
    if (error != 0){
        C_ERROR("error in getaddrinfo: %s", gai_strerror(error));
        return EXIT_FAILURE;
    }   
 
    /* loop over all returned results and do inverse lookup */
    for (res = result; res != NULL; res = res->ai_next){
		error = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0);
        if (error != 0){
            C_ERROR("error in getnameinfo: %s", gai_strerror(error));
            continue;
        }
        if (*hostname != '\0'){
			//TODO Make sure its the right one
			freeaddrinfo(result);
			return EXIT_SUCCESS;
		}
    }
 
    freeaddrinfo(result);

	return EXIT_FAILURE;
}
