/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 * @param {!WebInspector.CSSStyleModel} cssModel
 * @param {!WebInspector.Workspace} workspace
 * @param {!WebInspector.NetworkWorkspaceBinding} networkWorkspaceBinding
 */
WebInspector.CSSStyleSheetMapping = function(cssModel, workspace, networkWorkspaceBinding)
{
    this._cssModel = cssModel;
    this._workspace = workspace;
    this._stylesSourceMapping = new WebInspector.StylesSourceMapping(cssModel, workspace);
    this._sassSourceMapping = new WebInspector.SASSSourceMapping(cssModel, workspace, networkWorkspaceBinding);

    cssModel.addEventListener(WebInspector.CSSStyleModel.Events.StyleSheetAdded, this._styleSheetAdded, this);
    cssModel.addEventListener(WebInspector.CSSStyleModel.Events.StyleSheetRemoved, this._styleSheetRemoved, this);
}

WebInspector.CSSStyleSheetMapping.prototype = {
    /**
     * @param {!WebInspector.Event} event
     */
    _styleSheetAdded: function(event)
    {
        var header = /** @type {!WebInspector.CSSStyleSheetHeader} */ (event.data);
        this._stylesSourceMapping.addHeader(header);
        this._sassSourceMapping.addHeader(header);
    },

    /**
     * @param {!WebInspector.Event} event
     */
    _styleSheetRemoved: function(event)
    {
        var header = /** @type {!WebInspector.CSSStyleSheetHeader} */ (event.data);
        this._stylesSourceMapping.removeHeader(header);
        this._sassSourceMapping.removeHeader(header);
    }
}
