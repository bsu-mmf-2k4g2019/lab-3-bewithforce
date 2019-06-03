#include "widget.h"

#include <QDebug>
#include <QTcpSocket>
#include <QNetworkInterface>

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    qDebug() << "Server constructor is called";
    statusLabel = new QLabel();
    statusLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen()) {
        qDebug() << "Unable to make server listen";
        statusLabel->setText(QString("Unable to start the server: %1.")
                              .arg(tcpServer->errorString()));
        close();
        return;
    }

    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();

    statusLabel->setText(QString("The server is running on\n\nIP: %1\nport: %2\n\n"
                            "Run the Fortune Client example now.")
                         .arg(ipAddress).arg(tcpServer->serverPort()));
    qDebug() << "Start server on: " << ipAddress << ":" << tcpServer->serverPort();

    auto quitButton = new QPushButton(tr("Quit"));
    connect(quitButton, &QAbstractButton::clicked, this, &QWidget::close);
    connect(tcpServer, &QTcpServer::newConnection, this, &Widget::handleNewConnection);

    auto buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(quitButton);
    buttonLayout->addStretch(1);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(buttonLayout);

    in.setVersion(QDataStream::Qt_4_0);
}

Widget::~Widget()
{

}

void Widget::sendFortune(QTcpSocket *clientConnection)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << 1;
    out << fortunes[fortunes.size() - 1];
    clientConnection->write(block);
}

void Widget::sendFortunes(QTcpSocket *clientConnection)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);

    out << 2;
    out << fortunes;
    clientConnection->write(block);
}

void Widget::handleNewConnection()
{
    QTcpSocket *clientConnection = tcpServer->nextPendingConnection();
    clients.push_back(clientConnection);
    in.setDevice(clientConnection);
    connect(clientConnection, &QAbstractSocket::readyRead, this, &Widget::handleReadyRead);
    sendFortunes(clientConnection);
}

void Widget::handleReadyRead()
{
    qDebug() << "Read fortune is called";
    QString fortune;

    // Read fortune from client
    in.startTransaction();
    in >> fortune;
    if (!in.commitTransaction())
        return;
    qDebug() << "Fortune: " << fortune;
    if(fortunes.size() == 50){
        fortunes.pop_front();
    }
    fortunes.push_back(fortune);
    for(auto client : clients){
        sendFortune(client);
    }
}

void Widget::dropClient(QTcpSocket *client)
{
    trType = NO_TRANSACTION_TYPE;
    disconnect(client, &QAbstractSocket::readyRead, this, &Widget::handleReadyRead);
    connect(client, &QAbstractSocket::disconnected,
            client, &QObject::deleteLater);
    client->disconnectFromHost();
    clients.remove(clients.indexOf(client));
}
