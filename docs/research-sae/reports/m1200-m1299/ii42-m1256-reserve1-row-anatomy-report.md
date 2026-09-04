# M1256 Reserve1 Row Anatomy

## Goal

M1255 showed that corrected `reserve1_s1` is the best current macro-safe
low-reserve frontier.  It beats true `reserve0_s1`, but only slightly.

M1256 asks whether that single low-tail atom has qrels-free row-level structure
that can explain when it helps or hurts.

This is an observability audit, not a new guard or default-policy attempt.

## Runs

Smoke:

- `runs/m1256_reserve1_row_anatomy_smoke_v1/`
- Datasets:
  `cqadupstack`, `scidocs`, `webis-touche2020`

Full `shared15`:

- JSON:
  `runs/m1256_reserve1_row_anatomy_v1/m1256_reserve1_row_anatomy.json`
- Markdown:
  `runs/m1256_reserve1_row_anatomy_v1/m1256_reserve1_row_anatomy.md`
- Query count: `1342`

## Reserve1 vs Reserve0

| Comparison | dRecall | dMAP | dNDCG | dMRR | dCUB | Score |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `reserve1_s1 - reserve0_s1` | +0.000163 | +0.000200 | -0.000357 | -0.000132 | -0.000022 | +0.000414 |

The macro result is still positive, but the tradeoff is clear:

- improves Recall@100 and MAP@100
- weakens NDCG@10, MRR@20, and CUB slightly
- net score remains slightly positive

## Per-Dataset Anatomy

| Dataset | NegMetrics | Score | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 3 | -0.077070 | +0.000701 | +0.000044 | -0.000746 | -0.000507 | -0.002525 |
| `climate-fever` | 2 | -0.046667 | +0.000000 | +0.000067 | -0.003815 | -0.000091 | +0.000000 |
| `trec-covid` | 4 | -0.034734 | -0.000233 | -0.000152 | -0.002570 | +0.000000 | -0.000029 |
| `cqadupstack` | 3 | -0.007337 | -0.000588 | -0.000109 | +0.000000 | +0.000000 | -0.000115 |
| `fiqa` | 2 | -0.006019 | +0.000000 | +0.000398 | -0.000015 | -0.000586 | +0.000000 |
| `webis-touche2020` | 1 | -0.004663 | +0.000000 | -0.000364 | +0.000036 | +0.000000 | +0.000000 |
| `dbpedia-entity` | 2 | -0.001530 | -0.000002 | +0.001258 | +0.000288 | -0.000500 | +0.000130 |
| `arguana` | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `fever` | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `nq` | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `quora` | 0 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| `scifact` | 0 | +0.000175 | +0.000000 | +0.000034 | +0.000000 | +0.000037 | +0.000000 |
| `scidocs` | 3 | +0.002650 | +0.002000 | -0.000516 | -0.000101 | -0.000119 | +0.002000 |
| `msmarco` | 0 | +0.003788 | +0.000443 | +0.000259 | +0.000135 | +0.000000 | +0.000527 |
| `hotpotqa` | 0 | +0.006569 | +0.000000 | +0.001654 | +0.000803 | +0.000000 | +0.000000 |

Reserve1 helps mostly through a few datasets (`hotpotqa`, `msmarco`, `scidocs`)
and hurts several others.  This is not row-safe enough for default promotion.

## Feature Buckets

The simple qrels-free features do not produce a clean safe region.

| Bucket | Count | AnyNeg | CubNeg | RankNeg | RecallNeg | MeanScoreDelta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `tail_present:1` | 1319 | 0.1440 | 0.0106 | 0.1372 | 0.0076 | -0.016895 |
| `tail_present:0` | 23 | 0.0000 | 0.0000 | 0.0000 | 0.0000 | +0.000000 |
| `tail_delta:positive` | 1319 | 0.1440 | 0.0106 | 0.1372 | 0.0076 | -0.016895 |
| `tail_abs_delta:q1` | 331 | 0.1269 | 0.0242 | 0.1118 | 0.0091 | -0.005444 |
| `tail_abs_delta:q2` | 329 | 0.1155 | 0.0030 | 0.1125 | 0.0030 | -0.000364 |
| `tail_abs_delta:q3` | 329 | 0.1459 | 0.0061 | 0.1398 | 0.0061 | -0.022844 |
| `tail_abs_delta:q4` | 330 | 0.1879 | 0.0091 | 0.1848 | 0.0121 | -0.038929 |

Higher tail magnitude is actually riskier.  The best bucket is near-neutral, not
a strong positive safe region.

## Best/Worst Rows

Best rows include large wins on `scidocs`, `hotpotqa`, `dbpedia-entity`, `fiqa`,
and `nfcorpus`.  Worst rows include large losses on `nfcorpus`,
`climate-fever`, `scidocs`, `fiqa`, and `trec-covid`.

The same tail feature shape appears on both sides: positive low-tail delta,
base count 8, and similar magnitude.  That means the current qrels-free atom
feature view is not enough to distinguish help from harm.

## Decision

Keep `reserve1_s1` as the current best macro-safe source-construction
candidate.

Do not promote it as an engineering default.

Do not build a simple query-time guard from these features.

## Next Step

The useful signal is real, but the observable feature set is too shallow.

Next work should change what is observed or learned:

1. Add row-local document movement features for the reserved atom, not just atom
   delta magnitude.
2. Check whether the reserved atom pulls in new candidate documents, displaces
   protected top documents, or only rescales existing evidence.
3. If document movement separates help/harm, use it as a source feature.
4. If document movement also fails, convert reserve1 into a training target or
   objective regularizer instead of a deployable query-time policy.

This matches the current broader conclusion: useful added atoms exist, but
qrels-free safe selection remains the bottleneck.
