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

#include "QXmppAvatarManager.h"

#include "QXmppConstants_p.h"

#include <QSharedPointer>
#include <QCryptographicHash>

QXmppAvatarManager::QXmppAvatarManager()
    : QXmppClientExtension()
{
}

QFuture<AvatarMetadata> QXmppAvatarManager::fetchAvatarMetadata(const QString &bareJid)
{
    return client()->findExtension<QXmppPubSubManager>()
            ->requestItems<QXmppAvatarMetadataItem>(bareJid, ns_avatar_metadata);
}

QFuture<AvatarData> QXmppAvatarManager::fetchAvatar(const QString &bareJid, const QString &itemId)
{
    return client()->findExtension<QXmppPubSubManager>()
            ->requestItems<QXmppAvatarDataItem>(bareJid, ns_avatar_data, {itemId});
}

QFuture<QXmppPubSubManager::PublishItemResult> QXmppAvatarManager::publishAvatar(const QXmppAvatarInfo &avatarInfo, const QByteArray &avatarData)
{
    auto resultInterface = std::make_shared<QFutureInterface<QXmppPubSubManager::PublishItemResult>>(QFutureInterfaceBase::Started);

    const auto hash = QCryptographicHash::hash(avatarData, QCryptographicHash::Sha1).toHex();

    QXmppAvatarDataItem dataItem(avatarData);
    dataItem.setId(hash);

    const auto uploadFuture = client()->findExtension<QXmppPubSubManager>()
            ->publishPepItem(ns_avatar_data, dataItem);

    auto updatedAvatarInfo = avatarInfo;
    updatedAvatarInfo.setId(hash);
    QXmppAvatarMetadataItem meta;
    meta.setInfos({avatarInfo});

    auto *watcher = new QFutureWatcher<QXmppPubSubManager::PublishItemResult>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [=]() {
        const auto result = watcher->result();

        const auto metaFuture = client()->findExtension<QXmppPubSubManager>()
                ->publishPepItem(ns_avatar_metadata, meta);

        auto *metaWatcher = new QFutureWatcher<QXmppPubSubManager::PublishItemResult>(this);
        connect(metaWatcher, &QFutureWatcherBase::finished, this, [=]() {
            resultInterface->reportFinished();
        });
        metaWatcher->setFuture(metaFuture);
    });

    watcher->setFuture(uploadFuture);

    return resultInterface->future();
}
