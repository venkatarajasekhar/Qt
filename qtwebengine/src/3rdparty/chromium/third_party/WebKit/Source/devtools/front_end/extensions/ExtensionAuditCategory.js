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
 * @implements {WebInspector.AuditCategory}
 * @param {string} extensionOrigin
 * @param {string} id
 * @param {string} displayName
 * @param {number=} ruleCount
 */
WebInspector.ExtensionAuditCategory = function(extensionOrigin, id, displayName, ruleCount)
{
    this._extensionOrigin = extensionOrigin;
    this._id = id;
    this._displayName = displayName;
    this._ruleCount  = ruleCount;
}

WebInspector.ExtensionAuditCategory.prototype = {
    /**
     * @override
     */
    get id()
    {
        return this._id;
    },

    /**
     * @override
     */
    get displayName()
    {
        return this._displayName;
    },

    /**
     * @override
     * @param {!WebInspector.Target} target
     * @param {!Array.<!WebInspector.NetworkRequest>} requests
            * @param {function(!WebInspector.AuditRuleResult)} ruleResultCallback
     * @param {function()} categoryDoneCallback
     * @param {!WebInspector.Progress} progress
     */
    run: function(target, requests, ruleResultCallback, categoryDoneCallback, progress)
    {
        var results = new WebInspector.ExtensionAuditCategoryResults(this, target, ruleResultCallback, categoryDoneCallback, progress);
        WebInspector.extensionServer.startAuditRun(this, results);
    }
}

/**
 * @constructor
 * @param {!WebInspector.ExtensionAuditCategory} category
 * @param {!WebInspector.Target} target
 * @param {function(!WebInspector.AuditRuleResult)} ruleResultCallback
 * @param {function()} categoryDoneCallback
 * @param {!WebInspector.Progress} progress
 */
WebInspector.ExtensionAuditCategoryResults = function(category, target, ruleResultCallback, categoryDoneCallback, progress)
{
    this._target = target;
    this._category = category;
    this._ruleResultCallback = ruleResultCallback;
    this._categoryDoneCallback = categoryDoneCallback;
    this._progress = progress;
    this._progress.setTotalWork(1);
    this._expectedResults = category._ruleCount;
    this._actualResults = 0;

    this.id = category.id + "-" + ++WebInspector.ExtensionAuditCategoryResults._lastId;
}

WebInspector.ExtensionAuditCategoryResults.prototype = {
    done: function()
    {
        WebInspector.extensionServer.stopAuditRun(this);
        this._progress.done();
        this._categoryDoneCallback();
    },

    addResult: function(displayName, description, severity, details)
    {
        var result = new WebInspector.AuditRuleResult(displayName);
        result.addChild(description);
        result.severity = severity;
        if (details)
            this._addNode(result, details);
        this._addResult(result);
    },

    _addNode: function(parent, node)
    {
        var contents = WebInspector.auditFormatters.partiallyApply(WebInspector.ExtensionAuditFormatters, this, node.contents);
        var addedNode = parent.addChild(contents, node.expanded);
        if (node.children) {
            for (var i = 0; i < node.children.length; ++i)
                this._addNode(addedNode, node.children[i]);
        }
    },

    _addResult: function(result)
    {
        this._ruleResultCallback(result);
        ++this._actualResults;
        if (typeof this._expectedResults === "number") {
            this._progress.setWorked(this._actualResults / this._expectedResults);
            if (this._actualResults === this._expectedResults)
                this.done();
        }
    },

    /**
     * @param {number} progress
     */
    updateProgress: function(progress)
    {
        this._progress.setWorked(progress);
    },

    /**
     * @param {string} expression
     * @param {?Object} evaluateOptions
     * @param {function(!WebInspector.RemoteObject)} callback
     */
    evaluate: function(expression, evaluateOptions, callback)
    {
        /**
         * @param {?string} error
         * @param {!RuntimeAgent.RemoteObject} result
         * @param {boolean=} wasThrown
         * @this {WebInspector.ExtensionAuditCategoryResults}
         */
        function onEvaluate(error, result, wasThrown)
        {
            if (wasThrown)
                return;
            var object = this._target.runtimeModel.createRemoteObject(result);
            callback(object);
        }
        WebInspector.extensionServer.evaluate(expression, false, false, evaluateOptions, this._category._extensionOrigin, onEvaluate.bind(this));
    }
}

WebInspector.ExtensionAuditFormatters = {
    /**
     * @this {WebInspector.ExtensionAuditCategoryResults}
     * @param {string} expression
     * @param {string} title
     * @param {?Object} evaluateOptions
     * @return {!Element}
     */
    object: function(expression, title, evaluateOptions)
    {
        var parentElement = document.createElement("div");
        function onEvaluate(remoteObject)
        {
            var section = new WebInspector.ObjectPropertiesSection(remoteObject, title);
            section.expanded = true;
            section.editable = false;
            parentElement.appendChild(section.element);
        }
        this.evaluate(expression, evaluateOptions, onEvaluate);
        return parentElement;
    },

    /**
     * @this {WebInspector.ExtensionAuditCategoryResults}
     * @param {string} expression
     * @param {?Object} evaluateOptions
     * @return {!Element}
     */
    node: function(expression, evaluateOptions)
    {
        var parentElement = document.createElement("div");
        /**
         * @param {?WebInspector.DOMNode} node
         */
        function onNodeAvailable(node)
        {
            if (!node)
                return;
            var renderer = WebInspector.moduleManager.instance(WebInspector.Renderer, node);
            if (renderer)
                parentElement.appendChild(renderer.render(node));
            else
                console.error("No renderer for node found");
        }
        /**
         * @param {!WebInspector.RemoteObject} remoteObject
         */
        function onEvaluate(remoteObject)
        {
            remoteObject.pushNodeToFrontend(onNodeAvailable);
        }
        this.evaluate(expression, evaluateOptions, onEvaluate);
        return parentElement;
    }
}

WebInspector.ExtensionAuditCategoryResults._lastId = 0;
