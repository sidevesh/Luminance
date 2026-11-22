import Adw from 'gi://Adw';
import Gio from 'gi://Gio';
import {ExtensionPreferences} from 'resource:///org/gnome/Shell/Extensions/js/extensions/prefs.js';

export default class LuminancePreferences extends ExtensionPreferences {
    fillPreferencesWindow(window) {
        const settings = this.getSettings();
        window._settings = settings;

        const page = new Adw.PreferencesPage();
        window.add(page);

        const keys = new Adw.PreferencesGroup({
            title: 'Brightness keys',
            description: 'Keys change the display under the mouse pointer',
        });
        page.add(keys);

        const override = new Adw.SwitchRow({
            title: 'Handle brightness keys',
            subtitle: 'Use DDC/CI for displays GNOME cannot control',
        });
        settings.bind('override-brightness-keys', override, 'active', Gio.SettingsBindFlags.DEFAULT);
        keys.add(override);

        const step = Adw.SpinRow.new_with_range(1, 25, 1);
        step.set({
            title: 'Step size',
            subtitle: 'Percent per key press',
        });
        settings.bind('brightness-step', step, 'value', Gio.SettingsBindFlags.DEFAULT);
        keys.add(step);
    }
}
