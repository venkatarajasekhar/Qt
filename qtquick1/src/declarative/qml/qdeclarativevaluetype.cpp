/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#include "private/qdeclarativevaluetype_p.h"

#include "private/qdeclarativemetatype_p.h"
#include "private/qfont_p.h"

#include <QtWidgets/qapplication.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

template<typename T>
int qmlRegisterValueTypeEnums(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    QByteArray name(T::staticMetaObject.className());

    QByteArray pointerName(name + '*');

    QDeclarativePrivate::RegisterType type = {
        0,

        qRegisterMetaType<T *>(pointerName.constData()), 0, 0, 0,

        QString(),

        uri, versionMajor, versionMinor, qmlName, &T::staticMetaObject,

        0, 0,

        0, 0, 0,

        0, 0,

        0,
        0
    };

    return QDeclarativePrivate::qmlregister(QDeclarativePrivate::TypeRegistration, &type);
}

QDeclarativeValueTypeFactory::QDeclarativeValueTypeFactory()
{
    static int isGuiApp = -1;
    memset(valueTypes, 0, sizeof(valueTypes));
    /*FIXME: is there a better way to tell if an app is Gui or not?*/
    if (isGuiApp == -1)
        isGuiApp = (int) qApp->inherits("QGuiApplication");
    // ### Optimize
    for (unsigned int ii = 0; ii <= QVariant::LastCoreType; ++ii)
        valueTypes[ii] = valueType(ii);

    if (isGuiApp) {
        for (unsigned int ii = QVariant::LastCoreType + 1; ii < (QVariant::UserType - 1); ++ii)
            valueTypes[ii] = valueType(ii);
    }
}

QDeclarativeValueTypeFactory::~QDeclarativeValueTypeFactory()
{
    for (unsigned int ii = 0; ii < (QVariant::UserType - 1); ++ii)
        delete valueTypes[ii];
}

bool QDeclarativeValueTypeFactory::isValueType(int idx)
{
    if ((uint)idx < QVariant::UserType
            && idx != QVariant::StringList
            && idx != QMetaType::QObjectStar
            && idx != QMetaType::VoidStar
            && idx != QMetaType::QVariant) {
        return true;
    }
    return false;
}

void QDeclarativeValueTypeFactory::registerValueTypes()
{
    qmlRegisterValueTypeEnums<QDeclarativeEasingValueType>("QtQuick",1,0,"Easing");
    qmlRegisterValueTypeEnums<QDeclarativeFontValueType>("QtQuick",1,0,"Font");
}

void QDeclarativeValueTypeFactory::registerValueTypesCompat()
{
    if (!qobject_cast<QApplication *>(QCoreApplication::instance()))
        return;

    qmlRegisterValueTypeEnums<QDeclarativeEasingValueType>("Qt",4,7,"Easing");
    qmlRegisterValueTypeEnums<QDeclarativeFontValueType>("Qt",4,7,"Font");
}

QDeclarativeValueType *QDeclarativeValueTypeFactory::valueType(int t)
{
    QDeclarativeValueType *rv = 0;

    switch (t) {
    case QVariant::Point:
        rv = new QDeclarativePointValueType;
        break;
    case QVariant::PointF:
        rv = new QDeclarativePointFValueType;
        break;
    case QVariant::Size:
        rv = new QDeclarativeSizeValueType;
        break;
    case QVariant::SizeF:
        rv = new QDeclarativeSizeFValueType;
        break;
    case QVariant::Rect:
        rv = new QDeclarativeRectValueType;
        break;
    case QVariant::RectF:
        rv = new QDeclarativeRectFValueType;
        break;
    case QVariant::Vector2D:
        rv = new QDeclarativeVector2DValueType;
        break;
    case QVariant::Vector3D:
        rv = new QDeclarativeVector3DValueType;
        break;
    case QVariant::Vector4D:
        rv = new QDeclarativeVector4DValueType;
        break;
    case QVariant::Quaternion:
        rv = new QDeclarativeQuaternionValueType;
        break;
    case QVariant::Matrix4x4:
        rv = new QDeclarativeMatrix4x4ValueType;
        break;
    case QVariant::EasingCurve:
        rv = new QDeclarativeEasingValueType;
        break;
    case QVariant::Font:
        rv = new QDeclarativeFontValueType;
        break;
    default:
        break;
    }

    Q_ASSERT(!rv || rv->metaObject()->propertyCount() < 32);
    return rv;
}

QDeclarativeValueType::QDeclarativeValueType(QObject *parent)
: QObject(parent)
{
}

QDeclarativePointFValueType::QDeclarativePointFValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativePointFValueType::read(QObject *obj, int idx)
{
    void *a[] = { &point, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativePointFValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &point, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativePointFValueType::value()
{
    return QVariant(point);
}

void QDeclarativePointFValueType::setValue(QVariant value)
{
    point = qvariant_cast<QPointF>(value);
}

qreal QDeclarativePointFValueType::x() const
{
    return point.x();
}

qreal QDeclarativePointFValueType::y() const
{
    return point.y();
}

void QDeclarativePointFValueType::setX(qreal x)
{
    point.setX(x);
}

void QDeclarativePointFValueType::setY(qreal y)
{
    point.setY(y);
}

QDeclarativePointValueType::QDeclarativePointValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativePointValueType::read(QObject *obj, int idx)
{
    void *a[] = { &point, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativePointValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &point, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativePointValueType::value()
{
    return QVariant(point);
}

void QDeclarativePointValueType::setValue(QVariant value)
{
    point = qvariant_cast<QPoint>(value);
}

int QDeclarativePointValueType::x() const
{
    return point.x();
}

int QDeclarativePointValueType::y() const
{
    return point.y();
}

void QDeclarativePointValueType::setX(int x)
{
    point.setX(x);
}

void QDeclarativePointValueType::setY(int y)
{
    point.setY(y);
}

QDeclarativeSizeFValueType::QDeclarativeSizeFValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeSizeFValueType::read(QObject *obj, int idx)
{
    void *a[] = { &size, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeSizeFValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &size, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativeSizeFValueType::value()
{
    return QVariant(size);
}

void QDeclarativeSizeFValueType::setValue(QVariant value)
{
    size = qvariant_cast<QSizeF>(value);
}

qreal QDeclarativeSizeFValueType::width() const
{
    return size.width();
}

qreal QDeclarativeSizeFValueType::height() const
{
    return size.height();
}

void QDeclarativeSizeFValueType::setWidth(qreal w)
{
    size.setWidth(w);
}

void QDeclarativeSizeFValueType::setHeight(qreal h)
{
    size.setHeight(h);
}

QDeclarativeSizeValueType::QDeclarativeSizeValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeSizeValueType::read(QObject *obj, int idx)
{
    void *a[] = { &size, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeSizeValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &size, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativeSizeValueType::value()
{
    return QVariant(size);
}

void QDeclarativeSizeValueType::setValue(QVariant value)
{
    size = qvariant_cast<QSize>(value);
}

int QDeclarativeSizeValueType::width() const
{
    return size.width();
}

int QDeclarativeSizeValueType::height() const
{
    return size.height();
}

void QDeclarativeSizeValueType::setWidth(int w)
{
    size.setWidth(w);
}

void QDeclarativeSizeValueType::setHeight(int h)
{
    size.setHeight(h);
}

QDeclarativeRectFValueType::QDeclarativeRectFValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeRectFValueType::read(QObject *obj, int idx)
{
    void *a[] = { &rect, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeRectFValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &rect, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativeRectFValueType::value()
{
    return QVariant(rect);
}

void QDeclarativeRectFValueType::setValue(QVariant value)
{
    rect = qvariant_cast<QRectF>(value);
}

qreal QDeclarativeRectFValueType::x() const
{
    return rect.x();
}

qreal QDeclarativeRectFValueType::y() const
{
    return rect.y();
}

void QDeclarativeRectFValueType::setX(qreal x)
{
    rect.moveLeft(x);
}

void QDeclarativeRectFValueType::setY(qreal y)
{
    rect.moveTop(y);
}

qreal QDeclarativeRectFValueType::width() const
{
    return rect.width();
}

qreal QDeclarativeRectFValueType::height() const
{
    return rect.height();
}

void QDeclarativeRectFValueType::setWidth(qreal w)
{
    rect.setWidth(w);
}

void QDeclarativeRectFValueType::setHeight(qreal h)
{
    rect.setHeight(h);
}

QDeclarativeRectValueType::QDeclarativeRectValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeRectValueType::read(QObject *obj, int idx)
{
    void *a[] = { &rect, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeRectValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &rect, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativeRectValueType::value()
{
    return QVariant(rect);
}

void QDeclarativeRectValueType::setValue(QVariant value)
{
    rect = qvariant_cast<QRect>(value);
}

int QDeclarativeRectValueType::x() const
{
    return rect.x();
}

int QDeclarativeRectValueType::y() const
{
    return rect.y();
}

void QDeclarativeRectValueType::setX(int x)
{
    rect.moveLeft(x);
}

void QDeclarativeRectValueType::setY(int y)
{
    rect.moveTop(y);
}

int QDeclarativeRectValueType::width() const
{
    return rect.width();
}

int QDeclarativeRectValueType::height() const
{
    return rect.height();
}

void QDeclarativeRectValueType::setWidth(int w)
{
    rect.setWidth(w);
}

void QDeclarativeRectValueType::setHeight(int h)
{
    rect.setHeight(h);
}

QDeclarativeVector2DValueType::QDeclarativeVector2DValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeVector2DValueType::read(QObject *obj, int idx)
{
    void *a[] = { &vector, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeVector2DValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &vector, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant  QDeclarativeVector2DValueType::value()
{
    return QVariant(vector);
}

void QDeclarativeVector2DValueType::setValue(QVariant value)
{
    vector = qvariant_cast<QVector2D>(value);
}

qreal QDeclarativeVector2DValueType::x() const
{
    return vector.x();
}

qreal QDeclarativeVector2DValueType::y() const
{
    return vector.y();
}

void QDeclarativeVector2DValueType::setX(qreal x)
{
    vector.setX(x);
}

void QDeclarativeVector2DValueType::setY(qreal y)
{
    vector.setY(y);
}

QDeclarativeVector3DValueType::QDeclarativeVector3DValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeVector3DValueType::read(QObject *obj, int idx)
{
    void *a[] = { &vector, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeVector3DValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &vector, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant  QDeclarativeVector3DValueType::value()
{
    return QVariant(vector);
}

void QDeclarativeVector3DValueType::setValue(QVariant value)
{
    vector = qvariant_cast<QVector3D>(value);
}

qreal QDeclarativeVector3DValueType::x() const
{
    return vector.x();
}

qreal QDeclarativeVector3DValueType::y() const
{
    return vector.y();
}

qreal QDeclarativeVector3DValueType::z() const
{
    return vector.z();
}

void QDeclarativeVector3DValueType::setX(qreal x)
{
    vector.setX(x);
}

void QDeclarativeVector3DValueType::setY(qreal y)
{
    vector.setY(y);
}

void QDeclarativeVector3DValueType::setZ(qreal z)
{
    vector.setZ(z);
}

QDeclarativeVector4DValueType::QDeclarativeVector4DValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeVector4DValueType::read(QObject *obj, int idx)
{
    void *a[] = { &vector, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeVector4DValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &vector, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant  QDeclarativeVector4DValueType::value()
{
    return QVariant(vector);
}

void QDeclarativeVector4DValueType::setValue(QVariant value)
{
    vector = qvariant_cast<QVector4D>(value);
}

qreal QDeclarativeVector4DValueType::x() const
{
    return vector.x();
}

qreal QDeclarativeVector4DValueType::y() const
{
    return vector.y();
}

qreal QDeclarativeVector4DValueType::z() const
{
    return vector.z();
}

qreal QDeclarativeVector4DValueType::w() const
{
    return vector.w();
}

void QDeclarativeVector4DValueType::setX(qreal x)
{
    vector.setX(x);
}

void QDeclarativeVector4DValueType::setY(qreal y)
{
    vector.setY(y);
}

void QDeclarativeVector4DValueType::setZ(qreal z)
{
    vector.setZ(z);
}

void QDeclarativeVector4DValueType::setW(qreal w)
{
    vector.setW(w);
}

QDeclarativeQuaternionValueType::QDeclarativeQuaternionValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeQuaternionValueType::read(QObject *obj, int idx)
{
    void *a[] = { &quaternion, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeQuaternionValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &quaternion, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant  QDeclarativeQuaternionValueType::value()
{
    return QVariant(quaternion);
}

void QDeclarativeQuaternionValueType::setValue(QVariant value)
{
    quaternion = qvariant_cast<QQuaternion>(value);
}

qreal QDeclarativeQuaternionValueType::scalar() const
{
    return quaternion.scalar();
}

qreal QDeclarativeQuaternionValueType::x() const
{
    return quaternion.x();
}

qreal QDeclarativeQuaternionValueType::y() const
{
    return quaternion.y();
}

qreal QDeclarativeQuaternionValueType::z() const
{
    return quaternion.z();
}

void QDeclarativeQuaternionValueType::setScalar(qreal scalar)
{
    quaternion.setScalar(scalar);
}

void QDeclarativeQuaternionValueType::setX(qreal x)
{
    quaternion.setX(x);
}

void QDeclarativeQuaternionValueType::setY(qreal y)
{
    quaternion.setY(y);
}

void QDeclarativeQuaternionValueType::setZ(qreal z)
{
    quaternion.setZ(z);
}

QDeclarativeMatrix4x4ValueType::QDeclarativeMatrix4x4ValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeMatrix4x4ValueType::read(QObject *obj, int idx)
{
    void *a[] = { &matrix, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeMatrix4x4ValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &matrix, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant  QDeclarativeMatrix4x4ValueType::value()
{
    return QVariant(matrix);
}

void QDeclarativeMatrix4x4ValueType::setValue(QVariant value)
{
    matrix = qvariant_cast<QMatrix4x4>(value);
}

QDeclarativeEasingValueType::QDeclarativeEasingValueType(QObject *parent)
: QDeclarativeValueType(parent)
{
}

void QDeclarativeEasingValueType::read(QObject *obj, int idx)
{
    void *a[] = { &easing, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QDeclarativeEasingValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &easing, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QDeclarativeEasingValueType::value()
{
    return QVariant(easing);
}

void QDeclarativeEasingValueType::setValue(QVariant value)
{
    easing = qvariant_cast<QEasingCurve>(value);
}

QDeclarativeEasingValueType::Type QDeclarativeEasingValueType::type() const
{
    return (QDeclarativeEasingValueType::Type)easing.type();
}

qreal QDeclarativeEasingValueType::amplitude() const
{
    return easing.amplitude();
}

qreal QDeclarativeEasingValueType::overshoot() const
{
    return easing.overshoot();
}

qreal QDeclarativeEasingValueType::period() const
{
    return easing.period();
}

void QDeclarativeEasingValueType::setType(QDeclarativeEasingValueType::Type type)
{
    easing.setType((QEasingCurve::Type)type);
}

void QDeclarativeEasingValueType::setAmplitude(qreal amplitude)
{
    easing.setAmplitude(amplitude);
}

void QDeclarativeEasingValueType::setOvershoot(qreal overshoot)
{
    easing.setOvershoot(overshoot);
}

void QDeclarativeEasingValueType::setPeriod(qreal period)
{
    easing.setPeriod(period);
}

QDeclarativeFontValueType::QDeclarativeFontValueType(QObject *parent)
: QDeclarativeValueType(parent), pixelSizeSet(false), pointSizeSet(false)
{
}

void QDeclarativeFontValueType::read(QObject *obj, int idx)
{
    void *a[] = { &font, 0 };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
    pixelSizeSet = false;
    pointSizeSet = false;
}

void QDeclarativeFontValueType::write(QObject *obj, int idx, QDeclarativePropertyPrivate::WriteFlags flags)
{
    int status = -1;
    void *a[] = { &font, 0, &status, &flags };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant  QDeclarativeFontValueType::value()
{
    return QVariant(font);
}

void QDeclarativeFontValueType::setValue(QVariant value)
{
    font = qvariant_cast<QFont>(value);
}


QString QDeclarativeFontValueType::family() const
{
    return font.family();
}

void QDeclarativeFontValueType::setFamily(const QString &family)
{
    font.setFamily(family);
}

bool QDeclarativeFontValueType::bold() const
{
    return font.bold();
}

void QDeclarativeFontValueType::setBold(bool b)
{
    font.setBold(b);
}

QDeclarativeFontValueType::FontWeight QDeclarativeFontValueType::weight() const
{
    return (QDeclarativeFontValueType::FontWeight)font.weight();
}

void QDeclarativeFontValueType::setWeight(QDeclarativeFontValueType::FontWeight w)
{
    font.setWeight((QFont::Weight)w);
}

bool QDeclarativeFontValueType::italic() const
{
    return font.italic();
}

void QDeclarativeFontValueType::setItalic(bool b)
{
    font.setItalic(b);
}

bool QDeclarativeFontValueType::underline() const
{
    return font.underline();
}

void QDeclarativeFontValueType::setUnderline(bool b)
{
    font.setUnderline(b);
}

bool QDeclarativeFontValueType::overline() const
{
    return font.overline();
}

void QDeclarativeFontValueType::setOverline(bool b)
{
    font.setOverline(b);
}

bool QDeclarativeFontValueType::strikeout() const
{
    return font.strikeOut();
}

void QDeclarativeFontValueType::setStrikeout(bool b)
{
    font.setStrikeOut(b);
}

qreal QDeclarativeFontValueType::pointSize() const
{
    if (font.pointSizeF() == -1) {
        if (dpi.isNull)
            dpi = qt_defaultDpi();
        return font.pixelSize() * qreal(72.) / qreal(dpi);
    }
    return font.pointSizeF();
}

void QDeclarativeFontValueType::setPointSize(qreal size)
{
    if (pixelSizeSet) {
        qWarning() << "Both point size and pixel size set. Using pixel size.";
        return;
    }

    if (size >= 0.0) {
        pointSizeSet = true;
        font.setPointSizeF(size);
    } else {
        pointSizeSet = false;
    }
}

int QDeclarativeFontValueType::pixelSize() const
{
    if (font.pixelSize() == -1) {
        if (dpi.isNull)
            dpi = qt_defaultDpi();
        return (font.pointSizeF() * dpi) / qreal(72.);
    }
    return font.pixelSize();
}

void QDeclarativeFontValueType::setPixelSize(int size)
{
    if (size >0) {
        if (pointSizeSet)
            qWarning() << "Both point size and pixel size set. Using pixel size.";
        font.setPixelSize(size);
        pixelSizeSet = true;
    } else {
        pixelSizeSet = false;
    }
}

QDeclarativeFontValueType::Capitalization QDeclarativeFontValueType::capitalization() const
{
    return (QDeclarativeFontValueType::Capitalization)font.capitalization();
}

void QDeclarativeFontValueType::setCapitalization(QDeclarativeFontValueType::Capitalization c)
{
    font.setCapitalization((QFont::Capitalization)c);
}

qreal QDeclarativeFontValueType::letterSpacing() const
{
    return font.letterSpacing();
}

void QDeclarativeFontValueType::setLetterSpacing(qreal size)
{
    font.setLetterSpacing(QFont::AbsoluteSpacing, size);
}

qreal QDeclarativeFontValueType::wordSpacing() const
{
    return font.wordSpacing();
}

void QDeclarativeFontValueType::setWordSpacing(qreal size)
{
    font.setWordSpacing(size);
}

QT_END_NAMESPACE
