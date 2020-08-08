/*
 * Copyright (C) 2008-2020 The QXmpp developers
 *
 * Author:
 *  Linus Jahn
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

#include "QXmppPubSubSubscription.h"

#include "QXmppConstants_p.h"
#include "QXmppUtils.h"

#include <QDateTime>
#include <QDomElement>
#include <QXmlStreamWriter>

static const QStringList SUBSCRIPTION_STATES = {
    QString(),
    QStringLiteral("none"),
    QStringLiteral("pending"),
    QStringLiteral("subscribed"),
    QStringLiteral("unconfigured"),
};

class QXmppPubSubSubscriptionPrivate : public QSharedData
{
public:
    QXmppPubSubSubscriptionPrivate(const QString &jid,
                                   const QString &node,
                                   const QString &subId,
                                   QXmppPubSubSubscription::State state,
                                   QXmppPubSubSubscription::ConfigurationSupport configurationSupport,
                                   const QDateTime &expiry);

    QString jid;
    QString node;
    QString subId;
    QDateTime expiry;
    QXmppPubSubSubscription::State state;
    QXmppPubSubSubscription::ConfigurationSupport configurationSupport;
};

///
/// Converts a subscription state to string.
///
QString QXmppPubSubSubscription::stateToString(State state)
{
    return SUBSCRIPTION_STATES.at(int(state));
}

///
/// Converts a string with a subscription state to the enum value.
///
QXmppPubSubSubscription::State QXmppPubSubSubscription::stateFromString(const QString &str)
{
    auto state = SUBSCRIPTION_STATES.indexOf(str);
    if (state == -1)
        return Invalid;
    return State(state);
}

QXmppPubSubSubscriptionPrivate::QXmppPubSubSubscriptionPrivate(const QString &jid,
                                                               const QString &node,
                                                               const QString &subId,
                                                               QXmppPubSubSubscription::State state,
                                                               QXmppPubSubSubscription::ConfigurationSupport configurationSupport,
                                                               const QDateTime &expiry)
    : jid(jid),
      node(node),
      subId(subId),
      expiry(expiry),
      state(state),
      configurationSupport(configurationSupport)
{
}

QXmppPubSubSubscription::QXmppPubSubSubscription(const QString &jid,
                                                 const QString &node,
                                                 const QString &subId,
                                                 State state,
                                                 ConfigurationSupport configurationSupport,
                                                 const QDateTime &expiry)
    : d(new QXmppPubSubSubscriptionPrivate(jid, node, subId, state, configurationSupport, expiry))
{
}

QXmppPubSubSubscription::QXmppPubSubSubscription(const QXmppPubSubSubscription &) = default;

QXmppPubSubSubscription::~QXmppPubSubSubscription() = default;

QXmppPubSubSubscription &QXmppPubSubSubscription::operator=(const QXmppPubSubSubscription &) = default;

QString QXmppPubSubSubscription::jid() const
{
    return d->jid;
}

void QXmppPubSubSubscription::setJid(const QString &jid)
{
    d->jid = jid;
}

QString QXmppPubSubSubscription::node() const
{
    return d->node;
}

void QXmppPubSubSubscription::setNode(const QString &node)
{
    d->node = node;
}

QString QXmppPubSubSubscription::subId() const
{
    return d->subId;
}

void QXmppPubSubSubscription::setSubId(const QString &subId)
{
    d->subId = subId;
}

QXmppPubSubSubscription::State QXmppPubSubSubscription::state() const
{
    return d->state;
}

void QXmppPubSubSubscription::setState(State state)
{
    d->state = state;
}

QDateTime QXmppPubSubSubscription::expiry() const
{
    return d->expiry;
}

void QXmppPubSubSubscription::setExpiry(const QDateTime &expiry)
{
    d->expiry = expiry;
}

///
/// Returns the availability of a subscription configuration.
///
QXmppPubSubSubscription::ConfigurationSupport QXmppPubSubSubscription::configurationSupport() const
{
    return d->configurationSupport;
}

///
/// Sets the availability of a subscription configuration.
///
void QXmppPubSubSubscription::setConfigurationSupport(ConfigurationSupport support)
{
    d->configurationSupport = support;
}

///
/// Returns whether a configuration of the subscription is possible.
///
bool QXmppPubSubSubscription::isConfigurationSupported() const
{
    return d->configurationSupport > Unavailable;
}

///
/// Returns whether configuration of the subscription required before event
/// notifications are going to be sent to the user.
///
bool QXmppPubSubSubscription::isConfigurationRequired() const
{
    return d->configurationSupport == Required || d->state == Unconfigured;
}

///
/// Returns true, if the element is a PubSub subscription element.
///
bool QXmppPubSubSubscription::isSubscription(const QDomElement &element)
{
    if (element.tagName() != QStringLiteral("subscription")) {
        return false;
    }

    if (element.hasAttribute(QStringLiteral("subscription"))) {
        const auto subStateStr = element.attribute(QStringLiteral("subscription"));
        if (stateFromString(subStateStr) == QXmppPubSubSubscription::Invalid)
            return false;
    }

    if (element.namespaceURI() == ns_pubsub || element.namespaceURI() == ns_pubsub_event) {
        return element.hasAttribute(QStringLiteral("jid"));
    }
    if (element.namespaceURI() == ns_pubsub_owner) {
        return element.hasAttribute(QStringLiteral("jid")) &&
            element.hasAttribute(QStringLiteral("subscription"));
    }
    return false;
}

/// \cond
void QXmppPubSubSubscription::parse(const QDomElement &element)
{
    bool isPubSub = element.namespaceURI() == ns_pubsub;
    bool isPubSubEvent = !isPubSub && element.namespaceURI() == ns_pubsub_event;

    d->jid = element.attribute(QStringLiteral("jid"));
    d->state = stateFromString(element.attribute(QStringLiteral("subscription")));

    if (isPubSub || isPubSubEvent) {
        d->node = element.attribute(QStringLiteral("node"));
        d->subId = element.attribute(QStringLiteral("subid"));

        if (isPubSubEvent) {
            d->expiry = QXmppUtils::datetimeFromString(element.attribute(QStringLiteral("expiry")));
        } else if (isPubSub) {
            auto options = element.firstChildElement(QStringLiteral("subscribe-options"));
            if (options.isNull()) {
                d->configurationSupport = Unavailable;
            } else {
                if (!options.firstChildElement(QStringLiteral("required")).isNull())
                    d->configurationSupport = Required;
                else
                    d->configurationSupport = Available;
            }
        }
    }
}

void QXmppPubSubSubscription::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("subscription"));

    // jid is required
    writer->writeAttribute(QStringLiteral("jid"), d->jid);
    helperToXmlAddAttribute(writer, QStringLiteral("node"), d->node);
    helperToXmlAddAttribute(writer, QStringLiteral("subid"), d->subId);
    helperToXmlAddAttribute(writer, QStringLiteral("subscription"), stateToString(d->state));
    if (!d->expiry.isNull() && d->expiry.isValid()) {
        writer->writeAttribute(QStringLiteral("expiry"),
                               QXmppUtils::datetimeToString(d->expiry));
    }

    if (d->configurationSupport > Unavailable) {
        writer->writeStartElement(QStringLiteral("subscribe-options"));
        if (d->configurationSupport == Required)
            writer->writeEmptyElement(QStringLiteral("required"));
        writer->writeEndElement();
    }

    writer->writeEndElement();
}
/// \endcond