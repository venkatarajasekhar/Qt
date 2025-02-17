// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
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


// Tick Processor's code flow.

function processArguments(args) {
  var processor = new ArgumentsProcessor(args);
  if (processor.parse()) {
    return processor.result();
  } else {
    processor.printUsageAndExit();
  }
}

function initSourceMapSupport() {
  // Pull dev tools source maps  into our name space.
  SourceMap = WebInspector.SourceMap;

  // Overwrite the load function to load scripts synchronously.
  SourceMap.load = function(sourceMapURL) {
    var content = readFile(sourceMapURL);
    var sourceMapObject = (JSON.parse(content));
    return new SourceMap(sourceMapURL, sourceMapObject);
  };
}

var entriesProviders = {
  'unix': UnixCppEntriesProvider,
  'windows': WindowsCppEntriesProvider,
  'mac': MacCppEntriesProvider
};

var params = processArguments(arguments);
var sourceMap = null;
if (params.sourceMap) {
  initSourceMapSupport();
  sourceMap = SourceMap.load(params.sourceMap);
}
var snapshotLogProcessor;
if (params.snapshotLogFileName) {
  snapshotLogProcessor = new SnapshotLogProcessor();
  snapshotLogProcessor.processLogFile(params.snapshotLogFileName);
}
var tickProcessor = new TickProcessor(
  new (entriesProviders[params.platform])(params.nm, params.targetRootFS),
  params.separateIc,
  params.callGraphSize,
  params.ignoreUnknown,
  params.stateFilter,
  snapshotLogProcessor,
  params.distortion,
  params.range,
  sourceMap);
tickProcessor.processLogFile(params.logFileName);
tickProcessor.printStatistics();
