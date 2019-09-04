/*
 * Copyright (C) 2008-2019 The QXmpp developers
 *
 * Authors:
 *  Jeremy Lainé
 *
 * Source:
 *  https://github.com/qxmpp-project/qxmpp
 *
 * This file is a part of QXmpp library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#include <QBuffer>
#include <QObject>

#include "QXmppCallManager.h"
#include "QXmppClient.h"
#include "QXmppServer.h"
#include "util.h"

//Q_DECLARE_METATYPE(QXmppTransferJob::Method)

class tst_QXmppCallManager : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void testCall();

    void acceptCall(QXmppCall *call);

private:
    QXmppCall *receiverCall;
};

void tst_QXmppCallManager::init()
{
    receiverCall = nullptr;
}

void tst_QXmppCallManager::acceptCall(QXmppCall *call)
{
    receiverCall = call;
    call->accept();
}

void tst_QXmppCallManager::testCall()
{
    const QString testDomain(QStringLiteral("localhost"));
    const QHostAddress testHost(QHostAddress::LocalHost);
    const quint16 testPort = 12345;

    QXmppLogger logger;
    logger.setLoggingType(QXmppLogger::StdoutLogging);

    // prepare server
    TestPasswordChecker passwordChecker;
    passwordChecker.addCredentials(QStringLiteral("sender"), QStringLiteral("testpwd"));
    passwordChecker.addCredentials(QStringLiteral("receiver"), QStringLiteral("testpwd"));

    QXmppServer server;
    server.setDomain(testDomain);
    server.setPasswordChecker(&passwordChecker);
    server.listenForClients(testHost, testPort);

    // prepare sender
    QXmppClient sender;
    QXmppCallManager *senderManager = new QXmppCallManager;
    sender.addExtension(senderManager);
    sender.setLogger(&logger);

    QEventLoop senderLoop;
    connect(&sender, SIGNAL(connected()), &senderLoop, SLOT(quit()));
    connect(&sender, &QXmppClient::disconnected, &senderLoop, &QEventLoop::quit);

    QXmppConfiguration config;
    config.setDomain(testDomain);
    config.setHost(testHost.toString());
    config.setPort(testPort);
    config.setUser(QStringLiteral("sender"));
    config.setPassword(QStringLiteral("testpwd"));
    sender.connectToServer(config);
    senderLoop.exec();
    QCOMPARE(sender.isConnected(), true);

    // prepare receiver
    QXmppClient receiver;
    QXmppCallManager *receiverManager = new QXmppCallManager;
    connect(receiverManager, &QXmppCallManager::callReceived,
            this, &tst_QXmppCallManager::acceptCall);
    receiver.addExtension(receiverManager);
    receiver.setLogger(&logger);

    QEventLoop receiverLoop;
    connect(&receiver, &QXmppClient::connected, &receiverLoop, &QEventLoop::quit);
    connect(&receiver, &QXmppClient::disconnected, &receiverLoop, &QEventLoop::quit);

    config.setUser(QStringLiteral("receiver"));
    config.setPassword(QStringLiteral("testpwd"));
    receiver.connectToServer(config);
    receiverLoop.exec();
    QCOMPARE(receiver.isConnected(), true);

    // connect call
    qDebug() << "======== CONNECT ========";
    QEventLoop loop;
    QXmppCall *senderCall = senderManager->call(QStringLiteral("receiver@localhost/QXmpp"));
    QVERIFY(senderCall);
    connect(senderCall, &QXmppCall::connected, &loop, &QEventLoop::quit);
    loop.exec();
    QVERIFY(receiverCall);

    QCOMPARE(senderCall->direction(), QXmppCall::OutgoingDirection);
    QCOMPARE(senderCall->state(), QXmppCall::ActiveState);

    QCOMPARE(receiverCall->direction(), QXmppCall::IncomingDirection);
    QCOMPARE(receiverCall->state(), QXmppCall::ActiveState);

    // exchange some media
    qDebug() << "======== TALK ========";
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);
    loop.exec();

    // hangup call
    qDebug() << "======== HANGUP ========";
    connect(senderCall, &QXmppCall::finished, &loop, &QEventLoop::quit);
    senderCall->hangup();
    loop.exec();

    QCOMPARE(senderCall->direction(), QXmppCall::OutgoingDirection);
    QCOMPARE(senderCall->state(), QXmppCall::FinishedState);

    QCOMPARE(receiverCall->direction(), QXmppCall::IncomingDirection);
    QCOMPARE(receiverCall->state(), QXmppCall::FinishedState);
}

QTEST_MAIN(tst_QXmppCallManager)
#include "tst_qxmppcallmanager.moc"
