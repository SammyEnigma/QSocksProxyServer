#include "SocksProxyServer.h"
#include "ClientHandler.h"

SocksProxyServer::SocksProxyServer(QString address, quint16 port, QString whitelist)
{
    server = new QTcpServer();
    this->whitelist = QHostAddress(whitelist);

    if (!server->listen(QHostAddress(address), port))
	{
        qWarning() << "Unable to start the server:" << server->errorString();
        server->close();
		return;
	}

    connect(server, &QTcpServer::newConnection, this, &SocksProxyServer::newConnection);
    qDebug() << "Server started at" << address << port;
}

bool SocksProxyServer::isListening() const
{
    server->isListening();
}

void SocksProxyServer::newConnection()
{
    QTcpSocket *socket = server->nextPendingConnection();
    if (!whitelist.isNull() && socket->peerAddress() != whitelist) {
        qDebug() << "Refused connection from" << socket->peerAddress() << "because whitelist is" << whitelist;
        delete socket;
    } else {
        auto connection = new SocksConnection(socket);
        qDebug() << connection << "New connection registred";
    }
}
