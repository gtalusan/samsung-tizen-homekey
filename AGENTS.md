# AGENTS.md

## What this repo is

A single-purpose Tizen TPK that registers one **app-context key rule** on
Samsung Tizen 5.x TVs: the remote's Home key (`XF86Home`) launches a custom
launcher. It exists because the platform registers key rules only from TPK
manifests, never from WGT (web app) manifests.

## Invariants

- **Do not add features.** This package must stay minimal. Anything more
  belongs in the launcher or a separate service.
- **Keep the identity stable.** `package="HomeKeyRu1"` /
  `appid="HomeKeyRu1.HomeKey"` must not change for a TV where it is already
  installed; a new identity creates a second rule rather than updating the
  first.
- **Do not weaken the stub.** `bin/homekey-stub` is a static armv7 ELF that is
  never executed; it must remain a real, valid ELF for the capp install path.
- **The only supported manifest edit** is the rule's `<target>` app id.
- **No signing secrets in the repo.** The released tpk is unsigned; end users
  sideload it through Apps2Samsung (or sign it themselves).

## Build and verify

```sh
./build.sh
unzip -l homekey.tpk          # manifest + bin/homekey-stub + shared/res/homekey.png
```

A build is good when the tpk contains exactly: `tizen-manifest.xml`,
`bin/homekey-stub`, `shared/res/homekey.png`.

## Device verification checklist

1. Install through Apps2Samsung; the install must succeed.
2. Confirm registration on the TV (sdb / uid-901 shell):
   `sqlite3 /opt/dbspace/.app-context.db "SELECT * FROM key_filter WHERE key='XF86Home';"`
   — a row with `appid=HomeKeyRu1.HomeKey` and the launcher's package in the
   target must appear.
3. Press Home from an app and from an HDMI input: the launcher must take
   over (a brief Samsung Smart Hub flash is expected and platform-imposed).

## Rollback

Uninstall the package from the TV's app management UI; the rule disappears
with it and Home reverts to Samsung Home.

## Reference

QN55Q90TAFXZC / T-KNMAKUC-REL-202505100505 / Tizen 5.5.0 / armv7 — the device
this was developed and verified on.
