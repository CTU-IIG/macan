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
#include "macanconnection.h"

MaCANConnection *MaCANConnection::instance = NULL;

void MaCANConnection::sig_callback(uint8_t sig_num, uint32_t sig_val, enum macan_signal_status status)
{
    (void) status;
    std::cout << "Received message[" << (int)sig_num << "] [" << status << "]: " << std::hex << sig_val << ": ";
    switch (sig_num) {
    case SIGNAL_SIN1 :
        std::cout << "SIN1 signal." << std::endl;
        emit instance->graphValueReceived(0, sig_val);
        break;
    case SIGNAL_SIN2 :
        std::cout << "SIN2 signal." << std::endl;
        emit instance->graphValueReceived(1, sig_val);
        break;
    default:
        std::cout << "Unknown message id: " << std::hex << sig_num << std::endl;
        break;
    }

}

void MaCANConnection::sig_invalid(uint8_t sig_num, uint32_t sig_val, enum macan_signal_status status)
{
    (void) status;
    std::cout << "Received invalid message[" << (int)sig_num << "]: " << std::hex << sig_val << "." << std::endl;
}

bool MaCANConnection::send_buttons_states(const bool *butStates, unsigned int numButtons, uint8_t msgId){
    uint32_t data = 0;
    for (unsigned int i = 0; i < numButtons; i++) {
        data |= butStates[i] << i;
    }
    std::cout << "Sending message [" << (int)msgId << "]:" << std::hex << data << std::endl;
    if (!macanWorker.sendSignal(msgId, data)) {
        std::cerr << "[ERR] Sending signal via MaCAN failed." << std::endl;
        return false;
    }
    return true;
}


MaCANConnection::MaCANConnection(unsigned int numButtons, unsigned int numIndicators, QObject *parent) : QObject(parent)
{
    if (instance != NULL) {
        std::cerr << "[WARN]:MaCANConnection: MaCANConnection instantioned more than once. This may cause unexpected bahavioral." << std::endl;
    }
    else {
        instance = this;
    }
    buttonsCnt = numButtons;
    indicatorsCnt = numIndicators;
    if (numButtons)
    mButtonOn = new bool[numButtons+1]; /* Adding one to avoid 0. */
    mIndicatorOn = new bool[numIndicators+1]; /* Adding one to avoid 0 */
    mIsRunning = false;
    for (unsigned int i = 0; i < numButtons; i++) {
        mButtonOn[i] = false;
    }
    for (unsigned int i = 0; i < numIndicators; i++) {
        mIndicatorOn[i] = false;
    }
}

MaCANConnection::~MaCANConnection()
{
    delete [] mButtonOn;
    delete [] mIndicatorOn;
    if (mIsRunning) {
        // TODO: Fix thread termination
    }
}

bool MaCANConnection::connect(const char* canBus)
{
    if (isRunning()) {
        std::cerr << "[ERR] CAN is already connected./n" << std::endl;
        return false;
    }

    if (!macanWorker.configure(canBus, &macan_ltk_node2, sig_callback, sig_invalid)) {
        std::cerr << "[ERR] CAN connection failed./n" << std::endl;
        return false;
    }

    macanWorker.start();
    mIsRunning = true;
    return true;
}

bool MaCANConnection::isRunning() const
{
    return mIsRunning;
}

void MaCANConnection::virtualButtonReleased(unsigned int buttId)
{
    if (!mIsRunning) {
        std::cerr << "[ERR] CAN is not connected." << std::endl;
        return;
    }
    mButtonOn[buttId]=0;
    send_buttons_states(mButtonOn, buttonsCnt, SIGNAL_LED);

}

void MaCANConnection::virtualButtonClicked(unsigned int buttId)
{
    if (!mIsRunning) {
        std::cerr << "[ERR] CAN is not connected." << std::endl;
        return;
    }
    mButtonOn[buttId]=!mButtonOn[buttId];
    send_buttons_states(mButtonOn, buttonsCnt, SIGNAL_LED);
}

void MaCANConnection::virtualButtonPressed(unsigned int buttId)
{
    if (!mIsRunning) {
        std::cerr << "[ERR] CAN is not connected./n" << std::endl;
        return;
    }
    mButtonOn[buttId]=0;
    send_buttons_states(mButtonOn, buttonsCnt, SIGNAL_LED);
}

