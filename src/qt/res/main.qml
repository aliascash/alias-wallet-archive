import QtQuick 2.9
import QtWebView 1.1

Item {
    id: webViewRootItem
    visible: true
    WebView {
        visible: true
        id: webView
        objectName: "webView"
        anchors.fill: parent
        onLoadingChanged: {
            if (loadRequest.errorString)
                { console.error(loadRequest.errorString); }
        }
    }
}
