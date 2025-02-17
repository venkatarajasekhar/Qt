/****************************************************************************
**
** Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QLineEdit>
#include <QApplication>
#include <QTableView>
#include <QStandardItemModel>
#include <QItemDelegate>
#include <QDebug>
#include <QComboBox>

class ExampleEditor : public QLineEdit
{
public:
    ExampleEditor(QWidget *parent = 0):QLineEdit(parent) { qDebug() << "ctor"; }
    ~ExampleEditor() { QApplication::instance()->quit(); }
};

class ExampleDelegate : public QItemDelegate
{
public:
    ExampleDelegate() : QItemDelegate()
    {
        m_editor = new ExampleEditor(0);
        m_combobox = new QComboBox(0);
        m_combobox->addItem(QString::fromUtf8("item1"));
        m_combobox->addItem(QString::fromUtf8("item2"));
    }
protected:
    QWidget* createEditor(QWidget *p, const QStyleOptionViewItem &o, const QModelIndex &i) const
    {
        // doubleclick rownumber 3 (last row) to see the difference.
        if (i.row() == 3) {
            m_combobox->setParent(p);
            m_combobox->setGeometry(o.rect);
            return m_combobox;
        } else {
            m_editor->setParent(p);
            m_editor->setGeometry(o.rect);
            return m_editor;
        }
    }
    void destroyEditor(QWidget *editor, const QModelIndex &) const
    {
        editor->setParent(0);
        qDebug() << "intercepted destroy :)";
    }

    // Avoid setting data - and therefore show that the editor keeps its state.
    void setEditorData(QWidget* w, const QModelIndex &) const
    {
        QComboBox *combobox = qobject_cast<QComboBox*>(w);
        if (combobox) {
            qDebug() << "Try to show popup at once";
            // Now we could try to make a call to
            // QCoreApplication::processEvents();
            // But it does not matter. The fix:
            // https://codereview.qt-project.org/40608
            // is blocking QComboBox from reacting to this doubleclick edit event
            // and we need to do that since the mouseReleaseEvent has not yet happened,
            // and therefore cannot be processed.
            combobox->showPopup();
        }
    }

    ~ExampleDelegate() { delete m_editor; }
    mutable ExampleEditor *m_editor;
    mutable QComboBox *m_combobox;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTableView tv;
    QStandardItemModel m;
    m.setRowCount(4);
    m.setColumnCount(2);
    tv.setModel(&m);
    tv.show();
    tv.setItemDelegate(new ExampleDelegate());
    app.exec();
}
