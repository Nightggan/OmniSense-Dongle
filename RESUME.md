# RESUME — état des chantiers DS5Dongle (reprise multi-machine)

> Fichier de reprise pour ouvrir une session Claude depuis **n'importe quelle machine**
> (notamment le PC Windows) et continuer là où on en est. Mis à jour à la main.
> Dernière mise à jour : 2026-06-21.

Deux chantiers **cloisonnés** sur des branches séparées — **ne pas les mélanger**.

---

## 1. Suppression de VoiceMeeter sous Windows (loopback WASAPI)

**Branche : `windows-develop`** (cette branche).

**Objectif** : supprimer la dépendance à VoiceMeeter pour les auto-haptics sous
Windows. Au lieu de demander d'installer/configurer VoiceMeeter, l'appli
`config-electron` capturerait le **loopback WASAPI de la sortie par défaut
courante** (casque / HDMI / autre, détectée dynamiquement) et la mirrorerait vers
la carte son USB du dongle. La sortie par défaut reste intacte (duplication, pas
de reroutage). **Linux est déjà couvert par `pw-loopback` — NE PAS Y TOUCHER.**

**Format dongle** (`src/usb_descriptors.cpp`) : 48 kHz / 16-bit, interface
principale 4ch (`out:4`), alt stéréo 2ch dispo. WASAPI shared mode auto-resample.

**Fait** : POC Node dans `poc-win-audio/` (lib `audify`/RtAudio ; loopback =
ouvrir un flux *input* sur un device *output* en WASAPI). Détection dynamique
default-out + dongle OK, `getStreamLatency()` exposé. Validé structurellement sur
Linux (chiffres non représentatifs : ALSA default gonfle à ~67 ms).

**Reste à faire (sur Windows) :**
```powershell
cd poc-win-audio
npm install
node loopback-poc.js          # ou: node loopback-poc.js "controller"
# tuner la latence: set FRAME=240 && node loopback-poc.js
```
- Mesurer la **latence software** réelle en WASAPI (FRAME plus petit).
- Mesurer la **latence physique** son→vibration, comparer au ressenti VoiceMeeter.
- Si comportement bizarre : tester `CHANNELS=4` (interface principale 4ch).
- Si viable → intégrer le moteur dans `config-electron`, piloté par le hotplug
  existant (`src/main/hid/hotplug.ts`), activé seulement si dongle présent (éviter
  la boucle de feedback).
- macOS resterait dépendant d'un driver tiers (BlackHole).

---

## 2. Télémétrie anonyme opt-in (cross-platform Win + Linux)

**Branche : `feature/telemetry`** (poussée sur origin, commit `6799cb8`, build OK).
Calquée sur ASM. Montre : utilisateurs uniques, distro Linux **ou** version
Windows (champ `platform` pour séparer), version appli + version firmware.

**Fait (13 fichiers) :**
- `config-electron/src/main/telemetry/telemetry.ts` — client opt-in. machine-id
  haché SHA-256 salé (Linux `/etc/machine-id`, Windows `MachineGuid` registre,
  macOS `ioreg`), label OS (Linux `PRETTY_NAME` / Windows `os.version()`+build),
  rate-limit quotidien, fire-and-forget, **garde `PLACEHOLDER`** (aucun envoi tant
  que l'endpoint n'est pas configuré).
- `config-electron/src/renderer/components/sections/SettingsSection.tsx` — toggle
  réglages (React 19 + Zustand, `ToggleRow`).
- Dialog natif de consentement au 1er lancement (`main/index.ts`) ; envoi déclenché
  à la détection dongle avec firmware via `readFirmwareVersion()`.
- `cloudflare/worker.js` + `schema.sql` + `wrangler.toml` (name `ds5-telemetry`).
- `docs/stats/index.html` — page stats publique (Chart.js, branding DualSense).

**Reste à faire :**
1. Déployer le Worker :
   ```bash
   cd cloudflare
   wrangler d1 create ds5-telemetry          # → mettre database_id dans wrangler.toml
   wrangler d1 execute ds5-telemetry --file=schema.sql
   wrangler deploy
   ```
2. Remplacer `https://ds5-telemetry.PLACEHOLDER.workers.dev/collect` dans **2 fichiers** :
   - `config-electron/src/main/telemetry/telemetry.ts` (`TELEMETRY_ENDPOINT`)
   - `docs/stats/index.html` (`STATS_URL`)
3. Tester sur Windows :
   ```powershell
   git checkout feature/telemetry
   cd config-electron
   npm install
   npm run dev          # ou npm run build:win pour un installeur NSIS
   ```
   - **Niveau 1** (sans déploiement) : vérifier le dialog de consentement au 1er
     lancement + le toggle réglages. L'envoi reste bloqué par la garde PLACEHOLDER.
   - **Niveau 2** (bout-en-bout, après déploiement + remplacement de l'URL) :
     accepter le consentement → le ping remonte, visible sur le dashboard du worker
     (`/`) et la page stats.
4. Quand validé → merge sur `master`.

---

## Branches (rappel)

| Branche | Chantier | Plateformes | État |
|---|---|---|---|
| `windows-develop` | Loopback WASAPI (anti-VoiceMeeter) | Windows | POC à mesurer |
| `feature/telemetry` | Télémétrie opt-in | Win + Linux | build OK, à déployer + tester |
| `master` | Ligne stable (release `v1.2.19-autohaptics`) | — | rumble jeu résolu |

**Note `gh`** : `gh` pointe par défaut sur l'upstream `awalol/DS5Dongle` →
toujours utiliser `--repo loteran/DS5Dongle`.
