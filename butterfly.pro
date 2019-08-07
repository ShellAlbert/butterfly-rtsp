#-------------------------------------------------
#
# Project created by QtCreator 2019-07-02T14:47:33
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Tiger
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        zmainwidget.cpp \
    src/ALSACapture.cpp \
    src/DeviceSource.cpp \
    src/H264_V4l2DeviceSource.cpp \
    src/HTTPServer.cpp \
    src/MemoryBufferSink.cpp \
    src/MJPEGVideoSource.cpp \
    src/MulticastServerMediaSubsession.cpp \
    src/ServerMediaSubsession.cpp \
    src/TSServerMediaSubsession.cpp \
    src/UnicastServerMediaSubsession.cpp \
    libv4l2wrapper/src/logger.cpp  \
    libv4l2wrapper/src/V4l2Capture.cpp  \
    libv4l2wrapper/src/V4l2Device.cpp  \
    libv4l2wrapper/src/V4l2MmapDevice.cpp  \
    libv4l2wrapper/src/V4l2Output.cpp \
    zusbthread.cpp \
    src/zh264_v4l2sink.cpp

HEADERS  += zmainwidget.h \
    inc/AddH26xMarkerFilter.h \
    inc/ALSACapture.h \
    inc/DeviceInterface.h \
    inc/DeviceSource.h \
    inc/H264_V4l2DeviceSource.h \
    inc/HTTPServer.h \
    inc/MemoryBufferSink.h \
    inc/MJPEGVideoSource.h \
    inc/MulticastServerMediaSubsession.h \
    inc/ServerMediaSubsession.h \
    inc/TSServerMediaSubsession.h \
    inc/UnicastServerMediaSubsession.h \
    libv4l2wrapper/inc/logger.h \
    libv4l2wrapper/inc/V4l2Access.h \
    libv4l2wrapper/inc/V4l2Capture.h \
    libv4l2wrapper/inc/V4l2Device.h \
    libv4l2wrapper/inc/V4l2MmapDevice.h \
    libv4l2wrapper/inc/V4l2Output.h \
    libv4l2wrapper/inc/V4l2ReadWriteDevice.h \
    zusbthread.h \
    src/zh264_v4l2sink.h

INCLUDEPATH += $$PWD/inc
INCLUDEPATH += $$PWD/libv4l2wrapper/inc
INCLUDEPATH += $$PWD/../3rdlibs/live/BasicUsageEnvironment/include
INCLUDEPATH += $$PWD/../3rdlibs/live/UsageEnvironment/include
INCLUDEPATH += $$PWD/../3rdlibs/live/groupsock/include
INCLUDEPATH += $$PWD/../3rdlibs/live/liveMedia/include
INCLUDEPATH += /usr/include/glib-2.0 /usr/lib/aarch64-linux-gnu/glib-2.0/include
INCLUDEPATH += /usr/include/gstreamer-1.0

LIBS += $$PWD/../3rdlibs/live/liveMedia/libliveMedia.a
LIBS += $$PWD/../3rdlibs/live/UsageEnvironment/libUsageEnvironment.a
LIBS += $$PWD/../3rdlibs/live/groupsock/libgroupsock.a
LIBS += $$PWD/../3rdlibs/live/BasicUsageEnvironment/libBasicUsageEnvironment.a
LIBS += -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 -lgstapp-1.0
LIBS += -lasound
