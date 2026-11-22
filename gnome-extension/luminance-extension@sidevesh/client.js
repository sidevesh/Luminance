import Gio from 'gi://Gio';
import GLib from 'gi://GLib';
import {EventEmitter} from 'resource:///org/gnome/shell/misc/signals.js';

export const APP_ID = 'com.sidevesh.Luminance';
const IFACE_NAME = APP_ID;
const BUS_NAME = APP_ID;
const OBJECT_PATH = '/com/sidevesh/Luminance/Service';

// Cached brightness is trusted for this long before a hotkey press refetches.
const STALE_MS = 10000;
// After a write, ignore fetched values for a while: the service may report a
// reading taken before our write landed.
const WRITE_SETTLE_MS = 1500;

Gio._promisify(Gio.DBusConnection.prototype, 'call', 'call_finish');

/**
 * D-Bus client for the Luminance service. Caches the DDC display list and
 * coalesces brightness writes so at most one call per display is in flight.
 *
 * Signals:
 *   'changed'                      display list or availability changed
 *   'brightness-changed' (id, pct) brightness of one display changed
 */
export class LuminanceClient extends EventEmitter {
    constructor() {
        super();
        // [{id, name, brightness, vendor?, model?, serial?}] — DDC displays only.
        // Kept when the service vanishes so UI keeps last known values.
        this.displays = [];
        this.available = false;
        this._lastFetch = 0;
        this._refreshPromise = null;
        this._writes = new Map(); // id -> {pending: number|null, inFlight: bool, lastWrite: ms}
        this._quietSupported = undefined;

        // NONE: 'appeared' fires immediately if the name is already owned.
        // Auto-activation of the service is triggered by method calls, not this.
        this._nameWatchId = Gio.DBus.session.watch_name(
            BUS_NAME, Gio.BusNameWatcherFlags.NONE,
            () => {
                this.available = true;
                this.refresh();
            },
            () => {
                this.available = false;
                for (const w of this._writes.values())
                    w.pending = null;
                this.emit('changed');
            });
    }

    destroy() {
        if (this._nameWatchId) {
            Gio.DBus.session.unwatch_name(this._nameWatchId);
            this._nameWatchId = 0;
        }
        this._writes.clear();
        this.displays = [];
        this.disconnectAll();
    }

    /** True when the service reports EDID identity (vendor/model/serial). */
    get hasIdentity() {
        return this.displays.some(d => typeof d.vendor === 'string');
    }

    get(id) {
        return this.displays.find(d => d.id === id) ?? null;
    }

    refresh() {
        this._refreshPromise ??= this._doRefresh().finally(() => {
            this._refreshPromise = null;
        });
        return this._refreshPromise;
    }

    async ensureFresh() {
        if (this._refreshPromise || !this.displays.length ||
            Date.now() - this._lastFetch > STALE_MS)
            await this.refresh();
    }

    async _doRefresh() {
        let list;
        try {
            // NONE flag: auto-activates the service if it is not running.
            const v = await Gio.DBus.session.call(
                BUS_NAME, OBJECT_PATH, IFACE_NAME, 'GetMonitors',
                null, new GLib.VariantType('(s)'), Gio.DBusCallFlags.NONE, -1, null);
            list = JSON.parse(v.deep_unpack()[0]);
        } catch (e) {
            console.error('Luminance: GetMonitors error', e);
            return;
        }

        // Internal backlights are handled natively by GNOME Shell.
        const ddc = list.filter(m => m.type
            ? m.type === 'ddc'
            : !m.name.startsWith('Built-in Display'));

        const now = Date.now();
        this._lastFetch = now;
        this.displays = ddc.map(m => {
            const w = this._writes.get(m.id);
            const settling = w && (w.inFlight || w.pending !== null ||
                now - w.lastWrite < WRITE_SETTLE_MS);
            return {
                id: m.id,
                name: m.name,
                brightness: settling ? this.get(m.id)?.brightness ?? m.brightness : m.brightness,
                vendor: m.vendor,
                model: m.model,
                serial: m.serial,
            };
        });
        this.emit('changed');
    }

    /**
     * Ask the service to re-enumerate displays (monitor plugged/unplugged),
     * then refetch. Skipped when the service is not running: it enumerates
     * fresh on its next start anyway.
     */
    async rescan() {
        if (!this.available)
            return;
        try {
            await Gio.DBus.session.call(
                BUS_NAME, OBJECT_PATH, IFACE_NAME, 'Rescan',
                null, null, Gio.DBusCallFlags.NO_AUTO_START, -1, null);
        } catch (e) {
            // UnknownMethod: older service without Rescan — refetch anyway.
            if (!Gio.DBusError.is_remote_error(e) ||
                Gio.DBusError.get_remote_error(e) !== 'org.freedesktop.DBus.Error.UnknownMethod')
                console.error('Luminance: Rescan error', e);
        }
        await this.refresh();
    }

    /** Update cache from a ShowOSD signal (brightness changed elsewhere). */
    updateFromSignal(name, pct) {
        const d = this.displays.find(x => x.name === name);
        if (!d)
            return null;
        d.brightness = pct;
        this.emit('brightness-changed', d.id, pct);
        return d;
    }

    /** Set brightness (0–100). Updates the cache now, writes coalesced. */
    setBrightness(id, pct) {
        pct = Math.max(0, Math.min(100, Math.round(pct)));
        const d = this.get(id);
        if (d)
            d.brightness = pct;
        this.emit('brightness-changed', id, pct);

        let w = this._writes.get(id);
        if (!w) {
            w = {pending: null, inFlight: false, lastWrite: 0};
            this._writes.set(id, w);
        }
        w.pending = pct;
        this._pump(id, w);
        return pct;
    }

    async _pump(id, w) {
        if (w.inFlight)
            return;
        while (w.pending !== null) {
            const pct = w.pending;
            w.pending = null;
            w.inFlight = true;
            try {
                await this._write(id, pct);
            } finally {
                w.inFlight = false;
                w.lastWrite = Date.now();
            }
        }
    }

    // Falls back to SetBrightness (shows OSD) if app predates SetBrightnessQuiet
    async _write(id, pct) {
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
                await this._write(id, pct);
                return;
            }
            console.error(`Luminance: SetBrightness error for ${id}`, e);
        }
    }

    /** Subscribe to service signals. Returns an unsubscribe function. */
    subscribe(signal, callback) {
        const subId = Gio.DBus.session.signal_subscribe(
            null, IFACE_NAME, signal, null, null, Gio.DBusSignalFlags.NONE,
            (c, s, o, i, n, params) => {
                try {
                    callback(...params.deep_unpack());
                } catch (e) {
                    console.error(`Luminance: ${signal} handler error`, e);
                }
            });
        return () => Gio.DBus.session.signal_unsubscribe(subId);
    }
}
