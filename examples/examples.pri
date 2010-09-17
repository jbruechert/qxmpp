include(../qxmpp.pri)

TEMPLATE = app

QT += network xml

CONFIG += console

INCLUDEPATH += $$QXMPP_INCLUDE_DIR
LIBS += $$QXMPP_LIBS

# FIXME: we need to express a dependency on the library, but the file name
# depends on the platform and whether the library is static or dynamic
# PRE_TARGETDEPS += $${QXMPP_LIBRARY_DIR}/lib$${QXMPP_LIB}.a

