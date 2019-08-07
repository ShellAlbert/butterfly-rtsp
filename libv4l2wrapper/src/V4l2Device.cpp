/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** V4l2Device.cpp
** 
** -------------------------------------------------------------------------*/

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

// libv4l2
#include <linux/videodev2.h>

#include "logger.h"

#include "V4l2Device.h"

#include <iostream>
// -----------------------------------------
//    V4L2Device
// -----------------------------------------
V4l2Device::V4l2Device(const V4L2DeviceParameters&  params, v4l2_buf_type deviceType) : m_params(params), m_fd(-1), m_deviceType(deviceType), m_bufferSize(0), m_format(0)
{
}

V4l2Device::~V4l2Device() 
{
    this->close();
}

void V4l2Device::close() 
{
    if (m_fd != -1)
        ::close(m_fd);

    m_fd = -1;
}

// query current format
void V4l2Device::queryFormat()
{
    struct v4l2_format     fmt;
    memset(&fmt,0,sizeof(fmt));
    fmt.type  = m_deviceType;
    if (0 == ioctl(m_fd,VIDIOC_G_FMT,&fmt))
    {
        m_format     = fmt.fmt.pix.pixelformat;
        m_width      = fmt.fmt.pix.width;
        m_height     = fmt.fmt.pix.height;
        m_bufferSize = fmt.fmt.pix.sizeimage;
    }
}

// intialize the V4L2 connection
bool V4l2Device::init(unsigned int mandatoryCapabilities)
{
    struct stat sb;
    if ( (stat(m_params.m_devName.c_str(), &sb)==0) && ((sb.st_mode & S_IFMT) == S_IFCHR) )
    {
        if (initdevice(m_params.m_devName.c_str(), mandatoryCapabilities) == -1)
        {
            std::cout<< "Cannot init device:" << m_params.m_devName;
        }
    }
    else
    {
        // open a normal file
        m_fd = open(m_params.m_devName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    }
    return (m_fd!=-1);
}

std::string fourcc(unsigned int format)
{
    char formatArray[] = { (char)(format&0xff), (char)((format>>8)&0xff), (char)((format>>16)&0xff), (char)((format>>24)&0xff), 0 };
    return std::string(formatArray, strlen(formatArray));
}

// intialize the V4L2 device
int V4l2Device::initdevice(const char *dev_name, unsigned int mandatoryCapabilities)
{
    std::cout<<"V4l2Device::initdevice begin\n";
    m_fd = open(dev_name,  O_RDWR | O_NONBLOCK);
    if (m_fd < 0)
    {
        std::cout<<"Cannot open device:" << m_params.m_devName << " " << strerror(errno);
        this->close();
        return -1;
    }
    if (checkCapabilities(m_fd,mandatoryCapabilities) !=0)
    {
        this->close();
        return -1;
    }
    if (configureFormat(m_fd) !=0)
    {
        this->close();
        return -1;
    }
    if (configureParam(m_fd) !=0)
    {
        this->close();
        return -1;
    }

    std::cout<<"zsy debug:\n";
    this->queryFormat();
    std::cout<<m_params.m_devName << ":" << fourcc(m_format) << " size:" << m_width << "x" << m_height<<"\n";

    std::cout<<"V4l2Device::initdevice end\n";
    return m_fd;
}

// check needed V4L2 capabilities
int V4l2Device::checkCapabilities(int fd, unsigned int mandatoryCapabilities)
{
    struct v4l2_capability cap;
    memset(&(cap), 0, sizeof(cap));
    if (-1 == ioctl(fd, VIDIOC_QUERYCAP, &cap))
    {
        std::cout<< "Cannot get capabilities for device:" << m_params.m_devName << " " << strerror(errno)<<"\n";
        return -1;
    }
    std::cout<< "driver:" << cap.driver << " capabilities:" << std::hex << cap.capabilities <<  " mandatory:" << mandatoryCapabilities << std::dec<<"\n";

    if ((cap.capabilities & V4L2_CAP_VIDEO_OUTPUT))
        std::cout<< m_params.m_devName << " support output\n";
    if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
        std::cout<<m_params.m_devName << " support capture\n";

    if ((cap.capabilities & V4L2_CAP_READWRITE))
        std::cout<< m_params.m_devName << " support read/write\n";
    if ((cap.capabilities & V4L2_CAP_STREAMING))
        std::cout<< m_params.m_devName << " support streaming\n";

    if ((cap.capabilities & V4L2_CAP_TIMEPERFRAME))
        std::cout<< m_params.m_devName << " support timeperframe\n";

    if((cap.capabilities & mandatoryCapabilities) != mandatoryCapabilities )
    {
        std::cout<< "Mandatory capability not available for device:" << m_params.m_devName<<"\n";
        return -1;
    }

    return 0;
}



// configure capture format 
int V4l2Device::configureFormat(int fd)
{
    // get current configuration
    this->queryFormat();
    std::cout<<m_params.m_devName << ":" << fourcc(m_format) << " size:" << m_width << "x" << m_height<<"\n";
#if 1
    unsigned int width = m_width;
    unsigned int height = m_height;
    if (m_params.m_width != 0)  {
        width= m_params.m_width;
    }
    if (m_params.m_height != 0)  {
        height= m_params.m_height;
    }
    if  (m_params.m_formatList.size()==0)
    {
        m_params.m_formatList.push_back(m_format);
    }

    // try to set format, widht, height
    std::list<unsigned int>::iterator it;
    for (it = m_params.m_formatList.begin(); it != m_params.m_formatList.end(); ++it) {
        unsigned int format = *it;
        if ( (format == m_format) && (width == m_width) && (height == m_height) )
        {
            // format is the current one
            std::cout<<"device's current format is okay,no need to set\n";
            return 0;
        }
        else if (this->configureFormat(fd, format, width, height)==0) {
            // format has been set
            return 0;
        }
    }
#endif
    return -1;
}

// configure capture format 
int V4l2Device::configureFormat(int fd, unsigned int format, unsigned int width, unsigned int height)
{
    std::cout<<"V4l2Device::configureFormat begin\n";
    struct v4l2_format   fmt;
    memset(&(fmt), 0, sizeof(fmt));
    fmt.type                = m_deviceType;
    fmt.fmt.pix.width       = width;
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = format;
    fmt.fmt.pix.field       = V4L2_FIELD_ANY;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
    {
        std::cout<< "Cannot set format for device:" << m_params.m_devName << " " << strerror(errno)<<"\n";
        return -1;
    }
    if (fmt.fmt.pix.pixelformat != format)
    {
        std::cout<< "Cannot set pixelformat to:" << fourcc(format) << " format is:" << fourcc(fmt.fmt.pix.pixelformat)<<"\n";
        return -1;
    }
    if ((fmt.fmt.pix.width != width) || (fmt.fmt.pix.height != height))
    {
        std::cout<< "Cannot set size to:" << width << "x" << height << " size is:"  << fmt.fmt.pix.width << "x" << fmt.fmt.pix.height<<"\n";
    }

    m_format     = fmt.fmt.pix.pixelformat;
    m_width      = fmt.fmt.pix.width;
    m_height     = fmt.fmt.pix.height;
    m_bufferSize = fmt.fmt.pix.sizeimage;

    std::cout<< m_params.m_devName << ":" << fourcc(m_format) << " size:" << m_width << "x" << m_height << " bufferSize:" << m_bufferSize<<"\n";

    std::cout<<"V4l2Device::configureFormat end\n";
    return 0;
}

// configure capture FPS 
int V4l2Device::configureParam(int fd)
{
    if (m_params.m_fps!=0)
    {
        struct v4l2_streamparm   param;
        memset(&(param), 0, sizeof(param));
        param.type = m_deviceType;
        param.parm.capture.timeperframe.numerator = 1;
        param.parm.capture.timeperframe.denominator = m_params.m_fps;

        if (ioctl(fd, VIDIOC_S_PARM, &param) == -1)
        {
            std::cout<< "Cannot set param for device:" << m_params.m_devName << " " << strerror(errno)<<"\n";
        }

        std::cout << "fps:" << param.parm.capture.timeperframe.numerator << "/" << param.parm.capture.timeperframe.denominator<<"\n";
        std::cout << "nbBuffer:" << param.parm.capture.readbuffers<<"\n";
    }

    return 0;
}


