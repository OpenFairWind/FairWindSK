import QtQuick
import QtWebView

Item {
    id: root

    signal loadStarted()
    signal loadProgressChanged(int progress)
    signal loadFinished(bool ok)
    signal currentUrlNotified(string url)
    signal titleChanged(string title)

    property string currentUrl: webView.url.toString()
    property bool canGoBack: webView.canGoBack
    property bool canGoForward: webView.canGoForward

    function loadUrl(urlString) {
        if (!urlString || urlString.length === 0) {
            return
        }
        webView.url = urlString
    }

    function setHtmlContent(htmlString, baseUrlString) {
        const encoded = encodeURIComponent(htmlString)
        webView.url = "data:text/html;charset=UTF-8," + encoded
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
        if (webView.runJavaScript) {
            webView.runJavaScript(script)
        }
    }

    WebView {
        id: webView
        anchors.fill: parent

        onLoadingChanged: function(loadRequest) {
            if (loadRequest.status === WebView.LoadStartedStatus) {
                root.loadStarted()
            }
            if (loadRequest.status === WebView.LoadSucceededStatus) {
                root.loadFinished(true)
            } else if (loadRequest.status === WebView.LoadFailedStatus) {
                root.loadFinished(false)
            }
        }

        onLoadProgressChanged: root.loadProgressChanged(loadProgress)
        onUrlChanged: root.currentUrlNotified(url.toString())
        onTitleChanged: root.titleChanged(title)
    }
}
