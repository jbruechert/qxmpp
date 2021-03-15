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

#ifndef QXMPPAVATARMANAGER_H
#define QXMPPAVATARMANAGER_H

#include <QObject>

#include <QXmppClientExtension.h>
#include <QXmppPubSubManager.h>
#include "QXmppAvatar.h"

class QXmppAvatarMetadataItem;
class QXmppAvatarDataItem;

using AvatarMetadata = QXmppPubSubManager::ItemsResult<QXmppAvatarMetadataItem>;
using AvatarData = QXmppPubSubManager::ItemsResult<QXmppAvatarDataItem>;

class QXmppAvatarManager : public QXmppClientExtension
{
    Q_OBJECT

public:
    explicit QXmppAvatarManager();

    QFuture<AvatarMetadata> fetchAvatarMetadata(const QString &bareJid);
    QFuture<AvatarData> fetchAvatar(const QString &bareJid, const QString &itemId);

    QFuture<QXmppPubSubManager::PublishItemResult> publishAvatar(const QXmppAvatarInfo &avatarInfo,
            const QByteArray &avatarData);
};

#endif // QXMPPAVATARMANAGER_H
