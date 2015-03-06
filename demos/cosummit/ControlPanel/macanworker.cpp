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

    if (macan_init(&macan_ctx, &config, &node, loop, socket) != 0) {
        std::cerr << "[ERR]:MaCanWorker::configure macan_init returned with an error." << std::endl;
        return false;
    }
    if (macan_reg_callback(&macan_ctx, SIGNAL_SIN1, sig_callback, sig_invalid) != 0) {
        std::cerr << "[ERR]:MaCanWorker::configure macan_reg_callback for SIGNAL_SIN1 returned with an error." << std::endl;
        return false;
    }
    if (macan_reg_callback(&macan_ctx, SIGNAL_SIN2, sig_callback, sig_invalid) != 0) {
        std::cerr << "[ERR]:MaCanWorker::configure macan_reg_callback or SIGNAL_SIN2 returned with an error." << std::endl;
        return false;
    }

    isConfigured = true;
    return true;
}

bool MaCanWorker::sendSignal(uint8_t sig_num, uint32_t signal) {
    macan_send_sig(&macan_ctx, sig_num, signal);
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
