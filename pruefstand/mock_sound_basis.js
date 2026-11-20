// Simuliertes ESP32-RC-Sound (Firmware src/ESP32-RC-Sound, Platine V1/V2/V3).
// Startwerte aller Antworten kommen aus dem Code (build.py fuehrt
// buildConfigJson(), handleApiDebug() und handleApiSoundFiles() trocken aus).
// Die Platine (Hardware_Config) ist im Modul eine Einstellung; der
// Pruefstand startet je Variante mit V1, V2 oder V3.
PST.register('mock_sound_basis', function (variant, ctx) {
  var M = PST.autoMock(ctx, { alias: { wifi_always_on: 'always_on', wifi_auto_enable: 'auto_enable', wifi_auto_timeout: 'auto_timeout' } });
  var C = M.get('/api/config'), St = C.settings;
  var HW = { v1: 0, v2: 1, v3: 2 }[variant] || 0;
  var RC_NAMES = ['FrSky', 'FlySky', 'ELRS SBUS', 'Hott', 'ELRS CRSF', 'CRSF + Funk', 'CRSF aus Funk'];
  C.version = ctx.variant.version;
  St.hw_config = St.hw_config_pending = HW;
  var boot = { hw: HW, rc: St.rc_system, portb: St.portb_role };
  var apActive = true;
  var snd = PST.soundCore({ C: C, store: 'SD-Karte', total: 7948206080, version: C.version,
    pins: function () { return boot.hw < 2; }, sport: function () { return boot.portb === 2; } });
  // wie funkModeFromRc()/rcFromFunkMode() in config.h
  function funkMode(rc) { return rc === 5 ? 1 : rc === 6 ? 2 : 0; }
  function rcFromFunk(rc, m) { return m === 1 ? 5 : m === 2 ? 6 : (rc >= 4 && rc <= 6 ? 4 : rc); }
  var common = PST.common({
    name: function () { return C.wifi.device; }, addr: function () { return St.modul_adr; },
    funkRole: function () { return boot.rc === 5 ? 'Zentrale' : (boot.rc === 6 ? 'Teilnehmer' : null); },
    funkCfg: function () { return { id: C.funk.id, ch: C.funk.ch, pwr: C.funk.pwr, rate: C.funk.rate, back: !!C.funk.back }; },
    backupData: function () { return C; }
  });
  function debug() {
    var d = JSON.parse(JSON.stringify(M.get('/api/debug'))), t = Date.now() / 1000;
    d.channels = d.channels.map(function (x, i) { return i === 2 ? 172 + Math.round(800 * (1 + Math.sin(t / 2))) : 992; });
    d.pwm = d.pwm.map(function (p, i) { var us = boot.hw < 2 ? 1500 + (i === 0 ? Math.round(400 * Math.sin(t / 2)) : 0) : 0; return { us: us, pct: us ? Math.round((us - St.pwm_min) * 100 / (St.pwm_max - St.pwm_min)) : 0 }; });
    var ok = function (k) { return snd.fsize(C.sound_files[k]) ? 1 : 0; };
    d.wav = { loop: ok('loop'), shut: ok('shut'), start: ok('start'), s: d.wav.s.map(function (x, i) { return ok(String(i + 1)); }) };
    d.hw_config = boot.hw; d.portb_role = boot.portb;
    d.crsf_diag = { raw_bytes: Math.round(t * 1450) % 1e7, valid_frames: Math.round(t * 150) % 1e6, crc_errors: 0, device_pings: 3, param_reads: 41, param_writes: 2 };
    d.wifi = { active: apActive, bus_ok: true };
    if (boot.portb === 2) { d.sport.total_v = 11.82; d.sport.min_cell = 3.93; d.sport.cells = 3; d.sport.soc = 78;
      d.sport.packs[0] = { online: true, cells: 3, total: 11.82, min: 3.93, cv: [3.95, 3.94, 3.93, 0, 0, 0] }; }
    if (boot.portb === 3 || St.portc_gps) d.gps = { active: 1, speed: 12.4, sats: 9, fix: 1, baud: 9600, auto: 1, locked: 1, pinConflict: 0 };
    return d;
  }
  var routes = PST.merge({}, snd.routes);
  PST.merge(routes, {
    'GET /api/config': function () { St.hw_config = boot.hw; St.restart_pending = St.hw_config_pending !== boot.hw ? 1 : 0; C.funk.boot = funkMode(boot.rc); return PST.json(C); },
    'POST /api/config': function (q, b) {
      // Feldnamen wie handleApiConfigPost(): flach, Funk als funk_*
      var d = PST.parseBody(b), rest = {};
      Object.keys(d).forEach(function (k) {
        var f = /^funk_(.*)$/.exec(k);
        if (k === 'funk_mode') St.rc_system = rcFromFunk(St.rc_system, Math.min(2, +d[k]));
        else if (f) C.funk[f[1]] = +d[k];
        else rest[k] = d[k];
      });
      M.apply(JSON.stringify(rest));
      C.funk.mode = funkMode(St.rc_system);
      return PST.json({ ok: true });
    },
    '/api/debug': function () { return PST.json(debug()); },
    '/api/wifi_enable': function (q, b) { var d = PST.parseBody(b); apActive = d.on === true || d.on === 1 || d.on === '1' || d.on === 'true'; return PST.json({ ok: true, on: apActive }); }
  });
  function handle(m, path, q, b) {
    var r = common.handle(m, path, q, b); if (r) return r;
    var h = routes[m + ' ' + path] || routes[path];
    if (h) return h(q, b, m);
    return M.handle(m, path, q, b);
  }
  // ── Lua, Funktionsnamen wie in ESP32-RC-Sound.ino ──
  var BAUDS = [1, 9600, 19200, 38400, 57600, 115200, 230400, 420000], ROLE_BAUD = St.portb_baud_defaults || [];
  function hwText(h) { return ['V1 (GPIO 22,0,2,4)', 'V2 (GPIO 14,27,32,33)', 'V3 (Port A/B/C)'][Math.min(h, 2)]; }
  var CH = ['ek_channel', 'sbusgrp_ch0', 'sbusgrp_ch1'], MODE = ['ek_mode', 'sbusgrp_mode0', 'sbusgrp_mode1'];
  var funkCfg = { get id() { return C.funk.id; }, set id(v) { C.funk.id = v; }, get ch() { return C.funk.ch; }, set ch(v) { C.funk.ch = v; },
    get pwr() { return C.funk.pwr; }, set pwr(v) { C.funk.pwr = v; }, get rate() { return C.funk.rate; }, set rate(v) { C.funk.rate = v; },
    get back() { return !!C.funk.back; }, set back(v) { C.funk.back = v ? 1 : 0; } };
  var lua = PST.mergeLua({ get: {}, set: {}, views: {} }, snd.lua, {
    get: {
      luaGetModAdr: function () { return St.modul_adr; }, luaGetGroupAdr: function (a) { return St['mkan_adr' + a]; },
      luaGetBusChannel: function (a) { var c = St[CH[a]]; return c === 999 ? 255 : c; },
      luaGetWmMode: function (a) { var m = St[MODE[a]]; return m >= 10 && m <= 13 ? m - 9 : 0; },
      luaGetHwConfig: function () { return St.hw_config_pending; },
      luaGetPwm: function (a) { return Math.round((a ? St.pwm_max : St.pwm_min) / 16); },
      luaGetPortBRole: function () { return St.portb_role; }, luaGetPortBBaud: function () { return Math.max(0, BAUDS.indexOf(St.portb_baud)); },
      luaGetPortCGps: function () { return St.portc_gps ? 1 : 0; },
      luaGetWifiNow: function () { return apActive ? 1 : 0; }, luaGetWifiAuto: function () { return C.wifi.auto_enable; },
      luaGetWifiTimeout: function () { return C.wifi.auto_timeout; }, luaGetWifiAlways: function () { return C.wifi.always_on; }
    },
    set: {
      luaSetModAdr: function (a, v) { St.modul_adr = v; }, luaSetGroupAdr: function (a, v) { St['mkan_adr' + a] = v; },
      luaSetBusChannel: function (a, v) { St[CH[a]] = v === 255 ? 999 : Math.min(v, 15); },
      luaSetWmMode: function (a, v) { St[MODE[a]] = v ? v + 9 : 0; },
      luaSetHwConfig: function (a, v) { St.hw_config_pending = v; },
      luaSetPwm: function (a, v) { St[a ? 'pwm_max' : 'pwm_min'] = v * 16; },
      luaSetPortBRole: function (a, v) { if (v !== St.portb_role && ROLE_BAUD[v] && v !== 4) St.portb_baud = ROLE_BAUD[v]; St.portb_role = v; },
      luaSetPortBBaud: function (a, v) { St.portb_baud = BAUDS[v]; }, luaSetPortCGps: function (a, v) { St.portc_gps = v; },
      luaSetWifiNow: function (a, v) { apActive = !!v; }, luaSetWifiAuto: function (a, v) { C.wifi.auto_enable = v; },
      luaSetWifiTimeout: function (a, v) { C.wifi.auto_timeout = v; }, luaSetWifiAlways: function (a, v) { C.wifi.always_on = v; }
    },
    views: {
      luaShowVersion: function (it, v) { v.text = ctx.variant.version + ' ' + hwText(boot.hw); },
      luaShowHardware: function (it, v) { if (St.hw_config_pending !== boot.hw) { v.name = 'Hardware (aktiv)'; v.text = hwText(boot.hw) + ' (Neustart noetig!)'; } else v.text = hwText(boot.hw); },
      luaShowDeviceName: function (it, v) { v.text = C.wifi.device + ' (Web aendern)'; },
      luaShowSettingsFolder: function (it, v) { v.name = 'Einstellungen (' + RC_NAMES[Math.min(St.rc_system, 6)] + ')'; },
      luaShowRcSystem: function (it, v) { v.text = RC_NAMES[Math.min(St.rc_system, 6)] + ' (Web aendern)'; },
      luaHideSbusOnly: function (it, v) { if (boot.rc >= 4) v.hidden = true; },
      luaViewPwmScale: function (it, v) { if (St.hw_config_pending >= 2) v.hidden = true; },
      luaViewPortBBaud: function (it, v) { if (!St.portb_role) v.hidden = true; }
    }
  }, PST.funkLua(funkCfg, function () { return funkMode(St.rc_system); },
    function (v) { St.rc_system = rcFromFunk(St.rc_system, Math.min(v, 2)); C.funk.mode = funkMode(St.rc_system); }));
  // Ordner Funk: Rate/Rueckkanal haengen am RC-System (5 = Zentrale, 6 = Teilnehmer)
  lua.views.luaViewFunkSet = function (it, v) { if (!funkMode(St.rc_system)) v.hidden = true; };
  lua.views.luaViewFunkRate = function (it, v) { if (St.rc_system !== 5) v.hidden = true; };
  lua.views.luaViewFunkBack = function (it, v) { if (St.rc_system !== 6) v.hidden = true; };
  return {
    handle: handle, get: lua.get, set: lua.set, views: lua.views,
    deviceName: function () { return C.wifi.device; },
    reboot: function () { boot.hw = St.hw_config_pending; boot.rc = St.rc_system; boot.portb = St.portb_role; },
    hw: function () {
      return [['Platine', hwText(boot.hw)], ['RC-System (Start)', RC_NAMES[Math.min(boot.rc, 6)]], ['PWM-/Pin-Eingänge', boot.hw < 2 ? 'ja' : 'nein (V3)'],
        ['Sound-Speicher', 'SD-Karte'], ['Port C', boot.hw < 2 ? '–' : 'GPS']];
    }
  };
});
