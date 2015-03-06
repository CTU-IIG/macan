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
#ifndef MACANWORKER_H
#define MACANWORKER_H

#include <QThread>
#include "macan.h"
#include "helper.h"
#include "macan_config.h"
#include "macan_private.h"
#include <iostream>

/**
 * @brief The MaCanWorker class
 * This class implements the MaCAN thread, running the MaCAN event loop in paralel with other application components,
 * especially the UI.
 */
class MaCanWorker : public QThread
{
private:
    /**
     * @brief Flag whether the configure method has been called befor the thread start.
     */
    bool isConfigured;
    /**
     * @brief Socket used for CAN.
     */
    int socket;
    /**
     * @brief MaCAN event loop
     */
    macan_ev_loop *loop;
    /**
     * @brief MaCAN node configuration.
     */
    struct macan_node_config node;
    /**
     * @brief MaCAN context
     */
    struct macan_ctx macan_ctx;
    /**
     * @brief Pointers to a MaCAN callback function for valid messages.
     */
    macan_sig_cback sig_callback;
    /**
     * @brief Pointers to a MaCAN callback function for invalid messages.
     */
    macan_sig_cback sig_invalid;

public:
    MaCanWorker();
    ~MaCanWorker();
    /**
     * @brief Configures the MaCAN and starts the event loop thread.
     * @param port The name of the CAN port
     * @param ltk_node Pointer to the node long term key
     * @param cb Pointer to the MaCAN callback for the valid message reception.
     * @param inv Pointer to the MaCAN callback for the invalid message reception.
     * @return True if success, false otherwise.
     */
    bool configure(const char* port, const struct macan_key *ltk_node, macan_sig_cback cb, macan_sig_cback inv);
    /**
     * @brief Send a signal via MaCAN interface.
     * @param sig_num Signal identifier.
     * @param signal Signal data.
     * @return True if success, false otherwise.
     */
    bool sendSignal(uint8_t sig_num, uint32_t signal);

    // QThread interface
protected:
    /**
     * @brief Runs the MaCAN event loop in the thread.
     */
    void run();
};

#endif // MACANWORKER_H
