#ifndef MACANCONNECTION_H
#define MACANCONNECTION_H

#include <QObject>
#include <iostream>
#include "macanworker.h"

#define HW_BUTTONS_CNT 2
#define SW_INDICATORS_CNT 2

extern const struct macan_key macan_ltk_node2;

class MaCANConnection : public QObject
{
    Q_OBJECT

private:
    bool mIsConnected;
    bool mButtonPressed[HW_BUTTONS_CNT];
    bool mIndicatorOn[SW_INDICATORS_CNT];
    MaCanWorker macanWorker;
    static void sig_callback(uint8_t sig_num, uint32_t sig_val, enum macan_signal_status status);
    static void sig_invalid(uint8_t sig_num, uint32_t sig_val, enum macan_signal_status status);


public:
    explicit MaCANConnection(QObject *parent = 0);
    ~MaCANConnection();
    bool connect(const char* canBus);
    bool isConnected() const;
    static MaCANConnection *instance;


public slots:
    void virtualButtonPressed(unsigned int buttId);
    void virtualButtonReleased(unsigned int buttId);

signals:
    void setLEDOn(unsigned int ledId);
    void setLEDOff(unsigned int ledId);
    void graphValueReceived(int graphId, int value);

};

#endif // MACANCONNECTION_H
