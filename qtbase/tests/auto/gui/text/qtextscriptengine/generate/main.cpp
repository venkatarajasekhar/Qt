/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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


#include <QApplication>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QFontDialog>
#include <QPushButton>

#define private public
#include <qfont.h>
#include <private/qtextengine_p.h>
#include <private/qfontengine_p.h>
#include <qtextlayout.h>
#undef private


class MyEdit : public QTextEdit {
    Q_OBJECT
public:
    MyEdit(QWidget *p) : QTextEdit(p) { setReadOnly(true); }
public slots:
    void setText(const QString &str);
    void changeFont();
public:
    QLineEdit *lineEdit;
};

void MyEdit::setText(const QString &str)
{
    if (str.isEmpty()) {
        clear();
        return;
    }
    QTextLayout layout(str, lineEdit->font());
    QTextEngine *e = layout.d;
    e->itemize();
    e->shape(0);

    QString result;
    result = "Using font '" + e->fontEngine(e->layoutData->items[0])->fontDef.family + "'\n\n";

    result += "{ { ";
    for (int i = 0; i < str.length(); ++i)
        result += "0x" + QString::number(str.at(i).unicode(), 16) + ", ";
    result += "0x0 },\n  { ";
    for (int i = 0; i < e->layoutData->items[0].num_glyphs; ++i)
        result += "0x" + QString::number(e->layoutData->glyphLayout.glyphs[i], 16) + ", ";
    result += "0x0 } }";

    setPlainText(result);
}

void MyEdit::changeFont()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok, lineEdit->font(), topLevelWidget());
    if (ok)
        lineEdit->setFont(f);
}

int main(int argc,  char **argv)
{
    QApplication a(argc, argv);

    QWidget *mw = new QWidget(0);
    QVBoxLayout *l = new QVBoxLayout(mw);

    QLineEdit *le = new QLineEdit(mw);
    l->addWidget(le);

    MyEdit *view = new MyEdit(mw);
    view->lineEdit = le;
    l->addWidget(view);

    QPushButton *button = new QPushButton("Change Font", mw);
    l->addWidget(button);

    QObject::connect(le, SIGNAL(textChanged(QString)), view, SLOT(setText(QString)));
    QObject::connect(button, SIGNAL(clicked()), view, SLOT(changeFont()));

    mw->resize(500, 300);
    mw->show();

    return a.exec();
}


#include <main.moc>
