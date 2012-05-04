#pragma once

#include <QtGui/QMovie>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QAbstractVideoSurface;
class QSlider;
QT_END_NAMESPACE

class VideoPlayer : public QWidget
{
    Q_OBJECT
public:
    VideoPlayer(QWidget *parent = 0);
    ~VideoPlayer();

public slots:
    void openFile();
    void play();

private slots:
    void movieStateChanged(QMovie::MovieState state);
    void frameChanged(int frame);
    void setPosition(int frame);

private:
    bool presentImage(const QImage &image);

    QMovie movie;
    QAbstractVideoSurface *surface;
    QAbstractButton *playButton;
    QSlider *positionSlider;
};
