#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDateTime>
#include <QVector>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_date_scale_draw.h>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onTimerTimeout();
    void onNetworkReply(QNetworkReply *reply);
    void onPeriod1Hour();
    void onPeriod6Hours();
    void onPeriod24Hours();

private:
    void setupUI();
    void setupPlots();
    void fetchData();
    void fetchRawData(qint64 from, qint64 to);
    void fetchHourlyStats(qint64 from, qint64 to);
    void updateCurrentTemperature(double temp, qint64 timestamp);
    void updateRawPlot(const QVector<QPointF> &data);
    void updateStatsPlot(const QVector<QPointF> &avg,
                         const QVector<QPointF> &min,
                         const QVector<QPointF> &max);
    QString timestampToTime(qint64 ts);

    QTimer *timer;
    QNetworkAccessManager *networkManager;
    qint64 currentPeriodHours;

    QLabel *tempLabel;
    QLabel *timeLabel;
    QPushButton *btn1h;
    QPushButton *btn6h;
    QPushButton *btn24h;
    QwtPlot *rawPlot;
    QwtPlot *statsPlot;
    QwtPlotCurve *rawCurve;
    QwtPlotCurve *hourlyAvgCurve;
    QwtPlotCurve *hourlyMinCurve;
    QwtPlotCurve *hourlyMaxCurve;
};

#endif // MAINWINDOW_H
