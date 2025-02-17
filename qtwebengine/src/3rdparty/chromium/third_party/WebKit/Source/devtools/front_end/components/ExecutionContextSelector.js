// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @implements {WebInspector.TargetManager.Observer}
 */
WebInspector.ExecutionContextSelector = function()
{
    WebInspector.targetManager.observeTargets(this);
    WebInspector.context.addFlavorChangeListener(WebInspector.ExecutionContext, this._executionContextChanged, this);
    WebInspector.context.addFlavorChangeListener(WebInspector.Target, this._targetChanged, this);
}

WebInspector.ExecutionContextSelector.prototype = {

    /**
     * @param {!WebInspector.Target} target
     */
    targetAdded: function(target)
    {
        if (!WebInspector.context.flavor(WebInspector.Target))
            WebInspector.context.setFlavor(WebInspector.Target, target);

        target.runtimeModel.addEventListener(WebInspector.RuntimeModel.Events.ExecutionContextCreated, this._onExecutionContextCreated, this);
        target.runtimeModel.addEventListener(WebInspector.RuntimeModel.Events.ExecutionContextDestroyed, this._onExecutionContextDestroyed, this);
    },

    /**
     * @param {!WebInspector.Target} target
     */
    targetRemoved: function(target)
    {
        target.runtimeModel.removeEventListener(WebInspector.RuntimeModel.Events.ExecutionContextCreated, this._onExecutionContextCreated, this);
        target.runtimeModel.removeEventListener(WebInspector.RuntimeModel.Events.ExecutionContextDestroyed, this._onExecutionContextDestroyed, this);
        var currentExecutionContext = WebInspector.context.flavor(WebInspector.Target);
        if (currentExecutionContext && currentExecutionContext.target() === target)
            this._currentExecutionContextGone();

        var targets = WebInspector.targetManager.targets();
        if (WebInspector.context.flavor(WebInspector.Target) === target && targets.length)
            WebInspector.context.setFlavor(WebInspector.Target, targets[0]);
    },

    /**
     * @param {!WebInspector.Event} event
     */
    _executionContextChanged: function(event)
    {
        var newContext = /** @type {?WebInspector.ExecutionContext} */ (event.data);
        if (newContext)
            WebInspector.context.setFlavor(WebInspector.Target, newContext.target());
    },

    /**
     * @param {!WebInspector.Event} event
     */
    _targetChanged: function(event)
    {
        var newTarget = /** @type {?WebInspector.Target} */(event.data);
        var currentContext = WebInspector.context.flavor(WebInspector.ExecutionContext);

        if (!newTarget || (currentContext && currentContext.target() === newTarget))
            return;

        var executionContexts = newTarget.runtimeModel.executionContexts();
        if (!executionContexts.length)
            return;

        var newContext = executionContexts[0];
        for (var i = 1; i < executionContexts.length; ++i) {
            if (executionContexts[i].isMainWorldContext)
                newContext = executionContexts[i];
        }
        WebInspector.context.setFlavor(WebInspector.ExecutionContext, newContext);
    },

    /**
     * @param {!WebInspector.Event} event
     */
    _onExecutionContextCreated: function(event)
    {
        var executionContext = /** @type {!WebInspector.ExecutionContext}*/ (event.data);
        if (!WebInspector.context.flavor(WebInspector.ExecutionContext))
            WebInspector.context.setFlavor(WebInspector.ExecutionContext, executionContext);
    },

    /**
     * @param {!WebInspector.Event} event
     */
    _onExecutionContextDestroyed: function(event)
    {
        var executionContext = /** @type {!WebInspector.ExecutionContext}*/ (event.data);
        if (WebInspector.context.flavor(WebInspector.ExecutionContext) === executionContext)
            this._currentExecutionContextGone();
    },

    _currentExecutionContextGone: function()
    {
        var targets = WebInspector.targetManager.targets();
        var newContext = null;
        for (var i = 0; i < targets.length; ++i) {
            var executionContexts = targets[i].runtimeModel.executionContexts();
            if (executionContexts.length) {
                newContext = executionContexts[0];
                break;
            }
        }
        WebInspector.context.setFlavor(WebInspector.ExecutionContext, newContext);
    }

}

/**
 * @param {!Element} proxyElement
 * @param {!Range} wordRange
 * @param {boolean} force
 * @param {function(!Array.<string>, number=)} completionsReadyCallback
 */
WebInspector.ExecutionContextSelector.completionsForTextPromptInCurrentContext = function(proxyElement, wordRange, force, completionsReadyCallback)
{
    var executionContext = WebInspector.context.flavor(WebInspector.ExecutionContext);
    if (!executionContext) {
        completionsReadyCallback([]);
        return;
    }

    // Pass less stop characters to rangeOfWord so the range will be a more complete expression.
    var expressionRange = wordRange.startContainer.rangeOfWord(wordRange.startOffset, " =:[({;,!+-*/&|^<>", proxyElement, "backward");
    var expressionString = expressionRange.toString();
    var prefix = wordRange.toString();
    executionContext.completionsForExpression(expressionString, prefix, force, completionsReadyCallback);
}