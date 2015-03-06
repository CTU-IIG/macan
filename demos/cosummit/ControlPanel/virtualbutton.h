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
#ifndef VIRTUALBUTTON_H
#define VIRTUALBUTTON_H

#include <QObject>
#include <iostream>

/**
 * @brief The VirtualButton class
 * This class implements an interface between QButton widget and MaCAN.
 * I just receives a signal from the QButton and translates it to new signals, carying
 * the identifier of the button for further identifications and actions.
 */
class VirtualButton : public QObject
{
    Q_OBJECT
private:
    /**
     * @brief The identifier of the button
     */
    unsigned int mButtonId;
public:
    explicit VirtualButton(QObject *parent = 0);
    /**
     * @brief Set the button identifier
     * @param id The new button identifier
     */
    void setButtonId(unsigned int id);
    /**
     * @brief Get the button identifier
     * @return The button identifier
     */
    unsigned int getButtonId() const;
    ~VirtualButton();

signals:
    /**
     * @brief Button has been pressed
     * @param The button identifier.
     */
    void buttonPressed(unsigned int buttonId);
    /**
     * @brief Button has been released
     * @param The button identifier.
     */
    void buttonReleased(unsigned int buttonId);
    /**
     * @brief Button has been clicked
     * @param The button identifier.
     */
    void buttonClicked(unsigned int buttonId);

public slots:
    /**
     * @brief Button has been pressed.
     */
    void pressed();
    /**
     * @brief Button has been released
     */
    void released();
    /**
     * @brief Button has been clicked
     */
    void clicked();

};

#endif // VIRTUALBUTTON_H
