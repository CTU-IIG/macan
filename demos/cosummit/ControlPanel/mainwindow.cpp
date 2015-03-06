#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    button1.setButtonId(1);
    button2.setButtonId(0);
    /*
    led1.setIndicatorView(ui->gw_virtualLed1);
    led1.setLEDId(0);
    led2.setIndicatorView(ui->gw_virtualLed2);
    led2.setLEDId(1);
    */
    graphPlotter1.setGraphView(ui->graph);
    graphPlotter1.setAmplitude(1);
    graphPlotter1.setPeriod(1);
    graphPlotter1.setGraphId(0);


    graphPlotter2.setGraphView(ui->graph2);
    graphPlotter2.setAmplitude(1);
    graphPlotter2.setPeriod(1);
    graphPlotter2.setGraphId(1);


    /* Connect Virtual buttons views with its class */
    connect(ui->btn_toogleLed1, SIGNAL(pressed()), &button1, SLOT(pressed()));
    connect(ui->btn_toogleLed1, SIGNAL(released()), &button1, SLOT(released()));
    connect(ui->btn_toogleLed2, SIGNAL(pressed()), &button2, SLOT(pressed()));
    connect(ui->btn_toogleLed2, SIGNAL(released()), &button2, SLOT(released()));

    /* Connect Virtual buttons objects with macan connection object */
    connect(&button1, SIGNAL(buttonPressed(uint)), &macan, SLOT(virtualButtonPressed(uint)));
    connect(&button1, SIGNAL(buttonReleased(uint)), &macan, SLOT(virtualButtonReleased(uint)));
    connect(&button2, SIGNAL(buttonPressed(uint)), &macan, SLOT(virtualButtonPressed(uint)));
    connect(&button2, SIGNAL(buttonReleased(uint)), &macan, SLOT(virtualButtonReleased(uint)));

    /* Connect virtual LEDs objects with macan connection object */
    /*
    connect(&macan, SIGNAL(setLEDOn(uint)), &led1, SLOT(setLEDOn(uint)));
    connect(&macan, SIGNAL(setLEDOff(uint)), &led1, SLOT(setLEDOff(uint)));
    connect(&macan, SIGNAL(setLEDOn(uint)), &led2, SLOT(setLEDOn(uint)));
    connect(&macan, SIGNAL(setLEDOff(uint)), &led2, SLOT(setLEDOff(uint)));
    */

    if (!macan.connect("can0")) {
        std::cerr << "[ERR] MaCAN connection failed." << std::endl;
        ui->statusBar->showMessage("Connection failed (can0)...");
    }
    ui->statusBar->showMessage("Connected (can0)");

    // make left and bottom axes transfer their ranges to right and top axes:
    connect(ui->graph->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->graph->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->graph->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->graph->yAxis2, SLOT(setRange(QCPRange)));

    connect(&macan, SIGNAL(graphValueReceived(int,int)), &graphPlotter1, SLOT(setGraphvalue(int,int)), Qt::QueuedConnection);
    connect(&macan, SIGNAL(graphValueReceived(int,int)), &graphPlotter2, SLOT(setGraphvalue(int,int)), Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    delete ui;
}
