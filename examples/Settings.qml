import QtQuick 2.0
import QtQuick.LocalStorage 2.0

QtObject {
    id: root

    property var __db: LocalStorage.openDatabaseSync("QtAmazonS3", "1.0", "Application data", 10000)

    function readData(key, defaultValue) {
        var ret = defaultValue;
        root.__db.readTransaction( function (tx) {
            try {
                var rs = tx.executeSql('SELECT value FROM Settings WHERE key = ?', [key])
                if (rs.rows.length > 0)
                    ret = rs.rows.item(0).value
            } catch (e) {}
        } )
        return ret;
    }

    function saveData(key, value) {
        root.__db.transaction( function (tx) {
            tx.executeSql('CREATE TABLE IF NOT EXISTS Settings(key TEXT, value TEXT)')
            var rs = tx.executeSql('SELECT value FROM Settings WHERE key = ?', [key])
            if (rs.rows.length > 0) {
                tx.executeSql('UPDATE Settings SET value = ? WHERE key = ?', [value, key])
            } else {
                tx.executeSql('INSERT INTO Settings(key, value) VALUES(?, ?)', [key, value])
            }
        } )
    }
}
