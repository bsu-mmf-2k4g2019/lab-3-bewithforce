#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QDataStream>
#include <QTcpSocket>
#include <QPlainTextEdit>

// int is used for this constants
#define READ_FORTUNE_MARKER (0u)
#define WRITE_FORTUNE_MARKER (1u)

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void setConnection();
    void sendMessage();
    void handleMsgFromServer();
    void displayError(QAbstractSocket::SocketError socketError);
    void enableButtons();

private:
    QComboBox *hostCombo = nullptr;
    QLineEdit *portLineEdit = nullptr;
    QLineEdit *messageEditText = nullptr;
    QPlainTextEdit *messagesBox = nullptr;
    QPushButton *connectionButton = nullptr;
    QPushButton *sendMessageButton = nullptr;

    QTcpSocket *tcpSocket = nullptr;
    QDataStream in;
    QStringList fortunes;
};

#endif // WIDGET_H
