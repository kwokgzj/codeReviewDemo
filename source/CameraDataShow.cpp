#include "CameraDataShow.h"
#include <QPainter>
#include <QTransform>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QTimer>
#include <QTransform>
#include <QToolTip>

#include "scan/CameraManager.h"
#include "lang/language.h"
#include "log/RvDebug.h"
#include "ScanFrameHandler.h"
#include "RvApplication.h"

#define CALIBRATION_LINE_HEIGHT		2
#define ROI_RADIUS_FACTOR   6

CameraDataShow::CameraDataShow(QWidget *parent)
    : QLabel(parent)
{
    initCursor();
    m_delayResizeROI.setSingleShot(true);
    connect(&m_delayResizeROI, &QTimer::timeout, [=]{ m_needToResizeROI = true; });
    
    m_buffer = new int[1000];
}

CameraDataShow::~CameraDataShow()
{
    qDebug() << "~CameraDataShow end";

    if (m_buffer) {
        delete[] m_buffer;
        // delete[] m_buffer;
    }
}


void CameraDataShow::setROIExposureEnable(bool enable)
{
    m_ROIExposureEnable = enable;

    if (!m_ROIExposureEnable)
    {
        setMouseTracking(false);
    }
    else
    {
        setMouseTracking(true);

        if(isVisible())
        {
            setROIRectToCamera();
        }
    }
}

void CameraDataShow::setScannerConnected(bool connected)
{
    if (!connected)
    {
        m_needToResetROI = true;
        m_ROIRect = QRect();
        setMouseTracking(false);
        m_lastPixmap.fill(QColor("#474747"));
        update();
    }
}

void CameraDataShow::showCalibrationLine(bool isShowed)
{
    m_showCaliLine = isShowed;
}


void CameraDataShow::slotDisplayFrame(const QPixmap &pixmap)
{
    if(!m_isShowData)
    {
        return;
    }
    m_dataMutex->lock();

    getCameraRotateAngle();
    //根据相机画面的旋转角度判断是否需要旋转画面；
    if (0 != m_rotateAngle)
    {
        QTransform matrix;
        matrix.rotate(m_rotateAngle);
        m_lastPixmap = pixmap.transformed(matrix, Qt::SmoothTransformation);
    }
    else
    {
        m_lastPixmap = pixmap;
    }

    //分辨率发生变化时，更新；
    if(m_frameWidth != m_lastPixmap.width() || m_frameHeight != m_lastPixmap.height())
    {
        m_frameWidth = m_lastPixmap.width();
        m_frameHeight = m_lastPixmap.height();
        m_needToResetROI = true;
    }

    //由于显示窗口和帧画面的宽高比不一定相等，为了铺满显示窗口需要在纵向或横向进行帧画面裁剪；
    m_lastPixmap = m_lastPixmap.scaledToWidth(width(), Qt::SmoothTransformation);
    if(m_lastPixmap.height()> height())
    {
        m_lastPixmap = m_lastPixmap.scaledToHeight(height(), Qt::SmoothTransformation);
        m_relativePos = QPoint((width() - m_lastPixmap.width())/ 2, 0);
    }
    else
    {
        m_relativePos = QPoint(0, (height() - m_lastPixmap.height())/ 2);
    }
    update();

    if(m_needToResetROI)
    {
        m_needToResetROI = false;
        auto centerPos = QPoint(width()/ 2, height()/ 2);
        calculateROIRect(centerPos, std::min(m_lastPixmap.width(), m_lastPixmap.height())/ ROI_RADIUS_FACTOR);
        setROIRectToCamera(true);
    }
    else if(m_needToResizeROI)
    {
        m_needToResizeROI = false;

        //计算帧数据与显示窗口的放缩比例；
        float scale = (float)m_frameWidth/ width();
        //纵向显示时用高度来计算scale；
        if(m_rotateAngle == 90 || m_rotateAngle == 270)
        {
            scale = (float)m_frameHeight/ height();
        }

        //使用上一次的绝对ROI区域计算出当前窗口对应的ROI区域的中心点；
        auto centerPos = QPoint(m_lastActualCenterPoint.x()/ scale, m_lastActualCenterPoint.y()/ scale);

        calculateROIRect(centerPos+m_relativePos, std::min(m_lastPixmap.width(), m_lastPixmap.height())/ ROI_RADIUS_FACTOR);
        setROIRectToCamera();
    }
    m_dataMutex->unLock();
}


void CameraDataShow::paintEvent(QPaintEvent *event)
{
    QPainter painter;
    painter.begin(this);
    if (!m_lastPixmap.isNull())
    {
        painter.drawPixmap(m_relativePos, m_lastPixmap);

        if (m_ROIExposureEnable)
        {
            drawROIRect(painter);
        }
        else if (m_showCaliLine)
        {
            drawCalibrationLine(painter);
        }
    }

    painter.end();
}

void CameraDataShow::resizeEvent(QResizeEvent *event)
{
    if (m_ROIExposureEnable && !m_needToResetROI)
    {
        rvInfo << "timer start";
        m_delayResizeROI.start(50);
    }

    update();
}

void CameraDataShow::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_ROIExposureEnable)
    {
        return;
    }

    calculateROIRect(event->pos(), std::min(m_lastPixmap.width(), m_lastPixmap.height())/ ROI_RADIUS_FACTOR);
    setROIRectToCamera(true);
}

void CameraDataShow::mouseMoveEvent(QMouseEvent *event)
{
    bool rsl = false;
    do
    {
        //自动曝光是否开启；
        if (!m_ROIExposureEnable)
        {
            break;
        }

        //鼠标是否在相机画面范围内；
        if(!(event->pos().x()>= m_relativePos.x() && event->pos().x()< (m_relativePos.x()+ m_lastPixmap.width()) &&
                event->pos().y()>= m_relativePos.y() && event->pos().y()< (m_relativePos.y()+ m_lastPixmap.height())))
        {
            break;
        }

        if(m_isMouseEnter)
        {
            return;
        }

        setCursor(m_cursorHand);
        m_isMouseEnter = true;

        QTimer::singleShot(500, [&]{
            if(m_isMouseEnter)
            {
                foreach(auto child, children())
                {
                    if(child->objectName() != "ScaleButton")
                    {
                        continue;
                    }

                    QRect rect(mapToGlobal(static_cast<QWidget*>(child)->geometry().topLeft()),
                               mapToGlobal(static_cast<QWidget*>(child)->geometry().bottomRight()));
                    //鼠标在悬浮button上则不弹出tips;
                    if(rect.contains(QCursor::pos()))
                    {
                        return;
                    }
                }

                QToolTip::showText(QCursor::pos()+ QPoint(10, 10), LAN_CLICK_PARTIAL_METERING, nullptr, QRect(), 2000);
            }
        });

        rsl = true;
    }while(0);

    if(!rsl)
    {
        setCursor(QCursor(Qt::ArrowCursor));
        QToolTip::hideText();
        m_isMouseEnter = false;
    }
}


void CameraDataShow::initCursor()
{
    QPixmap pixmap;
    pixmap.load(":/res/icon/tip/mouseHand.png");
    m_cursorHand = QCursor(pixmap, 10, 1);
}

void CameraDataShow::getCameraRotateAngle()
{
    std::map<CameraManager::CameraParamType, std::string> paramMap = rvApp->cameraManager()->getCameraParam();
    auto iter = paramMap.find(CameraManager::CAMERA_PARAM_DEPTH_ROTATE_ANGLE);
    if (iter != paramMap.end())
    {
        m_rotateAngle = stoi(iter->second);
    }
    else
    {
        m_rotateAngle = 0;
    }

    //把负角度转换成正的；
    while(m_rotateAngle< 0)
    {
        m_rotateAngle += 360;
    }
}

void CameraDataShow::calculateROIRect(QPoint centerPoint, int ROIRadius)
{
    QRect ROIRect = QRect(centerPoint.x()- ROIRadius, centerPoint.y()- ROIRadius, ROIRadius*2, ROIRadius*2);

    int rightBorder = (m_lastPixmap.width()+ m_relativePos.x())- ROIRect.width();
    if(ROIRect.x()< m_relativePos.x()) //左端越界；
    {
        ROIRect.setX(m_relativePos.x());
    }
    else if(ROIRect.x()> rightBorder) //右端越界；
    {
        ROIRect.setX(rightBorder);
    }

    int bottomBorder = (m_lastPixmap.height()+ m_relativePos.y())- ROIRect.height();
    if(ROIRect.y()< m_relativePos.y()) //顶部越界；
    {
        ROIRect.setY(m_relativePos.y());
    }
    else if(ROIRect.y()> bottomBorder) //底部越界；
    {
        ROIRect.setY(bottomBorder);
    }

    ROIRect.setWidth(ROIRadius*2);
    ROIRect.setHeight(ROIRadius*2);
    m_ROIRect = QRect(ROIRect.x()- m_relativePos.x(), ROIRect.y()- m_relativePos.y(), ROIRect.width(), ROIRect.height());
}

void CameraDataShow::drawCalibrationLine(QPainter &painter)
{
    int lineWidth = m_lastPixmap.size().width() > m_lastPixmap.size().height() ? m_lastPixmap.size().height() / 2 : m_lastPixmap.size().width() / 2;

    QPen pen;
    pen.setColor("#1097FF");
    pen.setWidth(CALIBRATION_LINE_HEIGHT);
    painter.setPen(pen);
    QPoint p1Begin((width() - lineWidth) / 2, (height() - CALIBRATION_LINE_HEIGHT) / 2);
    QPoint p1End((width() - lineWidth) / 2 + lineWidth, (height() - CALIBRATION_LINE_HEIGHT) / 2);
    painter.drawLine(p1Begin, p1End);
    QPoint p2Begin((width() - CALIBRATION_LINE_HEIGHT) / 2, (height() - lineWidth) / 2);
    QPoint p2End((width() - CALIBRATION_LINE_HEIGHT) / 2, (height() - lineWidth) / 2 + lineWidth);
    painter.drawLine(p2Begin, p2End);
}

void CameraDataShow::drawROIRect(QPainter &painter)
{
    QPen pen;
    pen.setColor("#1097FF");
    pen.setWidth(1);
    painter.setPen(pen);
    painter.drawRoundedRect(QRect(m_ROIRect.x()+ m_relativePos.x(), m_ROIRect.y()+ m_relativePos.y(), m_ROIRect.width(), m_ROIRect.height()), 4, 4);
}

void CameraDataShow::setROIRectToCamera(bool markPos)
{
    auto originalROIRect = m_ROIRect;

    //计算帧数据与显示窗口的放缩比例；
    float scale = (float)m_frameWidth/ width();
    //纵向显示时用高度来计算scale；
    if(m_rotateAngle == 90 || m_rotateAngle == 270)
    {
        scale = (float)m_frameHeight/ height();
    }
    auto actualROIRect = QRect(originalROIRect.x()* scale,
                                 originalROIRect.y()* scale,
                                 originalROIRect.width()* scale,
                                 originalROIRect.height()* scale);

    if(markPos)
    {
        m_lastActualCenterPoint = actualROIRect.center();
    }

    //如果画面有旋转角度，则ROI区域需要根据旋转角度进行还原；
    if (0 != m_rotateAngle)
    {
        QRect actualRect = QRect(0, 0, m_frameWidth, m_frameHeight);
        QPoint relativePos;
        if(m_rotateAngle == 90)
        {
            relativePos = actualROIRect.topRight()- actualRect.topRight();
            actualROIRect = QRect(abs(relativePos.y()), abs(relativePos.x()), actualROIRect.width(), actualROIRect.height());
        }
        else if(m_rotateAngle == 180)
        {
            relativePos = actualROIRect.bottomRight()- actualRect.bottomRight();
            actualROIRect = QRect(abs(relativePos.x()), abs(relativePos.y()), actualROIRect.width(), actualROIRect.height());
        }
        else if(m_rotateAngle == 270)
        {
            QRect rect;
            rect.setLeft(m_frameHeight- actualROIRect.bottom());
            rect.setTop(actualROIRect.left());
            rect.setWidth(actualROIRect.height());
            rect.setHeight(actualROIRect.width());
            actualROIRect = rect;
        }
    }

    //根据相机当前放缩比例还原真实ROI区域；
    float frameScale = rvApp->cameraManager()->getFrameScale();
    int frameWidth = m_frameWidth* frameScale;
    int frameHeight = m_frameHeight* frameScale;
    QRect absoluteROIRect = QRect(actualROIRect.x()+ (frameWidth- m_frameWidth)/2,
                          actualROIRect.y()+ (frameHeight- m_frameHeight)/2,
                          actualROIRect.width(),
                          actualROIRect.height());

    emit sigSetROIRect(absoluteROIRect, frameWidth, frameHeight, m_rotateAngle);
}
