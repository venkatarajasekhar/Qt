/*
 * Copyright (C) 2007, 2008 Apple Inc.  All rights reserved.
 * Copyright (C) IBM Corp. 2009  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @extends {WebInspector.VBox}
 * @constructor
 */
WebInspector.ResourceView = function(resource)
{
    WebInspector.VBox.call(this);
    this.registerRequiredCSS("resourceView.css");

    this.element.classList.add("resource-view");
    this.resource = resource;
}

WebInspector.ResourceView.prototype = {
    /**
     * @return {boolean}
     */
    hasContent: function()
    {
        return false;
    },

    __proto__: WebInspector.VBox.prototype
}

/**
 * @param {!WebInspector.Resource} resource
 * @return {boolean}
 */
WebInspector.ResourceView.hasTextContent = function(resource)
{
    if (resource.type.isTextType())
        return true;
    if (resource.type === WebInspector.resourceTypes.Other)
        return !!resource.content && !resource.contentEncoded;
    return false;
}

/**
 * @param {!WebInspector.Resource} resource
 * @return {!WebInspector.ResourceView}
 */
WebInspector.ResourceView.nonSourceViewForResource = function(resource)
{
    switch (resource.type) {
    case WebInspector.resourceTypes.Image:
        return new WebInspector.ImageView(resource);
    case WebInspector.resourceTypes.Font:
        return new WebInspector.FontView(resource);
    default:
        return new WebInspector.ResourceView(resource);
    }
}

/**
 * @extends {WebInspector.SourceFrame}
 * @constructor
 * @param {!WebInspector.Resource} resource
 */
WebInspector.ResourceSourceFrame = function(resource)
{
    this._resource = resource;
    WebInspector.SourceFrame.call(this, resource);
}

WebInspector.ResourceSourceFrame.prototype = {
    get resource()
    {
        return this._resource;
    },

    populateTextAreaContextMenu: function(contextMenu, lineNumber)
    {
        contextMenu.appendApplicableItems(this._resource);
    },

    __proto__: WebInspector.SourceFrame.prototype
}

/**
 * @constructor
 * @extends {WebInspector.VBox}
 * @param {!WebInspector.Resource} resource
 */
WebInspector.ResourceSourceFrameFallback = function(resource)
{
    WebInspector.VBox.call(this);
    this._resource = resource;
    this.element.classList.add("script-view");
    this._content = this.element.createChild("div", "script-view-fallback monospace");
}

WebInspector.ResourceSourceFrameFallback.prototype = {
    wasShown: function()
    {
        if (!this._contentRequested) {
            this._contentRequested = true;
            this._resource.requestContent(this._contentLoaded.bind(this));
        }
    },

    /**
     * @param {?string} content
     */
    _contentLoaded: function(content)
    {
        this._content.textContent = content;
    },

    __proto__: WebInspector.VBox.prototype
}
