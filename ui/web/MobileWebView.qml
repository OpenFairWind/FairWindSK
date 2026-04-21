import QtQuick
import QtWebView

Item {
    id: root

    signal loadStarted()
    signal loadProgressChanged(int progress)
    signal loadFinished(bool ok)
    signal currentUrlNotified(string url)
    signal titleChanged(string title)
    signal webFocusChanged(bool focused)
    signal textInputActiveChanged(bool active)
    signal viewportMetricsChanged(real width, real height, bool keyboardVisible)

    property string currentUrl: webView.url.toString()
    property bool canGoBack: webView.canGoBack
    property bool canGoForward: webView.canGoForward
    property string pendingScript: ""
    property int safeAreaLeft: 0
    property int safeAreaTop: 0
    property int safeAreaRight: 0
    property int safeAreaBottom: 0
    property int keyboardInset: 0
    property bool keyboardVisible: Qt.inputMethod.visible
    property bool compactMode: false

    function loadUrl(urlString) {
        if (!urlString || urlString.length === 0) {
            return
        }
        webView.url = urlString
        root.currentUrlNotified(urlString)
    }

    function setHtmlContent(htmlString, baseUrlString) {
        const encoded = encodeURIComponent(htmlString)
        webView.url = "data:text/html;charset=UTF-8," + encoded
        root.currentUrlNotified(webView.url.toString())
    }

    function reloadPage() {
        webView.reload()
    }

    function stopLoading() {
        webView.stop()
    }

    function goBack() {
        if (webView.canGoBack) {
            webView.goBack()
        }
    }

    function goForward() {
        if (webView.canGoForward) {
            webView.goForward()
        }
    }

    function runScript(script) {
        pendingScript = script
        if (webView.runJavaScript) {
            webView.runJavaScript(script)
        }
    }

    function applyShellMetrics(leftInset, topInset, rightInset, bottomInset, keyboardHeight, keyboardShown, compactShell) {
        safeAreaLeft = leftInset
        safeAreaTop = topInset
        safeAreaRight = rightInset
        safeAreaBottom = bottomInset
        keyboardInset = keyboardHeight
        keyboardVisible = keyboardShown
        compactMode = compactShell
        root.viewportMetricsChanged(webView.width, webView.height, keyboardVisible)
    }

    function releaseWebFocus() {
        if (webView.focus) {
            webView.focus = false
        }
        root.forceActiveFocus()
        if (Qt.inputMethod.visible) {
            Qt.inputMethod.hide()
        }
        root.webFocusChanged(false)
        root.textInputActiveChanged(false)
    }

    Component.onCompleted: root.viewportMetricsChanged(webView.width, webView.height, keyboardVisible)
    onWidthChanged: root.viewportMetricsChanged(webView.width, webView.height, keyboardVisible)
    onHeightChanged: root.viewportMetricsChanged(webView.width, webView.height, keyboardVisible)

    Connections {
        target: Qt.inputMethod

        function onVisibleChanged() {
            root.keyboardVisible = Qt.inputMethod.visible
            root.textInputActiveChanged(root.keyboardVisible && webView.activeFocus)
            root.viewportMetricsChanged(webView.width, webView.height, root.keyboardVisible)
        }

        function onKeyboardRectangleChanged() {
            root.viewportMetricsChanged(webView.width, webView.height, root.keyboardVisible)
        }
    }

    WebView {
        id: webView
        anchors {
            fill: parent
            leftMargin: root.safeAreaLeft
            topMargin: root.safeAreaTop
            rightMargin: root.safeAreaRight
            bottomMargin: Math.max(root.safeAreaBottom, root.keyboardVisible ? root.keyboardInset : 0)
        }
        focus: true

        onLoadingChanged: function(loadRequest) {
            if (loadRequest.status === WebView.LoadStartedStatus) {
                root.loadStarted()
            }
            if (loadRequest.status === WebView.LoadSucceededStatus) {
                if (root.pendingScript && root.pendingScript.length > 0 && webView.runJavaScript) {
                    webView.runJavaScript(root.pendingScript)
                }
                root.loadFinished(true)
            } else if (loadRequest.status === WebView.LoadFailedStatus) {
                root.loadFinished(false)
            }
        }

        onLoadProgressChanged: root.loadProgressChanged(loadProgress)
        onUrlChanged: root.currentUrlNotified(url.toString())
        onTitleChanged: root.titleChanged(title)
        onActiveFocusChanged: {
            root.webFocusChanged(activeFocus)
            root.textInputActiveChanged(root.keyboardVisible && activeFocus)
        }
    }
}
