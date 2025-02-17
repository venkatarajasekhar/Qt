/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -N -p qibusinputcontextproxy -c QIBusInputContextProxy interfaces/org.freedesktop.IBus.InputContext.xml
 *
 * qdbusxml2cpp is Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef QIBUSINPUTCONTEXTPROXY_H_1394889529
#define QIBUSINPUTCONTEXTPROXY_H_1394889529

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface org.freedesktop.IBus.InputContext
 */
class QIBusInputContextProxy: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.freedesktop.IBus.InputContext"; }

public:
    QIBusInputContextProxy(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~QIBusInputContextProxy();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> Destroy()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("Destroy"), argumentList);
    }

    inline QDBusPendingReply<> Disable()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("Disable"), argumentList);
    }

    inline QDBusPendingReply<> Enable()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("Enable"), argumentList);
    }

    inline QDBusPendingReply<> FocusIn()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("FocusIn"), argumentList);
    }

    inline QDBusPendingReply<> FocusOut()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("FocusOut"), argumentList);
    }

    inline QDBusPendingReply<QDBusVariant> GetEngine()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("GetEngine"), argumentList);
    }

    inline QDBusPendingReply<bool> IsEnabled()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("IsEnabled"), argumentList);
    }

    inline QDBusPendingReply<bool> ProcessKeyEvent(uint keyval, uint keycode, uint state)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(keyval) << QVariant::fromValue(keycode) << QVariant::fromValue(state);
        return asyncCallWithArgumentList(QLatin1String("ProcessKeyEvent"), argumentList);
    }

    inline QDBusPendingReply<> PropertyActivate(const QString &name, int state)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(name) << QVariant::fromValue(state);
        return asyncCallWithArgumentList(QLatin1String("PropertyActivate"), argumentList);
    }

    inline QDBusPendingReply<> Reset()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("Reset"), argumentList);
    }

    inline QDBusPendingReply<> SetCapabilities(uint caps)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(caps);
        return asyncCallWithArgumentList(QLatin1String("SetCapabilities"), argumentList);
    }

    inline QDBusPendingReply<> SetCursorLocation(int x, int y, int w, int h)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(x) << QVariant::fromValue(y) << QVariant::fromValue(w) << QVariant::fromValue(h);
        return asyncCallWithArgumentList(QLatin1String("SetCursorLocation"), argumentList);
    }

    inline QDBusPendingReply<> SetEngine(const QString &name)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(name);
        return asyncCallWithArgumentList(QLatin1String("SetEngine"), argumentList);
    }

    inline QDBusPendingReply<> SetSurroundingText(const QDBusVariant &text, uint cursor_pos, uint anchor_pos)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(text) << QVariant::fromValue(cursor_pos) << QVariant::fromValue(anchor_pos);
        return asyncCallWithArgumentList(QLatin1String("SetSurroundingText"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void CommitText(const QDBusVariant &text);
    void CursorDownLookupTable();
    void CursorUpLookupTable();
    void DeleteSurroundingText(int offset, uint n_chars);
    void Disabled();
    void Enabled();
    void ForwardKeyEvent(uint keyval, uint keycode, uint state);
    void HideAuxiliaryText();
    void HideLookupTable();
    void HidePreeditText();
    void PageDownLookupTable();
    void PageUpLookupTable();
    void RegisterProperties(const QDBusVariant &props);
    void RequireSurroundingText();
    void ShowAuxiliaryText();
    void ShowLookupTable();
    void ShowPreeditText();
    void UpdateAuxiliaryText(const QDBusVariant &text, bool visible);
    void UpdateLookupTable(const QDBusVariant &table, bool visible);
    void UpdatePreeditText(const QDBusVariant &text, uint cursor_pos, bool visible);
    void UpdateProperty(const QDBusVariant &prop);
};

#endif
