#include "ClientHandler.h"
#include <QtEndian>
#include <QHostAddress>
#include <QFile>

// Buffer size is 8KB
#define BUFSIZE (8*1024)

#pragma pack(push,1)
// Client lists auth methods it supports.
struct ClientAuthMethods {
    quint8 version = 5;
    quint8 methods_count;
    quint8 methods[1];
};

// We respond with the auth method we have selected.
struct ServerAuthSelection {
    quint8 version = 5;
    quint8 methodOfAuth = 0;
};

// Client says where they want to connect.
struct TargetSelectionRequest {
    quint8 version = 5;
    quint8 command;
    quint8 reserv = 0;
    quint8 typeAddress;
    quint32 host;
    quint16 port;
};

struct TargetSelectionResponse {
    quint8 version = 5;
    quint8 statusAnswer;
    quint8 reserv = 0;
    quint8 typeAddress;
    quint32 host;
    quint16 port;
};
#pragma pack(pop)

const quint16 DNS_name = 3;
const quint16 IPv6 = 4;
const quint16 IPv4 = 1;

const quint16 GSSAPI = 1;
const quint16 NamePass = 2;

const bool printTransferBlocks = false;

SocksConnection::SocksConnection(QTcpSocket *socket, QObject *parent /*= 0*/)
    : QObject(parent)
{
    clientSocket = socket;
    clientSocket->setParent(this);
    connect(clientSocket, &QTcpSocket::disconnected, this, &SocksConnection::deleteLater);
    connect(clientSocket, &QTcpSocket::readyRead, this, &SocksConnection::authNegotiation);
}

SocksConnection::~SocksConnection()
{
    qDebug() << this << "Connection ended.";
}

void SocksConnection::authNegotiation()
{
    // Just read what the client sent, we don't look at it.
    clientSocket->readAll();

    // We don't require authentication.
    ServerAuthSelection answer;
    clientSocket->write(reinterpret_cast<char*>(&answer), sizeof(answer));

    disconnect(clientSocket, &QTcpSocket::readyRead, this, &SocksConnection::authNegotiation);
    connect(clientSocket, &QTcpSocket::readyRead, this, &SocksConnection::targetHostNegotiation);
}

//second request from client
void SocksConnection::targetHostNegotiation()
{
    TargetSelectionRequest request;
    int n = clientSocket->read((char*) &request, sizeof(request));
    if (n != sizeof(request)) {
        qWarning() << this << "Wrong size of request.";
        deleteLater();
        return;
    }

    quint8 typeAddr = request.typeAddress;
    // We don't support IPv6.
    if ((typeAddr == DNS_name)||(typeAddr == IPv6)) {
        TargetSelectionResponse answer;
        answer.statusAnswer = 8;
        answer.host = 0;
        answer.port = 0;
        answer.typeAddress = 0;
        clientSocket->write(reinterpret_cast<char*>(&answer), sizeof(answer));
        clientSocket->disconnectFromHost();
    } else if (typeAddr == IPv4) {
        QString host = QHostAddress(qFromBigEndian(request.host)).toString();
        int port = qFromBigEndian(request.port);
        qDebug() << this << "connecting to" << host << port;

        disconnect(clientSocket, &QTcpSocket::readyRead, this, &SocksConnection::targetHostNegotiation);

        // Connect to target.
        targetSocket = new QTcpSocket(this);
        connect(targetSocket, &QTcpSocket::connected, this, &SocksConnection::onTargetConnected);
        connect(targetSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(targetConnectionError(QAbstractSocket::SocketError)));
        connect(targetSocket, &QTcpSocket::disconnected, this, &SocksConnection::deleteLater);
        targetSocket->connectToHost(host, port);
    }
}

void SocksConnection::onTargetConnected()
{
    TargetSelectionResponse answer;
    answer.statusAnswer = 0;
    answer.typeAddress = 1;
    answer.host = 0;
    answer.port = 0;
    clientSocket->write(reinterpret_cast<char*>(&answer), sizeof(answer));
    startRedirect();
}

//connect left and right socket
void SocksConnection::startRedirect()
{
    connect(clientSocket, &QTcpSocket::readyRead, this, &SocksConnection::writeToTarget);
    connect(targetSocket, &QTcpSocket::readyRead, this, &SocksConnection::writeToClient);
    connect(clientSocket, &QTcpSocket::bytesWritten, this, &SocksConnection::writeToClient);
    connect(targetSocket, &QTcpSocket::bytesWritten, this, &SocksConnection::writeToTarget);
    writeToClient();
    writeToTarget();
}

void SocksConnection::writeToTarget()
{
    char buffer[BUFSIZE];
    int bytesRead = clientSocket->read(buffer, sizeof(buffer));
    if (bytesRead > 0) {
        int bytesWritten = targetSocket->write(buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            qWarning("%p: failed to write everything - read %d bytes, wrote %d bytes.", this, bytesRead, bytesWritten);
        } else if (printTransferBlocks) {
            qDebug("%p: %s:%d->%s:%d %.2lf kb", this,
                   qPrintable(clientSocket->peerAddress().toString()), clientSocket->peerPort(),
                   qPrintable(targetSocket->peerAddress().toString()), targetSocket->peerPort(),
                   bytesWritten / 1024.0);
        }
    }
}


void SocksConnection::writeToClient()
{
    char buffer[BUFSIZE];
    int bytesRead = targetSocket->read(buffer, sizeof(buffer));
    if (bytesRead > 0) {
        int bytesWritten = clientSocket->write(buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            qWarning("%p: failed to write everything - read %d bytes, wrote %d bytes.", this, bytesRead, bytesWritten);
        } else if (printTransferBlocks) {
            qDebug("%p: %s:%d->%s:%d %.2lf kb", this,
                   qPrintable(targetSocket->peerAddress().toString()), targetSocket->peerPort(),
                   qPrintable(clientSocket->peerAddress().toString()), clientSocket->peerPort(),
                   bytesWritten / 1024.0);
        }
    }
}

void SocksConnection::targetConnectionError(QAbstractSocket::SocketError err)
{
    qWarning() << this << err;
    clientSocket->disconnectFromHost();
}
