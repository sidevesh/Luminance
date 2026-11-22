import Gio from 'gi://Gio';
import Meta from 'gi://Meta';
import Shell from 'gi://Shell';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';

import {getMutterMonitors, getNativeScale, getPointerMonitor, matchDisplays, showOSD} from './monitors.js';

// GNOME Shell's own brightness key bindings (misc/brightnessManager.js).
const SHELL_KEYBINDING_SCHEMA = 'org.gnome.shell.keybindings';
const SHELL_BINDINGS = {
    'screen-brightness-up': '_screenBrightnessUp',
    'screen-brightness-down': '_screenBrightnessDown',
};
const OWN_BINDINGS = {
    'brightness-up': +1,
    'brightness-down': -1,
};

/**
 * Takes over the brightness keys. A press changes the monitor under the
 * pointer: through GNOME's native backlight if it has one, otherwise through
 * DDC/CI via the Luminance service. Monitors with neither are left alone.
 */
export class Hotkeys {
    constructor(settings, client) {
        this._settings = settings;
        this._client = client;
        this._active = false;
    }

    enable() {
        if (this._active)
            return;

        // An accelerator can be grabbed only once: release GNOME's first.
        for (const name of Object.keys(SHELL_BINDINGS))
            Main.wm.removeKeybinding(name);

        for (const [name, dir] of Object.entries(OWN_BINDINGS)) {
            Main.wm.addKeybinding(name, this._settings,
                Meta.KeyBindingFlags.NONE, Shell.ActionMode.ALL,
                () => this._step(dir).catch(e => console.error('Luminance: hotkey error', e)));
        }
        this._active = true;
    }

    disable() {
        if (!this._active)
            return;

        for (const name of Object.keys(OWN_BINDINGS))
            Main.wm.removeKeybinding(name);

        // Restore GNOME's bindings with its own handlers.
        const shellSettings = new Gio.Settings({schema_id: SHELL_KEYBINDING_SCHEMA});
        for (const [name, method] of Object.entries(SHELL_BINDINGS)) {
            Main.wm.addKeybinding(name, shellSettings,
                Meta.KeyBindingFlags.NONE, Shell.ActionMode.ALL,
                () => Main.brightnessManager?.[method]?.());
        }
        this._active = false;
    }

    async _step(dir) {
        const monitors = getMutterMonitors();
        const monitor = getPointerMonitor(monitors);
        if (!monitor)
            return;

        const native = getNativeScale(monitor.logical);
        if (native) {
            // Shell shows its own OSD for native scales.
            if (dir > 0)
                native.stepUp();
            else
                native.stepDown();
            return;
        }

        await this._client.ensureFresh();
        const ddc = matchDisplays(monitors, this._client.displays)
            .byConnector.get(monitor.connector);
        if (!ddc)
            return;

        const step = this._settings.get_int('brightness-step');
        const pct = this._client.setBrightness(ddc.id, ddc.brightness + dir * step);
        showOSD(monitor.index, pct);
    }
}
