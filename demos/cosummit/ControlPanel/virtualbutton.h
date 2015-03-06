#ifndef VIRTUALBUTTON_H
#define VIRTUALBUTTON_H

#include <QObject>
#include <iostream>

class VirtualButton : public QObject
{
    Q_OBJECT
private:
    unsigned int mButtonId;
public:
    explicit VirtualButton(QObject *parent = 0);
    void setButtonId(unsigned int id);
    unsigned int getButtonId() const;
    ~VirtualButton();

signals:
    void buttonPressed(unsigned int buttonId);
    void buttonReleased(unsigned int buttonId);
    void buttonClicked(unsigned int buttonId);

public slots:
    void pressed();
    void released();
    void clicked();

};

#endif // VIRTUALBUTTON_H
