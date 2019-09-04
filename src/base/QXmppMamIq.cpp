/*
 * Copyright (C) 2008-2019 The QXmpp developers
 *
 * Author:
 *  Niels Ole Salscheider
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

#include "QXmppMamIq.h"
#include "QXmppConstants_p.h"

QXmppMamQueryIq::QXmppMamQueryIq() : QXmppIq(QXmppIq::Set)
{
}

/// Returns the form that specifies the query.
QXmppDataForm QXmppMamQueryIq::form() const
{
    return m_form;
}

/// Sets the data form that specifies the query.
///
/// \param form The data form.
void QXmppMamQueryIq::setForm(const QXmppDataForm &form)
{
    m_form = form;
}

/// Returns the result set query for result set management.
QXmppResultSetQuery QXmppMamQueryIq::resultSetQuery() const
{
    return m_resultSetQuery;
}

/// Sets the result set query for result set management.
///
/// \param resultSetQuery The result set query.
void QXmppMamQueryIq::setResultSetQuery(const QXmppResultSetQuery &resultSetQuery)
{
    m_resultSetQuery = resultSetQuery;
}

/// Returns the node to query.
QString QXmppMamQueryIq::node() const
{
    return m_node;
}

/// Sets the node to query.
///
/// \param node The node to query.
void QXmppMamQueryIq::setNode(const QString &node)
{
    m_node = node;
}

/// Returns the queryid that will be included in the results.
QString QXmppMamQueryIq::queryId() const
{
    return m_queryId;
}

/// Sets the queryid that will be included in the results.
///
/// \param id The query id.
void QXmppMamQueryIq::setQueryId(const QString &id)
{
    m_queryId = id;
}

/// \cond
bool QXmppMamQueryIq::isMamQueryIq(const QDomElement &element)
{
    if (element.tagName() == QLatin1String("iq")) {
        QDomElement queryElement = element.firstChildElement(QStringLiteral("query"));
        if (!queryElement.isNull() && queryElement.namespaceURI() == ns_mam) {
            return true;
        }
    }
    return false;
}

void QXmppMamQueryIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement queryElement = element.firstChildElement(QStringLiteral("query"));
    m_node = queryElement.attribute(QStringLiteral("node"));
    m_queryId = queryElement.attribute(QStringLiteral("queryId"));
    QDomElement resultSetElement = queryElement.firstChildElement(QStringLiteral("set"));
    if (!resultSetElement.isNull()) {
        m_resultSetQuery.parse(resultSetElement);
    }
    QDomElement formElement = queryElement.firstChildElement(QStringLiteral("x"));
    if (!formElement.isNull()) {
        m_form.parse(formElement);
    }
}

void QXmppMamQueryIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("query"));
    writer->writeAttribute(QStringLiteral("xmlns"), ns_mam);
    if (!m_node.isEmpty()) {
        writer->writeAttribute(QStringLiteral("node"), m_node);
    }
    if (!m_queryId.isEmpty()) {
        writer->writeAttribute(QStringLiteral("queryid"), m_queryId);
    }
    m_form.toXml(writer);
    m_resultSetQuery.toXml(writer);
    writer->writeEndElement();
}
/// \endcond


QXmppMamResultIq::QXmppMamResultIq() : m_complete(false)
{
}

/// Returns the result set reply for result set management.
QXmppResultSetReply QXmppMamResultIq::resultSetReply() const
{
    return m_resultSetReply;
}

/// Sets the result set reply for result set management
void QXmppMamResultIq::setResultSetReply(const QXmppResultSetReply &resultSetReply)
{
    m_resultSetReply = resultSetReply;
}

/// Returns true if the results returned by the server are complete (not
/// limited by the server).
bool QXmppMamResultIq::complete() const
{
    return m_complete;
}

/// Sets if the results returned by the server are complete (not limited by the
/// server).
void QXmppMamResultIq::setComplete(bool complete)
{
    m_complete = complete;
}

/// \cond
bool QXmppMamResultIq::isMamResultIq(const QDomElement &element)
{
    if (element.tagName() == QLatin1String("iq")) {
        QDomElement finElement = element.firstChildElement(QStringLiteral("fin"));
        if (!finElement.isNull() && finElement.namespaceURI() == ns_mam) {
            return true;
        }
    }
    return false;
}

void QXmppMamResultIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement finElement = element.firstChildElement(QStringLiteral("fin"));
    m_complete = finElement.attribute(QStringLiteral("complete")) == QStringLiteral("true");
    QDomElement resultSetElement = finElement.firstChildElement(QStringLiteral("set"));
    if (!resultSetElement.isNull()) {
        m_resultSetReply.parse(resultSetElement);
    }
}

void QXmppMamResultIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("fin"));
    writer->writeAttribute(QStringLiteral("xmlns"), ns_mam);
    if (m_complete) {
        writer->writeAttribute(QStringLiteral("complete"), QStringLiteral("true"));
    }
    m_resultSetReply.toXml(writer);
    writer->writeEndElement();
}
/// \endcond
