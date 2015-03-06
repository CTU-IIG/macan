/*
 *  Copyright 2014, 2015 Czech Technical University in Prague
 *
 *  Authors: Michal Horn <hornmich@fel.cvut.cz>
 *
 *  This file is part of MaCAN.
 *
 *  MaCAN is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  MaCAN is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with MaCAN.   If not, see <http://www.gnu.org/licenses/>.
 */
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

void VirtualLED::setIndicatorId(unsigned int id)
{
    IndicatorId = id;
}

unsigned int VirtualLED::getIndicatorId() const
{
    return IndicatorId;
}

void VirtualLED::setIndicatorOn(unsigned int id)
{
    if (id == IndicatorId) {
       scene.removeItem(&ledOff);
       scene.addItem(&ledOn);
       indicatorView->setScene(&scene);
       indicatorView->repaint();
    }
}

void VirtualLED::setIndicatorOff(unsigned int id)
{
    if (id == IndicatorId) {
        scene.removeItem(&ledOn);
        scene.addItem(&ledOff);
        indicatorView->setScene(&scene);
        indicatorView->repaint();
    }
}
