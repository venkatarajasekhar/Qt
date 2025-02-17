/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Linguist of the Qt Toolkit.
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

/*  TRANSLATOR MessageEditor

  This is the right panel of the main window.
*/

#include "messageeditor.h"
#include "messageeditorwidgets.h"
#include "simtexth.h"
#include "phrasemodel.h"

#include <QApplication>
#include <QBoxLayout>
#ifndef QT_NO_CLIPBOARD
#include <QClipboard>
#endif
#include <QDebug>
#include <QDockWidget>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMainWindow>
#include <QPainter>
#include <QTreeView>
#include <QVBoxLayout>

QT_BEGIN_NAMESPACE

#ifdef NEVER_TRUE
// Allow translators to provide localized names for QLocale::languageToString
// At least the own language should be translated ... This is a "hack" until
// functionality is provided within Qt (see task 196275).
static const char * language_strings[] =
{
    QT_TRANSLATE_NOOP("MessageEditor", "Russian"),
    QT_TRANSLATE_NOOP("MessageEditor", "German"),
    QT_TRANSLATE_NOOP("MessageEditor", "Japanese"),
    QT_TRANSLATE_NOOP("MessageEditor", "French"),
    QT_TRANSLATE_NOOP("MessageEditor", "Polish"),
    QT_TRANSLATE_NOOP("MessageEditor", "Chinese")
};
#endif

/*
   MessageEditor class impl.

   Handles layout of dock windows and the editor page.
*/
MessageEditor::MessageEditor(MultiDataModel *dataModel, QMainWindow *parent)
    : QScrollArea(parent->centralWidget()),
      m_dataModel(dataModel),
      m_currentModel(-1),
      m_currentNumerus(-1),
      m_lengthVariants(false),
      m_fontSize(font().pointSize()),
      m_undoAvail(false),
      m_redoAvail(false),
      m_cutAvail(false),
      m_copyAvail(false),
      m_visualizeWhitespace(true),
      m_selectionHolder(0),
      m_focusWidget(0)
{
    setObjectName(QLatin1String("scroll area"));

    QPalette p;
    p.setBrush(QPalette::Window, p.brush(QPalette::Active, QPalette::Base));
    setPalette(p);

    setupEditorPage();

    // Signals
#ifndef QT_NO_CLIPBOARD
    connect(qApp->clipboard(), SIGNAL(dataChanged()),
            SLOT(clipboardChanged()));
#endif
    connect(m_dataModel, SIGNAL(modelAppended()),
            SLOT(messageModelAppended()));
    connect(m_dataModel, SIGNAL(modelDeleted(int)),
            SLOT(messageModelDeleted(int)));
    connect(m_dataModel, SIGNAL(allModelsDeleted()),
            SLOT(allModelsDeleted()));
    connect(m_dataModel, SIGNAL(languageChanged(int)),
            SLOT(setTargetLanguage(int)));

    m_tabOrderTimer.setSingleShot(true);
    connect(&m_tabOrderTimer, SIGNAL(timeout()), SLOT(reallyFixTabOrder()));

#ifndef QT_NO_CLIPBOARD
    clipboardChanged();
#endif

    setWhatsThis(tr("This whole panel allows you to view and edit "
                    "the translation of some source text."));
    showNothing();
}

void MessageEditor::setupEditorPage()
{
    QFrame *editorPage = new QFrame;
    editorPage->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

    m_source = new FormWidget(tr("Source text"), false);
    m_source->setHideWhenEmpty(true);
    m_source->setWhatsThis(tr("This area shows the source text."));
    connect(m_source, SIGNAL(selectionChanged(QTextEdit*)),
            SLOT(selectionChanged(QTextEdit*)));

    m_pluralSource = new FormWidget(tr("Source text (Plural)"), false);
    m_pluralSource->setHideWhenEmpty(true);
    m_pluralSource->setWhatsThis(tr("This area shows the plural form of the source text."));
    connect(m_pluralSource, SIGNAL(selectionChanged(QTextEdit*)),
            SLOT(selectionChanged(QTextEdit*)));

    m_commentText = new FormWidget(tr("Developer comments"), false);
    m_commentText->setHideWhenEmpty(true);
    m_commentText->setObjectName(QLatin1String("comment/context view"));
    m_commentText->setWhatsThis(tr("This area shows a comment that"
                        " may guide you, and the context in which the text"
                        " occurs.") );
    connect(m_commentText, SIGNAL(selectionChanged(QTextEdit*)),
            SLOT(selectionChanged(QTextEdit*)));

    QBoxLayout *subLayout = new QVBoxLayout;

    subLayout->setMargin(5);
    subLayout->addWidget(m_source);
    subLayout->addWidget(m_pluralSource);
    subLayout->addWidget(m_commentText);

    m_layout = new QVBoxLayout;
    m_layout->setSpacing(2);
    m_layout->setMargin(2);
    m_layout->addLayout(subLayout);
    m_layout->addStretch(1);
    editorPage->setLayout(m_layout);

    setWidget(editorPage);
    setWidgetResizable(true);
}

QPalette MessageEditor::paletteForModel(int model) const
{
    QBrush brush = m_dataModel->brushForModel(model);
    QPalette pal;

    if (m_dataModel->isModelWritable(model)) {
        pal.setBrush(QPalette::Window, brush);
    } else {
        QPixmap pm(brush.texture().size());
        pm.fill();
        QPainter p(&pm);
        p.fillRect(brush.texture().rect(), brush);
        pal.setBrush(QPalette::Window, pm);
    }
    return pal;
}

void MessageEditor::messageModelAppended()
{
    int model = m_editors.size();
    m_editors.append(MessageEditorData());
    MessageEditorData &ed = m_editors.last();
    ed.pluralEditMode = false;
    ed.fontSize = m_fontSize;
    ed.container = new QWidget;
    if (model > 0) {
        ed.container->setPalette(paletteForModel(model));
        ed.container->setAutoFillBackground(true);
        if (model == 1) {
            m_editors[0].container->setPalette(paletteForModel(0));
            m_editors[0].container->setAutoFillBackground(true);
        }
    }
    bool writable = m_dataModel->isModelWritable(model);
    ed.transCommentText = new FormWidget(QString(), true);
    ed.transCommentText->setEditingEnabled(writable);
    ed.transCommentText->setHideWhenEmpty(!writable);
    ed.transCommentText->setWhatsThis(tr("Here you can enter comments for your own use."
                        " They have no effect on the translated applications.") );
    ed.transCommentText->getEditor()->installEventFilter(this);
    ed.transCommentText->getEditor()->setVisualizeWhitespace(m_visualizeWhitespace);
    connect(ed.transCommentText, SIGNAL(selectionChanged(QTextEdit*)),
            SLOT(selectionChanged(QTextEdit*)));
    connect(ed.transCommentText, SIGNAL(textChanged(QTextEdit*)),
            SLOT(emitTranslatorCommentChanged(QTextEdit*)));
    connect(ed.transCommentText, SIGNAL(textChanged(QTextEdit*)), SLOT(resetHoverSelection()));
    connect(ed.transCommentText, SIGNAL(cursorPositionChanged()), SLOT(resetHoverSelection()));
    fixTabOrder();
    QBoxLayout *box = new QVBoxLayout(ed.container);
    box->setMargin(5);
    box->addWidget(ed.transCommentText);
    box->addSpacing(ed.transCommentText->getEditor()->fontMetrics().height() / 2);
    m_layout->addWidget(ed.container);
    setTargetLanguage(model);
}

void MessageEditor::allModelsDeleted()
{
    foreach (const MessageEditorData &med, m_editors)
        med.container->deleteLater();
    m_editors.clear();
    m_currentModel = -1;
    // Do not emit activeModelChanged() - the main window will refresh anyway
    m_currentNumerus = -1;
    showNothing();
}

void MessageEditor::messageModelDeleted(int model)
{
    m_editors[model].container->deleteLater();
    m_editors.removeAt(model);
    if (model <= m_currentModel) {
        if (model < m_currentModel || m_currentModel == m_editors.size())
            --m_currentModel;
        // Do not emit activeModelChanged() - the main window will refresh anyway
        if (m_currentModel >= 0) {
            if (m_currentNumerus >= m_editors[m_currentModel].transTexts.size())
                m_currentNumerus = m_editors[m_currentModel].transTexts.size() - 1;
            activeEditor()->setFocus();
        } else {
            m_currentNumerus = -1;
        }
    }
    if (m_editors.size() == 1) {
        m_editors[0].container->setAutoFillBackground(false);
    } else {
        for (int i = model; i < m_editors.size(); ++i)
            m_editors[i].container->setPalette(paletteForModel(i));
    }
}

void MessageEditor::addPluralForm(int model, const QString &label, bool writable)
{
    FormMultiWidget *transEditor = new FormMultiWidget(label);
    connect(transEditor, SIGNAL(editorCreated(QTextEdit*)), SLOT(editorCreated(QTextEdit*)));
    transEditor->setEditingEnabled(writable);
    transEditor->setHideWhenEmpty(!writable);
    if (!m_editors[model].transTexts.isEmpty())
        transEditor->setVisible(false);
    transEditor->setMultiEnabled(m_lengthVariants);
    static_cast<QBoxLayout *>(m_editors[model].container->layout())->insertWidget(
        m_editors[model].transTexts.count(), transEditor);

    connect(transEditor, SIGNAL(selectionChanged(QTextEdit*)),
            SLOT(selectionChanged(QTextEdit*)));
    connect(transEditor, SIGNAL(textChanged(QTextEdit*)),
            SLOT(emitTranslationChanged(QTextEdit*)));
    connect(transEditor, SIGNAL(textChanged(QTextEdit*)), SLOT(resetHoverSelection()));
    connect(transEditor, SIGNAL(cursorPositionChanged()), SLOT(resetHoverSelection()));

    m_editors[model].transTexts << transEditor;
}

void MessageEditor::editorCreated(QTextEdit *te)
{
    QFont font;
    font.setPointSize(static_cast<int>(m_fontSize));

    FormMultiWidget *snd = static_cast<FormMultiWidget *>(sender());
    for (int model = 0; ; ++model) {
        MessageEditorData med = m_editors.at(model);
        med.transCommentText->getEditor()->setFont(font);
        if (med.transTexts.contains(snd)) {
            te->setFont(font);

            te->installEventFilter(this);

            if (m_visualizeWhitespace) {
                QTextOption option = te->document()->defaultTextOption();

                option.setFlags(option.flags()
                                | QTextOption::ShowLineAndParagraphSeparators
                                | QTextOption::ShowTabsAndSpaces);
                te->document()->setDefaultTextOption(option);
            }

            fixTabOrder();
            return;
        }
    }
}

void MessageEditor::editorDestroyed()
{
    if (m_selectionHolder == sender())
        resetSelection();
}

void MessageEditor::fixTabOrder()
{
    m_tabOrderTimer.start(0);
}

void MessageEditor::reallyFixTabOrder()
{
    QWidget *prev = this;
    foreach (const MessageEditorData &med, m_editors) {
        foreach (FormMultiWidget *fmw, med.transTexts)
            foreach (QTextEdit *te, fmw->getEditors()) {
                setTabOrder(prev, te);
                prev = te;
            }
        QTextEdit *te = med.transCommentText->getEditor();
        setTabOrder(prev, te);
        prev = te;
    }
}

/*! internal
    Returns all translations for an item.
    The number of translations is dependent on if we have a plural form or not.
    If we don't have a plural form, then this should only contain one item.
    Otherwise it will contain the number of numerus forms for the particular language.
*/
QStringList MessageEditor::translations(int model) const
{
    QStringList translations;
    for (int i = 0; i < m_editors[model].transTexts.count() &&
                    m_editors[model].transTexts.at(i)->isVisible(); ++i)
        translations << m_editors[model].transTexts[i]->getTranslation();
    return translations;
}

static void clearSelection(QTextEdit *t)
{
    bool oldBlockState = t->blockSignals(true);
    QTextCursor c = t->textCursor();
    c.clearSelection();
    t->setTextCursor(c);
    t->blockSignals(oldBlockState);
}

void MessageEditor::selectionChanged(QTextEdit *te)
{
    if (te != m_selectionHolder) {
        if (m_selectionHolder) {
            clearSelection(m_selectionHolder);
            disconnect(this, SLOT(editorDestroyed()));
        }
        m_selectionHolder = (te->textCursor().hasSelection() ? te : 0);
        if (FormatTextEdit *fte = qobject_cast<FormatTextEdit*>(m_selectionHolder))
            connect(fte, SIGNAL(editorDestroyed()), SLOT(editorDestroyed()));
#ifndef QT_NO_CLIPBOARD
        updateCanCutCopy();
#endif
    }
}

void MessageEditor::resetHoverSelection()
{
    if (m_selectionHolder &&
        (m_selectionHolder == m_source->getEditor()
         || m_selectionHolder == m_pluralSource->getEditor()))
        resetSelection();
}

void MessageEditor::resetSelection()
{
    if (m_selectionHolder) {
        clearSelection(m_selectionHolder);
        disconnect(this, SLOT(editorDestroyed()));
        m_selectionHolder = 0;
#ifndef QT_NO_CLIPBOARD
        updateCanCutCopy();
#endif
    }
}

void MessageEditor::activeModelAndNumerus(int *model, int *numerus) const
{
    for (int j = 0; j < m_editors.count(); ++j) {
        for (int i = 0; i < m_editors[j].transTexts.count(); ++i)
            foreach (QTextEdit *te, m_editors[j].transTexts[i]->getEditors())
                if (m_focusWidget == te) {
                    *model = j;
                    *numerus = i;
                    return;
                }
        if (m_focusWidget == m_editors[j].transCommentText->getEditor()) {
            *model = j;
            *numerus = -1;
            return;
        }
    }
    *model = -1;
    *numerus = -1;
}

QTextEdit *MessageEditor::activeTranslation() const
{
    if (m_currentNumerus < 0)
        return 0;
    const QList<FormatTextEdit *> &editors =
            m_editors[m_currentModel].transTexts[m_currentNumerus]->getEditors();
    foreach (QTextEdit *te, editors)
        if (te->hasFocus())
            return te;
    return editors.first();
}

QTextEdit *MessageEditor::activeOr1stTranslation() const
{
    if (m_currentNumerus < 0) {
        for (int i = 0; i < m_editors.size(); ++i)
            if (m_editors[i].container->isVisible()
                && !m_editors[i].transTexts.first()->getEditors().first()->isReadOnly())
                return m_editors[i].transTexts.first()->getEditors().first();
        return 0;
    }
    return activeTranslation();
}

QTextEdit *MessageEditor::activeTransComment() const
{
    if (m_currentModel < 0 || m_currentNumerus >= 0)
        return 0;
    return m_editors[m_currentModel].transCommentText->getEditor();
}

QTextEdit *MessageEditor::activeEditor() const
{
    if (QTextEdit *te = activeTransComment())
        return te;
    return activeTranslation();
}

QTextEdit *MessageEditor::activeOr1stEditor() const
{
    if (QTextEdit *te = activeTransComment())
        return te;
    return activeOr1stTranslation();
}

void MessageEditor::setTargetLanguage(int model)
{
    const QStringList &numerusForms = m_dataModel->model(model)->numerusForms();
    const QString &langLocalized = m_dataModel->model(model)->localizedLanguage();
    for (int i = 0; i < numerusForms.count(); ++i) {
        const QString &label = tr("%1 translation (%2)").arg(langLocalized, numerusForms[i]);
        if (!i)
            m_editors[model].firstForm = label;
        if (i >= m_editors[model].transTexts.count())
            addPluralForm(model, label, m_dataModel->isModelWritable(model));
        else
            m_editors[model].transTexts[i]->setLabel(label);
        m_editors[model].transTexts[i]->setVisible(!i || m_editors[model].pluralEditMode);
        m_editors[model].transTexts[i]->setWhatsThis(
                tr("This is where you can enter or modify"
                   " the translation of the above source text.") );
    }
    for (int j = m_editors[model].transTexts.count() - numerusForms.count(); j > 0; --j)
        delete m_editors[model].transTexts.takeLast();
    m_editors[model].invariantForm = tr("%1 translation").arg(langLocalized);
    m_editors[model].transCommentText->setLabel(tr("%1 translator comments").arg(langLocalized));
}

MessageEditorData *MessageEditor::modelForWidget(const QObject *o)
{
    for (int j = 0; j < m_editors.count(); ++j) {
        for (int i = 0; i < m_editors[j].transTexts.count(); ++i)
            foreach (QTextEdit *te, m_editors[j].transTexts[i]->getEditors())
                if (te == o)
                    return &m_editors[j];
        if (m_editors[j].transCommentText->getEditor() == o)
            return &m_editors[j];
    }
    return 0;
}

bool MessageEditor::eventFilter(QObject *o, QEvent *e)
{
    // handle copying from the source
    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);

        if (ke->modifiers() & Qt::ControlModifier) {
#ifndef QT_NO_CLIPBOARD
            if (ke->key() == Qt::Key_C) {
                if (m_source->getEditor()->textCursor().hasSelection()) {
                    m_source->getEditor()->copy();
                    return true;
                }
                if (m_pluralSource->getEditor()->textCursor().hasSelection()) {
                    m_pluralSource->getEditor()->copy();
                    return true;
                }
            } else
#endif
                if (ke->key() == Qt::Key_A) {
                return true;
            }
        }
    } else if (e->type() == QEvent::KeyPress) {
        // Ctrl-Tab is still passed through to the textedit and causes a tab to be inserted.
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Tab &&
            !(ke->modifiers() & Qt::ControlModifier)) {
            focusNextChild();
            return true;
        }
    } else if (e->type() == QEvent::FocusIn) {
        QWidget *widget = static_cast<QWidget *>(o);
        if (widget != m_focusWidget)
            trackFocus(widget);
    }

    return QScrollArea::eventFilter(o, e);
}

void MessageEditor::grabFocus(QWidget *widget)
{
    if (widget != m_focusWidget) {
        widget->setFocus();
        trackFocus(widget);
    }
}

void MessageEditor::trackFocus(QWidget *widget)
{
    m_focusWidget = widget;

    int model, numerus;
    activeModelAndNumerus(&model, &numerus);
    if (model != m_currentModel || numerus != m_currentNumerus) {
        resetSelection();
        m_currentModel = model;
        m_currentNumerus = numerus;
        emit activeModelChanged(activeModel());
        updateBeginFromSource();
        updateUndoRedo();
#ifndef QT_NO_CLIPBOARD
        updateCanPaste();
#endif
    }
}

void MessageEditor::showNothing()
{
    m_source->clearTranslation();
    m_pluralSource->clearTranslation();
    m_commentText->clearTranslation();
    for (int j = 0; j < m_editors.count(); ++j) {
        setEditingEnabled(j, false);
        foreach (FormMultiWidget *widget, m_editors[j].transTexts)
            widget->clearTranslation();
        m_editors[j].transCommentText->clearTranslation();
    }
#ifndef QT_NO_CLIPBOARD
    emit pasteAvailable(false);
#endif
    updateBeginFromSource();
    updateUndoRedo();
}

void MessageEditor::showMessage(const MultiDataIndex &index)
{
    m_currentIndex = index;

    bool hadMsg = false;
    for (int j = 0; j < m_editors.size(); ++j) {

        MessageEditorData &ed = m_editors[j];

        MessageItem *item = m_dataModel->messageItem(index, j);
        if (!item) {
            ed.container->hide();
            continue;
        }
        ed.container->show();

        if (!hadMsg) {

            // Source text form
            m_source->setTranslation(item->text());
            m_pluralSource->setTranslation(item->pluralText());
            // Use location from first non-obsolete message
            if (!item->fileName().isEmpty()) {
                QString toolTip = tr("'%1'\nLine: %2").arg(item->fileName(), QString::number(item->lineNumber()));
                m_source->setToolTip(toolTip);
            } else {
                m_source->setToolTip(QLatin1String(""));
            }

            // Comment field
            QString commentText = item->comment().simplified();

            if (!item->extraComment().isEmpty()) {
                if (!commentText.isEmpty())
                    commentText += QLatin1String("\n");
                commentText += item->extraComment().simplified();
            }

            m_commentText->setTranslation(commentText);

            hadMsg = true;
        }

        setEditingEnabled(j, m_dataModel->isModelWritable(j)
                             && item->message().type() != TranslatorMessage::Obsolete
                             && item->message().type() != TranslatorMessage::Vanished);

        // Translation label
        ed.pluralEditMode = item->translations().count() > 1;
        ed.transTexts.first()->setLabel(ed.pluralEditMode ? ed.firstForm : ed.invariantForm);

        // Translation forms
        if (item->text().isEmpty() && !item->context().isEmpty()) {
            for (int i = 0; i < ed.transTexts.size(); ++i)
                ed.transTexts.at(i)->setVisible(false);
        } else {
            QStringList normalizedTranslations =
                m_dataModel->model(j)->normalizedTranslations(*item);
            for (int i = 0; i < ed.transTexts.size(); ++i) {
                bool shouldShow = (i < normalizedTranslations.count());
                if (shouldShow)
                    setTranslation(j, normalizedTranslations.at(i), i);
                else
                    setTranslation(j, QString(), i);
                ed.transTexts.at(i)->setVisible(i == 0 || shouldShow);
            }
        }

        ed.transCommentText->setTranslation(item->translatorComment().trimmed(), false);
    }

    updateUndoRedo();
}

void MessageEditor::setTranslation(int model, const QString &translation, int numerus)
{
    MessageEditorData &ed = m_editors[model];
    if (numerus >= ed.transTexts.count())
        numerus = 0;
    FormMultiWidget *transForm = ed.transTexts[numerus];
    transForm->setTranslation(translation, false);

    updateBeginFromSource();
}

void MessageEditor::setTranslation(int latestModel, const QString &translation)
{
    int numerus;
    if (m_currentNumerus < 0) {
        numerus = 0;
    } else {
        latestModel = m_currentModel;
        numerus = m_currentNumerus;
    }
    FormMultiWidget *transForm = m_editors[latestModel].transTexts[numerus];
    transForm->getEditors().first()->setFocus();
    transForm->setTranslation(translation, true);

    updateBeginFromSource();
}

void MessageEditor::setEditingEnabled(int model, bool enabled)
{
    MessageEditorData &ed = m_editors[model];
    foreach (FormMultiWidget *widget, ed.transTexts)
        widget->setEditingEnabled(enabled);
    ed.transCommentText->setEditingEnabled(enabled);

#ifndef QT_NO_CLIPBOARD
    updateCanPaste();
#endif
}

void MessageEditor::setLengthVariants(bool on)
{
    m_lengthVariants = on;
    foreach (const MessageEditorData &ed, m_editors)
        foreach (FormMultiWidget *widget, ed.transTexts)
            widget->setMultiEnabled(on);
}

void MessageEditor::undo()
{
    activeEditor()->document()->undo();
}

void MessageEditor::redo()
{
    activeEditor()->document()->redo();
}

void MessageEditor::updateUndoRedo()
{
    bool newUndoAvail = false;
    bool newRedoAvail = false;
    if (QTextEdit *te = activeEditor()) {
        QTextDocument *doc = te->document();
        newUndoAvail = doc->isUndoAvailable();
        newRedoAvail = doc->isRedoAvailable();
    }

    if (newUndoAvail != m_undoAvail) {
        m_undoAvail = newUndoAvail;
        emit undoAvailable(newUndoAvail);
    }

    if (newRedoAvail != m_redoAvail) {
        m_redoAvail = newRedoAvail;
        emit redoAvailable(newRedoAvail);
    }
}

#ifndef QT_NO_CLIPBOARD
void MessageEditor::cut()
{
    m_selectionHolder->cut();
}

void MessageEditor::copy()
{
    m_selectionHolder->copy();
}

void MessageEditor::updateCanCutCopy()
{
    bool newCopyState = false;
    bool newCutState = false;

    if (m_selectionHolder) {
        newCopyState = true;
        newCutState = !m_selectionHolder->isReadOnly();
    }

    if (newCopyState != m_copyAvail) {
        m_copyAvail = newCopyState;
        emit copyAvailable(m_copyAvail);
    }

    if (newCutState != m_cutAvail) {
        m_cutAvail = newCutState;
        emit cutAvailable(m_cutAvail);
    }
}

void MessageEditor::paste()
{
    activeEditor()->paste();
}

void MessageEditor::updateCanPaste()
{
    QTextEdit *te;
    emit pasteAvailable(!m_clipboardEmpty
                        && (te = activeEditor()) && !te->isReadOnly());
}

void MessageEditor::clipboardChanged()
{
    // this is expensive, so move it out of the common path in updateCanPaste
    m_clipboardEmpty = qApp->clipboard()->text().isNull();
    updateCanPaste();
}
#endif

void MessageEditor::selectAll()
{
    // make sure we don't select the selection of a translator textedit,
    // if we really want the source text editor to be selected.
    QTextEdit *te;
    if ((te = m_source->getEditor())->underMouse()
        || (te = m_pluralSource->getEditor())->underMouse()
        || ((te = activeEditor()) && te->hasFocus()))
        te->selectAll();
}

void MessageEditor::emitTranslationChanged(QTextEdit *widget)
{
    grabFocus(widget); // DND proofness
    updateBeginFromSource();
    updateUndoRedo();
    emit translationChanged(translations(m_currentModel));
}

void MessageEditor::emitTranslatorCommentChanged(QTextEdit *widget)
{
    grabFocus(widget); // DND proofness
    updateUndoRedo();
    emit translatorCommentChanged(m_editors[m_currentModel].transCommentText->getTranslation());
}

void MessageEditor::updateBeginFromSource()
{
    bool overwrite = false;
    if (QTextEdit *activeEditor = activeTranslation())
        overwrite = !activeEditor->isReadOnly()
            && activeEditor->toPlainText().trimmed().isEmpty();
    emit beginFromSourceAvailable(overwrite);
}

void MessageEditor::beginFromSource()
{
    MessageItem *item = m_dataModel->messageItem(m_currentIndex, m_currentModel);
    setTranslation(m_currentModel,
                   m_currentNumerus > 0 && !item->pluralText().isEmpty() ?
                        item->pluralText() : item->text());
}

void MessageEditor::setEditorFocus()
{
    if (!widget()->hasFocus())
        if (QTextEdit *activeEditor = activeOr1stEditor())
            activeEditor->setFocus();
}

void MessageEditor::setEditorFocus(int model)
{
    if (m_currentModel != model) {
        if (model < 0) {
            resetSelection();
            m_currentNumerus = -1;
            m_currentModel = -1;
            m_focusWidget = 0;
            emit activeModelChanged(activeModel());
            updateBeginFromSource();
            updateUndoRedo();
#ifndef QT_NO_CLIPBOARD
            updateCanPaste();
#endif
        } else {
            m_editors[model].transTexts.first()->getEditors().first()->setFocus();
        }
    }
}

bool MessageEditor::focusNextUnfinished(int start)
{
    for (int j = start; j < m_editors.count(); ++j)
        if (m_dataModel->isModelWritable(j))
            if (MessageItem *item = m_dataModel->messageItem(m_currentIndex, j))
                if (item->type() == TranslatorMessage::Unfinished) {
                    m_editors[j].transTexts.first()->getEditors().first()->setFocus();
                    return true;
                }
    return false;
}

void MessageEditor::setUnfinishedEditorFocus()
{
    focusNextUnfinished(0);
}

bool MessageEditor::focusNextUnfinished()
{
    return focusNextUnfinished(m_currentModel + 1);
}

void MessageEditor::setVisualizeWhitespace(bool value)
{
    m_visualizeWhitespace = value;
    m_source->getEditor()->setVisualizeWhitespace(value);
    m_pluralSource->getEditor()->setVisualizeWhitespace(value);
    m_commentText->getEditor()->setVisualizeWhitespace(value);

    foreach (const MessageEditorData &med, m_editors) {
        med.transCommentText->getEditor()->setVisualizeWhitespace(value);
        foreach (FormMultiWidget *widget, med.transTexts)
            foreach (FormatTextEdit *te, widget->getEditors())
                te->setVisualizeWhitespace(value);
    }
}

void MessageEditor::setFontSize(const float fontSize)
{
    if (m_fontSize != fontSize) {
        m_fontSize = fontSize;
        applyFontSize();
    }
}

float MessageEditor::fontSize()
{
    return m_fontSize;
}

void MessageEditor::applyFontSize()
{
    QFont font;
    font.setPointSize(static_cast<int>(m_fontSize));

    m_source->getEditor()->setFont(font);
    m_pluralSource->getEditor()->setFont(font);
    m_commentText->getEditor()->setFont(font);

    foreach (MessageEditorData med, m_editors) {
        for (int i = 0; i < med.transTexts.count(); ++i)
            foreach (QTextEdit *te, med.transTexts[i]->getEditors())
                te->setFont(font);
        med.transCommentText->getEditor()->setFont(font);
    }
}

void MessageEditor::increaseFontSize()
{
    if (m_fontSize >= 32)
        return;

    m_fontSize *= 1.2f;
    applyFontSize();
}

void MessageEditor::decreaseFontSize()
{
    if (m_fontSize > 8) {
        m_fontSize /= 1.2f;
        applyFontSize();
    }
}

void MessageEditor::resetFontSize()
{
    m_fontSize = font().pointSize();
    applyFontSize();
}

QT_END_NAMESPACE
