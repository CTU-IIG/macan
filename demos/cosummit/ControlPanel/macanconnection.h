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

#ifndef MACANCONNECTION_H
#define MACANCONNECTION_H

#include <QObject>
#include <iostream>
#include "macanworker.h"

extern const struct macan_key macan_ltk_node2;

/**
 * @brief The MaCANConnection class
 * The class implements an interface between MaCAN and other application components for
 * MaCAN demo purposes.
 * The class is ready to:
 *      - send data according the Buttons events received from the UI,
 *      - send received data to graph plotter,
 *      - toogle virtual indicators in the UI.
 *
 * The class is designed to support numbers of buttons, indicators and graphs.
 * The MaCAN main event loop is running in its own thread.
 *
 * This class should be instantioned only once in the application. @see sig_callback.
 */
class MaCANConnection : public QObject
{
    Q_OBJECT

private:
    /**
     * @brief True if MaCAN is connected to the CAN bus and running. False otherwise.
     */
    bool mIsRunning;
    /**
     * @brief Pointer to array of buttons states.
     */
    bool *mButtonOn;
    /**
     * @brief Pointer to array of indicators states.
     */
    bool *mIndicatorOn;
    /**
     * @brief Numeber of buttons.
     */
    unsigned int buttonsCnt;
    /**
     * @brief Number of indicators.
     */
    unsigned int indicatorsCnt;
    /**
     * @brief Instance of MaCAN Worker, which contains the MaCAN main event loop and calls other MaCAN functions.
     */
    MaCanWorker macanWorker;
    /**
     * @brief The callback, caller when new valid CAN message is received by MaCAN.
     * Both authenticated messages with valid key and non-autenticated messages are considered as valid.
     *
     * The callbacks have to be declared as static because of the MaCAN interface design. The callbacks are referencing
     * the class instance via static instance pointer, initialized in the constructor. That is why there can be only one
     * working instalce of this class.
     *
     * @param sig_num The number of received signal
     * @param sig_val The value of received signal
     * @param status The status of the signal
     */
    static void sig_callback(uint8_t sig_num, uint32_t sig_val, enum macan_signal_status status);
    /**
     * @brief The callback, caller when new invalid CAN message is received by MaCAN.
     * Only autentized CAN messages with wrong key are considered invalid.
     *
     * The callbacks have to be declared as static because of the MaCAN interface design. The callbacks are referencing
     * the class instance via static instance pointer, initialized in the constructor. That is why there can be only one
     * working instalce of this class.
     *
     * @param sig_num The number of received signal
     * @param sig_val The value of received signal
     * @param status The status of the signal
     */
    static void sig_invalid(uint8_t sig_num, uint32_t sig_val, enum macan_signal_status status);
    /**
     * @brief Internal function for transfering buttons states.
     * @param butStates Pointer to an array with buttons states
     * @param numButtons Number of the buttons
     * @param msgId The identifier if the signal.
     * @return true if success, false otherwise.
     */
    bool send_buttons_states(const bool *butStates, unsigned int numButtons, uint8_t msgId);

public:
    /**
     * @brief MaCANConnection constructor
     * @param numButtons The number of buttons in the UI
     * @param numIndicators The number of indicators in the UI
     * @param parent The pointer to the parent widget
     */
    explicit MaCANConnection(unsigned int numButtons, unsigned int numIndicators, QObject *parent = 0);
    ~MaCANConnection();
    /**
     * @brief Configures tha MaCAN, connect MaCAN to the CAN bus and starts the MaCAN main event loop thread.
     * @param canBus the name of the can device.
     * @return true if success false if error
     */
    bool connect(const char* canBus);
    /**
     * @brief Is MaCAN connected and running
     * @return true if MaCAN is configured, connected and running.
     */
    bool isRunning() const;
    /**
     * @brief Static pointer to the instance of this class, used in MaCAN callbacks to emit signals.
     * @see sig_callback
     * @see sig_invalid
     */
    static MaCANConnection *instance;

public slots:
    /**
     * @brief Set the button pressed and send the CAN message with the buttons states.
     * @param buttId identifier of the button.
     */
    void virtualButtonPressed(unsigned int buttId);
    /**
     * @brief Set the button released and send the CAN message with the buttons states.
     * @param buttId identifier of the button.
     */
    void virtualButtonReleased(unsigned int buttId);
    /**
     * @brief Toogle the button and send the CAN message with the buttons states.
     * @param buttId identifier of the button.
     */
    void virtualButtonClicked(unsigned int buttId);

signals:
    /**
     * @brief Received message for switching an indicator on.
     * @param ledId Identifier of the indicator.
     */
    void indicatorOn(unsigned int ledId);
    /**
     * @brief Received message for switching an indicator off.
     * @param ledId Identifier of the indicator.
     */
    void indicatorOff(unsigned int ledId);
    /**
     * @brief Received message for adding new value to a graph.
     * @param graphId Identifier of the graph.
     * @param value Value to be addedd to the graph.
     */
    void graphValueReceived(int graphId, int value);

};

#endif // MACANCONNECTION_H
