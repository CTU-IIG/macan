#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define __BYTE_ORDER __BIG_ENDIAN

#include <QMainWindow>
#include <QTimer>
#include <virtualbutton.h>
#include <virtualleds.h>
#include <macanconnection.h>
#include <graph.h>
#include <QShortcut>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QShortcut *but1Shortcut;
    QShortcut *but2Shortcut;
    VirtualButton button1;
    VirtualButton button2;
    VirtualLED led1;
    VirtualLED led2;
    MaCANConnection macan;
    Graph graphPlotter1;
    Graph graphPlotter2;

};

#endif // MAINWINDOW_H
