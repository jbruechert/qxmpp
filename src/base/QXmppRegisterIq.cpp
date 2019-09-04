/*
 * Copyright (C) 2008-2019 The QXmpp developers
 *
 * Author:
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

#include "QXmppConstants_p.h"
#include "QXmppRegisterIq.h"

/// Returns the email for this registration IQ.

QString QXmppRegisterIq::email() const
{
    return m_email;
}

/// Sets the \a email for this registration IQ.

void QXmppRegisterIq::setEmail(const QString &email)
{
    m_email = email;
}

/// Returns the QXmppDataForm for this registration IQ.

QXmppDataForm QXmppRegisterIq::form() const
{
    return m_form;
}

/// Sets the QXmppDataForm for this registration IQ.
///
/// \param form

void QXmppRegisterIq::setForm(const QXmppDataForm &form)
{
    m_form = form;
}

/// Returns the instructions for this registration IQ.

QString QXmppRegisterIq::instructions() const
{
    return m_instructions;
}

/// Sets the \a instructions for this registration IQ.

void QXmppRegisterIq::setInstructions(const QString &instructions)
{
    m_instructions = instructions;
}

/// Returns the password for this registration IQ.

QString QXmppRegisterIq::password() const
{
    return m_password;
}

/// Sets the \a password for this registration IQ.

void QXmppRegisterIq::setPassword(const QString &password)
{
    m_password = password;
}

/// Returns the username for this registration IQ.

QString QXmppRegisterIq::username() const
{
    return m_username;
}

/// Sets the \a username for this registration IQ.

void QXmppRegisterIq::setUsername(const QString &username)
{
    m_username = username;
}

/// \cond
bool QXmppRegisterIq::isRegisterIq(const QDomElement &element)
{
    return (element.firstChildElement(QStringLiteral("query")).namespaceURI() == ns_register);
}

void QXmppRegisterIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement queryElement = element.firstChildElement(QStringLiteral("query"));
    m_instructions = queryElement.firstChildElement(QStringLiteral("instructions")).text();
    m_username = queryElement.firstChildElement(QStringLiteral("username")).text();
    m_password = queryElement.firstChildElement(QStringLiteral("password")).text();
    m_email = queryElement.firstChildElement(QStringLiteral("email")).text();
    m_form.parse(queryElement.firstChildElement(QStringLiteral("x")));
}

void QXmppRegisterIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("query"));
    writer->writeAttribute(QStringLiteral("xmlns"), ns_register);
    if (!m_instructions.isEmpty())
        writer->writeTextElement(QStringLiteral("instructions"), m_instructions);

    if (!m_username.isEmpty())
        writer->writeTextElement(QStringLiteral("username"), m_username);
    else if (!m_username.isNull())
        writer->writeEmptyElement(QStringLiteral("username"));

    if (!m_password.isEmpty())
        writer->writeTextElement(QStringLiteral("password"), m_password);
    else if (!m_password.isNull())
        writer->writeEmptyElement(QStringLiteral("password"));

    if (!m_email.isEmpty())
        writer->writeTextElement(QStringLiteral("email"), m_email);
    else if (!m_email.isNull())
        writer->writeEmptyElement(QStringLiteral("email"));

    m_form.toXml(writer);
    writer->writeEndElement();
}
/// \endcond
