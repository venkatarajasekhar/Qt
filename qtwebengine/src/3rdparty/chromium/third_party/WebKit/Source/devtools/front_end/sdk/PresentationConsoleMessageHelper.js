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
 * @implements {WebInspector.TargetManager.Observer}
 * @param {!WebInspector.Workspace} workspace
 */
WebInspector.PresentationConsoleMessageHelper = function(workspace)
{
    /**
     * @type {!Object.<string, !Array.<!WebInspector.ConsoleMessage>>}
     */
    this._pendingConsoleMessages = {};
    this._presentationConsoleMessages = [];
    this._workspace = workspace;

    WebInspector.targetManager.observeTargets(this);
}

WebInspector.PresentationConsoleMessageHelper.prototype = {
    /**
     * @param {!WebInspector.Target} target
     */
    targetAdded: function(target)
    {
        target.consoleModel.addEventListener(WebInspector.ConsoleModel.Events.MessageAdded, this._consoleMessageAdded, this);
        target.consoleModel.addEventListener(WebInspector.ConsoleModel.Events.ConsoleCleared, this._consoleCleared, this);

        target.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.ParsedScriptSource, this._parsedScriptSource, this);
        target.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.FailedToParseScriptSource, this._parsedScriptSource, this);
        target.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.GlobalObjectCleared, this._debuggerReset, this);
    },

    /**
     * @param {!WebInspector.Target} target
     */
    targetRemoved: function(target)
    {
        target.consoleModel.removeEventListener(WebInspector.ConsoleModel.Events.MessageAdded, this._consoleMessageAdded, this);
        target.consoleModel.removeEventListener(WebInspector.ConsoleModel.Events.ConsoleCleared, this._consoleCleared, this);

        target.debuggerModel.removeEventListener(WebInspector.DebuggerModel.Events.ParsedScriptSource, this._parsedScriptSource, this);
        target.debuggerModel.removeEventListener(WebInspector.DebuggerModel.Events.FailedToParseScriptSource, this._parsedScriptSource, this);
        target.debuggerModel.removeEventListener(WebInspector.DebuggerModel.Events.GlobalObjectCleared, this._debuggerReset, this);
    },

    /**
     * @param {!WebInspector.Event} event
     */
    _consoleMessageAdded: function(event)
    {
        var message = /** @type {!WebInspector.ConsoleMessage} */ (event.data);
        if (!message.url || !message.isErrorOrWarning())
            return;

        var rawLocation = this._rawLocation(message);
        if (rawLocation)
            this._addConsoleMessageToScript(message, rawLocation);
        else
            this._addPendingConsoleMessage(message);
    },

    /**
     * @param {!WebInspector.ConsoleMessage} message
     * @return {?WebInspector.DebuggerModel.Location}
     */
    _rawLocation: function(message)
    {
        // FIXME(62725): stack trace line/column numbers are one-based.
        var lineNumber = message.stackTrace ? message.stackTrace[0].lineNumber - 1 : message.line - 1;
        var columnNumber = message.stackTrace && message.stackTrace[0].columnNumber ? message.stackTrace[0].columnNumber - 1 : 0;
        return message.target().debuggerModel.createRawLocationByURL(message.url || "", lineNumber, columnNumber);
    },

    /**
     * @param {!WebInspector.ConsoleMessage} message
     * @param {!WebInspector.DebuggerModel.Location} rawLocation
     */
    _addConsoleMessageToScript: function(message, rawLocation)
    {
        this._presentationConsoleMessages.push(new WebInspector.PresentationConsoleMessage(message, rawLocation));
    },

    /**
     * @param {!WebInspector.ConsoleMessage} message
     */
    _addPendingConsoleMessage: function(message)
    {
        if (!message.url)
            return;
        if (!this._pendingConsoleMessages[message.url])
            this._pendingConsoleMessages[message.url] = [];
        this._pendingConsoleMessages[message.url].push(message);
    },

    /**
     * @param {!WebInspector.Event} event
     */
    _parsedScriptSource: function(event)
    {
        var script = /** @type {!WebInspector.Script} */ (event.data);

        var messages = this._pendingConsoleMessages[script.sourceURL];
        if (!messages)
            return;

        var pendingMessages = [];
        for (var i = 0; i < messages.length; i++) {
            var message = messages[i];
            var rawLocation = this._rawLocation(message);
            if (script.scriptId === rawLocation.scriptId)
                this._addConsoleMessageToScript(message, rawLocation);
            else
                pendingMessages.push(message);
        }

        if (pendingMessages.length)
            this._pendingConsoleMessages[script.sourceURL] = pendingMessages;
        else
            delete this._pendingConsoleMessages[script.sourceURL];
    },

    _consoleCleared: function()
    {
        this._pendingConsoleMessages = {};
        for (var i = 0; i < this._presentationConsoleMessages.length; ++i)
            this._presentationConsoleMessages[i].dispose();
        this._presentationConsoleMessages = [];
        var uiSourceCodes = this._workspace.uiSourceCodes();
        for (var i = 0; i < uiSourceCodes.length; ++i)
            uiSourceCodes[i].consoleMessagesCleared();
    },

    _debuggerReset: function()
    {
        this._pendingConsoleMessages = {};
        this._presentationConsoleMessages = [];
    }
}

/**
 * @constructor
 * @param {!WebInspector.ConsoleMessage} message
 * @param {!WebInspector.DebuggerModel.Location} rawLocation
 */
WebInspector.PresentationConsoleMessage = function(message, rawLocation)
{
    this.originalMessage = message;
    this._liveLocation = rawLocation.createLiveLocation(this._updateLocation.bind(this));
}

WebInspector.PresentationConsoleMessage.prototype = {
    /**
     * @param {!WebInspector.UILocation} uiLocation
     */
    _updateLocation: function(uiLocation)
    {
        if (this._uiLocation)
            this._uiLocation.uiSourceCode.consoleMessageRemoved(this);
        this._uiLocation = uiLocation;
        this._uiLocation.uiSourceCode.consoleMessageAdded(this);
    },

    get lineNumber()
    {
        return this._uiLocation.lineNumber;
    },

    dispose: function()
    {
        this._liveLocation.dispose();
    }
}
