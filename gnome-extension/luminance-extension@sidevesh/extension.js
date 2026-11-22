import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import Gio from 'gi://Gio';
import GLib from 'gi://GLib';
import {Extension} from 'resource:///org/gnome/shell/extensions/extension.js';

import {LuminanceClient} from './client.js';
import {Hotkeys} from './hotkeys.js';
import {LuminanceIndicator} from './indicator.js';
import {findMonitorIndex, showOSD} from './monitors.js';

// Monitors need a moment after hotplug before DDC/CI answers.
const RESCAN_DELAY_MS = 2000;

export default class LuminanceExtension extends Extension {
    enable() {
        this._settings = this.getSettings();
        this._client = new LuminanceClient();

        // OSD for changes made outside GNOME Shell (CLI, app window).
        this._unsubs = [
            this._client.subscribe('ShowOSD', (pct, name) => {
                const d = this._client.updateFromSignal(name, pct);
                showOSD(findMonitorIndex(this._client.displays, d), pct);
            }),
            this._client.subscribe('ShowContrastOSD', (pct, name) => {
                const d = this._client.displays.find(x => x.name === name);
                showOSD(findMonitorIndex(this._client.displays, d), pct, 'weather-clear-night-symbolic');
            }),
        ];

        // Watch for lid open/close directly via UPower (system bus) so display
        // list updates even when Luminance is not running.
        this._lidSubId = Gio.DBus.system.signal_subscribe(
            'org.freedesktop.UPower', 'org.freedesktop.DBus.Properties',
            'PropertiesChanged', '/org/freedesktop/UPower', null,
            Gio.DBusSignalFlags.NONE,
            (c, s, o, i, n, params) => {
                try {
                    const [ifaceName, changed] = params.deep_unpack();
                    if (ifaceName === 'org.freedesktop.UPower' && 'LidIsClosed' in changed)
                        this._client.refresh();
                } catch (e) {
                    console.error('Luminance: UPower lid event error', e);
                }
            });

        // Monitor plugged/unplugged: let the service re-enumerate DDC displays.
        global.backend.get_monitor_manager().connectObject('monitors-changed', () => {
            if (this._rescanId)
                GLib.source_remove(this._rescanId);
            this._rescanId = GLib.timeout_add(GLib.PRIORITY_DEFAULT, RESCAN_DELAY_MS, () => {
                this._rescanId = 0;
                this._client.rescan();
                return GLib.SOURCE_REMOVE;
            });
        }, this);

        this._indicator = new LuminanceIndicator(this, this._client);
        Main.panel.addToStatusArea(this.uuid, this._indicator);

        this._hotkeys = new Hotkeys(this._settings, this._client);
        this._settings.connectObject(
            'changed::override-brightness-keys', () => this._syncHotkeys(),
            this);
        this._syncHotkeys();

        // Initial fetch so the popup has data at login. Auto-activates the
        // service if it is not already running.
        this._client.refresh();
    }

    disable() {
        this._settings.disconnectObject(this);
        global.backend.get_monitor_manager().disconnectObject(this);
        if (this._rescanId) {
            GLib.source_remove(this._rescanId);
            this._rescanId = 0;
        }
        this._hotkeys.disable();
        this._indicator.destroy();

        for (const unsub of this._unsubs)
            unsub();
        Gio.DBus.system.signal_unsubscribe(this._lidSubId);

        this._client.destroy();
        this._hotkeys = null;
        this._indicator = null;
        this._client = null;
        this._settings = null;
        this._unsubs = null;
        this._lidSubId = 0;
    }

    _syncHotkeys() {
        if (this._settings.get_boolean('override-brightness-keys'))
            this._hotkeys.enable();
        else
            this._hotkeys.disable();
    }
}
