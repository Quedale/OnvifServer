#include "clogger.h"
#include "../common/service_common.h"
#include "wsseapi.h"

struct trt__Capabilities * 
OnvifMediaService__createCapabilities(struct soap * soap){
    struct trt__Capabilities * media_cap = soap_new_trt__Capabilities(soap, 1);
	//TODO Fill service capabilities
    return media_cap;
}

ONVIF_DEFINE_SECURE_METHOD(trt__GetProfiles)
    response->__sizeProfiles = 1;
    response->Profiles = soap_new_tt__Profile(soap, 1);
    response->Profiles->Name = "Default";
    response->Profiles->token = "default";
    response->Profiles->fixed = xsd__boolean__false_;
    response->Profiles->VideoSourceConfiguration = NULL;
    response->Profiles->AudioSourceConfiguration = NULL;
    response->Profiles->VideoEncoderConfiguration = NULL;
    response->Profiles->AudioEncoderConfiguration = NULL;
    response->Profiles->VideoAnalyticsConfiguration = NULL;
    response->Profiles->PTZConfiguration = NULL;
    response->Profiles->MetadataConfiguration = NULL;
    response->Profiles->Extension = NULL;
    response->Profiles->__anyAttribute = NULL;
ONVIF_METHOD_RETURNVAL(SOAP_OK)

ONVIF_DEFINE_SECURE_METHOD(trt__GetStreamUri)
	//Retrieve IP used by the socket. The client should be able to connect using it
	char server_ip[16];
	struct sockaddr_in my_addr;
	memset (&my_addr, 0, sizeof (my_addr));
	socklen_t len = sizeof(my_addr);
	if(getsockname(soap->socket,(struct sockaddr *) &my_addr, &len) || !inet_ntop(AF_INET, &my_addr.sin_addr, server_ip, sizeof(server_ip))){
		return SOAP_SVR_FAULT;
	} //TODO Get domain from TLS handshake so that the rtsps stream can be properly initiated

	OnvifUserConfig * data = (OnvifUserConfig *) soap->user;
    char bind_port[5];
    itoa(bind_port, data->rtsp_port, 10);

    response->MediaUri = soap_new_tt__MediaUri(soap, 1);
	response->MediaUri->Uri = soap_malloc(soap,sizeof(char) * (strlen("rtsp://") + strlen(server_ip) + 1 + strlen(bind_port) + strlen("/h264")));
	strcpy(response->MediaUri->Uri,"rtsp://");
	strcat(response->MediaUri->Uri,server_ip);
	strcat(response->MediaUri->Uri,":");
	strcat(response->MediaUri->Uri,bind_port);
	strcat(response->MediaUri->Uri,"/h264");

    response->MediaUri->InvalidAfterConnect = xsd__boolean__false_;
    response->MediaUri->InvalidAfterReboot = xsd__boolean__false_;
    response->MediaUri->Timeout = "PT0S";
    response->MediaUri->__size = 0;
    response->MediaUri->__any = NULL;
    response->MediaUri->__anyAttribute = NULL;
ONVIF_METHOD_RETURNVAL(SOAP_OK)

int  
OnvifMediaService__serve(struct soap *soap){
	(void)soap_peek_element(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetServiceCapabilities"))
		return soap_serve___trt__GetServiceCapabilities(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetVideoSources"))
		return soap_serve___trt__GetVideoSources(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioSources"))
		return soap_serve___trt__GetAudioSources(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioOutputs"))
		return soap_serve___trt__GetAudioOutputs(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:CreateProfile"))
		return soap_serve___trt__CreateProfile(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetProfile"))
		return soap_serve___trt__GetProfile(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetProfiles"))
		return soap_serve___trt__GetProfiles(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:AddVideoEncoderConfiguration"))
		return soap_serve___trt__AddVideoEncoderConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:AddVideoSourceConfiguration"))
		return soap_serve___trt__AddVideoSourceConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:AddAudioEncoderConfiguration"))
		return soap_serve___trt__AddAudioEncoderConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:AddAudioSourceConfiguration"))
		return soap_serve___trt__AddAudioSourceConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:AddPTZConfiguration"))
		return soap_serve___trt__AddPTZConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:AddVideoAnalyticsConfiguration"))
		return soap_serve___trt__AddVideoAnalyticsConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:AddMetadataConfiguration"))
		return soap_serve___trt__AddMetadataConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:AddAudioOutputConfiguration"))
		return soap_serve___trt__AddAudioOutputConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:AddAudioDecoderConfiguration"))
		return soap_serve___trt__AddAudioDecoderConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:RemoveVideoEncoderConfiguration"))
		return soap_serve___trt__RemoveVideoEncoderConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:RemoveVideoSourceConfiguration"))
		return soap_serve___trt__RemoveVideoSourceConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:RemoveAudioEncoderConfiguration"))
		return soap_serve___trt__RemoveAudioEncoderConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:RemoveAudioSourceConfiguration"))
		return soap_serve___trt__RemoveAudioSourceConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:RemovePTZConfiguration"))
		return soap_serve___trt__RemovePTZConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:RemoveVideoAnalyticsConfiguration"))
		return soap_serve___trt__RemoveVideoAnalyticsConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:RemoveMetadataConfiguration"))
		return soap_serve___trt__RemoveMetadataConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:RemoveAudioOutputConfiguration"))
		return soap_serve___trt__RemoveAudioOutputConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:RemoveAudioDecoderConfiguration"))
		return soap_serve___trt__RemoveAudioDecoderConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:DeleteProfile"))
		return soap_serve___trt__DeleteProfile(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetVideoSourceConfigurations"))
		return soap_serve___trt__GetVideoSourceConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetVideoEncoderConfigurations"))
		return soap_serve___trt__GetVideoEncoderConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioSourceConfigurations"))
		return soap_serve___trt__GetAudioSourceConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioEncoderConfigurations"))
		return soap_serve___trt__GetAudioEncoderConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetVideoAnalyticsConfigurations"))
		return soap_serve___trt__GetVideoAnalyticsConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetMetadataConfigurations"))
		return soap_serve___trt__GetMetadataConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioOutputConfigurations"))
		return soap_serve___trt__GetAudioOutputConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioDecoderConfigurations"))
		return soap_serve___trt__GetAudioDecoderConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetVideoSourceConfiguration"))
		return soap_serve___trt__GetVideoSourceConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetVideoEncoderConfiguration"))
		return soap_serve___trt__GetVideoEncoderConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioSourceConfiguration"))
		return soap_serve___trt__GetAudioSourceConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioEncoderConfiguration"))
		return soap_serve___trt__GetAudioEncoderConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetVideoAnalyticsConfiguration"))
		return soap_serve___trt__GetVideoAnalyticsConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetMetadataConfiguration"))
		return soap_serve___trt__GetMetadataConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioOutputConfiguration"))
		return soap_serve___trt__GetAudioOutputConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioDecoderConfiguration"))
		return soap_serve___trt__GetAudioDecoderConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetCompatibleVideoEncoderConfigurations"))
		return soap_serve___trt__GetCompatibleVideoEncoderConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetCompatibleVideoSourceConfigurations"))
		return soap_serve___trt__GetCompatibleVideoSourceConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetCompatibleAudioEncoderConfigurations"))
		return soap_serve___trt__GetCompatibleAudioEncoderConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetCompatibleAudioSourceConfigurations"))
		return soap_serve___trt__GetCompatibleAudioSourceConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetCompatibleVideoAnalyticsConfigurations"))
		return soap_serve___trt__GetCompatibleVideoAnalyticsConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetCompatibleMetadataConfigurations"))
		return soap_serve___trt__GetCompatibleMetadataConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetCompatibleAudioOutputConfigurations"))
		return soap_serve___trt__GetCompatibleAudioOutputConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetCompatibleAudioDecoderConfigurations"))
		return soap_serve___trt__GetCompatibleAudioDecoderConfigurations(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:SetVideoSourceConfiguration"))
		return soap_serve___trt__SetVideoSourceConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:SetVideoEncoderConfiguration"))
		return soap_serve___trt__SetVideoEncoderConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:SetAudioSourceConfiguration"))
		return soap_serve___trt__SetAudioSourceConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:SetAudioEncoderConfiguration"))
		return soap_serve___trt__SetAudioEncoderConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:SetVideoAnalyticsConfiguration"))
		return soap_serve___trt__SetVideoAnalyticsConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:SetMetadataConfiguration"))
		return soap_serve___trt__SetMetadataConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:SetAudioOutputConfiguration"))
		return soap_serve___trt__SetAudioOutputConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:SetAudioDecoderConfiguration"))
		return soap_serve___trt__SetAudioDecoderConfiguration(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetVideoSourceConfigurationOptions"))
		return soap_serve___trt__GetVideoSourceConfigurationOptions(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetVideoEncoderConfigurationOptions"))
		return soap_serve___trt__GetVideoEncoderConfigurationOptions(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioSourceConfigurationOptions"))
		return soap_serve___trt__GetAudioSourceConfigurationOptions(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioEncoderConfigurationOptions"))
		return soap_serve___trt__GetAudioEncoderConfigurationOptions(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetMetadataConfigurationOptions"))
		return soap_serve___trt__GetMetadataConfigurationOptions(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioOutputConfigurationOptions"))
		return soap_serve___trt__GetAudioOutputConfigurationOptions(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetAudioDecoderConfigurationOptions"))
		return soap_serve___trt__GetAudioDecoderConfigurationOptions(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetGuaranteedNumberOfVideoEncoderInstances"))
		return soap_serve___trt__GetGuaranteedNumberOfVideoEncoderInstances(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetStreamUri"))
		return soap_serve___trt__GetStreamUri(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:StartMulticastStreaming"))
		return soap_serve___trt__StartMulticastStreaming(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:StopMulticastStreaming"))
		return soap_serve___trt__StopMulticastStreaming(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:SetSynchronizationPoint"))
		return soap_serve___trt__SetSynchronizationPoint(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetSnapshotUri"))
		return soap_serve___trt__GetSnapshotUri(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetVideoSourceModes"))
		return soap_serve___trt__GetVideoSourceModes(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:SetVideoSourceMode"))
		return soap_serve___trt__SetVideoSourceMode(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetOSDs"))
		return soap_serve___trt__GetOSDs(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetOSD"))
		return soap_serve___trt__GetOSD(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:GetOSDOptions"))
		return soap_serve___trt__GetOSDOptions(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:SetOSD"))
		return soap_serve___trt__SetOSD(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:CreateOSD"))
		return soap_serve___trt__CreateOSD(soap);
	if (!soap_match_tag(soap, soap->tag, "trt:DeleteOSD"))
		return soap_serve___trt__DeleteOSD(soap);
	return soap->error = SOAP_NO_METHOD;
}

ONVIF_DEFINE_NO_METHOD(trt__GetServiceCapabilities)
ONVIF_DEFINE_NO_METHOD(trt__GetVideoSources)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioSources)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioOutputs)
ONVIF_DEFINE_NO_METHOD(trt__CreateProfile)
ONVIF_DEFINE_NO_METHOD(trt__GetProfile)
ONVIF_DEFINE_NO_METHOD(trt__AddVideoEncoderConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__AddVideoSourceConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__AddAudioEncoderConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__AddAudioSourceConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__AddPTZConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__AddVideoAnalyticsConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__AddMetadataConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__AddAudioOutputConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__AddAudioDecoderConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__RemoveVideoEncoderConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__RemoveVideoSourceConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__RemoveAudioEncoderConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__RemoveAudioSourceConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__RemovePTZConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__RemoveVideoAnalyticsConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__RemoveMetadataConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__RemoveAudioOutputConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__RemoveAudioDecoderConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__DeleteProfile)
ONVIF_DEFINE_NO_METHOD(trt__GetVideoSourceConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetVideoEncoderConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioSourceConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioEncoderConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetVideoAnalyticsConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetMetadataConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioOutputConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioDecoderConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetVideoSourceConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__GetVideoEncoderConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioSourceConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioEncoderConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__GetVideoAnalyticsConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__GetMetadataConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioOutputConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioDecoderConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__GetCompatibleVideoEncoderConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetCompatibleVideoSourceConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetCompatibleAudioEncoderConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetCompatibleAudioSourceConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetCompatibleVideoAnalyticsConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetCompatibleMetadataConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetCompatibleAudioOutputConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__GetCompatibleAudioDecoderConfigurations)
ONVIF_DEFINE_NO_METHOD(trt__SetVideoSourceConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__SetVideoEncoderConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__SetAudioSourceConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__SetAudioEncoderConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__SetVideoAnalyticsConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__SetMetadataConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__SetAudioOutputConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__SetAudioDecoderConfiguration)
ONVIF_DEFINE_NO_METHOD(trt__GetVideoSourceConfigurationOptions)
ONVIF_DEFINE_NO_METHOD(trt__GetVideoEncoderConfigurationOptions)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioSourceConfigurationOptions)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioEncoderConfigurationOptions)
ONVIF_DEFINE_NO_METHOD(trt__GetMetadataConfigurationOptions)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioOutputConfigurationOptions)
ONVIF_DEFINE_NO_METHOD(trt__GetAudioDecoderConfigurationOptions)
ONVIF_DEFINE_NO_METHOD(trt__GetGuaranteedNumberOfVideoEncoderInstances)
ONVIF_DEFINE_NO_METHOD(trt__StartMulticastStreaming)
ONVIF_DEFINE_NO_METHOD(trt__StopMulticastStreaming)
ONVIF_DEFINE_NO_METHOD(trt__SetSynchronizationPoint)
ONVIF_DEFINE_NO_METHOD(trt__GetSnapshotUri)
ONVIF_DEFINE_NO_METHOD(trt__GetVideoSourceModes)
ONVIF_DEFINE_NO_METHOD(trt__SetVideoSourceMode)
ONVIF_DEFINE_NO_METHOD(trt__GetOSDs)
ONVIF_DEFINE_NO_METHOD(trt__GetOSD)
ONVIF_DEFINE_NO_METHOD(trt__GetOSDOptions)
ONVIF_DEFINE_NO_METHOD(trt__SetOSD)
ONVIF_DEFINE_NO_METHOD(trt__CreateOSD)
ONVIF_DEFINE_NO_METHOD(trt__DeleteOSD)