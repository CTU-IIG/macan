#ifndef VIRTUALLED_H
#define VIRTUALLED_H

#include <QObject>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <iostream>

#define LED_HW_CNT  2
class VirtualLED : public QObject
{
    Q_OBJECT
private:
    QGraphicsView *indicatorView;
    QGraphicsScene scene;
    QGraphicsEllipseItem ledOn;
    QGraphicsEllipseItem ledOff;
    unsigned int ledId;

public:
    explicit VirtualLED(QObject *parent = 0);
    ~VirtualLED();
    void setIndicatorView(QGraphicsView *view);
    QGraphicsView* getIndicatorView() const;
    void setLEDId(unsigned int id);
    unsigned int getLEDId() const;

public slots:
    void setLEDOn(unsigned int id);
    void setLEDOff(unsigned int id);

signals:
};

#endif // VIRTUALLED_H
