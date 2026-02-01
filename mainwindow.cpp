#include "mainwindow.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <qwt_plot_grid.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>
#include <qwt_legend.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      currentPeriodHours(1)
{
    setupUI();
    setupPlots();
    
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);
    
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);
    timer->start(5000);
    
    fetchData();
    
    statusBar()->showMessage("Подключение к серверу: ожидание данных...");
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI()
{
    setWindowTitle("Мониторинг температуры");
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);
    
    // Текущая температура
    QWidget *tempWidget = new QWidget();
    QVBoxLayout *tempLayout = new QVBoxLayout(tempWidget);
    
    QLabel *tempTitle = new QLabel("Текущая температура:");
    tempTitle->setStyleSheet("font-size: 18px; font-weight: bold;");
    
    tempLabel = new QLabel("-- °C");
    tempLabel->setAlignment(Qt::AlignCenter);
    tempLabel->setStyleSheet("font-size: 48px; font-weight: bold; color: #e74c3c; margin: 10px 0;");
    
    timeLabel = new QLabel("Последнее обновление: --");
    timeLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setStyleSheet("font-size: 14px; color: #666;");
    
    tempLayout->addWidget(tempTitle);
    tempLayout->addWidget(tempLabel);
    tempLayout->addWidget(timeLabel);
    
    // Кнопки периода
    QWidget *periodWidget = new QWidget();
    QHBoxLayout *periodLayout = new QHBoxLayout(periodWidget);
    
    QLabel *periodLabel = new QLabel("Период:");
    periodLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    
    btn1h = new QPushButton("1 час");
    btn6h = new QPushButton("6 часов");
    btn24h = new QPushButton("24 часа");
    
    btn1h->setCheckable(true);
    btn6h->setCheckable(true);
    btn24h->setCheckable(true);
    
    btn1h->setChecked(true);
    
    connect(btn1h, &QPushButton::clicked, this, &MainWindow::onPeriod1Hour);
    connect(btn6h, &QPushButton::clicked, this, &MainWindow::onPeriod6Hours);
    connect(btn24h, &QPushButton::clicked, this, &MainWindow::onPeriod24Hours);
    
    periodLayout->addWidget(periodLabel);
    periodLayout->addStretch();
    periodLayout->addWidget(btn1h);
    periodLayout->addWidget(btn6h);
    periodLayout->addWidget(btn24h);
    
    // Графики
    QWidget *plotsWidget = new QWidget();
    QVBoxLayout *plotsLayout = new QVBoxLayout(plotsWidget);
    
    QLabel *rawLabel = new QLabel("Сырые данные:");
    rawLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin-top: 10px;");
    
    rawPlot = new QwtPlot();
    rawPlot->setMinimumHeight(300);
    
    QLabel *statsLabel = new QLabel("Часовые средние:");
    statsLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin-top: 10px;");
    
    statsPlot = new QwtPlot();
    statsPlot->setMinimumHeight(300);
    
    plotsLayout->addWidget(rawLabel);
    plotsLayout->addWidget(rawPlot);
    plotsLayout->addWidget(statsLabel);
    plotsLayout->addWidget(statsPlot);
    
    // Сборка основного интерфейса
    mainLayout->addWidget(tempWidget);
    mainLayout->addWidget(periodWidget);
    mainLayout->addWidget(plotsWidget);
}

void MainWindow::setupPlots()
{
    // Сырые данные
    rawPlot->setTitle("Температура");
    rawPlot->setAxisTitle(QwtPlot::xBottom, "Время");
    rawPlot->setAxisTitle(QwtPlot::yLeft, "Температура, °C");
    rawPlot->setAxisScale(QwtPlot::yLeft, 15, 30);
    qint64 now = QDateTime::currentSecsSinceEpoch();
    rawPlot->setAxisScale(QwtPlot::xBottom, now - 600, now); 
    rawPlot->setAxisScaleDraw(QwtPlot::xBottom, new QwtDateScaleDraw());
    rawPlot->setAxisScale(QwtPlot::xBottom, 
                          QDateTime::currentSecsSinceEpoch() - 3600, 
                          QDateTime::currentSecsSinceEpoch());
    
    QwtPlotGrid *grid1 = new QwtPlotGrid();
    grid1->attach(rawPlot);
    grid1->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
    
    rawCurve = new QwtPlotCurve("Температура");
    rawCurve->setPen(QPen(Qt::red, 2));
    rawCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    rawCurve->setStyle(QwtPlotCurve::Lines);
    rawCurve->attach(rawPlot);
    
    QwtLegend *legend1 = new QwtLegend();
    rawPlot->insertLegend(legend1, QwtPlot::BottomLegend);
    
    // Статистика
    statsPlot->setTitle("Часовые средние");
    statsPlot->setAxisTitle(QwtPlot::xBottom, "Время");
    statsPlot->setAxisTitle(QwtPlot::yLeft, "Температура, °C");
    statsPlot->setAxisScale(QwtPlot::yLeft, 15, 30); // Фиксированный диапазон
    qint64 now2 = QDateTime::currentSecsSinceEpoch();
    statsPlot->setAxisScale(QwtPlot::xBottom, now2 - 3600, now2);     
    statsPlot->setAxisScaleDraw(QwtPlot::xBottom, new QwtDateScaleDraw());
    statsPlot->setAxisScale(QwtPlot::xBottom, 
                            QDateTime::currentSecsSinceEpoch() - 3600, 
                            QDateTime::currentSecsSinceEpoch());

    QwtPlotGrid *grid2 = new QwtPlotGrid();
    grid2->attach(statsPlot);
    grid2->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
    
    hourlyAvgCurve = new QwtPlotCurve("Средняя");
    hourlyAvgCurve->setPen(QPen(Qt::blue, 2));
    hourlyAvgCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    hourlyAvgCurve->setStyle(QwtPlotCurve::Lines);
    hourlyAvgCurve->attach(statsPlot);
    
    hourlyMinCurve = new QwtPlotCurve("Минимум");
    hourlyMinCurve->setPen(QPen(Qt::green, 1, Qt::DashLine));
    hourlyMinCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    hourlyMinCurve->setStyle(QwtPlotCurve::Lines);
    hourlyMinCurve->attach(statsPlot);
    
    hourlyMaxCurve = new QwtPlotCurve("Максимум");
    hourlyMaxCurve->setPen(QPen(Qt::red, 1, Qt::DashLine));
    hourlyMaxCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    hourlyMaxCurve->setStyle(QwtPlotCurve::Lines);
    hourlyMaxCurve->attach(statsPlot);
    
    QwtLegend *legend2 = new QwtLegend();
    statsPlot->insertLegend(legend2, QwtPlot::BottomLegend);
    
    // Интерактивность
    new QwtPlotZoomer(rawPlot->canvas());
    new QwtPlotPanner(rawPlot->canvas());
    new QwtPlotMagnifier(rawPlot->canvas());
    
    new QwtPlotZoomer(statsPlot->canvas());
    new QwtPlotPanner(statsPlot->canvas());
    new QwtPlotMagnifier(statsPlot->canvas());
}

void MainWindow::fetchData()
{
    QNetworkRequest request(QUrl("http://localhost:8080/api/current"));
    networkManager->get(request);
    
    qint64 now = QDateTime::currentSecsSinceEpoch();
    qint64 from = now - (currentPeriodHours * 3600);
    
    fetchRawData(from, now);
    fetchHourlyStats(from, now);
}

void MainWindow::fetchRawData(qint64 from, qint64 to)
{
    QString url = QString("http://localhost:8080/api/raw?from=%1&to=%2").arg(from).arg(to);
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    networkManager->get(request);
}

void MainWindow::fetchHourlyStats(qint64 from, qint64 to)
{
    QString url = QString("http://localhost:8080/api/hourly?from=%1&to=%2").arg(from).arg(to);
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    networkManager->get(request);
}

void MainWindow::onTimerTimeout()
{
    fetchData();
}

void MainWindow::onNetworkReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return;
    }

    QByteArray response = reply->readAll();
    QString url = reply->url().toString();
    
    if (url.contains("/api/current")) {
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            double temp = obj.value("temperature").toDouble();
            qint64 ts = obj.value("timestamp").toVariant().toLongLong();
            updateCurrentTemperature(temp, ts);
            statusBar()->showMessage(QString("Температура: %1 °C").arg(temp, 0, 'f', 1));
        }
    }
    else if (url.contains("/api/raw")) {
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (!doc.isNull() && doc.isObject()) {
            QJsonArray dataArray = doc.object().value("data").toArray();
            QVector<QPointF> points;
            for (int i = 0; i < dataArray.size(); ++i) {
                QJsonObject item = dataArray.at(i).toObject();
                qint64 ts = item.value("timestamp").toVariant().toLongLong();
                double temp = item.value("temperature").toDouble();
                points.append(QPointF(ts, temp));
            }
            updateRawPlot(points);
        }
    }
    else if (url.contains("/api/hourly")) {
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (!doc.isNull() && doc.isObject()) {
            QJsonArray dataArray = doc.object().value("data").toArray();
            QVector<QPointF> avgPoints, minPoints, maxPoints;
            for (int i = 0; i < dataArray.size(); ++i) {
                QJsonObject item = dataArray.at(i).toObject();
                qint64 ts = item.value("timestamp").toVariant().toLongLong();
                double avg = item.value("avg").toDouble();
                double min = item.value("min").toDouble();
                double max = item.value("max").toDouble();
                
                avgPoints.append(QPointF(ts, avg));
                minPoints.append(QPointF(ts, min));
                maxPoints.append(QPointF(ts, max));
            }
            updateStatsPlot(avgPoints, minPoints, maxPoints);
        }
    }
    
    reply->deleteLater();
}

void MainWindow::updateCurrentTemperature(double temp, qint64 timestamp)
{
    tempLabel->setText(QString("%1 °C").arg(temp, 0, 'f', 1));
    timeLabel->setText(QString("Последнее обновление: %1").arg(timestampToTime(timestamp)));
}

void MainWindow::updateRawPlot(const QVector<QPointF> &data)
{
    if (data.isEmpty()) {
        rawCurve->setSamples(QVector<QPointF>());
        rawPlot->replot();
        return;
    }
    
// Убираем умножение на 1000 (время уже в секундах)
    QVector<QPointF> fixedData;
    for (const auto &point : data) {
        fixedData.append(QPointF(point.x(), point.y()));
    }
    rawCurve->setSamples(fixedData);
    rawPlot->replot();
}

void MainWindow::updateStatsPlot(const QVector<QPointF> &avg,
                                 const QVector<QPointF> &min,
                                 const QVector<QPointF> &max)
{
    if (avg.isEmpty()) {
        hourlyAvgCurve->setSamples(QVector<QPointF>());
        hourlyMinCurve->setSamples(QVector<QPointF>());
        hourlyMaxCurve->setSamples(QVector<QPointF>());
        statsPlot->replot();
        return;
    }
    
// Убираем умножение на 1000 (время уже в секундах)
    QVector<QPointF> fixedAvg;
    for (const auto &point : avg) {
        fixedAvg.append(QPointF(point.x(), point.y()));
    }
    hourlyAvgCurve->setSamples(fixedAvg);
    QVector<QPointF> fixedMin;
    for (const auto &point : min) {
        fixedMin.append(QPointF(point.x(), point.y()));
    }
    hourlyMinCurve->setSamples(fixedMin);

    QVector<QPointF> fixedMax;
    for (const auto &point : max) {
        fixedMax.append(QPointF(point.x(), point.y()));
    }
    hourlyMaxCurve->setSamples(fixedMax);
    statsPlot->replot();
}

void MainWindow::onPeriod1Hour()
{
    currentPeriodHours = 1;
    btn1h->setChecked(true);
    btn6h->setChecked(false);
    btn24h->setChecked(false);
    fetchData();
}

void MainWindow::onPeriod6Hours()
{
    currentPeriodHours = 6;
    btn1h->setChecked(false);
    btn6h->setChecked(true);
    btn24h->setChecked(false);
    fetchData();
}

void MainWindow::onPeriod24Hours()
{
    currentPeriodHours = 24;
    btn1h->setChecked(false);
    btn6h->setChecked(false);
    btn24h->setChecked(true);
    fetchData();
}

QString MainWindow::timestampToTime(qint64 ts)
{
    QDateTime dt = QDateTime::fromSecsSinceEpoch(ts);
    return dt.toString("dd.MM.yyyy HH:mm:ss");
}
