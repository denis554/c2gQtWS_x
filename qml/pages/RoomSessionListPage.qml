// ekke (Ekkehard Gentz) @ekkescorner
import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0
import QtGraphicalEffects 1.0
import org.ekkescorner.data 1.0
import "../common"

Page {
    id: roomSessionListPage
    focus: true
    property string name: "roomSessionListPage"
    property int roomId
    property Room room
    bottomPadding: 6
    topPadding: 6

    header:
        ColumnLayout {
        Layout.fillWidth: true
        LabelHeadline {
            topPadding: 10
            leftPadding: 24
            rightPadding: 12
            id: headerLabel
            color: accentColor
            elide: Label.ElideRight
        }
        // workaround for BUG: if elide then bottompadding lost
        Item {
            height: 2
        }
    }



    // SECTION HEADER DELEGATE
    Component {
        id: sectionHeading
        Pane {
            topPadding: 0
            bottomPadding: 12
            leftPadding: 0
            rightPadding: 0
            width: parent.width
            background: Rectangle{color: Material.listHighlightColor}
            ColumnLayout {
                y: -6
                width: parent.width
                height: 48
                RowLayout {
                    Layout.topMargin: 6
                    spacing: 10
                    // TODO BUG IconColored sometimes washed out in list
                    IconActive {
                        Layout.leftMargin: 16 +36 + 20
                        imageName: "time.png"
                    }
                    LabelTitle {
                        text: dataUtil.localWeekdayAndTime(section)
                        anchors.verticalCenter: parent.verticalCenter
                        color: primaryColor
                        font.bold: true
                    }
                } // section row
                //HorizontalListDivider{}
            } // section col
        } // section Pane
    } // sectionHeading Component

    // LIST VIEW
    SessionListView {
        id: listView
    }

    Component.onDestruction: {
        cleanup()
    }

    // called immediately after Loader.loaded
    function init() {
        console.log("Init from roomSessionListPage")
        room = dataManager.findRoomByRoomId(roomId)
        headerLabel.text = room.roomName
        listView.model = room.sessionsPropertyList
        console.log(" Room "+room.roomName+" Sessions:"+room.sessionsPropertyList.length)

    }
    // called from Component.destruction
    function cleanup() {
        console.log("Cleanup done from roomSessionListPage")
    }
} // end primaryPage
