/* Copyright (c) 2012 Silk Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Silk nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SILK BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

import QtQuick 2.0

import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

import QtAmazonS3 0.1 as S3

ApplicationWindow {
    id: window
    width: 360
    height: 360
    minimumWidth: 400
    minimumHeight: 300

    title: qsTr('Amazon S3')

    Settings { id: settings }

    S3.Account {
        id: account
        property bool signedIn: account.awsAccessKeyId != '' && account.awsSecretAccessKey != ''
        awsAccessKeyId: settings.readData('account/awsAccessKeyId', '')
        awsSecretAccessKey: settings.readData('account/awsSecretAccessKey', '')
    }

    S3.Service {
        id: service
        account: account
        onRowsInserted: {
            for (var i = first; i <= last; i++) {
                var tab = tabView.addTab(service.get(i).name, tabComponent)
                tab.active = true
                tab.item.account = account
                tab.item.name = service.get(i).name
            }
        }
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr('Account')
            MenuItem {
                text: qsTr('Sign in')
                visible: !account.signedIn
                onTriggered: stackView.push(signInPage)
            }
            MenuItem {
                text: qsTr('Sign out')
                visible: account.signedIn
                onTriggered: {
                    settings.saveData('account/awsAccessKeyId', '')
                    settings.saveData('account/awsSecretAccessKey', '')
                    account.awsAccessKeyId = ''
                    account.awsSecretAccessKey = ''
                    stackView.clear()
                    stackView.push(signInPage)
                }
            }
        }
        Menu {
            title: "Help"
            MenuItem { text: "About..."; enabled: false }
        }
    }
    statusBar: Item {
        height: 0
        width: window.width
        clip: true
        RowLayout {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 2
            Label { text: qsTr('test') }
            ProgressBar {
                Layout.fillWidth: true
                indeterminate: service.loading && service.progress < 0.01
                value: service.progress / 100.0
            }
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent

        initialItem: account.signedIn ? tabView : signInPage
    }

    Item {
        id: signInPage
        visible: false

        Column {
            anchors.centerIn: parent
            spacing: 20
            TextField {
                id: awsAccessKeyId
                anchors.horizontalCenter: parent.horizontalCenter
                placeholderText: qsTr('Access Key ID')
            }

            TextField {
                id: awsSecretAccessKey
                anchors.horizontalCenter: parent.horizontalCenter
                placeholderText: qsTr('Secret Access Key')
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 20

                Button {
                    text: qsTr('Sign in')
                    onClicked: {
                        settings.saveData('account/awsAccessKeyId', awsAccessKeyId.text)
                        settings.saveData('account/awsSecretAccessKey', awsSecretAccessKey.text)
                        account.awsAccessKeyId = awsAccessKeyId.text
                        account.awsSecretAccessKey = awsSecretAccessKey.text
                        stackView.push(tabView, true, true)
                    }
                }
            }
        }
    }

    TabView {
        id: tabView
        tabPosition: Qt.TopEdge
        visible: false

        Tab {
            title: 'Create Bucket'

            Item {
                enabled: false
                Column {
                    anchors.centerIn: parent
                    spacing: 20
                    GridLayout {
                        id: grid
                        anchors.horizontalCenter: parent.horizontalCenter
                        columns: 2

                        Label {
                            text: qsTr('Bucket Name:')
                        }

                        TextField {

                        }

                        Label {
                            text: qsTr('Region:')
                        }

                        ComboBox {
                            model: ListModel {
                                ListElement {
                                    text: 'US Standard'
                                    value: ''
                                }
                                ListElement {
                                    text: 'Oregon'
                                    value: 'us-west-2'
                                }
                                ListElement {
                                    text: 'Northern Calfornia'
                                    value: 'us-west-1'
                                }
                                ListElement {
                                    text: 'Ireland'
                                    value: 'EU'
                                }
                                ListElement {
                                    text: 'Singapore'
                                    value: 'ap-southeast-1'
                                }
                                ListElement {
                                    text: 'Tokyo'
                                    value: 'ap-northeast-1'
                                }
                                ListElement {
                                    text: 'Sydney'
                                    value: 'ap-southeast-2'
                                }
                                ListElement {
                                    text: 'Sao Paulo'
                                    value: 'sa-east-1'
                                }
                            }
                        }

                    }
                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 20

                        Button {
                            text: qsTr('Create Bucket')
                        }
                        Button {
                            text: qsTr('Cancel')
                        }
                    }
                }
            }
        }
    }

    Component {
        id: tabComponent
        Item {
            property alias name: bucket.name
            property alias account: bucket.account

            ToolBar {
                id: toolbar
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                }

                RowLayout {
                    spacing: 2
                    anchors.verticalCenter: parent.verticalCenter
                    ToolButton { iconSource: "properties.png" }
                    ToolButton { iconSource: "qrc:images/qt_icon.png" }
                }
            }

            TableView {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: toolbar.bottom
                    bottom: parent.bottom
                }

                frameVisible: false
                highlightOnFocus: false
                model: S3.Bucket {
                    id: bucket
                    delimiter: '/'
                }

                TableViewColumn {
                    title: qsTr('')
                    role: 'key'
                    width: 23
                    delegate: Item {
                        Image {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.margins: 2
                            source: itemValue[itemValue.length - 1] === '/' ? 'folder.png' : 'object.png'
                        }
                    }
                }
                TableViewColumn {
                    title: qsTr('Name')
                    role: 'key'
                }
                TableViewColumn {
                    title: qsTr('Storage Class')
                    role: 'storageClass'
                }
                TableViewColumn {
                    title: qsTr('Size')
                    role: 'size'
                }
                TableViewColumn {
                    title: qsTr('Last Modified')
                    role: 'lastModified'
                }
            }
        }
    }
}
