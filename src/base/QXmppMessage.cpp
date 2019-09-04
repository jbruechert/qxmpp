/*
 * Copyright (C) 2008-2019 The QXmpp developers
 *
 * Authors:
 *  Manjeet Dahiya
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

#include <QDomElement>
#include <QTextStream>
#include <QXmlStreamWriter>
#include <QPair>

#include "QXmppConstants_p.h"
#include "QXmppMessage.h"
#include "QXmppUtils.h"

static const char* chat_states[] = {
    "",
    "active",
    "inactive",
    "gone",
    "composing",
    "paused",
};

static const char* message_types[] = {
    "error",
    "normal",
    "chat",
    "groupchat",
    "headline"
};

static const char* marker_types[] = {
    "",
    "received",
    "displayed",
    "acknowledged"
};

static const char *ns_xhtml = "http://www.w3.org/1999/xhtml";

enum StampType
{
    LegacyDelayedDelivery,  // XEP-0091: Legacy Delayed Delivery
    DelayedDelivery         // XEP-0203: Delayed Delivery
};

class QXmppMessagePrivate : public QSharedData
{
public:
    QXmppMessage::Type type;
    QDateTime stamp;
    StampType stampType;
    QXmppMessage::State state;

    bool attentionRequested;
    QString body;
    QString subject;
    QString thread;

    // XEP-0071: XHTML-IM
    QString xhtml;

    // Request message receipt as per XEP-0184.
    QString receiptId;
    bool receiptRequested;

    // XEP-0249: Direct MUC Invitations
    QString mucInvitationJid;
    QString mucInvitationPassword;
    QString mucInvitationReason;

    // XEP-0333: Chat Markers
    bool markable;
    QXmppMessage::Marker marker;
    QString markedId;
    QString markedThread;

    // XEP-0280: Message Carbons
    bool privatemsg;

    // XEP-0066: Out of Band Data
    QString outOfBandUrl;

    // XEP-0308: Last Message Correction
    QString replaceId;

    // XEP-0369: Mediated Information eXchange (MIX)
    QString mixUserJid;
    QString mixUserNick;

    // XEP-0382: Spoiler messages
    bool isSpoiler = false;
    QString spoilerHint;
};

/// Constructs a QXmppMessage.
///
/// \param from
/// \param to
/// \param body
/// \param thread

QXmppMessage::QXmppMessage(const QString& from, const QString& to, const
                         QString& body, const QString& thread)
    : QXmppStanza(from, to)
    , d(new QXmppMessagePrivate)
{
    d->type = Chat;
    d->stampType = DelayedDelivery;
    d->state = None;
    d->attentionRequested = false;
    d->body = body;
    d->thread = thread;
    d->receiptRequested = false;

    d->markable = false;
    d->marker = NoMarker;

    d->privatemsg = false;
}

/// Constructs a copy of \a other.

QXmppMessage::QXmppMessage(const QXmppMessage &other)
    : QXmppStanza(other)
    , d(other.d)
{
}

QXmppMessage::~QXmppMessage()
{

}

/// Assigns \a other to this message.

QXmppMessage& QXmppMessage::operator=(const QXmppMessage &other)
{
    QXmppStanza::operator=(other);
    d = other.d;
    return *this;
}

/// Returns the message's body.
///

QString QXmppMessage::body() const
{
    return d->body;
}

/// Sets the message's body.
///
/// \param body

void QXmppMessage::setBody(const QString& body)
{
    d->body = body;
}

/// Returns true if the user's attention is requested, as defined
/// by XEP-0224: Attention.

bool QXmppMessage::isAttentionRequested() const
{
    return d->attentionRequested;
}

/// Sets whether the user's attention is requested, as defined
/// by XEP-0224: Attention.
///
/// \a param requested

void QXmppMessage::setAttentionRequested(bool requested)
{
    d->attentionRequested = requested;
}

/// Returns true if a delivery receipt is requested, as defined
/// by XEP-0184: Message Delivery Receipts.

bool QXmppMessage::isReceiptRequested() const
{
    return d->receiptRequested;
}

/// Sets whether a delivery receipt is requested, as defined
/// by XEP-0184: Message Delivery Receipts.
///
/// \a param requested

void QXmppMessage::setReceiptRequested(bool requested)
{
    d->receiptRequested = requested;
    if (requested && id().isEmpty())
        generateAndSetNextId();
}

/// If this message is a delivery receipt, returns the ID of the
/// original message.

QString QXmppMessage::receiptId() const
{
    return d->receiptId;
}

/// Make this message a delivery receipt for the message with
/// the given \a id.

void QXmppMessage::setReceiptId(const QString &id)
{
    d->receiptId = id;
}

/// Returns the JID for a multi-user chat direct invitation as defined
/// by XEP-0249: Direct MUC Invitations.

QString QXmppMessage::mucInvitationJid() const
{
    return d->mucInvitationJid;
}

/// Sets the JID for a multi-user chat direct invitation as defined
/// by XEP-0249: Direct MUC Invitations.

void QXmppMessage::setMucInvitationJid(const QString &jid)
{
    d->mucInvitationJid = jid;
}

/// Returns the password for a multi-user chat direct invitation as defined
/// by XEP-0249: Direct MUC Invitations.

QString QXmppMessage::mucInvitationPassword() const
{
    return d->mucInvitationPassword;
}

/// Sets the \a password for a multi-user chat direct invitation as defined
/// by XEP-0249: Direct MUC Invitations.

void QXmppMessage::setMucInvitationPassword(const QString &password)
{
    d->mucInvitationPassword = password;
}

/// Returns the reason for a multi-user chat direct invitation as defined
/// by XEP-0249: Direct MUC Invitations.

QString QXmppMessage::mucInvitationReason() const
{
    return d->mucInvitationReason;
}

/// Sets the \a reason for a multi-user chat direct invitation as defined
/// by XEP-0249: Direct MUC Invitations.

void QXmppMessage::setMucInvitationReason(const QString &reason)
{
    d->mucInvitationReason = reason;
}

/// Returns the message's type.
///

QXmppMessage::Type QXmppMessage::type() const
{
    return d->type;
}

/// Sets the message's type.
///
/// \param type

void QXmppMessage::setType(QXmppMessage::Type type)
{
    d->type = type;
}

/// Returns the message's timestamp (if any).

QDateTime QXmppMessage::stamp() const
{
    return d->stamp;
}

/// Sets the message's timestamp.
///
/// \param stamp

void QXmppMessage::setStamp(const QDateTime &stamp)
{
    d->stamp = stamp;
}

/// Returns the message's chat state.
///

QXmppMessage::State QXmppMessage::state() const
{
    return d->state;
}

/// Sets the message's chat state.
///
/// \param state

void QXmppMessage::setState(QXmppMessage::State state)
{
    d->state = state;
}

/// Returns the message's subject.
///

QString QXmppMessage::subject() const
{
    return d->subject;
}

/// Sets the message's subject.
///
/// \param subject

void QXmppMessage::setSubject(const QString& subject)
{
    d->subject = subject;
}

/// Returns the message's thread.

QString QXmppMessage::thread() const
{
    return d->thread;
}

/// Sets the message's thread.
///
/// \param thread

void QXmppMessage::setThread(const QString& thread)
{
    d->thread = thread;
}

/// Returns the message's XHTML body as defined by
/// XEP-0071: XHTML-IM.

QString QXmppMessage::xhtml() const
{
    return d->xhtml;
}

/// Sets the message's XHTML body as defined by
/// XEP-0071: XHTML-IM.

void QXmppMessage::setXhtml(const QString &xhtml)
{
    d->xhtml = xhtml;
}

namespace
{
    static QList<QPair<QString, QString> > knownMessageSubelems()
    {
        QList<QPair<QString, QString> > result;
        result << qMakePair(QStringLiteral("body"), QString())
               << qMakePair(QStringLiteral("subject"), QString())
               << qMakePair(QStringLiteral("thread"), QString())
               << qMakePair(QStringLiteral("html"), QString())
               << qMakePair(QStringLiteral("received"), QString(ns_message_receipts))
               << qMakePair(QStringLiteral("replace"), QString(ns_message_correct))
               << qMakePair(QStringLiteral("request"), QString())
               << qMakePair(QStringLiteral("delay"), QString())
               << qMakePair(QStringLiteral("attention"), QString())
               << qMakePair(QStringLiteral("addresses"), QString())
               << qMakePair(QStringLiteral("private"), QString(ns_carbons));
        for (int i = QXmppMessage::Active; i <= QXmppMessage::Paused; i++)
            result << qMakePair(QString(chat_states[i]), QString());
        return result;
    }
}

/// Returns true if a message is markable, as defined
/// XEP-0333: Chat Markers.

bool QXmppMessage::isMarkable() const
{
    return d->markable;
}

/// Sets if the message is markable, as defined
/// XEP-0333: Chat Markers.

void QXmppMessage::setMarkable(const bool markable)
{
    d->markable = markable;
}

/// Returns the message's marker id, as defined
/// XEP-0333: Chat Markers.

QString QXmppMessage::markedId() const
{
    return d->markedId;
}

/// Sets the message's marker id, as defined
/// XEP-0333: Chat Markers.

void QXmppMessage::setMarkerId(const QString &markerId)
{
    d->markedId = markerId;
}

/// Returns the message's marker thread, as defined
/// XEP-0333: Chat Markers.

QString QXmppMessage::markedThread() const
{
    return d->markedThread;
}

/// Sets the message's marked thread, as defined
/// XEP-0333: Chat Markers.

void QXmppMessage::setMarkedThread(const QString &markedThread)
{
    d->markedThread = markedThread;
}

/// Returns the message's marker, as defined
/// XEP-0333: Chat Markers.

QXmppMessage::Marker QXmppMessage::marker() const
{
    return d->marker;
}

/// Sets the message's marker, as defined
/// XEP-0333: Chat Markers

void QXmppMessage::setMarker(const Marker marker)
{
    d->marker = marker;
}

/// Returns if the message is marked with a <private> tag,
/// in which case it will not be forwarded to other resources
/// according to XEP-0280: Message Carbons.

bool QXmppMessage::isPrivate() const
{
    return d->privatemsg;
}

/// If true is passed, the message is marked with a <private> tag,
/// in which case it will not be forwarded to other resources
/// according to XEP-0280: Message Carbons.

void QXmppMessage::setPrivate(const bool priv)
{
    d->privatemsg = priv;
}

/// Indicates if the QXmppStanza is a stanza in the XMPP sense (i. e. a message,
/// iq or presence)

bool QXmppMessage::isXmppStanza() const
{
    return true;
}

/// Returns a possibly attached URL from XEP-0066: Out of Band Data

QString QXmppMessage::outOfBandUrl() const
{
    return d->outOfBandUrl;
}

/// Sets the attached URL for XEP-0066: Out of Band Data

void QXmppMessage::setOutOfBandUrl(const QString &url)
{
    d->outOfBandUrl = url;
}

/// Returns the message id to replace with this message as used in XEP-0308:
/// Last Message Correction. If the returned string is empty, this message is
/// not replacing another.

QString QXmppMessage::replaceId() const
{
    return d->replaceId;
}

/// Sets the message id to replace with this message as in XEP-0308: Last
/// Message Correction.

void QXmppMessage::setReplaceId(const QString &replaceId)
{
    d->replaceId = replaceId;
}

/// Returns the actual JID of a MIX channel participant.

QString QXmppMessage::mixUserJid() const
{
    return d->mixUserJid;
}

/// Sets the actual JID of a MIX channel participant.

void QXmppMessage::setMixUserJid(const QString& mixUserJid)
{
    d->mixUserJid = mixUserJid;
}

/// Returns the MIX participant's nickname.

QString QXmppMessage::mixUserNick() const
{
    return d->mixUserNick;
}

/// Sets the MIX participant's nickname.

void QXmppMessage::setMixUserNick(const QString& mixUserNick)
{
    d->mixUserNick = mixUserNick;
}

/// Returns true, if this is a spoiler message according to XEP-0382: Spoiler
/// messages. The spoiler hint however can still be empty.
///
/// A spoiler message's content should not be visible to the user by default.

bool QXmppMessage::isSpoiler() const
{
    return d->isSpoiler;
}

/// Sets whether this is a spoiler message as specified in XEP-0382: Spoiler
/// messages.
///
/// The content of spoiler messages will not be displayed by default to the
/// user. However, clients not supporting spoiler messages will still display
/// the content as usual.

void QXmppMessage::setIsSpoiler(bool isSpoiler)
{
    d->isSpoiler = isSpoiler;
}

/// Returns the spoiler hint as specified in XEP-0382: Spoiler messages.
///
/// The hint may be empty, even if isSpoiler is true.

QString QXmppMessage::spoilerHint() const
{
    return d->spoilerHint;
}

/// Sets a spoiler hint for XEP-0382: Spoiler messages. If the spoiler hint
/// is not empty, isSpoiler will be set to true.
///
/// A spoiler hint is optional for spoiler messages.
///
/// Keep in mind that the spoiler hint is not displayed at all by clients not
/// supporting spoiler messages.

void QXmppMessage::setSpoilerHint(const QString &spoilerHint)
{
    d->spoilerHint = spoilerHint;
    if (!spoilerHint.isEmpty())
        d->isSpoiler = true;
}

/// \cond
void QXmppMessage::parse(const QDomElement &element)
{
    QXmppStanza::parse(element);

    const QString type = element.attribute(QStringLiteral("type"));
    d->type = Normal;
    for (int i = Error; i <= Headline; i++) {
        if (type == message_types[i]) {
            d->type = static_cast<Type>(i);
            break;
        }
    }

    d->body = element.firstChildElement(QStringLiteral("body")).text();
    d->subject = element.firstChildElement(QStringLiteral("subject")).text();
    d->thread = element.firstChildElement(QStringLiteral("thread")).text();

    // chat states
    for (int i = Active; i <= Paused; i++)
    {
        QDomElement stateElement = element.firstChildElement(chat_states[i]);
        if (!stateElement.isNull() &&
            stateElement.namespaceURI() == ns_chat_states)
        {
            d->state = static_cast<QXmppMessage::State>(i);
            break;
        }
    }

    // XEP-0071: XHTML-IM
    QDomElement htmlElement = element.firstChildElement(QStringLiteral("html"));
    if (!htmlElement.isNull() && htmlElement.namespaceURI() == ns_xhtml_im) {
        QDomElement bodyElement = htmlElement.firstChildElement(QStringLiteral("body"));
        if (!bodyElement.isNull() && bodyElement.namespaceURI() == ns_xhtml) {
            QTextStream stream(&d->xhtml, QIODevice::WriteOnly);
            bodyElement.save(stream, 0);

            d->xhtml = d->xhtml.mid(d->xhtml.indexOf('>') + 1);
            d->xhtml.replace(QLatin1String(" xmlns=\"http://www.w3.org/1999/xhtml\""), QLatin1String(""));
            d->xhtml.replace(QLatin1String("</body>"), QLatin1String(""));
            d->xhtml = d->xhtml.trimmed();
        }
    }

    // XEP-0184: Message Delivery Receipts
    QDomElement receivedElement = element.firstChildElement(QStringLiteral("received"));
    if (!receivedElement.isNull() && receivedElement.namespaceURI() == ns_message_receipts) {
        d->receiptId = receivedElement.attribute(QStringLiteral("id"));

        // compatibility with old-style XEP
        if (d->receiptId.isEmpty())
            d->receiptId = id();
    } else {
        d->receiptId = QString();
    }
    d->receiptRequested = element.firstChildElement(QStringLiteral("request")).namespaceURI() == ns_message_receipts;

    // XEP-0203: Delayed Delivery
    QDomElement delayElement = element.firstChildElement(QStringLiteral("delay"));
    if (!delayElement.isNull() && delayElement.namespaceURI() == ns_delayed_delivery)
    {
        const QString str = delayElement.attribute(QStringLiteral("stamp"));
        d->stamp = QXmppUtils::datetimeFromString(str);
        d->stampType = DelayedDelivery;
    }

    // XEP-0224: Attention
    d->attentionRequested = element.firstChildElement(QStringLiteral("attention")).namespaceURI() == ns_attention;

    // XEP-0333: Chat Markers
    QDomElement markableElement = element.firstChildElement(QStringLiteral("markable"));
    if (!markableElement.isNull())
    {
        d->markable = true;
    }
    // check for all the marker types
    QDomElement chatStateElement;
    QXmppMessage::Marker marker = QXmppMessage::NoMarker;
    for (int i = Received; i <= Acknowledged; i++)
    {
        chatStateElement = element.firstChildElement(marker_types[i]);
        if (!chatStateElement.isNull() &&
            chatStateElement.namespaceURI() == ns_chat_markers)
        {
            marker = static_cast<QXmppMessage::Marker>(i);
            break;
        }
    }
    // if marker is present, check it's the right ns
    if (!chatStateElement.isNull())
    {
        if (chatStateElement.namespaceURI() == ns_chat_markers)
        {
            d->marker = marker;
            d->markedId = chatStateElement.attribute(QStringLiteral("id"), QString());
            d->markedThread = chatStateElement.attribute(QStringLiteral("thread"), QString());
        }
    }

    // XEP-0280: Message Carbons
    QDomElement privateElement = element.firstChildElement(QStringLiteral("private"));
    if (!privateElement.isNull())
        d->privatemsg = true;

    // XEP-0308: Last Message Correction
    QDomElement replaceElement = element.firstChildElement(QStringLiteral("replace"));
    if (!replaceElement.isNull() && replaceElement.namespaceURI() == ns_message_correct)
        d->replaceId = replaceElement.attribute(QStringLiteral("id"));

    const QList<QPair<QString, QString> > &knownElems = knownMessageSubelems();

    QXmppElementList extensions;
    QDomElement xElement = element.firstChildElement();
    while (!xElement.isNull())
    {
        if (xElement.tagName() == QLatin1String("x"))
        {
            if (xElement.namespaceURI() == ns_legacy_delayed_delivery)
            {
                // if XEP-0203 exists, XEP-0091 has no need to parse because XEP-0091 is no more standard protocol)
                if (d->stamp.isNull())
                {
                    // XEP-0091: Legacy Delayed Delivery
                    const QString str = xElement.attribute(QStringLiteral("stamp"));
                    d->stamp = QDateTime::fromString(str, QStringLiteral("yyyyMMddThh:mm:ss"));
                    d->stamp.setTimeSpec(Qt::UTC);
                    d->stampType = LegacyDelayedDelivery;
                }
            } else if (xElement.namespaceURI() == ns_conference) {
                // XEP-0249: Direct MUC Invitations
                d->mucInvitationJid = xElement.attribute(QStringLiteral("jid"));
                d->mucInvitationPassword = xElement.attribute(QStringLiteral("password"));
                d->mucInvitationReason = xElement.attribute(QStringLiteral("reason"));
            } else if (xElement.namespaceURI() == ns_oob) {
                // XEP-0066: Out of Band Data
                d->outOfBandUrl = xElement.firstChildElement(QStringLiteral("url")).text();
            }
            else {
                extensions << QXmppElement(xElement);
            }
        // XEP-0369: Mediated Information eXchange (MIX)
        } else if (xElement.tagName() == QLatin1String("mix") && xElement.namespaceURI() == ns_mix) {
            d->mixUserJid = xElement.firstChildElement(QStringLiteral("jid")).text();
            d->mixUserNick = xElement.firstChildElement(QStringLiteral("nick")).text();
        // XEP-0382: Spoiler messages
        } else if (xElement.tagName() == QLatin1String("spoiler") && xElement.namespaceURI() == ns_spoiler) {
            d->isSpoiler = true;
            d->spoilerHint = xElement.text();
        } else if (!knownElems.contains(qMakePair(xElement.tagName(), xElement.namespaceURI())) &&
                   !knownElems.contains(qMakePair(xElement.tagName(), QString()))) {
            // other extensions
            extensions << QXmppElement(xElement);
        }
        xElement = xElement.nextSiblingElement();
    }
    setExtensions(extensions);
}

void QXmppMessage::toXml(QXmlStreamWriter *xmlWriter) const
{
    xmlWriter->writeStartElement(QStringLiteral("message"));
    helperToXmlAddAttribute(xmlWriter, QStringLiteral("xml:lang"), lang());
    helperToXmlAddAttribute(xmlWriter, QStringLiteral("id"), id());
    helperToXmlAddAttribute(xmlWriter, QStringLiteral("to"), to());
    helperToXmlAddAttribute(xmlWriter, QStringLiteral("from"), from());
    helperToXmlAddAttribute(xmlWriter, "type", message_types[d->type]);
    if (!d->subject.isEmpty())
        helperToXmlAddTextElement(xmlWriter, QStringLiteral("subject"), d->subject);
    if (!d->body.isEmpty())
        helperToXmlAddTextElement(xmlWriter, QStringLiteral("body"), d->body);
    if (!d->thread.isEmpty())
        helperToXmlAddTextElement(xmlWriter, QStringLiteral("thread"), d->thread);
    error().toXml(xmlWriter);

    // chat states
    if (d->state > None && d->state <= Paused)
    {
        xmlWriter->writeStartElement(chat_states[d->state]);
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_chat_states);
        xmlWriter->writeEndElement();
    }

    // XEP-0071: XHTML-IM
    if (!d->xhtml.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("html"));
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_xhtml_im);
        xmlWriter->writeStartElement(QStringLiteral("body"));
        xmlWriter->writeAttribute("xmlns", ns_xhtml);
        xmlWriter->writeCharacters(QLatin1String(""));
        xmlWriter->device()->write(d->xhtml.toUtf8());
        xmlWriter->writeEndElement();
        xmlWriter->writeEndElement();
    }

    // time stamp
    if (d->stamp.isValid())
    {
        QDateTime utcStamp = d->stamp.toUTC();
        if (d->stampType == DelayedDelivery)
        {
            // XEP-0203: Delayed Delivery
            xmlWriter->writeStartElement(QStringLiteral("delay"));
            xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_delayed_delivery);
            helperToXmlAddAttribute(xmlWriter, QStringLiteral("stamp"), QXmppUtils::datetimeToString(utcStamp));
            xmlWriter->writeEndElement();
        } else {
            // XEP-0091: Legacy Delayed Delivery
            xmlWriter->writeStartElement(QStringLiteral("x"));
            xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_legacy_delayed_delivery);
            helperToXmlAddAttribute(xmlWriter, QStringLiteral("stamp"), utcStamp.toString(QStringLiteral("yyyyMMddThh:mm:ss")));
            xmlWriter->writeEndElement();
        }
    }

    // XEP-0184: Message Delivery Receipts
    if (!d->receiptId.isEmpty()) {
        xmlWriter->writeStartElement("received");
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_message_receipts);
        xmlWriter->writeAttribute(QStringLiteral("id"), d->receiptId);
        xmlWriter->writeEndElement();
    }
    if (d->receiptRequested) {
        xmlWriter->writeStartElement("request");
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_message_receipts);
        xmlWriter->writeEndElement();
    }

    // XEP-0224: Attention
    if (d->attentionRequested) {
        xmlWriter->writeStartElement(QStringLiteral("attention"));
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_attention);
        xmlWriter->writeEndElement();
    }

    // XEP-0249: Direct MUC Invitations
    if (!d->mucInvitationJid.isEmpty()) {
        xmlWriter->writeStartElement("x");
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_conference);
        xmlWriter->writeAttribute(QStringLiteral("jid"), d->mucInvitationJid);
        if (!d->mucInvitationPassword.isEmpty())
            xmlWriter->writeAttribute(QStringLiteral("password"), d->mucInvitationPassword);
        if (!d->mucInvitationReason.isEmpty())
            xmlWriter->writeAttribute(QStringLiteral("reason"), d->mucInvitationReason);
        xmlWriter->writeEndElement();
    }

    // XEP-0333: Chat Markers
    if (d->markable) {
        xmlWriter->writeStartElement(QStringLiteral("markable"));
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_chat_markers);
        xmlWriter->writeEndElement();
    }
    if (d->marker != NoMarker) {
        xmlWriter->writeStartElement(marker_types[d->marker]);
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_chat_markers);
        xmlWriter->writeAttribute(QStringLiteral("id"), d->markedId);
        if (!d->markedThread.isNull() && !d->markedThread.isEmpty()) {
            xmlWriter->writeAttribute(QStringLiteral("thread"), d->markedThread);
        }
        xmlWriter->writeEndElement();
    }

    // XEP-0280: Message Carbons
    if (d->privatemsg) {
        xmlWriter->writeStartElement(QStringLiteral("private"));
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_carbons);
        xmlWriter->writeEndElement();
    }

    // XEP-0066: Out of Band Data
    if (!d->outOfBandUrl.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("x"));
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_oob);
        xmlWriter->writeTextElement(QStringLiteral("url"), d->outOfBandUrl);
        xmlWriter->writeEndElement();
    }

    // XEP-0308: Last Message Correction
    if (!d->replaceId.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("replace"));
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_message_correct);
        xmlWriter->writeAttribute(QStringLiteral("id"), d->replaceId);
        xmlWriter->writeEndElement();
    }

    // XEP-0369: Mediated Information eXchange (MIX)
    if (!d->mixUserJid.isEmpty() || !d->mixUserNick.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("mix"));
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_mix);
        helperToXmlAddTextElement(xmlWriter, QStringLiteral("jid"), d->mixUserJid);
        helperToXmlAddTextElement(xmlWriter, QStringLiteral("nick"), d->mixUserNick);
        xmlWriter->writeEndElement();
    }

    // XEP-0382: Spoiler messages
    if (d->isSpoiler) {
        xmlWriter->writeStartElement(QStringLiteral("spoiler"));
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_spoiler);
        xmlWriter->writeCharacters(d->spoilerHint);
        xmlWriter->writeEndElement();
    }

    // other extensions
    QXmppStanza::extensionsToXml(xmlWriter);

    xmlWriter->writeEndElement();
}
/// \endcond
