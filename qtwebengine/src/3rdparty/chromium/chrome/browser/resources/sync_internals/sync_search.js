// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// require: cr.js

cr.define('chrome.sync', function() {
  var currSearchId = 0;

  var setQueryString = function(queryControl, query) {
    queryControl.value = query;
  };

  var createDoQueryFunction = function(queryControl, submitControl, query) {
    return function() {
      setQueryString(queryControl, query);
      submitControl.click();
    };
  };

  /**
   * Decorates the quick search controls
   *
   * @param {Array of DOM elements} quickLinkArray The <a> object which
   *     will be given a link to a quick filter option.
   * @param {!HTMLInputElement} queryControl The <input> object of
   *     type=search where the user types in his query.
   */
  var decorateQuickQueryControls = function(quickLinkArray, submitControl,
                                            queryControl) {
    for (var index = 0; index < allLinks.length; ++index) {
      var quickQuery = allLinks[index].getAttribute('data-query');
      var quickQueryFunction = createDoQueryFunction(queryControl,
          submitControl, quickQuery);
      allLinks[index].addEventListener('click', quickQueryFunction);
    }
  };

  /**
   * Runs a search with the given query.
   *
   * @param {string} query The regex to do the search with.
   * @param {function} callback The callback called with the search results;
   *     not called if doSearch() is called again while the search is running.
   */
  var doSearch = function(query, callback) {
    var searchId = ++currSearchId;
    try {
      var regex = new RegExp(query);
      chrome.sync.getAllNodes(function(node_map) {
        // Put all nodes into one big list that ignores the type.
        var nodes = node_map.
            map(function(x) { return x.nodes; }).
            reduce(function(a, b) { return a.concat(b); });

        if (currSearchId != searchId) {
          return;
        }
        callback(nodes.filter(function(elem) {
          return regex.test(JSON.stringify(elem, null, 2));
        }), null);
      });
    } catch (err) {
      // Sometimes the provided regex is invalid.  This and other errors will
      // be caught and handled here.
      callback([], err);
    }
  };

  /**
   * Decorates the various search controls.
   *
   * @param {!HTMLInputElement} queryControl The <input> object of
   *     type=search where the user types in his query.
   * @param {!HTMLButtonElement} submitControl The <button> object
   *     where the user can click to do his query.
   * @param {!HTMLElement} statusControl The <span> object display the
   *     search status.
   * @param {!HTMLElement} listControl The <list> object which holds
   *     the list of returned results.
   * @param {!HTMLPreElement} detailsControl The <pre> object which
   *     holds the details of the selected result.
   */
  function decorateSearchControls(queryControl, submitControl, statusControl,
                                  resultsControl, detailsControl) {
    var resultsDataModel = new cr.ui.ArrayDataModel([]);

    var searchFunction = function() {
      var query = queryControl.value;
      statusControl.textContent = '';
      resultsDataModel.splice(0, resultsDataModel.length);
      if (!query) {
        return;
      }
      statusControl.textContent = 'Searching for ' + query + '...';
      queryControl.removeAttribute('error');
      var timer = chrome.sync.makeTimer();
      doSearch(query, function(nodes, error) {
        if (error) {
          statusControl.textContent = 'Error: ' + error;
          queryControl.setAttribute('error', '');
        } else {
          statusControl.textContent =
            'Found ' + nodes.length + ' nodes in ' +
            timer.getElapsedSeconds() + 's';
          queryControl.removeAttribute('error');

          // TODO(akalin): Write a nicer list display.
          for (var i = 0; i < nodes.length; ++i) {
            nodes[i].toString = function() {
              return this.NON_UNIQUE_NAME;
            };
          }
          resultsDataModel.push.apply(resultsDataModel, nodes);
          // Workaround for http://crbug.com/83452 .
          resultsControl.redraw();
        }
      });
    };

    submitControl.addEventListener('click', searchFunction);
    // Decorate search box.
    queryControl.onsearch = searchFunction;
    queryControl.value = '';

    // Decorate results list.
    cr.ui.List.decorate(resultsControl);
    resultsControl.dataModel = resultsDataModel;
    resultsControl.selectionModel.addEventListener('change', function(event) {
      detailsControl.textContent = '';
      var selected = resultsControl.selectedItem;
      if (selected) {
        detailsControl.textContent = JSON.stringify(selected, null, 2);
      }
    });
  }

  return {
    decorateSearchControls: decorateSearchControls,
    decorateQuickQueryControls: decorateQuickQueryControls
  };
});
