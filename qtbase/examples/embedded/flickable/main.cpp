/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

#include <QtCore>
#include <QtWidgets>

#include "flickable.h"

// Returns a list of two-word color names
static QStringList colorPairs(int max)
{
    // capitalize the first letter
    QStringList colors = QColor::colorNames();
    colors.removeAll("transparent");
    int num = colors.count();
    for (int c = 0; c < num; ++c)
        colors[c] = colors[c][0].toUpper() + colors[c].mid(1);

    // combine two colors, e.g. "lime skyblue"
    QStringList combinedColors;
    for (int i = 0; i < num; ++i)
        for (int j = 0; j < num; ++j)
            combinedColors << QString("%1 %2").arg(colors[i]).arg(colors[j]);

    // randomize it
    colors.clear();
    while (combinedColors.count()) {
        int i = qrand() % combinedColors.count();
        colors << combinedColors[i];
        combinedColors.removeAt(i);
        if (colors.count() == max)
            break;
    }

    return colors;
}

class ColorList : public QWidget, public Flickable
{
    Q_OBJECT

public:
    ColorList(QWidget *parent = 0)
            : QWidget(parent) {
        m_offset = 0;
        m_height = QFontMetrics(font()).height() + 5;
        m_highlight = -1;
        m_selected = -1;

        QStringList colors = colorPairs(999);
        for (int i = 0; i < colors.count(); ++i) {
            QString c = colors[i];
            QString str;
            str.sprintf("%4d", i + 1);
            m_colorNames << (str + "   " + c);

            QStringList duet = c.split(' ');
            m_firstColor << duet[0];
            m_secondColor << duet[1];
        }

        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAttribute(Qt::WA_NoSystemBackground, true);

        setMouseTracking(true);
        Flickable::setAcceptMouseClick(this);
    }

protected:
    // reimplement from Flickable
    virtual QPoint scrollOffset() const {
        return QPoint(0, m_offset);
    }

    // reimplement from Flickable
    virtual void setScrollOffset(const QPoint &offset) {
        int yy = offset.y();
        if (yy != m_offset) {
            m_offset = qBound(0, yy, m_height * m_colorNames.count() - height());
            update();
        }
    }

protected:
    void paintEvent(QPaintEvent *event) {
        QPainter p(this);
        p.fillRect(event->rect(), Qt::white);
        int start = m_offset / m_height;
        int y = start * m_height - m_offset;
        if (m_offset <= 0) {
            start = 0;
            y = -m_offset;
        }
        int end = start + height() / m_height + 1;
        if (end > m_colorNames.count() - 1)
            end = m_colorNames.count() - 1;
        for (int i = start; i <= end; ++i, y += m_height) {

            p.setBrush(Qt::NoBrush);
            p.setPen(Qt::black);
            if (i == m_highlight) {
                p.fillRect(0, y, width(), m_height, QColor(0, 64, 128));
                p.setPen(Qt::white);
            }
            if (i == m_selected) {
                p.fillRect(0, y, width(), m_height, QColor(0, 128, 240));
                p.setPen(Qt::white);
            }

            p.drawText(m_height + 2, y, width(), m_height, Qt::AlignVCenter, m_colorNames[i]);

            p.setPen(Qt::NoPen);
            p.setBrush(m_firstColor[i]);
            p.drawRect(1, y + 1, m_height - 2, m_height - 2);
            p.setBrush(m_secondColor[i]);
            p.drawRect(5, y + 5, m_height - 11, m_height - 11);
        }
        p.end();
    }

    void keyReleaseEvent(QKeyEvent *event) {
        if (event->key() == Qt::Key_Down) {
            m_offset += 20;
            event->accept();
            update();
            return;
        }
        if (event->key() == Qt::Key_Up) {
            m_offset -= 20;
            event->accept();
            update();
            return;
        }
    }

    void mousePressEvent(QMouseEvent *event) {
        Flickable::handleMousePress(event);
        if (event->isAccepted())
            return;

        if (event->button() == Qt::LeftButton) {
            int y = event->pos().y() + m_offset;
            int i = y / m_height;
            if (i != m_highlight) {
                m_highlight = i;
                m_selected = -1;
                update();
            }
            event->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent *event) {
        Flickable::handleMouseMove(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) {
        Flickable::handleMouseRelease(event);
        if (event->isAccepted())
            return;

        if (event->button() == Qt::LeftButton) {
            m_selected = m_highlight;
            event->accept();
            update();
        }
    }

private:
    int m_offset;
    int m_height;
    int m_highlight;
    int m_selected;
    QStringList m_colorNames;
    QList<QColor> m_firstColor;
    QList<QColor> m_secondColor;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ColorList list;
    list.setWindowTitle("Kinetic Scrolling");
    list.resize(320, 320);
    list.show();

    return app.exec();
}
