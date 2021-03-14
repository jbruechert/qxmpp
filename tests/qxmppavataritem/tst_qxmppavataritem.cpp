/*
 * Copyright (C) 2008-2021 The QXmpp developers
 *
 * Authors:
 *  Jeremy Lainé
 *  Manjeet Dahiya
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

#include <QObject>
#include <QTest>
#include <QXmppAvatar.h>

#include "util.h"

class tst_QXmppAvatarItem : public QObject
{
    Q_OBJECT

private slots:
    void testAvatarMetadataItem();
    void testMultipleInfo();
    void testAvatarDataItem();
};

void tst_QXmppAvatarItem::testAvatarMetadataItem()
{
    QByteArray xml(
    "<item id=\"111f4b3c50d7b0df729d299bc6f8e9ef9066971f\">"
        "<metadata xmlns=\"urn:xmpp:avatar:metadata\">"
            "<info bytes=\"12345\" "
                "height=\"64\" "
                "id=\"111f4b3c50d7b0df729d299bc6f8e9ef9066971f\" "
                "type=\"image/png\" "
                "width=\"64\"/>"
        "</metadata>"
    "</item>");

    QXmppAvatarMetadataItem meta;
    parsePacket(meta, xml);
    QCOMPARE(meta.id(), "111f4b3c50d7b0df729d299bc6f8e9ef9066971f");
    const auto infos = meta.infos();
    const auto info = infos.front();
    QCOMPARE(info.bytes(), 12345);
    QCOMPARE(info.id(), "111f4b3c50d7b0df729d299bc6f8e9ef9066971f");
    QCOMPARE(info.height(), 64);
    QCOMPARE(info.type(), QMimeDatabase().mimeTypeForName("image/png"));
    QCOMPARE(info.width(), 64);

    serializePacket(meta, xml);
}

void tst_QXmppAvatarItem::testMultipleInfo()
{
    QByteArray xml(
    "<item>"
        "<metadata xmlns=\"urn:xmpp:avatar:metadata\">"
            "<info bytes=\"12345\" "
                "height=\"64\" "
                "id=\"111f4b3c50d7b0df729d299bc6f8e9ef9066971f\" "
                "type=\"image/png\" "
                "width=\"64\""
            "/>"
            "<info bytes=\"23456\" "
                "height=\"64\" "
                "id=\"357a8123a30844a3aa99861b6349264ba67a5694\" "
                "type=\"image/gif\" "
                "url=\"http://avatars.example.org/happy.gif\" "
                "width=\"64\""
            "/>"
        "</metadata>"
    "</item>");

    QXmppAvatarMetadataItem meta;

    parsePacket(meta, xml);

    const auto firstI = meta.infos().front();
    QCOMPARE(firstI.bytes(), 12345);

    serializePacket(meta, xml);

    QXmppAvatarMetadataItem meta2;

    QXmppAvatarInfo info1;
    info1.setBytes(12345);
    info1.setId(QStringLiteral("111f4b3c50d7b0df729d299bc6f8e9ef9066971f"));
    info1.setHeight(64);
    info1.setType(QMimeDatabase().mimeTypeForName(QStringLiteral("image/png")));
    info1.setWidth(64);

    QXmppAvatarInfo info2;
    info2.setBytes(23456);
    info2.setId(QStringLiteral("357a8123a30844a3aa99861b6349264ba67a5694"));
    info2.setHeight(64);
    info2.setType(QMimeDatabase().mimeTypeForName(QStringLiteral("image/gif")));
    info2.setUrl(QStringLiteral("http://avatars.example.org/happy.gif"));
    info2.setWidth(64);

    meta2.setInfos({
        info1,
        info2
    });

    serializePacket(meta2, xml);
}

void tst_QXmppAvatarItem::testAvatarDataItem()
{
    QByteArray xml(
        "<item id=\"111f4b3c50d7b0df729d299bc6f8e9ef9066971f\">"
            "<data xmlns=\"urn:xmpp:avatar:data\">"
                "SGVsbG8gV29ybGQK"
            "</data>"
        "</item>"
    );

    QXmppAvatarDataItem data;
    parsePacket(data, xml);

    QCOMPARE(data.data(), "Hello World\n");

    serializePacket(data, xml);
}

QTEST_GUILESS_MAIN(tst_QXmppAvatarItem)

#include "tst_qxmppavataritem.moc"
