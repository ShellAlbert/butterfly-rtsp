/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** main.cpp
** 
** V4L2 RTSP streamer                                                                 
**                                                                                    
** H264 capture using V4L2                                                            
** RTSP using live555                                                                 
**                                                                                    
** -------------------------------------------------------------------------*/

#include <QApplication>
#include <zusbthread.h>
#include <src/zh264_v4l2sink.h>
int main(int argc, char** argv) 
{

    QApplication app(argc,argv);
    //create usb camera (mjpeg-yuv-h264) thread with alsa(usb).
//    ZUsbThread *threadUsb=new ZUsbThread;
//    threadUsb->ZStartThread();
    ZH264_V4L2Sink *thread=new ZH264_V4L2Sink;
    thread->start();
    int ret=app.exec();
    return ret;
}



