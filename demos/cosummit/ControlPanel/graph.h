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

#ifndef GRAPH_H
#define GRAPH_H

#include <QObject>
#include <qcustomplot.h>

/**
 * @brief The Graph class
 *
 * The class implements an interface between the application components and QCustomPlot
 * widget for the porposes of MaCAN demo.
 */
class Graph : public QObject
{
    Q_OBJECT
private:
    /**
     * @brief Identifier of the graph.
     * This identifier is used in slots to identify the right signal.
     */
    int graphId;
    /**
     * @brief A pointer to the UI widget for plotting graphs.
     */
    QCustomPlot *graphView;
public:
    explicit Graph(QObject *parent = 0);
    ~Graph();
    /**
     * @brief Assign the UI graph plotting widget.
     * @param plot A pointer to QCustomPlot widget instance.
     */
    void setGraphView(QCustomPlot* plot);
    /**
     * @brief Set the graph identifier. @see graphId.
     * @param id the Graph identifier.
     */
    void setGraphId(int id);
    /**
     * @brief Get the graph identifier. @see graphId.
     * @return The Graph identifier.
     */
    int getGraphId() const;
    /**
     * @brief Get a pointer to the UI graph plotting widget.
     * @return A pointer to QCustomPlot widget instance.
     */
    QCustomPlot* getGraphView() const;

signals:

public slots:
    /**
     * @brief Add point to the graph. The Value is added only if the graph identifier matches the id value.
     * @param id An identifier of the Graph, for which this signal is intended.
     * @param value A value to be added to the graph.
     */
    void addGraphvalue(int id, int value);
};

#endif // GRAPH_H
