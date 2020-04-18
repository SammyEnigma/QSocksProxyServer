#include "SocksProxyServer.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QLoggingCategory>

static void (*oldMessageHandler)(QtMsgType /*type*/, const QMessageLogContext &/*context*/, const QString &msg);
static void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (type > QtDebugMsg) {
        oldMessageHandler(type, context, msg);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({{"p", "port"}, "The port on which the SOCKS server listens.", "port", "1080"});
    parser.addOption({{"i", "interface"}, "The interface on which the SOCKS server listens.", "interface", "0.0.0.0"});
    parser.addOption({{"whitelist", "w"}, "Whitelist a single IPv4 address to accept connections from.", "whitelist"});
    parser.addOption({{"verbose", "v"}, "Verbose mode.", "verbose"});

    parser.parse(a.arguments());

    if (parser.isSet("help")) {
        QTextStream(stdout) << parser.helpText() << endl;
        return 0;
    }

    if (!parser.isSet("verbose")) {
        oldMessageHandler = qInstallMessageHandler(myMessageOutput);
    }

    SocksProxyServer server(parser.value("interface"), parser.value("port").toInt(), parser.value("whitelist"));
    if (!server.isListening()) {
        qFatal("Server failed to start...");
        return 1;
    }

    return a.exec();
}
