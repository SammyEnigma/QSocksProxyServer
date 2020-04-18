#include <QTcpSocket>
#include <QDataStream>

class SocksConnection: public QObject
{
    Q_OBJECT
public:
    SocksConnection(QTcpSocket * socket, QObject *parent = 0);
    ~SocksConnection();

private:
    QTcpSocket *clientSocket;
    QTcpSocket *targetSocket;

    void startRedirect();

private slots:
    void targetConnectionError(QAbstractSocket::SocketError);
    void authNegotiation();
    void targetHostNegotiation();
    void writeToTarget();
    void writeToClient();
    void onTargetConnected();
};
