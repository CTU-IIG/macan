#include "macanconnection.h"

MaCANConnection *MaCANConnection::instance;

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


MaCANConnection::MaCANConnection(QObject *parent) : QObject(parent)
{
    instance = this;
    mIsConnected = false;
}

MaCANConnection::~MaCANConnection()
{

}

bool MaCANConnection::connect(const char* canBus)
{
    if (isConnected()) {
        std::cerr << "[ERR] CAN is already connected./n" << std::endl;
        return false;
    }

    if (!macanWorker.configure(canBus, &macan_ltk_node2, sig_callback, sig_invalid)) {
        std::cerr << "[ERR] CAN connection failed./n" << std::endl;
        return false;
    }

    macanWorker.start();
    mIsConnected = true;
    return true;
}

bool MaCANConnection::isConnected() const
{
    return mIsConnected;
}

void MaCANConnection::virtualButtonReleased(unsigned int buttId)
{
    if (!mIsConnected) {
        std::cerr << "[ERR] CAN is not connected." << std::endl;
        return;
    }
    uint8_t msgId = SIGNAL_LED;
    uint32_t data = 0<<buttId;
    std::cout << "Sending message [" << (int)msgId << "]:" << std::hex << data << std::endl;
    if (!macanWorker.sendSignal(msgId, data)) {
        std::cerr << "[ERR] Sending signal via MaCAN failed." << std::endl;
    }
}

void MaCANConnection::virtualButtonPressed(unsigned int buttId)
{
    if (!mIsConnected) {
        std::cerr << "[ERR] CAN is not connected./n" << std::endl;
        return;
    }


    uint8_t msgId = SIGNAL_LED;
    uint32_t data = 1<<buttId;
    std::cout << "Sending message [" << (int)msgId << "]:" << std::hex << data << std::endl;
    if (!macanWorker.sendSignal(msgId, data)) {
        std::cerr << "[ERR] Sending signal via MaCAN failed." << std::endl;
    }
}

