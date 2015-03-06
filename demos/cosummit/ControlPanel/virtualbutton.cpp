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
#include "virtualbutton.h"

VirtualButton::VirtualButton(QObject *parent) : QObject(parent)
{
    mButtonId = 0;
}

void VirtualButton::setButtonId(unsigned int id)
{
    mButtonId = id;
}

unsigned int VirtualButton::getButtonId() const
{
    return mButtonId;
}

VirtualButton::~VirtualButton()
{

}

void VirtualButton::pressed()
{
    emit buttonPressed(mButtonId);
}

void VirtualButton::released()
{
    emit buttonReleased(mButtonId);
}

void VirtualButton::clicked()
{
    emit buttonClicked(mButtonId);
}

