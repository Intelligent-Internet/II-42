# SAE Research Artifact Archive

Updated: 2026-07-26

The public source tree contains durable reports and small test fixtures, not
hundreds of megabytes of generated experiment output. Raw evidence from the
repository snapshot below was frozen before removal from Git:

- source commit:
  `d4c38e66e75a3ef971b962b16cc6e09cdd5917f1`;
- artifact count: `1,401`;
- total bytes: `321,529,302`;
- archive manifest SHA-256:
  `50eff315e36899bed4f96560486ab68a7ac5b505bd2d557d445d70a629eaa65e`;
- checksum list SHA-256:
  `2cbeeeb048b5be99c74307627d6a5d4b5041b39305082c93ff99431b26e49a5e`.

The machine-readable [manifest](manifest.jsonl) maps every original path to an
artifact ID, SHA-256 digest, byte size, content type, experiment bucket, and
archive key. Formal conclusions remain under
[`docs/research-sae/reports`](../reports/README.md).

The manifest digest changed only because one retained report path moved into
the unified `docs/research-sae/` hierarchy; artifact identities, archive keys,
content digests, counts, and external bytes are unchanged.
Historical experiment code has a separate
[source archive](../source/README.md).

Maintainers store the exact archive at:

```text
/Volumes/Betty/II42-research-artifacts/
  d4c38e66e75a3ef971b962b16cc6e09cdd5917f1/
```

That path is operational storage, not a public release dependency. The
manifest is the durable public integrity record. A maintainer restoring an
artifact should select its `archive_key`, verify its `sha256`, and place active
work under `runs/`, which is intentionally ignored by Git.

The M1520 manifest entry records the retained path that existed at the frozen
source commit. It now lives only in the artifact and historical-source
archives. Formal reports remain in the source tree, including the M3xx route
reassessment under `docs/research-sae/reports/m0300-m0399/`.

Historical runners must write generated JSON, JSONL, checkpoints, and logs
under `runs/` or another explicitly ignored output root. Product and release
tests must not depend on the external archive.
