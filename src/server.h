// src/server.h – Server for detached controllers
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <QtNetwork>

#include "engine.h"


namespace plstim {
class Server : public QTcpServer
{
    Q_OBJECT
protected:
    Engine* m_engine;

public:
    Server (Engine* engine, QObject* parent=NULL)
	: QTcpServer (parent), m_engine (engine)
    {}

public slots:
    void start ()
    {
	qDebug () << "SSL supported:" << QSslSocket::supportsSsl ();

	auto settings = m_engine->settings ();
	quint16 port = 3928;
	if (settings->contains ("serverPort"))
	    port = settings->value ("serverPort").toInt ();

	qDebug () << "Listening on port" << port;
	listen (QHostAddress::Any, port);

	for (;;) {
	    qDebug () << "waiting for clients";
	    // Wait for a client to connect
	    waitForNewConnection (-1);
	    QTcpSocket* sock = nextPendingConnection ();
	    if (! sock) continue;

	    // Read commands from the client
	    while (sock->isValid ()
		    && sock->state () == QAbstractSocket::ConnectedState) {
		// Wait for a full line to be available
		if (! sock->waitForReadyRead ())
		    continue;
		auto line = sock->readLine ();
		qDebug () << "client command:" << line;

		// Evaluate the expression
		QVariant answer = m_engine->evaluate (line);

		// Send the answer
		QString fmt ("%1\n");
		if (answer.canConvert<int> ())
		    sock->write (fmt.arg (answer.toInt ()).toUtf8 ());
		else
		    sock->write (answer.toString ().toUtf8 ());
	    }
	    
	    // Cleanup
	    delete sock;
	}
    }
};
} // namespace plstim
