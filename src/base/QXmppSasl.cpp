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

#include <cstdlib>

#include <QDomElement>
#include <QMessageAuthenticationCode>
#include <QStringList>
#include <QtEndian>
#include <QUrlQuery>

#include "QXmppSasl_p.h"
#include "QXmppUtils.h"

const char *ns_xmpp_sasl = "urn:ietf:params:xml:ns:xmpp-sasl";

static QByteArray forcedNonce;

// Calculate digest response for use with XMPP/SASL.

static QByteArray calculateDigest(const QByteArray &method, const QByteArray &digestUri, const QByteArray &secret, const QByteArray &nonce, const QByteArray &cnonce, const QByteArray &nc)
{
    const QByteArray A1 = secret + ':' + nonce + ':' + cnonce;
    const QByteArray A2 = method + ':' + digestUri;

    QByteArray HA1 = QCryptographicHash::hash(A1, QCryptographicHash::Md5).toHex();
    QByteArray HA2 = QCryptographicHash::hash(A2, QCryptographicHash::Md5).toHex();
    const QByteArray KD = HA1 + ':' + nonce + ':' + nc + ':' + cnonce + ":auth:" + HA2;
    return QCryptographicHash::hash(KD, QCryptographicHash::Md5).toHex();
}

// Perform PBKFD2 key derivation, code taken from Qt 5.12

static QByteArray deriveKeyPbkdf2(QCryptographicHash::Algorithm algorithm,
                                  const QByteArray &data, const QByteArray &salt,
                                  int iterations, quint64 dkLen)
{
    QByteArray key;
    quint32 currentIteration = 1;
    QMessageAuthenticationCode hmac(algorithm, data);
    QByteArray index(4, Qt::Uninitialized);
    while (quint64(key.length()) < dkLen) {
        hmac.addData(salt);
        qToBigEndian(currentIteration, reinterpret_cast<uchar*>(index.data()));
        hmac.addData(index);
        QByteArray u = hmac.result();
        hmac.reset();
        QByteArray tkey = u;
        for (int iter = 1; iter < iterations; iter++) {
            hmac.addData(u);
            u = hmac.result();
            hmac.reset();
            std::transform(tkey.cbegin(), tkey.cend(), u.cbegin(), tkey.begin(),
                           std::bit_xor<char>());
        }
        key += tkey;
        currentIteration++;
    }
    return key.left(dkLen);
}

static QByteArray generateNonce()
{
    if (!forcedNonce.isEmpty())
        return forcedNonce;

    QByteArray nonce = QXmppUtils::generateRandomBytes(32);

    // The random data can the '=' char is not valid as it is a delimiter,
    // so to be safe, base64 the nonce
    return nonce.toBase64();
}

static QMap<char, QByteArray> parseGS2(const QByteArray &ba)
{
    QMap<char, QByteArray> map;
    foreach (const QByteArray &keyValue, ba.split(',')) {
        if (keyValue.size() >= 2 && keyValue[1] == '=') {
            map[keyValue[0]] = keyValue.mid(2);
        }
    }
    return map;
}

QXmppSaslAuth::QXmppSaslAuth(const QString &mechanism, const QByteArray &value)
    : m_mechanism(mechanism)
    , m_value(value)
{
}

QString QXmppSaslAuth::mechanism() const
{
    return m_mechanism;
}

void QXmppSaslAuth::setMechanism(const QString &mechanism)
{
    m_mechanism = mechanism;
}

QByteArray QXmppSaslAuth::value() const
{
    return m_value;
}

void QXmppSaslAuth::setValue(const QByteArray &value)
{
    m_value = value;
}

void QXmppSaslAuth::parse(const QDomElement &element)
{
    m_mechanism = element.attribute(QStringLiteral("mechanism"));
    m_value = QByteArray::fromBase64(element.text().toLatin1());
}

void QXmppSaslAuth::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("auth"));
    writer->writeAttribute(QStringLiteral("xmlns"), ns_xmpp_sasl);
    writer->writeAttribute(QStringLiteral("mechanism"), m_mechanism);
    if (!m_value.isEmpty())
        writer->writeCharacters(m_value.toBase64());
    writer->writeEndElement();
}

QXmppSaslChallenge::QXmppSaslChallenge(const QByteArray &value)
    : m_value(value)
{
}

QByteArray QXmppSaslChallenge::value() const
{
    return m_value;
}

void QXmppSaslChallenge::setValue(const QByteArray &value)
{
    m_value = value;
}

void QXmppSaslChallenge::parse(const QDomElement &element)
{
    m_value = QByteArray::fromBase64(element.text().toLatin1());
}

void QXmppSaslChallenge::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("challenge"));
    writer->writeAttribute(QStringLiteral("xmlns"), ns_xmpp_sasl);
    if (!m_value.isEmpty())
        writer->writeCharacters(m_value.toBase64());
    writer->writeEndElement();
}

QXmppSaslFailure::QXmppSaslFailure(const QString &condition)
    : m_condition(condition)
{
}

QString QXmppSaslFailure::condition() const
{
    return m_condition;
}

void QXmppSaslFailure::setCondition(const QString &condition)
{
    m_condition = condition;
}

void QXmppSaslFailure::parse(const QDomElement &element)
{
    m_condition = element.firstChildElement().tagName();
}

void QXmppSaslFailure::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("failure"));
    writer->writeAttribute(QStringLiteral("xmlns"), ns_xmpp_sasl);
    if (!m_condition.isEmpty())
        writer->writeEmptyElement(m_condition);
    writer->writeEndElement();
}

QXmppSaslResponse::QXmppSaslResponse(const QByteArray &value)
    : m_value(value)
{
}

QByteArray QXmppSaslResponse::value() const
{
    return m_value;
}

void QXmppSaslResponse::setValue(const QByteArray &value)
{
    m_value = value;
}

void QXmppSaslResponse::parse(const QDomElement &element)
{
    m_value = QByteArray::fromBase64(element.text().toLatin1());
}

void QXmppSaslResponse::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("response"));
    writer->writeAttribute(QStringLiteral("xmlns"), ns_xmpp_sasl);
    if (!m_value.isEmpty())
        writer->writeCharacters(m_value.toBase64());
    writer->writeEndElement();
}

QXmppSaslSuccess::QXmppSaslSuccess()
{
}

void QXmppSaslSuccess::parse(const QDomElement &element)
{
    Q_UNUSED(element);
}

void QXmppSaslSuccess::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("success"));
    writer->writeAttribute(QStringLiteral("xmlns"), ns_xmpp_sasl);
    writer->writeEndElement();
}

class QXmppSaslClientPrivate
{
public:
    QString host;
    QString serviceType;
    QString username;
    QString password;
};

QXmppSaslClient::QXmppSaslClient(QObject *parent)
    : QXmppLoggable(parent)
    , d(new QXmppSaslClientPrivate)
{
}

QXmppSaslClient::~QXmppSaslClient()
{
    delete d;
}

/// Returns a list of supported mechanisms.

QStringList QXmppSaslClient::availableMechanisms()
{
    return QStringList() << QStringLiteral("SCRAM-SHA-256") << QStringLiteral("SCRAM-SHA-1") << QStringLiteral("DIGEST-MD5")
                         << QStringLiteral("PLAIN") << QStringLiteral("ANONYMOUS")
                         << QStringLiteral("X-FACEBOOK-PLATFORM") << QStringLiteral("X-MESSENGER-OAUTH2") << QStringLiteral("X-OAUTH2");
}

/// Creates an SASL client for the given mechanism.

QXmppSaslClient* QXmppSaslClient::create(const QString &mechanism, QObject *parent)
{
    if (mechanism == QLatin1String("PLAIN")) {
        return new QXmppSaslClientPlain(parent);
    } else if (mechanism == QLatin1String("DIGEST-MD5")) {
        return new QXmppSaslClientDigestMd5(parent);
    } else if (mechanism == QLatin1String("ANONYMOUS")) {
        return new QXmppSaslClientAnonymous(parent);
    } else if (mechanism == QLatin1String("SCRAM-SHA-1")) {
        return new QXmppSaslClientScram(QCryptographicHash::Sha1, parent);
    } else if (mechanism == QLatin1String("SCRAM-SHA-256")) {
        return new QXmppSaslClientScram(QCryptographicHash::Sha256, parent);
    } else if (mechanism == QLatin1String("X-FACEBOOK-PLATFORM")) {
        return new QXmppSaslClientFacebook(parent);
    } else if (mechanism == QLatin1String("X-MESSENGER-OAUTH2")) {
        return new QXmppSaslClientWindowsLive(parent);
    } else if (mechanism == QLatin1String("X-OAUTH2")) {
        return new QXmppSaslClientGoogle(parent);
    } else {
        return nullptr;
    }
}

/// Returns the host.

QString QXmppSaslClient::host() const
{
    return d->host;
}

/// Sets the host.

void QXmppSaslClient::setHost(const QString &host)
{
    d->host = host;
}

/// Returns the service type, e.g. "xmpp".

QString QXmppSaslClient::serviceType() const
{
    return d->serviceType;
}

/// Sets the service type, e.g. "xmpp".

void QXmppSaslClient::setServiceType(const QString &serviceType)
{
    d->serviceType = serviceType;
}

/// Returns the username.

QString QXmppSaslClient::username() const
{
    return d->username;
}

/// Sets the username.

void QXmppSaslClient::setUsername(const QString &username)
{
    d->username = username;
}

/// Returns the password.

QString QXmppSaslClient::password() const
{
    return d->password;
}

/// Sets the password.

void QXmppSaslClient::setPassword(const QString &password)
{
    d->password = password;
}

QXmppSaslClientAnonymous::QXmppSaslClientAnonymous(QObject *parent)
    : QXmppSaslClient(parent)
    , m_step(0)
{
}

QString QXmppSaslClientAnonymous::mechanism() const
{
    return QStringLiteral("ANONYMOUS");
}

bool QXmppSaslClientAnonymous::respond(const QByteArray &challenge, QByteArray &response)
{
    Q_UNUSED(challenge);
    if (m_step == 0) {
        response = QByteArray();
        m_step++;
        return true;
    } else {
        warning(QStringLiteral("QXmppSaslClientAnonymous : Invalid step"));
        return false;
    }
}

QXmppSaslClientDigestMd5::QXmppSaslClientDigestMd5(QObject *parent)
    : QXmppSaslClient(parent)
    , m_nc("00000001")
    , m_step(0)
{
    m_cnonce = generateNonce();
}

QString QXmppSaslClientDigestMd5::mechanism() const
{
    return QStringLiteral("DIGEST-MD5");
}

bool QXmppSaslClientDigestMd5::respond(const QByteArray &challenge, QByteArray &response)
{
    Q_UNUSED(challenge);
    const QByteArray digestUri = QStringLiteral("%1/%2").arg(serviceType(), host()).toUtf8();

    if (m_step == 0) {
        response = QByteArray();
        m_step++;
        return true;
    } else if (m_step == 1) {
        const QMap<QByteArray, QByteArray> input = QXmppSaslDigestMd5::parseMessage(challenge);

        if (!input.contains("nonce")) {
            warning(QStringLiteral("QXmppSaslClientDigestMd5 : Invalid input on step 1"));
            return false;
        }

        // determine realm
        const QByteArray realm = input.value("realm");

        // determine quality of protection
        const QList<QByteArray> qops = input.value("qop", "auth").split(',');
        if (!qops.contains("auth")) {
            warning(QStringLiteral("QXmppSaslClientDigestMd5 : Invalid quality of protection"));
            return false;
        }

        m_nonce = input.value("nonce");
        m_secret = QCryptographicHash::hash(
            username().toUtf8() + ":" + realm + ":" + password().toUtf8(),
            QCryptographicHash::Md5);

        // Build response
        QMap<QByteArray, QByteArray> output;
        output["username"] = username().toUtf8();
        if (!realm.isEmpty())
            output["realm"] = realm;
        output["nonce"] = m_nonce;
        output["qop"] = "auth";
        output["cnonce"] = m_cnonce;
        output["nc"] = m_nc;
        output["digest-uri"] = digestUri;
        output["response"] = calculateDigest("AUTHENTICATE", digestUri, m_secret, m_nonce, m_cnonce, m_nc);
        output["charset"] = "utf-8";

        response = QXmppSaslDigestMd5::serializeMessage(output);
        m_step++;
        return true;
    } else if (m_step == 2) {
        const QMap<QByteArray, QByteArray> input = QXmppSaslDigestMd5::parseMessage(challenge);

        // check new challenge
        if (input.value("rspauth") != calculateDigest(QByteArray(), digestUri, m_secret, m_nonce, m_cnonce, m_nc)) {
            warning(QStringLiteral("QXmppSaslClientDigestMd5 : Invalid challenge on step 2"));
            return false;
        }

        response = QByteArray();
        m_step++;
        return true;
    } else {
        warning(QStringLiteral("QXmppSaslClientDigestMd5 : Invalid step"));
        return false;
    }
}

QXmppSaslClientFacebook::QXmppSaslClientFacebook(QObject *parent)
    : QXmppSaslClient(parent)
    , m_step(0)
{
}

QString QXmppSaslClientFacebook::mechanism() const
{
    return QStringLiteral("X-FACEBOOK-PLATFORM");
}

bool QXmppSaslClientFacebook::respond(const QByteArray &challenge, QByteArray &response)
{
    if (m_step == 0) {
        // no initial response
        response = QByteArray();
        m_step++;
        return true;
    } else if (m_step == 1) {
        // parse request
        QUrlQuery requestUrl(challenge);
        if (!requestUrl.hasQueryItem(QStringLiteral("method")) || !requestUrl.hasQueryItem(QStringLiteral("nonce"))) {
            warning(QStringLiteral("QXmppSaslClientFacebook : Invalid challenge, nonce or method missing"));
            return false;
        }

        // build response
        QUrlQuery responseUrl;
        responseUrl.addQueryItem(QStringLiteral("access_token"), password());
        responseUrl.addQueryItem(QStringLiteral("api_key"), username());
        responseUrl.addQueryItem(QStringLiteral("call_id"), nullptr);
        responseUrl.addQueryItem(QStringLiteral("method"), requestUrl.queryItemValue(QStringLiteral("method")));
        responseUrl.addQueryItem(QStringLiteral("nonce"), requestUrl.queryItemValue(QStringLiteral("nonce")));
        responseUrl.addQueryItem(QStringLiteral("v"), QStringLiteral("1.0"));

        response = responseUrl.query().toUtf8();

        m_step++;
        return true;
    } else {
        warning(QStringLiteral("QXmppSaslClientFacebook : Invalid step"));
        return false;
    }
}

QXmppSaslClientGoogle::QXmppSaslClientGoogle(QObject *parent)
    : QXmppSaslClient(parent)
    , m_step(0)
{
}

QString QXmppSaslClientGoogle::mechanism() const
{
    return QStringLiteral("X-OAUTH2");
}

bool QXmppSaslClientGoogle::respond(const QByteArray &challenge, QByteArray &response)
{
    Q_UNUSED(challenge);
    if (m_step == 0) {
        // send initial response
        response = QString('\0' + username() + '\0' + password()).toUtf8();
        m_step++;
        return true;
    } else {
        warning(QStringLiteral("QXmppSaslClientGoogle : Invalid step"));
        return false;
    }
}

QXmppSaslClientPlain::QXmppSaslClientPlain(QObject *parent)
    : QXmppSaslClient(parent)
    , m_step(0)
{
}

QString QXmppSaslClientPlain::mechanism() const
{
    return QStringLiteral("PLAIN");
}

bool QXmppSaslClientPlain::respond(const QByteArray &challenge, QByteArray &response)
{
    Q_UNUSED(challenge);
    if (m_step == 0) {
        response = QString('\0' + username() + '\0' + password()).toUtf8();
        m_step++;
        return true;
    } else {
        warning(QStringLiteral("QXmppSaslClientPlain : Invalid step"));
        return false;
    }
}

QXmppSaslClientScram::QXmppSaslClientScram(QCryptographicHash::Algorithm algorithm, QObject *parent)
    : QXmppSaslClient(parent)
    , m_algorithm(algorithm)
    , m_step(0)
{
    Q_ASSERT(m_algorithm == QCryptographicHash::Sha1 || m_algorithm == QCryptographicHash::Sha256);
    m_nonce = generateNonce();

    if (m_algorithm == QCryptographicHash::Sha256) {
        m_dklen = 32;
        m_mechanism = QLatin1String("SCRAM-SHA-256");
    } else {
        m_dklen = 20;
        m_mechanism = QLatin1String("SCRAM-SHA-1");
    }
}

QString QXmppSaslClientScram::mechanism() const
{
    return m_mechanism;
}

bool QXmppSaslClientScram::respond(const QByteArray &challenge, QByteArray &response)
{
    Q_UNUSED(challenge);
    if (m_step == 0) {
        m_gs2Header = "n,,";
        m_clientFirstMessageBare = "n=" + username().toUtf8() + ",r=" + m_nonce;

        response = m_gs2Header + m_clientFirstMessageBare;
        m_step++;
        return true;
    } else if (m_step == 1) {
        // validate input
        const QMap<char, QByteArray> input = parseGS2(challenge);
        const QByteArray nonce = input.value('r');
        const QByteArray salt = QByteArray::fromBase64(input.value('s'));
        const int iterations = input.value('i').toInt();
        if (!nonce.startsWith(m_nonce) || salt.isEmpty() || iterations < 1) {
            return false;
        }

        // calculate proofs
        const QByteArray clientFinalMessageBare = "c=" + m_gs2Header.toBase64() + ",r=" + nonce;
        const QByteArray saltedPassword = deriveKeyPbkdf2(m_algorithm, password().toUtf8(), salt,
                                                          iterations, m_dklen);
        const QByteArray clientKey = QMessageAuthenticationCode::hash("Client Key", saltedPassword, m_algorithm);
        const QByteArray storedKey = QCryptographicHash::hash(clientKey, m_algorithm);
        const QByteArray authMessage = m_clientFirstMessageBare + "," + challenge + "," + clientFinalMessageBare;
        QByteArray clientProof = QMessageAuthenticationCode::hash(authMessage, storedKey, m_algorithm);
        std::transform(clientProof.cbegin(), clientProof.cend(), clientKey.cbegin(),
                       clientProof.begin(), std::bit_xor<char>());

        const QByteArray serverKey = QMessageAuthenticationCode::hash("Server Key", saltedPassword, m_algorithm);
        m_serverSignature = QMessageAuthenticationCode::hash(authMessage, serverKey, m_algorithm);

        response = clientFinalMessageBare + ",p=" + clientProof.toBase64();
        m_step++;
        return true;
    } else if (m_step == 2) {
        const QMap<char, QByteArray> input = parseGS2(challenge);
        response = QByteArray();
        m_step++;
        return QByteArray::fromBase64(input.value('v')) == m_serverSignature;
    } else {
        warning(QStringLiteral("QXmppSaslClientPlain : Invalid step"));
        return false;
    }
}

QXmppSaslClientWindowsLive::QXmppSaslClientWindowsLive(QObject *parent)
    : QXmppSaslClient(parent)
    , m_step(0)
{
}

QString QXmppSaslClientWindowsLive::mechanism() const
{
    return QStringLiteral("X-MESSENGER-OAUTH2");
}

bool QXmppSaslClientWindowsLive::respond(const QByteArray &challenge, QByteArray &response)
{
    Q_UNUSED(challenge);
    if (m_step == 0) {
        // send initial response
        response = QByteArray::fromBase64(password().toLatin1());
        m_step++;
        return true;
    } else {
        warning(QStringLiteral("QXmppSaslClientWindowsLive : Invalid step"));
        return false;
    }
}

class QXmppSaslServerPrivate
{
public:
    QString username;
    QString password;
    QByteArray passwordDigest;
    QString realm;
};

QXmppSaslServer::QXmppSaslServer(QObject *parent)
    : QXmppLoggable(parent)
    , d(new QXmppSaslServerPrivate)
{
}

QXmppSaslServer::~QXmppSaslServer()
{
    delete d;
}

/// Creates an SASL server for the given mechanism.

QXmppSaslServer* QXmppSaslServer::create(const QString &mechanism, QObject *parent)
{
    if (mechanism == QLatin1String("PLAIN")) {
        return new QXmppSaslServerPlain(parent);
    } else if (mechanism == QLatin1String("DIGEST-MD5")) {
        return new QXmppSaslServerDigestMd5(parent);
    } else if (mechanism == QLatin1String("ANONYMOUS")) {
        return new QXmppSaslServerAnonymous(parent);
    } else {
        return nullptr;
    }
}

/// Returns the username.

QString QXmppSaslServer::username() const
{
    return d->username;
}

/// Sets the username.

void QXmppSaslServer::setUsername(const QString &username)
{
    d->username = username;
}

/// Returns the password.

QString QXmppSaslServer::password() const
{
    return d->password;
}

/// Sets the password.

void QXmppSaslServer::setPassword(const QString &password)
{
    d->password = password;
}

/// Returns the password digest.

QByteArray QXmppSaslServer::passwordDigest() const
{
    return d->passwordDigest;
}

/// Sets the password digest.

void QXmppSaslServer::setPasswordDigest(const QByteArray &digest)
{
    d->passwordDigest = digest;
}

/// Returns the realm.

QString QXmppSaslServer::realm() const
{
    return d->realm;
}

/// Sets the realm.

void QXmppSaslServer::setRealm(const QString &realm)
{
    d->realm = realm;
}

QXmppSaslServerAnonymous::QXmppSaslServerAnonymous(QObject *parent)
    : QXmppSaslServer(parent)
    , m_step(0)
{
}

QString QXmppSaslServerAnonymous::mechanism() const
{
    return QStringLiteral("ANONYMOUS");
}

QXmppSaslServer::Response QXmppSaslServerAnonymous::respond(const QByteArray &request, QByteArray &response)
{
    Q_UNUSED(request);
    if (m_step == 0) {
        m_step++;
        response = QByteArray();
        return Succeeded;
    } else {
        warning(QStringLiteral("QXmppSaslServerAnonymous : Invalid step"));
        return Failed;
    }
}

QXmppSaslServerDigestMd5::QXmppSaslServerDigestMd5(QObject *parent)
    : QXmppSaslServer(parent)
    , m_step(0)
{
    m_nonce = generateNonce();
}

QString QXmppSaslServerDigestMd5::mechanism() const
{
    return QStringLiteral("DIGEST-MD5");
}

QXmppSaslServer::Response QXmppSaslServerDigestMd5::respond(const QByteArray &request, QByteArray &response)
{
    if (m_step == 0) {
        QMap<QByteArray, QByteArray> output;
        output["nonce"] = m_nonce;
        if (!realm().isEmpty())
            output["realm"] = realm().toUtf8();
        output["qop"] = "auth";
        output["charset"] = "utf-8";
        output["algorithm"] = "md5-sess";

        m_step++;
        response = QXmppSaslDigestMd5::serializeMessage(output);
        return Challenge;
    } else if (m_step == 1) {
        const QMap<QByteArray, QByteArray> input = QXmppSaslDigestMd5::parseMessage(request);
        const QByteArray realm = input.value("realm");
        const QByteArray digestUri = input.value("digest-uri");

        if (input.value("qop") != "auth") {
            warning(QStringLiteral("QXmppSaslServerDigestMd5 : Invalid quality of protection"));
            return Failed;
        }

        setUsername(QString::fromUtf8(input.value("username")));
        if (password().isEmpty() && passwordDigest().isEmpty())
            return InputNeeded;

        m_nc = input.value("nc");
        m_cnonce = input.value("cnonce");
        if (!password().isEmpty()) {
            m_secret = QCryptographicHash::hash(
                username().toUtf8() + ":" + realm + ":" + password().toUtf8(),
                QCryptographicHash::Md5);
        } else {
            m_secret = passwordDigest();
        }

        if (input.value("response") != calculateDigest("AUTHENTICATE", digestUri, m_secret, m_nonce, m_cnonce, m_nc))
            return Failed;

        QMap<QByteArray, QByteArray> output;
        output["rspauth"] = calculateDigest(QByteArray(), digestUri, m_secret, m_nonce, m_cnonce, m_nc);

        m_step++;
        response = QXmppSaslDigestMd5::serializeMessage(output);
        return Challenge;
    } else if (m_step == 2) {
        m_step++;
        response = QByteArray();
        return Succeeded;
    } else {
        warning(QStringLiteral("QXmppSaslServerDigestMd5 : Invalid step"));
        return Failed;
    }
}

QXmppSaslServerPlain::QXmppSaslServerPlain(QObject *parent)
    : QXmppSaslServer(parent)
    , m_step(0)
{
}

QString QXmppSaslServerPlain::mechanism() const
{
    return QStringLiteral("PLAIN");
}

QXmppSaslServer::Response QXmppSaslServerPlain::respond(const QByteArray &request, QByteArray &response)
{
    if (m_step == 0) {
        if (request.isEmpty()) {
            response = QByteArray();
            return Challenge;
        }

        QList<QByteArray> auth = request.split('\0');
        if (auth.size() != 3) {
            warning(QStringLiteral("QXmppSaslServerPlain : Invalid input"));
            return Failed;
        }
        setUsername(QString::fromUtf8(auth[1]));
        setPassword(QString::fromUtf8(auth[2]));

        m_step++;
        response = QByteArray();
        return InputNeeded;
    } else {
        warning(QStringLiteral("QXmppSaslServerPlain : Invalid step"));
        return Failed;
    }
}

void QXmppSaslDigestMd5::setNonce(const QByteArray &nonce)
{
    forcedNonce = nonce;
}

QMap<QByteArray, QByteArray> QXmppSaslDigestMd5::parseMessage(const QByteArray &ba)
{
    QMap<QByteArray, QByteArray> map;
    int startIndex = 0;
    int pos = 0;
    while ((pos = ba.indexOf("=", startIndex)) >= 0)
    {
        // key get name and skip equals
        const QByteArray key = ba.mid(startIndex, pos - startIndex).trimmed();
        pos++;

        // check whether string is quoted
        if (ba.at(pos) == '"')
        {
            // skip opening quote
            pos++;
            int endPos = ba.indexOf('"', pos);
            // skip quoted quotes
            while (endPos >= 0 && ba.at(endPos - 1) == '\\')
                endPos = ba.indexOf('"', endPos + 1);
            if (endPos < 0)
            {
                qWarning("Unfinished quoted string");
                return map;
            }
            // unquote
            QByteArray value = ba.mid(pos, endPos - pos);
            value.replace("\\\"", "\"");
            value.replace("\\\\", "\\");
            map[key] = value;
            // skip closing quote and comma
            startIndex = endPos + 2;
        } else {
            // non-quoted string
            int endPos = ba.indexOf(',', pos);
            if (endPos < 0)
                endPos = ba.size();
            map[key] = ba.mid(pos, endPos - pos);
            // skip comma
            startIndex = endPos + 1;
        }
    }
    return map;
}

QByteArray QXmppSaslDigestMd5::serializeMessage(const QMap<QByteArray, QByteArray> &map)
{
    QByteArray ba;
    foreach (const QByteArray &key, map.keys())
    {
        if (!ba.isEmpty())
            ba.append(',');
        ba.append(key + "=");
        QByteArray value = map[key];
        const char *separators = "()<>@,;:\\\"/[]?={} \t";
        bool quote = false;
        for (const char *c = separators; *c; c++)
        {
            if (value.contains(*c))
            {
                quote = true;
                break;
            }
        }
        if (quote)
        {
            value.replace("\\", "\\\\");
            value.replace("\"", "\\\"");
            ba.append("\"" + value + "\"");
        }
        else
            ba.append(value);
    }
    return ba;
}
