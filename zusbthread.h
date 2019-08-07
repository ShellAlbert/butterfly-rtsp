#ifndef ZUSBTHREAD_H
#define ZUSBTHREAD_H

#include <QThread>

class ZUsbThread : public QThread
{
    Q_OBJECT
public:
    ZUsbThread();

    qint32 ZStartThread();
    qint32 ZStopThread();
protected:
    void run();

private:
    char m_quit;
};

#endif // ZUSBTHREAD_H
