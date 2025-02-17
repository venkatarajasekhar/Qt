/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.VBox}
 * @implements {WebInspector.TextEditor}
 * @param {?string} url
 * @param {!WebInspector.TextEditorDelegate} delegate
 */
WebInspector.CodeMirrorTextEditor = function(url, delegate)
{
    WebInspector.VBox.call(this);
    this._delegate = delegate;
    this._url = url;

    this.registerRequiredCSS("cm/codemirror.css");
    this.registerRequiredCSS("cm/cmdevtools.css");

    this._codeMirror = new window.CodeMirror(this.element, {
        lineNumbers: true,
        gutters: ["CodeMirror-linenumbers"],
        matchBrackets: true,
        smartIndent: false,
        styleSelectedText: true,
        electricChars: false,
    });
    this._codeMirror._codeMirrorTextEditor = this;

    CodeMirror.keyMap["devtools-common"] = {
        "Left": "goCharLeft",
        "Right": "goCharRight",
        "Up": "goLineUp",
        "Down": "goLineDown",
        "End": "goLineEnd",
        "Home": "goLineStartSmart",
        "PageUp": "goPageUp",
        "PageDown": "goPageDown",
        "Delete": "delCharAfter",
        "Backspace": "delCharBefore",
        "Tab": "defaultTab",
        "Shift-Tab": "indentLess",
        "Enter": "smartNewlineAndIndent",
        "Ctrl-Space": "autocomplete",
        "Esc": "dismissMultipleSelections"
    };

    CodeMirror.keyMap["devtools-pc"] = {
        "Ctrl-A": "selectAll",
        "Ctrl-Z": "undoAndReveal",
        "Shift-Ctrl-Z": "redoAndReveal",
        "Ctrl-Y": "redo",
        "Ctrl-Home": "goDocStart",
        "Ctrl-Up": "goDocStart",
        "Ctrl-End": "goDocEnd",
        "Ctrl-Down": "goDocEnd",
        "Ctrl-Left": "goGroupLeft",
        "Ctrl-Right": "goGroupRight",
        "Alt-Left": "goLineStart",
        "Alt-Right": "goLineEnd",
        "Ctrl-Backspace": "delGroupBefore",
        "Ctrl-Delete": "delGroupAfter",
        "Ctrl-/": "toggleComment",
        "Ctrl-D": "selectNextOccurrence",
        "Ctrl-U": "undoLastSelection",
        fallthrough: "devtools-common"
    };

    CodeMirror.keyMap["devtools-mac"] = {
        "Cmd-A" : "selectAll",
        "Cmd-Z" : "undoAndReveal",
        "Shift-Cmd-Z": "redoAndReveal",
        "Cmd-Up": "goDocStart",
        "Cmd-Down": "goDocEnd",
        "Alt-Left": "goGroupLeft",
        "Alt-Right": "goGroupRight",
        "Cmd-Left": "goLineStartSmart",
        "Cmd-Right": "goLineEnd",
        "Alt-Backspace": "delGroupBefore",
        "Alt-Delete": "delGroupAfter",
        "Cmd-/": "toggleComment",
        "Cmd-D": "selectNextOccurrence",
        "Cmd-U": "undoLastSelection",
        fallthrough: "devtools-common"
    };

    WebInspector.settings.textEditorIndent.addChangeListener(this._updateEditorIndentation, this);
    this._updateEditorIndentation();
    WebInspector.settings.showWhitespacesInEditor.addChangeListener(this._updateCodeMirrorMode, this);
    WebInspector.settings.textEditorBracketMatching.addChangeListener(this._enableBracketMatchingIfNeeded, this);
    this._enableBracketMatchingIfNeeded();

    this._codeMirror.setOption("keyMap", WebInspector.isMac() ? "devtools-mac" : "devtools-pc");
    this._codeMirror.setOption("flattenSpans", false);

    this._codeMirror.setOption("maxHighlightLength", WebInspector.CodeMirrorTextEditor.maxHighlightLength);
    this._codeMirror.setOption("mode", null);
    this._codeMirror.setOption("crudeMeasuringFrom", 1000);

    this._shouldClearHistory = true;
    this._lineSeparator = "\n";

    this._autocompleteController = WebInspector.CodeMirrorTextEditor.AutocompleteController.Dummy;
    this._tokenHighlighter = new WebInspector.CodeMirrorTextEditor.TokenHighlighter(this, this._codeMirror);
    this._blockIndentController = new WebInspector.CodeMirrorTextEditor.BlockIndentController(this._codeMirror);
    this._fixWordMovement = new WebInspector.CodeMirrorTextEditor.FixWordMovement(this._codeMirror);
    this._selectNextOccurrenceController = new WebInspector.CodeMirrorTextEditor.SelectNextOccurrenceController(this, this._codeMirror);

    this._codeMirror.on("changes", this._changes.bind(this));
    this._codeMirror.on("gutterClick", this._gutterClick.bind(this));
    this._codeMirror.on("cursorActivity", this._cursorActivity.bind(this));
    this._codeMirror.on("beforeSelectionChange", this._beforeSelectionChange.bind(this));
    this._codeMirror.on("scroll", this._scroll.bind(this));
    this._codeMirror.on("focus", this._focus.bind(this));
    this.element.addEventListener("contextmenu", this._contextMenu.bind(this), false);
    /**
     * @this {WebInspector.CodeMirrorTextEditor}
     */
    function updateAnticipateJumpFlag(value)
    {
        this._isHandlingMouseDownEvent = value;
    }
    this.element.addEventListener("mousedown", updateAnticipateJumpFlag.bind(this, true), true);
    this.element.addEventListener("mousedown", updateAnticipateJumpFlag.bind(this, false), false);

    this.element.style.overflow = "hidden";
    this.element.firstChild.classList.add("source-code");
    this.element.firstChild.classList.add("fill");
    this._elementToWidget = new Map();
    this._nestedUpdatesCounter = 0;

    this.element.addEventListener("focus", this._handleElementFocus.bind(this), false);
    this.element.addEventListener("keydown", this._handleKeyDown.bind(this), true);
    this.element.addEventListener("keydown", this._handlePostKeyDown.bind(this), false);
    this.element.tabIndex = 0;

    this._setupWhitespaceHighlight();
}

/** @typedef {{canceled: boolean, from: !CodeMirror.Pos, to: !CodeMirror.Pos, text: string, origin: string, cancel: function()}} */
WebInspector.CodeMirrorTextEditor.BeforeChangeObject;

/** @typedef {{from: !CodeMirror.Pos, to: !CodeMirror.Pos, origin: string, text: !Array.<string>, removed: !Array.<string>}} */
WebInspector.CodeMirrorTextEditor.ChangeObject;

WebInspector.CodeMirrorTextEditor.maxHighlightLength = 1000;

/**
 * @param {!CodeMirror} codeMirror
 */
WebInspector.CodeMirrorTextEditor.autocompleteCommand = function(codeMirror)
{
    codeMirror._codeMirrorTextEditor._autocompleteController.autocomplete();
}
CodeMirror.commands.autocomplete = WebInspector.CodeMirrorTextEditor.autocompleteCommand;

/**
 * @param {!CodeMirror} codeMirror
 */
WebInspector.CodeMirrorTextEditor.undoLastSelectionCommand = function(codeMirror)
{
    codeMirror._codeMirrorTextEditor._selectNextOccurrenceController.undoLastSelection();
}
CodeMirror.commands.undoLastSelection = WebInspector.CodeMirrorTextEditor.undoLastSelectionCommand;

/**
 * @param {!CodeMirror} codeMirror
 */
WebInspector.CodeMirrorTextEditor.selectNextOccurrenceCommand = function(codeMirror)
{
    codeMirror._codeMirrorTextEditor._selectNextOccurrenceController.selectNextOccurrence();
}
CodeMirror.commands.selectNextOccurrence = WebInspector.CodeMirrorTextEditor.selectNextOccurrenceCommand;

/**
 * @param {!CodeMirror} codeMirror
 */
CodeMirror.commands.smartNewlineAndIndent = function(codeMirror)
{
    codeMirror.operation(innerSmartNewlineAndIndent.bind(null, codeMirror));

    function countIndent(line)
    {
        for (var i = 0; i < line.length; ++i) {
            if (!WebInspector.TextUtils.isSpaceChar(line[i]))
                return i;
        }
        return line.length;
    }

    function innerSmartNewlineAndIndent(codeMirror)
    {
        var cur = codeMirror.getCursor("start");
        var line = codeMirror.getLine(cur.line);
        var indent = cur.line > 0 ? countIndent(line) : 0;
        if (cur.ch <= indent) {
            codeMirror.replaceSelection("\n" + line.substring(0, cur.ch), "end", "+input");
            codeMirror.setSelection(new CodeMirror.Pos(cur.line + 1, cur.ch));
        } else
            codeMirror.execCommand("newlineAndIndent");
    }
}

CodeMirror.commands.undoAndReveal = function(codemirror)
{
    var scrollInfo = codemirror.getScrollInfo();
    codemirror.execCommand("undo");
    var cursor = codemirror.getCursor("start");
    codemirror._codeMirrorTextEditor._innerRevealLine(cursor.line, scrollInfo);
    codemirror._codeMirrorTextEditor._autocompleteController.finishAutocomplete();
}

CodeMirror.commands.redoAndReveal = function(codemirror)
{
    var scrollInfo = codemirror.getScrollInfo();
    codemirror.execCommand("redo");
    var cursor = codemirror.getCursor("start");
    codemirror._codeMirrorTextEditor._innerRevealLine(cursor.line, scrollInfo);
    codemirror._codeMirrorTextEditor._autocompleteController.finishAutocomplete();
}

/**
 * @return {!Object|undefined}
 */
CodeMirror.commands.dismissMultipleSelections = function(codemirror)
{
    var selections = codemirror.listSelections();
    var selection = selections[0];
    if (selections.length === 1) {
        if (codemirror._codeMirrorTextEditor._isSearchActive())
            return CodeMirror.Pass;
        if (WebInspector.CodeMirrorUtils.toRange(selection.anchor, selection.head).isEmpty())
            return CodeMirror.Pass;
        codemirror.setSelection(selection.anchor, selection.anchor, {scroll: false});
        codemirror._codeMirrorTextEditor._revealLine(selection.anchor.line);
        return;
    }

    codemirror.setSelection(selection.anchor, selection.head, {scroll: false});
    codemirror._codeMirrorTextEditor._revealLine(selection.anchor.line);
}

WebInspector.CodeMirrorTextEditor.LongLineModeLineLengthThreshold = 2000;
WebInspector.CodeMirrorTextEditor.MaximumNumberOfWhitespacesPerSingleSpan = 16;
WebInspector.CodeMirrorTextEditor.MaxEditableTextSize = 1024 * 1024 * 10;

WebInspector.CodeMirrorTextEditor.prototype = {
    dispose: function()
    {
        WebInspector.settings.textEditorIndent.removeChangeListener(this._updateEditorIndentation, this);
        WebInspector.settings.showWhitespacesInEditor.removeChangeListener(this._updateCodeMirrorMode, this);
        WebInspector.settings.textEditorBracketMatching.removeChangeListener(this._enableBracketMatchingIfNeeded, this);
    },

    _enableBracketMatchingIfNeeded: function()
    {
        this._codeMirror.setOption("autoCloseBrackets", WebInspector.settings.textEditorBracketMatching.get() ? { explode: false } : false);
    },

    wasShown: function()
    {
        if (this._wasOnceShown)
            return;
        this._wasOnceShown = true;
        this._codeMirror.refresh();
    },

    _guessIndentationLevel: function()
    {
        var tabRegex = /^\t+/;
        var tabLines = 0;
        var indents = {};
        function processLine(lineHandle)
        {
            var text = lineHandle.text;
            if (text.length === 0 || !WebInspector.TextUtils.isSpaceChar(text[0]))
                return;
            if (tabRegex.test(text)) {
                ++tabLines;
                return;
            }
            var i = 0;
            while (i < text.length && WebInspector.TextUtils.isSpaceChar(text[i]))
                ++i;
            if (i % 2 !== 0)
                return;
            indents[i] = 1 + (indents[i] || 0);
        }
        this._codeMirror.eachLine(0, 1000, processLine);

        var onePercentFilterThreshold = this.linesCount / 100;
        if (tabLines && tabLines > onePercentFilterThreshold)
            return "\t";
        var minimumIndent = Infinity;
        for (var i in indents) {
            if (indents[i] < onePercentFilterThreshold)
                continue;
            var indent = parseInt(i, 10);
            if (minimumIndent > indent)
                minimumIndent = indent;
        }
        if (minimumIndent === Infinity)
            return WebInspector.TextUtils.Indent.FourSpaces;
        return new Array(minimumIndent + 1).join(" ");
    },

    _updateEditorIndentation: function()
    {
        var extraKeys = {};
        var indent = WebInspector.settings.textEditorIndent.get();
        if (WebInspector.settings.textEditorAutoDetectIndent.get())
            indent = this._guessIndentationLevel();
        if (indent === WebInspector.TextUtils.Indent.TabCharacter) {
            this._codeMirror.setOption("indentWithTabs", true);
            this._codeMirror.setOption("indentUnit", 4);
        } else {
            this._codeMirror.setOption("indentWithTabs", false);
            this._codeMirror.setOption("indentUnit", indent.length);
            extraKeys.Tab = function(codeMirror)
            {
                if (codeMirror.somethingSelected())
                    return CodeMirror.Pass;
                var pos = codeMirror.getCursor("head");
                codeMirror.replaceRange(indent.substring(pos.ch % indent.length), codeMirror.getCursor());
            }
        }
        this._codeMirror.setOption("extraKeys", extraKeys);
        this._indentationLevel = indent;
    },

    /**
     * @return {string}
     */
    indent: function()
    {
        return this._indentationLevel;
    },

    /**
     * @return {boolean}
     */
    _isSearchActive: function()
    {
        return !!this._tokenHighlighter.highlightedRegex();
    },

    /**
     * @param {!RegExp} regex
     * @param {?WebInspector.TextRange} range
     */
    highlightSearchResults: function(regex, range)
    {
        /**
         * @this {WebInspector.CodeMirrorTextEditor}
         */
        function innerHighlightRegex()
        {
            if (range) {
                this._revealLine(range.startLine);
                if (range.endColumn > WebInspector.CodeMirrorTextEditor.maxHighlightLength)
                    this.setSelection(range);
                else
                    this.setSelection(WebInspector.TextRange.createFromLocation(range.startLine, range.startColumn));
            } else {
                // Collapse selection to end on search start so that we jump to next occurrence on the first enter press.
                this.setSelection(this.selection().collapseToEnd());
            }
            this._tokenHighlighter.highlightSearchResults(regex, range);
        }
        if (!this._selectionBeforeSearch)
            this._selectionBeforeSearch = this.selection();
        this._codeMirror.operation(innerHighlightRegex.bind(this));
    },

    cancelSearchResultsHighlight: function()
    {
        this._codeMirror.operation(this._tokenHighlighter.highlightSelectedTokens.bind(this._tokenHighlighter));
        if (this._selectionBeforeSearch) {
            this._reportJump(this._selectionBeforeSearch, this.selection());
            delete this._selectionBeforeSearch;
        }
    },

    undo: function()
    {
        this._codeMirror.undo();
    },

    redo: function()
    {
        this._codeMirror.redo();
    },

    _setupWhitespaceHighlight: function()
    {
        if (WebInspector.CodeMirrorTextEditor._whitespaceStyleInjected || !WebInspector.settings.showWhitespacesInEditor.get())
            return;
        WebInspector.CodeMirrorTextEditor._whitespaceStyleInjected = true;
        const classBase = ".show-whitespaces .CodeMirror .cm-whitespace-";
        const spaceChar = "·";
        var spaceChars = "";
        var rules = "";
        for (var i = 1; i <= WebInspector.CodeMirrorTextEditor.MaximumNumberOfWhitespacesPerSingleSpan; ++i) {
            spaceChars += spaceChar;
            var rule = classBase + i + "::before { content: '" + spaceChars + "';}\n";
            rules += rule;
        }
        var style = document.createElement("style");
        style.textContent = rules;
        document.head.appendChild(style);
    },

    _handleKeyDown: function(e)
    {
        if (this._autocompleteController.keyDown(e))
            e.consume(true);
    },

    _handlePostKeyDown: function(e)
    {
        if (e.defaultPrevented)
            e.consume(true);
    },

    /**
     * @param {?WebInspector.CompletionDictionary} dictionary
     */
    setCompletionDictionary: function(dictionary)
    {
        this._autocompleteController.dispose();
        if (dictionary)
            this._autocompleteController = new WebInspector.CodeMirrorTextEditor.AutocompleteController(this, this._codeMirror, dictionary);
        else
            this._autocompleteController = WebInspector.CodeMirrorTextEditor.AutocompleteController.Dummy;
    },

    /**
     * @param {number} lineNumber
     * @param {number} column
     * @return {?{x: number, y: number, height: number}}
     */
    cursorPositionToCoordinates: function(lineNumber, column)
    {
        if (lineNumber >= this._codeMirror.lineCount() || lineNumber < 0 || column < 0 || column > this._codeMirror.getLine(lineNumber).length)
            return null;

        var metrics = this._codeMirror.cursorCoords(new CodeMirror.Pos(lineNumber, column));

        return {
            x: metrics.left,
            y: metrics.top,
            height: metrics.bottom - metrics.top
        };
    },

    /**
     * @param {number} x
     * @param {number} y
     * @return {?WebInspector.TextRange}
     */
    coordinatesToCursorPosition: function(x, y)
    {
        var element = document.elementFromPoint(x, y);
        if (!element || !element.isSelfOrDescendant(this._codeMirror.getWrapperElement()))
            return null;
        var gutterBox = this._codeMirror.getGutterElement().boxInWindow();
        if (x >= gutterBox.x && x <= gutterBox.x + gutterBox.width &&
            y >= gutterBox.y && y <= gutterBox.y + gutterBox.height)
            return null;
        var coords = this._codeMirror.coordsChar({left: x, top: y});
        return WebInspector.CodeMirrorUtils.toRange(coords, coords);
    },

    /**
     * @param {number} lineNumber
     * @param {number} column
     * @return {?{startColumn: number, endColumn: number, type: string}}
     */
    tokenAtTextPosition: function(lineNumber, column)
    {
        if (lineNumber < 0 || lineNumber >= this._codeMirror.lineCount())
            return null;
        var token = this._codeMirror.getTokenAt(new CodeMirror.Pos(lineNumber, (column || 0) + 1));
        if (!token || !token.type)
            return null;
        return {
            startColumn: token.start,
            endColumn: token.end - 1,
            type: token.type
        };
    },

    /**
     * @param {!WebInspector.TextRange} textRange
     * @return {string}
     */
    copyRange: function(textRange)
    {
        var pos = WebInspector.CodeMirrorUtils.toPos(textRange.normalize());
        return this._codeMirror.getRange(pos.start, pos.end);
    },

    /**
     * @return {boolean}
     */
    isClean: function()
    {
        return this._codeMirror.isClean();
    },

    markClean: function()
    {
        this._codeMirror.markClean();
    },

    _hasLongLines: function()
    {
        function lineIterator(lineHandle)
        {
            if (lineHandle.text.length > WebInspector.CodeMirrorTextEditor.LongLineModeLineLengthThreshold)
                hasLongLines = true;
            return hasLongLines;
        }
        var hasLongLines = false;
        this._codeMirror.eachLine(lineIterator);
        return hasLongLines;
    },

    /**
     * @param {string} mimeType
     * @return {string}
     */
    _whitespaceOverlayMode: function(mimeType)
    {
        var modeName = CodeMirror.mimeModes[mimeType] ? (CodeMirror.mimeModes[mimeType].name || CodeMirror.mimeModes[mimeType]) : CodeMirror.mimeModes["text/plain"];
        modeName += "+whitespaces";
        if (CodeMirror.modes[modeName])
            return modeName;

        function modeConstructor(config, parserConfig)
        {
            function nextToken(stream)
            {
                if (stream.peek() === " ") {
                    var spaces = 0;
                    while (spaces < WebInspector.CodeMirrorTextEditor.MaximumNumberOfWhitespacesPerSingleSpan && stream.peek() === " ") {
                        ++spaces;
                        stream.next();
                    }
                    return "whitespace whitespace-" + spaces;
                }
                while (!stream.eol() && stream.peek() !== " ")
                    stream.next();
                return null;
            }
            var whitespaceMode = {
                token: nextToken
            };
            return CodeMirror.overlayMode(CodeMirror.getMode(config, mimeType), whitespaceMode, false);
        }
        CodeMirror.defineMode(modeName, modeConstructor);
        return modeName;
    },

    _enableLongLinesMode: function()
    {
        this._codeMirror.setOption("styleSelectedText", false);
        this._longLinesMode = true;
    },

    _disableLongLinesMode: function()
    {
        this._codeMirror.setOption("styleSelectedText", true);
        this._longLinesMode = false;
    },

    _updateCodeMirrorMode: function()
    {
        var showWhitespaces = WebInspector.settings.showWhitespacesInEditor.get();
        this.element.classList.toggle("show-whitespaces", showWhitespaces);
        this._codeMirror.setOption("mode", showWhitespaces ? this._whitespaceOverlayMode(this._mimeType) : this._mimeType);
    },

    /**
     * @param {string} mimeType
     */
    setMimeType: function(mimeType)
    {
        this._mimeType = mimeType;
        if (this._hasLongLines())
            this._enableLongLinesMode();
        else
            this._disableLongLinesMode();
        this._updateCodeMirrorMode();
        this._autocompleteController.setMimeType(mimeType);
    },

    /**
     * @param {boolean} readOnly
     */
    setReadOnly: function(readOnly)
    {
        this.element.classList.toggle("CodeMirror-readonly", readOnly)
        this._codeMirror.setOption("readOnly", readOnly);
    },

    /**
     * @return {boolean}
     */
    readOnly: function()
    {
        return !!this._codeMirror.getOption("readOnly");
    },

    /**
     * @param {!Object} highlightDescriptor
     */
    removeHighlight: function(highlightDescriptor)
    {
        highlightDescriptor.clear();
    },

    /**
     * @param {!WebInspector.TextRange} range
     * @param {string} cssClass
     * @return {!Object}
     */
    highlightRange: function(range, cssClass)
    {
        cssClass = "CodeMirror-persist-highlight " + cssClass;
        var pos = WebInspector.CodeMirrorUtils.toPos(range);
        ++pos.end.ch;
        return this._codeMirror.markText(pos.start, pos.end, {
            className: cssClass,
            startStyle: cssClass + "-start",
            endStyle: cssClass + "-end"
        });
    },

    /**
     * @return {!Element}
     */
    defaultFocusedElement: function()
    {
        return this.element;
    },

    focus: function()
    {
        this._codeMirror.focus();
    },

    _handleElementFocus: function()
    {
        this._codeMirror.focus();
    },

    beginUpdates: function()
    {
        ++this._nestedUpdatesCounter;
    },

    endUpdates: function()
    {
        if (!--this._nestedUpdatesCounter)
            this._codeMirror.refresh();
    },

    /**
     * @param {number} lineNumber
     */
    _revealLine: function(lineNumber)
    {
        this._innerRevealLine(lineNumber, this._codeMirror.getScrollInfo());
    },

    /**
     * @param {number} lineNumber
     * @param {!{left: number, top: number, width: number, height: number, clientWidth: number, clientHeight: number}} scrollInfo
     */
    _innerRevealLine: function(lineNumber, scrollInfo)
    {
        var topLine = this._codeMirror.lineAtHeight(scrollInfo.top, "local");
        var bottomLine = this._codeMirror.lineAtHeight(scrollInfo.top + scrollInfo.clientHeight, "local");
        var linesPerScreen = bottomLine - topLine + 1;
        if (lineNumber < topLine) {
            var topLineToReveal = Math.max(lineNumber - (linesPerScreen / 2) + 1, 0) | 0;
            this._codeMirror.scrollIntoView(new CodeMirror.Pos(topLineToReveal, 0));
        } else if (lineNumber > bottomLine) {
            var bottomLineToReveal = Math.min(lineNumber + (linesPerScreen / 2) - 1, this.linesCount - 1) | 0;
            this._codeMirror.scrollIntoView(new CodeMirror.Pos(bottomLineToReveal, 0));
        }
    },

    _gutterClick: function(instance, lineNumber, gutter, event)
    {
        this.dispatchEventToListeners(WebInspector.TextEditor.Events.GutterClick, { lineNumber: lineNumber, event: event });
    },

    _contextMenu: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu(event);
        var target = event.target.enclosingNodeOrSelfWithClass("CodeMirror-gutter-elt");
        if (target)
            this._delegate.populateLineGutterContextMenu(contextMenu, parseInt(target.textContent, 10) - 1);
        else
            this._delegate.populateTextAreaContextMenu(contextMenu, 0);
        contextMenu.show();
    },

    /**
     * @param {number} lineNumber
     * @param {boolean} disabled
     * @param {boolean} conditional
     */
    addBreakpoint: function(lineNumber, disabled, conditional)
    {
        if (lineNumber < 0 || lineNumber >= this._codeMirror.lineCount())
            return;
        var className = "cm-breakpoint" + (conditional ? " cm-breakpoint-conditional" : "") + (disabled ? " cm-breakpoint-disabled" : "");
        this._codeMirror.addLineClass(lineNumber, "wrap", className);
    },

    /**
     * @param {number} lineNumber
     */
    removeBreakpoint: function(lineNumber)
    {
        if (lineNumber < 0 || lineNumber >= this._codeMirror.lineCount())
            return;
        var wrapClasses = this._codeMirror.getLineHandle(lineNumber).wrapClass;
        if (!wrapClasses)
            return;
        var classes = wrapClasses.split(" ");
        for (var i = 0; i < classes.length; ++i) {
            if (classes[i].startsWith("cm-breakpoint"))
                this._codeMirror.removeLineClass(lineNumber, "wrap", classes[i]);
        }
    },

    /**
     * @param {number} lineNumber
     */
    setExecutionLine: function(lineNumber)
    {
        this.clearPositionHighlight();
        this._executionLine = this._codeMirror.getLineHandle(lineNumber);
        if (!this._executionLine)
            return;
        this._codeMirror.addLineClass(this._executionLine, "wrap", "cm-execution-line");
    },

    clearExecutionLine: function()
    {
        this.clearPositionHighlight();
        if (this._executionLine)
            this._codeMirror.removeLineClass(this._executionLine, "wrap", "cm-execution-line");
        delete this._executionLine;
    },

    /**
     * @param {number} lineNumber
     * @param {!Element} element
     */
    addDecoration: function(lineNumber, element)
    {
        var widget = this._codeMirror.addLineWidget(lineNumber, element);
        this._elementToWidget.put(element, widget);
    },

    /**
     * @param {number} lineNumber
     * @param {!Element} element
     */
    removeDecoration: function(lineNumber, element)
    {
        var widget = this._elementToWidget.remove(element);
        if (widget)
            this._codeMirror.removeLineWidget(widget);
    },

    /**
     * @param {number} lineNumber
     * @param {number=} columnNumber
     * @param {boolean=} shouldHighlight
     */
    revealPosition: function(lineNumber, columnNumber, shouldHighlight)
    {
        lineNumber = Number.constrain(lineNumber, 0, this._codeMirror.lineCount() - 1);
        if (typeof columnNumber !== "number")
            columnNumber = 0;
        columnNumber = Number.constrain(columnNumber, 0, this._codeMirror.getLine(lineNumber).length);

        this.clearPositionHighlight();
        this._highlightedLine = this._codeMirror.getLineHandle(lineNumber);
        if (!this._highlightedLine)
          return;
        this._revealLine(lineNumber);
        if (shouldHighlight) {
            this._codeMirror.addLineClass(this._highlightedLine, null, "cm-highlight");
            this._clearHighlightTimeout = setTimeout(this.clearPositionHighlight.bind(this), 2000);
        }
        this.setSelection(WebInspector.TextRange.createFromLocation(lineNumber, columnNumber));
    },

    clearPositionHighlight: function()
    {
        if (this._clearHighlightTimeout)
            clearTimeout(this._clearHighlightTimeout);
        delete this._clearHighlightTimeout;

        if (this._highlightedLine)
            this._codeMirror.removeLineClass(this._highlightedLine, null, "cm-highlight");
        delete this._highlightedLine;
    },

    /**
     * @return {!Array.<!Element>}
     */
    elementsToRestoreScrollPositionsFor: function()
    {
        return [];
    },

    /**
     * @param {!WebInspector.TextEditor} textEditor
     */
    inheritScrollPositions: function(textEditor)
    {
    },

    /**
     * @param {number} width
     * @param {number} height
     */
    _updatePaddingBottom: function(width, height)
    {
        var scrollInfo = this._codeMirror.getScrollInfo();
        var newPaddingBottom;
        var linesElement = this.element.firstElementChild.querySelector(".CodeMirror-lines");
        var lineCount = this._codeMirror.lineCount();
        if (lineCount <= 1)
            newPaddingBottom = 0;
        else
            newPaddingBottom = Math.max(scrollInfo.clientHeight - this._codeMirror.getLineHandle(this._codeMirror.lastLine()).height, 0);
        newPaddingBottom += "px";
        linesElement.style.paddingBottom = newPaddingBottom;
        this._codeMirror.setSize(width, height);
    },

    _resizeEditor: function()
    {
        var parentElement = this.element.parentElement;
        if (!parentElement || !this.isShowing())
            return;
        var scrollLeft = this._codeMirror.doc.scrollLeft;
        var scrollTop = this._codeMirror.doc.scrollTop;
        var width = parentElement.offsetWidth;
        var height = parentElement.offsetHeight;
        this._codeMirror.setSize(width, height);
        this._updatePaddingBottom(width, height);
        this._codeMirror.scrollTo(scrollLeft, scrollTop);
    },

    onResize: function()
    {
        this._autocompleteController.finishAutocomplete();
        this._resizeEditor();
    },

    /**
     * @param {!WebInspector.TextRange} range
     * @param {string} text
     * @return {!WebInspector.TextRange}
     */
    editRange: function(range, text)
    {
        var pos = WebInspector.CodeMirrorUtils.toPos(range);
        this._codeMirror.replaceRange(text, pos.start, pos.end);
        var newRange = WebInspector.CodeMirrorUtils.toRange(pos.start, this._codeMirror.posFromIndex(this._codeMirror.indexFromPos(pos.start) + text.length));
        this._delegate.onTextChanged(range, newRange);
        if (WebInspector.settings.textEditorAutoDetectIndent.get())
            this._updateEditorIndentation();
        return newRange;
    },

    /**
     * @param {number} lineNumber
     * @param {number} column
     * @param {function(string):boolean} isWordChar
     * @return {!WebInspector.TextRange}
     */
    _wordRangeForCursorPosition: function(lineNumber, column, isWordChar)
    {
        var line = this.line(lineNumber);
        var wordStart = column;
        if (column !== 0 && isWordChar(line.charAt(column - 1))) {
            wordStart = column - 1;
            while (wordStart > 0 && isWordChar(line.charAt(wordStart - 1)))
                --wordStart;
        }
        var wordEnd = column;
        while (wordEnd < line.length && isWordChar(line.charAt(wordEnd)))
            ++wordEnd;
        return new WebInspector.TextRange(lineNumber, wordStart, lineNumber, wordEnd);
    },

    /**
     * @param {!WebInspector.CodeMirrorTextEditor.ChangeObject} changeObject
     * @return {{oldRange: !WebInspector.TextRange, newRange: !WebInspector.TextRange}}
     */
    _changeObjectToEditOperation: function(changeObject)
    {
        var oldRange = WebInspector.CodeMirrorUtils.toRange(changeObject.from, changeObject.to);
        var newRange = oldRange.clone();
        var linesAdded = changeObject.text.length;
        if (linesAdded === 0) {
            newRange.endLine = newRange.startLine;
            newRange.endColumn = newRange.startColumn;
        } else if (linesAdded === 1) {
            newRange.endLine = newRange.startLine;
            newRange.endColumn = newRange.startColumn + changeObject.text[0].length;
        } else {
            newRange.endLine = newRange.startLine + linesAdded - 1;
            newRange.endColumn = changeObject.text[linesAdded - 1].length;
        }
        return {
            oldRange: oldRange,
            newRange: newRange
        };
    },

    /**
     * @param {!CodeMirror} codeMirror
     * @param {!Array.<!WebInspector.CodeMirrorTextEditor.ChangeObject>} changes
     */
    _changes: function(codeMirror, changes)
    {
        if (!changes.length)
            return;
        // We do not show "scroll beyond end of file" span for one line documents, so we need to check if "document has one line" changed.
        var hasOneLine = this._codeMirror.lineCount() === 1;
        if (hasOneLine !== this._hasOneLine)
            this._resizeEditor();
        this._hasOneLine = hasOneLine;
        var widgets = this._elementToWidget.values();
        for (var i = 0; i < widgets.length; ++i)
            this._codeMirror.removeLineWidget(widgets[i]);
        this._elementToWidget.clear();

        for (var changeIndex = 0; changeIndex < changes.length; ++changeIndex) {
            var changeObject = changes[changeIndex];

            var editInfo = this._changeObjectToEditOperation(changeObject);
            if (!this._muteTextChangedEvent)
                this._delegate.onTextChanged(editInfo.oldRange, editInfo.newRange);
        }
    },

    _cursorActivity: function()
    {
        var start = this._codeMirror.getCursor("anchor");
        var end = this._codeMirror.getCursor("head");
        this._delegate.selectionChanged(WebInspector.CodeMirrorUtils.toRange(start, end));
        if (!this._isSearchActive())
            this._codeMirror.operation(this._tokenHighlighter.highlightSelectedTokens.bind(this._tokenHighlighter));
    },

    /**
     * @param {!CodeMirror} codeMirror
     * @param {{ranges: !Array.<{head: !CodeMirror.Pos, anchor: !CodeMirror.Pos}>}} selection
     */
    _beforeSelectionChange: function(codeMirror, selection)
    {
        this._selectNextOccurrenceController.selectionWillChange();
        if (!this._isHandlingMouseDownEvent)
            return;
        if (!selection.ranges.length)
            return;
        var primarySelection = selection.ranges[0];
        this._reportJump(this.selection(), WebInspector.CodeMirrorUtils.toRange(primarySelection.anchor, primarySelection.head));
    },

    /**
     * @param {?WebInspector.TextRange} from
     * @param {?WebInspector.TextRange} to
     */
    _reportJump: function(from, to)
    {
        if (from && to && from.equal(to))
            return;
        this._delegate.onJumpToPosition(from, to);
    },

    _scroll: function()
    {
        if (this._scrollTimer)
            clearTimeout(this._scrollTimer);
        var topmostLineNumber = this._codeMirror.lineAtHeight(this._codeMirror.getScrollInfo().top, "local");
        this._scrollTimer = setTimeout(this._delegate.scrollChanged.bind(this._delegate, topmostLineNumber), 100);
    },

    _focus: function()
    {
        this._delegate.editorFocused();
    },

    /**
     * @param {number} lineNumber
     */
    scrollToLine: function(lineNumber)
    {
        var pos = new CodeMirror.Pos(lineNumber, 0);
        var coords = this._codeMirror.charCoords(pos, "local");
        this._codeMirror.scrollTo(0, coords.top);
    },

    /**
     * @return {number}
     */
    firstVisibleLine: function()
    {
        return this._codeMirror.lineAtHeight(this._codeMirror.getScrollInfo().top, "local");
    },

    /**
     * @return {number}
     */
    lastVisibleLine: function()
    {
        var scrollInfo = this._codeMirror.getScrollInfo();
        return this._codeMirror.lineAtHeight(scrollInfo.top + scrollInfo.clientHeight, "local");
    },

    /**
     * @return {!WebInspector.TextRange}
     */
    selection: function()
    {
        var start = this._codeMirror.getCursor("anchor");
        var end = this._codeMirror.getCursor("head");

        return WebInspector.CodeMirrorUtils.toRange(start, end);
    },

    /**
     * @return {!Array.<!WebInspector.TextRange>}
     */
    selections: function()
    {
        var selectionList = this._codeMirror.listSelections();
        var result = [];
        for (var i = 0; i < selectionList.length; ++i) {
            var selection = selectionList[i];
            result.push(WebInspector.CodeMirrorUtils.toRange(selection.anchor, selection.head));
        }
        return result;
    },

    /**
     * @return {?WebInspector.TextRange}
     */
    lastSelection: function()
    {
        return this._lastSelection;
    },

    /**
     * @param {!WebInspector.TextRange} textRange
     */
    setSelection: function(textRange)
    {
        this._lastSelection = textRange;
        var pos = WebInspector.CodeMirrorUtils.toPos(textRange);
        this._codeMirror.setSelection(pos.start, pos.end);
    },

    /**
     * @param {!Array.<!WebInspector.TextRange>} ranges
     * @param {number=} primarySelectionIndex
     */
    setSelections: function(ranges, primarySelectionIndex)
    {
        var selections = [];
        for (var i = 0; i < ranges.length; ++i) {
            var selection = WebInspector.CodeMirrorUtils.toPos(ranges[i]);
            selections.push({
                anchor: selection.start,
                head: selection.end
            });
        }
        primarySelectionIndex = primarySelectionIndex || 0;
        this._codeMirror.setSelections(selections, primarySelectionIndex, { scroll: false });
    },

    /**
     * @param {string} text
     */
    _detectLineSeparator: function(text)
    {
        this._lineSeparator = text.indexOf("\r\n") >= 0 ? "\r\n" : "\n";
    },

    /**
     * @param {string} text
     */
    setText: function(text)
    {
        this._muteTextChangedEvent = true;
        if (text.length > WebInspector.CodeMirrorTextEditor.MaxEditableTextSize) {
            this._autocompleteController.setEnabled(false);
            this.setReadOnly(true);
        }
        this._codeMirror.setValue(text);
        this._updateEditorIndentation();
        if (this._shouldClearHistory) {
            this._codeMirror.clearHistory();
            this._shouldClearHistory = false;
        }
        this._detectLineSeparator(text);
        delete this._muteTextChangedEvent;
    },

    /**
     * @return {string}
     */
    text: function()
    {
        return this._codeMirror.getValue().replace(/\n/g, this._lineSeparator);
    },

    /**
     * @return {!WebInspector.TextRange}
     */
    range: function()
    {
        var lineCount = this.linesCount;
        var lastLine = this._codeMirror.getLine(lineCount - 1);
        return WebInspector.CodeMirrorUtils.toRange(new CodeMirror.Pos(0, 0), new CodeMirror.Pos(lineCount - 1, lastLine.length));
    },

    /**
     * @param {number} lineNumber
     * @return {string}
     */
    line: function(lineNumber)
    {
        return this._codeMirror.getLine(lineNumber);
    },

    /**
     * @return {number}
     */
    get linesCount()
    {
        return this._codeMirror.lineCount();
    },

    /**
     * @param {number} line
     * @param {string} name
     * @param {?Object} value
     */
    setAttribute: function(line, name, value)
    {
        if (line < 0 || line >= this._codeMirror.lineCount())
            return;
        var handle = this._codeMirror.getLineHandle(line);
        if (handle.attributes === undefined) handle.attributes = {};
        handle.attributes[name] = value;
    },

    /**
     * @param {number} line
     * @param {string} name
     * @return {?Object} value
     */
    getAttribute: function(line, name)
    {
        if (line < 0 || line >= this._codeMirror.lineCount())
            return null;
        var handle = this._codeMirror.getLineHandle(line);
        return handle.attributes && handle.attributes[name] !== undefined ? handle.attributes[name] : null;
    },

    /**
     * @param {number} line
     * @param {string} name
     */
    removeAttribute: function(line, name)
    {
        if (line < 0 || line >= this._codeMirror.lineCount())
            return;
        var handle = this._codeMirror.getLineHandle(line);
        if (handle && handle.attributes)
            delete handle.attributes[name];
    },

    /**
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {!WebInspector.TextEditorPositionHandle}
     */
    textEditorPositionHandle: function(lineNumber, columnNumber)
    {
        return new WebInspector.CodeMirrorPositionHandle(this._codeMirror, new CodeMirror.Pos(lineNumber, columnNumber));
    },

    __proto__: WebInspector.VBox.prototype
}

/**
 * @constructor
 * @implements {WebInspector.TextEditorPositionHandle}
 * @param {!CodeMirror} codeMirror
 * @param {!CodeMirror.Pos} pos
 */
WebInspector.CodeMirrorPositionHandle = function(codeMirror, pos)
{
    this._codeMirror = codeMirror;
    this._lineHandle = codeMirror.getLineHandle(pos.line);
    this._columnNumber = pos.ch;
}

WebInspector.CodeMirrorPositionHandle.prototype = {
    /**
     * @return {?{lineNumber: number, columnNumber: number}}
     */
    resolve: function()
    {
        var lineNumber = this._codeMirror.getLineNumber(this._lineHandle);
        if (typeof lineNumber !== "number")
            return null;
        return {
            lineNumber: lineNumber,
            columnNumber: this._columnNumber
        };
    },

    /**
     * @param {!WebInspector.TextEditorPositionHandle} positionHandle
     * @return {boolean}
     */
    equal: function(positionHandle)
    {
        return positionHandle._lineHandle === this._lineHandle && positionHandle._columnNumber == this._columnNumber && positionHandle._codeMirror === this._codeMirror;
    }
}

/**
 * @constructor
 * @param {!WebInspector.CodeMirrorTextEditor} textEditor
 * @param {!CodeMirror} codeMirror
 */
WebInspector.CodeMirrorTextEditor.TokenHighlighter = function(textEditor, codeMirror)
{
    this._textEditor = textEditor;
    this._codeMirror = codeMirror;
}

WebInspector.CodeMirrorTextEditor.TokenHighlighter.prototype = {
    /**
     * @param {!RegExp} regex
     * @param {?WebInspector.TextRange} range
     */
    highlightSearchResults: function(regex, range)
    {
        var oldRegex = this._highlightRegex;
        this._highlightRegex = regex;
        this._highlightRange = range;
        if (this._searchResultMarker) {
            this._searchResultMarker.clear();
            delete this._searchResultMarker;
        }
        if (this._highlightDescriptor && this._highlightDescriptor.selectionStart)
            this._codeMirror.removeLineClass(this._highlightDescriptor.selectionStart.line, "wrap", "cm-line-with-selection");
        var selectionStart = this._highlightRange ? new CodeMirror.Pos(this._highlightRange.startLine, this._highlightRange.startColumn) : null;
        if (selectionStart)
            this._codeMirror.addLineClass(selectionStart.line, "wrap", "cm-line-with-selection");
        if (this._highlightRegex === oldRegex) {
            // Do not re-add overlay mode if regex did not change for better performance.
            if (this._highlightDescriptor)
                this._highlightDescriptor.selectionStart = selectionStart;
        } else {
            this._removeHighlight();
            this._setHighlighter(this._searchHighlighter.bind(this, this._highlightRegex), selectionStart);
        }
        if (this._highlightRange) {
            var pos = WebInspector.CodeMirrorUtils.toPos(this._highlightRange);
            this._searchResultMarker = this._codeMirror.markText(pos.start, pos.end, {className: "cm-column-with-selection"});
        }
    },

    /**
     * @return {!RegExp|undefined}
     */
    highlightedRegex: function()
    {
        return this._highlightRegex;
    },

    highlightSelectedTokens: function()
    {
        delete this._highlightRegex;
        delete this._highlightRange;

        if (this._highlightDescriptor && this._highlightDescriptor.selectionStart)
            this._codeMirror.removeLineClass(this._highlightDescriptor.selectionStart.line, "wrap", "cm-line-with-selection");
        this._removeHighlight();
        var selectionStart = this._codeMirror.getCursor("start");
        var selectionEnd = this._codeMirror.getCursor("end");
        if (selectionStart.line !== selectionEnd.line)
            return;
        if (selectionStart.ch === selectionEnd.ch)
            return;

        var selections = this._codeMirror.getSelections();
        if (selections.length > 1)
            return;
        var selectedText = selections[0];
        if (this._isWord(selectedText, selectionStart.line, selectionStart.ch, selectionEnd.ch)) {
            if (selectionStart)
                this._codeMirror.addLineClass(selectionStart.line, "wrap", "cm-line-with-selection")
            this._setHighlighter(this._tokenHighlighter.bind(this, selectedText, selectionStart), selectionStart);
        }
    },

    /**
     * @param {string} selectedText
     * @param {number} lineNumber
     * @param {number} startColumn
     * @param {number} endColumn
     */
    _isWord: function(selectedText, lineNumber, startColumn, endColumn)
    {
        var line = this._codeMirror.getLine(lineNumber);
        var leftBound = startColumn === 0 || !WebInspector.TextUtils.isWordChar(line.charAt(startColumn - 1));
        var rightBound = endColumn === line.length || !WebInspector.TextUtils.isWordChar(line.charAt(endColumn));
        return leftBound && rightBound && WebInspector.TextUtils.isWord(selectedText);
    },

    _removeHighlight: function()
    {
        if (this._highlightDescriptor) {
            this._codeMirror.removeOverlay(this._highlightDescriptor.overlay);
            delete this._highlightDescriptor;
        }
    },

    /**
     * @param {!RegExp} regex
     * @param {!CodeMirror.StringStream} stream
     */
    _searchHighlighter: function(regex, stream)
    {
        if (stream.column() === 0)
            delete this._searchMatchLength;
        if (this._searchMatchLength) {
            if (this._searchMatchLength > 2) {
                for (var i = 0; i < this._searchMatchLength - 2; ++i)
                    stream.next();
                this._searchMatchLength = 1;
                return "search-highlight";
            } else {
                stream.next();
                delete this._searchMatchLength;
                return "search-highlight search-highlight-end";
            }
        }
        var match = stream.match(regex, false);
        if (match) {
            stream.next();
            var matchLength = match[0].length;
            if (matchLength === 1)
                return "search-highlight search-highlight-full";
            this._searchMatchLength = matchLength;
            return "search-highlight search-highlight-start";
        }

        while (!stream.match(regex, false) && stream.next()) {};
    },

    /**
     * @param {string} token
     * @param {!CodeMirror.Pos} selectionStart
     * @param {!CodeMirror.StringStream} stream
     */
    _tokenHighlighter: function(token, selectionStart, stream)
    {
        var tokenFirstChar = token.charAt(0);
        if (stream.match(token) && (stream.eol() || !WebInspector.TextUtils.isWordChar(stream.peek())))
            return stream.column() === selectionStart.ch ? "token-highlight column-with-selection" : "token-highlight";

        var eatenChar;
        do {
            eatenChar = stream.next();
        } while (eatenChar && (WebInspector.TextUtils.isWordChar(eatenChar) || stream.peek() !== tokenFirstChar));
    },

    /**
     * @param {function(!CodeMirror.StringStream)} highlighter
     * @param {?CodeMirror.Pos} selectionStart
     */
    _setHighlighter: function(highlighter, selectionStart)
    {
        var overlayMode = {
            token: highlighter
        };
        this._codeMirror.addOverlay(overlayMode);
        this._highlightDescriptor = {
            overlay: overlayMode,
            selectionStart: selectionStart
        };
    }
}

/**
 * @constructor
 * @param {!CodeMirror} codeMirror
 */
WebInspector.CodeMirrorTextEditor.BlockIndentController = function(codeMirror)
{
    codeMirror.addKeyMap(this);
}

WebInspector.CodeMirrorTextEditor.BlockIndentController.prototype = {
    name: "blockIndentKeymap",

    /**
     * @return {*}
     */
    Enter: function(codeMirror)
    {
        if (codeMirror.somethingSelected())
            return CodeMirror.Pass;
        var cursor = codeMirror.getCursor();
        if (cursor.ch === 0)
            return CodeMirror.Pass;
        var line = codeMirror.getLine(cursor.line);
        if (line.substr(cursor.ch - 1, 2) === "{}") {
            codeMirror.execCommand("newlineAndIndent");
            codeMirror.setCursor(cursor);
            codeMirror.execCommand("newlineAndIndent");
            codeMirror.execCommand("indentMore");
        } else if (line.substr(cursor.ch - 1, 1) === "{") {
            codeMirror.execCommand("newlineAndIndent");
            codeMirror.execCommand("indentMore");
        } else
            return CodeMirror.Pass;
    },

    /**
     * @return {*}
     */
    "'}'": function(codeMirror)
    {
        var cursor = codeMirror.getCursor();
        var line = codeMirror.getLine(cursor.line);
        for (var i = 0 ; i < line.length; ++i) {
            if (!WebInspector.TextUtils.isSpaceChar(line.charAt(i)))
                return CodeMirror.Pass;
        }

        codeMirror.replaceRange("}", cursor);
        var matchingBracket = codeMirror.findMatchingBracket(cursor);
        if (!matchingBracket || !matchingBracket.match)
            return;

        line = codeMirror.getLine(matchingBracket.to.line);
        var desiredIndentation = 0;
        while (desiredIndentation < line.length && WebInspector.TextUtils.isSpaceChar(line.charAt(desiredIndentation)))
            ++desiredIndentation;

        codeMirror.replaceRange(line.substr(0, desiredIndentation) + "}", new CodeMirror.Pos(cursor.line, 0), new CodeMirror.Pos(cursor.line, cursor.ch + 1));
    }
}

/**
 * @constructor
 * @param {!CodeMirror} codeMirror
 */
WebInspector.CodeMirrorTextEditor.FixWordMovement = function(codeMirror)
{
    function moveLeft(shift, codeMirror)
    {
        codeMirror.setExtending(shift);
        var cursor = codeMirror.getCursor("head");
        codeMirror.execCommand("goGroupLeft");
        var newCursor = codeMirror.getCursor("head");
        if (newCursor.ch === 0 && newCursor.line !== 0) {
            codeMirror.setExtending(false);
            return;
        }

        var skippedText = codeMirror.getRange(newCursor, cursor, "#");
        if (/^\s+$/.test(skippedText))
            codeMirror.execCommand("goGroupLeft");
        codeMirror.setExtending(false);
    }

    function moveRight(shift, codeMirror)
    {
        codeMirror.setExtending(shift);
        var cursor = codeMirror.getCursor("head");
        codeMirror.execCommand("goGroupRight");
        var newCursor = codeMirror.getCursor("head");
        if (newCursor.ch === 0 && newCursor.line !== 0) {
            codeMirror.setExtending(false);
            return;
        }

        var skippedText = codeMirror.getRange(cursor, newCursor, "#");
        if (/^\s+$/.test(skippedText))
            codeMirror.execCommand("goGroupRight");
        codeMirror.setExtending(false);
    }

    var modifierKey = WebInspector.isMac() ? "Alt" : "Ctrl";
    var leftKey = modifierKey + "-Left";
    var rightKey = modifierKey + "-Right";
    var keyMap = {};
    keyMap[leftKey] = moveLeft.bind(null, false);
    keyMap[rightKey] = moveRight.bind(null, false);
    keyMap["Shift-" + leftKey] = moveLeft.bind(null, true);
    keyMap["Shift-" + rightKey] = moveRight.bind(null, true);
    codeMirror.addKeyMap(keyMap);
}

/**
 * @interface
 */
WebInspector.CodeMirrorTextEditor.AutocompleteControllerAPI = function() {}

WebInspector.CodeMirrorTextEditor.AutocompleteControllerAPI.prototype = {
    dispose: function() { },

    /**
     * @param {boolean} enabled
     */
    setEnabled: function(enabled) { },

    /**
     * @param {string} mimeType
     */
    setMimeType: function(mimeType) { },

    autocomplete: function() { },

    finishAutocomplete: function() { },

    /**
     * @param {?Event} e
     * @return {boolean}
     */
    keyDown: function(e) { }
}

/**
 * @constructor
 * @implements {WebInspector.CodeMirrorTextEditor.AutocompleteControllerAPI}
 */
WebInspector.CodeMirrorTextEditor.DummyAutocompleteController = function() { }

WebInspector.CodeMirrorTextEditor.DummyAutocompleteController.prototype = {
    dispose: function() { },

    /**
     * @param {boolean} enabled
     */
    setEnabled: function(enabled) { },

    /**
     * @param {string} mimeType
     */
    setMimeType: function(mimeType) { },

    autocomplete: function() { },

    finishAutocomplete: function() { },

    /**
     * @param {?Event} e
     * @return {boolean}
     */
    keyDown: function(e)
    {
        return false;
    }
}

/**
 * @constructor
 * @implements {WebInspector.SuggestBoxDelegate}
 * @implements {WebInspector.CodeMirrorTextEditor.AutocompleteControllerAPI}
 * @param {!WebInspector.CodeMirrorTextEditor} textEditor
 * @param {!CodeMirror} codeMirror
 * @param {?WebInspector.CompletionDictionary} dictionary
 */
WebInspector.CodeMirrorTextEditor.AutocompleteController = function(textEditor, codeMirror, dictionary)
{
    this._textEditor = textEditor;
    this._codeMirror = codeMirror;

    this._onScroll = this._onScroll.bind(this);
    this._onCursorActivity = this._onCursorActivity.bind(this);
    this._changes = this._changes.bind(this);
    this._beforeChange = this._beforeChange.bind(this);
    this._blur = this._blur.bind(this);
    this._codeMirror.on("scroll", this._onScroll);
    this._codeMirror.on("cursorActivity", this._onCursorActivity);
    this._codeMirror.on("changes", this._changes);
    this._codeMirror.on("beforeChange", this._beforeChange);
    this._codeMirror.on("blur", this._blur);

    this._additionalWordChars = WebInspector.CodeMirrorTextEditor._NoAdditionalWordChars;
    this._enabled = true;

    this._dictionary = dictionary;
    this._addTextToCompletionDictionary(this._textEditor.text());
}

WebInspector.CodeMirrorTextEditor.AutocompleteController.Dummy = new WebInspector.CodeMirrorTextEditor.DummyAutocompleteController();
WebInspector.CodeMirrorTextEditor._NoAdditionalWordChars = {};
WebInspector.CodeMirrorTextEditor._CSSAdditionalWordChars = { ".": true, "-": true };

WebInspector.CodeMirrorTextEditor.AutocompleteController.prototype = {
    dispose: function()
    {
        this._codeMirror.off("scroll", this._onScroll);
        this._codeMirror.off("cursorActivity", this._onCursorActivity);
        this._codeMirror.off("changes", this._changes);
        this._codeMirror.off("beforeChange", this._beforeChange);
        this._codeMirror.off("blur", this._blur);
    },

    /**
     * @param {boolean} enabled
     */
    setEnabled: function(enabled)
    {
        if (enabled === this._enabled)
            return;
        this._enabled = enabled;
        if (!enabled)
            this._dictionary.reset();
        else
            this._addTextToCompletionDictionary(this._textEditor.text());
    },

    /**
     * @param {string} mimeType
     */
    setMimeType: function(mimeType)
    {
        var additionalWordChars = mimeType.indexOf("css") !== -1 ? WebInspector.CodeMirrorTextEditor._CSSAdditionalWordChars : WebInspector.CodeMirrorTextEditor._NoAdditionalWordChars;
        if (additionalWordChars !== this._additionalWordChars) {
            this._additionalWordChars = additionalWordChars;
            this._dictionary.reset();
            this._addTextToCompletionDictionary(this._textEditor.text());
        }
    },

    /**
     * @param {string} char
     * @return {boolean}
     */
    _isWordChar: function(char)
    {
        return WebInspector.TextUtils.isWordChar(char) || !!this._additionalWordChars[char];
    },

    /**
     * @param {string} word
     * @return {boolean}
     */
    _shouldProcessWordForAutocompletion: function(word)
    {
        return !!word.length && (word[0] < '0' || word[0] > '9');
    },

    /**
     * @param {string} text
     */
    _addTextToCompletionDictionary: function(text)
    {
        if (!this._enabled)
            return;
        var words = WebInspector.TextUtils.textToWords(text, this._isWordChar.bind(this));
        for (var i = 0; i < words.length; ++i) {
            if (this._shouldProcessWordForAutocompletion(words[i]))
                this._dictionary.addWord(words[i]);
        }
    },

    /**
     * @param {string} text
     */
    _removeTextFromCompletionDictionary: function(text)
    {
        if (!this._enabled)
            return;
        var words = WebInspector.TextUtils.textToWords(text, this._isWordChar.bind(this));
        for (var i = 0; i < words.length; ++i) {
            if (this._shouldProcessWordForAutocompletion(words[i]))
                this._dictionary.removeWord(words[i]);
        }
    },

    /**
     * @param {!CodeMirror} codeMirror
     * @param {!WebInspector.CodeMirrorTextEditor.BeforeChangeObject} changeObject
     */
    _beforeChange: function(codeMirror, changeObject)
    {
        if (!this._enabled)
            return;
        this._updatedLines = this._updatedLines || {};
        for (var i = changeObject.from.line; i <= changeObject.to.line; ++i)
            this._updatedLines[i] = this._textEditor.line(i);
    },

    /**
     * @param {!CodeMirror} codeMirror
     * @param {!Array.<!WebInspector.CodeMirrorTextEditor.ChangeObject>} changes
     */
    _changes: function(codeMirror, changes)
    {
        if (!changes.length || !this._enabled)
            return;

        if (this._updatedLines) {
            for (var lineNumber in this._updatedLines)
                this._removeTextFromCompletionDictionary(this._updatedLines[lineNumber]);
            delete this._updatedLines;
        }

        var linesToUpdate = {};
        var singleCharInput = false;
        for (var changeIndex = 0; changeIndex < changes.length; ++changeIndex) {
            var changeObject = changes[changeIndex];
            singleCharInput = (changeObject.origin === "+input" && changeObject.text.length === 1 && changeObject.text[0].length === 1) ||
                (changeObject.origin === "+delete" && changeObject.removed.length === 1 && changeObject.removed[0].length === 1);

            var editInfo = this._textEditor._changeObjectToEditOperation(changeObject);
            for (var i = editInfo.newRange.startLine; i <= editInfo.newRange.endLine; ++i)
                linesToUpdate[i] = this._textEditor.line(i);
        }
        for (var lineNumber in linesToUpdate)
            this._addTextToCompletionDictionary(linesToUpdate[lineNumber]);

        if (singleCharInput)
            this.autocomplete();
    },

    _blur: function()
    {
        this.finishAutocomplete();
    },

    /**
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {!WebInspector.TextRange}
     */
    _autocompleteWordRange: function(lineNumber, columnNumber)
    {
        return this._textEditor._wordRangeForCursorPosition(lineNumber, columnNumber, this._isWordChar.bind(this));
    },

    /**
     * @param {!WebInspector.TextRange} mainSelection
     * @param {!Array.<!{head: !CodeMirror.Pos, anchor: !CodeMirror.Pos}>} selections
     * @return {boolean}
     */
    _validateSelectionsContexts: function(mainSelection, selections)
    {
        var mainSelectionContext = this._textEditor.copyRange(mainSelection);
        for (var i = 0; i < selections.length; ++i) {
            var wordRange = this._autocompleteWordRange(selections[i].head.line, selections[i].head.ch);
            if (!wordRange)
                return false;
            var context = this._textEditor.copyRange(wordRange);
            if (context !== mainSelectionContext)
                return false;
        }
        return true;
    },

    autocomplete: function()
    {
        var dictionary = this._dictionary;
        if (this._codeMirror.somethingSelected()) {
            this.finishAutocomplete();
            return;
        }

        var selections = this._codeMirror.listSelections().slice();
        var topSelection = selections.shift();
        var cursor = topSelection.head;
        var substituteRange = this._autocompleteWordRange(cursor.line, cursor.ch);
        if (!substituteRange || substituteRange.startColumn === cursor.ch || !this._validateSelectionsContexts(substituteRange, selections)) {
            this.finishAutocomplete();
            return;
        }

        var prefixRange = substituteRange.clone();
        prefixRange.endColumn = cursor.ch;

        var substituteWord = this._textEditor.copyRange(substituteRange);
        var hasPrefixInDictionary = dictionary.hasWord(substituteWord);
        if (hasPrefixInDictionary)
            dictionary.removeWord(substituteWord);
        var wordsWithPrefix = dictionary.wordsWithPrefix(this._textEditor.copyRange(prefixRange));
        if (hasPrefixInDictionary)
            dictionary.addWord(substituteWord);

        function sortSuggestions(a, b)
        {
            return dictionary.wordCount(b) - dictionary.wordCount(a) || a.length - b.length;
        }

        wordsWithPrefix.sort(sortSuggestions);

        if (!this._suggestBox)
            this._suggestBox = new WebInspector.SuggestBox(this, 6);
        var oldPrefixRange = this._prefixRange;
        this._prefixRange = prefixRange;
        if (!oldPrefixRange || prefixRange.startLine !== oldPrefixRange.startLine || prefixRange.startColumn !== oldPrefixRange.startColumn)
            this._updateAnchorBox();
        this._suggestBox.updateSuggestions(this._anchorBox, wordsWithPrefix, 0, true, this._textEditor.copyRange(prefixRange));
        if (!this._suggestBox.visible())
            this.finishAutocomplete();
    },

    finishAutocomplete: function()
    {
        if (!this._suggestBox)
            return;
        this._suggestBox.hide();
        this._suggestBox = null;
        this._prefixRange = null;
        this._anchorBox = null;
    },

    /**
     * @param {?Event} e
     * @return {boolean}
     */
    keyDown: function(e)
    {
        if (!this._suggestBox)
            return false;
        if (e.keyCode === WebInspector.KeyboardShortcut.Keys.Esc.code) {
            this.finishAutocomplete();
            return true;
        }
        if (e.keyCode === WebInspector.KeyboardShortcut.Keys.Tab.code) {
            this._suggestBox.acceptSuggestion();
            this.finishAutocomplete();
            return true;
        }
        return this._suggestBox.keyPressed(e);
    },

    /**
     * @param {string} suggestion
     * @param {boolean=} isIntermediateSuggestion
     */
    applySuggestion: function(suggestion, isIntermediateSuggestion)
    {
        this._currentSuggestion = suggestion;
    },

    acceptSuggestion: function()
    {
        if (this._prefixRange.endColumn - this._prefixRange.startColumn === this._currentSuggestion.length)
            return;

        var selections = this._codeMirror.listSelections().slice();
        var prefixLength = this._prefixRange.endColumn - this._prefixRange.startColumn;
        for (var i = selections.length - 1; i >= 0; --i) {
            var start = selections[i].head;
            var end = new CodeMirror.Pos(start.line, start.ch - prefixLength);
            this._codeMirror.replaceRange(this._currentSuggestion, start, end, "+autocomplete");
        }
    },

    _onScroll: function()
    {
        if (!this._suggestBox)
            return;
        var cursor = this._codeMirror.getCursor();
        var scrollInfo = this._codeMirror.getScrollInfo();
        var topmostLineNumber = this._codeMirror.lineAtHeight(scrollInfo.top, "local");
        var bottomLine = this._codeMirror.lineAtHeight(scrollInfo.top + scrollInfo.clientHeight, "local");
        if (cursor.line < topmostLineNumber || cursor.line > bottomLine)
            this.finishAutocomplete();
        else {
            this._updateAnchorBox();
            this._suggestBox.setPosition(this._anchorBox);
        }
    },

    _onCursorActivity: function()
    {
        if (!this._suggestBox)
            return;
        var cursor = this._codeMirror.getCursor();
        if (cursor.line !== this._prefixRange.startLine || cursor.ch > this._prefixRange.endColumn || cursor.ch <= this._prefixRange.startColumn)
            this.finishAutocomplete();
    },

    _updateAnchorBox: function()
    {
        var line = this._prefixRange.startLine;
        var column = this._prefixRange.startColumn;
        var metrics = this._textEditor.cursorPositionToCoordinates(line, column);
        this._anchorBox = metrics ? new AnchorBox(metrics.x, metrics.y, 0, metrics.height) : null;
    },
}

/**
 * @constructor
 * @param {!WebInspector.CodeMirrorTextEditor} textEditor
 * @param {!CodeMirror} codeMirror
 */
WebInspector.CodeMirrorTextEditor.SelectNextOccurrenceController = function(textEditor, codeMirror)
{
    this._textEditor = textEditor;
    this._codeMirror = codeMirror;
}

WebInspector.CodeMirrorTextEditor.SelectNextOccurrenceController.prototype = {
    selectionWillChange: function()
    {
        if (!this._muteSelectionListener)
            delete this._fullWordSelection;
    },

    /**
     * @param {!Array.<!WebInspector.TextRange>} selections
     * @param {!WebInspector.TextRange} range
     * @return {boolean}
     */
    _findRange: function(selections, range)
    {
        for (var i = 0; i < selections.length; ++i) {
            if (range.equal(selections[i]))
                return true;
        }
        return false;
    },

    undoLastSelection: function()
    {
        this._muteSelectionListener = true;
        this._codeMirror.execCommand("undoSelection");
        this._muteSelectionListener = false;
    },

    selectNextOccurrence: function()
    {
        var selections = this._textEditor.selections();
        var anyEmptySelection = false;
        for (var i = 0; i < selections.length; ++i) {
            var selection = selections[i];
            anyEmptySelection = anyEmptySelection || selection.isEmpty();
            if (selection.startLine !== selection.endLine)
                return;
        }
        if (anyEmptySelection) {
            this._expandSelectionsToWords(selections);
            return;
        }

        var last = selections[selections.length - 1];
        var next = last;
        do {
            next = this._findNextOccurrence(next, !!this._fullWordSelection);
        } while (next && this._findRange(selections, next) && !next.equal(last));

        if (!next)
            return;
        selections.push(next);

        this._muteSelectionListener = true;
        this._textEditor.setSelections(selections, selections.length - 1);
        delete this._muteSelectionListener;

        this._textEditor._revealLine(next.startLine);
    },

    /**
     * @param {!Array.<!WebInspector.TextRange>} selections
     */
    _expandSelectionsToWords: function(selections)
    {
        var newSelections = [];
        for (var i = 0; i < selections.length; ++i) {
            var selection = selections[i];
            var startRangeWord = this._textEditor._wordRangeForCursorPosition(selection.startLine, selection.startColumn, WebInspector.TextUtils.isWordChar)
                || WebInspector.TextRange.createFromLocation(selection.startLine, selection.startColumn);
            var endRangeWord = this._textEditor._wordRangeForCursorPosition(selection.endLine, selection.endColumn, WebInspector.TextUtils.isWordChar)
                || WebInspector.TextRange.createFromLocation(selection.endLine, selection.endColumn);
            var newSelection = new WebInspector.TextRange(startRangeWord.startLine, startRangeWord.startColumn, endRangeWord.endLine, endRangeWord.endColumn);
            newSelections.push(newSelection);
        }
        this._textEditor.setSelections(newSelections, newSelections.length - 1);
        this._fullWordSelection = true;
    },

    /**
     * @param {!WebInspector.TextRange} range
     * @param {boolean} fullWord
     * @return {?WebInspector.TextRange}
     */
    _findNextOccurrence: function(range, fullWord)
    {
        range = range.normalize();
        var matchedLineNumber;
        var matchedColumnNumber;
        var textToFind = this._textEditor.copyRange(range);
        function findWordInLine(wordRegex, lineNumber, lineText, from, to)
        {
            if (typeof matchedLineNumber === "number")
                return true;
            wordRegex.lastIndex = from;
            var result = wordRegex.exec(lineText);
            if (!result || result.index + textToFind.length > to)
                return false;
            matchedLineNumber = lineNumber;
            matchedColumnNumber = result.index;
            return true;
        }

        var iteratedLineNumber;
        function lineIterator(regex, lineHandle)
        {
            if (findWordInLine(regex, iteratedLineNumber++, lineHandle.text, 0, lineHandle.text.length))
                return true;
        }

        var regexSource = textToFind.escapeForRegExp();
        if (fullWord)
            regexSource = "\\b" + regexSource + "\\b";
        var wordRegex = new RegExp(regexSource, "gi");
        var currentLineText = this._codeMirror.getLine(range.startLine);

        findWordInLine(wordRegex, range.startLine, currentLineText, range.endColumn, currentLineText.length);
        iteratedLineNumber = range.startLine + 1;
        this._codeMirror.eachLine(range.startLine + 1, this._codeMirror.lineCount(), lineIterator.bind(null, wordRegex));
        iteratedLineNumber = 0;
        this._codeMirror.eachLine(0, range.startLine, lineIterator.bind(null, wordRegex));
        findWordInLine(wordRegex, range.startLine, currentLineText, 0, range.startColumn);

        if (typeof matchedLineNumber !== "number")
            return null;
        return new WebInspector.TextRange(matchedLineNumber, matchedColumnNumber, matchedLineNumber, matchedColumnNumber + textToFind.length);
    }
}

/**
 * @param {string} modeName
 * @param {string} tokenPrefix
 */
WebInspector.CodeMirrorTextEditor._overrideModeWithPrefixedTokens = function(modeName, tokenPrefix)
{
    var oldModeName = modeName + "-old";
    if (CodeMirror.modes[oldModeName])
        return;

    CodeMirror.defineMode(oldModeName, CodeMirror.modes[modeName]);
    CodeMirror.defineMode(modeName, modeConstructor);

    function modeConstructor(config, parserConfig)
    {
        var innerConfig = {};
        for (var i in parserConfig)
            innerConfig[i] = parserConfig[i];
        innerConfig.name = oldModeName;
        var codeMirrorMode = CodeMirror.getMode(config, innerConfig);
        codeMirrorMode.name = modeName;
        codeMirrorMode.token = tokenOverride.bind(null, codeMirrorMode.token);
        return codeMirrorMode;
    }

    function tokenOverride(superToken, stream, state)
    {
        var token = superToken(stream, state);
        return token ? tokenPrefix + token : token;
    }
}

WebInspector.CodeMirrorTextEditor._overrideModeWithPrefixedTokens("css", "css-");
WebInspector.CodeMirrorTextEditor._overrideModeWithPrefixedTokens("javascript", "js-");
WebInspector.CodeMirrorTextEditor._overrideModeWithPrefixedTokens("xml", "xml-");

(function() {
    var backgroundColor = InspectorFrontendHost.getSelectionBackgroundColor();
    var backgroundColorRule = backgroundColor ? ".CodeMirror .CodeMirror-selected { background-color: " + backgroundColor + ";}" : "";
    var foregroundColor = InspectorFrontendHost.getSelectionForegroundColor();
    var foregroundColorRule = foregroundColor ? ".CodeMirror .CodeMirror-selectedtext:not(.CodeMirror-persist-highlight) { color: " + foregroundColor + "!important;}" : "";
    if (!foregroundColorRule && !backgroundColorRule)
        return;

    var style = document.createElement("style");
    style.textContent = backgroundColorRule + foregroundColorRule;
    document.head.appendChild(style);
})();
