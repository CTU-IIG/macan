#include "virtualleds.h"

VirtualLED::VirtualLED(QObject *parent) : QObject(parent)
{
    QBrush brush;
    brush.setColor(Qt::yellow);
    brush.setStyle(Qt::SolidPattern);
    ledOn.setRect(0, 0, 50, 50);
    ledOn.setBrush(brush);

    brush.setColor(Qt::black);
    brush.setStyle(Qt::SolidPattern);
    ledOff.setRect(0, 0, 50, 50);
    ledOff.setBrush(brush);

    scene.addItem(&ledOff);
}

VirtualLED::~VirtualLED()
{

}

void VirtualLED::setIndicatorView(QGraphicsView *view)
{
    indicatorView = view ;
    indicatorView->setScene(&scene);
}

QGraphicsView *VirtualLED::getIndicatorView() const
{
    return indicatorView;
}

void VirtualLED::setLEDId(unsigned int id)
{
    ledId = id;
}

unsigned int VirtualLED::getLEDId() const
{
    return ledId;
}

void VirtualLED::setLEDOn(unsigned int id)
{
    if (id == ledId) {
       scene.removeItem(&ledOff);
       scene.addItem(&ledOn);
       indicatorView->setScene(&scene);
       indicatorView->repaint();
    }
}

void VirtualLED::setLEDOff(unsigned int id)
{
    if (id == ledId) {
        scene.removeItem(&ledOn);
        scene.addItem(&ledOff);
        indicatorView->setScene(&scene);
        indicatorView->repaint();
    }
}
