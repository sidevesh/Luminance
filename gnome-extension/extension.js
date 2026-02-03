import Gio from 'gi://Gio';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';

const IFACE_NAME = 'com.sidevesh.Luminance';
const SIGNAL_NAME = 'ShowOSD';

export default class Extension {
    enable() {
        this._signalId = Gio.DBus.session.signal_subscribe(
            null, // sender (null matches any)
            IFACE_NAME,
            SIGNAL_NAME,
            null, // object path
            null, // arg0
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
        // The original eval code divided by 100.0.
        const normalizedValue = percentage / 100.0;
        const icon = Gio.Icon.new_for_string('display-brightness-symbolic');

        // Note: Using show() instead of showAll() as showAll() is not standard in recent Shell versions.
        // If your specific setup requires showAll, please revert.
        // show(monitorIndex, icon, label, level, maxLevel)
        // monitorIndex -1 usually means current/primary.
        
        // Trying to match previous behavior: "Main.osdWindowManager.showAll(..., ..., %f, 1)"
        if (Main.osdWindowManager.showAll) {
             Main.osdWindowManager.showAll(icon, null, normalizedValue, 1);
        } else {
             Main.osdWindowManager.show(-1, icon, null, normalizedValue, 1);
        }
    }
}
