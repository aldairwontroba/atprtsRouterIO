
#include "mainwindow.h"
#include "ui_mainwindow.h"

/////////////////////////////////////////////////////////////////////////////////

bool ATP_SIM = false;
bool ATP_ISCONECTED = false;
bool GRAFICS_ISSHOW = false;

double* DATA_TIME = NULL;
double* DATA_VOLTAGE = NULL;
double* DATA_CURRENT = NULL;

TCHAR MapObjATP_Memory[]=TEXT("ATP_Memory_Point_IO");
TCHAR MapObjATP_WorkEvent[]=TEXT("WorkEvent_RouterIO");
TCHAR MapObjATP_BackEvent[]=TEXT("BackEvent_RouterIO");

HANDLE hMapFile;
HANDLE workHandle;
HANDLE backHandle;

DWORD timeout;

int vecLen = 8;

struct BufferValue
{
    double DataVar[8];
    double DataIn[8]; //o indice 0 é o tempo
    double DataOut[8];
    int errorMsg;
    int overrun;
};

BufferValue* structurePointer;
BufferValue structureLocal;

QByteArray macBytes;

/////////////////////////////////////////////////////////////////////////////

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionShow_Graphics_triggered()
{

    if (GRAFICS_ISSHOW == false)
    {
        GRAFICS_ISSHOW = true;
        setGrafics(ui->customPlot);
    }
    else
    {
        updateGrafics(ui->customPlot);
    }
    ui->textEdit->append("Grafico atualizado!");

}
void MainWindow::updateGrafics(QCustomPlot *customPlot)
{
    customPlot->graph(0)->setData(dataVectorT, dataVectorC);
    customPlot->graph(1)->setData(dataVectorT, dataVectorV);
    customPlot->rescaleAxes();
    customPlot->replot();
}
void MainWindow::setGrafics(QCustomPlot *customPlot)
{
    // Adicionar gráfico para corrente
    customPlot->addGraph(customPlot->xAxis, customPlot->yAxis);

    // Adicionar gráfico para tensão
    QCPAxisRect *axisRect = customPlot->axisRect();
    QCPAxis *axisVoltage = new QCPAxis(axisRect, QCPAxis::atRight);
    axisRect->addAxis(QCPAxis::atRight, axisVoltage);
    customPlot->addGraph(customPlot->xAxis, axisVoltage);

    // Configurar gráfico da corrente
    customPlot->graph(0)->setData(dataVectorT, dataVectorC);
    customPlot->graph(0)->setPen(QPen(Qt::blue));
    customPlot->graph(0)->setName("Current");
    customPlot->graph(0)->setVisible(showC);

    // Configurar gráfico da tensão
    customPlot->graph(1)->setData(dataVectorT, dataVectorV);
    customPlot->graph(1)->setPen(QPen(Qt::red));
    customPlot->graph(1)->setName("Voltage");
    customPlot->graph(1)->setVisible(showV);

    // Configurar eixos
    customPlot->xAxis->setLabel("Time (s)");
    customPlot->yAxis->setLabel("Current (A)");
    axisVoltage->setLabel("Voltage (V)");

    // Replotar o gráfico
    customPlot->rescaleAxes();

    // make left and bottom axes always transfer their ranges to right and top axes:
//    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
//    connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));

    // let the ranges scale themselves so graph 0 fits perfectly in the visible area:

    // Note: we could have also just called customPlot->rescaleAxes(); instead
    // Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
    customPlot->setInteractions(QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iRangeDrag);
    customPlot->setSelectionRectMode(QCP::SelectionRectMode::srmZoom);

    customPlot->replot();

    ///////////////////////////////////////////////////

    // Definir a política de exibição do menu de contexto
    customPlot->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(customPlot, &QCustomPlot::customContextMenuRequested, this, &MainWindow::contextMenuRequest);
}
void MainWindow::contextMenuRequest(QPoint pos)
{
    if (contextMenu == nullptr)
    {
        // Criar o menu de contexto e as ações
        contextMenu = new QMenu(this);

        onZoomOut = new QAction("Unzoom", this);
        connect(onZoomOut, &QAction::triggered, this, &MainWindow::unZoom);

        showVoltageAction = new QAction("Show Voltage", this);
        showVoltageAction->setCheckable(true);
        showVoltageAction->setChecked(showV);
        connect(showVoltageAction, &QAction::toggled, this, &MainWindow::showVoltage);

        showCurrentAction = new QAction("Show Current", this);
        showCurrentAction->setCheckable(true);
        showCurrentAction->setChecked(showC);
        connect(showCurrentAction, &QAction::toggled, this, &MainWindow::showCurrent);

        // Adicionar as ações ao menu
        contextMenu->addAction(onZoomOut);
        contextMenu->addSeparator();
        contextMenu->addAction(showVoltageAction);
        contextMenu->addAction(showCurrentAction);
    }

    // Exibir o menu de contexto
    contextMenu->popup(ui->customPlot->mapToGlobal(pos));
}
void MainWindow::unZoom()
{
    ui->customPlot->rescaleAxes();
    ui->customPlot->replot();
}
void MainWindow::showVoltage()
{
    if (showVoltageAction->isChecked())
    {
        showV = true;
        ui->customPlot->graph(1)->setVisible(true);
        ui->customPlot->replot();
    }else{
        showV = false;
        ui->customPlot->graph(1)->setVisible(false);
        ui->customPlot->replot();
    }
}
void MainWindow::showCurrent()
{
    if (showCurrentAction->isChecked())
    {
        showC = true;
        ui->customPlot->graph(0)->setVisible(true);
        ui->customPlot->replot();
    }else{
        showC = false;
        ui->customPlot->graph(0)->setVisible(false);
        ui->customPlot->replot();
    }

}
void MainWindow::onUpdateUI(QString msg){
    ui->textEdit->append(msg);
}
void MainWindow::on_actionConnect_triggered()
{
    if (ATP_ISCONECTED)
    {
        ui->textEdit->append("O ATP já está conectado!");
        return;
    }
    hMapFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, //Read,write access
                                FALSE, //do not inherit the name
                                MapObjATP_Memory);

    if (hMapFile == NULL)
    {
        ui->textEdit->append(QString("Could not open file mapping object: %1").arg(GetLastError()));
        return;
    }
    structurePointer = (BufferValue*)MapViewOfFile( hMapFile,
                                                     FILE_MAP_ALL_ACCESS,
                                                     0,
                                                     0,
                                                     sizeof(BufferValue));

    if (structurePointer == NULL)
    {
        ui->textEdit->append(QString("Could not map view of file: %d\n").arg(GetLastError()));
        return;
    }
    ui->textEdit->append("File mapping conectado!");

    workHandle = CreateEvent(NULL, TRUE, FALSE, MapObjATP_WorkEvent); // Manually. no signal
    backHandle = CreateEvent(NULL, TRUE, FALSE, MapObjATP_BackEvent); // Manually. no signal

    if (workHandle == NULL || backHandle == NULL)
    {
        ui->textEdit->append(QString("CreateEvent failed %d\n").arg(GetLastError()));
    }

    timeout  = WaitForSingleObject(workHandle, 1000);

    if (timeout == WAIT_TIMEOUT) {
        ui->textEdit->append("Simulação no ATP não iniciada");
        return;
    }
    ResetEvent(workHandle);

    std::memcpy(&structureLocal, structurePointer, sizeof(BufferValue));

    SetEvent(backHandle);

    ATP_ISCONECTED = true;

    simulationThread = new SimulationThread(this);

    connect(simulationThread, &SimulationThread::finished, simulationThread, &SimulationThread::deleteLater);
    connect(simulationThread, &SimulationThread::updateUI, this, &MainWindow::onUpdateUI);

    ui->textEdit->append("ATP Conectado");

    ATP_SIM = true;
    ui->textEdit->append(QString("Step time: %1 segundos \n"
                                 "End time: %2 segundos").arg(structureLocal.DataVar[2])
                                                         .arg(structureLocal.DataVar[1]));

    simulationThread->start(QThread::TimeCriticalPriority);
}
void MainWindow::on_actionStart_triggered()
{
    if (ATP_ISCONECTED)
    {

    }
    else
    {
        ui->textEdit->append("ATP não conectado!");
        ATP_SIM = false;
    }

}
void MainWindow::on_actionSettings_triggered()
{
    QGridLayout *gridLayout;
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QLabel *label;
    QLabel *label_2;

    janela = new QDialog(this);

    janela->setObjectName("JanelaConfig");
    janela->setWindowTitle("Config");
    janela->resize(366, 183);

    gridLayout = new QGridLayout(janela);

    pushButton = new QPushButton(janela);
    pushButton->setText("OK");

    gridLayout->addWidget(pushButton, 2, 0, 1, 1);

    pushButton_2 = new QPushButton(janela);
    pushButton_2->setText("Cancelar");

    gridLayout->addWidget(pushButton_2, 2, 1, 1, 1);

    label = new QLabel(janela);
    label->setText("Frequencia:");

    gridLayout->addWidget(label, 0, 0, 1, 1);

    label_2 = new QLabel(janela);
    label_2->setText("Multiplicador da Corrente:");

    gridLayout->addWidget(label_2, 1, 0, 1, 1);

    doubleSpinBox = new QDoubleSpinBox(janela);
    doubleSpinBox->setValue(frequencia);

    gridLayout->addWidget(doubleSpinBox, 0, 1, 1, 1);

    doubleSpinBox_2 = new QDoubleSpinBox(janela);
    doubleSpinBox_2->setValue(multiplicador);

    gridLayout->addWidget(doubleSpinBox_2, 1, 1, 1, 1);

    connect(pushButton, &QPushButton::clicked, this, &MainWindow::configok);
    connect(pushButton_2, &QPushButton::clicked, this, &MainWindow::configcancel);

    janela->show();
}
void MainWindow::configok()
{
    frequencia = doubleSpinBox->value();
    multiplicador = doubleSpinBox_2->value();
    janela->close();
}
void MainWindow::configcancel()
{
   janela->close();
}
void MainWindow::on_actionChange_dstMAC_triggered()
{
    QDialog dialog;
    dialog.setWindowTitle("Configurar Endereço MAC");

    // Layout para organizar widgets
    QFormLayout formLayout(&dialog);

    // Campo de entrada para o endereço MAC
    QLineEdit macLineEdit;
    macLineEdit.setInputMask("HH:HH:HH:HH:HH:HH;_");  // Máscara para endereço MAC
    formLayout.addRow("Novo Endereço MAC:", &macLineEdit);

    // Botões OK e Cancelar
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    formLayout.addRow(&buttonBox);

    // Conectar o clique do botão OK para atualizar o endereço MAC
    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, [&]() {
        QString macString = macLineEdit.text();
        macBytes = QByteArray::fromHex(macString.toLatin1());
        dialog.accept();  // Fechar o QDialog após a atualização
    });

    // Conectar o clique do botão Cancelar para fechar o QDialog
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, [&]() {
        dialog.reject();
    });

    // Exibir o QDialog
    dialog.exec();
}
// Função para configurar os parâmetros de comunicação
CommParameters* configureCommParameters() {
    CommParameters* parameters = (CommParameters*)malloc(sizeof(CommParameters));

    // Preencher os campos com os valores desejados
    parameters->vlanPriority = 0;  // Prioridade da VLAN (0 se não estiver usando VLAN)
    parameters->vlanId = 0;  // VLAN ID (0 se não estiver usando VLAN)
    parameters->appId = 0x4000;   // ID da aplicação
    // Substitua pelos seis bytes do endereço MAC de destino
    parameters->dstAddress[0] = 0x38;
    parameters->dstAddress[1] = 0xB1;
    parameters->dstAddress[2] = 0xDB;
    parameters->dstAddress[3] = 0xCD;
    parameters->dstAddress[4] = 0xCE;
    parameters->dstAddress[5] = 0xC5;
    if (macBytes.size() == 6) {
        // Atualizar parameters->dstAddress
        for (int i = 0; i < 6; ++i) {
            parameters->dstAddress[i] = static_cast<uint8_t>(macBytes[i]);
        }
    }
    // Outros parâmetros conforme necessário

    return parameters;
}

SimulationThread::SimulationThread(QObject *parent) :QThread(parent)
{

}
void SimulationThread::run()
{
    const char* inteface = "3";
    qDebug() << "Using interface" << inteface;

    CommParameters* parameters = configureCommParameters();

    SVPublisher svPublisher = SVPublisher_create(parameters, inteface);
    //SVPublisher svPublisher = SVPublisher_create(NULL, inteface);

    if (svPublisher) {
        qDebug() << "SV publisher created";
    }
    else {
        qDebug() << "Failed to create SV publisher";
        return;
    }

    /* Create first ASDU and add data points */

    SVPublisher_ASDU asdu1 = SVPublisher_addASDU(svPublisher, "svpub1", NULL, 1);


    int floatia = SVPublisher_ASDU_addFLOAT(asdu1);
    int floatib = SVPublisher_ASDU_addFLOAT(asdu1);
    int floatic = SVPublisher_ASDU_addFLOAT(asdu1);
    int floatin = SVPublisher_ASDU_addFLOAT(asdu1);
    int floatva = SVPublisher_ASDU_addFLOAT(asdu1);
    int floatvb = SVPublisher_ASDU_addFLOAT(asdu1);
    int floatvc = SVPublisher_ASDU_addFLOAT(asdu1);
    int floatvn = SVPublisher_ASDU_addFLOAT(asdu1);
    int ts1 = SVPublisher_ASDU_addTimestamp(asdu1);


    SVPublisher_setupComplete(svPublisher);


    /////////////////////////////////////////////////////////////

    QThread::Priority currentPriority = this->priority();
    qDebug() << "Prioridade atual da thread:" << currentPriority;

    timeout  = WaitForSingleObject(workHandle, INFINITE);
    std::memcpy(&structureLocal, structurePointer, sizeof(BufferValue));
    SetEvent(backHandle);

    while(true)
    {
        // espero
        timeout  = WaitForSingleObject(workHandle, 1000);
        if (timeout == WAIT_TIMEOUT) {
            emit updateUI("Perda de conexao");
            break;
        }
        ResetEvent(workHandle);

        std::memcpy(&structureLocal.DataIn, structurePointer->DataIn, sizeof(structureLocal.DataIn));
        std::memcpy(&structureLocal.overrun, &structurePointer->overrun, sizeof(structureLocal.overrun));

        SetEvent(backHandle);


        // SV server
        /////////////////////////////////////////////////////////////

        Timestamp ts;
        Timestamp_clearFlags(&ts);
        Timestamp_setTimeInMilliseconds(&ts, Hal_getTimeInMs());

        /* update the values in the SV ASDUs */


        SVPublisher_ASDU_setFLOAT(asdu1, floatia, (float)structureLocal.DataIn[4]);
        SVPublisher_ASDU_setFLOAT(asdu1, floatib, (float)structureLocal.DataIn[5]);
        SVPublisher_ASDU_setFLOAT(asdu1, floatic, (float)structureLocal.DataIn[6]);
        SVPublisher_ASDU_setFLOAT(asdu1, floatin, (float)structureLocal.DataIn[7]);
        SVPublisher_ASDU_setFLOAT(asdu1, floatva, (float)structureLocal.DataIn[1]);
        SVPublisher_ASDU_setFLOAT(asdu1, floatvb, (float)structureLocal.DataIn[2]);
        SVPublisher_ASDU_setFLOAT(asdu1, floatvc, (float)structureLocal.DataIn[3]);
        SVPublisher_ASDU_setFLOAT(asdu1, floatvn, (float)( structureLocal.DataIn[1]+
                                                           structureLocal.DataIn[2]+
                                                           structureLocal.DataIn[3]));
        SVPublisher_ASDU_setTimestamp(asdu1, ts1, ts);

        /* update the sample counters */

        SVPublisher_ASDU_increaseSmpCnt(asdu1);

        /* send the SV message */
        SVPublisher_publish(svPublisher);

        ////////////////////////////////////////////////////////////
    }

    if (structureLocal.overrun > 0) emit updateUI("numero de overrun: " + QString::number(structureLocal.overrun));
    QThread::msleep(10);
    emit updateUI("Encerrando a Thread");
    //implementar o close nas conexoes
    SVPublisher_destroy(svPublisher);

    QThread::quit();
}






