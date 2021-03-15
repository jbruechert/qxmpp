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

#include "QXmppAvatar.h"
#include "QXmppConstants_p.h"

#include <QSharedPointer>

QXmppAvatarManager::QXmppAvatarManager()
    : QXmppClientExtension()
{
}

AvatarMetadataFuture QXmppAvatarManager::fetchAvatarMetadata(const QString &bareJid)
{
    auto *pubSubManager = client()->findExtension<QXmppPubSubManager>();
    return pubSubManager->requestItems<QXmppAvatarMetadataItem>(bareJid, ns_avatar_metadata);
}

AvatarDataFuture QXmppAvatarManager::fetchAvatar(const QString &bareJid)
{
    auto *pubSubManager = client()->findExtension<QXmppPubSubManager>();
    return pubSubManager->requestItems<QXmppAvatarDataItem>(bareJid, ns_avatar_data);
}
