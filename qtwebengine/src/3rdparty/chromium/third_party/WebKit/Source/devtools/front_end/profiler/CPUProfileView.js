/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * @extends {WebInspector.VBox}
 * @param {!WebInspector.CPUProfileHeader} profileHeader
 */
WebInspector.CPUProfileView = function(profileHeader)
{
    WebInspector.VBox.call(this);
    this.element.classList.add("cpu-profile-view");

    this._viewType = WebInspector.settings.createSetting("cpuProfilerView", WebInspector.CPUProfileView._TypeHeavy);

    var columns = [];
    columns.push({id: "self", title: WebInspector.UIString("Self"), width: "120px", sort: WebInspector.DataGrid.Order.Descending, sortable: true});
    columns.push({id: "total", title: WebInspector.UIString("Total"), width: "120px", sortable: true});
    columns.push({id: "function", title: WebInspector.UIString("Function"), disclosure: true, sortable: true});

    this.dataGrid = new WebInspector.DataGrid(columns);
    this.dataGrid.addEventListener(WebInspector.DataGrid.Events.SortingChanged, this._sortProfile, this);
    this.dataGrid.show(this.element);

    this.viewSelectComboBox = new WebInspector.StatusBarComboBox(this._changeView.bind(this));

    var options = {};
    options[WebInspector.CPUProfileView._TypeFlame] = this.viewSelectComboBox.createOption(WebInspector.UIString("Chart"), "", WebInspector.CPUProfileView._TypeFlame);
    options[WebInspector.CPUProfileView._TypeHeavy] = this.viewSelectComboBox.createOption(WebInspector.UIString("Heavy (Bottom Up)"), "", WebInspector.CPUProfileView._TypeHeavy);
    options[WebInspector.CPUProfileView._TypeTree] = this.viewSelectComboBox.createOption(WebInspector.UIString("Tree (Top Down)"), "", WebInspector.CPUProfileView._TypeTree);

    var optionName = this._viewType.get() || WebInspector.CPUProfileView._TypeFlame;
    var option = options[optionName] || options[WebInspector.CPUProfileView._TypeFlame];
    this.viewSelectComboBox.select(option);

    this._statusBarButtonsElement = document.createElement("span");

    this.focusButton = new WebInspector.StatusBarButton(WebInspector.UIString("Focus selected function."), "focus-profile-node-status-bar-item");
    this.focusButton.setEnabled(false);
    this.focusButton.addEventListener("click", this._focusClicked, this);
    this._statusBarButtonsElement.appendChild(this.focusButton.element);

    this.excludeButton = new WebInspector.StatusBarButton(WebInspector.UIString("Exclude selected function."), "exclude-profile-node-status-bar-item");
    this.excludeButton.setEnabled(false);
    this.excludeButton.addEventListener("click", this._excludeClicked, this);
    this._statusBarButtonsElement.appendChild(this.excludeButton.element);

    this.resetButton = new WebInspector.StatusBarButton(WebInspector.UIString("Restore all functions."), "reset-profile-status-bar-item");
    this.resetButton.visible = false;
    this.resetButton.addEventListener("click", this._resetClicked, this);
    this._statusBarButtonsElement.appendChild(this.resetButton.element);

    this._profileHeader = profileHeader;
    this._linkifier = new WebInspector.Linkifier(new WebInspector.Linkifier.DefaultFormatter(30));

    this.profile = new WebInspector.CPUProfileDataModel(profileHeader._profile || profileHeader.protocolProfile());

    this._changeView();
    if (this._flameChart)
        this._flameChart.update();
}

WebInspector.CPUProfileView._TypeFlame = "Flame";
WebInspector.CPUProfileView._TypeTree = "Tree";
WebInspector.CPUProfileView._TypeHeavy = "Heavy";

WebInspector.CPUProfileView.prototype = {
    /**
     * @param {!number} timeLeft
     * @param {!number} timeRight
     */
    selectRange: function(timeLeft, timeRight)
    {
        if (!this._flameChart)
            return;
        this._flameChart.selectRange(timeLeft, timeRight);
    },

    get statusBarItems()
    {
        return [this.viewSelectComboBox.element, this._statusBarButtonsElement];
    },

    /**
     * @return {!WebInspector.ProfileDataGridTree}
     */
    _getBottomUpProfileDataGridTree: function()
    {
        if (!this._bottomUpProfileDataGridTree)
            this._bottomUpProfileDataGridTree = new WebInspector.BottomUpProfileDataGridTree(this, /** @type {!ProfilerAgent.CPUProfileNode} */ (this.profile.profileHead));
        return this._bottomUpProfileDataGridTree;
    },

    /**
     * @return {!WebInspector.ProfileDataGridTree}
     */
    _getTopDownProfileDataGridTree: function()
    {
        if (!this._topDownProfileDataGridTree)
            this._topDownProfileDataGridTree = new WebInspector.TopDownProfileDataGridTree(this, /** @type {!ProfilerAgent.CPUProfileNode} */ (this.profile.profileHead));
        return this._topDownProfileDataGridTree;
    },

    willHide: function()
    {
        this._currentSearchResultIndex = -1;
    },

    refresh: function()
    {
        var selectedProfileNode = this.dataGrid.selectedNode ? this.dataGrid.selectedNode.profileNode : null;

        this.dataGrid.rootNode().removeChildren();

        var children = this.profileDataGridTree.children;
        var count = children.length;

        for (var index = 0; index < count; ++index)
            this.dataGrid.rootNode().appendChild(children[index]);

        if (selectedProfileNode)
            selectedProfileNode.selected = true;
    },

    refreshVisibleData: function()
    {
        var child = this.dataGrid.rootNode().children[0];
        while (child) {
            child.refresh();
            child = child.traverseNextNode(false, null, true);
        }
    },

    searchCanceled: function()
    {
        if (this._searchResults) {
            for (var i = 0; i < this._searchResults.length; ++i) {
                var profileNode = this._searchResults[i].profileNode;

                delete profileNode._searchMatchedSelfColumn;
                delete profileNode._searchMatchedTotalColumn;
                delete profileNode._searchMatchedFunctionColumn;

                profileNode.refresh();
            }
        }

        delete this._searchFinishedCallback;
        this._currentSearchResultIndex = -1;
        this._searchResults = [];
    },

    performSearch: function(query, finishedCallback)
    {
        // Call searchCanceled since it will reset everything we need before doing a new search.
        this.searchCanceled();

        query = query.trim();

        if (!query.length)
            return;

        this._searchFinishedCallback = finishedCallback;

        var greaterThan = (query.startsWith(">"));
        var lessThan = (query.startsWith("<"));
        var equalTo = (query.startsWith("=") || ((greaterThan || lessThan) && query.indexOf("=") === 1));
        var percentUnits = (query.lastIndexOf("%") === (query.length - 1));
        var millisecondsUnits = (query.length > 2 && query.lastIndexOf("ms") === (query.length - 2));
        var secondsUnits = (!millisecondsUnits && query.lastIndexOf("s") === (query.length - 1));

        var queryNumber = parseFloat(query);
        if (greaterThan || lessThan || equalTo) {
            if (equalTo && (greaterThan || lessThan))
                queryNumber = parseFloat(query.substring(2));
            else
                queryNumber = parseFloat(query.substring(1));
        }

        var queryNumberMilliseconds = (secondsUnits ? (queryNumber * 1000) : queryNumber);

        // Make equalTo implicitly true if it wasn't specified there is no other operator.
        if (!isNaN(queryNumber) && !(greaterThan || lessThan))
            equalTo = true;

        var matcher = createPlainTextSearchRegex(query, "i");

        function matchesQuery(/*ProfileDataGridNode*/ profileDataGridNode)
        {
            delete profileDataGridNode._searchMatchedSelfColumn;
            delete profileDataGridNode._searchMatchedTotalColumn;
            delete profileDataGridNode._searchMatchedFunctionColumn;

            if (percentUnits) {
                if (lessThan) {
                    if (profileDataGridNode.selfPercent < queryNumber)
                        profileDataGridNode._searchMatchedSelfColumn = true;
                    if (profileDataGridNode.totalPercent < queryNumber)
                        profileDataGridNode._searchMatchedTotalColumn = true;
                } else if (greaterThan) {
                    if (profileDataGridNode.selfPercent > queryNumber)
                        profileDataGridNode._searchMatchedSelfColumn = true;
                    if (profileDataGridNode.totalPercent > queryNumber)
                        profileDataGridNode._searchMatchedTotalColumn = true;
                }

                if (equalTo) {
                    if (profileDataGridNode.selfPercent == queryNumber)
                        profileDataGridNode._searchMatchedSelfColumn = true;
                    if (profileDataGridNode.totalPercent == queryNumber)
                        profileDataGridNode._searchMatchedTotalColumn = true;
                }
            } else if (millisecondsUnits || secondsUnits) {
                if (lessThan) {
                    if (profileDataGridNode.selfTime < queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedSelfColumn = true;
                    if (profileDataGridNode.totalTime < queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedTotalColumn = true;
                } else if (greaterThan) {
                    if (profileDataGridNode.selfTime > queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedSelfColumn = true;
                    if (profileDataGridNode.totalTime > queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedTotalColumn = true;
                }

                if (equalTo) {
                    if (profileDataGridNode.selfTime == queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedSelfColumn = true;
                    if (profileDataGridNode.totalTime == queryNumberMilliseconds)
                        profileDataGridNode._searchMatchedTotalColumn = true;
                }
            }

            if (profileDataGridNode.functionName.match(matcher) || (profileDataGridNode.url && profileDataGridNode.url.match(matcher)))
                profileDataGridNode._searchMatchedFunctionColumn = true;

            if (profileDataGridNode._searchMatchedSelfColumn ||
                profileDataGridNode._searchMatchedTotalColumn ||
                profileDataGridNode._searchMatchedFunctionColumn)
            {
                profileDataGridNode.refresh();
                return true;
            }

            return false;
        }

        var current = this.profileDataGridTree.children[0];

        while (current) {
            if (matchesQuery(current)) {
                this._searchResults.push({ profileNode: current });
            }

            current = current.traverseNextNode(false, null, false);
        }

        finishedCallback(this, this._searchResults.length);
    },

    jumpToFirstSearchResult: function()
    {
        if (!this._searchResults || !this._searchResults.length)
            return;
        this._currentSearchResultIndex = 0;
        this._jumpToSearchResult(this._currentSearchResultIndex);
    },

    jumpToLastSearchResult: function()
    {
        if (!this._searchResults || !this._searchResults.length)
            return;
        this._currentSearchResultIndex = (this._searchResults.length - 1);
        this._jumpToSearchResult(this._currentSearchResultIndex);
    },

    jumpToNextSearchResult: function()
    {
        if (!this._searchResults || !this._searchResults.length)
            return;
        if (++this._currentSearchResultIndex >= this._searchResults.length)
            this._currentSearchResultIndex = 0;
        this._jumpToSearchResult(this._currentSearchResultIndex);
    },

    jumpToPreviousSearchResult: function()
    {
        if (!this._searchResults || !this._searchResults.length)
            return;
        if (--this._currentSearchResultIndex < 0)
            this._currentSearchResultIndex = (this._searchResults.length - 1);
        this._jumpToSearchResult(this._currentSearchResultIndex);
    },

    /**
     * @return {boolean}
     */
    showingFirstSearchResult: function()
    {
        return (this._currentSearchResultIndex === 0);
    },

    /**
     * @return {boolean}
     */
    showingLastSearchResult: function()
    {
        return (this._searchResults && this._currentSearchResultIndex === (this._searchResults.length - 1));
    },

    /**
     * @return {number}
     */
    currentSearchResultIndex: function() {
        return this._currentSearchResultIndex;
    },

    _jumpToSearchResult: function(index)
    {
        var searchResult = this._searchResults[index];
        if (!searchResult)
            return;

        var profileNode = searchResult.profileNode;
        profileNode.revealAndSelect();
    },

    _ensureFlameChartCreated: function()
    {
        if (this._flameChart)
            return;
        this._dataProvider = new WebInspector.CPUFlameChartDataProvider(this.profile, this._profileHeader.target());
        this._flameChart = new WebInspector.CPUProfileFlameChart(this._dataProvider);
        this._flameChart.addEventListener(WebInspector.FlameChart.Events.EntrySelected, this._onEntrySelected.bind(this));
    },

    /**
     * @param {!WebInspector.Event} event
     */
    _onEntrySelected: function(event)
    {
        var entryIndex = event.data;
        var node = this._dataProvider._entryNodes[entryIndex];
        if (!node || !node.scriptId)
            return;
        var script = WebInspector.debuggerModel.scriptForId(node.scriptId)
        if (!script)
            return;
        WebInspector.Revealer.reveal(script.rawLocationToUILocation(node.lineNumber));
    },

    _changeView: function()
    {
        if (!this.profile)
            return;

        switch (this.viewSelectComboBox.selectedOption().value) {
        case WebInspector.CPUProfileView._TypeFlame:
            this._ensureFlameChartCreated();
            this.dataGrid.detach();
            this._flameChart.show(this.element);
            this._viewType.set(WebInspector.CPUProfileView._TypeFlame);
            this._statusBarButtonsElement.classList.toggle("hidden", true);
            return;
        case WebInspector.CPUProfileView._TypeTree:
            this.profileDataGridTree = this._getTopDownProfileDataGridTree();
            this._sortProfile();
            this._viewType.set(WebInspector.CPUProfileView._TypeTree);
            break;
        case WebInspector.CPUProfileView._TypeHeavy:
            this.profileDataGridTree = this._getBottomUpProfileDataGridTree();
            this._sortProfile();
            this._viewType.set(WebInspector.CPUProfileView._TypeHeavy);
            break;
        }

        this._statusBarButtonsElement.classList.toggle("hidden", false);

        if (this._flameChart)
            this._flameChart.detach();
        this.dataGrid.show(this.element);

        if (!this.currentQuery || !this._searchFinishedCallback || !this._searchResults)
            return;

        // The current search needs to be performed again. First negate out previous match
        // count by calling the search finished callback with a negative number of matches.
        // Then perform the search again the with same query and callback.
        this._searchFinishedCallback(this, -this._searchResults.length);
        this.performSearch(this.currentQuery, this._searchFinishedCallback);
    },

    _focusClicked: function(event)
    {
        if (!this.dataGrid.selectedNode)
            return;

        this.resetButton.visible = true;
        this.profileDataGridTree.focus(this.dataGrid.selectedNode);
        this.refresh();
        this.refreshVisibleData();
    },

    _excludeClicked: function(event)
    {
        var selectedNode = this.dataGrid.selectedNode

        if (!selectedNode)
            return;

        selectedNode.deselect();

        this.resetButton.visible = true;
        this.profileDataGridTree.exclude(selectedNode);
        this.refresh();
        this.refreshVisibleData();
    },

    _resetClicked: function(event)
    {
        this.resetButton.visible = false;
        this.profileDataGridTree.restore();
        this._linkifier.reset();
        this.refresh();
        this.refreshVisibleData();
    },

    _dataGridNodeSelected: function(node)
    {
        this.focusButton.setEnabled(true);
        this.excludeButton.setEnabled(true);
    },

    _dataGridNodeDeselected: function(node)
    {
        this.focusButton.setEnabled(false);
        this.excludeButton.setEnabled(false);
    },

    _sortProfile: function()
    {
        var sortAscending = this.dataGrid.isSortOrderAscending();
        var sortColumnIdentifier = this.dataGrid.sortColumnIdentifier();
        var sortProperty = {
                "self": "selfTime",
                "total": "totalTime",
                "function": "functionName"
            }[sortColumnIdentifier];

        this.profileDataGridTree.sort(WebInspector.ProfileDataGridTree.propertyComparator(sortProperty, sortAscending));

        this.refresh();
    },

    __proto__: WebInspector.VBox.prototype
}

/**
 * @constructor
 * @extends {WebInspector.ProfileType}
 * @implements {WebInspector.CPUProfilerModel.Delegate}
 */
WebInspector.CPUProfileType = function()
{
    WebInspector.ProfileType.call(this, WebInspector.CPUProfileType.TypeId, WebInspector.UIString("Collect JavaScript CPU Profile"));
    this._recording = false;

    this._nextAnonymousConsoleProfileNumber = 1;
    this._anonymousConsoleProfileIdToTitle = {};

    WebInspector.CPUProfileType.instance = this;
    WebInspector.cpuProfilerModel.setDelegate(this);
}

WebInspector.CPUProfileType.TypeId = "CPU";

WebInspector.CPUProfileType.prototype = {
    /**
     * @override
     * @return {string}
     */
    fileExtension: function()
    {
        return ".cpuprofile";
    },

    get buttonTooltip()
    {
        return this._recording ? WebInspector.UIString("Stop CPU profiling.") : WebInspector.UIString("Start CPU profiling.");
    },

    /**
     * @override
     * @return {boolean}
     */
    buttonClicked: function()
    {
        if (this._recording) {
            this.stopRecordingProfile();
            return false;
        } else {
            this.startRecordingProfile();
            return true;
        }
    },

    get treeItemTitle()
    {
        return WebInspector.UIString("CPU PROFILES");
    },

    get description()
    {
        return WebInspector.UIString("CPU profiles show where the execution time is spent in your page's JavaScript functions.");
    },

    /**
     * @param {string} id
     * @param {!WebInspector.DebuggerModel.Location} scriptLocation
     * @param {string=} title
     */
    consoleProfileStarted: function(id, scriptLocation, title)
    {
        var resolvedTitle = title;
        if (!resolvedTitle) {
            resolvedTitle = WebInspector.UIString("Profile %s", this._nextAnonymousConsoleProfileNumber++);
            this._anonymousConsoleProfileIdToTitle[id] = resolvedTitle;
        }
        this._addMessageToConsole(WebInspector.ConsoleMessage.MessageType.Profile, scriptLocation, WebInspector.UIString("Profile '%s' started.", resolvedTitle));
    },

    /**
     * @param {string} protocolId
     * @param {!WebInspector.DebuggerModel.Location} scriptLocation
     * @param {!ProfilerAgent.CPUProfile} cpuProfile
     * @param {string=} title
     */
    consoleProfileFinished: function(protocolId, scriptLocation, cpuProfile, title)
    {
        var resolvedTitle = title;
        if (typeof title === "undefined") {
            resolvedTitle = this._anonymousConsoleProfileIdToTitle[protocolId];
            delete this._anonymousConsoleProfileIdToTitle[protocolId];
        }

        var target = /** @type {!WebInspector.Target} */ (WebInspector.targetManager.activeTarget());
        var profile = new WebInspector.CPUProfileHeader(target, this, resolvedTitle);
        profile.setProtocolProfile(cpuProfile);
        this.addProfile(profile);
        this._addMessageToConsole(WebInspector.ConsoleMessage.MessageType.ProfileEnd, scriptLocation, WebInspector.UIString("Profile '%s' finished.", resolvedTitle));
    },

    /**
     * @param {string} type
     * @param {!WebInspector.DebuggerModel.Location} scriptLocation
     * @param {string} messageText
     */
    _addMessageToConsole: function(type, scriptLocation, messageText)
    {
        var script = scriptLocation.script();
        var message = new WebInspector.ConsoleMessage(
            WebInspector.console.target(),
            WebInspector.ConsoleMessage.MessageSource.ConsoleAPI,
            WebInspector.ConsoleMessage.MessageLevel.Debug,
            messageText,
            type,
            undefined,
            undefined,
            undefined,
            undefined,
            undefined,
            [{
                functionName: "",
                scriptId: scriptLocation.scriptId,
                url: script ? script.contentURL() : "",
                lineNumber: scriptLocation.lineNumber,
                columnNumber: scriptLocation.columnNumber || 0
            }]);

        WebInspector.console.addMessage(message);
    },

    /**
     * @return {boolean}
     */
    isRecordingProfile: function()
    {
        return this._recording;
    },

    startRecordingProfile: function()
    {
        if (this._profileBeingRecorded)
            return;
        var target = /** @type {!WebInspector.Target} */ (WebInspector.targetManager.activeTarget());
        var profile = new WebInspector.CPUProfileHeader(target, this);
        this.setProfileBeingRecorded(profile);
        this.addProfile(profile);
        profile.updateStatus(WebInspector.UIString("Recording\u2026"));
        this._recording = true;
        WebInspector.cpuProfilerModel.setRecording(true);
        WebInspector.userMetrics.ProfilesCPUProfileTaken.record();
        ProfilerAgent.start();
    },

    stopRecordingProfile: function()
    {
        this._recording = false;
        WebInspector.cpuProfilerModel.setRecording(false);

        /**
         * @param {?string} error
         * @param {?ProfilerAgent.CPUProfile} profile
         * @this {WebInspector.CPUProfileType}
         */
        function didStopProfiling(error, profile)
        {
            if (!this._profileBeingRecorded)
                return;
            this._profileBeingRecorded.setProtocolProfile(profile);
            this._profileBeingRecorded.updateStatus("");
            var recordedProfile = this._profileBeingRecorded;
            this.setProfileBeingRecorded(null);
            this.dispatchEventToListeners(WebInspector.ProfileType.Events.ProfileComplete, recordedProfile);
        }
        ProfilerAgent.stop(didStopProfiling.bind(this));
    },

    /**
     * @override
     * @param {string} title
     * @return {!WebInspector.ProfileHeader}
     */
    createProfileLoadedFromFile: function(title)
    {
        var target = /** @type {!WebInspector.Target} */ (WebInspector.targetManager.activeTarget());
        return new WebInspector.CPUProfileHeader(target, this, title);
    },

    /**
     * @override
     */
    profileBeingRecordedRemoved: function()
    {
        this.stopRecordingProfile();
    },

    __proto__: WebInspector.ProfileType.prototype
}

/**
 * @constructor
 * @extends {WebInspector.ProfileHeader}
 * @implements {WebInspector.OutputStream}
 * @implements {WebInspector.OutputStreamDelegate}
 * @param {!WebInspector.Target} target
 * @param {!WebInspector.CPUProfileType} type
 * @param {string=} title
 */
WebInspector.CPUProfileHeader = function(target, type, title)
{
    WebInspector.ProfileHeader.call(this, target, type, title || WebInspector.UIString("Profile %d", type._nextProfileUid));
    this._tempFile = null;
}

WebInspector.CPUProfileHeader.prototype = {
    onTransferStarted: function()
    {
        this._jsonifiedProfile = "";
        this.updateStatus(WebInspector.UIString("Loading\u2026 %s", Number.bytesToString(this._jsonifiedProfile.length)), true);
    },

    /**
     * @param {!WebInspector.ChunkedReader} reader
     */
    onChunkTransferred: function(reader)
    {
        this.updateStatus(WebInspector.UIString("Loading\u2026 %d\%", Number.bytesToString(this._jsonifiedProfile.length)));
    },

    onTransferFinished: function()
    {
        this.updateStatus(WebInspector.UIString("Parsing\u2026"), true);
        this._profile = JSON.parse(this._jsonifiedProfile);
        this._jsonifiedProfile = null;
        this.updateStatus(WebInspector.UIString("Loaded"), false);

        if (this._profileType.profileBeingRecorded() === this)
            this._profileType.setProfileBeingRecorded(null);
    },

    /**
     * @param {!WebInspector.ChunkedReader} reader
     * @param {?Event} e
     */
    onError: function(reader, e)
    {
        var subtitle;
        switch(e.target.error.code) {
        case e.target.error.NOT_FOUND_ERR:
            subtitle = WebInspector.UIString("'%s' not found.", reader.fileName());
            break;
        case e.target.error.NOT_READABLE_ERR:
            subtitle = WebInspector.UIString("'%s' is not readable", reader.fileName());
            break;
        case e.target.error.ABORT_ERR:
            return;
        default:
            subtitle = WebInspector.UIString("'%s' error %d", reader.fileName(), e.target.error.code);
        }
        this.updateStatus(subtitle);
    },

    /**
     * @param {string} text
     */
    write: function(text)
    {
        this._jsonifiedProfile += text;
    },

    close: function() { },

    /**
     * @override
     */
    dispose: function()
    {
        this.removeTempFile();
    },

    /**
     * @override
     * @param {!WebInspector.ProfilesPanel} panel
     * @return {!WebInspector.ProfileSidebarTreeElement}
     */
    createSidebarTreeElement: function(panel)
    {
        return new WebInspector.ProfileSidebarTreeElement(panel, this, "profile-sidebar-tree-item");
    },

    /**
     * @override
     * @return {!WebInspector.CPUProfileView}
     */
    createView: function()
    {
        return new WebInspector.CPUProfileView(this);
    },

    /**
     * @override
     * @return {boolean}
     */
    canSaveToFile: function()
    {
        return !this.fromFile() && this._protocolProfile;
    },

    saveToFile: function()
    {
        var fileOutputStream = new WebInspector.FileOutputStream();

        /**
         * @param {boolean} accepted
         * @this {WebInspector.CPUProfileHeader}
         */
        function onOpenForSave(accepted)
        {
            if (!accepted)
                return;
            function didRead(data)
            {
                if (data)
                    fileOutputStream.write(data, fileOutputStream.close.bind(fileOutputStream));
                else
                    fileOutputStream.close();
            }
            if (this._failedToCreateTempFile) {
                WebInspector.messageSink.addErrorMessage("Failed to open temp file with heap snapshot");
                fileOutputStream.close();
            } else if (this._tempFile) {
                this._tempFile.read(didRead);
            } else {
                this._onTempFileReady = onOpenForSave.bind(this, accepted);
            }
        }
        this._fileName = this._fileName || "CPU-" + new Date().toISO8601Compact() + this._profileType.fileExtension();
        fileOutputStream.open(this._fileName, onOpenForSave.bind(this));
    },

    /**
     * @param {!File} file
     */
    loadFromFile: function(file)
    {
        this.updateStatus(WebInspector.UIString("Loading\u2026"), true);
        var fileReader = new WebInspector.ChunkedFileReader(file, 10000000, this);
        fileReader.start(this);
    },


    /**
     * @return {?ProfilerAgent.CPUProfile}
     */
    protocolProfile: function()
    {
        return this._protocolProfile;
    },

    /**
     * @param {!ProfilerAgent.CPUProfile} cpuProfile
     */
    setProtocolProfile: function(cpuProfile)
    {
        this._protocolProfile = cpuProfile;
        this._saveProfileDataToTempFile(cpuProfile);
        if (this.canSaveToFile())
            this.dispatchEventToListeners(WebInspector.ProfileHeader.Events.ProfileReceived);
    },

    /**
     * @param {!ProfilerAgent.CPUProfile} data
     */
    _saveProfileDataToTempFile: function(data)
    {
        var serializedData = JSON.stringify(data);

        /**
         * @this {WebInspector.CPUProfileHeader}
         */
        function didCreateTempFile(tempFile)
        {
            this._writeToTempFile(tempFile, serializedData);
        }
        new WebInspector.TempFile("cpu-profiler", this.uid,  didCreateTempFile.bind(this));
    },

    /**
     * @param {?WebInspector.TempFile} tempFile
     * @param {string} serializedData
     */
    _writeToTempFile: function(tempFile, serializedData)
    {
        this._tempFile = tempFile;
        if (!tempFile) {
            this._failedToCreateTempFile = true;
            this._notifyTempFileReady();
            return;
        }
        /**
         * @param {boolean} success
         * @this {WebInspector.CPUProfileHeader}
         */
        function didWriteToTempFile(success)
        {
            if (!success)
                this._failedToCreateTempFile = true;
            tempFile.finishWriting();
            this._notifyTempFileReady();
        }
        tempFile.write(serializedData, didWriteToTempFile.bind(this));
    },

    _notifyTempFileReady: function()
    {
        if (this._onTempFileReady) {
            this._onTempFileReady();
            this._onTempFileReady = null;
        }
    },

    __proto__: WebInspector.ProfileHeader.prototype
}
