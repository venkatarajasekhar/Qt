// Copyright (C) 2012 Google Inc. All rights reserved.
// Copyright (C) 2012 Zan Dobersek <zandobersek@gmail.com>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//         * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//         * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//         * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

var loader = loader || {};

(function() {

var TEST_RESULTS_SERVER = 'http://test-results.appspot.com/';

function pathToBuilderResultsFile(builderName) {
    return TEST_RESULTS_SERVER + 'testfile?builder=' + builderName +
           '&master=' + builders.master(builderName).name +
           '&testtype=' + g_history.crossDashboardState.testType + '&name=';
}

loader.request = function(url, success, error, opt_isBinaryData)
{
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    if (opt_isBinaryData)
        xhr.overrideMimeType('text/plain; charset=x-user-defined');
    xhr.onreadystatechange = function(e) {
        if (xhr.readyState == 4) {
            if (xhr.status == 200)
                success(xhr);
            else
                error(xhr);
        }
    }
    xhr.send();
}

loader.Loader = function()
{
    this._loadingSteps = [
        this._loadBuildersList,
        this._loadResultsFiles,
    ];

    this._buildersThatFailedToLoad = [];
    this._staleBuilders = [];
    this._errors = new ui.Errors();
    // TODO(jparent): Pass in the appropriate history obj per db.
    this._history = g_history;
}

// TODO(aboxhall): figure out whether this is a performance bottleneck and
// change calling code to understand the trie structure instead if necessary.
loader.Loader._flattenTrie = function(trie, prefix)
{
    var result = {};
    for (var name in trie) {
        var fullName = prefix ? prefix + "/" + name : name;
        var data = trie[name];
        if ("results" in data)
            result[fullName] = data;
        else {
            var partialResult = loader.Loader._flattenTrie(data, fullName);
            for (var key in partialResult) {
                result[key] = partialResult[key];
            }
        }
    }
    return result;
}

loader.Loader.prototype = {
    load: function()
    {
        this._loadNext();
    },
    showErrors: function()
    {
        this._errors.show();
    },
    buildersThatFailedToLoad: function() {
        return this._buildersThatFailedToLoad;
    },
    staleBuilders: function() {
        return this._staleBuilders;
    },
    _loadNext: function()
    {
        var loadingStep = this._loadingSteps.shift();
        if (!loadingStep) {
            this._addErrors();
            this._history.initialize();
            return;
        }
        loadingStep.apply(this);
    },
    _loadBuildersList: function()
    {
        builders.loadBuildersList(currentBuilderGroupName(), this._history.crossDashboardState.testType);
        this._loadNext();
    },
    _loadResultsFiles: function()
    {
        var builderNames = Object.keys(currentBuilders());
        if (builderNames.length)
            builderNames.forEach(this._loadResultsFileForBuilder.bind(this));
        else
            this._loadNext();

    },
    _loadResultsFileForBuilder: function(builderName)
    {
        var resultsFilename;
        if (history.isTreeMap())
            resultsFilename = 'times_ms.json';
        else if (this._history.crossDashboardState.showAllRuns)
            resultsFilename = 'results.json';
        else
            resultsFilename = 'results-small.json';

        var resultsFileLocation = pathToBuilderResultsFile(builderName) + resultsFilename;
        loader.request(resultsFileLocation,
                partial(function(loader, builderName, xhr) {
                    loader._handleResultsFileLoaded(builderName, xhr.responseText);
                }, this, builderName),
                partial(function(loader, builderName, xhr) {
                    loader._handleResultsFileLoadError(builderName);
                }, this, builderName));
    },
    _handleResultsFileLoaded: function(builderName, fileData)
    {
        if (history.isTreeMap())
            this._processTimesJSONData(builderName, fileData);
        else
            this._processResultsJSONData(builderName, fileData);

        // We need this work-around for webkit.org/b/50589.
        if (!g_resultsByBuilder[builderName]) {
            this._handleResultsFileLoadError(builderName);
            return;
        }

        this._handleResourceLoad();
    },
    _processTimesJSONData: function(builderName, fileData)
    {
        // FIXME: We should probably include the builderName in the JSON
        // rather than relying on only loading one JSON file per page.
        g_resultsByBuilder[builderName] = JSON.parse(fileData);
    },
    _processResultsJSONData: function(builderName, fileData)
    {
        var builds = JSON.parse(fileData);

        if (builderName == 'version' || builderName == 'failure_map')
             return;

        var ONE_DAY_SECONDS = 60 * 60 * 24;
        var ONE_WEEK_SECONDS = ONE_DAY_SECONDS * 7;

        // If a test suite stops being run on a given builder, we don't want to show it.
        // Assume any builder without a run in two weeks for a given test suite isn't
        // running that suite anymore.
        // FIXME: Grab which bots run which tests directly from the buildbot JSON instead.
        var lastRunSeconds = builds[builderName].secondsSinceEpoch[0];
        if ((Date.now() / 1000) - lastRunSeconds > ONE_WEEK_SECONDS)
            return;

        if ((Date.now() / 1000) - lastRunSeconds > ONE_DAY_SECONDS)
            this._staleBuilders.push(builderName);

        builds[builderName][results.TESTS] = loader.Loader._flattenTrie(builds[builderName][results.TESTS]);
        g_resultsByBuilder[builderName] = builds[builderName];
    },
    _handleResultsFileLoadError: function(builderName)
    {
        // FIXME: loader shouldn't depend on state defined in dashboard_base.js.
        this._buildersThatFailedToLoad.push(builderName);

        // Remove this builder from builders, so we don't try to use the
        // data that isn't there.
        delete currentBuilders()[builderName];

        // Proceed as if the resource had loaded.
        this._handleResourceLoad();
    },
    _handleResourceLoad: function()
    {
        if (this._haveResultsFilesLoaded())
            this._loadNext();
    },
    _haveResultsFilesLoaded: function()
    {
        for (var builderName in currentBuilders()) {
            if (!g_resultsByBuilder[builderName] && this._buildersThatFailedToLoad.indexOf(builderName) < 0)
                return false;
        }
        return true;
    },
    _addErrors: function()
    {
        if (this._buildersThatFailedToLoad.length)
            this._errors.addError('ERROR: Failed to get data from ' + this._buildersThatFailedToLoad.toString() +'.');

        if (this._staleBuilders.length)
            this._errors.addError('ERROR: Data from ' + this._staleBuilders.toString() + ' is more than 1 day stale.');
    }
}

})();
