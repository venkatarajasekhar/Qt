/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qcursor.h"

#ifndef QT_NO_CURSOR

#include <qcoreapplication.h>
#include <qbitmap.h>
#include <qimage.h>
#include <qdatastream.h>
#include <qvariant.h>
#include <private/qcursor_p.h>
#include <qdebug.h>

#include <qpa/qplatformcursor.h>
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QCursor

    \brief The QCursor class provides a mouse cursor with an arbitrary
    shape.

    \inmodule QtGui
    \ingroup appearance
    \ingroup shared


    This class is mainly used to create mouse cursors that are
    associated with particular widgets and to get and set the position
    of the mouse cursor.

    Qt has a number of standard cursor shapes, but you can also make
    custom cursor shapes based on a QBitmap, a mask and a hotspot.

    To associate a cursor with a widget, use QWidget::setCursor(). To
    associate a cursor with all widgets (normally for a short period
    of time), use QGuiApplication::setOverrideCursor().

    To set a cursor shape use QCursor::setShape() or use the QCursor
    constructor which takes the shape as argument, or you can use one
    of the predefined cursors defined in the \l Qt::CursorShape enum.

    If you want to create a cursor with your own bitmap, either use
    the QCursor constructor which takes a bitmap and a mask or the
    constructor which takes a pixmap as arguments.

    To set or get the position of the mouse cursor use the static
    methods QCursor::pos() and QCursor::setPos().

    \b{Note:} It is possible to create a QCursor before
    QGuiApplication, but it is not useful except as a place-holder for a
    real QCursor created after QGuiApplication. Attempting to use a
    QCursor that was created before QGuiApplication will result in a
    crash.

    \section1 A Note for X11 Users

    On X11, Qt supports the \l{Xcursor}{Xcursor}
    library, which allows for full color icon themes. The table below
    shows the cursor name used for each Qt::CursorShape value. If a
    cursor cannot be found using the name shown below, a standard X11
    cursor will be used instead. Note: X11 does not provide
    appropriate cursors for all possible Qt::CursorShape values. It
    is possible that some cursors will be taken from the Xcursor
    theme, while others will use an internal bitmap cursor.

    \table
    \header \li Shape \li Qt::CursorShape Value \li Cursor Name
            \li Shape \li Qt::CursorShape Value \li Cursor Name
    \row \li \inlineimage cursor-arrow.png
         \li Qt::ArrowCursor   \li \c left_ptr
         \li \inlineimage      cursor-sizev.png
         \li Qt::SizeVerCursor \li \c size_ver
    \row \li \inlineimage      cursor-uparrow.png
         \li Qt::UpArrowCursor \li \c up_arrow
         \li \inlineimage      cursor-sizeh.png
         \li Qt::SizeHorCursor \li \c size_hor
    \row \li \inlineimage      cursor-cross.png
         \li Qt::CrossCursor   \li \c cross
         \li \inlineimage      cursor-sizeb.png
         \li Qt::SizeBDiagCursor \li \c size_bdiag
    \row \li \inlineimage      cursor-ibeam.png
         \li Qt::IBeamCursor   \li \c ibeam
         \li \inlineimage      cursor-sizef.png
         \li Qt::SizeFDiagCursor \li \c size_fdiag
    \row \li \inlineimage      cursor-wait.png
         \li Qt::WaitCursor    \li \c wait
         \li \inlineimage      cursor-sizeall.png
         \li Qt::SizeAllCursor \li \c size_all
    \row \li \inlineimage      cursor-busy.png
         \li Qt::BusyCursor    \li \c left_ptr_watch
         \li \inlineimage      cursor-vsplit.png
         \li Qt::SplitVCursor  \li \c split_v
    \row \li \inlineimage      cursor-forbidden.png
         \li Qt::ForbiddenCursor \li \c forbidden
         \li \inlineimage      cursor-hsplit.png
         \li Qt::SplitHCursor  \li \c split_h
    \row \li \inlineimage      cursor-hand.png
         \li Qt::PointingHandCursor \li \c pointing_hand
         \li \inlineimage      cursor-openhand.png
         \li Qt::OpenHandCursor  \li \c openhand
    \row \li \inlineimage      cursor-whatsthis.png
         \li Qt::WhatsThisCursor \li \c whats_this
         \li \inlineimage      cursor-closedhand.png
         \li Qt::ClosedHandCursor \li \c closedhand
    \row \li
         \li Qt::DragMoveCursor      \li \c dnd-move or \c move
         \li
         \li Qt::DragCopyCursor      \li \c dnd-copy or \c copy
    \row \li
         \li Qt::DragLinkCursor      \li \c dnd-link or \c link
    \endtable

    \sa QWidget, {fowler}{GUI Design Handbook: Cursors}
*/

/*!
    \fn QCursor &QCursor::operator=(QCursor &&other)

    Move-assigns \a other to this QCursor instance.

    \since 5.2
*/

/*!
    \fn QPoint QCursor::pos(const QScreen *screen)

    Returns the position of the cursor (hot spot) of the \a screen
    in global screen coordinates.

    You can call QWidget::mapFromGlobal() to translate it to widget
    coordinates.

    \sa setPos(), QWidget::mapFromGlobal(), QWidget::mapToGlobal()
*/
QPoint QCursor::pos(const QScreen *screen)
{
    if (screen)
        if (const QPlatformCursor *cursor = screen->handle()->cursor())
            return cursor->pos();
    return QGuiApplicationPrivate::lastCursorPosition.toPoint();
}

/*!
    \fn QPoint QCursor::pos()

    Returns the position of the cursor (hot spot) of
    the primary screen in global screen coordinates.

    You can call QWidget::mapFromGlobal() to translate it to widget
    coordinates.

    \note The position is queried from the windowing system. If mouse events are generated
    via other means (e.g., via QWindowSystemInterface in a unit test), those fake mouse
    moves will not be reflected in the returned value.

    \note On platforms where there is no windowing system or cursors are not available, the returned
    position is based on the mouse move events generated via QWindowSystemInterface.

    \sa setPos(), QWidget::mapFromGlobal(), QWidget::mapToGlobal(), QGuiApplication::primaryScreen()
*/
QPoint QCursor::pos()
{
    return QCursor::pos(QGuiApplication::primaryScreen());
}

/*!
    \fn void QCursor::setPos(QScreen *screen, int x, int y)

    Moves the cursor (hot spot) of the \a screen to the global
    screen position (\a x, \a y).

    You can call QWidget::mapToGlobal() to translate widget
    coordinates to global screen coordinates.

    \note Calling this function results in changing the cursor position through the windowing
    system. The windowing system will typically respond by sending mouse events to the application's
    window. This means that the usage of this function should be avoided in unit tests and
    everywhere where fake mouse events are being injected via QWindowSystemInterface because the
    windowing system's mouse state (with regards to buttons for example) may not match the state in
    the application-generated events.

    \note On platforms where there is no windowing system or cursors are not available, this
    function may do nothing.

    \sa pos(), QWidget::mapFromGlobal(), QWidget::mapToGlobal()
*/
void QCursor::setPos(QScreen *screen, int x, int y)
{
    if (screen) {
        if (QPlatformCursor *cursor = screen->handle()->cursor()) {
            const QPoint pos = QPoint(x, y);
            // Need to check, since some X servers generate null mouse move
            // events, causing looping in applications which call setPos() on
            // every mouse move event.
            if (pos != cursor->pos())
                cursor->setPos(pos);
        }
    }
}

/*!
    \fn void QCursor::setPos(int x, int y)

    Moves the cursor (hot spot) of the primary screen
    to the global screen position (\a x, \a y).

    You can call QWidget::mapToGlobal() to translate widget
    coordinates to global screen coordinates.

    \sa pos(), QWidget::mapFromGlobal(), QWidget::mapToGlobal(), QGuiApplication::primaryScreen()
*/
void QCursor::setPos(int x, int y)
{
    QCursor::setPos(QGuiApplication::primaryScreen(), x, y);
}

/*!
    \fn void QCursor::setPos (const QPoint &p)

    \overload

    Moves the cursor (hot spot) to the global screen position at point
    \a p.
*/

/*!
    \fn void QCursor::setPos (QScreen *screen,const QPoint &p)

    \overload

    Moves the cursor (hot spot) to the global screen position of the
    \a screen at point \a p.
*/

/*****************************************************************************
  QCursor stream functions
 *****************************************************************************/

#ifndef QT_NO_DATASTREAM


/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QCursor &cursor)
    \relates QCursor

    Writes the \a cursor to the \a stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QCursor &c)
{
    s << (qint16)c.shape();                        // write shape id to stream
    if (c.shape() == Qt::BitmapCursor) {                // bitmap cursor
        bool isPixmap = false;
        if (s.version() >= 7) {
            isPixmap = !c.pixmap().isNull();
            s << isPixmap;
        }
        if (isPixmap)
            s << c.pixmap();
        else
            s << *c.bitmap() << *c.mask();
        s << c.hotSpot();
    }
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QCursor &cursor)
    \relates QCursor

    Reads the \a cursor from the \a stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QCursor &c)
{
    qint16 shape;
    s >> shape;                                        // read shape id from stream
    if (shape == Qt::BitmapCursor) {                // read bitmap cursor
        bool isPixmap = false;
        if (s.version() >= 7)
            s >> isPixmap;
        if (isPixmap) {
            QPixmap pm;
            QPoint hot;
            s >> pm >> hot;
            c = QCursor(pm, hot.x(), hot.y());
        } else {
            QBitmap bm, bmm;
            QPoint hot;
            s >> bm >> bmm >> hot;
            c = QCursor(bm, bmm, hot.x(), hot.y());
        }
    } else {
        c.setShape((Qt::CursorShape)shape);                // create cursor with shape
    }
    return s;
}
#endif // QT_NO_DATASTREAM


/*!
    Constructs a custom pixmap cursor.

    \a pixmap is the image. It is usual to give it a mask (set using
    QPixmap::setMask()). \a hotX and \a hotY define the cursor's hot
    spot.

    If \a hotX is negative, it is set to the \c{pixmap().width()/2}.
    If \a hotY is negative, it is set to the \c{pixmap().height()/2}.

    Valid cursor sizes depend on the display hardware (or the
    underlying window system). We recommend using 32 x 32 cursors,
    because this size is supported on all platforms. Some platforms
    also support 16 x 16, 48 x 48, and 64 x 64 cursors.

    \note On Windows CE, the cursor size is fixed. If the pixmap
    is bigger than the system size, it will be scaled.

    \sa QPixmap::QPixmap(), QPixmap::setMask()
*/

QCursor::QCursor(const QPixmap &pixmap, int hotX, int hotY)
    : d(0)
{
    QImage img = pixmap.toImage().convertToFormat(QImage::Format_Indexed8, Qt::ThresholdDither|Qt::AvoidDither);
    QBitmap bm = QBitmap::fromImage(img, Qt::ThresholdDither|Qt::AvoidDither);
    QBitmap bmm = pixmap.mask();
    if (!bmm.isNull()) {
        QBitmap nullBm;
        bm.setMask(nullBm);
    }
    else if (!pixmap.mask().isNull()) {
        QImage mimg = pixmap.mask().toImage().convertToFormat(QImage::Format_Indexed8, Qt::ThresholdDither|Qt::AvoidDither);
        bmm = QBitmap::fromImage(mimg, Qt::ThresholdDither|Qt::AvoidDither);
    }
    else {
        bmm = QBitmap(bm.size());
        bmm.fill(Qt::color1);
    }

    d = QCursorData::setBitmap(bm, bmm, hotX, hotY);
    d->pixmap = pixmap;
}



/*!
    Constructs a custom bitmap cursor.

    \a bitmap and
    \a mask make up the bitmap.
    \a hotX and
    \a hotY define the cursor's hot spot.

    If \a hotX is negative, it is set to the \c{bitmap().width()/2}.
    If \a hotY is negative, it is set to the \c{bitmap().height()/2}.

    The cursor \a bitmap (B) and \a mask (M) bits are combined like this:
    \list
    \li B=1 and M=1 gives black.
    \li B=0 and M=1 gives white.
    \li B=0 and M=0 gives transparent.
    \li B=1 and M=0 gives an XOR'd result under Windows, undefined
    results on all other platforms.
    \endlist

    Use the global Qt color Qt::color0 to draw 0-pixels and Qt::color1 to
    draw 1-pixels in the bitmaps.

    Valid cursor sizes depend on the display hardware (or the
    underlying window system). We recommend using 32 x 32 cursors,
    because this size is supported on all platforms. Some platforms
    also support 16 x 16, 48 x 48, and 64 x 64 cursors.

    \note On Windows CE, the cursor size is fixed. If the pixmap
    is bigger than the system size, it will be scaled.

    \sa QBitmap::QBitmap(), QBitmap::setMask()
*/

QCursor::QCursor(const QBitmap &bitmap, const QBitmap &mask, int hotX, int hotY)
    : d(0)
{
    d = QCursorData::setBitmap(bitmap, mask, hotX, hotY);
}

/*!
    Constructs a cursor with the default arrow shape.
*/
QCursor::QCursor()
{
    if (!QCursorData::initialized) {
        if (QCoreApplication::startingUp()) {
            d = 0;
            return;
        }
        QCursorData::initialize();
    }
    QCursorData *c = qt_cursorTable[0];
    c->ref.ref();
    d = c;
}

/*!
    Constructs a cursor with the specified \a shape.

    See \l Qt::CursorShape for a list of shapes.

    \sa setShape()
*/
QCursor::QCursor(Qt::CursorShape shape)
    : d(0)
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    setShape(shape);
}


/*!
    Returns the cursor shape identifier. The return value is one of
    the \l Qt::CursorShape enum values (cast to an int).

    \sa setShape()
*/
Qt::CursorShape QCursor::shape() const
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    return d->cshape;
}

/*!
    Sets the cursor to the shape identified by \a shape.

    See \l Qt::CursorShape for the list of cursor shapes.

    \sa shape()
*/
void QCursor::setShape(Qt::CursorShape shape)
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    QCursorData *c = uint(shape) <= Qt::LastCursor ? qt_cursorTable[shape] : 0;
    if (!c)
        c = qt_cursorTable[0];
    c->ref.ref();
    if (!d) {
        d = c;
    } else {
        if (!d->ref.deref())
            delete d;
        d = c;
    }
}

/*!
    Returns the cursor bitmap, or 0 if it is one of the standard
    cursors.
*/
const QBitmap *QCursor::bitmap() const
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    return d->bm;
}

/*!
    Returns the cursor bitmap mask, or 0 if it is one of the standard
    cursors.
*/

const QBitmap *QCursor::mask() const
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    return d->bmm;
}

/*!
    Returns the cursor pixmap. This is only valid if the cursor is a
    pixmap cursor.
*/

QPixmap QCursor::pixmap() const
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    return d->pixmap;
}

/*!
    Returns the cursor hot spot, or (0, 0) if it is one of the
    standard cursors.
*/

QPoint QCursor::hotSpot() const
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    return QPoint(d->hx, d->hy);
}

/*!
    Constructs a copy of the cursor \a c.
*/

QCursor::QCursor(const QCursor &c)
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    d = c.d;
    d->ref.ref();
}

/*!
    Destroys the cursor.
*/

QCursor::~QCursor()
{
    if (d && !d->ref.deref())
        delete d;
}


/*!
    Assigns \a c to this cursor and returns a reference to this
    cursor.
*/

QCursor &QCursor::operator=(const QCursor &c)
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    if (c.d)
        c.d->ref.ref();
    if (d && !d->ref.deref())
        delete d;
    d = c.d;
    return *this;
}

/*!
   Returns the cursor as a QVariant.
*/
QCursor::operator QVariant() const
{
    return QVariant(QVariant::Cursor, this);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QCursor &c)
{
    dbg.nospace() << "QCursor(Qt::CursorShape(" << c.shape() << "))";
    return dbg.space();
}
#endif

/*****************************************************************************
  Internal QCursorData class
 *****************************************************************************/

QCursorData *qt_cursorTable[Qt::LastCursor + 1];
bool QCursorData::initialized = false;

QCursorData::QCursorData(Qt::CursorShape s)
    : ref(1), cshape(s), bm(0), bmm(0), hx(0), hy(0)
{
}

QCursorData::~QCursorData()
{
    delete bm;
    delete bmm;
}

/*! \internal */
void QCursorData::cleanup()
{
    if(!QCursorData::initialized)
        return;

    for (int shape = 0; shape <= Qt::LastCursor; ++shape) {
        // In case someone has a static QCursor defined with this shape
        if (!qt_cursorTable[shape]->ref.deref())
            delete qt_cursorTable[shape];
        qt_cursorTable[shape] = 0;
    }
    QCursorData::initialized = false;
}

/*! \internal */
void QCursorData::initialize()
{
    if (QCursorData::initialized)
        return;
    for (int shape = 0; shape <= Qt::LastCursor; ++shape)
        qt_cursorTable[shape] = new QCursorData((Qt::CursorShape)shape);
    QCursorData::initialized = true;
}

QCursorData *QCursorData::setBitmap(const QBitmap &bitmap, const QBitmap &mask, int hotX, int hotY)
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    if (bitmap.depth() != 1 || mask.depth() != 1 || bitmap.size() != mask.size()) {
        qWarning("QCursor: Cannot create bitmap cursor; invalid bitmap(s)");
        QCursorData *c = qt_cursorTable[0];
        c->ref.ref();
        return c;
    }
    QCursorData *d = new QCursorData;
    d->bm  = new QBitmap(bitmap);
    d->bmm = new QBitmap(mask);
    d->cshape = Qt::BitmapCursor;
    d->hx = hotX >= 0 ? hotX : bitmap.width() / 2;
    d->hy = hotY >= 0 ? hotY : bitmap.height() / 2;

    return d;
}

void QCursorData::update()
{
}

QT_END_NAMESPACE
#endif // QT_NO_CURSOR

