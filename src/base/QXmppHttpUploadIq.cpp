/*
 * Copyright (C) 2008-2019 The QXmpp developers
 *
 * Authors:
 *  Linus Jahn <lnj@kaidan.im>
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
#include <QMimeDatabase>

#include "QXmppHttpUploadIq.h"
#include "QXmppConstants_p.h"

class QXmppHttpUploadRequestIqPrivate
{
public:
    QString fileName;
    qint64 size;
    QMimeType contentType;
};

QXmppHttpUploadRequestIq::QXmppHttpUploadRequestIq()
    : d(new QXmppHttpUploadRequestIqPrivate())
{
}

QXmppHttpUploadRequestIq::~QXmppHttpUploadRequestIq()
{
    delete d;
}

/// Returns the file name of the file to be uploaded.

QString QXmppHttpUploadRequestIq::fileName() const
{
    return d->fileName;
}

/// Sets the file name. The upload service will use this to create the upload/
/// download URLs. This may also differ from the actual file name to get a
/// different URL. It's not required to replace special characters (this is the
/// server's job).

void QXmppHttpUploadRequestIq::setFileName(const QString &fileName)
{
    d->fileName = fileName;
}

/// Returns the file's size in bytes.

qint64 QXmppHttpUploadRequestIq::size() const
{
    return d->size;
}

/// Sets the file's size in bytes.

void QXmppHttpUploadRequestIq::setSize(qint64 size)
{
    d->size = size;
}

/// Returns the (optional) MIME-type of the file.

QMimeType QXmppHttpUploadRequestIq::contentType() const
{
    return d->contentType;
}

/// Sets the MIME-type of the file. This is optional.

void QXmppHttpUploadRequestIq::setContentType(const QMimeType &type)
{
    d->contentType = type;
}

bool QXmppHttpUploadRequestIq::isHttpUploadRequestIq(const QDomElement &element)
{
    if (element.tagName() == QLatin1String("iq")) {
        QDomElement request = element.firstChildElement(QStringLiteral("request"));
        return !request.isNull() && request.namespaceURI() == ns_http_upload;
    }
    return false;
}

/// \cond
void QXmppHttpUploadRequestIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement request = element.firstChildElement(QStringLiteral("request"));
    d->fileName = request.attribute(QStringLiteral("filename"));
    d->size = request.attribute(QStringLiteral("size")).toLongLong();
    if (request.hasAttribute(QStringLiteral("content-type"))) {
        QMimeDatabase mimeDb;
        QMimeType type = mimeDb.mimeTypeForName(request.attribute(QStringLiteral("content-type")));
        if (!type.isDefault() && type.isValid())
            d->contentType = type;
    }
}

void QXmppHttpUploadRequestIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("request"));
    writer->writeAttribute(QStringLiteral("xmlns"), ns_http_upload);
    // filename and size are required
    writer->writeAttribute(QStringLiteral("filename"), d->fileName);
    writer->writeAttribute(QStringLiteral("size"), QString::number(d->size));
    // content-type is optional
    if (!d->contentType.isDefault() && d->contentType.isValid())
        writer->writeAttribute(QStringLiteral("content-type"), d->contentType.name());
    writer->writeEndElement();
}
/// \endcond

class QXmppHttpUploadSlotIqPrivate
{
public:
    QUrl putUrl;
    QUrl getUrl;
    QMap<QString, QString> putHeaders;
};

QXmppHttpUploadSlotIq::QXmppHttpUploadSlotIq()
    : d(new QXmppHttpUploadSlotIqPrivate())
{
}

QXmppHttpUploadSlotIq::~QXmppHttpUploadSlotIq()
{
    delete d;
}

/// Returns the URL for uploading via. HTTP PUT.

QUrl QXmppHttpUploadSlotIq::putUrl() const
{
    return d->putUrl;
}

/// Sets the URL the client should use for uploading.

void QXmppHttpUploadSlotIq::setPutUrl(const QUrl &putUrl)
{
    d->putUrl = putUrl;
}

/// Returns the URL to where the file will be served.

QUrl QXmppHttpUploadSlotIq::getUrl() const
{
    return d->getUrl;
}

/// Sets the download URL.

void QXmppHttpUploadSlotIq::setGetUrl(const QUrl &getUrl)
{
    d->getUrl = getUrl;
}

/// Returns a map of header fields (header name -> value) that need to be
/// included in the PUT (upload) request. This won't contain any other fields
/// than: "Authorization", "Cookie" or "Expires".

QMap<QString, QString> QXmppHttpUploadSlotIq::putHeaders() const
{
    return d->putHeaders;
}

/// Sets the header fields the client needs to include in the PUT (upload)
/// request. All fields other than "Authorization", "Cookie" or "Expires" will
/// be ignored.

void QXmppHttpUploadSlotIq::setPutHeaders(const QMap<QString, QString> &putHeaders)
{
    d->putHeaders.clear();
    for (QString &name : putHeaders.keys()) {
        if (name == QLatin1String("Authorization") || name == QLatin1String("Cookie") || name == QLatin1String("Expires"))
            d->putHeaders[name] = putHeaders[name];
    }
}

bool QXmppHttpUploadSlotIq::isHttpUploadSlotIq(const QDomElement &element)
{
    if (element.tagName() == QLatin1String("iq")) {
        QDomElement slot = element.firstChildElement(QStringLiteral("slot"));
        return !slot.isNull() && slot.namespaceURI() == ns_http_upload;
    }
    return false;
}

/// \cond
void QXmppHttpUploadSlotIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement slot = element.firstChildElement(QStringLiteral("slot"));
    QDomElement put = slot.firstChildElement(QStringLiteral("put"));
    d->getUrl = QUrl::fromEncoded(slot.firstChildElement(QStringLiteral("get")).attribute(QStringLiteral("url")).toUtf8());
    d->putUrl = QUrl::fromEncoded(put.attribute(QStringLiteral("url")).toUtf8());
    if (put.hasChildNodes()) {
        QMap<QString, QString> headers;
        QDomElement header = put.firstChildElement(QStringLiteral("header"));
        while (!header.isNull()) {
            headers[header.attribute(QStringLiteral("name"))] = header.text();

            header = header.nextSiblingElement(QStringLiteral("header"));
        }

        setPutHeaders(headers);
    }
}

void QXmppHttpUploadSlotIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("slot"));
    writer->writeAttribute(QStringLiteral("xmlns"), ns_http_upload);

    writer->writeStartElement(QStringLiteral("put"));
    writer->writeAttribute(QStringLiteral("url"), d->putUrl.toEncoded());
    if (!d->putHeaders.isEmpty()) {
        const auto putHeaders = d->putHeaders.keys();
        for (const QString &name : putHeaders) {
            writer->writeStartElement(QStringLiteral("header"));
            writer->writeAttribute(QStringLiteral("name"), name);
            writer->writeCharacters(d->putHeaders[name]);
            writer->writeEndElement();
        }
    }
    writer->writeEndElement();

    writer->writeStartElement(QStringLiteral("get"));
    writer->writeAttribute(QStringLiteral("url"), d->getUrl.toEncoded());
    writer->writeEndElement();

    writer->writeEndElement();
}
/// \endcond
