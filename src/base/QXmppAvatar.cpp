/*
 * Copyright (C) 2008-2021 The QXmpp developers
 *
 * Author:
 *  Jonah Brüchert
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

#include "QXmppAvatar.h"

#include <QXmlStreamWriter>
#include <QMimeType>
#include <QMimeDatabase>
#include <QDebug>

#include <QXmppConstants_p.h>

class QXmppAvatarInfoPrivate : public QSharedData
{
public:
    quint32 bytes = 0;
    std::optional<quint16> height;
    QString id;
    QMimeType type;
    QString url;  // optional
    std::optional<quint16> width;
};

class QXmppAvatarMetadataItemPrivate : public QSharedData
{
public:
    QDomElement pointer;
    QVector<QXmppAvatarInfo> infos;
};

class QXmppAvatarDataItemPrivate : public QSharedData
{
public:
    QByteArray data;
};

QXmppAvatarMetadataItem::QXmppAvatarMetadataItem()
    : d(new QXmppAvatarMetadataItemPrivate())
{
}
QXmppAvatarMetadataItem::~QXmppAvatarMetadataItem() = default;
QXmppAvatarMetadataItem::QXmppAvatarMetadataItem(const QXmppAvatarMetadataItem &other) = default;

QVector<QXmppAvatarInfo> QXmppAvatarMetadataItem::infos() const
{
    return d->infos;
}

void QXmppAvatarMetadataItem::setInfos(const QVector<QXmppAvatarInfo> &infos)
{
    d->infos = infos;
}

void QXmppAvatarMetadataItem::parsePayload(const QDomElement &payloadElement)
{
    for (auto element = payloadElement.firstChildElement();
         !element.isNull();
         element = element.nextSiblingElement()) {

        if (element.tagName() != QStringLiteral("info")) {
            continue;
        }

        Q_ASSERT(element.tagName() == QStringLiteral("info"));

        std::optional<quint16> height = std::nullopt;
        if (!element.attribute(QStringLiteral("height")).isEmpty()) {
            height = element.attribute(QStringLiteral("height")).toUInt();
        }

        QXmppAvatarInfo info;
        info.setBytes(element.attribute(QStringLiteral("bytes")).toUInt());
        info.setHeight(height);
        info.setId(element.attribute(QStringLiteral("id")));
        info.setType(QMimeDatabase().mimeTypeForName(element.attribute(QStringLiteral("type"))));
        info.setUrl(element.attribute(QStringLiteral("url")));
        info.setWidth(element.attribute(QStringLiteral("width")).toUShort());

        d->infos.push_back(std::move(info));
    }
}

void QXmppAvatarMetadataItem::serializePayload(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("metadata"));
    writer->writeDefaultNamespace(ns_avatar_metadata);

    for (const auto &info : std::as_const(d->infos)) {
        writer->writeStartElement(QStringLiteral("info"));

        writer->writeAttribute(QStringLiteral("bytes"), QString::number(info.bytes()));
        if (info.height()) {
            writer->writeAttribute(QStringLiteral("height"), QString::number(*info.height()));
        }
        writer->writeAttribute(QStringLiteral("id"), info.id());
        writer->writeAttribute(QStringLiteral("type"), info.type().name());
        if (!info.url().isEmpty()) {
            writer->writeAttribute(QStringLiteral("url"), info.url());
        }
        if (info.width()) {
            writer->writeAttribute(QStringLiteral("width"), QString::number(*info.width()));
        }

        writer->writeEndElement();
    }

    writer->writeEndElement();
}

QXmppAvatarInfo::QXmppAvatarInfo()
    : d(new QXmppAvatarInfoPrivate())
{
}

QXmppAvatarInfo::QXmppAvatarInfo(const QXmppAvatarInfo &other) = default;
QXmppAvatarInfo::~QXmppAvatarInfo() = default;

quint32 QXmppAvatarInfo::bytes() const
{
    return d->bytes;
}

void QXmppAvatarInfo::setBytes(const quint32 value)
{
    d->bytes = value;
}

std::optional<quint16> QXmppAvatarInfo::height() const
{
    return d->height;
}

void QXmppAvatarInfo::setHeight(std::optional<quint16> value)
{
    d->height = value;
}

QString QXmppAvatarInfo::id() const
{
    return d->id;
}

void QXmppAvatarInfo::setId(const QString &value)
{
    d->id = value;
}

QMimeType QXmppAvatarInfo::type() const
{
    return d->type;
}

void QXmppAvatarInfo::setType(const QMimeType &value)
{
    d->type = value;
}

QString QXmppAvatarInfo::url() const
{
    return d->url;
}

std::optional<quint16> QXmppAvatarInfo::width() const
{
    return d->width;
}

void QXmppAvatarInfo::setWidth(const quint16 value)
{
    d->width = value;
}

void QXmppAvatarInfo::setUrl(const QString &value)
{
    d->url = value;
}

QXmppAvatarDataItem::QXmppAvatarDataItem()
    : d(new QXmppAvatarDataItemPrivate())
{
}
QXmppAvatarDataItem::~QXmppAvatarDataItem() = default;
QXmppAvatarDataItem::QXmppAvatarDataItem(const QXmppAvatarDataItem &other) = default;

QByteArray QXmppAvatarDataItem::data() const
{
    return d->data;
}

void QXmppAvatarDataItem::parsePayload(const QDomElement &payloadElement)
{
    d->data = QByteArray::fromBase64(payloadElement.text().toUtf8());
}

void QXmppAvatarDataItem::serializePayload(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("data"));
    writer->writeDefaultNamespace(ns_avatar_data);

    writer->writeCharacters(d->data.toBase64());

    writer->writeEndElement();
}
