/*
 * Copyright (C) 2008-2020 The QXmpp developers
 *
 * Author:
 *  Germán Márquez Mejía
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

#include "QXmppPubSubManager.h"

#include <QDomElement>
#include <QFutureInterface>

#include "QXmppClient.h"
#include "QXmppPubSubItem.h"
#include "QXmppPubSubEventManager.h"
#include "QXmppConstants_p.h"
#include "QXmppUtils.h"

///
/// \class QXmppPubSubManager
///
/// \brief The QXmppPubSubManager aims to provide publish-subscribe
/// functionality as specified in \xep{0060}: Publish-Subscribe (PubSub).
///
/// However, it currently only supports a few PubSub use cases but all of the
/// \xep{0060}: Personal Eventing Protocol (PEP) ones. PEP allows
/// a standard XMPP user account to function as a virtual PubSub service.
///
/// To make use of this manager, you need to instantiate it and load it into
/// the QXmppClient instance as follows:
///
/// \code
/// QXmppPubSubManager *manager = new QXmppPubSubManager;
/// client->addExtension(manager);
/// \endcode
///
/// \note To subscribe to PEP event notifications use the
/// QXmppClientExtension::discoveryFeatures method of your client extension
/// according to section 9.2 of \xep{0060}. For example:
/// \code
/// QStringList YourExtension::discoveryFeatures() const
/// {
///    return { "http://jabber.org/protocol/tune+notify" };
/// }
/// \endcode
///
/// \todo
///  - Item pagination:
///    https://xmpp.org/extensions/xep-0060.html#subscriber-retrieve-returnsome
///  - Requesting most recent items (max_items=2):
///    https://xmpp.org/extensions/xep-0060.html#subscriber-retrieve-requestrecent
///
/// \ingroup Managers
///
/// \since QXmpp 1.4
///

///
/// \typedef QXmppPubSubManager::Success
///
/// Type used to indicate the success case in an std::variant based return
/// value.
///

///
/// \typedef QXmppPubSubManager::Result
///
/// Result of a generic request without a return value. Contains Success in case
/// everything went well. If the returned IQ contained an error a
/// QXmppStanza::Error is reported. If sending the IQ was unsuccessful
/// QXmpp::PacketState is reported.
///

///
/// \typedef QXmppPubSubManager::InstantNodeResult
///
/// Contains the name of the new node (QString) or the returned IQ error
/// (QXmppStanza::Error) or a packet sending error (QXmpp::PacketState).
///

///
/// \typedef QXmppPubSubManager::ItemResult
///
/// Contains the item if it has been found (std::optional<T>) or the returned IQ
/// error (QXmppStanza::Error) or a packet sending error (QXmpp::PacketState).
///

///
/// \typedef QXmppPubSubManager::ItemsResult
///
/// Contains all items that have been found (QList<T>) or the returned IQ error
/// (QXmppStanza::Error) or a packet sending error (QXmpp::PacketState).
///

///
/// \typedef QXmppPubSubManager::PublishItemResult
///
/// Contains the ID of the item, if no ID was set in the request (QString) or
/// the returned IQ error (QXmppStanza::Error) or a packet sending error
/// (QXmpp::PacketState).
///

///
/// \typedef QXmppPubSubManager::PublishItemsResult
///
/// Contains the IDs of the items, if no IDs were set in the request
/// (QVector<QString>) or the returned IQ error (QXmppStanza::Error) or a packet
/// sending error (QXmpp::PacketState).
///

///
/// Default constructor.
///
QXmppPubSubManager::QXmppPubSubManager()
{
}

///
/// Default destructor.
///
QXmppPubSubManager::~QXmppPubSubManager()
{
}

///
/// Creates an empty pubsub node with the default configuration.
///
/// Calling this before QXmppPubSubManager::publishItems is usually not
/// necessary when publishing to a node for the first time if the service
/// suppports the auto-create feature (Section 7.1.4 of \xep{0060}).
///
/// \param jid Jabber ID of the entity hosting the pubsub service
/// \param nodeName the name of the node to be created
/// \return
///
auto QXmppPubSubManager::createNode(const QString &jid, const QString &nodeName) -> QFuture<Result>
{
    QXmppPubSubIq request;
    request.setType(QXmppIq::Set);
    request.setQueryType(QXmppPubSubIq<>::Create);
    request.setQueryNode(nodeName);
    request.setTo(jid);

    return sendGenericIq(request);
}

///
/// Creates an instant pubsub node with the default configuration.
///
/// The pubsub service automatically generates a random node name. On success
/// it is returned via the QFuture.
///
/// \param jid Jabber ID of the entity hosting the pubsub service
/// \param nodeName the name of the node to be created
/// \return
///
QFuture<QXmppPubSubManager::InstantNodeResult> QXmppPubSubManager::createInstantNode(const QString &jid)
{
    QXmppPubSubIq request;
    request.setType(QXmppIq::Set);
    request.setQueryType(QXmppPubSubIq<>::Create);
    request.setTo(jid);

    auto resultInterface = QSharedPointer<QFutureInterface<InstantNodeResult>>(
        new QFutureInterface<InstantNodeResult>(QFutureInterfaceBase::Started));
    auto sendFuture = client()->sendIq(request);

    auto *watcher = new QFutureWatcher<QXmppClient::IqResult>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [=](){
        auto result = watcher->result();
        if (auto element = std::get_if<QDomElement>(&result)) {
            QXmppPubSubIq iq;
            iq.parse(*element);

            switch (iq.type()) {
            case QXmppIq::Result:
                // report generated node name
                resultInterface->reportResult(iq.queryNode());
                break;
            default:
                resultInterface->reportResult(iq.error());
            }
        } else if (const auto *error = std::get_if<QXmpp::PacketState>(&result)) {
            resultInterface->reportResult(*error);
        }
        resultInterface->reportFinished();
        watcher->deleteLater();
    });
    watcher->setFuture(sendFuture);

    return resultInterface->future();
}

///
/// Deletes a pubsub node.
///
/// \param jid Jabber ID of the entity hosting the pubsub service
/// \param nodeName the name of the node to delete along with all of its items
/// \return
///
auto QXmppPubSubManager::deleteNode(const QString &jid, const QString &nodeName) -> QFuture<Result>
{
    QXmppPubSubIq request;
    request.setType(QXmppIq::Set);
    request.setQueryType(QXmppPubSubIq<>::Delete);
    request.setQueryNode(nodeName);
    request.setTo(jid);

    return sendGenericIq(request);
}

///
/// Deletes an item from a pubsub node.
///
/// \param jid Jabber ID of the entity hosting the pubsub service
/// \param nodeName the name of the node to delete the item from
/// \param itemId the ID of the item to delete
/// \return
///
auto QXmppPubSubManager::retractItem(const QString &jid, const QString &nodeName, const QString &itemId) -> QFuture<Result>
{
    QXmppPubSubIq request;
    request.setType(QXmppIq::Set);
    request.setQueryType(QXmppPubSubIq<>::Retract);
    request.setQueryNode(nodeName);
    request.setItems({QXmppPubSubItem(itemId)});
    request.setTo(jid);

    return sendGenericIq(request);
}

///
/// Purges all items from a node.
///
/// \param jid Jabber ID of the entity hosting the pubsub service
/// \param nodeName the name of the PEP node to delete along with all of its
/// items
/// \return
///
auto QXmppPubSubManager::purgeItems(const QString &jid, const QString &nodeName) -> QFuture<Result>
{
    QXmppPubSubIq request;
    request.setType(QXmppIq::Set);
    request.setQueryType(QXmppPubSubIq<>::Purge);
    request.setQueryNode(nodeName);
    request.setTo(jid);

    return sendGenericIq(request);
}

///
/// Creates an empty PEP node with the default configuration.
///
/// This is a convenience method equivalent to calling
/// QXmppPubSubManager::createNode on the current account's bare JID.
///
/// Calling this before QXmppPubSubManager::publishPepItems is usually not
/// necessary when publishing to a node for the first time if the service
/// suppports the auto-create feature (Section 7.1.4 of XEP-0060).
///
/// \param nodeName the name of the PEP node to be created
/// \return
///
auto QXmppPubSubManager::createPepNode(const QString &nodeName) -> QFuture<Result>
{
    return createNode(client()->configuration().jidBare(), nodeName);
}

///
/// Deletes a PEP node.
///
/// This is a convenience method equivalent to calling
/// QXmppPubSubManager::deleteNode on the current account's bare JID.
///
/// \param nodeName the name of the PEP node to delete along with all of its
/// items
/// \return
///
auto QXmppPubSubManager::deletePepNode(const QString &nodeName) -> QFuture<Result>
{
    return deleteNode(client()->configuration().jidBare(), nodeName);
}

///
/// Deletes an item from a PEP node.
///
/// This is a convenience method equivalent to calling
/// QXmppPubSubManager::retractItem on the current account's bare JID.
///
/// \param nodeName the name of the PEP node to delete the item from
/// \param itemId the ID of the item to delete
/// \return
///
auto QXmppPubSubManager::retractPepItem(const QString &nodeName, const QString &itemId) -> QFuture<Result>
{
    return retractItem(client()->configuration().jidBare(), nodeName, itemId);
}

///
/// Purges all items from a PEP node.
///
/// This is a convenience method equivalent to calling
/// QXmppPubSubManager::purgeItems on the current account's bare JID.
///
/// \param nodeName the name of the PEP node to delete along with all of its
/// items
/// \return
///
auto QXmppPubSubManager::purgePepItems(const QString &nodeName) -> QFuture<Result>
{
    return purgeItems(client()->configuration().jidBare(), nodeName);
}

/// \cond
bool QXmppPubSubManager::handleStanza(const QDomElement &element)
{
    for (auto child = element.firstChildElement("event");
         !child.isNull();
         child = child.nextSiblingElement("event")) {
        if (child.namespaceURI() == ns_pubsub_event) {
            const auto service = element.attribute("from");
            const auto node = child.attribute("node");

            for (auto *extension : client()->extensions()) {
                if (auto *eventManager = qobject_cast<QXmppPubSubEventManager*>(extension)) {
                    if (eventManager->handlePubSubEvent(element, service, node)) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

auto QXmppPubSubManager::sendGenericIq(const QXmppIq &iq) -> QFuture<Result>
{
    auto resultInterface = QSharedPointer<QFutureInterface<Result>>(new QFutureInterface<Result>(QFutureInterfaceBase::Started));
    auto sendFuture = client()->sendIq(iq);

    auto *watcher = new QFutureWatcher<QXmppClient::IqResult>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [=](){
        auto result = watcher->result();
        if (auto element = std::get_if<QDomElement>(&result)) {
            QXmppIq iq;
            iq.parse(*element);
            if (iq.type() == QXmppIq::Result) {
                resultInterface->reportResult(Success());
            } else {
                resultInterface->reportResult(iq.error());
            }
        } else if (const auto *error = std::get_if<QXmpp::PacketState>(&result)) {
            resultInterface->reportResult(*error);
        }
        resultInterface->reportFinished();
        watcher->deleteLater();
    });
    watcher->setFuture(sendFuture);

    return resultInterface->future();
}

QXmppPubSubIq<> QXmppPubSubManager::requestItemsIq(const QString &jid, const QString &nodeName, const QStringList &itemIds)
{
    QXmppPubSubIq request;
    request.setTo(jid);
    request.setType(QXmppIq::Get);
    request.setQueryType(QXmppPubSubIqBase::Items);
    request.setQueryNode(nodeName);

    if (!itemIds.isEmpty()) {
        QList<QXmppPubSubItem> items;
        items.reserve(itemIds.size());
        for (const auto &id : itemIds) {
            items << QXmppPubSubItem(id);
        }
        request.setItems(items);
    }
    return request;
}

auto QXmppPubSubManager::publishItem(QXmppPubSubIqBase &&request) -> QFuture<PublishItemResult>
{
    request.setType(QXmppIq::Set);
    request.setQueryType(QXmppPubSubIqBase::Publish);

    auto resultInterface = QSharedPointer<QFutureInterface<PublishItemResult>>(new QFutureInterface<PublishItemResult>(QFutureInterfaceBase::Started));
    auto sendFuture = client()->sendIq(request);

    auto *watcher = new QFutureWatcher<QXmppClient::IqResult>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [=](){
        auto result = watcher->result();
        if (auto element = std::get_if<QDomElement>(&result)) {
            QXmppPubSubIq<> resultIq;
            resultIq.parse(*element);

            switch (resultIq.type()) {
            case QXmppIq::Result:
                if (!resultIq.items().isEmpty()) {
                    const auto items = resultIq.items();
                    resultInterface->reportResult(items.first().id());
                } else {
                    resultInterface->reportResult(QString());
                }
                break;
            default:
                resultInterface->reportResult(resultIq.error());
            }
        } else if (const auto *error = std::get_if<QXmpp::PacketState>(&result)) {
            resultInterface->reportResult(*error);
        }
        resultInterface->reportFinished();
        watcher->deleteLater();
    });
    watcher->setFuture(sendFuture);

    return resultInterface->future();
}

auto QXmppPubSubManager::publishItems(QXmppPubSubIqBase &&request) -> QFuture<PublishItemsResult>
{
    request.setType(QXmppIq::Set);
    request.setQueryType(QXmppPubSubIqBase::Publish);

    auto resultInterface = QSharedPointer<QFutureInterface<PublishItemsResult>>(new QFutureInterface<PublishItemsResult>(QFutureInterfaceBase::Started));
    auto sendFuture = client()->sendIq(request);

    auto *watcher = new QFutureWatcher<QXmppClient::IqResult>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [=](){
        auto result = watcher->result();
        if (auto element = std::get_if<QDomElement>(&result)) {
            QXmppPubSubIq<> resultIq;
            resultIq.parse(*element);

            switch (resultIq.type()) {
            case QXmppIq::Result:
                if (!resultIq.items().isEmpty()) {
                    const auto items = resultIq.items();
                    QVector<QString> ids(items.size());
                    std::transform(items.cbegin(), items.cend(), ids.begin(),
                                   [](const QXmppPubSubItem &item) {
                        return item.id();
                    });
                    resultInterface->reportResult(ids);
                } else {
                    resultInterface->reportResult({});
                }
                break;
            default:
                resultInterface->reportResult(resultIq.error());
            }
        } else if (const auto *error = std::get_if<QXmpp::PacketState>(&result)) {
            resultInterface->reportResult(*error);
        }
        resultInterface->reportFinished();
        watcher->deleteLater();
    });
    watcher->setFuture(sendFuture);

    return resultInterface->future();
}
/// \endcond
