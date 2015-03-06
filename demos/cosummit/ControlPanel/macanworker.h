#ifndef MACANWORKER_H
#define MACANWORKER_H

#include <QThread>
#include <QObject>
#include "macan.h"
#include "helper.h"
#include "macan_config.h"
#include "macan_private.h"
#include <iostream>

class MaCanWorker : public QThread
{
private:
    bool isConfigured;
    int socket;
    macan_ev_loop *loop;
    struct macan_node_config node;
    struct macan_ctx macan_ctx;
    macan_sig_cback sig_callback;
    macan_sig_cback sig_invalid;


public:
    MaCanWorker();
    ~MaCanWorker();
    bool configure(const char* port, const struct macan_key *ltk_node, macan_sig_cback cb, macan_sig_cback inv);
    bool sendSignal(uint8_t sig_num, uint32_t signal);

    // QThread interface
protected:
    void run();
};

#endif // MACANWORKER_H
