# P1 Route Milestone: M549U + M601 Auditable Stage-2 Scorer

## Definition

P1 is the current frozen product/research route:

```text
text
-> frozen dense root
-> M549U active-locked output compiler
-> dense-equivalent first-stage score
-> deterministic BM25 z-blend scorer
-> final retrieval ranking
```

P1 is not a single unified encoder yet.  The first stage is the M549U
dense-equivalent encoder/compiler.  The second stage is an auditable scoring
layer:

```text
final_score = zscore(m549u_score) + alpha * zscore(bm25_score)
```

The frozen baseline is `alpha=0.10`.  The current fixed-alpha candidate is
`alpha=0.125`.

## Why This Is The Main Route

The core project question was whether we could preserve dense retrieval ability
through an encoder/compiler route before adding lexical/ranking optimization.
P1 answers that in two steps:

1. M549U preserves the dense-equivalent retrieval surface.
2. M601 reconstructs the prior successful BM25 second-stage improvement in an
   auditable, deterministic way.

This separates two claims that should not be conflated:

- **First-stage claim:** M549U can stand in for dense scoring.
- **Second-stage claim:** BM25 z-blend can improve final retrieval quality over
  dense alone.

The BM25 gain does not justify weakening M549U.  M549U remains the first-stage
contract.

## Frozen Components

### First Stage

- Name: `m549u_active_locked`
- Role: dense-equivalent output compiler.
- Source checkpoint:
  `/home/huoju/leask/runs/ii42-m549u-unified-output-compiler-v1/m549u_active_locked_numpy_broad10_seed5532/m549u_active_locked_numpy_broad10_seed5532.compiler.pt`
- Transform lineage: `tail768_1p025625`
- Contract: preserve dense-equivalent retrieval before BM25 or ranking fusion.

### Second Stage

- Name: M601 deterministic BM25 scorer.
- BM25 config: `k1=0.9`, `b=0.4`.
- Score normalization: per-query z-score over the full sampled document set.
- Baseline alpha: `0.10`.
- Fixed candidate alpha: `0.125`.
- Learned gate: tested, not promoted.

## Reproduction Surface

Remote runner:

```bash
cd /home/huoju/leask/dev/ii42-m327-work
USE_DOCKER=1 \
RUN_NAME=m601_stage2_gate_alpha_sweep_broad10_seed601 \
scripts/run_m601_stage2_audit_rebuild_spark.sh
```

Local scripts:

- `scripts/research_sae_m601_stage2_audit_rebuild.py`
- `scripts/run_m601_stage2_audit_rebuild_spark.sh`

Local report artifacts:

- `docs/research-sae/reports/m0600-m0699/ii42-m601-stage2-auditable-bm25-ranking-rebuild-report.md`
- `outputs/m601_stage2_audit_rebuild/m601_stage2_replay_alpha_sweep_broad10_seed601.md`
- `outputs/m601_stage2_audit_rebuild/m601_stage2_gate_alpha_sweep_broad10_seed601.md`

Raw JSON artifacts are intentionally kept local and ignored by git because the
formal gate JSON is large.  The local files are:

- `outputs/m601_stage2_audit_rebuild/m601_stage2_replay_alpha_sweep_broad10_seed601.json`
- `outputs/m601_stage2_audit_rebuild/m601_stage2_gate_alpha_sweep_broad10_seed601.json`

## Broad10 Results

### M601.1 Exact Replay

The fixed `alpha=0.10` replay exactly reproduced the prior M550 surface.

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 0.57263 | 0.43535 | 0.74803 | 0.65179 | 1.00000 |
| `m549u_active_locked` | 0.57279 | 0.43545 | 0.74802 | 0.65195 | 0.99525 |
| `bm25` | 0.40515 | 0.28117 | 0.58537 | 0.47993 | 0.28114 |
| `m549u_active_locked_bm25_zblend_a010` | 0.59045 | 0.44785 | 0.75494 | 0.66396 | 0.87669 |

### M601.2 Fixed Alpha Sweep

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

- `alpha=0.10` is the proven replay baseline.
- `alpha=0.125` is the best fixed candidate on heldout NDCG.
- The `alpha=0.125` improvement is small; it is useful but should remain a
  fixed-control candidate, not evidence for a learned ranker by itself.

### M601.3 Conservative Gate

The learned query-level gate used only generic candidate-set features:

- BM25 entropy/top-gap/variance/nonzero fraction.
- M549U entropy/top-gap/variance.
- BM25/M549U score correlation and top-100 overlap.

Forbidden inference features were not used:

- dataset id.
- document id.
- qrel-derived values.

Selected policy:

```json
{"candidate_alpha": 0.075, "direction": "lt", "fallback_alpha": 0.1, "feature": "m549u_entropy_top100", "kind": "feature_threshold", "threshold": 0.999774}
```

Macro result:

| Source | Split | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | O@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| fixed `a010` | all | 0.59045 | 0.44785 | 0.75494 | 0.66396 | 0.87669 |
| fixed `a0125` | all | 0.59196 | 0.44859 | 0.75479 | 0.66548 | 0.85120 |
| gate | all | 0.59086 | 0.44780 | 0.75479 | 0.66437 | 0.88959 |
| fixed `a010` | heldout | 0.59176 | 0.44956 | 0.75897 | 0.65787 | 0.87669 |
| fixed `a0125` | heldout | 0.59277 | 0.44973 | 0.75910 | 0.65832 | 0.85120 |
| gate | heldout | 0.59199 | 0.44916 | 0.75882 | 0.65819 | 0.88959 |

Decision:

- The gate improves heldout NDCG versus fixed `a010`.
- The gate loses heldout Recall versus fixed `a010`.
- The gate does not beat fixed `a0125`.
- Therefore M601.3 is not promoted.

## Training And Delivery Design

P1 should be delivered as a two-stage system:

1. Train or load M549U as the dense-equivalent first-stage compiler.
2. Build a BM25 lexical index over the same document corpus.
3. For each query, score candidates with M549U and BM25.
4. Normalize both score vectors per query.
5. Use fixed alpha fusion for final ranking.

Default delivery setting:

```text
alpha = 0.10
```

Experimental fixed candidate:

```text
alpha = 0.125
```

Do not ship the learned gate as the default route.

## Stop Boundary

P1 stops before a learned second-stage ranker because M601.3 did not satisfy
the recall-safe promotion gate.  The following should not proceed from this
milestone without new evidence:

- M601.4 bounded ranker.
- M601.5 broader validation of a learned gate/ranker.
- M602 distillation of Stage-2 into a unified encoder.

Broader validation is still valuable for fixed `alpha=0.10` and `alpha=0.125`,
but it should be framed as validation of the fixed P1 product shape, not as
promotion of a learned second-stage model.

## Final P1 Decision

P1 is frozen as:

```text
M549U active-locked encoder/compiler + deterministic BM25 zblend scorer
```

Stable baseline:

```text
alpha = 0.10
```

Current fixed candidate:

```text
alpha = 0.125
```

Unified single-model encoder remains a future P2/M602-style research direction,
not part of the P1 milestone.
