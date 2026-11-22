import Gio from 'gi://Gio';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';

const norm = s => (s ?? '').trim().toLowerCase();

/** Active Mutter monitors, one entry per physical monitor. */
export function getMutterMonitors() {
    const manager = global.backend.get_monitor_manager();
    const result = [];
    for (const logical of manager.get_logical_monitors()) {
        for (const m of logical.get_monitors()) {
            if (!m.is_active())
                continue;
            result.push({
                logical,
                index: logical.get_number(),
                connector: m.get_connector(),
                vendor: m.get_vendor(),
                product: m.get_product(),
                serial: m.get_serial(),
                displayName: m.get_display_name(),
                hasBacklight: !!m.get_backlight(),
            });
        }
    }
    return result;
}

/** GNOME Shell's own brightness scale for a logical monitor, if any. */
export function getNativeScale(logical) {
    return Main.brightnessManager?._monitorScales?.get(logical) ?? null;
}

/**
 * Match DDC displays (from the Luminance service) to Mutter monitors by EDID
 * vendor + model (+ serial when several share a model).
 *
 * Old service versions report no EDID identity: then, if there is exactly one
 * DDC display and exactly one monitor without a native backlight, pair them.
 *
 * Returns {byConnector: Map<connector, ddcDisplay>, unmatched: ddcDisplay[]}.
 */
export function matchDisplays(monitors, displays) {
    const byConnector = new Map();
    const used = new Set();

    const hasIdentity = displays.some(d => typeof d.vendor === 'string');
    if (!hasIdentity) {
        const candidates = monitors.filter(m => !m.hasBacklight);
        if (displays.length === 1 && candidates.length === 1) {
            byConnector.set(candidates[0].connector, displays[0]);
            used.add(displays[0]);
        }
    } else {
        for (const m of monitors) {
            const free = displays.filter(d => !used.has(d) &&
                norm(d.vendor) === norm(m.vendor) &&
                norm(d.model) === norm(m.product));
            const pick = free.length > 1
                ? free.find(d => norm(d.serial) === norm(m.serial)) ?? free[0]
                : free[0];
            if (pick) {
                byConnector.set(m.connector, pick);
                used.add(pick);
            }
        }
    }

    return {byConnector, unmatched: displays.filter(d => !used.has(d))};
}

/** Index of the monitor showing DDC display `display`, or -1 if unknown. */
export function findMonitorIndex(displays, display) {
    if (!display)
        return -1;
    const monitors = getMutterMonitors();
    const {byConnector} = matchDisplays(monitors, displays);
    return monitors.find(m => byConnector.get(m.connector) === display)?.index ?? -1;
}

/** Monitor (Mutter) entry for the logical monitor under the pointer. */
export function getPointerMonitor(monitors) {
    const logical = global.backend.get_current_logical_monitor();
    return monitors.find(m => m.logical === logical) ?? null;
}

/**
 * Show the brightness OSD. monitorIndex < 0 shows it on all monitors.
 * Shell >= 49 has showOne/showAll; older shells take an index in show().
 */
export function showOSD(monitorIndex, pct, iconName = 'display-brightness-symbolic') {
    const icon = Gio.Icon.new_for_string(iconName);
    const level = pct / 100.0;
    const osd = Main.osdWindowManager;
    if (monitorIndex >= 0 && osd.showOne)
        osd.showOne(monitorIndex, icon, null, level, 1);
    else if (monitorIndex < 0 && osd.showAll)
        osd.showAll(icon, null, level, 1);
    else
        osd.show(monitorIndex, icon, null, level, 1);
}
