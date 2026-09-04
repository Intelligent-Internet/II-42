# M672 Native Same-Query Contract

## Objective

M672 fixes the current native-matrix credibility blocker: a matrix is only a
promotion surface if every source is evaluated on the same query ids per
dataset.  Query count equality is not sufficient; query identity must match.

This stage does not claim a new model-quality gain.  It hardens the audit
contract so the next native DB/plugin benchmark cannot silently mix query
sets.

## Changes

- `scripts/evaluate_p1_native_atoms_pg.py` now writes `query_ids_sha256` in
  `summary` and a top-level `query_set`.
- `scripts/slice_native_eval_by_query_ids.py` recomputes the same fingerprint
  after slicing.
- `scripts/apply_m671_bm25_rescue_slot_admission.py` writes query fingerprints
  for replay-style M671 eval artifacts.
- `scripts/compile_m603_native_surface_matrix.py` now reports
  `query_identity_consistency` and warns when any source is missing or mismatched
  by query id.

## Evidence

### M671 Replay Contract

Artifact:
`runs/m672_native_same_query_contract_v1/m671_replay_identity_contract_matrix.json`

Result:

- Query-count consistency: `false`
- Query identity checked: `true`
- Query identity consistent: `false`
- Mismatched datasets: `cqadupstack`, `nfcorpus`, `quora`,
  `webis-touche2020`

This confirms the previous M671 matrix remains an interface check only.  M671
replay rows have a clean internal signal, but they cannot be compared directly
against the existing BM25/dense baseline rows because they use different query
sets.

### Native Shared15 Smoke Contract

Artifact:
`runs/m672_native_same_query_contract_v1/shared15_smoke_contract_matrix.json`

Result:

- Query-count consistency: `true`
- Query identity checked: `true`
- Query identity consistent: `true`
- Sources checked: `BM25`, `dense`, `M549U`, `P1-a010`, `P1-a0125`
- Datasets checked: `fiqa`, `msmarco`

This proves the native matrix path can produce same-query comparable rows when
the artifacts include per-query evidence.

## Current Limitation

The M670 rescue rule is still not a native DB/plugin scorer.  It is replayed
from M604 audit rows.  The next formal benchmark must either:

1. implement the M670 tail-admission rule in the native candidate path, or
2. export same-query native candidate rows for BM25/dense/P1/P1+M670 and compile
   only after query identity matches.

Until then, M670 remains a small, clean replay signal, not a promoted matrix
result.

## Next Breakthrough Point

The useful next step is M673: implement a native same-query M670 scorer path.
Do not train a richer scorer yet.  The deterministic rule has a no-harm
Recall/MAP signal; the blocker is whether that signal survives true native
same-query execution.

Acceptance for M673:

- Same-query identity must be `true` for every source and dataset.
- P1-a0125+M670 must improve Recall@100 and MAP@100 over P1-a0125.
- NDCG@10 and MRR@20 must not regress.
- The result must run through native DB/plugin artifacts, not offline
  full-corpus scan.

If M673 fails this gate, stop the M670 scorer route and return to first-stage
generated-posting/output-head work.
