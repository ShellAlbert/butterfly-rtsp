#include "zusbthread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include <sstream>

// libv4l2
#include <linux/videodev2.h>

// live555
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>

// project
#include "logger.h"

#include "V4l2Device.h"
#include "V4l2Capture.h"
#include "V4l2Output.h"

#include "H264_V4l2DeviceSource.h"
#include "ServerMediaSubsession.h"
#include "UnicastServerMediaSubsession.h"
#include "MulticastServerMediaSubsession.h"
#include "TSServerMediaSubsession.h"
#include "HTTPServer.h"

#define HAVE_ALSA 1

#ifdef HAVE_ALSA
#include "ALSACapture.h"
#endif

#include <QDebug>

// -----------------------------------------
//    create UserAuthenticationDatabase for RTSP server
// -----------------------------------------
UserAuthenticationDatabase* createUserAuthenticationDatabase(const std::list<std::string> & userPasswordList, const char* realm)
{
    UserAuthenticationDatabase* auth = NULL;
    if (userPasswordList.size() > 0)
    {
        auth = new UserAuthenticationDatabase(realm, (realm != NULL) );

        std::list<std::string>::const_iterator it;
        for (it = userPasswordList.begin(); it != userPasswordList.end(); ++it)
        {
            std::istringstream is(*it);
            std::string user;
            getline(is, user, ':');
            std::string password;
            getline(is, password,'\n');
            auth->addUserRecord(user.c_str(), password.c_str());
        }
    }

    return auth;
}

// -----------------------------------------
//    create RTSP server
// -----------------------------------------
RTSPServer* createRTSPServer(UsageEnvironment& env, unsigned short rtspPort, unsigned short rtspOverHTTPPort, int timeout, unsigned int hlsSegment, const std::list<std::string> & userPasswordList, const char* realm, const std::string & webroot)
{
    UserAuthenticationDatabase* auth = createUserAuthenticationDatabase(userPasswordList, realm);
    RTSPServer* rtspServer = HTTPServer::createNew(env, rtspPort, auth, timeout, hlsSegment, webroot);
    if (rtspServer != NULL)
    {
        // set http tunneling
        if (rtspOverHTTPPort)
        {
            rtspServer->setUpTunnelingOverHTTP(rtspOverHTTPPort);
        }
    }
    return rtspServer;
}


// -----------------------------------------
//    create FramedSource server
// -----------------------------------------
FramedSource* createFramedSource(UsageEnvironment* env, int format, DeviceInterface* videoCapture, int outfd, int queueSize, bool useThread, bool repeatConfig)
{
    FramedSource* source = NULL;
    if (format == V4L2_PIX_FMT_H264)
    {
        source = H264_V4L2DeviceSource::createNew(*env, videoCapture, outfd, queueSize, useThread, repeatConfig, false);
    }
    else if (format == V4L2_PIX_FMT_HEVC)
    {
        source = H265_V4L2DeviceSource::createNew(*env, videoCapture, outfd, queueSize, useThread, repeatConfig, false);
    }
    else
    {
        source = V4L2DeviceSource::createNew(*env, videoCapture, outfd, queueSize, useThread);
    }
    return source;
}

// -----------------------------------------
//    add an RTSP session
// -----------------------------------------
int addSession(RTSPServer* rtspServer, const std::string & sessionName, const std::list<ServerMediaSubsession*> & subSession)
{
    int nbSubsession = 0;
    if (subSession.empty() == false)
    {
        UsageEnvironment& env(rtspServer->envir());
        ServerMediaSession* sms = ServerMediaSession::createNew(env, sessionName.c_str());
        if (sms != NULL)
        {
            std::list<ServerMediaSubsession*>::const_iterator subIt;
            for (subIt = subSession.begin(); subIt != subSession.end(); ++subIt)
            {
                sms->addSubsession(*subIt);
                nbSubsession++;
            }

            rtspServer->addServerMediaSession(sms);

            char* url = rtspServer->rtspURL(sms);
            if (url != NULL)
            {
                LOG(NOTICE) << "Play this stream using the URL \"" << url << "\"";
                delete[] url;
            }
        }
    }
    return nbSubsession;
}

// -----------------------------------------
//    convert V4L2 pix format to RTP mime
// -----------------------------------------
std::string getVideoRtpFormat(int format)
{
    std::string rtpFormat;
    switch(format)
    {
    case V4L2_PIX_FMT_HEVC : rtpFormat = "video/H265"; break;
    case V4L2_PIX_FMT_H264 : rtpFormat = "video/H264"; break;
    case V4L2_PIX_FMT_MJPEG: rtpFormat = "video/JPEG"; break;
    case V4L2_PIX_FMT_JPEG : rtpFormat = "video/JPEG"; break;
    case V4L2_PIX_FMT_VP8  : rtpFormat = "video/VP8" ; break;
    case V4L2_PIX_FMT_VP9  : rtpFormat = "video/VP9" ; break;
    case V4L2_PIX_FMT_YUYV : rtpFormat = "video/RAW" ; break;
    }

    return rtpFormat;
}

// -----------------------------------------
//    convert string video format to fourcc
// -----------------------------------------
int decodeVideoFormat(const char* fmt)
{
    char fourcc[4];
    memset(&fourcc, 0, sizeof(fourcc));
    if (fmt != NULL)
    {
        strncpy(fourcc, fmt, 4);
    }
    return v4l2_fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
}

// -----------------------------------------
//    convert string audio format to pcm
// -----------------------------------------
#ifdef HAVE_ALSA
snd_pcm_format_t decodeAudioFormat(const std::string& fmt)
{
    snd_pcm_format_t audioFmt = SND_PCM_FORMAT_UNKNOWN;
    if (fmt == "S16_BE") {
        audioFmt = SND_PCM_FORMAT_S16_BE;
    } else if (fmt == "S16_LE") {
        audioFmt = SND_PCM_FORMAT_S16_LE;
    } else if (fmt == "S24_BE") {
        audioFmt = SND_PCM_FORMAT_S24_BE;
    } else if (fmt == "S24_LE") {
        audioFmt = SND_PCM_FORMAT_S24_LE;
    } else if (fmt == "S32_BE") {
        audioFmt = SND_PCM_FORMAT_S32_BE;
    } else if (fmt == "S32_LE") {
        audioFmt = SND_PCM_FORMAT_S32_LE;
    } else if (fmt == "ALAW") {
        audioFmt = SND_PCM_FORMAT_A_LAW;
    } else if (fmt == "MULAW") {
        audioFmt = SND_PCM_FORMAT_MU_LAW;
    } else if (fmt == "S8") {
        audioFmt = SND_PCM_FORMAT_S8;
    } else if (fmt == "MPEG") {
        audioFmt = SND_PCM_FORMAT_MPEG;
    }
    return audioFmt;
}
std::string getAudioRtpFormat(snd_pcm_format_t format, int sampleRate, int channels)
{
    std::ostringstream os;
    os << "audio/";
    switch (format) {
    case SND_PCM_FORMAT_A_LAW:
        os << "PCMA";
        break;
    case SND_PCM_FORMAT_MU_LAW:
        os << "PCMU";
        break;
    case SND_PCM_FORMAT_S8:
        os << "L8";
        break;
    case SND_PCM_FORMAT_S24_BE:
    case SND_PCM_FORMAT_S24_LE:
        os << "L24";
        break;
    case SND_PCM_FORMAT_S32_BE:
    case SND_PCM_FORMAT_S32_LE:
        os << "L32";
        break;
    case SND_PCM_FORMAT_MPEG:
        os << "MPEG";
        break;
    default:
        os << "L16";
        break;
    }
    os << "/" << sampleRate << "/" << channels;
    return os.str();
}
#endif

// -------------------------------------------------------
//    decode multicast url <group>:<rtp_port>:<rtcp_port>
// -------------------------------------------------------
void decodeMulticastUrl(const std::string & maddr, in_addr & destinationAddress, unsigned short & rtpPortNum, unsigned short & rtcpPortNum)
{
    std::istringstream is(maddr);
    std::string ip;
    getline(is, ip, ':');
    if (!ip.empty())
    {
        destinationAddress.s_addr = inet_addr(ip.c_str());
    }

    std::string port;
    getline(is, port, ':');
    rtpPortNum = 20000;
    if (!port.empty())
    {
        rtpPortNum = atoi(port.c_str());
    }
    rtcpPortNum = rtpPortNum+1;
}

// -------------------------------------------------------
//    split video,audio device
// -------------------------------------------------------
void decodeDevice(const std::string & device, std::string & videoDev, std::string & audioDev)
{
    std::istringstream is(device);
    getline(is,videoDev, ',');
    getline(is,audioDev,'\n');
}

std::string getDeviceName(const std::string & devicePath)
{
    std::string deviceName(devicePath);
    size_t pos = deviceName.find_last_of('/');
    if (pos != std::string::npos)
    {
        deviceName.erase(0,pos+1);
    }
    return deviceName;
}


/* ---------------------------------------------------------------------------
**  get a "deviceid" from uevent sys file
** -------------------------------------------------------------------------*/
#ifdef HAVE_ALSA
std::string getDeviceId(const std::string& evt)
{
    std::string deviceid;
    std::istringstream f(evt);
    std::string key;
    while(getline(f,key,'='))
    {
        std::string value;
        if (getline(f,value,'\n'))
        {
            if ((key =="PRODUCT") || (key == "PCI_SUBSYS_ID") )
            {
                deviceid = value;
                break;
            }
        }
    }
    return deviceid;
}

std::string  getV4l2Alsa(const std::string& v4l2device) {
    std::string audioDevice(v4l2device);

    std::map<std::string,std::string> videodevices;
    std::string video4linuxPath("/sys/class/video4linux");
    DIR *dp = opendir(video4linuxPath.c_str());
    if (dp != NULL) {
        struct dirent *entry = NULL;
        while((entry = readdir(dp))) {
            std::string devicename;
            std::string deviceid;
            if (strstr(entry->d_name,"video") == entry->d_name) {
                std::string ueventPath(video4linuxPath);
                ueventPath.append("/").append(entry->d_name).append("/device/uevent");
                std::ifstream ifsd(ueventPath.c_str());
                deviceid = std::string(std::istreambuf_iterator<char>{ifsd}, {});
                deviceid.erase(deviceid.find_last_not_of("\n")+1);
            }

            if (!deviceid.empty()) {
                videodevices[entry->d_name] = getDeviceId(deviceid);
            }
        }
        closedir(dp);
    }

    std::map<std::string,std::string> audiodevices;
    int rcard = -1;
    while ( (snd_card_next(&rcard) == 0) && (rcard>=0) )
    {
        void **hints = NULL;
        if (snd_device_name_hint(rcard, "pcm", &hints) >= 0)
        {
            void **str = hints;
            while (*str)
            {
                std::ostringstream os;
                os << "/sys/class/sound/card" << rcard << "/device/uevent";

                std::ifstream ifs(os.str().c_str());
                std::string deviceid = std::string(std::istreambuf_iterator<char>{ifs}, {});
                deviceid.erase(deviceid.find_last_not_of("\n")+1);
                deviceid = getDeviceId(deviceid);

                if (!deviceid.empty())
                {
                    if (audiodevices.find(deviceid) == audiodevices.end())
                    {
                        std::string audioname = snd_device_name_get_hint(*str, "NAME");
                        audiodevices[deviceid] = audioname;
                    }
                }

                str++;
            }

            snd_device_name_free_hint(hints);
        }
    }

    auto deviceId  = videodevices.find(getDeviceName(v4l2device));
    if (deviceId != videodevices.end())
    {
        auto audioDeviceIt = audiodevices.find(deviceId->second);

        if (audioDeviceIt != audiodevices.end())
        {
            audioDevice = audioDeviceIt->second;
            std::cout <<  v4l2device << "=>" << audioDevice << std::endl;
        }
    }


    return audioDevice;
}
#endif

ZUsbThread::ZUsbThread()
{

}
qint32 ZUsbThread::ZStartThread()
{
    this->start();
    return 0;
}
qint32 ZUsbThread::ZStopThread()
{
    this->m_quit=1;
    return 0;
}
void ZUsbThread::run()
{
    V4l2Access::IoType ioTypeIn  = V4l2Access::IOTYPE_MMAP;
    std::string url = "unicast"; //unicast url.
    std::string murl = "multicast"; //multicast url.
    std::string tsurl = "ts";
    bool useThread = true;
    std::string maddr;
    bool repeatConfig = true;

    //init logger.
    int verbose=1;//no verbose.
    //int verbose=1;//verbose.
    //int verbose=2;//very verbose.
    initLogger(verbose);

    //create live555 environment
    TaskScheduler* scheduler=BasicTaskScheduler::createNew();
    UsageEnvironment* env=BasicUsageEnvironment::createNew(*scheduler);

    //split multicast info.
    struct in_addr destinationAddress;
    destinationAddress.s_addr=chooseRandomIPv4SSMAddress(*env);
    unsigned short rtpPortNum=20000;
    unsigned short rtcpPortNum=rtpPortNum+1;
    unsigned char ttl=5;
    decodeMulticastUrl(maddr,destinationAddress,rtpPortNum,rtcpPortNum);

    //create RTSP server.
    unsigned short rtspPort=6803;
    unsigned short rtspOverHTTPPort=0;
    int timeout=65;
    unsigned int hlsSegment=0;
    std::list<std::string> userPasswordList;
    userPasswordList.push_back("zhangshaoyan:12345678");
    const char* realm=NULL;
    std::string webroot;
    RTSPServer *rtspServer=createRTSPServer(*env,rtspPort,rtspOverHTTPPort,timeout,hlsSegment,userPasswordList,realm,webroot);
    if(rtspServer==NULL)
    {
        qDebug()<<"<error>:failed to create RTSP server:"<<env->getResultMsg();
        return;
    }

    // Init video capture.
    StreamReplicator* videoReplicator = NULL;
    std::string rtpFormat;
    if(1)
    {
         //camera default parameters. h264:1920x1080@30fps.
        std::string videoDev="/dev/video0";
        //std::string videoDev="/dev/video2";

//        std::list<unsigned int> videoformatList;
//        videoformatList.push_back(V4L2_PIX_FMT_H264);

        int width=1280;//V4L2 camera width.
        int height=720;//V4L2 camera height.
        int fps=30;//V4L2 camera framerate.

        int verbose=0;//no verbose.
        //int verbose=1;//verbose.
        //int verbose=2;//very verbose.

        qDebug()<<"<info>:create video source:"<<videoDev.c_str();

        V4L2DeviceParameters param(videoDev.c_str(),V4L2_PIX_FMT_H264,width,height,fps,verbose);
        V4l2Capture* videoCapture=V4l2Capture::create(param,ioTypeIn);
        if (videoCapture)
        {
            rtpFormat.assign(getVideoRtpFormat(videoCapture->getFormat()));
            if(rtpFormat.empty())
            {
                delete videoCapture;
                qDebug()<<"<error>:no streaming format supported for device"<<videoDev.c_str();
            }else{
                int outfd=-1;//we do not dump h264 to local file,so here set to -1.
                int queueSize=10;//Number of frame queue.
                FramedSource* videoSource=createFramedSource(env,videoCapture->getFormat(),new DeviceCaptureAccess<V4l2Capture>(videoCapture),outfd,queueSize,useThread,repeatConfig);
                if(videoSource==NULL)
                {
                    delete videoCapture;
                    qDebug()<<"<error>:failed to create video source"<<videoDev.c_str();
                }else{
                    //extend buffer size if needed.
                    if(videoCapture->getBufferSize()>OutPacketBuffer::maxSize)
                    {
                        OutPacketBuffer::maxSize=videoCapture->getBufferSize();
                    }
                    videoReplicator=StreamReplicator::createNew(*env,videoSource,false);
                    qDebug()<<"<info>:video source okay"<<videoDev.c_str();
                }
            }
        }
    }

    // Init Audio Capture
    StreamReplicator* audioReplicator = NULL;
    std::string rtpAudioFormat;
    if(1)
    {
        std::string audioDev="plughw:CARD=USBSA,DEV=0";
        std::list<snd_pcm_format_t> audioFmtList;
        audioFmtList.push_back(SND_PCM_FORMAT_S16_LE);
        audioFmtList.push_back(SND_PCM_FORMAT_S16_BE);
        int audioFreq=44100;//ALSA capture frequency.
        int audioNbChannels=2;//ALSA capture channels.

        int verbose=0;//no verbose.
        //int verbose=1;//verbose.
        //int verbose=2;//very verbose.

        qDebug()<<"<info>:create audio source:"<<audioDev.c_str();

        ALSACaptureParameters param(audioDev.c_str(),audioFmtList,audioFreq,audioNbChannels,verbose);
        ALSACapture* audioCapture=ALSACapture::createNew(param);
        if(audioCapture)
        {
            int outfd=-1;//we do not dump pcm to local file,so here set to -1.
            int queueSize=10;//Number of frame queue.
            FramedSource* audioSource=V4L2DeviceSource::createNew(*env,new DeviceCaptureAccess<ALSACapture>(audioCapture),outfd,queueSize,useThread);
            if(audioSource==NULL)
            {
                delete audioCapture;
                qDebug()<<"<error>:failed to init audio device"<<audioDev.c_str();

            }else{
                rtpAudioFormat.assign(getAudioRtpFormat(audioCapture->getFormat(),audioCapture->getSampleRate(),audioCapture->getChannels()));

                //extend buffer size if needed.
                if(audioCapture->getBufferSize()>OutPacketBuffer::maxSize)
                {
                    OutPacketBuffer::maxSize=audioCapture->getBufferSize();
                }
                audioReplicator=StreamReplicator::createNew(*env,audioSource,false);
                qDebug()<<"<info>:audio source okay"<<audioDev.c_str();
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //1.create unicast session.(tcp).
    //2.create mutlcast session.(udp).
    //3.create HLS session.

    int nbSource=0;

    //1.Create Unicast Session.
    std::list<ServerMediaSubsession*> subSession;
    if(videoReplicator)
    {
        subSession.push_back(UnicastServerMediaSubsession::createNew(*env,videoReplicator,rtpFormat));
    }
    if(audioReplicator)
    {
        subSession.push_back(UnicastServerMediaSubsession::createNew(*env,audioReplicator,rtpAudioFormat));
    }
    nbSource+=addSession(rtspServer,url,subSession);


    //2.Create Multicast Session.
    bool multicast=true;
    if(multicast)
    {
        LOG(NOTICE) << "RTP  address " << inet_ntoa(destinationAddress) << ":" << rtpPortNum;
        LOG(NOTICE) << "RTCP address " << inet_ntoa(destinationAddress) << ":" << rtcpPortNum;

        std::list<ServerMediaSubsession*> subSession;
        if(videoReplicator)
        {
            subSession.push_back(MulticastServerMediaSubsession::createNew(*env, destinationAddress, Port(rtpPortNum), Port(rtcpPortNum), ttl, videoReplicator, rtpFormat));
            //increment ports for next sessions.
            rtpPortNum+=2;
            rtcpPortNum+=2;
        }

        if(audioReplicator)
        {
            subSession.push_back(MulticastServerMediaSubsession::createNew(*env, destinationAddress, Port(rtpPortNum), Port(rtcpPortNum), ttl, audioReplicator, rtpAudioFormat));

            //increment ports for next sessions.
            rtpPortNum+=2;
            rtcpPortNum+=2;
        }
        nbSource+=addSession(rtspServer,murl,subSession);
    }

    //3.Create HLS Session.
    if(hlsSegment>0)
    {
        std::list<ServerMediaSubsession*> subSession;
        if (videoReplicator)
        {
            subSession.push_back(TSServerMediaSubsession::createNew(*env, videoReplicator, rtpFormat, audioReplicator, rtpAudioFormat, hlsSegment));
        }
        nbSource+=addSession(rtspServer,tsurl,subSession);

        struct in_addr ip;
        ip.s_addr = ourIPAddress(*env);
        LOG(NOTICE) << "HLS       http://" << inet_ntoa(ip) << ":" << rtspPort << "/" << tsurl << ".m3u8";
        LOG(NOTICE) << "MPEG-DASH http://" << inet_ntoa(ip) << ":" << rtspPort << "/" << tsurl << ".mpd";
    }

    //4.enter the main loop.
    if(nbSource>0)
    {
        qDebug()<<"<info>:enter main loop...";
        env->taskScheduler().doEventLoop(&m_quit);
        qDebug()<<"Exiting...";
    }

    Medium::close(rtspServer);
    env->reclaim();
    delete scheduler;
}
