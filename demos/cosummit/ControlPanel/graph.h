#ifndef GRAPH_H
#define GRAPH_H

#include <QObject>
#include <qcustomplot.h>

class Graph : public QObject
{
    Q_OBJECT
private:
    int graphId;
    QCustomPlot *graphView;
    double amplitude;
    double period;
    double lastPointKey;
public:
    explicit Graph(QObject *parent = 0);
    ~Graph();
    void setGraphView(QCustomPlot* plot);
    void setGraphId(int id);
    int getGraphId() const;
    QCustomPlot* getGraphView() const;
    double getAmplitude() const;
    double getPeriod() const;

signals:

public slots:
    void setAmplitude(double value);
    void setPeriod(double value);
    void update();
    void setGraphvalue(int id, int value);
};

#endif // GRAPH_H
