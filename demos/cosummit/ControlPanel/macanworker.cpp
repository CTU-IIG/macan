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
#include "macanworker.h"

MaCanWorker::MaCanWorker()
{
    isConfigured = false;
}

MaCanWorker::~MaCanWorker()
{

}

bool MaCanWorker::configure(const char* port, const macan_key *ltk_node, macan_sig_cback cb, macan_sig_cback inv) {
    if (isConfigured) {
        std::cerr << "[ERR]:MaCanWorker::configure MaCAN already configured." << std::endl;
        return false;
    }
    sig_callback = cb;
    sig_invalid = inv;

    loop = MACAN_EV_DEFAULT;
    socket = helper_init(port);
    node.node_id = NODE_PC;
    node.ltk = ltk_node;

    macan_ctx = macan_alloc_mem(&config, &node);

    if (macan_init(macan_ctx, loop, socket) != 0) {
        std::cerr << "[ERR]:MaCanWorker::configure macan_init returned with an error." << std::endl;
        return false;
    }
    if (macan_reg_callback(macan_ctx, SIGNAL_SIN1, sig_callback, sig_invalid) != 0) {
        std::cerr << "[ERR]:MaCanWorker::configure macan_reg_callback for SIGNAL_SIN1 returned with an error." << std::endl;
        return false;
    }
    if (macan_reg_callback(macan_ctx, SIGNAL_SIN2, sig_callback, sig_invalid) != 0) {
        std::cerr << "[ERR]:MaCanWorker::configure macan_reg_callback or SIGNAL_SIN2 returned with an error." << std::endl;
        return false;
    }

    isConfigured = true;
    return true;
}

bool MaCanWorker::sendSignal(uint8_t sig_num, uint32_t signal) {
    if (!isConfigured) {
        std::cerr << "[ERR]:MaCanWorker::sendSignal MaCAN has to been configured." << std::endl;
        return false;
    }
    macan_send_sig(macan_ctx, sig_num, signal);
    return true;
}

void MaCanWorker::run()
{
    if (!isConfigured) {
        std::cerr << "[ERR]:MaCanWorker::run MaCAN has to be configured befor entering the main loop." << std::endl;
        return;
    }
    macan_ev_run(loop);
}
