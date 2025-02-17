/*
 * Copyright (C) 2009 280 North Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.DataGridNode}
 * @param {!ProfilerAgent.CPUProfileNode} profileNode
 * @param {!WebInspector.TopDownProfileDataGridTree} owningTree
 * @param {boolean} hasChildren
 */
WebInspector.ProfileDataGridNode = function(profileNode, owningTree, hasChildren)
{
    this._target = /** @type {!WebInspector.Target} */ (WebInspector.targetManager.activeTarget());
    this.profileNode = profileNode;

    WebInspector.DataGridNode.call(this, null, hasChildren);

    this.tree = owningTree;

    this.childrenByCallUID = {};
    this.lastComparator = null;

    this.callUID = profileNode.callUID;
    this.selfTime = profileNode.selfTime;
    this.totalTime = profileNode.totalTime;
    this.functionName = profileNode.functionName;
    this._deoptReason = (!profileNode.deoptReason || profileNode.deoptReason === "no reason") ? "" : profileNode.deoptReason;
    this.url = profileNode.url;
}

WebInspector.ProfileDataGridNode.prototype = {
    /**
     * @override
     * @param {string} columnIdentifier
     * @return {!Element}
     */
    createCell: function(columnIdentifier)
    {
        var cell = this._createValueCell(columnIdentifier) || WebInspector.DataGridNode.prototype.createCell.call(this, columnIdentifier);

        if (columnIdentifier === "self" && this._searchMatchedSelfColumn)
            cell.classList.add("highlight");
        else if (columnIdentifier === "total" && this._searchMatchedTotalColumn)
            cell.classList.add("highlight");

        if (columnIdentifier !== "function")
            return cell;

        if (this._deoptReason)
            cell.classList.add("not-optimized");

        if (this.profileNode._searchMatchedFunctionColumn)
            cell.classList.add("highlight");

        if (this.profileNode.scriptId !== "0") {
            var lineNumber = this.profileNode.lineNumber ? this.profileNode.lineNumber - 1 : 0;
            var columnNumber = this.profileNode.columnNumber ? this.profileNode.columnNumber - 1 : 0;
            var location = new WebInspector.DebuggerModel.Location(/** @type {!WebInspector.Target} */ (WebInspector.targetManager.activeTarget()), this.profileNode.scriptId, lineNumber, columnNumber);
            var urlElement = this.tree.profileView._linkifier.linkifyRawLocation(location, "profile-node-file");
            if (!urlElement)
                urlElement = this.tree.profileView._linkifier.linkifyLocation(this._target, this.profileNode.url, lineNumber, columnNumber, "profile-node-file");
            urlElement.style.maxWidth = "75%";
            cell.insertBefore(urlElement, cell.firstChild);
        }

        return cell;
    },

    /**
     * @param {string} columnIdentifier
     * @return {?Element}
     */
    _createValueCell: function(columnIdentifier)
    {
        if (columnIdentifier !== "self" && columnIdentifier !== "total")
            return null;

        var cell = document.createElement("td");
        cell.className = "numeric-column";
        var div = document.createElement("div");
        var valueSpan = document.createElement("span");
        valueSpan.textContent = this.data[columnIdentifier];
        div.appendChild(valueSpan);
        var percentColumn = columnIdentifier + "-percent";
        if (percentColumn in this.data) {
            var percentSpan = document.createElement("span");
            percentSpan.className = "percent-column";
            percentSpan.textContent = this.data[percentColumn];
            div.appendChild(percentSpan);
            div.classList.add("profile-multiple-values");
        }
        cell.appendChild(div);
        return cell;
    },

    buildData: function()
    {
        function formatMilliseconds(time)
        {
            return WebInspector.UIString("%.1f\u2009ms", time);
        }
        function formatPercent(value)
        {
            return WebInspector.UIString("%.2f\u2009%", value);
        }

        var functionName;
        if (this._deoptReason) {
            var content = document.createDocumentFragment();
            var marker = content.createChild("span", "profile-warn-marker");
            marker.title = WebInspector.UIString("Not optimized: %s", this._deoptReason);
            content.createTextChild(this.functionName);
            functionName = content;
        } else {
            functionName = this.functionName;
        }

        this.data = {
            "function": functionName,
            "self-percent": formatPercent(this.selfPercent),
            "self": formatMilliseconds(this.selfTime),
            "total-percent": formatPercent(this.totalPercent),
            "total": formatMilliseconds(this.totalTime),
        };
    },

    select: function(supressSelectedEvent)
    {
        WebInspector.DataGridNode.prototype.select.call(this, supressSelectedEvent);
        this.tree.profileView._dataGridNodeSelected(this);
    },

    deselect: function(supressDeselectedEvent)
    {
        WebInspector.DataGridNode.prototype.deselect.call(this, supressDeselectedEvent);
        this.tree.profileView._dataGridNodeDeselected(this);
    },

    /**
     * @param {function(!T, !T)} comparator
     * @param {boolean} force
     * @template T
     */
    sort: function(comparator, force)
    {
        var gridNodeGroups = [[this]];

        for (var gridNodeGroupIndex = 0; gridNodeGroupIndex < gridNodeGroups.length; ++gridNodeGroupIndex) {
            var gridNodes = gridNodeGroups[gridNodeGroupIndex];
            var count = gridNodes.length;

            for (var index = 0; index < count; ++index) {
                var gridNode = gridNodes[index];

                // If the grid node is collapsed, then don't sort children (save operation for later).
                // If the grid node has the same sorting as previously, then there is no point in sorting it again.
                if (!force && (!gridNode.expanded || gridNode.lastComparator === comparator)) {
                    if (gridNode.children.length)
                        gridNode.shouldRefreshChildren = true;
                    continue;
                }

                gridNode.lastComparator = comparator;

                var children = gridNode.children;
                var childCount = children.length;

                if (childCount) {
                    children.sort(comparator);

                    for (var childIndex = 0; childIndex < childCount; ++childIndex)
                        children[childIndex]._recalculateSiblings(childIndex);

                    gridNodeGroups.push(children);
                }
            }
        }
    },

    /**
     * @param {!WebInspector.ProfileDataGridNode} profileDataGridNode
     * @param {number} index
     */
    insertChild: function(profileDataGridNode, index)
    {
        WebInspector.DataGridNode.prototype.insertChild.call(this, profileDataGridNode, index);

        this.childrenByCallUID[profileDataGridNode.callUID] = profileDataGridNode;
    },

    /**
     * @param {!WebInspector.ProfileDataGridNode} profileDataGridNode
     */
    removeChild: function(profileDataGridNode)
    {
        WebInspector.DataGridNode.prototype.removeChild.call(this, profileDataGridNode);

        delete this.childrenByCallUID[profileDataGridNode.callUID];
    },

    removeChildren: function()
    {
        WebInspector.DataGridNode.prototype.removeChildren.call(this);

        this.childrenByCallUID = {};
    },

    /**
     * @param {!WebInspector.ProfileDataGridNode} node
     * @return {?WebInspector.ProfileDataGridNode}
     */
    findChild: function(node)
    {
        if (!node)
            return null;
        return this.childrenByCallUID[node.callUID];
    },

    get selfPercent()
    {
        return this.selfTime / this.tree.totalTime * 100.0;
    },

    get totalPercent()
    {
        return this.totalTime / this.tree.totalTime * 100.0;
    },

    get _parent()
    {
        return this.parent !== this.dataGrid ? this.parent : this.tree;
    },

    populate: function()
    {
        if (this._populated)
            return;
        this._populated = true;

        this._sharedPopulate();

        var currentComparator = this.tree.lastComparator;

        if (currentComparator)
            this.sort(currentComparator, true);
    },

    // When focusing and collapsing we modify lots of nodes in the tree.
    // This allows us to restore them all to their original state when we revert.
    _save: function()
    {
        if (this._savedChildren)
            return;

        this._savedSelfTime = this.selfTime;
        this._savedTotalTime = this.totalTime;

        this._savedChildren = this.children.slice();
    },

    // When focusing and collapsing we modify lots of nodes in the tree.
    // This allows us to restore them all to their original state when we revert.
    _restore: function()
    {
        if (!this._savedChildren)
            return;

        this.selfTime = this._savedSelfTime;
        this.totalTime = this._savedTotalTime;

        this.removeChildren();

        var children = this._savedChildren;
        var count = children.length;

        for (var index = 0; index < count; ++index) {
            children[index]._restore();
            this.appendChild(children[index]);
        }
    },

    _merge: function(child, shouldAbsorb)
    {
        this.selfTime += child.selfTime;

        if (!shouldAbsorb)
            this.totalTime += child.totalTime;

        var children = this.children.slice();

        this.removeChildren();

        var count = children.length;

        for (var index = 0; index < count; ++index) {
            if (!shouldAbsorb || children[index] !== child)
                this.appendChild(children[index]);
        }

        children = child.children.slice();
        count = children.length;

        for (var index = 0; index < count; ++index) {
            var orphanedChild = children[index],
                existingChild = this.childrenByCallUID[orphanedChild.callUID];

            if (existingChild)
                existingChild._merge(orphanedChild, false);
            else
                this.appendChild(orphanedChild);
        }
    },

    __proto__: WebInspector.DataGridNode.prototype
}

/**
 * @constructor
 * @param {!WebInspector.CPUProfileView} profileView
 * @param {!ProfilerAgent.CPUProfileNode} rootProfileNode
 */
WebInspector.ProfileDataGridTree = function(profileView, rootProfileNode)
{
    this.tree = this;
    this.children = [];

    this.profileView = profileView;

    this.totalTime = rootProfileNode.totalTime;
    this.lastComparator = null;

    this.childrenByCallUID = {};
}

WebInspector.ProfileDataGridTree.prototype = {
    get expanded()
    {
        return true;
    },

    appendChild: function(child)
    {
        this.insertChild(child, this.children.length);
    },

    insertChild: function(child, index)
    {
        this.children.splice(index, 0, child);
        this.childrenByCallUID[child.callUID] = child;
    },

    removeChildren: function()
    {
        this.children = [];
        this.childrenByCallUID = {};
    },

    findChild: WebInspector.ProfileDataGridNode.prototype.findChild,
    sort: WebInspector.ProfileDataGridNode.prototype.sort,

    _save: function()
    {
        if (this._savedChildren)
            return;

        this._savedTotalTime = this.totalTime;
        this._savedChildren = this.children.slice();
    },

    restore: function()
    {
        if (!this._savedChildren)
            return;

        this.children = this._savedChildren;
        this.totalTime = this._savedTotalTime;

        var children = this.children;
        var count = children.length;

        for (var index = 0; index < count; ++index)
            children[index]._restore();

        this._savedChildren = null;
    }
}

WebInspector.ProfileDataGridTree.propertyComparators = [{}, {}];

/**
 * @param {string} property
 * @param {boolean} isAscending
 * @return {function(!Object.<string, *>, !Object.<string, *>)}
 */
WebInspector.ProfileDataGridTree.propertyComparator = function(property, isAscending)
{
    var comparator = WebInspector.ProfileDataGridTree.propertyComparators[(isAscending ? 1 : 0)][property];

    if (!comparator) {
        if (isAscending) {
            comparator = function(lhs, rhs)
            {
                if (lhs[property] < rhs[property])
                    return -1;

                if (lhs[property] > rhs[property])
                    return 1;

                return 0;
            }
        } else {
            comparator = function(lhs, rhs)
            {
                if (lhs[property] > rhs[property])
                    return -1;

                if (lhs[property] < rhs[property])
                    return 1;

                return 0;
            }
        }

        WebInspector.ProfileDataGridTree.propertyComparators[(isAscending ? 1 : 0)][property] = comparator;
    }

    return comparator;
}
