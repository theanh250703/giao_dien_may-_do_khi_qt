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
    connect(updateCom, &QTimer::timeout, this, &MainWindow::update_com_func);
    updateCom->start(1000);                      // cap nhat cong com moi 1s

    // checkAndReconnect va load portcom modbus
    connect(autoConnectTimer, &QTimer::timeout, this, &MainWindow::checkAndReconnect);
    // autoConnectTimer->start(500);

    // ket noi cong com
    connect(ui->btn_connect_com, &QPushButton::clicked, this,[=]
        {
            connect_port(ui->comboBoxCom->currentText());
        });

    // nhan tin hieu barcode va chay takt time
    // connect(ui->pushButton_4, &QPushButton::clicked, this, &MainWindow::takt_time_func);       // sgn_barcode

    //slot thuc hien phat ra signal dem nguoc mot giay 1 lan
    connect(taktTime, &QTimer::timeout, this, &MainWindow::countdown_func);

    // gia lap tin hieu phan hoi tu PLC va check trang thai san pham
    // connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::sgn_plc);
    connect(sts_product_timer, &QTimer::timeout, this, &MainWindow::check_sts_product);

    connect(ui->pushButton_5, &QPushButton::clicked, this, &MainWindow::readCoilDataPlc);

    connect(ui->pushButton_7, &QPushButton::clicked, this, [=]() {
        writeDataPlc(true);
        takt_time_func();
    });

    connect(ui->pushButton_6, &QPushButton::clicked, this, [=]() {
        writeDataPlc(false);
    });

    // connect doc data IO plc
    connect(readDataPlcTimer, &QTimer::timeout, this, &MainWindow::readIoPlc);
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

    ///////////////////////////////////////
    if(list_current_port.contains(portPlc) && plcConnected  == true)
    {
        autoConnectTimer->stop();
        // readDataPlcTimer->start(500);
    }
    if(!list_current_port.contains(portPlc))
    {
        autoConnectTimer->start(500);
    }
}

void MainWindow::connect_port(const QString &portName)
{
    if(portName.isEmpty())
    {
        QMessageBox::critical(this, "Error", "Cannot open");
        return;
    }

    // dung cap nhat cong com de tien hanh ket noi
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
    taktTime->start(1000);              // bat dau chay ham countdown_func
    sts_product_timer->start(100);      // bat dau chay ham check_sts_product
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
    // gia lap barcode
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
        ui->label_product->setStyleSheet("color: black;"
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

    else if((cycle_time <= 0))
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

void MainWindow::writeDataPlc(bool value)
{
    if (!qmodbus_serial)                                // kiem tra con tro qmodbus_serial rong ko (qmodbus_serial != nullptr)
        return;

    if (qmodbus_serial->state() != QModbusDevice::ConnectedState)
        return;

    QModbusDataUnit data(QModbusDataUnit::Coils, 5, 1);
    data.setValue(0, value);                            // dat thanh ghi o vi tri 0 (coil m3) voi gia tri value

    if (QModbusReply *reply = qmodbus_serial->sendWriteRequest(data, 1))    // slave id: 1
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

void MainWindow::readCoilDataPlc()
{
    // check trang thai xem da ket noi chua
    if (qmodbus_serial->state() != QModbusDevice::ConnectedState)
        return;

    QModbusDataUnit data(QModbusDataUnit::Coils, 3, 1);

    if (QModbusReply *reply = qmodbus_serial->sendReadRequest(data, 1))
    {
        connect(reply, &QModbusReply::finished,
                this, &MainWindow::ready_Read);
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
        coilValue = data.value(0);
        qDebug()<<"Coil value"<<coilValue;
        ui->data_serial->setText(coilValue ? "ON" : "OFF");
    }
    reply->deleteLater();                                   // xoa object o vong lap tiep theo tranh crash

}

void MainWindow::checkAndReconnect()
{
    // bool foundPlc = false;
    n++;
    for (const QString &port : list_current_port)
    {
        qDebug() << "Try PLC on" << port;

        qmodbus_serial->disconnectDevice();                 // dong ket noi
        initModbusSerial(port);
        qmodbus_serial->connectDevice();

        QModbusDataUnit test(QModbusDataUnit::Coils, 3, 1);

        // sendReadRequest tra ve 1 object reply neu ko co loi, tra ve nullptr neu ko doc duoc
        QModbusReply *reply = qmodbus_serial->sendReadRequest(test, 1);
        if (!reply)         // reply == nullptr
            continue;

        connect(reply, &QModbusReply::finished, this, [=]()
                {
                    if (reply->error() == QModbusDevice::NoError)
                    {
                        plcConnected = true;
                        foundPlc = true;
                        n = 0;
                        portPlc = port;

                        ui->label->setText(port);
                        qDebug() << "PLC FOUND ON" << port;

                        if (!readDataPlcTimer->isActive())      // neu chua bat dau doc data
                            readDataPlcTimer->start(500);
                    }
                    else
                    {
                        qDebug() << "PLC not on" << port << reply->errorString();
                    }
                    reply->deleteLater();
                });
        break;  //warning quet bo qua com (can sua) // sua toi qua
    }

    qDebug() << "PLC FOUND ON:" << portPlc;

    if(n > 3)
    {
        foundPlc = false;
        plcConnected = false;
        if (!foundPlc)
        {
            ui->label->setText("PLC OFFLINE");

            if (readDataPlcTimer->isActive())
                readDataPlcTimer->stop();
        }
    }
}

void MainWindow::readIoPlc()
{
    if(qmodbus_serial->state() == QModbusDevice::UnconnectedState)
        return;

    //IO
    // QModbusDataUnit data(QModbusDataUnit::DiscreteInputs, 2, 1);         // doc dau vao X2
    // QModbusReply *reply = qmodbus_serial->sendReadRequest(data, 1);

    //doc coil M
    QModbusDataUnit data(QModbusDataUnit::Coils, 3, 1);
    QModbusReply *reply = qmodbus_serial->sendReadRequest(data, 1);
    if(reply == nullptr)
    {
        qDebug()<<"no respon IO";
        return;
    }

    connect(reply, &QModbusReply::finished, this, [=]
            {
        if(reply->error() == QModbusDevice::NoError)
            {
            QModbusDataUnit dataPlcIO;
            dataPlcIO = reply->result();
            response_from_plc = dataPlcIO.value(0);
            // qDebug()<<"response_from_plc "<<response_from_plc;
            ui->label_2->setText(response_from_plc ? "ON" : "OFF");
        }
    });
}

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


