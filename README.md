# HomeKey

A minimal rule-carrier TPK for Samsung Tizen 5.x TVs that routes the remote's
**Home button** to a custom launcher (in our case, the CleanFrame fork).

## Why this exists

On Tizen 5.x retail firmware:

- The Home key (`XF86Home`) is routed by an **app-context key rule**, and the
  platform only registers such rules from **TPK manifests** — a WGT (web app)
  manifest is silently ignored.
- The rule's target can be *another app*, so this package exists only to carry
  one rule: `XF86Home → launch <launcher>`.

The stub binary never runs (the service is never launched); it only exists
because the manifest's `exec` must reference a file. A ~0-byte file may not
pass the installer's capp validation, so we ship a real static ELF.

## Install

Sideload the unsigned `homekey.tpk` (see `build.sh`) through
[Apps2Samsung](https://apps2samsung.com) (or sign it yourself — the rule
needs no special privilege level; a public signing is accepted).

## Build

```sh
./build.sh          # requires zig; produces homekey.tpk
```

## How it works

The manifest registers, at install time, into the platform's
`/opt/dbspace/.app-context.db` → `key_filter`:

```
HomeKeyRu1.HomeKey | XF86Home | launch | package:nClrkHome1.FrameHome
```

The target `package:` is the launcher's app id. To retarget, edit
`tizen-manifest.xml` (the `<target>` element) and reinstall.

The Samsung Home rule still fires alongside ours, so the Smart Hub flashes
briefly before the launcher takes over. Removing Samsung's own rule needs
platform-level access we don't have.

## Uninstall / rollback

Uninstall HomeKey from the TV's app management — the rule is removed with the
package and the Home button reverts to Samsung Home.

## Reference TV

QN55Q90TAFXZC, T-KNMAKUC-REL-202505100505, Tizen 5.5.0, armv7.
