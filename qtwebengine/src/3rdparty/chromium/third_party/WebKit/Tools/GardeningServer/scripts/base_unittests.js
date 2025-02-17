/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

(function () {

module("base");

test("joinPath", 1, function() {
    var value = base.joinPath("path/to", "test.html");
    equals(value, "path/to/test.html");
});

test("endsWith", 9, function() {
    ok(base.endsWith("xyz", ""));
    ok(base.endsWith("xyz", "z"));
    ok(base.endsWith("xyz", "yz"));
    ok(base.endsWith("xyz", "xyz"));
    ok(!base.endsWith("xyz", "wxyz"));
    ok(!base.endsWith("xyz", "gwxyz"));
    ok(base.endsWith("", ""));
    ok(!base.endsWith("", "z"));
    ok(!base.endsWith("xyxy", "yx"));
});

test("trimExtension", 6, function() {
    equals(base.trimExtension("xyz"), "xyz");
    equals(base.trimExtension("xy.z"), "xy");
    equals(base.trimExtension("x.yz"), "x");
    equals(base.trimExtension("x.y.z"), "x.y");
    equals(base.trimExtension(".xyz"), "");
    equals(base.trimExtension(""), "");
});

test("joinPath with empty parent", 1, function() {
    var value = base.joinPath("", "test.html");
    equals(value, "test.html");
});

test("dirName", 3, function() {
    equals(base.dirName("foo.html"), "foo.html");
    equals(base.dirName("foo/bar.html"), "foo");
    equals(base.dirName("foo/bar/baz.html"), "foo/bar");
});

test("uniquifyArray", 5, function() {
    deepEqual(base.uniquifyArray([]), []);
    deepEqual(base.uniquifyArray(["a"]), ["a"]);
    deepEqual(base.uniquifyArray(["a", "b"]), ["a", "b"]);
    deepEqual(base.uniquifyArray(["a", "b", "b"]), ["a", "b"]);
    deepEqual(base.uniquifyArray(["a", "b", "b", "a"]), ["a", "b"]);
});

test("flattenArray", 5, function() {
    deepEqual(base.flattenArray([]), []);
    deepEqual(base.flattenArray([["a"]]), ["a"]);
    deepEqual(base.flattenArray([["a"], ["b"]]), ["a", "b"]);
    deepEqual(base.flattenArray([["a"], ["b", "c"]]), ["a", "b", "c"]);
    deepEqual(base.flattenArray([["a"], [], ["b"]]), ["a", "b"]);
});

test("filterDictionary", 3, function() {
    var dictionary = {
        'foo': 43,
        'bar': 11
    };
    deepEqual(base.filterDictionary(dictionary, function() { return true; }), {
        "foo": 43,
        "bar": 11
    });
    deepEqual(base.filterDictionary(dictionary, function() { return false; }), { });
    deepEqual(base.filterDictionary(dictionary, function(key) { return key == 'foo'; }), {
        "foo": 43
    });
});

test("mapDictionary", 3, function() {
    deepEqual(base.mapDictionary({}, function(value) { return value - 10; }), {});
    var dictionary = {
        'foo': 43,
        'bar': 11
    };
    deepEqual(base.mapDictionary(dictionary, function(value) { return value - 10; }), {
        "foo": 33,
        "bar": 1
    });
    deepEqual(base.mapDictionary(dictionary, function(value) {
        if (value > 20)
            return value - 20;
    }), {
        "foo": 23,
    });
});

test("filterTree", 2, function() {
    var tree = {
        'path': {
            'to': {
                'test.html': {
                    'actual': 'PASS',
                    'expected': 'FAIL'
                }
            },
            'another.html': {
                'actual': 'TEXT',
                'expected': 'PASS'
            }
        }
    }

    function isLeaf(node)
    {
        return !!node.actual;
    }

    function actualIsText(node)
    {
        return node.actual == 'TEXT';
    }

    var all = base.filterTree(tree, isLeaf, function() { return true });
    deepEqual(all, {
        'path/to/test.html': {
            'actual': 'PASS',
            'expected': 'FAIL'
        },
        'path/another.html': {
            'actual': 'TEXT',
            'expected': 'PASS'
        }
    });

    var text = base.filterTree(tree, isLeaf, actualIsText);
    deepEqual(text, {
        'path/another.html': {
            'actual': 'TEXT',
            'expected': 'PASS'
        }
    });
});

test("UpdateTracker", 20, function() {
    var dict;

    function dumpKeys()
    {
        var updates = []
        dict.forEach(function(item, key, updated) {
            updates.push(key);
        });
        return updates;
    }

    function dumpUpdatedKeys()
    {
        var updates = []
        dict.forEach(function(item, key, updated) {
            updated && updates.push(key);
        });
        return updates;
    }


    dict = new base.UpdateTracker();
    dict.update("5", {});
    deepEqual(dumpUpdatedKeys(), ["5"]);
    dict.update("6", {});
    dict.update("7", {});
    deepEqual(dumpUpdatedKeys(), ["5", "6", "7"]);
    deepEqual(dict.get("6"), {});
    ok(dict.exists("7"));
    dict.purge();
    deepEqual(dumpUpdatedKeys(), []);
    deepEqual(dumpKeys(), ["5", "6", "7"]);
    dict.update("5", {});
    deepEqual(dumpUpdatedKeys(), ["5"]);
    dict.update("4", {});
    deepEqual(dumpUpdatedKeys(), ["4", "5"]);
    deepEqual(dumpKeys(), ["4", "5", "6", "7"]);
    dict.purge();
    deepEqual(dumpKeys(), ["4", "5"]);
    deepEqual(dumpUpdatedKeys(), []);
    dict.purge();
    deepEqual(dumpKeys(), []);

    var removeCount = 0;
    dict.update("one");
    deepEqual(dumpUpdatedKeys(), ["one"]);
    dict.update("two");
    deepEqual(dumpUpdatedKeys(), ["one", "two"]);
    dict.update("three");
    dict.purge();
    deepEqual(dumpKeys(), ["one", "three", "two"]);
    dict.update("two");
    dict.purge(function() {
        removeCount++;
    });
    deepEqual(dumpKeys(), ["two"]);
    equal(removeCount, 2);
    dict.update("four");
    var removeCounter = { count: 0 };
    dict.purge(function() {
        this.count++;
    }, removeCounter);
    equal(removeCounter.count, 1);
    dict.purge(function() {
        equal(String(this), "four");
    });

    dict = new base.UpdateTracker();
    dict.update("one");
    var thisObject = {}
    dict.forEach(function(item) {
        equal(this, thisObject);
    }, thisObject);

});

test("extends", 14, function() {

    var LikeDiv = base.extends("div", {
        init: function() {
            this.textContent = "awesome";
        },
        method: function(msg) {
            return 42;
        }
    });

    var LikeLikeDiv = base.extends(LikeDiv, {
        init: function() {
            this.className = "like";
        }
    });

    var LikeP = base.extends("p", {
        init: function(content) {
            this.textContent = content
        }
    });

    var LikeProgress = base.extends("progress", {
        init: function() {
            this.max = 100;
            this.value = 10;
        }
    });

    var LikeLikeProgress = base.extends(LikeProgress, {
        completed: function() {
            this.value = 100;
        }
    });

    document.body.appendChild(new LikeDiv());
    equals(document.body.lastChild.tagName, "DIV");
    equals(document.body.lastChild.innerHTML, "awesome");
    equals(document.body.lastChild.method(), 42);
    document.body.removeChild(document.body.lastChild);

    document.body.appendChild(new LikeLikeDiv());
    equals(document.body.lastChild.tagName, "DIV");
    equals(document.body.lastChild.innerHTML, "awesome");
    equals(document.body.lastChild.method(), 42);
    equals(document.body.lastChild.className, "like");
    document.body.removeChild(document.body.lastChild);

    document.body.appendChild(new LikeP("super"));
    equals(document.body.lastChild.tagName, "P");
    equals(document.body.lastChild.innerHTML, "super");
    raises(function() {
        document.body.lastChild.method();
    });
    document.body.removeChild(document.body.lastChild);

    document.body.appendChild(new LikeProgress());
    equals(document.body.lastChild.tagName, "PROGRESS");
    // Safari 5.1 lacks the <progress> element.
    // equals(document.body.lastChild.position, 0.1);
    equals(document.body.lastChild.innerHTML, "");
    raises(function() {
        document.body.lastChild.method();
    });
    document.body.removeChild(document.body.lastChild);

    document.body.appendChild(new LikeLikeProgress());
    equals(document.body.lastChild.tagName, "PROGRESS");
    // Safari 5.1 lacks the <progress> element.
    // equals(document.body.lastChild.position, 0.1);
    document.body.lastChild.completed();
    // Safari 5.1 lacks the <progress> element.
    // equals(document.body.lastChild.position, 1);
    document.body.removeChild(document.body.lastChild);
});

test("getURLParameter", 1, function() {
    ok(!base.getURLParameter('non-existant'));
});

test("parseJSONP", 6, function() {
    deepEqual(base.parseJSONP(""), {});
    deepEqual(base.parseJSONP('p({"key": "value"})'), {"key": "value"});
    deepEqual(base.parseJSONP('ADD_RESULTS({"dummy":"data"});'), {"dummy":"data"});
    deepEqual(base.parseJSONP('{"dummy":"data"}'), {"dummy":"data"});
    deepEqual(base.parseJSONP('ADD_RESULTS({"builder(1)":"data"});'), {"builder(1)":"data"});
    deepEqual(base.parseJSONP('{"builder(1)":"data"}'), {"builder(1)":"data"});
});

})();
