import QtQuick 1.0

VisualDataModel {
    function setRoot() {
        rootIndex = modelIndex(0);
    }
    function setRootToParent() {
        rootIndex = parentModelIndex();
    }
    model: myModel
}
