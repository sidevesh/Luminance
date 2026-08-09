import Gio from 'gi://Gio';
import GLib from 'gi://GLib';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import {BrightnessScale} from 'resource:///org/gnome/shell/misc/brightnessManager.js';

const IFACE_NAME = 'com.sidevesh.Luminance';
const BUS_NAME = 'com.sidevesh.Luminance';
const OBJECT_PATH = '/com/sidevesh/Luminance/Service';
const DEBOUNCE_MS = 300;

Gio._promisify(Gio.DBusConnection.prototype, 'call', 'call_finish');

export default class Extension {
    enable() {
        this._subs = [];
        this._subs.push(Gio.DBus.session.signal_subscribe(
            null, IFACE_NAME, 'ShowOSD', null, null, Gio.DBusSignalFlags.NONE,
            (c, s, o, i, n, params) => {
                try {
                    const [pct, name] = params.deep_unpack();
                    this._showOSD(pct, 'display-brightness-symbolic');
                    this._syncScale(name, pct);
                } catch (e) { console.error('Luminance: ShowOSD error', e); }
            }
        ));
        this._subs.push(Gio.DBus.session.signal_subscribe(
            null, IFACE_NAME, 'ShowContrastOSD', null, null, Gio.DBusSignalFlags.NONE,
            (c, s, o, i, n, params) => {
                try {
                    const [pct] = params.deep_unpack();
                    this._showOSD(pct, 'weather-clear-night-symbolic');
                } catch (e) { console.error('Luminance: ShowContrastOSD error', e); }
            }
        ));

        // _monitors stores persistent data (id -> {name, brightness}) and is NOT
        // cleared when the service vanishes — sliders stay visible with last known
        // values so the user is not surprised by them disappearing.
        // _scales stores live widget references, rebuilt on every inject since
        // GNOME's _sync() destroys the previous section contents each time.
        this._monitors = new Map();
        this._scales = new Map();
        this._debounces = new Map();
        this._quietSupported = undefined;

        // Watch for Luminance appearing/vanishing on the session bus.
        // NONE: fires appeared immediately if the name is already owned when
        // watching starts. Auto-activation of the ghost is triggered by the
        // D-Bus method calls themselves (GetMonitors, SetBrightness) not by
        // this watcher.
        this._nameWatchId = Gio.DBus.session.watch_name(
            BUS_NAME,
            Gio.BusNameWatcherFlags.NONE,
            () => this._refresh(),          // appeared: fetch fresh state
            () => this._onServiceVanished() // vanished: cancel pending writes
        );

        // Watch for lid open/close directly via UPower (system bus) so display
        // list updates even when Luminance is not running.
        this._lidSubId = Gio.DBus.system.signal_subscribe(
            'org.freedesktop.UPower',
            'org.freedesktop.DBus.Properties',
            'PropertiesChanged',
            '/org/freedesktop/UPower',
            null,
            Gio.DBusSignalFlags.NONE,
            (c, s, o, i, n, params) => {
                try {
                    const [ifaceName, changed] = params.deep_unpack();
                    if (ifaceName === 'org.freedesktop.UPower' && 'LidIsClosed' in changed)
                        this._refresh();
                } catch (e) { console.error('Luminance: UPower lid event error', e); }
            }
        );

        this._setupQuickSettings();

        // Trigger an initial refresh so the arrow appears at login without the
        // user having to first open quick settings. DBusCallFlags.NONE means
        // this auto-activates the ghost if it is not already running. _bi is
        // already set by _setupQuickSettings so _injectRows() will run once the
        // async GetMonitors returns and will set menuEnabled = true on the
        // brightness item.
        this._refresh();
    }

    disable() {
        for (const id of this._subs ?? [])
            Gio.DBus.session.signal_unsubscribe(id);
        this._subs = [];

        if (this._lidSubId) {
            Gio.DBus.system.signal_unsubscribe(this._lidSubId);
            this._lidSubId = null;
        }

        if (this._nameWatchId) {
            Gio.DBus.session.unwatch_name(this._nameWatchId);
            this._nameWatchId = null;
        }

        if (this._menuOpenId)
            this._bi?.menu.disconnect(this._menuOpenId);
        if (this._changedId)
            Main.brightnessManager.disconnect(this._changedId);

        this._bi?._sync();
        this._bi = null;
        this._menuOpenId = null;
        this._changedId = null;

        for (const id of this._debounces?.values() ?? [])
            GLib.source_remove(id);
        this._debounces?.clear();
        this._scales?.clear();
        this._monitors?.clear();
    }

    _showOSD(pct, icon) {
        const val = pct / 100.0;
        const ic = Gio.Icon.new_for_string(icon);
        if (Main.osdWindowManager.showAll)
            Main.osdWindowManager.showAll(ic, null, val, 1);
        else
            Main.osdWindowManager.show(-1, ic, null, val, 1);
    }

    _setupQuickSettings() {
        try {
            const bi = Main.panel.statusArea.quickSettings?._brightness?.quickSettingsItems?.[0];
            if (!bi?._monitorBrightnessSection || typeof bi._connectSlider !== 'function')
                return;

            this._bi = bi;
            // Connect after BrightnessItem so our handler fires after its native _sync clears the menu
            this._changedId = Main.brightnessManager.connect('changed', () => this._injectRows());
            this._menuOpenId = bi.menu.connect('open-state-changed', (m, open) => {
                if (open) this._refresh();
            });
        } catch (e) {
            console.error('Luminance: Quick Settings setup error', e);
        }
    }

    _onServiceVanished() {
        // Luminance stopped — keep _monitors data so sliders remain visible with
        // last known values. Cancel any pending debounced writes since the service
        // is gone; they will naturally re-trigger if the user interacts with the
        // slider again (which auto-starts the ghost via the NONE D-Bus flag).
        for (const id of this._debounces.values())
            GLib.source_remove(id);
        this._debounces.clear();
        // _scales remain valid; _monitors retained intentionally
    }

    _injectRows() {
        if (!this._bi?.visible || !this._monitors?.size) return;

        // Always create fresh BrightnessScale widgets — the previous ones were
        // destroyed by GNOME's native _sync() that ran before this call.
        for (const id of this._debounces.keys())
            GLib.source_remove(this._debounces.get(id));
        this._debounces.clear();
        this._scales.clear();

        this._bi.menuEnabled = true;
        for (const [id, m] of this._monitors) {
            const scale = new BrightnessScale(m.name, m.brightness / 100.0, 20);
            this._scales.set(id, scale);
            this._watchScale(id, scale);
            const slider = this._bi._monitorBrightnessSection.addSlider(scale);
            this._bi._connectSlider(slider, scale);
        }
    }

    async _refresh() {
        if (this._refreshing) return;
        this._refreshing = true;
        try {
            let monitors;
            try {
                // NONE flag: D-Bus auto-activates the ghost service if it is not
                // already running. Opening quick settings always fetches fresh state
                // and brings up the ghost ephemerally if needed.
                const v = await Gio.DBus.session.call(
                    BUS_NAME, OBJECT_PATH, IFACE_NAME, 'GetMonitors',
                    null, new GLib.VariantType('(s)'), Gio.DBusCallFlags.NONE, -1, null);
                monitors = JSON.parse(v.deep_unpack()[0])
                    .filter(m => !m.name.startsWith('Built-in Display'));
            } catch (e) {
                console.error('Luminance: GetMonitors error', e);
                return;
            }

            // Guard: empty response during startup/rescan — keep existing data
            if (monitors.length === 0) return;

            const seen = new Set(monitors.map(m => m.id));
            for (const id of [...this._monitors.keys()]) {
                if (!seen.has(id)) this._monitors.delete(id);
            }
            for (const m of monitors) {
                this._monitors.set(m.id, {name: m.name, brightness: m.brightness});
            }

            // Native _sync rebuilds the menu cleanly; then we append DDC rows
            this._bi?._sync();
            this._injectRows();
        } finally {
            this._refreshing = false;
        }
    }

    _watchScale(id, scale) {
        scale.connect('notify::value', () => {
            if (scale._external) return;
            const prev = this._debounces.get(id);
            if (prev) GLib.source_remove(prev);
            this._debounces.set(id, GLib.timeout_add(GLib.PRIORITY_DEFAULT, DEBOUNCE_MS, () => {
                this._debounces.delete(id);
                this._setBrightness(id, scale.value * 100.0);
                return GLib.SOURCE_REMOVE;
            }));
        });
    }

    // Falls back to SetBrightness (shows OSD) if app predates SetBrightnessQuiet
    async _setBrightness(id, pct) {
        const method = this._quietSupported === false ? 'SetBrightness' : 'SetBrightnessQuiet';
        try {
            await Gio.DBus.session.call(
                BUS_NAME, OBJECT_PATH, IFACE_NAME, method,
                new GLib.Variant('(sd)', [id, pct]),
                null, Gio.DBusCallFlags.NONE, -1, null);
            this._quietSupported ??= true;
        } catch (e) {
            if (method === 'SetBrightnessQuiet' &&
                Gio.DBusError.is_remote_error(e) &&
                Gio.DBusError.get_remote_error(e) === 'org.freedesktop.DBus.Error.UnknownMethod') {
                this._quietSupported = false;
                this._setBrightness(id, pct);
                return;
            }
            console.error(`Luminance: SetBrightness error for ${id}`, e);
        }
    }

    _syncScale(name, pct) {
        for (const scale of this._scales?.values() ?? []) {
            if (scale.name !== name) continue;
            scale._external = true;
            scale.value = Math.max(0, Math.min(1, pct / 100.0));
            scale._external = false;
            break;
        }
    }
}
