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

#include "graph.h"

Graph::Graph(QObject *parent) : QObject(parent)
{
    graphView = NULL;
    graphId = 0;
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

void Graph::addGraphvalue(int id, int value)
{
    if (graphId != id) {
        return;
    }

    // calculate two new data points:
    double key = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0;
    graphView->graph(0)->addData(key, value & 0xff);
    // set data of dots:
    // remove data of lines that's outside visible range:
    graphView->graph(0)->removeDataBefore(key-8);
    // rescale value (vertical) axis to fit the current data:
    graphView->graph(0)->rescaleValueAxis();
    // make key axis range scroll with the data (at a constant range size of 8):
    graphView->xAxis->setRange(key+0.25, 8, Qt::AlignRight);
    graphView->replot();

}

