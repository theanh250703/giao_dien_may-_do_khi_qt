#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    resize(600, 350);

    // init object
    updateCom = new QTimer(this);                           // timer cap nhat com moi 1s
    taktTime = new QTimer(this);                            // timer dem nguoc mot giay mot lan
    sts_product_timer = new QTimer(this);                   // timer quet trang thai san pham moi 100ms

    qmodbus_serial = new QModbusRtuSerialClient(this);
    readDataPlcTimer = new QTimer(this);                    // timer doc data plc
    autoConnectTimer = new QTimer(this);

    player = new QMediaPlayer;
    audioOutput = new QAudioOutput;

    // load port va cap nhat cong com
    // load_port();
    // update_com_func();
    connect(updateCom, &QTimer::timeout, this, &MainWindow::update_com_func);
    updateCom->start(1000);                         // cap nhat com moi 1s

    // load portcom modbus va doc du lieu
    // initModbusSerial("COM10");
    // connect(updateCom, &QTimer::timeout, this, &MainWindow::checkAndReconnect);
    // autoConnectTimer->start();

    // reconnect com modbus
    connect(autoConnectTimer, &QTimer::timeout, this, &MainWindow::checkAndReconnect);
    autoConnectTimer->start(1000);

    // ket noi cong com
    connect(ui->btn_connect_com, &QPushButton::clicked, this,[=]
        {
            connect_port(ui->comboBoxCom->currentText());
        });

    // nhan tin hieu barcode va chay takt time
    connect(ui->pushButton_4, &QPushButton::clicked, this, &MainWindow::takt_time_func);        // sgn_barcode

    //slot thuc hien phat ra signal dem nguoc mot giay 1 lan
    connect(taktTime, &QTimer::timeout, this, &MainWindow::countdown_func);

    // gia lap tin hieu phan hoi tu PLC va check trang thai san pham
    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::sgn_plc);
    connect(sts_product_timer, &QTimer::timeout, this, &MainWindow::check_sts_product);

    connect(ui->pushButton_5, &QPushButton::clicked, this, &MainWindow::readDataPlc);

    connect(ui->pushButton_7, &QPushButton::clicked, this, [=]() {
        writeDataPlc(true);
    });

    connect(ui->pushButton_6, &QPushButton::clicked, this, [=]() {
        writeDataPlc(false);
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::load_port()
{
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for(const auto &port:ports)
    {
        ui->comboBoxCom->addItem(port.portName());
        list_current_port.append(port.portName());
    }
}

void MainWindow::update_com_func()
{
    QStringList list_new_port;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        list_new_port.append(info.portName());
    }

    QSet<QString> set_new_port = QSet<QString>(list_new_port.begin(), list_new_port.end());
    QSet<QString> set_current_port = QSet<QString>(list_current_port.begin(), list_current_port.end());

    //Cập nhật các cổng cắm thêm
    QSet<QString> added = set_new_port - set_current_port;
    for(const QString &port : added) {
        // Chỉ thêm nếu ComboBox chưa có
        if (ui->comboBoxCom->findText(port) == -1) {
            ui->comboBoxCom->addItem(port);
        }
    }

    //Cập nhật các cổng bị rút
    QSet<QString> removed = set_current_port - set_new_port;
    for(const QString &port : removed) {
        int index = ui->comboBoxCom->findText(port);
        if (index != -1) {
            ui->comboBoxCom->removeItem(index);
            qDebug() << "Da xoa COM: " << port;
        }
    }

    list_current_port = list_new_port;


    if(list_new_port.isEmpty()) {
        ui->comboBoxCom->clear();
    }

    qDebug()<<"List Port: "<<list_current_port;

}

void MainWindow::connect_port(const QString &portName)
{
    if(portName.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Cannot open");
        return;
    }

    // dug cap nhat cong com de tien hanh ket noi
    updateCom->stop();

    if (COMPORT != nullptr && COMPORT->isOpen()) {
        COMPORT->close();
        return;
    }

    if (COMPORT == nullptr) {
        COMPORT = new QSerialPort(this);
    }

    COMPORT->setPortName(portName);
    COMPORT->setBaudRate(QSerialPort::Baud115200);
    COMPORT->setDataBits(QSerialPort::DataBits::Data8);
    COMPORT->setParity(QSerialPort::NoParity);
    COMPORT->setStopBits(QSerialPort::StopBits::OneStop);
    COMPORT->setFlowControl(QSerialPort::NoFlowControl);

    // check cong com, neu com da ket noi roi thi ko duoc ket noi nua
    if (COMPORT->open(QIODevice::ReadWrite) == false)
    {
        QMessageBox::critical(this, "Error", "Cannot open " + portName);
    }
    else
    {
        QMessageBox::information(this, "Succes", "Open " + portName);
        connect(COMPORT, &QSerialPort::readyRead, this, &MainWindow::read_data);
    }

    updateCom->start();
}

void MainWindow::read_data()
{
    if(COMPORT->isOpen())
    {
        while(COMPORT->bytesAvailable())
        {
            DATA_FROM_BARCODE += COMPORT->readAll();
            if (DATA_FROM_BARCODE.endsWith(char(10)))           // check ki tu cuoi co la \n ko
            {
                is_data_receive = true;
            }

            if(is_data_receive == true)
            {
                ui->data_serial->setText(DATA_FROM_BARCODE);
                DATA_FROM_BARCODE = "";
                is_data_receive = false;
            }
        }
    }
}

void MainWindow::takt_time_func()
{
    cycle_time = ui->spinBox->value();
    ui->lineEditTaktTime->setText(QString::number(cycle_time));
    taktTime->start(1000);
    sts_product_timer->start(100);
    barcode_active = true;
}

void MainWindow::countdown_func()
{
    if(cycle_time == 0)
    {
        ui->lineEditTaktTime->setText(QString::number(cycle_time));
        response_from_plc = false;
        barcode_active = false;
        taktTime->stop();
        sts_product_timer->stop();
        return;
    }
    cycle_time -= 1;
    ui->lineEditTaktTime->setText(QString::number(cycle_time));
}

void MainWindow::sgn_plc()
{
    if(barcode_active == true)
    {
        response_from_plc = true;
        return;
    }
}

void MainWindow::check_sts_product()
{
    if(cycle_time > 0 && response_from_plc == true)
    {
        ui->label_product->setText("OK");
        ui->label_product->setStyleSheet("color: yellow;"
                                             "font-size: 100px;"
                                             "font-weight: bold;"
                                             "background-color: rgb(0,255,0);"
                                             "min-width:250px;"
                                             "max-height:130px;");
        cycle_time = 0;
        countOK +=1;
        playAudio("OK");
        ui->label_count_ok->setText(QString::number(countOK));
        sts_product_timer->stop();
    }

    else if((cycle_time <= 0 && response_from_plc == false))
    {
        ui->label_product->setText("NG");
        ui->label_product->setStyleSheet("color: yellow;"
                                         "font-size: 100px;"
                                         "font-weight: bold;"
                                         "background-color: rgb(255,0,0);"
                                         "min-width:250px;"
                                         "max-height:130px;");
        cycle_time = 0;
        countNG +=1;
        playAudio("NG");
        ui->label_count_ng->setText(QString::number(countNG));
        sts_product_timer->stop();
    }
}

void MainWindow::initModbusSerial(const QString &port)
{
    qmodbus_serial->setConnectionParameter(
        QModbusDevice::SerialPortNameParameter, port);
    qmodbus_serial->setConnectionParameter(
        QModbusDevice::SerialParityParameter, QSerialPort::NoParity);
    qmodbus_serial->setConnectionParameter(
        QModbusDevice::SerialBaudRateParameter, QSerialPort::Baud9600);
    qmodbus_serial->setConnectionParameter(
        QModbusDevice::SerialDataBitsParameter, QSerialPort::Data8);
    qmodbus_serial->setConnectionParameter(
        QModbusDevice::SerialStopBitsParameter, QSerialPort::OneStop);

    qmodbus_serial->setTimeout(500);
    qmodbus_serial->setNumberOfRetries(2);
}

void MainWindow::readDataPlc()
{
    if (qmodbus_serial->state() != QModbusDevice::ConnectedState)
        return;

    QModbusDataUnit data(QModbusDataUnit::Coils, 3, 1);

    if (QModbusReply *reply = qmodbus_serial->sendReadRequest(data, 1))
    {
        connect(reply, &QModbusReply::finished,
                this, &MainWindow::ready_Read);
    }
}

void MainWindow::writeDataPlc(bool value)
{
    if (!qmodbus_serial)
        return;

    if (qmodbus_serial->state() != QModbusDevice::ConnectedState)
        return;

    QModbusDataUnit data(QModbusDataUnit::Coils, 3, 1);
    data.setValue(0, value);

    if (QModbusReply *reply = qmodbus_serial->sendWriteRequest(data, 1))
    {
        connect(reply, &QModbusReply::finished, this, [=]()
                {
                    if (reply->error() == QModbusDevice::NoError)
                    {
                        qDebug() << "Write coil OK";
                    }
                    else
                    {
                        qDebug() << "Write coil ERROR:" << reply->errorString();
                    }
                    reply->deleteLater();
                });
    }
    else
    {
        qDebug() << "Send write request failed";
    }
}

void MainWindow::ready_Read()
{
    auto reply = qobject_cast<QModbusReply*>(sender());             // sender: tra ve Qobject cua doi tuong da phat ra signal (QModbusReply)
    if(reply == nullptr)
    {
        qDebug()<<"No response";
        return;
    }

    else if(reply->error() == QModbusDevice::NoError)
    {
        const QModbusDataUnit data = reply->result();
        // qDebug()<< data.value(0);                        // tra ve mot QVector la q mang quint16
        // vector_data_plc.append(data.values(0));

        coilValue = data.value(0);

        vector_data_plc.append(data.values());

        qDebug()<<"Coil value"<<coilValue;

        // QString str1 = QString("%1").arg(data.value(0));

        ui->data_serial->setText(coilValue ? "ON" : "OFF");

        vector_data_plc.clear();                        // xoa du lieu trong vecto sau moi lan cap nhat du lieu, neu ko bi de them vao mang
    }
    reply->deleteLater();                               // xoa object o vong lap tiep theo tranh crash

}

void MainWindow::checkAndReconnect()
{
    if (plcConnected &&
        qmodbus_serial->state() == QModbusDevice::ConnectedState)
        return;

    for (const QString &port : list_current_port)
    {
        qDebug() << "Try PLC on" << port;

        qmodbus_serial->disconnectDevice();

        initModbusSerial(port);

        qmodbus_serial->connectDevice();

        QModbusDataUnit test(QModbusDataUnit::Coils, 3, 1);

        if (QModbusReply *reply = qmodbus_serial->sendReadRequest(test, 1))
        {
            connect(reply, &QModbusReply::finished, this, [=]()
                    {
                        if (reply->error() == QModbusDevice::NoError)
                        {
                            plcConnected = true;
                            portPlc = port;

                            ui->label->setText(port);
                            qDebug() << "PLC FOUND ON" << port;

                            readDataPlcTimer->start(100);
                        }
                        reply->deleteLater();
                    });
            return;
        }
    }

    plcConnected = false;
    ui->label->setText("PLC OFFLINE");
}


// void MainWindow::checkAndReconnect()
// {
//     if (plcConnected && qmodbus_serial->state() == QModbusDevice::ConnectedState)
//     {
//         return;
//     }

//     for (const QString &port : list_current_port)
//     {
//         qDebug() << "Try PLC on" << port;

//         qmodbus_serial->disconnectDevice();

//         if (qmodbus_serial->connectDevice())
//         {
//             if (qmodbus_serial->state() == QModbusDevice::ConnectedState)
//             {
//                 initModbusSerial(port);
//                 writeDataPlc(true);

//                 readDataPlcTimer->start(100);

//                 if(coilValue == true)
//                 {
//                     plcConnected = true;
//                     portPlc = port;

//                     ui->label->setText(port);
//                     qDebug() << "PLC FOUND ON" << port;
//                     return;
//                 }

//                 else
//                 {
//                     qDebug() << "COM KO MO" << port;
//                 }

//             }
//         }
//         else
//         {
//             qDebug() << "KO TIM THAY COM PLC" << port;
//         }
//     }

//     plcConnected = false;
//     ui->label->setText("PLC OFFLINE");
// }

void MainWindow::playAudio(const QString &text)
{
    player->setAudioOutput(audioOutput);
    if(text == "OK")
    {
        player->setSource(QUrl::fromLocalFile("E:/antue/giao dien may test do khi/Am thanh/audio-OK.mp3"));
    }

    else
    {
        player->setSource(QUrl::fromLocalFile("E:/antue/giao dien may test do khi/Am thanh/audio-NG.mp3"));
    }
    audioOutput->setVolume(1.0);
    player->play();
}


