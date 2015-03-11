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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <virtualbutton.h>
#include <virtualleds.h>
#include <macanconnection.h>
#include <graph.h>
#include <QShortcut>

namespace Ui {
class MainWindow;
}

/**
 * @brief The MainWindow class
 * The class implements the only one window of the application. The window contains two graph plotters to
 * visualize autenticated and non-autenticated data received from TriBoard and two buttons to toogle LEDs
 * on the TriBoard.
 *
 * The class is ready to use an indicators, implementad by VirtualLED class.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Number of buttons in the UI
     */
    static const unsigned int BUTTONS_CNT = 2;
    /**
     * @brief Number of indicators in the UI.
     * In this UI there is no indocator, but the rest of components are ready to use them. To Add an indicator just
     * define the object of class VirtualLED in this class, set this constatnt to proper value and make connection between
     * desired signals and slots.
     */
    static const unsigned int INDICATORS_CNT = 0;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    /**
     * @brief Pointer to the User Interface object
     */
    Ui::MainWindow *ui;
    /**
     * @brief A shortcut for clicking on the Button 1 (ALT+1)
     */
    QShortcut *but1Shortcut;
    /**
     * @brief A shortcut for clicking on the button 2 (ALT+2)
     */
    QShortcut *but2Shortcut;
    /**
     * @brief An instance of an object creating interface between MaCAN and QButton widget for toogling LED on the TriBoard.
     */
    VirtualButton button1;
    /**
     * @brief An instance of an object creating interface between MaCAN and QButton widget for toogling LED on the TriBoard.
     */
    VirtualButton button2;
    /**
     * @brief A pointer to MaCAN object managing the CAN connection, state of the components and Event loop thread.
     */
    MaCANConnection *macan;
    /**
     * @brief A Plotter for graph 1, which shows autenticated data
     */
    Graph graphPlotter1;
    /**
     * @brief A Plotter for graph 2, which shows non-autenticated data
     */
    Graph graphPlotter2;

    long authMsgNum;
    long forgedMsgNum;
    QTimer forgedHighlightTimer;

private slots:
    void incAuthMsgNum(int graph);
    void incForgedMsgNum(int graph);
    void forgedMsgNumDehighlight();

};

#endif // MAINWINDOW_H
