#include "zh264_v4l2sink.h"

extern "C"
{
    #include <gst/gst.h>
    #include <glib.h>
    #include <gst/app/gstappsink.h>
    #include <gst/app/app.h>
    #include <gst/gstbuffer.h>
}
#include <QFile>
#include <QDebug>
ZH264_V4L2Sink::ZH264_V4L2Sink()
{

}
void ZH264_V4L2Sink::run()
{
    GstAppSink *appsink;
    gst_init(NULL,NULL);
    GstElement *pipeline,*sink;
    //=(gchar*)"v4l2src device=/dev/video1 ! image/jpeg,width=1280,height=720,framerate=30/1 ! jpegparse ! jpegdec ! video/x-raw,format=(string)I420,width=(int)1280,height=(int)720 ! omxh264enc ! video/x-h264,stream-format=(string)byte-stream ! appsink name=sink";
    GError *err=NULL;
    char *dev_node="/dev/video1";
    gchar *desc=g_strdup_printf("v4l2src device=%s ! image/jpeg,width=1280,height=720,framerate=30/1 ! jpegparse ! jpegdec ! video/x-raw,format=(string)I420,width=(int)1280,height=(int)720 ! omxh264enc ! video/x-h264,stream-format=(string)byte-stream ! appsink name=sink",dev_node);
    pipeline=gst_parse_launch(desc,&err);
    if(err!=NULL)
    {
        g_print("could not construct pipeline :%s\n",err->message);
        g_error_free(err);
        return;
    }
    gst_element_set_state(pipeline,GST_STATE_PAUSED);
    printf("launch okay\n");

    sink=gst_bin_get_by_name(GST_BIN(pipeline),"sink");
    appsink=(GstAppSink*)sink;
    gst_app_sink_set_max_buffers(appsink,10);
    gst_app_sink_set_drop(appsink,true);

    gst_element_set_state(pipeline,GST_STATE_PLAYING);
    int i=0;
    QFile file("zsy.h264");
    if(!file.open(QIODevice::WriteOnly))
    {
        qDebug()<<"failed to create h264 file!\n";
        return;
    }
    while(!gst_app_sink_is_eos(appsink))
    {
        GstSample *sample=gst_app_sink_pull_sample(GST_APP_SINK(appsink));
        if(sample==NULL)
        {
            printf("gst_app_sink_pull_sample() NULL");
            break;
        }
        GstBuffer *buffer=gst_sample_get_buffer(sample);
        GstMapInfo map;
        gst_buffer_map(buffer,&map,GST_MAP_READ);

        //allocate appropriate buffer to store compressed image.
        //char *pRet=new char[map.size];
        //memmove(pRet,map.data,map.size);
        //write to to file
        file.write((const char*)map.data,map.size);
        file.flush();

        gst_buffer_unmap(buffer,&map);
        gst_sample_unref(sample);
        printf("%d:get image okay:%d\n",i++,map.size);
    }

    gst_element_set_state(pipeline,GST_STATE_NULL);
    gst_object_unref(pipeline);
}
