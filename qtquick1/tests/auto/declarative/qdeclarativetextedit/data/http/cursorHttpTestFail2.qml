import QtQuick 1.0

Rectangle { width: 300; height: 300; color: "white"
    resources: [
        Component { id:cursorWait; WaitItem { objectName: "delegateSlow" } },
        Component { id:cursorNorm; NormItem { objectName: "delegateOkay" } },
        Component { id:cursorErr; ErrItem { objectName: "delegateErrorA" } }
    ]
    TextEdit {
        cursorDelegate: cursorWait
    }
    TextEdit {
        cursorDelegate: cursorNorm
    }
    TextEdit {
        cursorDelegate: cursorErr
    }
}
