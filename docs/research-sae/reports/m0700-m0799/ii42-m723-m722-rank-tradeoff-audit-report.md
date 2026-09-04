# II-42 M723 M722 Rank-Tradeoff Audit

Date: 2026-07-07

## Objective

M722 showed that M721b passes the dense-equivalence guard but not the full
promotion gate: O@100, Recall@100, and CUB improve slightly, while NDCG@10,
MAP@100, and MRR@20 regress slightly.  M723 audits the per-query deltas to
identify whether this is broad instability or a concentrated top-rank
disturbance.

## Artifacts

- Script:
  `scripts/audit_m723_m722_rank_tradeoff.py`
- JSON:
  `runs/m723_m722_rank_tradeoff_audit_v1/m723_summary.json`
- Markdown:
  `runs/m723_m722_rank_tradeoff_audit_v1/m723_report.md`

Input:

- `runs/m722_p1p3_pair_interaction_retrieval_bridge_shared15_v1/`

## Macro Counts

| Queries | O@100 gain | Recall gain | CUB gain | Rank hurt | Rank improve | Safe gain | Tradeoff |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1342 | 687 | 18 | 16 | 175 | 115 | 614 | 81 |

Mean delta:

| Metric | Mean delta |
| --- | ---: |
| CUB | +0.000048 |
| O@100 | +0.005961 |
| MAP@100 | -0.000463 |
| MRR@20 | -0.000808 |
| NDCG@10 | -0.000269 |
| Recall@100 | +0.000185 |

## Diagnosis

The M721b movement is mostly safe at the query level:

- `687 / 1342` queries improve O@100.
- `614 / 1342` queries are useful safe gains.
- only `81 / 1342` queries combine useful gain with early-rank harm.

The promotion failure is therefore not a global collapse of the first-stage
representation.  It is a concentrated top-rank disturbance problem.

The largest disturbances are sparse but high-impact, for example:

- `climate-fever:135`: dNDCG `-0.226294`, dMAP `-0.250000`, dMRR `-0.500000`
- `nfcorpus:PLAIN-248`: dNDCG `-0.081229`, dMAP `-0.016098`, dMRR `-0.500000`
- `nq:test65`: dNDCG `-0.032532`, dMAP `-0.083333`
- `cqadupstack:android_65261`: dNDCG `-0.044406`, dMAP `-0.066667`

Several of these have unchanged Recall/CUB and small or zero O@100 gains, which
means they are not useful boundary fixes.  They are pure early-rank
disturbances and should be filtered by a top-rank preservation gate.

## Decision

Do not treat this as a simple training-duration problem.

The route remains alive because the majority of query-level movement is safe
and because M722 improves dense overlap and Recall/CUB.  But scaling training
before adding a top-rank disturbance guard is likely to amplify the same
failure mode.

Recommended next step:

1. Build M724 top-rank disturbance audit with ranked-doc traces for the worst
   M723 queries.
2. Identify whether regressions come from demoting relevant top10 docs,
   promoting non-dense docs, or changing near-tie dense margins.
3. Add a dense-rank margin/top10 preservation gate to M721b selection.
4. Re-run M722 after filtering harmful movements.

Stop condition for current M721b variant:

- do not promote until the `81` useful-gain tradeoff queries are reduced or
  their rank-metric harm is neutralized without losing the O@100/Recall/CUB
  gains.
