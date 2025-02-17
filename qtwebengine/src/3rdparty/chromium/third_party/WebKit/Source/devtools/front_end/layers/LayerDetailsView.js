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
 * @extends {WebInspector.VBox}
 */
WebInspector.LayerDetailsView = function()
{
    WebInspector.VBox.call(this);
    this.element.classList.add("layer-details-view");
    this._emptyView = new WebInspector.EmptyView(WebInspector.UIString("Select a layer to see its details"));
    this._createTable();
}

/**
 * @enum {string}
 */
WebInspector.LayerDetailsView.Events = {
    ObjectSelected: "ObjectSelected"
}

/**
 * @type {!Object.<string, string>}
 */
WebInspector.LayerDetailsView.CompositingReasonDetail = {
    "transform3D": WebInspector.UIString("Composition due to association with an element with a CSS 3D transform."),
    "video": WebInspector.UIString("Composition due to association with a <video> element."),
    "canvas": WebInspector.UIString("Composition due to the element being a <canvas> element."),
    "plugin": WebInspector.UIString("Composition due to association with a plugin."),
    "iFrame": WebInspector.UIString("Composition due to association with an <iframe> element."),
    "backfaceVisibilityHidden": WebInspector.UIString("Composition due to association with an element with a \"backface-visibility: hidden\" style."),
    "animation": WebInspector.UIString("Composition due to association with an animated element."),
    "filters": WebInspector.UIString("Composition due to association with an element with CSS filters applied."),
    "positionFixed": WebInspector.UIString("Composition due to association with an element with a \"position: fixed\" style."),
    "positionSticky": WebInspector.UIString("Composition due to association with an element with a \"position: sticky\" style."),
    "overflowScrollingTouch": WebInspector.UIString("Composition due to association with an element with a \"overflow-scrolling: touch\" style."),
    "blending": WebInspector.UIString("Composition due to association with an element that has blend mode other than \"normal\"."),
    "assumedOverlap": WebInspector.UIString("Composition due to association with an element that may overlap other composited elements."),
    "overlap": WebInspector.UIString("Composition due to association with an element overlapping other composited elements."),
    "negativeZIndexChildren": WebInspector.UIString("Composition due to association with an element with descendants that have a negative z-index."),
    "transformWithCompositedDescendants": WebInspector.UIString("Composition due to association with an element with composited descendants."),
    "opacityWithCompositedDescendants": WebInspector.UIString("Composition due to association with an element with opacity applied and composited descendants."),
    "maskWithCompositedDescendants": WebInspector.UIString("Composition due to association with a masked element and composited descendants."),
    "reflectionWithCompositedDescendants": WebInspector.UIString("Composition due to association with an element with a reflection and composited descendants."),
    "filterWithCompositedDescendants": WebInspector.UIString("Composition due to association with an element with CSS filters applied and composited descendants."),
    "blendingWithCompositedDescendants": WebInspector.UIString("Composition due to association with an element with CSS blending applied and composited descendants."),
    "clipsCompositingDescendants": WebInspector.UIString("Composition due to association with an element clipping compositing descendants."),
    "perspective": WebInspector.UIString("Composition due to association with an element with perspective applied."),
    "preserve3D": WebInspector.UIString("Composition due to association with an element with a \"transform-style: preserve-3d\" style."),
    "root": WebInspector.UIString("Root layer."),
    "layerForClip": WebInspector.UIString("Layer for clip."),
    "layerForScrollbar": WebInspector.UIString("Layer for scrollbar."),
    "layerForScrollingContainer": WebInspector.UIString("Layer for scrolling container."),
    "layerForForeground": WebInspector.UIString("Layer for foreground."),
    "layerForBackground": WebInspector.UIString("Layer for background."),
    "layerForMask": WebInspector.UIString("Layer for mask."),
    "layerForVideoOverlay": WebInspector.UIString("Layer for video overlay.")
};

WebInspector.LayerDetailsView.prototype = {
    /**
     * @param {!WebInspector.Layers3DView.ActiveObject} activeObject
     */
    setObject: function(activeObject)
    {
        this._layer = activeObject ? activeObject.layer : null;
        this._scrollRectIndex = activeObject ? activeObject.scrollRectIndex : null;
        if (this.isShowing())
            this.update();
    },

    wasShown: function()
    {
        WebInspector.View.prototype.wasShown.call(this);
        this.update();
    },

    /**
     * @param {number} index
     * @param {?Event} event
     */
    _onScrollRectClicked: function(index, event)
    {
        if (event.which !== 1)
            return;
        this.dispatchEventToListeners(WebInspector.LayerDetailsView.Events.ObjectSelected, {layer: this._layer, scrollRectIndex: index});
    },

    /**
     * @param {!LayerTreeAgent.ScrollRect} scrollRect
     * @param {number} index
     */
    _createScrollRectElement: function(scrollRect, index)
    {
        if (index)
            this._scrollRectsCell.createTextChild(", ");
        var element = this._scrollRectsCell.createChild("span");
        element.className = index === this._scrollRectIndex ? "scroll-rect active" : "scroll-rect";
        element.textContent = WebInspector.LayerTreeModel.ScrollRectType[scrollRect.type].description + " (" + scrollRect.rect.x + ", " + scrollRect.rect.y +
            ", " + scrollRect.rect.width + ", " + scrollRect.rect.height + ")";
        element.addEventListener("click", this._onScrollRectClicked.bind(this, index), false);
    },

    update: function()
    {
        if (!this._layer) {
            this._tableElement.remove();
            this._emptyView.show(this.element);
            return;
        }
        this._emptyView.detach();
        this.element.appendChild(this._tableElement);
        this._positionCell.textContent = WebInspector.UIString("%d,%d", this._layer.offsetX(), this._layer.offsetY());
        this._sizeCell.textContent = WebInspector.UIString("%d × %d", this._layer.width(), this._layer.height());
        this._paintCountCell.textContent = this._layer.paintCount();
        const bytesPerPixel = 4;
        this._memoryEstimateCell.textContent = Number.bytesToString(this._layer.invisible() ? 0 : this._layer.width() * this._layer.height() * bytesPerPixel);
        this._layer.requestCompositingReasons(this._updateCompositingReasons.bind(this));
        this._scrollRectsCell.removeChildren();
        this._layer.scrollRects().forEach(this._createScrollRectElement.bind(this));
    },

    _createTable: function()
    {
        this._tableElement = this.element.createChild("table");
        this._tbodyElement = this._tableElement.createChild("tbody");
        this._positionCell = this._createRow(WebInspector.UIString("Position in parent:"));
        this._sizeCell = this._createRow(WebInspector.UIString("Size:"));
        this._compositingReasonsCell = this._createRow(WebInspector.UIString("Compositing Reasons:"));
        this._memoryEstimateCell = this._createRow(WebInspector.UIString("Memory estimate:"));
        this._paintCountCell = this._createRow(WebInspector.UIString("Paint count:"));
        this._scrollRectsCell = this._createRow(WebInspector.UIString("Slow scroll regions:"));
    },

    /**
     * @param {string} title
     */
    _createRow: function(title)
    {
        var tr = this._tbodyElement.createChild("tr");
        var titleCell = tr.createChild("td");
        titleCell.textContent = title;
        return tr.createChild("td");
    },

    /**
     * @param {!Array.<string>} compositingReasons
     */
    _updateCompositingReasons: function(compositingReasons)
    {
        if (!compositingReasons || !compositingReasons.length) {
            this._compositingReasonsCell.textContent = "n/a";
            return;
        }
        var fragment = document.createDocumentFragment();
        for (var i = 0; i < compositingReasons.length; ++i) {
            if (i)
                fragment.appendChild(document.createTextNode(","));
            var span = document.createElement("span");
            span.title = WebInspector.LayerDetailsView.CompositingReasonDetail[compositingReasons[i]] || "";
            span.textContent = compositingReasons[i];
            fragment.appendChild(span);
        }
        this._compositingReasonsCell.removeChildren();
        this._compositingReasonsCell.appendChild(fragment);
    },

    __proto__: WebInspector.VBox.prototype
}
