# samsung-tizen-homekey

A minimal rule-carrier TPK for Samsung **Tizen 5.x** TVs that routes the
remote's **Home button** to a custom launcher. One rule, one package, nothing
else: `XF86Home → launch <your launcher>`.

Current package version: **1.0.0** (see `tizen-manifest.xml`).

## Tested on

| Device | Firmware build | Platform |
| --- | --- | --- |
| Samsung QN55Q90TAFXZC (2020 Q90T) | `T-NKMAKUC-REL-202505100505` | Tizen 5.5.0, armv7 |

Verified: the rule registers in `/opt/dbspace/.app-context.db` →
`key_filter` at install time, and the Home button opens the launcher from
apps and inputs. Samsung's own Home rule fires alongside ours, so the Smart
Hub flashes briefly first.

## Why this package exists

On Tizen 5.x retail firmware, the Home key is routed by an **app-context key
rule**, and the platform registers such rules **only from TPK manifests** —
the same block in a WGT (web app) manifest is silently ignored. The rule's
target can be any app, so this package exists solely to carry the rule.

The stub binary is never executed (the service is never launched); it exists
because the manifest's `exec` must reference a real file, and the installer
validates the capp type.

## Build

Requires [zig](https://ziglang.org) (`brew install zig`).

```sh
./build.sh          # produces homekey.tpk (unsigned)
```

## Install

Sideload `homekey.tpk` through [Apps2Samsung](https://apps2samsung.com), which
signs and installs it. A public-level signing is sufficient — the rule needs
no special privilege.

## Retarget to your launcher

Edit the `<target>` element in `tizen-manifest.xml`:

```xml
<target selector="app" request="launch" args="entry_point:home">YOUR.APP.ID</target>
```

Rebuild and reinstall. Keep the package/app identity stable once installed.

## Uninstall / rollback

Uninstall HomeKey from the TV's app management (or `pkgcmd` through the app
framework's path — not as the sdk user). The rule is removed with the package
and the Home button reverts to Samsung Home.

## For AI agents

Read `AGENTS.md` first. The invariants that matter: this package's only job is
the one key rule; the identity `HomeKeyRu1.HomeKey` must stay stable for a
given TV; and changing the rule's target is the only supported edit.
