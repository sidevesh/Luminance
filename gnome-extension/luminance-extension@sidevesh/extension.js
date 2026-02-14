import Gio from 'gi://Gio';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';

const IFACE_NAME = 'com.sidevesh.Luminance';
const SIGNAL_NAME = 'ShowOSD';

export default class Extension {
    enable() {
        this._signalId = Gio.DBus.session.signal_subscribe(
            null,
            IFACE_NAME,
            SIGNAL_NAME,
            null,
            null,
            Gio.DBusSignalFlags.NONE,
            (connection, senderName, objectPath, interfaceName, signalName, parameters) => {
                try {
                    const [percentage, monitorName] = parameters.deep_unpack();
                    this._showOSD(percentage);
                } catch (e) {
                    console.error('Luminance OSD: Error handling signal', e);
                }
            }
        );
        console.log('Luminance OSD: Enabled');
    }

    disable() {
        if (this._signalId) {
            Gio.DBus.session.signal_unsubscribe(this._signalId);
            this._signalId = null;
        }
        console.log('Luminance OSD: Disabled');
    }

    _showOSD(percentage) {
        // percentage is 0-100 from C app.
        // OSD usually takes 0.0 - 1.0 (or normalized).
        const normalizedValue = percentage / 100.0;
        const icon = Gio.Icon.new_for_string('display-brightness-symbolic');
        if (Main.osdWindowManager.showAll) {
             Main.osdWindowManager.showAll(icon, null, normalizedValue, 1);
        } else {
             Main.osdWindowManager.show(-1, icon, null, normalizedValue, 1);
        }
    }
}
