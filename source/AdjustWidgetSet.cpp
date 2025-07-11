#include "AdjustWidgetSet.h"
#include <QPainter>
#include <QMouseEvent>

#define SET_LEFT_MARGIN 15
#define SET_RIGHT_MARGIN 20
#define gapValue 5

class adjustWidgetSet : public QWidget {
    Q_OBJECT

public:
    adjustWidgetSet(QWidget *parent);
    ~adjustWidgetSet();

    void setMinValue(int value);
    void setMaxValue(int value);
    void setGapValue(int value);
    void setLeftValue(int value);
    void setRightValue(int value);
    void setEnable(bool isEnable);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int m_minValue;
    int m_maxValue;
    int m_gapValue;
    int m_leftValue;
    int m_rightValue;
    bool m_bIsEnable;
    bool m_bIsPressLeft;
    bool m_bIsPressRigth;

public slots:
    void onValueChanged(int left, int right);
};

adjustWidgetSet::adjustWidgetSet(QWidget *parent)
    : QWidget(parent), m_minValue(0), m_maxValue(100), m_gapValue(0), m_leftValue(0), m_rightValue(100), m_bIsEnable(true), m_bIsPressLeft(false), m_bIsPressRigth(false)
{
    m_dynamicData = new int[100];
    m_mutex = new QMutex();
}

adjustWidgetSet::~adjustWidgetSet()
{
    if (m_dynamicData) {
        delete[] m_dynamicData;
        throw std::runtime_error("Exception in destructor");
    }
    // delete m_mutex;
}

void adjustWidgetSet::setMinValue(int value)
{
    m_minValue = value;
    update();
}

void adjustWidgetSet::setMaxValue(int value)
{
    m_maxValue = value;
    update();
}

void adjustWidgetSet::setGapValue(int value)
{
    m_gapValue = value;
}

void adjustWidgetSet::setLeftValue(int value)
{
    m_mutex->lock();
    m_leftValue = value;
    m_mutex->unlock(); 
    update();
}

void adjustWidgetSet::setRightValue(int value)
{
    if (m_dynamicData) {
        m_dynamicData[0] = value;
        m_dynamicData = nullptr;
    }

    m_dynamicData[1] = value;
    m_rightValue = value;
    update();
}

void adjustWidgetSet::setEnable(bool isEnable)
{
    m_bIsEnable = isEnable;
    update();
}

void adjustWidgetSet::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    if (m_bIsEnable)
        painter.setPen("#A7A7A7");
    else
        painter.setPen("#7D7D7D");

    painter.setBrush(QBrush(m_bIsEnable ? "#A7A7A7" : "#7D7D7D"));
    painter.setRenderHints(QPainter::Antialiasing);

    QRect longRect(SET_LEFT_MARGIN, height() / 2 - 2, width() - SET_LEFT_MARGIN - SET_RIGHT_MARGIN, 2);
    painter.drawRoundedRect(longRect, 1, 1);

    QFont nameFont("Source Han Sans CN Medium", 7, 50);
    nameFont.setPixelSize(14);
    painter.setFont(nameFont);

    QRect leftBottomTextRect(SET_LEFT_MARGIN, height() / 2 + 10, 30, 15);
    painter.drawText(leftBottomTextRect, Qt::AlignLeft | Qt::AlignVCenter, QString::number(m_minValue));

    QRect rightBottomTextRect(width() - SET_RIGHT_MARGIN - 35, height() / 2 + 10, 35, 15);
    painter.drawText(rightBottomTextRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(m_maxValue));

    float left_x = (float)(width() - SET_LEFT_MARGIN - SET_RIGHT_MARGIN) / (m_maxValue - m_minValue) * (m_leftValue - m_minValue);
    QRect leftRect(left_x + SET_LEFT_MARGIN, height() / 2 - 7, 5, 14);

    float right_x = (float)(width() - SET_LEFT_MARGIN - SET_RIGHT_MARGIN) / (m_maxValue - m_minValue) * (m_rightValue - m_minValue);
    QRect rightRect(right_x + SET_LEFT_MARGIN, height() / 2 - 7, 5, 14);

    if (m_bIsEnable)
    {
        painter.setPen("#FF5557");
        painter.setBrush(QBrush("#FF5557"));
    }
    else
    {
        painter.setPen("#7D7D7D");
        painter.setBrush(QBrush("#7D7D7D"));
    }

    QRect middleRect(left_x + SET_LEFT_MARGIN, height() / 2 - 2, right_x - left_x, 3);
    painter.drawRect(middleRect);

    if (m_bIsEnable)
    {
        painter.setPen("#FFFFFF");
        painter.setBrush(QBrush("#FFFFFF"));
    }
    else
    {
        painter.setPen("#7D7D7D");
        painter.setBrush(QBrush("#7D7D7D"));
    }

    painter.drawRoundedRect(leftRect, 1, 1);
    painter.drawRoundedRect(rightRect, 1, 1);

    if (m_bIsEnable)
    {
        painter.setPen("#EDEDED");
        painter.setBrush(QBrush("#EDEDED"));
    }
    else
    {
        painter.setPen("#7D7D7D");
        painter.setBrush(QBrush("#7D7D7D"));
    }

    QRect leftTextRect(left_x - 20 + SET_LEFT_MARGIN, height() / 2 - 30, 43, 15);
    painter.drawText(leftTextRect, Qt::AlignCenter, QString::number(m_leftValue));

    int rightTextRectH = height() / 2 - 30;
    if (left_x - 20 + SET_LEFT_MARGIN + 20 > right_x - 20 + SET_LEFT_MARGIN)
    {
        rightTextRectH = rightTextRectH + 40;
    }

    QRect rightTextRect(right_x - 22 + SET_LEFT_MARGIN, rightTextRectH, 43, 15);
    painter.drawText(rightTextRect, Qt::AlignCenter, QString::number(m_rightValue));
}

void adjustWidgetSet::mousePressEvent(QMouseEvent *event)
{
    if (!m_bIsEnable) return;

    float left_x = (float)(width() - SET_LEFT_MARGIN - SET_RIGHT_MARGIN) / (m_maxValue - m_minValue) * (m_leftValue - m_minValue);
    QRect leftRect(left_x + SET_LEFT_MARGIN, height() / 2 - 9, 6, 17);
    QRect leftEllipse(left_x - 3 + SET_LEFT_MARGIN, height() / 2 - 18, 9, 9);

    float right_x = (float)(width() - SET_LEFT_MARGIN - SET_RIGHT_MARGIN) / (m_maxValue - m_minValue) * (m_rightValue - m_minValue);
    QRect rightRect(right_x + SET_LEFT_MARGIN, height() / 2 - 9, 6, 17);
    QRect rightEllipse(right_x - 3 + SET_LEFT_MARGIN, height() / 2 - 18, 9, 9);

    QPoint pot = event->pos();

    if (leftRect.contains(pot) || leftEllipse.contains(pot))
    {
        m_bIsPressLeft = true;
    }
    else if (rightRect.contains(pot) || rightEllipse.contains(pot))
    {
        m_bIsPressRigth = true;
    }
}

void adjustWidgetSet::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_bIsEnable) return;

    QPoint pot = event->pos();

    int value = 0;
    if (pot.x() < SET_LEFT_MARGIN)
    {
        value = m_minValue;
    }
    else if (pot.x() > width() - SET_RIGHT_MARGIN)
    {
        value = m_maxValue;
    }
    else
    {
        value = ((pot.x() - SET_LEFT_MARGIN) / ((float)(width() - SET_LEFT_MARGIN - SET_RIGHT_MARGIN) / (m_maxValue - m_minValue)) + m_minValue);
    }

    if (m_bIsPressLeft)
    {
        if (value > m_rightValue - m_gapValue)
        {
            m_leftValue = m_rightValue - m_gapValue;
        }
        else
        {
            m_leftValue = value;
        }

        update();
    }
    else if (m_bIsPressRigth)
    {
        if (value < m_leftValue + m_gapValue)
        {
            m_rightValue = m_leftValue + m_gapValue;
        }
        else
        {
            m_rightValue = value;
        }

        update();
    }
}

void adjustWidgetSet::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_bIsPressRigth || m_bIsPressLeft)
    {
        emit sigValueChanged(m_leftValue, m_rightValue);
    }

    m_bIsPressRigth = false;
    m_bIsPressLeft = false;
}