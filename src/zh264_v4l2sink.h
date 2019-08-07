#ifndef ZH264_V4L2SINK_H
#define ZH264_V4L2SINK_H

#include <QThread>

class ZH264_V4L2Sink:public QThread
{
    Q_OBJECT
public:
    ZH264_V4L2Sink();

protected:
    void run();

private:
};

#endif // ZH264_V4L2SINK_H
