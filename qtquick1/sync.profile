%modules = ( # path to module name map
    "QtDeclarative" => "$basedir/src/declarative",
);
# Module dependencies.
# Every module that is required to build this module should have one entry.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - any git symbolic ref resolvable from the module's repository (e.g. "refs/heads/master" to track master branch)
#   - an empty string to use the same branch under test (dependencies will become "refs/heads/master" if we are in the master branch)
#
%dependencies = (
    "qtbase" => "",
    "qtscript" => "",
    "qtxmlpatterns" => "",
    "qtdeclarative" => "",
    "qtactiveqt" => "",
    "qttools" => "",
    "qtwebkit" => "",
);
