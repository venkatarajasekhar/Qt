// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Takes the |componentsData| input argument which represents data about the
 * currently installed components and populates the html jstemplate with
 * that data. It expects an object structure like the above.
 * @param {Object} componentsData Detailed info about installed components.
 *      Same expected format as returnComponentsData().
 */
function renderTemplate(componentsData) {
  // This is the javascript code that processes the template:
  var input = new JsEvalContext(componentsData);
  var output = $('component-template').cloneNode(true);
  $('component-placeholder').innerHTML = '';
  $('component-placeholder').appendChild(output);
  jstProcess(input, output);
  output.removeAttribute('hidden');
}

/**
 * Asks the C++ ComponentsDOMHandler to get details about the installed
 * components.
 * The ComponentsDOMHandler should reply to returnComponentsData() (below).
 */
function requestComponentsData() {
  chrome.send('requestComponentsData');
}

/**
 * Called by the WebUI to re-populate the page with data representing the
 * current state of installed components.
 * @param {Object} componentsData Detailed info about installed components. The
 *     template expects each component's format to match the following
 *     structure to correctly populate the page:
 *   {
 *     components: [
 *       {
 *          name: 'Component1',
 *          version: '1.2.3',
 *       },
 *       {
 *          name: 'Component2',
 *          version: '4.5.6',
 *       },
 *     ]
 *   }
 */
function returnComponentsData(componentsData) {
  var bodyContainer = $('body-container');
  var body = document.body;

  bodyContainer.style.visibility = 'hidden';
  body.className = '';

  renderTemplate(componentsData);

  // Add handlers to dynamically created HTML elements.
  var links = document.getElementsByClassName('button-check-update');
  for (var i = 0; i < links.length; i++) {
    links[i].onclick = function(e) {
      handleCheckUpdate(this);
      e.preventDefault();
    };
  }

  // Disable some controls for Guest in ChromeOS.
  if (cr.isChromeOS)
    uiAccountTweaks.UIAccountTweaks.applyGuestModeVisibility(document);

  bodyContainer.style.visibility = 'visible';
  body.className = 'show-tmi-mode-initial';
}

/**
 * This event function is called from component UI indicating changed state
 * of component updater service.
 * @param {Object} eventArgs Contains event and component ID. Component ID is
 * optional.
 */
function onComponentEvent(eventArgs) {
  if (eventArgs['id']) {
    var id = eventArgs['id'];
    $('status-' + id).textContent = eventArgs['event'];
  }
  if (eventArgs['version']) {
    $('version-' + id).textContent = eventArgs['version'];
  }
}

/**
 * Handles an 'enable' or 'disable' button getting clicked.
 * @param {HTMLElement} node The HTML element representing the component
 *     being checked for update.
 */
function handleCheckUpdate(node) {
  $('status-' + String(node.id)).textContent =
      loadTimeData.getString('checkingLabel');

  // Tell the C++ ComponentssDOMHandler to check for update.
  chrome.send('checkUpdate', [String(node.id)]);
}

// Get data and have it displayed upon loading.
document.addEventListener('DOMContentLoaded', requestComponentsData);
