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

#include "mainwindow.h"
#include "ui_mainwindow.h"

const unsigned int MainWindow::BUTTONS_CNT;
const unsigned int MainWindow::INDICATORS_CNT;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    authMsgNum(0),
    forgedMsgNum(0),
    forgedHighlightTimer(),
    ledStatus(0)
{
    ui->setupUi(this);

    /* Prepare MaCAN */
    macan = new MaCANConnection(MainWindow::BUTTONS_CNT, MainWindow::INDICATORS_CNT);
    if (!macan->connect("can0")) {
        std::cerr << "[ERR] MaCAN connection failed." << std::endl;
        ui->statusBar->showMessage("Connection failed (can0)...");
    }
    ui->statusBar->showMessage("Connected (can0)");

    /* Connect Virtual buttons views with its class */
    connect(ui->btn_toogleLed1, SIGNAL(clicked()), this, SLOT(ledClicked1()));
    connect(ui->btn_toogleLed2, SIGNAL(clicked()), this, SLOT(ledClicked2()));

    /* Prepare graph plotters */
    graphPlotter1.setGraphView(ui->graph);
    graphPlotter1.setGraphId(0);
    graphPlotter2.setGraphView(ui->graph2);
    graphPlotter2.setGraphId(1);
    /* Make left and bottom axes transfer their ranges to right and top axes: */
    connect(ui->graph->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->graph->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->graph->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->graph->yAxis2, SLOT(setRange(QCPRange)));
    /* Conect the graph plotter with the data source */
    connect(macan, SIGNAL(graphValueReceived(int,int)), &graphPlotter1, SLOT(addGraphvalue(int,int)), Qt::QueuedConnection);
    connect(macan, SIGNAL(graphValueReceived(int,int)), this, SLOT(incAuthMsgNum(int)), Qt::QueuedConnection);
    connect(macan, SIGNAL(invalidValueReceived(int)), this, SLOT(incForgedMsgNum(int)), Qt::QueuedConnection);

    connect(macan, SIGNAL(graphValueReceived(int,int)), &graphPlotter2, SLOT(addGraphvalue(int,int)), Qt::QueuedConnection);
    connect(&forgedHighlightTimer, SIGNAL(timeout()), this, SLOT(forgedMsgNumDehighlight()));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete macan;
}

void MainWindow::incAuthMsgNum(int graph)
{
	if (graph == 0)
		ui->auth_num->setText(QString("%0").arg(++authMsgNum));
}

void MainWindow::incForgedMsgNum(int signal)
{
	if (signal == 1) {
		ui->forged_num->setText(QString("%0").arg(++forgedMsgNum));
		ui->forged->setStyleSheet("QLabel { color: white } QWidget { background-color: red }");
		forgedHighlightTimer.start(1000);
	}
}

void MainWindow::forgedMsgNumDehighlight()
{
  ui->forged->setStyleSheet("");
}

void MainWindow::ledClicked(int led)
{
  QPushButton *btn;

  switch (led) {
  case 0:
    btn = ui->btn_toogleLed1;
    break;
  default:
    btn = ui->btn_toogleLed2;
  }

  ledStatus ^= (1 << led);
  if (ledStatus & (1 << led))
    btn->setStyleSheet("background-color: rgb(0, 0, 255)");
  else
    btn->setStyleSheet("");

  macan->send_buttons_states(ledStatus);
}
