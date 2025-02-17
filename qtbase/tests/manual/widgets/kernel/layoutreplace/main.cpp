/****************************************************************************
**
** Copyright (C) 2013 Thorbjørn Martsum - tmartsum[at]gmail.com
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

#include <QGridLayout>
#include <QFormLayout>
#include <QStackedLayout>
#include <QPushButton>
#include <QLabel>
#include <QApplication>

class ReplaceButton : public QPushButton
{
    Q_OBJECT
public:
    ReplaceButton(const QString &text = QString("click to replace")) : QPushButton(text)
    {
        static int v = 0;
        ++v;
        QString txt = QString("click to replace %1").arg(v);
        setText(txt);
        connect(this, SIGNAL(clicked()), this, SLOT(replace()));
    }
protected slots:
    void replace()
    {
        if (!parentWidget())
            return;
        static int n = 0;
        ++n;
        if (parentWidget()->layout()->replaceWidget(this, new QLabel(QString("replaced(%1)").arg(n))))
            deleteLater();
    }
};

class StackButtonChange : public QPushButton
{
    Q_OBJECT
public:
    StackButtonChange(QStackedLayout *l) : QPushButton("stack wdg change")
    {
        sl = l;
        connect(this, SIGNAL(clicked()), this, SLOT(changeWdg()));
    }
protected slots:
    void changeWdg()
    {
        int index = sl->indexOf(sl->currentWidget());
        ++index;
        if (index >= sl->count())
            index = 0;
        sl->setCurrentWidget(sl->itemAt(index)->widget());
        sl->parentWidget()->update();
    }
protected:
    QStackedLayout *sl;
};

int main(int argc, char **argv)                                                                                                                                                               {
    QApplication app(argc, argv);
    QWidget wdg1;
    QGridLayout *l1 = new QGridLayout();
    l1->addWidget(new ReplaceButton(), 1, 1, 2, 2, Qt::AlignCenter);
    l1->addWidget(new ReplaceButton(), 3, 1, 1, 1, Qt::AlignRight);
    l1->addWidget(new ReplaceButton(), 1, 3, 1, 1, Qt::AlignLeft);
    l1->addWidget(new ReplaceButton(), 2, 3, 1, 1, Qt::AlignLeft);
    l1->addWidget(new ReplaceButton(), 3, 2, 1, 1, Qt::AlignRight);
    wdg1.setLayout(l1);
    wdg1.setWindowTitle("QGridLayout");
    wdg1.setGeometry(100, 100, 100, 100);
    wdg1.show();

    QWidget wdg2;
    QFormLayout *l2 = new QFormLayout();
    l2->addRow(QString("Label1"), new ReplaceButton());
    l2->addRow(QString("Label2"), new ReplaceButton());
    l2->addRow(new ReplaceButton(), new ReplaceButton());
    wdg2.setLayout(l2);
    wdg2.setWindowTitle("QFormLayout");
    wdg2.setGeometry(100 + wdg1.sizeHint().width() + 5, 100, 100, 100);
    wdg2.show();

    QWidget wdg3;
    QBoxLayout *l3 = new QVBoxLayout(); // new QHBoxLayout()
    QStackedLayout *sl = new QStackedLayout();
    sl->addWidget(new ReplaceButton());
    sl->addWidget(new ReplaceButton());
    sl->addWidget(new ReplaceButton());
    l3->addLayout(sl);
    l3->addWidget(new StackButtonChange(sl));
    l3->addWidget(new ReplaceButton());
    l3->addWidget(new ReplaceButton());
    wdg3.setLayout(l3);
    wdg3.setWindowTitle("QStackedLayout + BoxLayout");
    wdg3.setGeometry(100, 100 + wdg1.sizeHint().height() + 30, 100 , 100);
    wdg3.show();

    app.exec();
}
#include "main.moc"
