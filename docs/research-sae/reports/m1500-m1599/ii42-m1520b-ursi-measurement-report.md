# M1520B URSI Measurement Report

Date: 2026-07-10

Decision: pass the measurement-contract stage and proceed to M1520C source
capacity work. This report does not authorize training or claim URSI quality.

## Delivered Surface

M1520B adds a representation-agnostic exact posting evaluator:

- `scripts/audit_m1520_ursi_posting_surface.py`;
- `scripts/verify_m1520_ursi_baseline_manifest.py`;
- `docs/research-sae/reports/m1500-m1599/ii42-m1520-ursi-measurement-contract.md`;
- `tests/fixtures/research/ii42-m1520-ursi-baseline-manifest.json`;
- focused runtime and manifest tests.

The evaluator accepts new concept/route keys and existing II42 `atom_ids`
artifacts. It applies document/query TopK, builds a real inverted index, and
enumerates the exact union of reached posting lists for every query.

It reports index DF/head statistics, exact touch mean/p95, touched postings,
semantic quality, and dense-recoverable BM25-miss recovery. Qrels are read only
after postings, touched sets, and semantic rankings have been constructed.

## Frozen Evidence

Commit `f13658b8` closes M1510-M1518. The baseline manifest pins SHA-256 and
size for the prior sparse controls, P1.3 native shared15 matrix, and the three
locked canary BM25/dense baselines and dense candidate rankings.

The manifest also records the remaining M1520C prerequisite: exact BM25
ranking JSONL must be exported for each canary. Existing baseline JSON contains
per-query metrics but not ordered BM25 document IDs, so it cannot prove
BM25-miss residual recovery by itself.

## Real Artifact Compatibility Smoke

The CLI was run on the existing M1137 shared15 NFCorpus atom export. This is a
measurement compatibility check, not a URSI result.

| Metric | Observed |
| --- | ---: |
| Documents | 2,063 |
| Queries | 100 |
| Active keys | 2,549 / 4,096 |
| Postings/document mean | 33.0776 |
| Max DF ratio | 0.462918 |
| Head 1% posting share | 0.221252 |
| Query active mean / p95 | 2.32 / 5.00 |
| Exact touch mean / p95 | 0.164028 / 0.507780 |
| Touched postings mean / p95 | 403.62 / 1,274.35 |

Output artifacts:

- `runs/m1520_ursi_measurement_smoke_v1/`

The run demonstrates that average touch alone is insufficient: the old surface
has a modest 16.4% mean but a 50.8% p95. M1520C therefore keeps both mean and
p95 as hard evidence and cannot promote a source from mean touch alone.

## Verification

- Focused unit tests pass.
- The baseline manifest hashes all pinned local artifacts at runtime.
- Python compilation passes.
- Ruff passes.
- A real JSONL CLI run writes JSON, Markdown, input signatures, and per-query
  cost rows.
- `git diff --check` passes.

## Next Gate

M1520C must now produce a qrels-free, non-routed corpus vocabulary with an
explicit non-indexed background channel. Before evaluating capacity, it must
export ordered BM25 candidates and lock semantic document/query artifacts.

No contextual route, loss-weight sweep, or large training run is authorized
until the fixed non-routed capacity surface is measured on the three canaries.
