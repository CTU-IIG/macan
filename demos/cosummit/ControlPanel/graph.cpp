#include "graph.h"

Graph::Graph(QObject *parent) : QObject(parent)
{
    amplitude = 0;
    period = 0;
    graphView = NULL;
    lastPointKey = 0;
}

Graph::~Graph()
{

}

void Graph::setGraphView(QCustomPlot *plot)
{
    graphView = plot;
    graphView->clearGraphs();
    graphView->addGraph();
    graphView->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    graphView->xAxis->setDateTimeFormat("hh:mm:ss");
    graphView->xAxis->setAutoTickStep(false);
    graphView->xAxis->setTickStep(2);
    graphView->axisRect()->setupFullAxesBox();
}

void Graph::setGraphId(int id)
{
    graphId = id;
}

int Graph::getGraphId() const
{
    return graphId;
}

QCustomPlot *Graph::getGraphView() const
{
    return graphView;
}

double Graph::getAmplitude() const
{
    return amplitude;
}

double Graph::getPeriod() const
{
    return period;
}

void Graph::setAmplitude(double value)
{
    amplitude = value;
}

void Graph::setPeriod(double value)
{
    period = value;
}

void Graph::update()
{
    // calculate two new data points:
    double key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
    double value0 = amplitude*qSin(period*key); //qSin(key*1.6+qCos(key*1.7)*2)*10 + qSin(key*1.2+0.56)*20 + 26;
    // add data to lines:
    graphView->graph(0)->addData(key, value0);
    // set data of dots:
    // remove data of lines that's outside visible range:
    graphView->graph(0)->removeDataBefore(key-8);
    // rescale value (vertical) axis to fit the current data:
    graphView->graph(0)->rescaleValueAxis();
    lastPointKey = key;
    // make key axis range scroll with the data (at a constant range size of 8):
    graphView->xAxis->setRange(key+0.25, 8, Qt::AlignRight);
    graphView->replot();
}

void Graph::setGraphvalue(int id, int value)
{
    if (graphId != id) {
        return;
    }

    // calculate two new data points:
    double key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
    graphView->graph(0)->addData(key, value);
    // set data of dots:
    // remove data of lines that's outside visible range:
    graphView->graph(0)->removeDataBefore(key-8);
    // rescale value (vertical) axis to fit the current data:
    graphView->graph(0)->rescaleValueAxis();
    lastPointKey = key;
    // make key axis range scroll with the data (at a constant range size of 8):
    graphView->xAxis->setRange(key+0.25, 8, Qt::AlignRight);
    graphView->replot();

}

