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
#ifndef VIRTUALLED_H
#define VIRTUALLED_H

#include <QObject>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <iostream>

/**
 * @brief The VirtualLED class
 * This class implements a Widget visualising some circular indicator, for example a
 * virtual LED.
 * The indicator has two states:
 *      - ON, when it has yellow collor,
 *      - OFF, when it has black collor.
 * The Indicator is implemented as an GraphicsEllipse drawn into a GraphicsScene, viewed by GraphicsView.
 */
class VirtualLED : public QObject
{
    Q_OBJECT
private:
    /**
     * @brief A pointer to a QGraphicsView from the UI, which will show the indicator.
     */
    QGraphicsView *indicatorView;
    /**
     * @brief A scene, into which the ellipse is drawn.
     */
    QGraphicsScene scene;
    /**
     * @brief An Ellipse for ON state
     */
    QGraphicsEllipseItem ledOn;
    /**
     * @brief An Ellipse for OFF state
     */
    QGraphicsEllipseItem ledOff;
    /**
     * @brief An identifier of the indicator
     */
    unsigned int IndicatorId;

public:
    explicit VirtualLED(QObject *parent = 0);
    ~VirtualLED();
    /**
     * @brief Assign QGraphicsView object to the indicator.
     * The assigned object will be used to view the indicator scene.
     * @param view A pointer to the QGraphicsView object.
     */
    void setIndicatorView(QGraphicsView *view);
    /**
     * @brief Get pointer to a QGraphicsView object used to draw the indicator.
     * @return A pointer to a QGraphicsView object.
     */
    QGraphicsView* getIndicatorView() const;
    /**
     * @brief Set the identifier of the indicator.
     * @param The identifier of the indicator.
     */
    void setIndicatorId(unsigned int id);
    /**
     * @brief Get the identifier of the indicator.
     * @return The identifier of the indicator.
     */
    unsigned int getIndicatorId() const;

public slots:
    /**
     * @brief Set the indicator to the ON state. The Indicator will be set only if the identifier matches with the id passed as a parameter.
     * @param The identifier of the indicator.
     */
    void setIndicatorOn(unsigned int id);
    /**
     * @brief Set the indicator to the OFF state. The Indicator will be set only if the identifier matches with the id passed as a parameter.
     * @param The identifier of the indicator.
     */
    void setIndicatorOff(unsigned int id);

signals:
};

#endif // VIRTUALLED_H
