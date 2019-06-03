#include "widget.h"

#include <QHostInfo>
#include <QNetworkInterface>
#include <QGridLayout>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , hostCombo(new QComboBox)
    , portLineEdit(new QLineEdit)
    , messagesBox(new QPlainTextEdit())
    , connectionButton(new QPushButton("Connect"))
    , sendMessageButton(new QPushButton("Send"))
    , tcpSocket(new QTcpSocket(this))
{
    qDebug() << "Constructor is called";
    hostCombo->setEditable(true);
    // find out name of this machine
    QString name = QHostInfo::localHostName();
    if (!name.isEmpty()) {
        hostCombo->addItem(name);
        QString domain = QHostInfo::localDomainName();
        if (!domain.isEmpty())
            hostCombo->addItem(name + QChar('.') + domain);
    }
    if (name != QLatin1String("localhost"))
        hostCombo->addItem(QString("localhost"));
    // find out IP addresses of this machine
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // add non-localhost addresses
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (!ipAddressesList.at(i).isLoopback())
            hostCombo->addItem(ipAddressesList.at(i).toString());
    }
    // add localhost addresses
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i).isLoopback())
            hostCombo->addItem(ipAddressesList.at(i).toString());
    }

    portLineEdit->setValidator(new QIntValidator(1, 65535, this));

    auto hostLabel = new QLabel("Server name:");
    hostLabel->setBuddy(hostCombo);
    auto portLabel = new QLabel("Server port:");
    portLabel->setBuddy(portLineEdit);

    messageEditText = new QLineEdit();


    messagesBox = new QPlainTextEdit();
    messagesBox->setReadOnly(true);

    connectionButton->setDefault(true);
    connectionButton->setEnabled(false);

    auto quitButton = new QPushButton("Quit");

    in.setDevice(tcpSocket);
    in.setVersion(QDataStream::Qt_4_0);

    connect(hostCombo, &QComboBox::editTextChanged,
            this, &Widget::enableButtons);
    connect(portLineEdit, &QLineEdit::textChanged,
            this, &Widget::enableButtons);
    connect(messageEditText, &QLineEdit::textChanged,
            this, &Widget::enableButtons);
    connect(connectionButton, &QAbstractButton::clicked,
            this, &Widget::setConnection);
    connect(sendMessageButton, &QAbstractButton::clicked,
            this, &Widget::sendMessage);
    connect(quitButton, &QAbstractButton::clicked, this, &QWidget::close);

    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &Widget::displayError);

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(hostLabel, 0, 0);
    mainLayout->addWidget(hostCombo, 0, 1);
    mainLayout->addWidget(portLabel, 1, 0);
    mainLayout->addWidget(portLineEdit, 1, 1);
    mainLayout->addWidget(connectionButton, 2, 0, 1, 2);
    mainLayout->addWidget(messagesBox, 3, 0, 1, 2);
    mainLayout->addWidget(messageEditText, 4, 0, 1, 2);
    mainLayout->addWidget(sendMessageButton, 5, 0);
    mainLayout->addWidget(quitButton, 5, 1);

    portLineEdit->setFocus();

    enableButtons();
}

Widget::~Widget()
{

}

void Widget::setConnection()
{
    connectionButton->setEnabled(false);
    sendMessageButton->setEnabled(false);
    tcpSocket->abort();
    tcpSocket->connectToHost(hostCombo->currentText(),
                             portLineEdit->text().toInt());
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);
    connect(tcpSocket, &QAbstractSocket::readyRead,
            this, &Widget::handleMsgFromServer);
}

void Widget::sendMessage()
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);

    out << messageEditText->text();

    tcpSocket->write(block);
    tcpSocket->flush();
}

void Widget::handleMsgFromServer()
{
    in.startTransaction();

    int trType;
    in >> trType;
    if(trType == 1){
        QString nextFortune;
        in >> nextFortune;
        qDebug() << nextFortune;
        if (!in.commitTransaction())
            return;
        qDebug() << nextFortune;
        fortunes.push_back(nextFortune);
        messagesBox->clear();
        for(auto fortune : fortunes){
            messagesBox->appendPlainText(fortune);
        }
    } else {
        QStringList nextFortune;
        in >> nextFortune;

        qDebug() << nextFortune;
        if (!in.commitTransaction())
            return;

        fortunes = nextFortune;
        messagesBox->clear();
        for(auto fortune : fortunes){
            messagesBox->appendPlainText(fortune);
        }
    }

    connectionButton->setEnabled(true);
    sendMessageButton->setEnabled(true);
}

void Widget::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, tr("Fortune Client"),
                                 tr("The host was not found. Please check the "
                                    "host name and port settings."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr("Fortune Client"),
                                 tr("The connection was refused by the peer. "
                                    "Make sure the fortune server is running, "
                                    "and check that the host name and port "
                                    "settings are correct."));
        break;
    default:
        QMessageBox::information(this, tr("Fortune Client"),
                                 tr("The following error occurred: %1.")
                                 .arg(tcpSocket->errorString()));
    }

    connectionButton->setEnabled(true);
    sendMessageButton->setEnabled(true);
}

void Widget::enableButtons()
{
    connectionButton->setEnabled(!hostCombo->currentText().isEmpty() &&
                                 !portLineEdit->text().isEmpty());
    sendMessageButton->setEnabled(!hostCombo->currentText().isEmpty() &&
                                 !portLineEdit->text().isEmpty() &&
                                 !messageEditText->text().isEmpty());

}
