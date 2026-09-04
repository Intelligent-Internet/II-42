# M601 Stage-2 Auditable BM25 Ranking Rebuild Report

## Status

M601 rebuilds the second-stage BM25/ranking surface on top of the fixed
M549U first-stage encoder/compiler.  The first-stage M549U dense-equivalent
contract is unchanged.

Current state:

- M601.0 audit manifest: implemented in
  `scripts/research_sae_m601_stage2_audit_rebuild.py`.
- M601.1 exact replay: passed on broad10.
- M601.2 fixed alpha sweep: completed on broad10.
- M601.3 conservative gate: completed on broad10; not promoted.
- M601.4/M601.5/M602: stopped for this branch because M601.3 did not beat
  fixed controls under the task-macro promotion gate.

## Run Surface

- Remote host: `spark-1`
- Run name: `m601_stage2_replay_alpha_sweep_broad10_seed601`
- Shared root:
  `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks`
- M549U compiler checkpoint:
  `/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/m549u_active_locked_numpy_broad10_seed5532/m549u_active_locked_numpy_broad10_seed5532.compiler.pt`
- Local artifacts:
  `outputs/m601_stage2_audit_rebuild/m601_stage2_replay_alpha_sweep_broad10_seed601.json`
  and
  `outputs/m601_stage2_audit_rebuild/m601_stage2_replay_alpha_sweep_broad10_seed601.md`
- M601.3 artifacts:
  `outputs/m601_stage2_audit_rebuild/m601_stage2_gate_alpha_sweep_broad10_seed601.json`
  and
  `outputs/m601_stage2_audit_rebuild/m601_stage2_gate_alpha_sweep_broad10_seed601.md`
- BM25 config: `k1=0.9`, `b=0.4`
- Score normalization:
  `zscore(first_stage_scores) + alpha * zscore(bm25_scores)` per query
  over the full sampled document set.
- Alpha grid:
  `0.00, 0.025, 0.05, 0.075, 0.10, 0.125, 0.15, 0.175, 0.20`

## M601.1 Exact Replay

The required fixed `alpha=0.10` replay passed.

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 0.57263 | 0.43535 | 0.74803 | 0.65179 | 1.00000 |
| `m549_tail768` | 0.57279 | 0.43545 | 0.74802 | 0.65195 | 0.99525 |
| `m549u_active_locked` | 0.57279 | 0.43545 | 0.74802 | 0.65195 | 0.99525 |
| `bm25` | 0.40515 | 0.28117 | 0.58537 | 0.47993 | 0.28114 |
| `m549_bm25_zblend_a010` | 0.59045 | 0.44785 | 0.75494 | 0.66396 | 0.87669 |
| `m549u_active_locked_bm25_zblend_a010` | 0.59045 | 0.44785 | 0.75494 | 0.66396 | 0.87669 |

This matches the target M550 surface:

```text
NDCG@10 0.59045 / MAP@100 0.44785 / Recall@100 0.75494 / MRR@20 0.66396
```

## M601.2 Alpha Sweep

Heldout winner was fixed `alpha=0.125`.

| Alpha | Heldout NDCG@10 | Heldout Recall@100 | Heldout MRR@20 | All NDCG@10 | All Recall@100 | All MRR@20 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.000 | 0.57729 | 0.75303 | 0.65034 | 0.57278 | 0.74802 | 0.65195 |
| 0.025 | 0.58394 | 0.75499 | 0.65249 | 0.58008 | 0.75050 | 0.65493 |
| 0.050 | 0.58784 | 0.75714 | 0.65758 | 0.58490 | 0.75267 | 0.65818 |
| 0.075 | 0.59129 | 0.75816 | 0.66097 | 0.58891 | 0.75402 | 0.66211 |
| 0.100 | 0.59176 | 0.75897 | 0.65787 | 0.59045 | 0.75494 | 0.66396 |
| 0.125 | 0.59277 | 0.75910 | 0.65832 | 0.59196 | 0.75479 | 0.66548 |
| 0.150 | 0.59175 | 0.75981 | 0.65655 | 0.59179 | 0.75487 | 0.66425 |
| 0.175 | 0.59163 | 0.75909 | 0.65795 | 0.59126 | 0.75445 | 0.66397 |
| 0.200 | 0.59029 | 0.75825 | 0.65536 | 0.59010 | 0.75361 | 0.66319 |

Interpretation:

- `alpha=0.125` improves all-query NDCG by `+0.00151` versus `alpha=0.10`.
- `alpha=0.125` improves heldout NDCG by `+0.00101` versus `alpha=0.10`.
- Recall is effectively flat: all-query Recall changes from `0.75494` to
  `0.75479`; heldout Recall changes from `0.75897` to `0.75910`.
- The gain is real enough to justify M601.3, but too small to justify a free
  ranker or M602 unification.

## M601.3 Conservative Gate

The evaluator now supports query-level rows and a conservative selector.
The formal broad10 run completed with `8815` qrel query rows.

Allowed inference features:

- `bm25_entropy_top100`
- `bm25_nonzero_frac`
- `bm25_top_gap`
- `bm25_variance`
- `score_correlation`
- `top100_overlap`
- `m549u_entropy_top100`
- `m549u_top_gap`
- `m549u_variance`

Forbidden inference features:

- dataset id
- document id
- qrel-derived values

Policy family:

```text
single global feature threshold -> candidate alpha
otherwise fallback to alpha=0.10
```

Training uses train query groups only.  Heldout is used only for promotion.

Selected policy:

```json
{"candidate_alpha": 0.075, "direction": "lt", "fallback_alpha": 0.1, "feature": "m549u_entropy_top100", "kind": "feature_threshold", "threshold": 0.999774}
```

Macro comparison:

| Source | Split | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | O@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| fixed `a010` | all | 0.59045 | 0.44785 | 0.75494 | 0.66396 | 0.87669 |
| fixed `a0125` | all | 0.59196 | 0.44859 | 0.75479 | 0.66548 | 0.85120 |
| gate | all | 0.59086 | 0.44780 | 0.75479 | 0.66437 | 0.88959 |
| fixed `a010` | heldout | 0.59176 | 0.44956 | 0.75897 | 0.65787 | 0.87669 |
| fixed `a0125` | heldout | 0.59277 | 0.44973 | 0.75910 | 0.65832 | 0.85120 |
| gate | heldout | 0.59199 | 0.44916 | 0.75882 | 0.65819 | 0.88959 |

Promotion condition:

- heldout NDCG must beat fixed `alpha=0.10`;
- heldout Recall must not drop versus fixed `alpha=0.10`;
- all-query MRR must not drop versus fixed `alpha=0.10`;
- if it cannot beat fixed `alpha=0.125`, keep fixed `alpha=0.125` as the
  second-stage candidate and do not promote learned gate.

Result:

- Heldout NDCG beats fixed `a010`: `0.59199 > 0.59176`.
- All-query MRR beats fixed `a010`: `0.66437 > 0.66396`.
- Heldout Recall does not pass: `0.75882 < 0.75897`.
- Gate does not beat fixed `a0125`: heldout NDCG `0.59199 < 0.59277`.

Therefore M601.3 is useful evidence but does not pass promotion.  The
M601.4 bounded ranker is not authorized from this result.

## Decision

Fixed replay succeeded.  Alpha sweep found a small fixed-control improvement at
`alpha=0.125`.  The conservative learned gate did not satisfy the task-macro
promotion gate and did not beat the fixed `alpha=0.125` control.

The best current auditable product shape is:

```text
M549U active-locked encoder/compiler + fixed BM25 zblend
```

with `alpha=0.10` as the proven replay baseline and `alpha=0.125` as the
current fixed-alpha candidate.

M602 unified model is not justified yet.
