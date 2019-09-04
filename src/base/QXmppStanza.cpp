/*
 * Copyright (C) 2008-2019 The QXmpp developers
 *
 * Authors:
 *  Manjeet Dahiya
 *  Jeremy Lainé
 *  Georg Rudoy
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


#include "QXmppStanza.h"
#include "QXmppStanza_p.h"
#include "QXmppUtils.h"
#include "QXmppConstants_p.h"

#include <QDomElement>
#include <QXmlStreamWriter>

uint QXmppStanza::s_uniqeIdNo = 0;

class QXmppExtendedAddressPrivate : public QSharedData
{
public:
    bool delivered;
    QString description;
    QString jid;
    QString type;
};

/// Constructs an empty extended address.

QXmppExtendedAddress::QXmppExtendedAddress()
    : d(new QXmppExtendedAddressPrivate())
{
    d->delivered = false;
}

/// Constructs a copy of other.
///
/// \param other
///
QXmppExtendedAddress::QXmppExtendedAddress(const QXmppExtendedAddress &other)
    : d(other.d)
{
}

QXmppExtendedAddress::~QXmppExtendedAddress()
{
}

/// Assigns the other address to this one.
///
/// \param other
///
QXmppExtendedAddress& QXmppExtendedAddress::operator=(const QXmppExtendedAddress& other)
{
    d = other.d;
    return *this;
}

/// Returns the human-readable description of the address.

QString QXmppExtendedAddress::description() const
{
    return d->description;
}

/// Sets the human-readable \a description of the address.

void QXmppExtendedAddress::setDescription(const QString &description)
{
    d->description = description;
}

/// Returns the JID of the address.

QString QXmppExtendedAddress::jid() const
{
    return d->jid;
}

/// Sets the JID of the address.

void QXmppExtendedAddress::setJid(const QString &jid)
{
    d->jid = jid;
}

/// Returns the type of the address.

QString QXmppExtendedAddress::type() const
{
    return d->type;
}

/// Sets the \a type of the address.

void QXmppExtendedAddress::setType(const QString &type)
{
    d->type = type;
}

/// Returns whether the stanza has been delivered to this address.

bool QXmppExtendedAddress::isDelivered() const
{
    return d->delivered;
}

/// Sets whether the stanza has been \a delivered to this address.

void QXmppExtendedAddress::setDelivered(bool delivered)
{
    d->delivered = delivered;
}

/// Checks whether this address is valid. The extended address is considered
/// to be valid if at least type and JID fields are non-empty.

bool QXmppExtendedAddress::isValid() const
{
    return !d->type.isEmpty() && !d->jid.isEmpty();
}

/// \cond
void QXmppExtendedAddress::parse(const QDomElement &element)
{
    d->delivered = element.attribute(QStringLiteral("delivered")) == QLatin1String("true");
    d->description = element.attribute(QStringLiteral("desc"));
    d->jid = element.attribute(QStringLiteral("jid"));
    d->type = element.attribute(QStringLiteral("type"));
}

void QXmppExtendedAddress::toXml(QXmlStreamWriter *xmlWriter) const
{
    xmlWriter->writeStartElement(QStringLiteral("address"));
    if (d->delivered)
        xmlWriter->writeAttribute(QStringLiteral("delivered"), QStringLiteral("true"));
    if (!d->description.isEmpty())
        xmlWriter->writeAttribute(QStringLiteral("desc"), d->description);
    xmlWriter->writeAttribute(QStringLiteral("jid"), d->jid);
    xmlWriter->writeAttribute(QStringLiteral("type"), d->type);
    xmlWriter->writeEndElement();
}
/// \endcond

QXmppStanza::Error::Error():
    m_code(0),
    m_type(static_cast<QXmppStanza::Error::Type>(-1)),
    m_condition(static_cast<QXmppStanza::Error::Condition>(-1))
{
}

QXmppStanza::Error::Error(Type type, Condition cond, const QString& text):
    m_code(0),
    m_type(type),
    m_condition(cond),
    m_text(text)
{
}

QXmppStanza::Error::Error(const QString& type, const QString& cond,
                          const QString& text):
    m_code(0),
    m_text(text)
{
    setTypeFromStr(type);
    setConditionFromStr(cond);
}

QString QXmppStanza::Error::text() const
{
    return m_text;
}

void QXmppStanza::Error::setText(const QString& text)
{
    m_text = text;
}

int QXmppStanza::Error::code() const
{
    return m_code;
}

void QXmppStanza::Error::setCode(int code)
{
    m_code = code;
}

QXmppStanza::Error::Condition QXmppStanza::Error::condition() const
{
    return m_condition;
}

void QXmppStanza::Error::setCondition(QXmppStanza::Error::Condition cond)
{
    m_condition = cond;
}

QXmppStanza::Error::Type QXmppStanza::Error::type() const
{
    return m_type;
}

void QXmppStanza::Error::setType(QXmppStanza::Error::Type type)
{
    m_type = type;
}

/// \cond
QString QXmppStanza::Error::getTypeStr() const
{
    switch(m_type)
    {
    case Cancel:
        return QStringLiteral("cancel");
    case Continue:
        return QStringLiteral("continue");
    case Modify:
        return QStringLiteral("modify");
    case Auth:
        return QStringLiteral("auth");
    case Wait:
        return QStringLiteral("wait");
    default:
        return QLatin1String("");
    }
}

QString QXmppStanza::Error::getConditionStr() const
{
    return strFromCondition(m_condition);
}

void QXmppStanza::Error::setTypeFromStr(const QString& type)
{
    if(type == QLatin1String("cancel"))
        setType(Cancel);
    else if(type == QLatin1String("continue"))
        setType(Continue);
    else if(type == QLatin1String("modify"))
        setType(Modify);
    else if(type == QLatin1String("auth"))
        setType(Auth);
    else if(type == QLatin1String("wait"))
        setType(Wait);
    else
        setType(static_cast<QXmppStanza::Error::Type>(-1));
}

void QXmppStanza::Error::setConditionFromStr(const QString& type)
{
    setCondition(conditionFromStr(type));
}

void QXmppStanza::Error::parse(const QDomElement &errorElement)
{
    setCode(errorElement.attribute(QStringLiteral("code")).toInt());
    setTypeFromStr(errorElement.attribute(QStringLiteral("type")));

    QString text;
    QString cond;
    QDomElement element = errorElement.firstChildElement();
    while(!element.isNull())
    {
        if(element.tagName() == QLatin1String("text"))
            text = element.text();
        else if(element.namespaceURI() == ns_stanza)
        {
            cond = element.tagName();
        }
        element = element.nextSiblingElement();
    }

    setConditionFromStr(cond);
    setText(text);
}

void QXmppStanza::Error::toXml( QXmlStreamWriter *writer ) const
{
    QString cond = getConditionStr();
    QString type = getTypeStr();

    if(cond.isEmpty() && type.isEmpty())
        return;

    writer->writeStartElement(QStringLiteral("error"));
    helperToXmlAddAttribute(writer, QStringLiteral("type"), type);

    if (m_code > 0)
        helperToXmlAddAttribute(writer, QStringLiteral("code"), QString::number(m_code));

    if(!cond.isEmpty())
    {
        writer->writeStartElement(cond);
        writer->writeAttribute(QStringLiteral("xmlns"), ns_stanza);
        writer->writeEndElement();
    }
    if(!m_text.isEmpty())
    {
        writer->writeStartElement(QStringLiteral("text"));
        writer->writeAttribute(QStringLiteral("xml:lang"), QStringLiteral("en"));
        writer->writeAttribute(QStringLiteral("xmlns"), ns_stanza);
        writer->writeCharacters(m_text);
        writer->writeEndElement();
    }

    writer->writeEndElement();
}
/// \endcond

class QXmppStanzaPrivate : public QSharedData
{
public:
    QString to;
    QString from;
    QString id;
    QString lang;
    QXmppStanza::Error error;
    QXmppElementList extensions;
    QList<QXmppExtendedAddress> extendedAddresses;
};

/// Constructs a QXmppStanza with the specified sender and recipient.
///
/// \param from
/// \param to

QXmppStanza::QXmppStanza(const QString& from, const QString& to)
    : d(new QXmppStanzaPrivate)
{
    d->to = to;
    d->from = from;
}

/// Constructs a copy of \a other.

QXmppStanza::QXmppStanza(const QXmppStanza &other)
    : d(other.d)
{
}

/// Destroys a QXmppStanza.

QXmppStanza::~QXmppStanza()
{
}

/// Assigns \a other to this stanza.

QXmppStanza& QXmppStanza::operator=(const QXmppStanza &other)
{
    d = other.d;
    return *this;
}

/// Returns the stanza's recipient JID.
///

QString QXmppStanza::to() const
{
    return d->to;
}

/// Sets the stanza's recipient JID.
///
/// \param to

void QXmppStanza::setTo(const QString& to)
{
    d->to = to;
}

/// Returns the stanza's sender JID.

QString QXmppStanza::from() const
{
    return d->from;
}

/// Sets the stanza's sender JID.
///
/// \param from

void QXmppStanza::setFrom(const QString& from)
{
    d->from = from;
}

/// Returns the stanza's identifier.

QString QXmppStanza::id() const
{
    return d->id;
}

/// Sets the stanza's identifier.
///
/// \param id

void QXmppStanza::setId(const QString& id)
{
    d->id = id;
}

/// Returns the stanza's language.

QString QXmppStanza::lang() const
{
    return d->lang;
}

/// Sets the stanza's language.
///
/// \param lang

void QXmppStanza::setLang(const QString& lang)
{
    d->lang = lang;
}

/// Returns the stanza's error.

QXmppStanza::Error QXmppStanza::error() const
{
    return d->error;
}

/// Sets the stanza's error.
///
/// \param error

void QXmppStanza::setError(const QXmppStanza::Error& error)
{
    d->error = error;
}

/// Returns the stanza's "extensions".
///
/// Extensions are XML elements which are not handled internally by QXmpp.

QXmppElementList QXmppStanza::extensions() const
{
    return d->extensions;
}

/// Sets the stanza's "extensions".
///
/// \param extensions

void QXmppStanza::setExtensions(const QXmppElementList &extensions)
{
    d->extensions = extensions;
}

/// Returns the stanza's extended addresses as defined by
/// XEP-0033: Extended Stanza Addressing.

QList<QXmppExtendedAddress> QXmppStanza::extendedAddresses() const
{
    return d->extendedAddresses;
}

/// Sets the stanza's extended addresses as defined by
/// XEP-0033: Extended Stanza Addressing.

void QXmppStanza::setExtendedAddresses(const QList<QXmppExtendedAddress> &addresses)
{
    d->extendedAddresses = addresses;
}

/// Indicates if the QXmppStanza is a stanza in the XMPP sense (i. e. a message,
/// iq or presence)

bool QXmppStanza::isXmppStanza() const
{
    return false;
}

/// \cond
void QXmppStanza::generateAndSetNextId()
{
    // get back
    ++s_uniqeIdNo;
    d->id = "qxmpp" + QString::number(s_uniqeIdNo);
}

void QXmppStanza::parse(const QDomElement &element)
{
    d->from = element.attribute(QStringLiteral("from"));
    d->to = element.attribute(QStringLiteral("to"));
    d->id = element.attribute(QStringLiteral("id"));
    d->lang = element.attribute(QStringLiteral("lang"));

    QDomElement errorElement = element.firstChildElement(QStringLiteral("error"));
    if(!errorElement.isNull())
        d->error.parse(errorElement);

    // XEP-0033: Extended Stanza Addressing
    QDomElement addressElement = element.firstChildElement(QStringLiteral("addresses")).firstChildElement(QStringLiteral("address"));
    while (!addressElement.isNull()) {
        QXmppExtendedAddress address;
        address.parse(addressElement);
        if (address.isValid())
            d->extendedAddresses << address;
        addressElement = addressElement.nextSiblingElement(QStringLiteral("address"));
    }
}

void QXmppStanza::extensionsToXml(QXmlStreamWriter *xmlWriter) const
{
    // XEP-0033: Extended Stanza Addressing
    if (!d->extendedAddresses.isEmpty()) {
        xmlWriter->writeStartElement(QStringLiteral("addresses"));
        xmlWriter->writeAttribute(QStringLiteral("xmlns"), ns_extended_addressing);
        foreach (const QXmppExtendedAddress &address, d->extendedAddresses)
            address.toXml(xmlWriter);
        xmlWriter->writeEndElement();
    }

    // other extensions
    foreach (const QXmppElement &extension, d->extensions)
        extension.toXml(xmlWriter);
}

/// \endcond
