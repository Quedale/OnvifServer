#include "device_service.h"
#include "clogger.h"
#include "../common/service_common.h"
#include <linux/rtnetlink.h>
#include <linux/if.h>

#define ONVIF_NET_BUFFER_SIZE 8192

struct nl_req {
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
    C_DEBUG(" \n\tFlags\n");
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
	C_DEBUG("==================================");
    C_DEBUG("\tindex : \t%d",addr->ifa_index);
    C_DEBUG("\tprefix length:\t%d", addr->ifa_prefixlen);

    C_DEBUG(" \n\tAttributes\n");
    int found_attr = 0;
    // int len = ((struct nlmsghdr *)addr)->nlmsg_len - NLMSG_LENGTH( sizeof( *addr ) );
    int len = RTM_PAYLOAD((struct nlmsghdr *)addr);
    for (struct rtattr * attr = IFLA_RTA(addr); RTA_OK(attr, len); attr = RTA_NEXT(attr, len)) {
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
                C_DEBUG("\t\tifa_flags:\t%d", *(int*)RTA_DATA(attr));
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
    C_DEBUG(" \n\tFlags\n");
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
}

static void
print_ifinfo(struct ifinfomsg * info){
    C_DEBUG("===============================================");
    C_DEBUG("\tindex : %d",info->ifi_index);
    
    C_DEBUG(" \n\tAttributes\n");
    int found_attr = 0;
    int len = RTM_PAYLOAD((struct nlmsghdr *) info);
    for (struct rtattr * attr = IFLA_RTA(info); RTA_OK(attr, len); attr = RTA_NEXT(attr, len)){
        switch (attr->rta_type){
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

static void 
process_newaddr_attrs(struct soap * soap, struct _tds__GetNetworkInterfacesResponse * response, struct nlmsghdr * h){
    struct ifaddrmsg * addr = NLMSG_DATA(h);
#ifdef NET_DEBUG
    print_ifaddr(addr);
#endif
    if(addr->ifa_family != AF_INET && addr->ifa_family != AF_INET6){
        C_ERROR("Unexpected IP family :\t\t%d", addr->ifa_family);
        return;
    }

    int * ip_ptr = NULL;
    int attrlen = h->nlmsg_len - NLMSG_LENGTH( sizeof( *addr ) );
    for (struct rtattr * attr = IFLA_RTA(addr); RTA_OK(attr, attrlen); attr = RTA_NEXT(attr, attrlen)) {
        switch(attr->rta_type){
            case IFA_LOCAL:
                ip_ptr = (int*)RTA_DATA(attr);
                break;
            default:
                break;
        }
    }

    struct tt__NetworkInterface *NetworkInterface = NULL;
    for(int i=0;i<response->__sizeNetworkInterfaces;i++){
        //Retrieve int index from extra hidden token storage
        __u32 index = (__u32) *(response->NetworkInterfaces[i].token + strlen(response->NetworkInterfaces[i].token) + 1 + 1);
        if(index == addr->ifa_index){
            NetworkInterface = &response->NetworkInterfaces[i];
            break;
        }
    }

    if(!NetworkInterface) return; //Interface was ingored

    if(addr->ifa_family == AF_INET){
        if(!NetworkInterface->IPv4) NetworkInterface->IPv4 = soap_new_tt__IPv4NetworkInterface(soap, 1);
        NetworkInterface->IPv4->Enabled = xsd__boolean__true_;
        if(!NetworkInterface->IPv4->Config) NetworkInterface->IPv4->Config = soap_new_tt__IPv4Configuration(soap, 1);
        NetworkInterface->IPv4->Config->DHCP = !(addr->ifa_flags&IFA_F_PERMANENT);

        struct tt__PrefixedIPv4Address * address;
        if(!(addr->ifa_flags&IFA_F_PERMANENT)){
            if(NetworkInterface->IPv4->Config->FromDHCP) {
                C_ERROR("Multiple DHCP address found. Adding to manuals");
            } else {
                address = NetworkInterface->IPv4->Config->FromDHCP = soap_new_tt__PrefixedIPv4Address(soap, 1);
                goto skip_manual;
            }
        }

        NetworkInterface->IPv4->Config->__sizeManual++;
        if(!NetworkInterface->IPv4->Config->Manual){
            address = NetworkInterface->IPv4->Config->Manual = soap_new_tt__PrefixedIPv4Address(soap, 1);
        } else {
            soap_instance_resize(soap,
                NetworkInterface->IPv4->Config->Manual,
                NetworkInterface->IPv4->Config->__sizeManual,tt__PrefixedIPv4Address);
            address = &NetworkInterface->IPv4->Config->Manual[NetworkInterface->IPv4->Config->__sizeManual-1];
        }

skip_manual:
        address->PrefixLength = addr->ifa_prefixlen;;

        address->Address = soap_malloc(soap,16); //Max ipv4 length in worst case scenario
        char bytenumber[4];
        itoa(bytenumber, *ip_ptr & 0xFF, 10);
        strcpy(address->Address, bytenumber);
        strcat(address->Address, ".");
        itoa(bytenumber, (*ip_ptr >> 8) & 0xFF, 10);
        strcat(address->Address, bytenumber);
        strcat(address->Address, ".");
        itoa(bytenumber, (*ip_ptr >> 16) & 0xFF, 10);
        strcat(address->Address, bytenumber);
        strcat(address->Address, ".");
        itoa(bytenumber, (*ip_ptr >> 24) & 0xFF, 10);
        strcat(address->Address, bytenumber);
    } else if(addr->ifa_family == AF_INET6){
        if(!NetworkInterface->IPv6) NetworkInterface->IPv6 = soap_new_tt__IPv6NetworkInterface(soap, 1);
        NetworkInterface->IPv6->Enabled = xsd__boolean__true_;
        if(!NetworkInterface->IPv6->Config) NetworkInterface->IPv6->Config = soap_new_tt__IPv6Configuration(soap, 1);
        
        struct tt__PrefixedIPv6Address * address;
        if(!(addr->ifa_flags&IFA_F_PERMANENT)){
            NetworkInterface->IPv6->Config->__sizeFromDHCP++;
            if(NetworkInterface->IPv6->Config->FromDHCP) {
                soap_instance_resize(soap,
                    NetworkInterface->IPv6->Config->FromDHCP,
                    NetworkInterface->IPv6->Config->__sizeFromDHCP,tt__PrefixedIPv4Address);
                address = &NetworkInterface->IPv6->Config->FromDHCP[NetworkInterface->IPv6->Config->__sizeFromDHCP-1];
            } else {
                address = NetworkInterface->IPv6->Config->FromDHCP = soap_new_tt__PrefixedIPv6Address(soap, 1);
            }
        } else {
            NetworkInterface->IPv6->Config->__sizeManual++;
            if(NetworkInterface->IPv6->Config->Manual) {
                soap_instance_resize(soap,
                    NetworkInterface->IPv6->Config->Manual,
                    NetworkInterface->IPv6->Config->__sizeManual,tt__PrefixedIPv6Address);
                address = &NetworkInterface->IPv6->Config->Manual[NetworkInterface->IPv6->Config->__sizeManual-1];
            } else {
                address = NetworkInterface->IPv6->Config->Manual = soap_new_tt__PrefixedIPv6Address(soap, 1);
            }
        }
        
        if(ip_ptr){
            address->Address = soap_malloc(soap,128);
            snprintf(address->Address, 64, " %02x:%02x:%02x:%02x:%02x:%02x", 
                ip_ptr[0], ip_ptr[1], ip_ptr[2], ip_ptr[3], ip_ptr[4], ip_ptr[5]);
        }

        //TODO enum xsd__boolean *AcceptRouterAdvert;
        //TODO enum tt__IPv6DHCPConfiguration DHCP;
        //TODO struct tt__PrefixedIPv6Address *FromRA;
        //TODO Fill the rest of IPv6 config
    }
}

static void 
process_newlink_attrs(struct soap * soap, struct _tds__GetNetworkInterfacesResponse * response, struct nlmsghdr * h){
    struct ifinfomsg * addr = NLMSG_DATA(h);
#ifdef NET_DEBUG
    print_ifinfo(addr);
#endif
    if(addr->ifi_flags & IFF_LOOPBACK){
        return;
    }
    
    //Pointers to keep post loop
    unsigned char* mac_ptr = NULL;
    char* name_ptr = NULL;
    int * mtu_ptr = NULL;
    __u8 * state = NULL;

    /* loop over all attributes for the NEWLINK message */
    int len = RTM_PAYLOAD(h);
    for (struct rtattr * attr = IFLA_RTA(addr); RTA_OK(attr, len); attr = RTA_NEXT(attr, len)){
        switch (attr->rta_type){
			case IFLA_ADDRESS:
                mac_ptr = (unsigned char*)RTA_DATA(attr);
				break;
			case IFLA_IFNAME:
                name_ptr = (char*)RTA_DATA(attr);;
				break;
			case IFLA_MTU:
                mtu_ptr = (int*)RTA_DATA(attr);
				break;
            case IFLA_OPERSTATE:
                state = (__u8*)RTA_DATA(attr);
				break;
			case IFLA_PERM_ADDRESS:  //unsigned char* perm address (mac)
				break;
            case IFA_LOCAL: //int : IP Address Mask
                break;
            default:
                // C_TRACE("Ignored IFA Attr %d", attr->rta_type);
                break;
        }
    }

    if(!state || !name_ptr || !mac_ptr || !mtu_ptr){
        C_ERROR("Missing interface attrbute(s)");
        return;
    }

	response->__sizeNetworkInterfaces++;
	struct tt__NetworkInterface * NetworkInterface;
	if(!response->NetworkInterfaces){
		response->NetworkInterfaces = soap_new_tt__NetworkInterface(soap, 1);
		NetworkInterface = response->NetworkInterfaces;
	} else {
        soap_instance_resize(soap,
            response->NetworkInterfaces,
            response->__sizeNetworkInterfaces,tt__NetworkInterface);
        NetworkInterface = &response->NetworkInterfaces[response->__sizeNetworkInterfaces-1];
    }

    NetworkInterface->Info = soap_new_tt__NetworkInterfaceInfo(soap, 1);
    NetworkInterface->Info->HwAddress = soap_malloc(soap, 64);
    snprintf(NetworkInterface->Info->HwAddress, 64, " %02x:%02x:%02x:%02x:%02x:%02x", 
    	mac_ptr[0], mac_ptr[1], mac_ptr[2], mac_ptr[3], mac_ptr[4], mac_ptr[5]);

    //TODO Add index to token?
    NetworkInterface->token = soap_malloc(soap, strlen(name_ptr)+1 + sizeof(int)); //hide index at the end of the token for later mapping
    strcpy(NetworkInterface->token , name_ptr);
    *(NetworkInterface->token + strlen(name_ptr) + 1 + 1) = addr->ifi_index;

    NetworkInterface->Info->Name = soap_malloc(soap, strlen(name_ptr)+1);
    strcpy(NetworkInterface->Info->Name, name_ptr);

    NetworkInterface->Info->MTU = soap_malloc(soap, sizeof(int));
    memcpy(NetworkInterface->Info->MTU,mtu_ptr,sizeof(int));
}

static int 
submit_kernel_msg(int sock, int * len, struct soap * soap, struct _tds__GetNetworkInterfacesResponse * response, __u16 type, __u32 seq, struct nl_req * req, struct iovec * io, struct msghdr * msg, char * buf){
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
				switch (msg_ptr->nlmsg_type){
					case NLMSG_DONE:
						goto out;
					case RTM_NEWADDR:
                        process_newaddr_attrs(soap, response, msg_ptr);
                        break;
					case RTM_NEWLINK:
						process_newlink_attrs(soap, response, msg_ptr);
						break;
					default:
						C_TRACE("Ignored msg: type=%d, len=%d\n", msg_ptr->nlmsg_type, msg_ptr->nlmsg_len);
						break;
				}
		}
	}
out:
    return 0;
}




ONVIF_DEFINE_UNSECURE_METHOD(tds__GetNetworkInterfaces)
    C_DEBUG("tds__GetNetworkInterfaces");
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
	struct nl_req req;
	memset(&req, 0, sizeof(req));
	req.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	req.hdr.nlmsg_pid = getpid();
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
    submit_kernel_msg(sock,&len,soap, response,RTM_GETLINK,1,&req,&io,&msg,buf);
    submit_kernel_msg(sock,&len,soap, response,RTM_GETADDR,2,&req,&io,&msg,buf);

    close(sock);

ONVIF_METHOD_RETURNVAL(SOAP_OK)