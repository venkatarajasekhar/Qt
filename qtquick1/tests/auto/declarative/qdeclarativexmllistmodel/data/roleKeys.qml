import QtQuick 1.0

XmlListModel {
    query: "/data/item"
    XmlRole { id: nameRole; name: "name"; query: "name/string()"; isKey: true }
    XmlRole { name: "age"; query: "age/number()"; isKey: true }
    XmlRole { name: "sport"; query: "sport/string()" }

    function disableNameKey() {
        nameRole.isKey = false;
    }
}

