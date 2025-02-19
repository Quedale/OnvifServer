#include "device_service.h"
#include "clogger.h"
#include "../common/service_common.h"
#include <linux/if.h> //For IFF_* flags
#include "network_utils.h"

static void 
process_newaddr_attrs(struct soap * soap, struct _tds__GetNetworkInterfacesResponse * response, struct nlmsghdr * h){
    struct ifaddrmsg * addr = NLMSG_DATA(h);
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
    if(addr->ifi_flags & IFF_LOOPBACK){
        return;
    }
    
    //Pointers to keep post loop
    unsigned char* mac_ptr = NULL;
    char* name_ptr = NULL;
    int * mtu_ptr = NULL;
    __u8 * state = NULL;

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

int 
DeviceNetworkInterfaces__ifcallback(struct soap * soap, void * user_data, struct nlmsghdr * msg){
    switch (msg->nlmsg_type){
        case RTM_NEWADDR:
            process_newaddr_attrs(soap, (struct _tds__GetNetworkInterfacesResponse *) user_data, msg);
            break;
        case RTM_NEWLINK:
            process_newlink_attrs(soap, (struct _tds__GetNetworkInterfacesResponse *) user_data, msg);
            break;
        default:
            C_TRACE("Ignored msg: type=%d, len=%d\n", msg->nlmsg_type, msg->nlmsg_len);
            break;
    }

    return 1;
}

ONVIF_DEFINE_UNSECURE_METHOD(tds__GetNetworkInterfaces)
    C_DEBUG("tds__GetNetworkInterfaces");
    NetworkUtils__query(soap,response,NETUTILS_DUMP, DeviceNetworkInterfaces__ifcallback);
ONVIF_METHOD_RETURNVAL(SOAP_OK)