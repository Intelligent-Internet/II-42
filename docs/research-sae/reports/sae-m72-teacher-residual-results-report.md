# SAE M72 Teacher Residual Results Report

Date: 2026-05-21

Status: completed first teacher-residual structure search. Local smoke found a
balanced setting, but medium-scale Spark validation did not beat the M70
coverage baseline.

## Summary

M72 moved teacher influence out of the M71 global semantic-neighborhood loss and
into the final listwise target as a confidence-gated residual. The intent was:

```text
qrel positives = hard target
BM25 = lexical anchor
dense/SAE teacher near-misses = residual target only when BM25 is weak
```

This is closer to the product direction than M71 because it trains the final
BM25+SAE ranking surface directly. It also avoids treating every teacher
near-miss as a global positive.

Result:

- Local smoke: useful.
- Medium Spark: not promoted.
- Decision: keep the residual-target implementation for future controlled
  experiments, but stop single-stage residual-weight sweeps.

## Implemented Pieces

The M70 trainer now supports:

- `teacher_residual_weight`
- `teacher_residual_gate=low_bm25`
- `teacher_residual_min_target`

The target construction is:

```text
base_target = qrels * (1 + (1 - bm25_score))
teacher_only = max(semantic_target - qrel_label, 0)
teacher_only = teacher_only if teacher_only >= min_target else 0
teacher_only *= (1 - bm25_score)
row_gate = 1 - bm25_concentration
final_target = base_target + teacher_residual_weight * row_gate * teacher_only
```

This keeps qrels as hard evidence and only adds teacher residual where BM25 is
not already a strong exact-match signal.

## Local Smoke

Two-dataset smoke: `scifact + nfcorpus`, 2,400 docs, 112 train queries, 43 eval
queries.

BM25 baseline:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.5258 | 0.6051 | 0.4705 | 0.3531 |

Selected local runs:

| Structure | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Residual mass | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| residual `0.02` + teacher KL `0.10` | 0.5431 | 0.6026 | 0.4686 | 0.3539 | 0.1169 | recall only |
| residual `0.05` + teacher KL `0.10` | 0.5421 | 0.6085 | 0.4708 | 0.3538 | 0.2923 | best local balance |
| residual `0.10` + teacher KL `0.10` | 0.5364 | 0.6026 | 0.4701 | 0.3543 | 0.5846 | too much residual |
| residual `0.20` + teacher KL `0.10` | 0.5435 | 0.6026 | 0.4702 | 0.3541 | 1.1693 | too much residual |
| residual `0.05`, no teacher KL | 0.5304 | 0.6040 | 0.4686 | 0.3538 | 0.2923 | teacher KL needed |

Local interpretation:

- `teacher_residual_weight=0.05` plus light teacher KL is the only run where
  all four metrics are above BM25.
- Removing teacher KL weakens the target.
- Residual mass above roughly `0.3` quickly starts to distort ranking.

## Medium Spark Validation

M39 medium setup: 9 datasets, 92,816 docs, 1,650 train queries, 679 eval
queries, 31,904 qrel positives in candidate pools.

| Structure | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Residual mass | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| BM25 baseline | 0.6661 | 0.6685 | 0.5858 | 0.4949 | n/a | baseline |
| M70 coverage baseline | 0.6662 | 0.6696 | 0.5858 | 0.4949 | 0.0000 | still best |
| residual `0.05` + teacher KL `0.10` | 0.6644 | 0.6692 | 0.5858 | 0.4950 | 0.5900 | rejected |
| residual `0.02` + teacher KL `0.10` | 0.6649 | 0.6692 | 0.5858 | 0.4949 | 0.2360 | rejected |

Medium interpretation:

- The residual target slightly improves MRR/MAP surface, but Recall@100 falls
  below both BM25 and the M70 coverage baseline.
- Even `0.02` residual is not small enough to preserve coverage on the medium
  set.
- The learned BM25 scale is lower than the M70 coverage baseline, which means
  residual target pressure is still perturbing the lexical anchor.

## Decision

Do not continue M72 as another residual-weight sweep. The residual target is
conceptually better than M71's global neighborhood loss, but it still does not
scale on the medium run.

Keep:

- all-qrel-positive candidate inclusion;
- dense and SAE teacher candidates as mining sources;
- light teacher KL as a local stabilizer;
- residual-target implementation for future diagnostics.

Stop:

- global semantic-neighborhood loss sweeps;
- single-stage residual target sweeps;
- SAE-teacher-dominant targets;
- dense-dominant residual labels without better query-family gating.

## Next Direction

The next step should not be M73 as another loss-weight run. The blocker is now
diagnostic granularity and supervision structure:

```text
1. Build query-family diagnostics for the M39 medium set.
   - lexical-heavy
   - semantic-heavy
   - broad/many-positive
   - short keyword
   - long natural-language

2. Report per-family deltas for M70 coverage, M71 neighborhood, and M72 residual.

3. Use dense teacher only where a family-level signal proves it helps.
   - candidate curriculum for semantic-heavy/broad queries
   - no teacher residual for lexical-heavy queries

4. Only after that, run a family-aware training objective.
```

M72 therefore closes as a useful negative result: the embedding teacher matters,
but without query-family routing it still trades recall for small ranking-surface
gains.
