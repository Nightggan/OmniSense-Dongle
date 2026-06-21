/**
 * DS5Dongle Telemetry — Cloudflare Worker + D1
 *
 * Routes:
 *   POST /collect   — receive anonymous usage data from the Electron app
 *   GET  /stats     — return aggregated public stats (JSON, 1h cache)
 *   GET  /          — HTML stats dashboard
 *
 * Privacy:
 *   - No IP address is stored (CF-Connecting-IP is never read)
 *   - installation_id is a salted SHA-256 hash — irreversible
 *   - Only aggregated counts are exposed via /stats
 *
 * Deploy:
 *   1. Install Wrangler:  npm install -g wrangler
 *   2. Login:             wrangler login
 *   3. Create D1 db:      wrangler d1 create ds5-telemetry
 *      → copy the database_id into wrangler.toml
 *   4. Apply schema:      wrangler d1 execute ds5-telemetry --file=schema.sql
 *   5. Deploy:            wrangler deploy
 *   6. Update TELEMETRY_ENDPOINT in config-electron/src/main/telemetry/telemetry.ts
 */

const CORS = {
  'Access-Control-Allow-Origin':  '*',
  'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
  'Access-Control-Allow-Headers': 'Content-Type',
};

export default {
  async fetch(request, env) {
    const url = new URL(request.url);

    // CORS pre-flight
    if (request.method === 'OPTIONS') {
      return new Response(null, { status: 204, headers: CORS });
    }

    // ── POST /collect ──────────────────────────────────────────────────────────
    if (request.method === 'POST' && url.pathname === '/collect') {
      let body;
      try {
        body = await request.json();
      } catch {
        return new Response('Bad JSON', { status: 400, headers: CORS });
      }

      // Sanitise inputs — truncate to safe lengths, never store nulls
      const installation_id = String(body.installation_id ?? 'Unknown').slice(0, 36) || 'Unknown';
      const platform        = String(body.platform        ?? 'Unknown').slice(0, 30)  || 'Unknown';
      const os              = String(body.os              ?? 'Unknown').slice(0, 120) || 'Unknown';
      const version         = String(body.version         ?? 'Unknown').slice(0, 30)  || 'Unknown';
      const firmware        = String(body.firmware        ?? 'Unknown').slice(0, 60)  || 'Unknown';
      const now             = Date.now();

      await Promise.all([
        // Historical append log
        env.DB.prepare(
          'INSERT INTO stats (installation_id, platform, os, version, firmware, ts) VALUES (?, ?, ?, ?, ?, ?)',
        ).bind(installation_id, platform, os, version, firmware, now).run(),

        // Unique users — upsert keyed on installation_id
        env.DB.prepare(`
          INSERT INTO users (installation_id, platform, os, version, firmware, first_seen, last_seen)
          VALUES (?, ?, ?, ?, ?, ?, ?)
          ON CONFLICT(installation_id) DO UPDATE SET
            platform   = excluded.platform,
            os         = excluded.os,
            version    = excluded.version,
            firmware   = excluded.firmware,
            last_seen  = excluded.last_seen
        `).bind(installation_id, platform, os, version, firmware, now, now).run(),
      ]);

      return new Response('ok', { headers: CORS });
    }

    // ── GET /stats (JSON API) ──────────────────────────────────────────────────
    if (request.method === 'GET' && url.pathname === '/stats') {
      try {
        const [linuxDistros, windowsVersions, versions, firmwares, totalRow] = await Promise.all([
          // Linux distributions (users where platform = 'linux')
          env.DB.prepare(
            "SELECT os AS label, COUNT(*) AS nb FROM users WHERE platform = 'linux' GROUP BY os ORDER BY nb DESC LIMIT 30",
          ).all(),
          // Windows versions (users where platform = 'win32')
          env.DB.prepare(
            "SELECT os AS label, COUNT(*) AS nb FROM users WHERE platform = 'win32' GROUP BY os ORDER BY nb DESC LIMIT 20",
          ).all(),
          // App versions (all platforms)
          env.DB.prepare(
            'SELECT version AS label, COUNT(*) AS nb FROM users GROUP BY version ORDER BY nb DESC LIMIT 20',
          ).all(),
          // Firmware versions
          env.DB.prepare(
            'SELECT firmware AS label, COUNT(*) AS nb FROM users GROUP BY firmware ORDER BY nb DESC LIMIT 20',
          ).all(),
          // Total unique users
          env.DB.prepare('SELECT COUNT(*) AS nb FROM users').first(),
        ]);

        return Response.json(
          {
            total:            totalRow?.nb ?? 0,
            linux_distros:    linuxDistros.results,
            windows_versions: windowsVersions.results,
            versions:         versions.results,
            firmwares:        firmwares.results,
            generated_at:     new Date().toISOString(),
          },
          { headers: { ...CORS, 'Cache-Control': 'public, max-age=3600' } },
        );
      } catch (err) {
        return Response.json(
          { error: 'Internal error', message: String(err) },
          { status: 500, headers: CORS },
        );
      }
    }

    // ── GET / (HTML dashboard) ─────────────────────────────────────────────────
    if (request.method === 'GET' && url.pathname === '/') {
      return new Response(HTML_DASHBOARD, {
        headers: { 'Content-Type': 'text/html; charset=utf-8' },
      });
    }

    return new Response('Not found', { status: 404, headers: CORS });
  },
};

// Minimal inline dashboard — no external dependencies, DS5/DualSense branding
const HTML_DASHBOARD = `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>DS5Dongle — Anonymous Usage Stats</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: system-ui, sans-serif; background: #0d0d14; color: #C8C8C8;
         max-width: 960px; margin: 2rem auto; padding: 0 1rem 4rem; }
  h1   { color: #1a6dff; font-size: 1.6rem; margin-bottom: .3rem; }
  .subtitle { color: #888899; font-size: .9rem; margin-bottom: 1.8rem; }
  .subtitle a { color: #1a6dff; }
  h2   { color: #888899; font-size: .85rem; text-transform: uppercase;
         letter-spacing: .06em; margin-bottom: .8rem; }
  .counters { display: flex; gap: 1rem; margin-bottom: 2rem; flex-wrap: wrap; }
  .counter  { background: #13131f; border: 1px solid rgba(255,255,255,.07);
              border-radius: 8px; padding: 1rem 1.5rem; flex: 1; min-width: 140px; }
  .counter .val { font-size: 2rem; font-weight: bold; color: #1a6dff; }
  .counter .lbl { font-size: .8rem; color: #888899; margin-top: .2rem; text-transform: uppercase; }
  .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 1.5rem; margin-bottom: 1.5rem; }
  @media (max-width: 620px) { .grid { grid-template-columns: 1fr; } }
  .card { background: #13131f; border: 1px solid rgba(255,255,255,.07);
          border-radius: 8px; padding: 1.2rem; }
  table { width: 100%; border-collapse: collapse; }
  th, td { text-align: left; padding: .4rem .6rem; border-bottom: 1px solid #1a1a2a; }
  th   { color: #1a6dff; font-weight: 600; font-size: .85rem; }
  .nb  { text-align: right; color: #00d4aa; font-weight: bold; }
  .bar { display: inline-block; background: #1a6dff; height: 8px; border-radius: 3px; }
  footer { text-align: center; color: #888899; font-size: .8rem; margin-top: 2rem; }
  footer a { color: #1a6dff; }
  #error-msg { display: none; background: #1a0d0d; border: 1px solid #661a1a;
               color: #ff8080; border-radius: 6px; padding: .8rem 1rem;
               margin-bottom: 1.5rem; font-size: .9rem; }
</style>
</head>
<body>
<h1>DS5Dongle — Anonymous Usage Stats</h1>
<p class="subtitle">
  Anonymous and aggregated usage data shared voluntarily by DS5Dongle users.
  No personal data or IP address is stored. &nbsp;·&nbsp;
  <a href="https://github.com/loteran/DS5Dongle" target="_blank">GitHub</a>
</p>

<div id="error-msg">Could not load statistics. The endpoint may be unreachable.</div>

<div class="counters">
  <div class="counter">
    <div class="val" id="cnt-total">…</div>
    <div class="lbl">Unique users</div>
  </div>
  <div class="counter">
    <div class="val" id="cnt-linux">…</div>
    <div class="lbl">Linux distros</div>
  </div>
  <div class="counter">
    <div class="val" id="cnt-versions">…</div>
    <div class="lbl">App versions</div>
  </div>
  <div class="counter">
    <div class="val" id="cnt-updated">…</div>
    <div class="lbl">Last updated</div>
  </div>
</div>

<div class="grid">
  <div class="card">
    <h2>Linux Distributions</h2>
    <table id="tbl-linux"><tr><td>Loading…</td></tr></table>
  </div>
  <div class="card">
    <h2>Windows Versions</h2>
    <table id="tbl-windows"><tr><td>Loading…</td></tr></table>
  </div>
</div>

<div class="card" style="margin-bottom:1.5rem">
  <h2>App Versions</h2>
  <table id="tbl-versions"><tr><td>Loading…</td></tr></table>
</div>

<div class="card">
  <h2>Firmware Versions</h2>
  <table id="tbl-firmware"><tr><td>Loading…</td></tr></table>
</div>

<footer style="margin-top:2rem">
  Stats are cached for 1 hour &nbsp;·&nbsp; Data is anonymous — no personal data or IP address is stored. &nbsp;·&nbsp;
  <a href="https://github.com/loteran/DS5Dongle" target="_blank">DS5Dongle on GitHub</a>
</footer>

<script>
fetch('/stats')
  .then(function(r) {
    if (!r.ok) throw new Error('HTTP ' + r.status);
    return r.json();
  })
  .then(function(data) {
    document.getElementById('cnt-total').textContent    = data.total;
    document.getElementById('cnt-linux').textContent    = data.linux_distros.length;
    document.getElementById('cnt-versions').textContent = data.versions.length;
    document.getElementById('cnt-updated').textContent  =
      new Date(data.generated_at).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });

    function fillTable(id, rows) {
      if (!rows || !rows.length) {
        document.getElementById(id).innerHTML = '<tr><td style="color:#888899">No data yet</td></tr>';
        return;
      }
      var max = rows[0].nb || 1;
      document.getElementById(id).innerHTML =
        '<tr><th>Name</th><th class="nb">Count</th><th></th></tr>' +
        rows.map(function(r) {
          return '<tr><td>' + r.label + '</td>' +
            '<td class="nb">' + r.nb + '</td>' +
            '<td><span class="bar" style="width:' + Math.round(r.nb / max * 100) + 'px"></span></td></tr>';
        }).join('');
    }

    fillTable('tbl-linux',    data.linux_distros);
    fillTable('tbl-windows',  data.windows_versions);
    fillTable('tbl-versions', data.versions);
    fillTable('tbl-firmware', data.firmwares);
  })
  .catch(function(err) {
    console.error(err);
    document.getElementById('error-msg').style.display = 'block';
    ['cnt-total','cnt-linux','cnt-versions','cnt-updated'].forEach(function(id) {
      document.getElementById(id).textContent = '—';
    });
  });
</script>
</body>
</html>`;
