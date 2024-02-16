
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include <QMainWindow>
#include <QThread>
#include <QDebug>
#include <QTimer>
#include <QElapsedTimer>


#include <Windows.h>
#include <comutil.h>
#include <stdio.h>
#include <atlcomcli.h>
#include "stdafx.h"
#include "conio.h"
#include <signal.h>
#include <stdlib.h>

#include "qcustomplot.h"

#include "hal_thread.h"
#include "sv_publisher.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ServerThread : public QThread
{
public:
    explicit ServerThread(Ui::MainWindow *ui = nullptr, QObject *parent = nullptr);

private:
    void run() override;

    Ui::MainWindow *ui;
};

class SimulationThread : public QThread
{
    Q_OBJECT
public:
    explicit SimulationThread(QObject *parent = nullptr);

signals:
    void updateUI(QString msg);

private:
    void run() override;

    double ATPtime;
    double ATPtimeStart;
    double ATPtimeStop;
    double ATPtimeStep;

    double DataVar[4];
    double DataIn[8];
    double DataOut[8];

    bool flagOverrun = false;
    int contOverrun = 0;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setGrafics(QCustomPlot *customPlot);
    void updateGrafics(QCustomPlot *customPlot);

private slots:
    void on_actionConnect_triggered();
    void on_actionStart_triggered();

    void on_actionShow_Graphics_triggered();
    void contextMenuRequest(QPoint pos);
    void unZoom();
    void showVoltage();
    void showCurrent();

    void on_actionChange_dstMAC_triggered();
    void on_actionSettings_triggered();
    void configok();
    void configcancel();

    void onUpdateUI(QString msg);

private:
    Ui::MainWindow *ui;

    QDialog *janela;
    QDoubleSpinBox *doubleSpinBox;
    QDoubleSpinBox *doubleSpinBox_2;

    QString file;
    QString caminhoexec;
    double multiplicador = 1;
    double frequencia = 60;
    bool showV = false;
    bool showC = true;

    SimulationThread* simulationThread;
    ServerThread* serverThread;

    QMenu *contextMenu = nullptr;
    QAction *onZoomOut;
    QAction *showVoltageAction;
    QAction *showCurrentAction;

    //////////////////////////////////////////////////
    QVector<double> dataVectorT;
    QVector<double> dataVectorC;
    QVector<double> dataVectorV;
    //////////////////////////////////////////////////
};

#endif // MAINWINDOW_H
