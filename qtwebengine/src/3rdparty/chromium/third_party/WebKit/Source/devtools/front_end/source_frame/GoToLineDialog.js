/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * @extends {WebInspector.DialogDelegate}
 * @param {!WebInspector.SourceFrame} sourceFrame
 */
WebInspector.GoToLineDialog = function(sourceFrame)
{
    WebInspector.DialogDelegate.call(this);

    this.element = document.createElement("div");
    this.element.className = "go-to-line-dialog";

    this.element.createChild("label").textContent = WebInspector.UIString("Go to line: ");

    this._input = this.element.createChild("input");
    this._input.setAttribute("type", "text");
    this._input.setAttribute("size", 6);

    this._goButton = this.element.createChild("button");
    this._goButton.textContent = WebInspector.UIString("Go");
    this._goButton.addEventListener("click", this._onGoClick.bind(this), false);

    this._sourceFrame = sourceFrame;
}

/**
 * @param {!WebInspector.Panel} panel
 * @param {function():?WebInspector.SourceFrame} sourceFrameGetter
 */
WebInspector.GoToLineDialog.install = function(panel, sourceFrameGetter)
{
    var goToLineShortcut = WebInspector.GoToLineDialog.createShortcut();
    panel.registerShortcuts([goToLineShortcut], WebInspector.GoToLineDialog._show.bind(null, sourceFrameGetter));
}

/**
 * @param {function():?WebInspector.SourceFrame} sourceFrameGetter
 * @param {?Event=} event
 * @return {boolean}
 */
WebInspector.GoToLineDialog._show = function(sourceFrameGetter, event)
{
    var sourceFrame = sourceFrameGetter();
    if (!sourceFrame)
        return false;
    WebInspector.Dialog.show(sourceFrame.element, new WebInspector.GoToLineDialog(sourceFrame));
    return true;
}

/**
 * @return {!WebInspector.KeyboardShortcut.Descriptor}
 */
WebInspector.GoToLineDialog.createShortcut = function()
{
    return WebInspector.KeyboardShortcut.makeDescriptor("g", WebInspector.KeyboardShortcut.Modifiers.Ctrl);
}

WebInspector.GoToLineDialog.prototype = {
    focus: function()
    {
        WebInspector.setCurrentFocusElement(this._input);
        this._input.select();
    },

    _onGoClick: function()
    {
        this._applyLineNumber();
        WebInspector.Dialog.hide();
    },

    _applyLineNumber: function()
    {
        var value = this._input.value;
        var lineNumber = parseInt(value, 10) - 1;
        if (!isNaN(lineNumber) && lineNumber >= 0)
            this._sourceFrame.revealPosition(lineNumber, 0, true);
    },

    onEnter: function()
    {
        this._applyLineNumber();
    },

    __proto__: WebInspector.DialogDelegate.prototype
}
