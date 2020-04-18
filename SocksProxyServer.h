#include <QTcpSocket>
#include <QTcpServer>
#include <QDataStream>
#include "QTime"

class SocksProxyServer: public QObject
{
	Q_OBJECT

private:
    QTcpServer *server;

public:
    SocksProxyServer(QString address, quint16 port, QString whitelist);
    bool isListening() const;

public slots:
    void newConnection();

private:
    QHostAddress whitelist;
};
