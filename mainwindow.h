#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QMessageBox>
#include <qmodbusrtuserialclient>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QFile>
#include <QMediaPlayer>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    // khoi tao va ket noi com
    QSerialPort *COMPORT = nullptr;
    QTimer *updateCom;
    QStringList list_current_port;

    void load_port();
    void update_com_func();
    void connect_port(const QString &portName);

    // doc du lieu serial
    QString DATA_FROM_BARCODE;
    bool is_data_receive = false;

    void read_data();

    // nhan tin hieu tu barcode va chay takt time
    int cycle_time = 0;
    QTimer *taktTime;

    void takt_time_func();          // nhan tin hieu va dem nguoc
    void countdown_func();

    // nhan phan hoi tu PLC va hien thi trang thai san pham (gia lap)
    bool sts_prodcut = false;
    bool barcode_active = false;   // chi nhan gia tri plc khi da quet barcode

    int countNG = 0;
    int countOK = 0;
    QTimer *sts_product_timer;

    void sgn_plc();
    void check_sts_product();

    // khoi tao va doc du lieu tu plc
    QModbusRtuSerialClient *qmodbus_serial;
    QTimer *readDataPlcTimer;
    QTimer *autoConnectTimer;
    QVector<quint16> dataPlc;
    bool stsConnectComModbus = false;
    bool coilValue;
    QString portPlc;
    bool plcConnected = false;

    bool foundPlc;
    int n = 0;

    void initModbusSerial(const QString &port);
    void checkAndReconnect();           // tim va tu dong ket noi com voi plc
    void writeDataPlc(bool value);
    // doc coil
    void readCoilDataPlc();
    void ready_Read();
    // doc dau vao x
    void readIoPlc();
    bool response_from_plc = false;

    // phat am thanh ra loa
    QMediaPlayer *player;
    QAudioOutput *audioOutput;


    void playAudio(const QString &text);
};
#endif // MAINWINDOW_H
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QMessageBox>
#include <qmodbusrtuserialclient>
#include <QAudioOutput>
#include <QAudioFormat>
#include <QFile>
#include <QMediaPlayer>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    // khoi tao va ket noi com
    QSerialPort *COMPORT = nullptr;
    QTimer *updateCom;
    QStringList list_current_port;

    void load_port();
    void update_com_func();
    void connect_port(const QString &portName);

    // doc du lieu serial
    QString DATA_FROM_BARCODE;
    bool is_data_receive = false;

    void read_data();

    // nhan tin hieu tu barcode va chay takt time
    int cycle_time = 0;
    QTimer *taktTime;

    void takt_time_func();          // nhan tin hieu va dem nguoc
    void countdown_func();

    // nhan phan hoi tu PLC va hien thi trang thai san pham (gia lap)
    bool sts_prodcut = false;
    bool barcode_active = false;   // chi nhan gia tri plc khi da quet barcode

    int countNG = 0;
    int countOK = 0;
    QTimer *sts_product_timer;

    void sgn_plc();
    void check_sts_product();

    // khoi tao va doc du lieu tu plc
    QModbusRtuSerialClient *qmodbus_serial;
    QTimer *readDataPlcTimer;
    QTimer *autoConnectTimer;
    QVector<quint16> dataPlc;
    bool stsConnectComModbus = false;
    bool coilValue;
    QString portPlc;
    bool plcConnected = false;

    bool foundPlc;
    int n = 0;

    void initModbusSerial(const QString &port);
    void checkAndReconnect();           // tim va tu dong ket noi com voi plc
    void writeDataPlc(bool value);
    // doc coil
    void readCoilDataPlc();
    void ready_Read();
    // doc dau vao x
    void readIoPlc();
    bool response_from_plc = false;

    // phat am thanh ra loa
    QMediaPlayer *player;
    QAudioOutput *audioOutput;


    void playAudio(const QString &text);
};
#endif // MAINWINDOW_H
