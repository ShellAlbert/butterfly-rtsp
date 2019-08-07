/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** ServerMediaSubsession.cpp
** 
** -------------------------------------------------------------------------*/

#include <sstream>
#include <linux/videodev2.h>

// project
#include "ServerMediaSubsession.h"
#include "MJPEGVideoSource.h"
#include "DeviceSource.h"

// ---------------------------------
//   BaseServerMediaSubsession
// ---------------------------------
FramedSource* BaseServerMediaSubsession::createSource(UsageEnvironment& env, FramedSource* videoES, const std::string& format)
{
	FramedSource* source = NULL;
	if (format == "video/MP2T")
	{
		source = MPEG2TransportStreamFramer::createNew(env, videoES); 
	}
	else if (format == "video/H264")
	{
		source = H264VideoStreamDiscreteFramer::createNew(env, videoES);
	}
#if LIVEMEDIA_LIBRARY_VERSION_INT > 1414454400
	else if (format == "video/H265")
	{
		source = H265VideoStreamDiscreteFramer::createNew(env, videoES);
	}
#endif
	else if (format == "video/JPEG")
	{
		source = MJPEGVideoSource::createNew(env, videoES);
	}
	else 
	{
		source = videoES;
	}
	return source;
}

RTPSink*  BaseServerMediaSubsession::createSink(UsageEnvironment& env, Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, const std::string& format, V4L2DeviceSource* source)
{
	RTPSink* videoSink = NULL;
	if (format == "video/MP2T")
	{
		videoSink = SimpleRTPSink::createNew(env, rtpGroupsock,rtpPayloadTypeIfDynamic, 90000, "video", "MP2T", 1, True, False); 
	}
	else if (format == "video/H264")
        {
		videoSink = H264VideoRTPSink::createNew(env, rtpGroupsock,rtpPayloadTypeIfDynamic);
	}
	else if (format == "video/VP8")
	{
		videoSink = VP8VideoRTPSink::createNew (env, rtpGroupsock,rtpPayloadTypeIfDynamic); 
	}
#if LIVEMEDIA_LIBRARY_VERSION_INT > 1414454400
	else if (format == "video/VP9")
	{
		videoSink = VP9VideoRTPSink::createNew (env, rtpGroupsock,rtpPayloadTypeIfDynamic); 
	}
	else if (format == "video/H265")
        {
		videoSink = H265VideoRTPSink::createNew(env, rtpGroupsock,rtpPayloadTypeIfDynamic);
	}
#endif	
	else if (format == "video/JPEG")
	{
		videoSink = JPEGVideoRTPSink::createNew (env, rtpGroupsock); 
    } 
#if LIVEMEDIA_LIBRARY_VERSION_INT >= 1536192000	
	else if (format =="video/RAW") 
	{ 
		std::string sampling;
		switch (source->getCaptureFormat()) {
			case V4L2_PIX_FMT_YUV444: sampling = "YCbCr-4:4:4"; break;
			case V4L2_PIX_FMT_YUYV: sampling = "YCbCr-4:2:2"; break;
		}
		videoSink = RawVideoRTPSink::createNew(env, rtpGroupsock, rtpPayloadTypeIfDynamic, source->getHeight(), source->getWidth(), 8, sampling.c_str());
		if (videoSink) {
			source->setAuxLine(videoSink->auxSDPLine());
		}
    } 
#endif	
	else if (format.find("audio/L16") == 0)
	{
		std::istringstream is(format);
		std::string dummy;
		getline(is, dummy, '/');	
		getline(is, dummy, '/');	
		std::string sampleRate("44100");
		getline(is, sampleRate, '/');	
		std::string channels("2");
		getline(is, channels);	
		videoSink = SimpleRTPSink::createNew(env, rtpGroupsock,rtpPayloadTypeIfDynamic, atoi(sampleRate.c_str()), "audio", "L16", atoi(channels.c_str()), True, False); 
	}
	return videoSink;
}

char const* BaseServerMediaSubsession::getAuxLine(V4L2DeviceSource* source,unsigned char rtpPayloadType)
{
	const char* auxLine = NULL;
	if (source)
	{
		std::ostringstream os; 
		os << "a=fmtp:" << int(rtpPayloadType) << " ";				
		os << source->getAuxLine();				
		os << "\r\n";		
		int width = source->getWidth();
		int height = source->getHeight();
		if ( (width > 0) && (height>0) ) {
			os << "a=x-dimensions:" << width << "," <<  height  << "\r\n";				
		}
		auxLine = strdup(os.str().c_str());
	} 
	return auxLine;
}

