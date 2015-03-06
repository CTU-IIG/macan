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

