# M676 M675 Seed 6547 First-Stage Candidate

## Scope

M676 freezes the best M675 first-stage dense-equivalence candidate as the
current research baseline.

This is a documentation and manifest milestone only.  It does not change the
training objective, scorer, BM25 path, reranker, learned gate, or document
posting/index geometry.

## Candidate

- candidate id: `m675_full_shared15_m674_guard_seed6547`
- source commit: `0b7f28b7a2eb46edf7e36dbc617deb9c925d9fd7`
- checkpoint:
  `runs/m675_full_shared15_m674_guard_v1/m675_full_shared15_m674_guard_seed6547/m675_full_shared15_m674_guard_seed6547.query_compiler.pt`
- source JSON:
  `runs/m675_full_shared15_m674_guard_v1/m675_full_shared15_m674_guard_seed6547/m675_full_shared15_m674_guard_seed6547.json`
- source report:
  `docs/research-sae/reports/m0600-m0699/ii42-m675-full-shared15-m674-guard-seed6547-report.md`
- aggregate report:
  `docs/research-sae/reports/m0600-m0699/ii42-m675-full-shared15-m674-guard-report.md`

## Frozen Configuration

The candidate keeps the M674 guard configuration:

```text
--all-query-train-fraction 0.40
--all-query-dev-fraction 0.10
--emit-full-eval
--output-blend-scale 0.25
--output-lock-active
--output-preserve-dense-top-k-values 50,100,256
--output-preserve-p1-top-k-values 10,20,50,100,1000
--relative-support-gate
--support-cosine-floor 0.999
--support-regression-tolerance 0.00001
```

Training remains first-stage only:

- frozen baseline: `P1.3 / M549U native signed-dot`
- frozen document posting/index geometry
- query-side/output compiler only
- no BM25 in training or selection
- no reranker
- no learned gate
- no qrels-driven objective

## Why Seed 6547

All four M675 seeds pass dense-equivalence style guards on the full shared15
query surface: no O@10/O@50/O@100 regression, no Recall@100 regression, and no
CUB regression.

Seed `6547` is selected because it is the only M675 seed with positive full
shared15 MAP@100, NDCG@10, and MRR@20 while also passing all dense-equivalence
guards.

## Held-Out Test Deltas

The `test` split is the broad50 held-out split with `671` queries.

| Metric | Delta vs P1 |
| --- | ---: |
| dense overlap@10 | `0.000000000` |
| dense overlap@50 | `0.000000000` |
| dense overlap@100 | `0.000000000` |
| dense overlap@256 | `+0.000101568` |
| Recall@100 | `0.000000000` |
| CUB | `0.000000000` |
| MAP@100 | `+0.000371414` |
| NDCG@10 | `+0.000335251` |
| MRR@20 | `+0.000740741` |
| support cosine | `-0.000000159` |

Decision status: `dense_equivalence_gate_passed`.

Selected checkpoint: epoch `5`, global step `45`.

## Full Shared15 Deltas

The `full` split evaluates all `1342` shared15 qrels queries.

| Metric | Delta vs P1 |
| --- | ---: |
| dense overlap@10 | `0.000000000` |
| dense overlap@50 | `0.000000000` |
| dense overlap@100 | `0.000000000` |
| dense overlap@256 | `+0.000084287` |
| Recall@100 | `0.000000000` |
| CUB | `0.000000000` |
| MAP@100 | `+0.000164443` |
| NDCG@10 | `+0.000150863` |
| MRR@20 | `+0.000333333` |
| support cosine | `-0.000000163` |

Boundary audit on the full split:

```json
{
  "query_count": 1342,
  "dense_hit_gain_query_count": 0,
  "dense_hit_loss_query_count": 0,
  "total_gained_dense_docs": 0,
  "total_lost_dense_docs": 0,
  "total_net_dense_hit_delta": 0
}
```

## Training-Depth Assessment

The current evidence does not support a simple conclusion that failed routes
were abandoned only because they were under-trained.  Several prior routes
showed local improvements but lost dense overlap, support, Recall, or CUB when
validated on broader surfaces.  Those failures are geometry/gate failures, not
just shallow-training failures.

The reasonable exploration ratio is:

1. Run small probes to identify a plausible movement shape.
2. Require strict dense-equivalence gates before investing in deeper training.
3. Broaden from canary to broad50/full shared15 before promoting the line.
4. Only then spend longer training or larger surfaces.

M675 seed6547 is the first recent candidate that survives this ratio cleanly:
it preserves top100 dense membership and retrieval floor while producing a
small but positive O@256 and ranking delta on full shared15.

This is not a final breakthrough.  It is a credible first-stage candidate that
is worth broader native/official validation and, after that, controlled longer
training with the same gates.

## Decision

Freeze `m675_full_shared15_m674_guard_seed6547` as the current P1 first-stage
candidate baseline.

Next research step:

- validate the frozen candidate on the next broader/native or official surface;
- do not introduce BM25/reranker until this baseline is recorded and compared;
- if deeper training is attempted, keep the same dense-equivalence gates and
  reject any gain bought by overlap/support/CUB loss.
