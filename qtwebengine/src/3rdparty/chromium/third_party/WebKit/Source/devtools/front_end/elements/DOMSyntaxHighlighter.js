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
 */
WebInspector.DOMSyntaxHighlighter = function(mimeType, stripExtraWhitespace)
{
    this._mimeType = mimeType;
    this._stripExtraWhitespace = stripExtraWhitespace;
}

WebInspector.DOMSyntaxHighlighter.prototype = {
    /**
     * @param {string} content
     * @param {string} className
     * @return {!Element}
     */
    createSpan: function(content, className)
    {
        var span = document.createElement("span");
        span.className = "cm-" + className;
        if (this._stripExtraWhitespace && className !== "whitespace")
            content = content.replace(/^[\n\r]*/, "").replace(/\s*$/, "");
        span.appendChild(document.createTextNode(content));
        return span;
    },

    syntaxHighlightNode: function(node)
    {
        var lines = node.textContent.split("\n");
        node.removeChildren();

        /**
         * @param {string} token
         * @param {?string} tokenType
         * @param {number} column
         * @param {number} newColumn
         * @this {WebInspector.DOMSyntaxHighlighter}
         */
        function processToken(token, tokenType, column, newColumn)
        {
            if (!tokenType)
                return;

            if (column > plainTextStart) {
                var plainText = line.substring(plainTextStart, column);
                node.appendChild(document.createTextNode(plainText));
            }
            node.appendChild(this.createSpan(token, tokenType));
            plainTextStart = newColumn;
        }

        var tokenize = WebInspector.moduleManager.instance(WebInspector.TokenizerFactory).createTokenizer(this._mimeType);
        for (var i = lines[0].length ? 0 : 1; i < lines.length; ++i) {
            var line = lines[i];
            var plainTextStart = 0;
            tokenize(line, processToken.bind(this));
            if (plainTextStart < line.length) {
                var plainText = line.substring(plainTextStart, line.length);
                node.appendChild(document.createTextNode(plainText));
            }
            if (i < lines.length - 1)
                node.appendChild(document.createElement("br"));
        }
    }
}
