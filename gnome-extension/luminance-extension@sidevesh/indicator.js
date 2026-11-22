import Clutter from 'gi://Clutter';
import GObject from 'gi://GObject';
import Shell from 'gi://Shell';
import St from 'gi://St';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import * as PanelMenu from 'resource:///org/gnome/shell/ui/panelMenu.js';
import * as PopupMenu from 'resource:///org/gnome/shell/ui/popupMenu.js';
import {Slider} from 'resource:///org/gnome/shell/ui/slider.js';

import {APP_ID} from './client.js';
import {getMutterMonitors, getNativeScale, matchDisplays} from './monitors.js';

const ICON_NAME = 'display-brightness-symbolic';
const INACTIVE_OPACITY = 110;

/** One display in the popup: a name/percentage header and a slider. */
class DisplayRow {
    constructor(section, name, value, sensitive, note = null) {
        this.header = new PopupMenu.PopupBaseMenuItem({reactive: false});
        this.header.add_style_class_name('luminance-display-header');
        this._nameLabel = new St.Label({
            text: name,
            x_expand: true,
            y_align: Clutter.ActorAlign.CENTER,
        });
        this._valueLabel = new St.Label({
            text: note ?? '',
            style_class: 'luminance-display-value',
            y_align: Clutter.ActorAlign.CENTER,
        });
        this.header.add_child(this._nameLabel);
        this.header.add_child(this._valueLabel);

        this.slider = new Slider(value ?? 0);
        this.slider.accessible_name = name;
        this.slider.reactive = sensitive;
        this.sliderItem = new PopupMenu.PopupBaseMenuItem({reactive: false});
        this.sliderItem.add_child(new St.Bin({
            style_class: 'slider-bin',
            child: this.slider,
            reactive: sensitive,
            can_focus: sensitive,
            x_expand: true,
            y_align: Clutter.ActorAlign.CENTER,
        }));

        if (!sensitive) {
            this.header.add_style_class_name('luminance-unsupported');
            this.sliderItem.add_style_class_name('luminance-unsupported');
        }

        section.addMenuItem(this.header);
        section.addMenuItem(this.sliderItem);

        if (sensitive)
            this._showValue(value);
    }

    _showValue(value) {
        this._valueLabel.text = `${Math.round(value * 100)}%`;
    }

    /** Set value from outside without triggering onChange. */
    setValue(value) {
        if (this.slider._grab)
            return;
        this._blocked = true;
        this.slider.value = value;
        this._blocked = false;
        this._showValue(value);
    }

    onChange(callback) {
        this.slider.connect('notify::value', () => {
            this._showValue(this.slider.value);
            if (!this._blocked)
                callback(this.slider.value);
        });
    }
}

/** Top-bar button with a per-monitor brightness popup. */
export const LuminanceIndicator = GObject.registerClass(
class LuminanceIndicator extends PanelMenu.Button {
    _init(extension, client) {
        super._init(0.5, 'Luminance');
        this._extension = extension;
        this._client = client;
        this._rows = new Map(); // row key -> DisplayRow
        this._rowsKey = null;

        this._icon = new St.Icon({
            icon_name: ICON_NAME,
            style_class: 'system-status-icon',
        });
        this.add_child(this._icon);

        this._section = new PopupMenu.PopupMenuSection();
        this.menu.addMenuItem(this._section);
        this.menu.addMenuItem(new PopupMenu.PopupSeparatorMenuItem());
        this.menu.addAction('Open Luminance', () => this._openApp());
        this.menu.addAction('Settings', () => this._extension.openPreferences());

        this.menu.connect('open-state-changed', (m, open) => {
            if (open)
                this._client.refresh();
        });
        global.backend.get_monitor_manager().connectObject(
            'monitors-changed', () => this._sync(), this);
        Main.brightnessManager?.connectObject('changed', () => this._sync(), this);
        this._client.connectObject(
            'changed', () => this._sync(),
            'brightness-changed', (c, id, pct) => this._rows.get(`ddc:${id}`)?.setValue(pct / 100),
            this);

        this.connect('destroy', () => this._onDestroy());
        this._sync();
    }

    _openApp() {
        const app = Shell.AppSystem.get_default().lookup_app(`${APP_ID}.desktop`);
        if (!app) {
            Main.notifyError('Luminance', 'Luminance app is not installed');
            return;
        }
        app.activate();
    }

    /** Describe the rows the popup should show, in order. */
    _describeRows() {
        const monitors = getMutterMonitors();
        const {byConnector, unmatched} = matchDisplays(monitors, this._client.displays);
        const rows = [];

        for (const m of monitors) {
            const ddc = byConnector.get(m.connector);
            const native = getNativeScale(m.logical);
            if (ddc)
                rows.push({key: `ddc:${ddc.id}`, name: ddc.name, ddc});
            else if (native)
                rows.push({key: `native:${m.connector}`, name: native.name || m.displayName, native});
            else
                rows.push({key: `none:${m.connector}`, name: m.displayName || m.connector});
        }
        // DDC displays we could not place on a monitor still get a slider.
        for (const ddc of unmatched)
            rows.push({key: `ddc:${ddc.id}`, name: ddc.name, ddc});

        return rows;
    }

    _sync() {
        const rows = this._describeRows();
        const hasDdc = rows.some(r => r.ddc);
        this._icon.opacity = hasDdc ? 255 : INACTIVE_OPACITY;

        const key = rows.map(r => r.key).join('|');
        if (key === this._rowsKey) {
            // Same layout: update values in place so an open popup is not rebuilt.
            for (const r of rows) {
                if (r.ddc)
                    this._rows.get(r.key)?.setValue(r.ddc.brightness / 100);
            }
            return;
        }
        this._rowsKey = key;

        for (const s of this._nativeScales ?? [])
            s.disconnectObject(this);
        this._nativeScales = [];
        this._section.removeAll();
        this._rows.clear();

        if (rows.length === 0) {
            const item = new PopupMenu.PopupMenuItem('No displays found', {reactive: false});
            item.add_style_class_name('luminance-unsupported');
            this._section.addMenuItem(item);
            return;
        }

        for (const r of rows) {
            let row;
            if (r.ddc) {
                const id = r.ddc.id;
                row = new DisplayRow(this._section, r.name, r.ddc.brightness / 100, true);
                row.onChange(v => this._client.setBrightness(id, v * 100));
            } else if (r.native) {
                const scale = r.native;
                row = new DisplayRow(this._section, r.name, scale.value, true);
                row.onChange(v => {
                    scale.value = v;
                });
                scale.connectObject('notify::value', () => row.setValue(scale.value), this);
                this._nativeScales.push(scale);
            } else {
                row = new DisplayRow(this._section, r.name, 0, false, 'Not supported');
            }
            this._rows.set(r.key, row);
        }
    }

    _onDestroy() {
        for (const s of this._nativeScales ?? [])
            s.disconnectObject(this);
        this._nativeScales = [];
        global.backend.get_monitor_manager().disconnectObject(this);
        Main.brightnessManager?.disconnectObject(this);
        this._client.disconnectObject(this);
    }
});
