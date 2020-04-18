# QSocksProxyServer - a SOCKS server written in Qt

## Overview

QSocksProxyServer is a very simple and basic SOCKS server written in Qt.
I started with https://github.com/TatianaGV/Socks5_server and reworked it a bit.

## Features

* CONNECT method.
* Only IPv4.
* Doesn't support authentication.
* You can whitelist a single IP from
  which the server accepts connections
  in leau of authentication.

## Example usage

No arguments (will listen on 0.0.0.0:1080 by default):

    QSocksProxyServer

Specify interface to bind, port and accept connections only from 192.168.1.100:

    QSocksProxyServer -i 0.0.0.0 -p 1080 -w 192.168.1.100

## Command line options

Usage: ./QSocksProxyServer [options]

Options:
  -h, --help                   Displays this help.
  -p, --port <port>            The port on which the SOCKS server listens.
  -i, --interface <interface>  The interface on which the SOCKS server listens.
  --whitelist, -w <whitelist>  Whitelist a single IPv4 address to accept
                               connections from.
  --verbose, -v <verbose>      Verbose mode.
