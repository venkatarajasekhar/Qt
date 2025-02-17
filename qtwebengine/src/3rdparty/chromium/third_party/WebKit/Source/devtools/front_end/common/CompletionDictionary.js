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
 * @interface
 */
WebInspector.CompletionDictionary = function() { }

WebInspector.CompletionDictionary.prototype = {
    /**
     * @param {string} word
     */
    addWord: function(word) { },

    /**
     * @param {string} word
     */
    removeWord: function(word) { },

    /**
     * @param {string} word
     * @return {boolean}
     */
    hasWord: function(word) { },

    /**
     * @param {string} prefix
     * @return {!Array.<string>}
     */
    wordsWithPrefix: function(prefix) { },

    /**
     * @param {string} word
     * @return {number}
     */
    wordCount: function(word) { },

    reset: function() { }
}

/**
 * @constructor
 * @implements {WebInspector.CompletionDictionary}
 */
WebInspector.SampleCompletionDictionary = function() {
    this._words = {};
}

WebInspector.SampleCompletionDictionary.prototype = {
    /**
     * @param {string} word
     */
    addWord: function(word)
    {
        if (!this._words[word])
            this._words[word] = 1;
        else
            ++this._words[word];
    },

    /**
     * @param {string} word
     */
    removeWord: function(word)
    {
        if (!this._words[word])
            return;
        if (this._words[word] === 1)
            delete this._words[word];
        else
            --this._words[word];
    },

    /**
     * @param {string} prefix
     * @return {!Array.<string>}
     */
    wordsWithPrefix: function(prefix)
    {
        var words = [];
        for(var i in this._words) {
            if (i.startsWith(prefix))
                words.push(i);
        }
        return words;
    },

    /**
     * @param {string} word
     * @return {boolean}
     */
    hasWord: function(word)
    {
        return !!this._words[word];
    },

    /**
     * @param {string} word
     * @return {number}
     */
    wordCount: function(word)
    {
        return this._words[word] ? this._words[word] : 0;
    },

    reset: function()
    {
        this._words = {};
    }
}
