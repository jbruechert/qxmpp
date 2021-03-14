/*
 * Copyright (C) 2008-2021 The QXmpp developers
 *
 * Author:
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

#ifndef QXMPPAVATARMETADATAITEM_H
#define QXMPPAVATARMETADATAITEM_H

#include <QXmppPubSubItem.h>

#include <optional>

#include <QSharedDataPointer>
#include <QVector>
#include <QMimeType>

class QXmppAvatarMetadataItemPrivate;
class QXmppAvatarInfoPrivate;

class QXmppAvatarDataItemPrivate;

class QXmppAvatarInfo
{
public:
    QXmppAvatarInfo();
    ~QXmppAvatarInfo();
    QXmppAvatarInfo(const QXmppAvatarInfo &other);

    quint32 bytes() const;
    void setBytes(const quint32 value);

    std::optional<quint16> height() const;
    void setHeight(std::optional<quint16> value);

    QString id() const;
    void setId(const QString &value);

    QMimeType type() const;
    void setType(const QMimeType &value);

    QString url() const;
    void setUrl(const QString &value);

    std::optional<quint16> width() const;
    void setWidth(const quint16 value);

private:
    QSharedDataPointer<QXmppAvatarInfoPrivate> d;
};

class QXmppAvatarMetadataItem : public QXmppPubSubItem
{
public:
    QXmppAvatarMetadataItem();
    ~QXmppAvatarMetadataItem();
    QXmppAvatarMetadataItem(const QXmppAvatarMetadataItem &other);

    QVector<QXmppAvatarInfo> infos() const;
    void setInfos(const QVector<QXmppAvatarInfo> &infos);

    void parsePayload(const QDomElement &payloadElement) override;
    void serializePayload(QXmlStreamWriter *writer) const override;

private:
    QSharedDataPointer<QXmppAvatarMetadataItemPrivate> d;
};

class QXmppAvatarDataItem : public QXmppPubSubItem
{
public:
    QXmppAvatarDataItem();
    ~QXmppAvatarDataItem();
    QXmppAvatarDataItem(const QXmppAvatarDataItem &other);

    QByteArray data() const;

    void parsePayload(const QDomElement &payloadElement) override;
    void serializePayload(QXmlStreamWriter *writer) const override;

private:
    QSharedDataPointer<QXmppAvatarDataItemPrivate> d;
};

#endif  // QXMPPAVATARMETADATAITEM_H
