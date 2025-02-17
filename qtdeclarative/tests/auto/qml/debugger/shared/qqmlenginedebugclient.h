/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLENGINEDEBUGCLIENT_H
#define QQMLENGINEDEBUGCLIENT_H

#include "qqmldebugclient.h"

#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>

class QQmlDebugConnection;

struct QmlDebugPropertyReference
{
    QmlDebugPropertyReference()
        : objectDebugId(-1), hasNotifySignal(false)
    {
    }

    QmlDebugPropertyReference &operator=(
            const QmlDebugPropertyReference &o)
    {
        objectDebugId = o.objectDebugId; name = o.name; value = o.value;
        valueTypeName = o.valueTypeName; binding = o.binding;
        hasNotifySignal = o.hasNotifySignal;
        return *this;
    }

    int objectDebugId;
    QString name;
    QVariant value;
    QString valueTypeName;
    QString binding;
    bool hasNotifySignal;
};

struct QmlDebugFileReference
{
    QmlDebugFileReference()
        : lineNumber(-1), columnNumber(-1)
    {
    }

    QmlDebugFileReference &operator=(
            const QmlDebugFileReference &o)
    {
        url = o.url; lineNumber = o.lineNumber; columnNumber = o.columnNumber;
        return *this;
    }

    QUrl url;
    int lineNumber;
    int columnNumber;
};

struct QmlDebugObjectReference
{
    QmlDebugObjectReference()
        : debugId(-1), contextDebugId(-1)
    {
    }

    QmlDebugObjectReference(int id)
        : debugId(id), contextDebugId(-1)
    {
    }

    QmlDebugObjectReference &operator=(
            const QmlDebugObjectReference &o)
    {
        debugId = o.debugId; className = o.className; idString = o.idString;
        name = o.name; source = o.source; contextDebugId = o.contextDebugId;
        properties = o.properties; children = o.children;
        return *this;
    }
    int debugId;
    QString className;
    QString idString;
    QString name;
    QmlDebugFileReference source;
    int contextDebugId;
    QList<QmlDebugPropertyReference> properties;
    QList<QmlDebugObjectReference> children;
};

Q_DECLARE_METATYPE(QmlDebugObjectReference)

struct QmlDebugContextReference
{
    QmlDebugContextReference()
        : debugId(-1)
    {
    }

    QmlDebugContextReference &operator=(
            const QmlDebugContextReference &o)
    {
        debugId = o.debugId; name = o.name; objects = o.objects;
        contexts = o.contexts;
        return *this;
    }

    int debugId;
    QString name;
    QList<QmlDebugObjectReference> objects;
    QList<QmlDebugContextReference> contexts;
};

struct QmlDebugEngineReference
{
    QmlDebugEngineReference()
        : debugId(-1)
    {
    }

    QmlDebugEngineReference(int id)
        : debugId(id)
    {
    }

    QmlDebugEngineReference &operator=(
            const QmlDebugEngineReference &o)
    {
        debugId = o.debugId; name = o.name;
        return *this;
    }

    int debugId;
    QString name;
};

class QQmlEngineDebugClient : public QQmlDebugClient
{
    Q_OBJECT
public:
    explicit QQmlEngineDebugClient(QQmlDebugConnection *conn);

    quint32 addWatch(const QmlDebugPropertyReference &,
                     bool *success);
    quint32 addWatch(const QmlDebugContextReference &, const QString &,
                     bool *success);
    quint32 addWatch(const QmlDebugObjectReference &, const QString &,
                     bool *success);
    quint32 addWatch(const QmlDebugObjectReference &,
                     bool *success);
    quint32 addWatch(const QmlDebugFileReference &,
                     bool *success);

    void removeWatch(quint32 watch, bool *success);

    quint32 queryAvailableEngines(bool *success);
    quint32 queryRootContexts(const QmlDebugEngineReference &,
                              bool *success);
    quint32 queryObject(const QmlDebugObjectReference &,
                        bool *success);
    quint32 queryObjectsForLocation(const QString &file,
            int lineNumber, int columnNumber, bool *success);
    quint32 queryObjectRecursive(const QmlDebugObjectReference &,
                                 bool *success);
    quint32 queryObjectsForLocationRecursive(const QString &file,
            int lineNumber, int columnNumber, bool *success);
    quint32 queryExpressionResult(int objectDebugId,
                                  const QString &expr,
                                  bool *success);
    quint32 queryExpressionResultBC(int objectDebugId,
                                  const QString &expr,
                                  bool *success);
    quint32 setBindingForObject(int objectDebugId, const QString &propertyName,
                                const QVariant &bindingExpression,
                                bool isLiteralValue,
                                QString source, int line, bool *success);
    quint32 resetBindingForObject(int objectDebugId,
                                  const QString &propertyName, bool *success);
    quint32 setMethodBody(int objectDebugId, const QString &methodName,
                          const QString &methodBody, bool *success);

    quint32 getId() { return m_nextId++; }

    void decode(QDataStream &, QmlDebugContextReference &);
    void decode(QDataStream &, QmlDebugObjectReference &, bool simple);
    void decode(QDataStream &ds, QList<QmlDebugObjectReference> &o, bool simple);

    QList<QmlDebugEngineReference> engines() { return m_engines; }
    QmlDebugContextReference rootContext() { return m_rootContext; }
    QmlDebugObjectReference object() { return m_object; }
    QList<QmlDebugObjectReference> objects() { return m_objects; }
    QVariant resultExpr() { return m_exprResult; }
    bool valid() { return m_valid; }

signals:
    void newObjects();
    void valueChanged(QByteArray,QVariant);
    void result();

protected:
    void messageReceived(const QByteArray &);

private:
    quint32 m_nextId;
    bool m_valid;
    QList<QmlDebugEngineReference> m_engines;
    QmlDebugContextReference m_rootContext;
    QmlDebugObjectReference m_object;
    QList<QmlDebugObjectReference> m_objects;
    QVariant m_exprResult;

    QQmlDebugConnection *m_connection;
};

#endif // QQMLENGINEDEBUGCLIENT_H
