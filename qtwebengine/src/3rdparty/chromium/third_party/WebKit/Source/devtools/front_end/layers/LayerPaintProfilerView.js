// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @param {function(!WebInspector.Layer, string=)} showImageForLayerCallback
 * @extends {WebInspector.SplitView}
 */
WebInspector.LayerPaintProfilerView = function(showImageForLayerCallback)
{
    WebInspector.SplitView.call(this, true, false);

    this._showImageForLayerCallback = showImageForLayerCallback;
    this._logTreeView = new WebInspector.PaintProfilerCommandLogView();
    this._logTreeView.show(this.sidebarElement());
    this._paintProfilerView = new WebInspector.PaintProfilerView(this._showImage.bind(this));
    this._paintProfilerView.show(this.mainElement());

    this._paintProfilerView.addEventListener(WebInspector.PaintProfilerView.Events.WindowChanged, this._onWindowChanged, this);
}

WebInspector.LayerPaintProfilerView.prototype = {
    /**
     * @param {!WebInspector.Layer} layer
     */
    profileLayer: function(layer)
    {
        layer.requestSnapshot(onSnapshotDone.bind(this));

        /**
         * @param {!WebInspector.PaintProfilerSnapshot=} snapshot
         * @this {WebInspector.LayerPaintProfilerView}
         */
        function onSnapshotDone(snapshot)
        {
            this._layer = layer;
            this._paintProfilerView.setSnapshot(snapshot || null);
            this._logTreeView.setSnapshot(snapshot || null);
        }
    },

    _onWindowChanged: function()
    {
        var window = this._paintProfilerView.windowBoundaries();
        this._logTreeView.updateWindow(window.left, window.right);
    },

    /**
     * @param {string=} imageURL
     */
    _showImage: function(imageURL)
    {
        this._showImageForLayerCallback(this._layer, imageURL);
    },

    __proto__: WebInspector.SplitView.prototype
};

