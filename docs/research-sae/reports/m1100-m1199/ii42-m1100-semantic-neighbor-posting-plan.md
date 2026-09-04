# ii42 M1100 Semantic-Neighbor Posting Utility Plan

Date: 2026-07-08

## Goal

M1100 tests the next route selected by M825:

```text
M1050 low-fanout atom posting utility
    + frozen semantic qrel-positive selection
    + frozen semantic hard negatives
```

The objective is not to tune BM25 fusion, not to train a selector over M821
proposals, and not to continue blind df/reuse-band sweeps. M1100 keeps the
M1050 evaluator and posting utility loss, but changes the pair construction so
the atom vocabulary sees semantic confounders that BM25 hard negatives alone do
not expose.

## Why This Is Different

Recent evidence:

- M821 found a clean useful expanded oracle ceiling, but M822-M824 could not
  learn a clean leave-surface selector. That route is paused.
- M1050 created low-fanout atoms and a three-dataset heldout unified gain over
  lexical BM25, but atom-only retrieval stayed weak.
- M1060 reuse-band tuning improved one dataset and regressed another, proving
  that df/reuse statistics alone are not the missing supervision.
- B12/M150 historical controls show that BM25+SAE admission/scoring can beat
  dense when the training signal targets actual admission/ranking utility.

M1100 therefore adds semantic-neighborhood supervision while preserving qrels
as the only positive label source. It does not treat dense-neighbor non-qrels
as positives.

## Implementation

New files:

- `scripts/research_sae_m1100_semantic_neighbor_posting.py`
- `scripts/run_m1100_semantic_neighbor_posting_spark.sh`

The script reuses M1050 components:

- `prepare_dataset`
- `collect_seed_tokens`
- `train_posting_utility`
- `evaluate_rows`
- `aggregate_rows`

The only route-level change is pair construction:

1. Build M1050 base pairs: qrel positives vs lexical BM25 hard negatives.
2. Encode train queries and candidate documents with the frozen PPLX encoder.
3. For each train query, select qrel positives by frozen semantic similarity.
4. Select semantic hard negatives: nearest candidate documents not in qrels.
5. Add those semantic pairs to the M1050 posting utility objective.

## Active Run

Launched on `spark-1`:

```text
tmux session: ii42_m1100_semantic_neighbor
run dir: /home/huoju/leask/runs/ii42-m1100-semantic-neighbor-posting-v1
output: m1100_semantic_neighbor_posting_s1050.json
seed: 1050
```

`seed=1050` is intentional so the split remains comparable with M1050 and the
fair M1060 run.

## Final Status

Status at 2026-07-08 15:16 EDT:

- training completed 8/8 epochs;
- checkpoint written;
- `nfcorpus`, `scifact`, and `fiqa` evals completed;
- final JSON was written on `spark-1` and synced locally;
- formal audit report:
  `docs/research-sae/reports/m1100-m1199/ii42-m1100-semantic-neighbor-posting-result-report.md`.

Training signal:

```text
base_pairs=4644 semantic_pairs=4644
epoch 1 loss=1.308333 rank=0.586703 fanout=16.026449
epoch 8 loss=0.223624 rank=0.204980 fanout=0.128694
```

Final heldout comparison versus M1050:

| Row | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| atom-only | +0.013078 | +0.006161 | +0.006490 | +0.001004 |
| unified 0.5 | +0.008168 | -0.013221 | -0.006147 | -0.009072 |

M1100 versus lexical BM25:

| Row | dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| --- | ---: | ---: | ---: | ---: |
| atom-only | -0.281562 | -0.283333 | -0.223019 | -0.173919 |
| unified 0.5 | +0.027516 | -0.000408 | +0.006244 | +0.003049 |

Interpretation:

- semantic-neighbor supervision is real, not a no-op: atom-only improves over
  M1050 on all four macro metrics;
- unified 0.5 improves Recall@100 over both lexical BM25 and M1050;
- the cost is top-rank instability: unified MRR/NDCG/MAP regress versus M1050,
  and MRR narrowly regresses versus lexical BM25;
- verdict is `fail` for promotion, but the failure mode is precise enough to
  justify one rank-preserving follow-up.

## Pass Gate

Primary gate is heldout aggregate on the M1050 three-dataset surface:

- `unified_scale_0.5` must beat lexical BM25 on Recall@100 and MAP@100.
- It should not regress MRR@20 or NDCG@10 versus lexical BM25.
- It should beat or match M1050 `unified_scale_0.5` on at least two of four
  macro metrics.
- Atom-only retrieval should improve over M1050 atom-only without fanout
  returning to high-df/full-corpus behavior.

## Stop Conditions

Stop this branch if any of these hold:

- Train improves but heldout does not.
- Gains come only from lexical BM25 fallback while atom-only remains flat.
- `scifact` or `fiqa` regresses enough to erase macro gains.
- Atom df/fanout moves back toward M1030/M1040 high-fanout behavior.
- A positive result requires dataset-specific weights or thresholds.

## Next If Positive

If M1100 passes the three-dataset heldout gate:

1. rerun on a broader BEIR/shared surface;
2. compare against B12 named profiles and M1050;
3. only then consider native-index engineering or admission-profile training.

## Next If Negative

M1100 failed promotion, but it did not fail silently. Because it raised
atom-only quality and unified Recall while hurting top-rank metrics, the next
route is one low-strength rank-preserving semantic-neighbor replay:

```text
M1101: lower semantic pair pressure and lower LR.
```

If M1101 repeats the same Recall-for-ranking tradeoff, stop semantic-neighbor
pair construction as a standalone route and return to B12/M150-style
admission/scoring replay on the current native index contract.
