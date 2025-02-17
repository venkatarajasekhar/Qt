/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

importScript("../cm/codemirror.js");
importScript("../cm/css.js");
importScript("../cm/javascript.js");
importScript("../cm/xml.js");
importScript("../cm/htmlmixed.js");

importScript("../cm/matchbrackets.js");
importScript("../cm/closebrackets.js");
importScript("../cm/markselection.js");
importScript("../cm/comment.js");
importScript("../cm/overlay.js");

importScript("../cm/htmlembedded.js");
importScript("../cm/clike.js");
importScript("../cm/coffeescript.js");
importScript("../cm/php.js");
importScript("../cm/python.js");
importScript("../cm/shell.js");
importScript("CodeMirrorUtils.js");
importScript("CodeMirrorTextEditor.js");

/**
 * @extends {WebInspector.VBox}
 * @constructor
 * @implements {WebInspector.Replaceable}
 * @param {!WebInspector.ContentProvider} contentProvider
 */
WebInspector.SourceFrame = function(contentProvider)
{
    WebInspector.VBox.call(this);
    this.element.classList.add("script-view");

    this._url = contentProvider.contentURL();
    this._contentProvider = contentProvider;

    var textEditorDelegate = new WebInspector.TextEditorDelegateForSourceFrame(this);

    this._textEditor = new WebInspector.CodeMirrorTextEditor(this._url, textEditorDelegate);

    this._currentSearchResultIndex = -1;
    this._searchResults = [];

    this._messages = [];
    this._rowMessages = {};
    this._messageBubbles = {};

    this._textEditor.setReadOnly(!this.canEditSource());

    this._shortcuts = {};
    this.element.addEventListener("keydown", this._handleKeyDown.bind(this), false);

    this._sourcePosition = new WebInspector.StatusBarText("", "source-frame-cursor-position");
}

/**
 * @param {string} query
 * @param {string=} modifiers
 * @return {!RegExp}
 */
WebInspector.SourceFrame.createSearchRegex = function(query, modifiers)
{
    var regex;
    modifiers = modifiers || "";

    // First try creating regex if user knows the / / hint.
    try {
        if (/^\/.+\/$/.test(query)) {
            regex = new RegExp(query.substring(1, query.length - 1), modifiers);
            regex.__fromRegExpQuery = true;
        }
    } catch (e) {
        // Silent catch.
    }

    // Otherwise just do case-insensitive search.
    if (!regex)
        regex = createPlainTextSearchRegex(query, "i" + modifiers);

    return regex;
}

WebInspector.SourceFrame.Events = {
    ScrollChanged: "ScrollChanged",
    SelectionChanged: "SelectionChanged",
    JumpHappened: "JumpHappened"
}

WebInspector.SourceFrame.prototype = {
    /**
     * @param {number} key
     * @param {function()} handler
     */
    addShortcut: function(key, handler)
    {
        this._shortcuts[key] = handler;
    },

    wasShown: function()
    {
        this._ensureContentLoaded();
        this._textEditor.show(this.element);
        this._editorAttached = true;
        this._wasShownOrLoaded();
    },

    /**
     * @return {boolean}
     */
    _isEditorShowing: function()
    {
        return this.isShowing() && this._editorAttached;
    },

    willHide: function()
    {
        WebInspector.View.prototype.willHide.call(this);

        this._clearPositionToReveal();
    },

    /**
     * @return {?Element}
     */
    statusBarText: function()
    {
        return this._sourcePosition.element;
    },

    /**
     * @return {!Array.<!Element>}
     */
    statusBarItems: function()
    {
        return [];
    },

    /**
     * @return {!Element}
     */
    defaultFocusedElement: function()
    {
        return this._textEditor.defaultFocusedElement();
    },

    get loaded()
    {
        return this._loaded;
    },

    /**
     * @return {boolean}
     */
    hasContent: function()
    {
        return true;
    },

    get textEditor()
    {
        return this._textEditor;
    },

    _ensureContentLoaded: function()
    {
        if (!this._contentRequested) {
            this._contentRequested = true;
            this._contentProvider.requestContent(this.setContent.bind(this));
        }
    },

    addMessage: function(msg)
    {
        this._messages.push(msg);
        if (this.loaded)
            this.addMessageToSource(msg.line - 1, msg);
    },

    clearMessages: function()
    {
        for (var line in this._messageBubbles) {
            var bubble = this._messageBubbles[line];
            var lineNumber = parseInt(line, 10);
            this._textEditor.removeDecoration(lineNumber, bubble);
        }

        this._messages = [];
        this._rowMessages = {};
        this._messageBubbles = {};
    },

    /**
     * @param {number} line
     * @param {number=} column
     * @param {boolean=} shouldHighlight
     */
    revealPosition: function(line, column, shouldHighlight)
    {
        this._clearLineToScrollTo();
        this._clearSelectionToSet();
        this._positionToReveal = { line: line, column: column, shouldHighlight: shouldHighlight };
        this._innerRevealPositionIfNeeded();
    },

    _innerRevealPositionIfNeeded: function()
    {
        if (!this._positionToReveal)
            return;

        if (!this.loaded || !this._isEditorShowing())
            return;

        this._textEditor.revealPosition(this._positionToReveal.line, this._positionToReveal.column, this._positionToReveal.shouldHighlight);
        delete this._positionToReveal;
    },

    _clearPositionToReveal: function()
    {
        this._textEditor.clearPositionHighlight();
        delete this._positionToReveal;
    },

    /**
     * @param {number} line
     */
    scrollToLine: function(line)
    {
        this._clearPositionToReveal();
        this._lineToScrollTo = line;
        this._innerScrollToLineIfNeeded();
    },

    _innerScrollToLineIfNeeded: function()
    {
        if (typeof this._lineToScrollTo === "number") {
            if (this.loaded && this._isEditorShowing()) {
                this._textEditor.scrollToLine(this._lineToScrollTo);
                delete this._lineToScrollTo;
            }
        }
    },

    _clearLineToScrollTo: function()
    {
        delete this._lineToScrollTo;
    },

    /**
     * @return {!WebInspector.TextRange}
     */
    selection: function()
    {
        return this.textEditor.selection();
    },

    /**
     * @param {!WebInspector.TextRange} textRange
     */
    setSelection: function(textRange)
    {
        this._selectionToSet = textRange;
        this._innerSetSelectionIfNeeded();
    },

    _innerSetSelectionIfNeeded: function()
    {
        if (this._selectionToSet && this.loaded && this._isEditorShowing()) {
            this._textEditor.setSelection(this._selectionToSet);
            delete this._selectionToSet;
        }
    },

    _clearSelectionToSet: function()
    {
        delete this._selectionToSet;
    },

    _wasShownOrLoaded: function()
    {
        this._innerRevealPositionIfNeeded();
        this._innerSetSelectionIfNeeded();
        this._innerScrollToLineIfNeeded();
    },

    onTextChanged: function(oldRange, newRange)
    {
        if (this._searchResultsChangedCallback && !this._isReplacing)
            this._searchResultsChangedCallback();
        this.clearMessages();
    },

    _simplifyMimeType: function(content, mimeType)
    {
        if (!mimeType)
            return "";
        if (mimeType.indexOf("javascript") >= 0 ||
            mimeType.indexOf("jscript") >= 0 ||
            mimeType.indexOf("ecmascript") >= 0)
            return "text/javascript";
        // A hack around the fact that files with "php" extension might be either standalone or html embedded php scripts.
        if (mimeType === "text/x-php" && content.match(/\<\?.*\?\>/g))
            return "application/x-httpd-php";
        return mimeType;
    },

    /**
     * @param {string} highlighterType
     */
    setHighlighterType: function(highlighterType)
    {
        this._highlighterType = highlighterType;
        this._updateHighlighterType("");
    },

    /**
     * @param {string} content
     */
    _updateHighlighterType: function(content)
    {
        this._textEditor.setMimeType(this._simplifyMimeType(content, this._highlighterType));
    },

    /**
     * @param {?string} content
     */
    setContent: function(content)
    {
        if (!this._loaded) {
            this._loaded = true;
            this._textEditor.setText(content || "");
            this._textEditor.markClean();
        } else {
            var firstLine = this._textEditor.firstVisibleLine();
            var selection = this._textEditor.selection();
            this._textEditor.setText(content || "");
            this._textEditor.scrollToLine(firstLine);
            this._textEditor.setSelection(selection);
        }

        this._updateHighlighterType(content || "");

        this._textEditor.beginUpdates();

        this._setTextEditorDecorations();

        this._wasShownOrLoaded();

        if (this._delayedFindSearchMatches) {
            this._delayedFindSearchMatches();
            delete this._delayedFindSearchMatches;
        }

        this.onTextEditorContentLoaded();

        this._textEditor.endUpdates();
    },

    onTextEditorContentLoaded: function() {},

    _setTextEditorDecorations: function()
    {
        this._rowMessages = {};
        this._messageBubbles = {};

        this._textEditor.beginUpdates();

        this._addExistingMessagesToSource();

        this._textEditor.endUpdates();
    },

    /**
     * @param {string} query
     * @param {boolean} shouldJump
     * @param {boolean} jumpBackwards
     * @param {function(!WebInspector.View, number)} callback
     * @param {function(number)} currentMatchChangedCallback
     * @param {function()} searchResultsChangedCallback
     */
    performSearch: function(query, shouldJump, jumpBackwards, callback, currentMatchChangedCallback, searchResultsChangedCallback)
    {
        /**
         * @param {string} query
         * @this {WebInspector.SourceFrame}
         */
        function doFindSearchMatches(query)
        {
            this._currentSearchResultIndex = -1;
            this._searchResults = [];

            var regex = WebInspector.SourceFrame.createSearchRegex(query);
            this._searchRegex = regex;
            this._searchResults = this._collectRegexMatches(regex);
            if (!this._searchResults.length)
                this._textEditor.cancelSearchResultsHighlight();
            else if (shouldJump && jumpBackwards)
                this.jumpToPreviousSearchResult();
            else if (shouldJump)
                this.jumpToNextSearchResult();
            else
                this._textEditor.highlightSearchResults(regex, null);
            callback(this, this._searchResults.length);
        }

        this._resetSearch();
        this._currentSearchMatchChangedCallback = currentMatchChangedCallback;
        this._searchResultsChangedCallback = searchResultsChangedCallback;
        if (this.loaded)
            doFindSearchMatches.call(this, query);
        else
            this._delayedFindSearchMatches = doFindSearchMatches.bind(this, query);

        this._ensureContentLoaded();
    },

    _editorFocused: function()
    {
        this._resetCurrentSearchResultIndex();
    },

    _resetCurrentSearchResultIndex: function()
    {
        if (!this._searchResults.length)
            return;
        this._currentSearchResultIndex = -1;
        if (this._currentSearchMatchChangedCallback)
            this._currentSearchMatchChangedCallback(this._currentSearchResultIndex);
        this._textEditor.highlightSearchResults(this._searchRegex, null);
    },

    _resetSearch: function()
    {
        delete this._delayedFindSearchMatches;
        delete this._currentSearchMatchChangedCallback;
        delete this._searchResultsChangedCallback;
        this._currentSearchResultIndex = -1;
        this._searchResults = [];
        delete this._searchRegex;
    },

    searchCanceled: function()
    {
        var range = this._currentSearchResultIndex !== -1 ? this._searchResults[this._currentSearchResultIndex] : null;
        this._resetSearch();
        if (!this.loaded)
            return;
        this._textEditor.cancelSearchResultsHighlight();
        if (range)
            this._textEditor.setSelection(range);
    },

    /**
     * @return {boolean}
     */
    hasSearchResults: function()
    {
        return this._searchResults.length > 0;
    },

    jumpToFirstSearchResult: function()
    {
        this.jumpToSearchResult(0);
    },

    jumpToLastSearchResult: function()
    {
        this.jumpToSearchResult(this._searchResults.length - 1);
    },

    /**
     * @return {number}
     */
    _searchResultIndexForCurrentSelection: function()
    {
        return insertionIndexForObjectInListSortedByFunction(this._textEditor.selection(), this._searchResults, WebInspector.TextRange.comparator);
    },

    jumpToNextSearchResult: function()
    {
        var currentIndex = this._searchResultIndexForCurrentSelection();
        var nextIndex = this._currentSearchResultIndex === -1 ? currentIndex : currentIndex + 1;
        this.jumpToSearchResult(nextIndex);
    },

    jumpToPreviousSearchResult: function()
    {
        var currentIndex = this._searchResultIndexForCurrentSelection();
        this.jumpToSearchResult(currentIndex - 1);
    },

    /**
     * @return {boolean}
     */
    showingFirstSearchResult: function()
    {
        return this._searchResults.length &&  this._currentSearchResultIndex === 0;
    },

    /**
     * @return {boolean}
     */
    showingLastSearchResult: function()
    {
        return this._searchResults.length && this._currentSearchResultIndex === (this._searchResults.length - 1);
    },

    get currentSearchResultIndex()
    {
        return this._currentSearchResultIndex;
    },

    jumpToSearchResult: function(index)
    {
        if (!this.loaded || !this._searchResults.length)
            return;
        this._currentSearchResultIndex = (index + this._searchResults.length) % this._searchResults.length;
        if (this._currentSearchMatchChangedCallback)
            this._currentSearchMatchChangedCallback(this._currentSearchResultIndex);
        this._textEditor.highlightSearchResults(this._searchRegex, this._searchResults[this._currentSearchResultIndex]);
    },

    /**
     * @param {string} text
     */
    replaceSelectionWith: function(text)
    {
        var range = this._searchResults[this._currentSearchResultIndex];
        if (!range)
            return;
        this._textEditor.highlightSearchResults(this._searchRegex, null);

        this._isReplacing = true;
        var newRange = this._textEditor.editRange(range, text);
        delete this._isReplacing;

        this._textEditor.setSelection(newRange.collapseToEnd());
    },

    /**
     * @param {string} query
     * @param {string} replacement
     */
    replaceAllWith: function(query, replacement)
    {
        this._resetCurrentSearchResultIndex();

        var text = this._textEditor.text();
        var range = this._textEditor.range();
        var regex = WebInspector.SourceFrame.createSearchRegex(query, "g");
        if (regex.__fromRegExpQuery)
            text = text.replace(regex, replacement);
        else
            text = text.replace(regex, function() { return replacement; });

        var ranges = this._collectRegexMatches(regex);
        if (!ranges.length)
            return;

        // Calculate the position of the end of the last range to be edited.
        var currentRangeIndex = insertionIndexForObjectInListSortedByFunction(this._textEditor.selection(), ranges, WebInspector.TextRange.comparator);
        var lastRangeIndex = mod(currentRangeIndex - 1, ranges.length);
        var lastRange = ranges[lastRangeIndex];
        var replacementLineEndings = replacement.lineEndings();
        var replacementLineCount = replacementLineEndings.length;
        var lastLineNumber = lastRange.startLine + replacementLineEndings.length - 1;
        var lastColumnNumber = lastRange.startColumn;
        if (replacementLineEndings.length > 1)
            lastColumnNumber = replacementLineEndings[replacementLineCount - 1] - replacementLineEndings[replacementLineCount - 2] - 1;

        this._isReplacing = true;
        this._textEditor.editRange(range, text);
        this._textEditor.revealPosition(lastLineNumber, lastColumnNumber);
        this._textEditor.setSelection(WebInspector.TextRange.createFromLocation(lastLineNumber, lastColumnNumber));
        delete this._isReplacing;
    },

    _collectRegexMatches: function(regexObject)
    {
        var ranges = [];
        for (var i = 0; i < this._textEditor.linesCount; ++i) {
            var line = this._textEditor.line(i);
            var offset = 0;
            do {
                var match = regexObject.exec(line);
                if (match) {
                    if (match[0].length)
                        ranges.push(new WebInspector.TextRange(i, offset + match.index, i, offset + match.index + match[0].length));
                    offset += match.index + 1;
                    line = line.substring(match.index + 1);
                }
            } while (match && line);
        }
        return ranges;
    },

    _addExistingMessagesToSource: function()
    {
        var length = this._messages.length;
        for (var i = 0; i < length; ++i)
            this.addMessageToSource(this._messages[i].line - 1, this._messages[i]);
    },

    /**
     * @param {number} lineNumber
     * @param {!WebInspector.ConsoleMessage} msg
     */
    addMessageToSource: function(lineNumber, msg)
    {
        if (lineNumber >= this._textEditor.linesCount)
            lineNumber = this._textEditor.linesCount - 1;
        if (lineNumber < 0)
            lineNumber = 0;

        var rowMessages = this._rowMessages[lineNumber];
        if (!rowMessages) {
            rowMessages = [];
            this._rowMessages[lineNumber] = rowMessages;
        }

        for (var i = 0; i < rowMessages.length; ++i) {
            if (rowMessages[i].consoleMessage.isEqual(msg)) {
                rowMessages[i].repeatCount++;
                this._updateMessageRepeatCount(rowMessages[i]);
                return;
            }
        }

        var rowMessage = { consoleMessage: msg };
        rowMessages.push(rowMessage);

        this._textEditor.beginUpdates();
        var messageBubbleElement = this._messageBubbles[lineNumber];
        if (!messageBubbleElement) {
            messageBubbleElement = document.createElement("div");
            messageBubbleElement.className = "webkit-html-message-bubble";
            this._messageBubbles[lineNumber] = messageBubbleElement;
            this._textEditor.addDecoration(lineNumber, messageBubbleElement);
        }

        var imageElement = document.createElement("div");
        switch (msg.level) {
            case WebInspector.ConsoleMessage.MessageLevel.Error:
                messageBubbleElement.classList.add("webkit-html-error-message");
                imageElement.className = "error-icon-small";
                break;
            case WebInspector.ConsoleMessage.MessageLevel.Warning:
                messageBubbleElement.classList.add("webkit-html-warning-message");
                imageElement.className = "warning-icon-small";
                break;
        }

        var messageLineElement = document.createElement("div");
        messageLineElement.className = "webkit-html-message-line";
        messageBubbleElement.appendChild(messageLineElement);

        // Create the image element in the Inspector's document so we can use relative image URLs.
        messageLineElement.appendChild(imageElement);
        messageLineElement.appendChild(document.createTextNode(msg.messageText));

        rowMessage.element = messageLineElement;
        rowMessage.repeatCount = 1;
        this._updateMessageRepeatCount(rowMessage);
        this._textEditor.endUpdates();
    },

    _updateMessageRepeatCount: function(rowMessage)
    {
        if (rowMessage.repeatCount < 2)
            return;

        if (!rowMessage.repeatCountElement) {
            var repeatCountElement = document.createElement("span");
            rowMessage.element.appendChild(repeatCountElement);
            rowMessage.repeatCountElement = repeatCountElement;
        }

        rowMessage.repeatCountElement.textContent = WebInspector.UIString(" (repeated %d times)", rowMessage.repeatCount);
    },

    /**
     * @param {number} lineNumber
     * @param {!WebInspector.ConsoleMessage} msg
     */
    removeMessageFromSource: function(lineNumber, msg)
    {
        if (lineNumber >= this._textEditor.linesCount)
            lineNumber = this._textEditor.linesCount - 1;
        if (lineNumber < 0)
            lineNumber = 0;

        var rowMessages = this._rowMessages[lineNumber];
        for (var i = 0; rowMessages && i < rowMessages.length; ++i) {
            var rowMessage = rowMessages[i];
            if (rowMessage.consoleMessage !== msg)
                continue;

            var messageLineElement = rowMessage.element;
            var messageBubbleElement = messageLineElement.parentElement;
            messageBubbleElement.removeChild(messageLineElement);
            rowMessages.remove(rowMessage);
            if (!rowMessages.length)
                delete this._rowMessages[lineNumber];
            if (!messageBubbleElement.childElementCount) {
                this._textEditor.removeDecoration(lineNumber, messageBubbleElement);
                delete this._messageBubbles[lineNumber];
            }
            break;
        }
    },

    populateLineGutterContextMenu: function(contextMenu, lineNumber)
    {
    },

    populateTextAreaContextMenu: function(contextMenu, lineNumber)
    {
    },

    /**
     * @param {?WebInspector.TextRange} from
     * @param {?WebInspector.TextRange} to
     */
    onJumpToPosition: function(from, to)
    {
        this.dispatchEventToListeners(WebInspector.SourceFrame.Events.JumpHappened, {
            from: from,
            to: to
        });
    },

    inheritScrollPositions: function(sourceFrame)
    {
        this._textEditor.inheritScrollPositions(sourceFrame._textEditor);
    },

    /**
     * @return {boolean}
     */
    canEditSource: function()
    {
        return false;
    },

    /**
     * @param {!WebInspector.TextRange} textRange
     */
    selectionChanged: function(textRange)
    {
        this._updateSourcePosition();
        this.dispatchEventToListeners(WebInspector.SourceFrame.Events.SelectionChanged, textRange);
        WebInspector.notifications.dispatchEventToListeners(WebInspector.SourceFrame.Events.SelectionChanged, textRange);
    },

    _updateSourcePosition: function()
    {
        var selections = this._textEditor.selections();
        if (!selections.length)
            return;
        if (selections.length > 1) {
            this._sourcePosition.setText(WebInspector.UIString("%d selection regions", selections.length));
            return;
        }
        var textRange = selections[0];
        if (textRange.isEmpty()) {
            this._sourcePosition.setText(WebInspector.UIString("Line %d, Column %d", textRange.endLine + 1, textRange.endColumn + 1));
            return;
        }
        textRange = textRange.normalize();

        var selectedText = this._textEditor.copyRange(textRange);
        if (textRange.startLine === textRange.endLine)
            this._sourcePosition.setText(WebInspector.UIString("%d characters selected", selectedText.length));
        else
            this._sourcePosition.setText(WebInspector.UIString("%d lines, %d characters selected", textRange.endLine - textRange.startLine + 1, selectedText.length));
    },

    /**
     * @param {number} lineNumber
     */
    scrollChanged: function(lineNumber)
    {
        this.dispatchEventToListeners(WebInspector.SourceFrame.Events.ScrollChanged, lineNumber);
    },

    _handleKeyDown: function(e)
    {
        var shortcutKey = WebInspector.KeyboardShortcut.makeKeyFromEvent(e);
        var handler = this._shortcuts[shortcutKey];
        if (handler && handler())
            e.consume(true);
    },

    __proto__: WebInspector.VBox.prototype
}


/**
 * @implements {WebInspector.TextEditorDelegate}
 * @constructor
 */
WebInspector.TextEditorDelegateForSourceFrame = function(sourceFrame)
{
    this._sourceFrame = sourceFrame;
}

WebInspector.TextEditorDelegateForSourceFrame.prototype = {
    onTextChanged: function(oldRange, newRange)
    {
        this._sourceFrame.onTextChanged(oldRange, newRange);
    },

    /**
     * @param {!WebInspector.TextRange} textRange
     */
    selectionChanged: function(textRange)
    {
        this._sourceFrame.selectionChanged(textRange);
    },

    /**
     * @param {number} lineNumber
     */
    scrollChanged: function(lineNumber)
    {
        this._sourceFrame.scrollChanged(lineNumber);
    },

    editorFocused: function()
    {
        this._sourceFrame._editorFocused();
    },

    populateLineGutterContextMenu: function(contextMenu, lineNumber)
    {
        this._sourceFrame.populateLineGutterContextMenu(contextMenu, lineNumber);
    },

    populateTextAreaContextMenu: function(contextMenu, lineNumber)
    {
        this._sourceFrame.populateTextAreaContextMenu(contextMenu, lineNumber);
    },

    /**
     * @param {string} hrefValue
     * @param {boolean} isExternal
     * @return {!Element}
     */
    createLink: function(hrefValue, isExternal)
    {
        var targetLocation = WebInspector.ParsedURL.completeURL(this._sourceFrame._url, hrefValue);
        return WebInspector.linkifyURLAsNode(targetLocation || hrefValue, hrefValue, undefined, isExternal);
    },

    /**
     * @param {?WebInspector.TextRange} from
     * @param {?WebInspector.TextRange} to
     */
    onJumpToPosition: function(from, to)
    {
        this._sourceFrame.onJumpToPosition(from, to);
    }
}

importScript("GoToLineDialog.js");
importScript("ResourceView.js");
importScript("FontView.js");
importScript("ImageView.js");
