#include "device_service.h"
#include "clogger.h"
#include "../common/service_common.h"
#include "../media/media_service.h"

char * generated_service_endpoint(struct soap * soap, char * path){
    int proto_len = strcspn (soap->endpoint,":");
	int port_len = sizeof(char)*(int)log10(soap->proxy_port)+1;

    char * service_endpoint = soap_malloc(soap, proto_len + 3 + strlen(soap->host) + 1 + port_len + strlen(path)+1);

    strncpy(service_endpoint,soap->endpoint,proto_len);
    service_endpoint[proto_len] = '\0';
    strcat(service_endpoint,"://");
    strcat(service_endpoint,soap->host);
    strcat(service_endpoint,":");

    int port = soap->proxy_port;
    int port_start = strlen(service_endpoint);
    int port_index = port_start;
    while (port > 0) {
        service_endpoint[port_index++] = port % 10 + '0';
        port /= 10;
    }
    service_endpoint[port_index] = '\0';

    //Reverse port order
    for (int j = port_start , k = strlen(service_endpoint) - 1; j < k; j++, k--) {
        char temp = service_endpoint[j];
        service_endpoint[j] = service_endpoint[k];
        service_endpoint[k] = temp;
    }

    strcat(service_endpoint,path);
    return service_endpoint;
}

SOAP_FMAC5 int SOAP_FMAC6 
__tds__GetServices(struct soap* soap, struct _tds__GetServices *tds__GetServices, struct _tds__GetServicesResponse *tds__GetServicesResponse){
    C_DEBUG("tds__GetServices");

    tds__GetServicesResponse->__sizeService = 2;
    tds__GetServicesResponse->Service = soap_new_tds__Service(soap, tds__GetServicesResponse->__sizeService);

    tds__GetServicesResponse->Service->__size = 1;
    tds__GetServicesResponse->Service->__any = NULL;
    tds__GetServicesResponse->Service->__anyAttribute = NULL;
    tds__GetServicesResponse->Service->Version = soap_new_tt__OnvifVersion(soap, 1);
    tds__GetServicesResponse->Service->Version->Major = ONVIF_DEVICE_SERVICE_VERSION_MAJOR;
    tds__GetServicesResponse->Service->Version->Minor = ONVIF_DEVICE_SERVICE_VERSION_MINOR;
    tds__GetServicesResponse->Service->Namespace = ONVIF_DEVICE_SERVICE_NAMESPACE;
    tds__GetServicesResponse->Service->XAddr = soap->endpoint;
    
    tds__GetServicesResponse->Service->Capabilities = soap_new__tds__Service_Capabilities(soap,1);

    struct tds__DeviceServiceCapabilities * net_cap = soap_new_tds__DeviceServiceCapabilities(soap, 1);
    net_cap->Network = soap_new_tds__NetworkCapabilities(soap, 1);
    net_cap->Network->IPFilter = xsd__boolean__false_;
    net_cap->Network->ZeroConfiguration = xsd__boolean__false_;
    net_cap->Network->IPVersion6 = xsd__boolean__false_;
    net_cap->Network->DynDNS = xsd__boolean__false_;
    net_cap->Network->Dot11Configuration = xsd__boolean__false_;
    net_cap->Network->Dot1XConfigurations = NULL;
    net_cap->Network->HostnameFromDHCP = xsd__boolean__false_;
    net_cap->Network->NTP = xsd__boolean__false_;
    net_cap->Network->DHCPv6 = xsd__boolean__false_;
    net_cap->Network->__anyAttribute = NULL;

    net_cap->Security = soap_new_tds__SecurityCapabilities(soap, 1);
    net_cap->Security->TLS1_x002e0 = xsd__boolean__false_;
    net_cap->Security->TLS1_x002e1 = xsd__boolean__false_;
    net_cap->Security->TLS1_x002e2 = xsd__boolean__false_;
    net_cap->Security->OnboardKeyGeneration = xsd__boolean__false_;
    net_cap->Security->AccessPolicyConfig = xsd__boolean__false_;
    net_cap->Security->DefaultAccessPolicy = xsd__boolean__false_;
    net_cap->Security->Dot1X = xsd__boolean__false_;
    net_cap->Security->RemoteUserHandling = xsd__boolean__false_;
    net_cap->Security->X_x002e509Token = xsd__boolean__false_;
    net_cap->Security->SAMLToken = xsd__boolean__false_;
    net_cap->Security->KerberosToken = xsd__boolean__false_;
    net_cap->Security->UsernameToken = xsd__boolean__false_;
    net_cap->Security->HttpDigest = xsd__boolean__false_;
    net_cap->Security->RELToken = xsd__boolean__false_;
    net_cap->Security->JsonWebToken = xsd__boolean__false_;
    net_cap->Security->SupportedEAPMethods = xsd__boolean__false_;
    net_cap->Security->MaxUsers = NULL;
    net_cap->Security->MaxUserNameLength = NULL;
    net_cap->Security->MaxPasswordLength = NULL;
    net_cap->Security->SecurityPolicies = NULL;
    net_cap->Security->MaxPasswordHistory = NULL;
    net_cap->Security->HashingAlgorithms = NULL;
    net_cap->Security->__anyAttribute = NULL;

    net_cap->System = soap_new_tds__SystemCapabilities(soap, 1);
    net_cap->System->DiscoveryResolve = xsd__boolean__false_;
    net_cap->System->DiscoveryBye = xsd__boolean__false_;
    net_cap->System->RemoteDiscovery = xsd__boolean__false_;
    net_cap->System->SystemBackup = xsd__boolean__false_;
    net_cap->System->SystemLogging = xsd__boolean__false_;
    net_cap->System->FirmwareUpgrade = xsd__boolean__false_;
    net_cap->System->HttpFirmwareUpgrade = xsd__boolean__false_;
    net_cap->System->HttpSystemBackup = xsd__boolean__false_;
    net_cap->System->HttpSystemLogging = xsd__boolean__false_;
    net_cap->System->HttpSupportInformation = xsd__boolean__false_;
    net_cap->System->StorageConfiguration = xsd__boolean__false_;
    net_cap->System->MaxStorageConfigurations = NULL;
    net_cap->System->GeoLocationEntries = NULL;
    net_cap->System->AutoGeo = NULL;
    net_cap->System->StorageTypesSupported = NULL;
    net_cap->System->DiscoveryNotSupported = xsd__boolean__false_;
    net_cap->System->NetworkConfigNotSupported = xsd__boolean__false_;
    net_cap->System->UserConfigNotSupported = xsd__boolean__false_;
    net_cap->System->Addons = NULL;
    net_cap->System->__anyAttribute = NULL;

    net_cap->Misc = soap_new_tds__MiscCapabilities(soap, 1);
    net_cap->Misc->AuxiliaryCommands = "";
    net_cap->Misc->__anyAttribute = NULL;

    char *ret = NULL;
    ServiceCommon__serialize_data(soap, soap_write_tds__DeviceServiceCapabilities,ret, net_cap);

    tds__GetServicesResponse->Service->Capabilities->__any = ret;

    //Media Service Capabilities
    struct tds__Service * service = &tds__GetServicesResponse->Service[1];
    service->__any = NULL;
    service->__anyAttribute = NULL;
    service->Version = soap_new_tt__OnvifVersion(soap, 1);
    service->Version->Major = ONVIF_MEDIA_SERVICE_VERSION_MAJOR;
    service->Version->Minor = ONVIF_MEDIA_SERVICE_VERSION_MINOR;
    service->Namespace = ONVIF_MEDIA_SERVICE_NAMESPACE;
    service->XAddr = generated_service_endpoint(soap,ONVIF_MEDIA_SERVICE_PATH);
    service->Capabilities = soap_new__tds__Service_Capabilities(soap,1);
    ServiceCommon__serialize_data(soap, soap_write_trt__Capabilities,ret, OnvifMediaService__createCapabilities(soap));
    service->Capabilities->__any = ret;

    return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 
__tds__GetSystemDateAndTime(struct soap* soap, struct _tds__GetSystemDateAndTime *tds__GetSystemDateAndTime, struct _tds__GetSystemDateAndTimeResponse *tds__GetSystemDateAndTimeResponse){
    C_DEBUG("tds__GetSystemDateAndTime");

    tds__GetSystemDateAndTimeResponse->SystemDateAndTime = soap_new_tt__SystemDateTime(soap, 1);
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->DateTimeType = tt__SetDateTimeType__NTP;
    
    time_t utc_now = time( NULL );
    struct tm buf;
    struct tm* utc_tm = gmtime_r(&utc_now, &buf); 

    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime = soap_new_tt__DateTime(soap, 1);
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time = soap_new_tt__Time(soap, 1);
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Hour = utc_tm->tm_hour;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Minute = utc_tm->tm_min;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Time->Second = utc_tm->tm_sec;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date = soap_new_tt__Date(soap, 1);
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Day = utc_tm->tm_mday;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Month = utc_tm->tm_mon +1;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->UTCDateTime->Date->Year = utc_tm->tm_year + 1900;

    struct tm* local_tm = localtime_r(&utc_now, &buf);
    if(local_tm->tm_isdst){
        tds__GetSystemDateAndTimeResponse->SystemDateAndTime->DaylightSavings = xsd__boolean__true_;
    } else {
        tds__GetSystemDateAndTimeResponse->SystemDateAndTime->DaylightSavings = xsd__boolean__false_;
    }

    // /etc/timezone - Timezone string
    // /etc/localtime - current tz binary - change this file to change timezone
    // /usr/share/zoneinfo/... all timezones
    // /usr/share/zoneinfo/tzdata.zi - database
    // RFC https://datatracker.ietf.org/doc/html/rfc8536
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone = soap_new_tt__TimeZone(soap, 1);
    # ifdef	__USE_MISC
        tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone->TZ = (char*) local_tm->tm_zone;
    # else
        tds__GetSystemDateAndTimeResponse->SystemDateAndTime->TimeZone->TZ = (char*) local_tm->__tm_zone;
    # endif
    
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime = soap_new_tt__DateTime(soap, 1);
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time = soap_new_tt__Time(soap, 1);
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time->Hour = local_tm->tm_hour;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time->Minute = local_tm->tm_min;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Time->Second = local_tm->tm_sec;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date = soap_new_tt__Date(soap, 1);
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date->Day = local_tm->tm_mday;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date->Month = local_tm->tm_mon+1;
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->LocalDateTime->Date->Year = local_tm->tm_year + 1900;

    //No use
    tds__GetSystemDateAndTimeResponse->SystemDateAndTime->Extension = NULL;
    return SOAP_OK;
}

int  
OnvifDeviceService__serve(struct soap *soap){
	(void)soap_peek_element(soap);
	if (!soap_match_tag(soap, soap->tag, "SOAP-ENV:Fault"))
		return soap_serve_SOAP_ENV__Fault(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetServices"))
		return soap_serve___tds__GetServices(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetServiceCapabilities"))
		return soap_serve___tds__GetServiceCapabilities(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetDeviceInformation"))
		return soap_serve___tds__GetDeviceInformation(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetSystemDateAndTime"))
		return soap_serve___tds__SetSystemDateAndTime(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetSystemDateAndTime"))
		return soap_serve___tds__GetSystemDateAndTime(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetSystemFactoryDefault"))
		return soap_serve___tds__SetSystemFactoryDefault(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:UpgradeSystemFirmware"))
		return soap_serve___tds__UpgradeSystemFirmware(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SystemReboot"))
		return soap_serve___tds__SystemReboot(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:RestoreSystem"))
		return soap_serve___tds__RestoreSystem(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetSystemBackup"))
		return soap_serve___tds__GetSystemBackup(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetSystemLog"))
		return soap_serve___tds__GetSystemLog(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetSystemSupportInformation"))
		return soap_serve___tds__GetSystemSupportInformation(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetScopes"))
		return soap_serve___tds__GetScopes(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetScopes"))
		return soap_serve___tds__SetScopes(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:AddScopes"))
		return soap_serve___tds__AddScopes(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:RemoveScopes"))
		return soap_serve___tds__RemoveScopes(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetDiscoveryMode"))
		return soap_serve___tds__GetDiscoveryMode(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetDiscoveryMode"))
		return soap_serve___tds__SetDiscoveryMode(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetRemoteDiscoveryMode"))
		return soap_serve___tds__GetRemoteDiscoveryMode(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetRemoteDiscoveryMode"))
		return soap_serve___tds__SetRemoteDiscoveryMode(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetDPAddresses"))
		return soap_serve___tds__GetDPAddresses(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetEndpointReference"))
		return soap_serve___tds__GetEndpointReference(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetRemoteUser"))
		return soap_serve___tds__GetRemoteUser(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetRemoteUser"))
		return soap_serve___tds__SetRemoteUser(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetUsers"))
		return soap_serve___tds__GetUsers(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:CreateUsers"))
		return soap_serve___tds__CreateUsers(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:DeleteUsers"))
		return soap_serve___tds__DeleteUsers(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetUser"))
		return soap_serve___tds__SetUser(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetWsdlUrl"))
		return soap_serve___tds__GetWsdlUrl(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetPasswordComplexityOptions"))
		return soap_serve___tds__GetPasswordComplexityOptions(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetPasswordComplexityConfiguration"))
		return soap_serve___tds__GetPasswordComplexityConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetPasswordComplexityConfiguration"))
		return soap_serve___tds__SetPasswordComplexityConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetPasswordHistoryConfiguration"))
		return soap_serve___tds__GetPasswordHistoryConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetPasswordHistoryConfiguration"))
		return soap_serve___tds__SetPasswordHistoryConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetAuthFailureWarningOptions"))
		return soap_serve___tds__GetAuthFailureWarningOptions(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetAuthFailureWarningConfiguration"))
		return soap_serve___tds__GetAuthFailureWarningConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetAuthFailureWarningConfiguration"))
		return soap_serve___tds__SetAuthFailureWarningConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetCapabilities"))
		return soap_serve___tds__GetCapabilities(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetDPAddresses"))
		return soap_serve___tds__SetDPAddresses(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetHostname"))
		return soap_serve___tds__GetHostname(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetHostname"))
		return soap_serve___tds__SetHostname(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetHostnameFromDHCP"))
		return soap_serve___tds__SetHostnameFromDHCP(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetDNS"))
		return soap_serve___tds__GetDNS(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetDNS"))
		return soap_serve___tds__SetDNS(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetNTP"))
		return soap_serve___tds__GetNTP(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetNTP"))
		return soap_serve___tds__SetNTP(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetDynamicDNS"))
		return soap_serve___tds__GetDynamicDNS(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetDynamicDNS"))
		return soap_serve___tds__SetDynamicDNS(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetNetworkInterfaces"))
		return soap_serve___tds__GetNetworkInterfaces(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetNetworkInterfaces"))
		return soap_serve___tds__SetNetworkInterfaces(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetNetworkProtocols"))
		return soap_serve___tds__GetNetworkProtocols(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetNetworkProtocols"))
		return soap_serve___tds__SetNetworkProtocols(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetNetworkDefaultGateway"))
		return soap_serve___tds__GetNetworkDefaultGateway(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetNetworkDefaultGateway"))
		return soap_serve___tds__SetNetworkDefaultGateway(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetZeroConfiguration"))
		return soap_serve___tds__GetZeroConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetZeroConfiguration"))
		return soap_serve___tds__SetZeroConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetIPAddressFilter"))
		return soap_serve___tds__GetIPAddressFilter(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetIPAddressFilter"))
		return soap_serve___tds__SetIPAddressFilter(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:AddIPAddressFilter"))
		return soap_serve___tds__AddIPAddressFilter(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:RemoveIPAddressFilter"))
		return soap_serve___tds__RemoveIPAddressFilter(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetAccessPolicy"))
		return soap_serve___tds__GetAccessPolicy(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetAccessPolicy"))
		return soap_serve___tds__SetAccessPolicy(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:CreateCertificate"))
		return soap_serve___tds__CreateCertificate(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetCertificates"))
		return soap_serve___tds__GetCertificates(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetCertificatesStatus"))
		return soap_serve___tds__GetCertificatesStatus(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetCertificatesStatus"))
		return soap_serve___tds__SetCertificatesStatus(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:DeleteCertificates"))
		return soap_serve___tds__DeleteCertificates(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetPkcs10Request"))
		return soap_serve___tds__GetPkcs10Request(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:LoadCertificates"))
		return soap_serve___tds__LoadCertificates(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetClientCertificateMode"))
		return soap_serve___tds__GetClientCertificateMode(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetClientCertificateMode"))
		return soap_serve___tds__SetClientCertificateMode(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetRelayOutputs"))
		return soap_serve___tds__GetRelayOutputs(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetRelayOutputSettings"))
		return soap_serve___tds__SetRelayOutputSettings(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetRelayOutputState"))
		return soap_serve___tds__SetRelayOutputState(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SendAuxiliaryCommand"))
		return soap_serve___tds__SendAuxiliaryCommand(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetCACertificates"))
		return soap_serve___tds__GetCACertificates(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:LoadCertificateWithPrivateKey"))
		return soap_serve___tds__LoadCertificateWithPrivateKey(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetCertificateInformation"))
		return soap_serve___tds__GetCertificateInformation(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:LoadCACertificates"))
		return soap_serve___tds__LoadCACertificates(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:CreateDot1XConfiguration"))
		return soap_serve___tds__CreateDot1XConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetDot1XConfiguration"))
		return soap_serve___tds__SetDot1XConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetDot1XConfiguration"))
		return soap_serve___tds__GetDot1XConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetDot1XConfigurations"))
		return soap_serve___tds__GetDot1XConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:DeleteDot1XConfiguration"))
		return soap_serve___tds__DeleteDot1XConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetDot11Capabilities"))
		return soap_serve___tds__GetDot11Capabilities(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetDot11Status"))
		return soap_serve___tds__GetDot11Status(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:ScanAvailableDot11Networks"))
		return soap_serve___tds__ScanAvailableDot11Networks(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetSystemUris"))
		return soap_serve___tds__GetSystemUris(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:StartFirmwareUpgrade"))
		return soap_serve___tds__StartFirmwareUpgrade(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:StartSystemRestore"))
		return soap_serve___tds__StartSystemRestore(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetStorageConfigurations"))
		return soap_serve___tds__GetStorageConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:CreateStorageConfiguration"))
		return soap_serve___tds__CreateStorageConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetStorageConfiguration"))
		return soap_serve___tds__GetStorageConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetStorageConfiguration"))
		return soap_serve___tds__SetStorageConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:DeleteStorageConfiguration"))
		return soap_serve___tds__DeleteStorageConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:GetGeoLocation"))
		return soap_serve___tds__GetGeoLocation(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetGeoLocation"))
		return soap_serve___tds__SetGeoLocation(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:DeleteGeoLocation"))
		return soap_serve___tds__DeleteGeoLocation(soap);
	if (!soap_match_tag(soap, soap->tag, "tds:SetHashingAlgorithm"))
		return soap_serve___tds__SetHashingAlgorithm(soap);
	return soap->error = SOAP_NO_METHOD;
}

ONVIF_DEFINE_NO_METHOD(tds__GetServiceCapabilities)
ONVIF_DEFINE_NO_METHOD(tds__GetDeviceInformation)
ONVIF_DEFINE_NO_METHOD(tds__SetSystemDateAndTime)
ONVIF_DEFINE_NO_METHOD(tds__SetSystemFactoryDefault)
ONVIF_DEFINE_NO_METHOD(tds__UpgradeSystemFirmware)
ONVIF_DEFINE_NO_METHOD(tds__SystemReboot)
ONVIF_DEFINE_NO_METHOD(tds__RestoreSystem)
ONVIF_DEFINE_NO_METHOD(tds__GetSystemBackup)
ONVIF_DEFINE_NO_METHOD(tds__GetSystemLog)
ONVIF_DEFINE_NO_METHOD(tds__GetSystemSupportInformation)
ONVIF_DEFINE_NO_METHOD(tds__GetScopes)
ONVIF_DEFINE_NO_METHOD(tds__SetScopes)
ONVIF_DEFINE_NO_METHOD(tds__AddScopes)
ONVIF_DEFINE_NO_METHOD(tds__RemoveScopes)
ONVIF_DEFINE_NO_METHOD(tds__GetDiscoveryMode)
ONVIF_DEFINE_NO_METHOD(tds__SetDiscoveryMode)
ONVIF_DEFINE_NO_METHOD(tds__GetRemoteDiscoveryMode)
ONVIF_DEFINE_NO_METHOD(tds__SetRemoteDiscoveryMode)
ONVIF_DEFINE_NO_METHOD(tds__GetDPAddresses)
ONVIF_DEFINE_NO_METHOD(tds__GetEndpointReference)
ONVIF_DEFINE_NO_METHOD(tds__GetRemoteUser)
ONVIF_DEFINE_NO_METHOD(tds__SetRemoteUser)
ONVIF_DEFINE_NO_METHOD(tds__GetUsers)
ONVIF_DEFINE_NO_METHOD(tds__CreateUsers)
ONVIF_DEFINE_NO_METHOD(tds__DeleteUsers)
ONVIF_DEFINE_NO_METHOD(tds__SetUser)
ONVIF_DEFINE_NO_METHOD(tds__GetWsdlUrl)
ONVIF_DEFINE_NO_METHOD(tds__GetPasswordComplexityOptions)
ONVIF_DEFINE_NO_METHOD(tds__GetPasswordComplexityConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__SetPasswordComplexityConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__GetPasswordHistoryConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__SetPasswordHistoryConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__GetAuthFailureWarningOptions)
ONVIF_DEFINE_NO_METHOD(tds__GetAuthFailureWarningConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__SetAuthFailureWarningConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__GetCapabilities)
ONVIF_DEFINE_NO_METHOD(tds__SetDPAddresses)
ONVIF_DEFINE_NO_METHOD(tds__GetHostname)
ONVIF_DEFINE_NO_METHOD(tds__SetHostname)
ONVIF_DEFINE_NO_METHOD(tds__SetHostnameFromDHCP)
ONVIF_DEFINE_NO_METHOD(tds__GetDNS)
ONVIF_DEFINE_NO_METHOD(tds__SetDNS)
ONVIF_DEFINE_NO_METHOD(tds__GetNTP)
ONVIF_DEFINE_NO_METHOD(tds__SetNTP)
ONVIF_DEFINE_NO_METHOD(tds__GetDynamicDNS)
ONVIF_DEFINE_NO_METHOD(tds__SetDynamicDNS)
ONVIF_DEFINE_NO_METHOD(tds__GetNetworkInterfaces)
ONVIF_DEFINE_NO_METHOD(tds__SetNetworkInterfaces)
ONVIF_DEFINE_NO_METHOD(tds__GetNetworkProtocols)
ONVIF_DEFINE_NO_METHOD(tds__SetNetworkProtocols)
ONVIF_DEFINE_NO_METHOD(tds__GetNetworkDefaultGateway)
ONVIF_DEFINE_NO_METHOD(tds__SetNetworkDefaultGateway)
ONVIF_DEFINE_NO_METHOD(tds__GetZeroConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__SetZeroConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__GetIPAddressFilter)
ONVIF_DEFINE_NO_METHOD(tds__SetIPAddressFilter)
ONVIF_DEFINE_NO_METHOD(tds__AddIPAddressFilter)
ONVIF_DEFINE_NO_METHOD(tds__RemoveIPAddressFilter)
ONVIF_DEFINE_NO_METHOD(tds__GetAccessPolicy)
ONVIF_DEFINE_NO_METHOD(tds__SetAccessPolicy)
ONVIF_DEFINE_NO_METHOD(tds__CreateCertificate)
ONVIF_DEFINE_NO_METHOD(tds__GetCertificates)
ONVIF_DEFINE_NO_METHOD(tds__GetCertificatesStatus)
ONVIF_DEFINE_NO_METHOD(tds__SetCertificatesStatus)
ONVIF_DEFINE_NO_METHOD(tds__DeleteCertificates)
ONVIF_DEFINE_NO_METHOD(tds__GetPkcs10Request)
ONVIF_DEFINE_NO_METHOD(tds__LoadCertificates)
ONVIF_DEFINE_NO_METHOD(tds__GetClientCertificateMode)
ONVIF_DEFINE_NO_METHOD(tds__SetClientCertificateMode)
ONVIF_DEFINE_NO_METHOD(tds__GetRelayOutputs)
ONVIF_DEFINE_NO_METHOD(tds__SetRelayOutputSettings)
ONVIF_DEFINE_NO_METHOD(tds__SetRelayOutputState)
ONVIF_DEFINE_NO_METHOD(tds__SendAuxiliaryCommand)
ONVIF_DEFINE_NO_METHOD(tds__GetCACertificates)
ONVIF_DEFINE_NO_METHOD(tds__LoadCertificateWithPrivateKey)
ONVIF_DEFINE_NO_METHOD(tds__GetCertificateInformation)
ONVIF_DEFINE_NO_METHOD(tds__LoadCACertificates)
ONVIF_DEFINE_NO_METHOD(tds__CreateDot1XConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__SetDot1XConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__GetDot1XConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__GetDot1XConfigurations)
ONVIF_DEFINE_NO_METHOD(tds__DeleteDot1XConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__GetDot11Capabilities)
ONVIF_DEFINE_NO_METHOD(tds__GetDot11Status)
ONVIF_DEFINE_NO_METHOD(tds__ScanAvailableDot11Networks)
ONVIF_DEFINE_NO_METHOD(tds__GetSystemUris)
ONVIF_DEFINE_NO_METHOD(tds__StartFirmwareUpgrade)
ONVIF_DEFINE_NO_METHOD(tds__StartSystemRestore)
ONVIF_DEFINE_NO_METHOD(tds__GetStorageConfigurations)
ONVIF_DEFINE_NO_METHOD(tds__CreateStorageConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__GetStorageConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__SetStorageConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__DeleteStorageConfiguration)
ONVIF_DEFINE_NO_METHOD(tds__GetGeoLocation)
ONVIF_DEFINE_NO_METHOD(tds__SetGeoLocation)
ONVIF_DEFINE_NO_METHOD(tds__DeleteGeoLocation)
ONVIF_DEFINE_NO_METHOD(tds__SetHashingAlgorithm)