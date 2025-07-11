#ifndef CAMERADATASHOW_H
#define CAMERADATASHOW_H

#include <QLabel>
#include <QSize>
#include <QCursor>

class CameraDataShow : public QLabel
{
    Q_OBJECT

public:
    CameraDataShow(QWidget *parent = nullptr);
    virtual ~CameraDataShow();

    void setROIExposureEnable(bool enable);
    void setScannerConnected(bool connected);
    void setIsShowData(bool flag){m_isShowData = flag;};
    void showCalibrationLine(bool isShowed);

public slots:
    void slotDisplayFrame(const QPixmap &pixmap);

signals:
    void sigSetROIRect(const QRect &r, int orgWidth = 0, int orgHeight = 0, int rotation = 0);

protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;

private:
    void initCursor();
    void getCameraRotateAngle();
    void calculateROIRect(QPoint centerPoint, int ROIRadius);
	void drawCalibrationLine(QPainter &painter);
    void drawROIRect(QPainter &painter);

    void setROIRectToCamera(bool markPos=false);

private:
    QPixmap m_lastPixmap;
    QRect   m_ROIRect;
    bool    m_showCaliLine { false };
    int     m_rotateAngle { 0 };
    QCursor m_cursorHand;
    bool    m_isShowData{ true };

    QPoint  m_relativePos;
    int     m_frameWidth{0};
    int     m_frameHeight{0};
    bool    m_needToResetROI{true};
    bool    m_needToResizeROI{false};
    QPoint  m_lastActualCenterPoint;

    bool    m_ROIExposureEnable{false};
    bool    m_isMouseEnter{false};  
    QTimer  m_delayResizeROI;
    int* m_buffer;
    QMutex* m_dataMutex;
};

#endif //CAMERADATASHOW_H
