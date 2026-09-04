# ii42 M1101 Rank-Preserving Semantic-Neighbor Plan

Date: 2026-07-08

## Trigger

M1101 was launched after M1100 final JSON became available and matched the
expected trigger shape.

M1101 is only justified if M1100 final repeats the interim shape:

```text
M1100 improves unified Recall@100 or atom-only signal
but loses M1050 MRR@20 / NDCG@10 / MAP@100.
```

The final M1100 three-dataset result shows exactly this risk:

- versus lexical BM25, M1100 unified 0.5 improves Recall, NDCG, and MAP, but
  narrowly regresses MRR;
- versus M1050, M1100 unified 0.5 improves Recall but loses MRR/NDCG/MAP;
- atom-only improves over M1050 on all four macro metrics but remains far below
  lexical as a standalone row.

This means the semantic-neighbor idea has signal, but the supervision is too
strong or too rank-agnostic.

## Active Run

Completed on `spark-1`:

```text
tmux session: ii42_m1101_rank_preserving_semantic
run dir: /home/huoju/leask/runs/ii42-m1101-rank-preserving-semantic-neighbor-v1
output: m1101_rank_preserving_semantic_neighbor_s1050.json
seed: 1050
learning-rate: 0.0007
semantic-hard-negatives-per-query: 2
semantic-max-pairs-per-dataset: 800
```

Final audit report:

```text
docs/research-sae/reports/m1100-m1199/ii42-m1101-rank-preserving-semantic-neighbor-result-report.md
```

Final aggregate:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| lexical_bm25 | 0.484541 | 0.392835 | 0.325236 | 0.258054 |
| M1050 unified 0.5 | 0.503889 | 0.405647 | 0.337627 | 0.270175 |
| M1100 unified 0.5 | 0.512057 | 0.392427 | 0.331480 | 0.261103 |
| M1101 unified 0.5 | 0.498775 | 0.397779 | 0.330974 | 0.258977 |

M1101 versus M1050 unified 0.5:

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| -0.005115 | -0.007868 | -0.006653 | -0.011198 |

M1101 did make the row safer versus lexical BM25 than M1100:

| dRecall@100 | dMRR@20 | dNDCG@10 | dMAP@100 |
| ---: | ---: | ---: | ---: |
| +0.014234 | +0.004944 | +0.005738 | +0.000923 |

But it lost all four macro metrics versus M1050. This means lowering semantic
pressure made the branch conservative, not better.

## Hypothesis

M1100 adds semantic hard negatives at the same order as the base qrel/BM25
pairs. That teaches the atom vocabulary to separate semantic confounders, but
it can move top-rank geometry away from the M1050 lexical-compatible ordering.

M1101 should keep the semantic signal but make it rank-preserving:

```text
base M1050 pairs remain dominant
semantic pairs become auxiliary
semantic changes must not erase M1050's top-rank advantage
```

## Minimal Variant

No architecture change.

Use the same script as M1100 with lower semantic pressure:

- `semantic-max-pairs-per-dataset`: `600-800` instead of `2000`;
- `semantic-hard-negatives-per-query`: `2` instead of `4`;
- `learning-rate`: `5e-4` or `7e-4` instead of `1e-3`;
- keep `rank-epochs=8`;
- keep `seed=1050` for direct M1050/M1100 comparison.

This is the first variant because it tests whether M1100 simply over-weighted
semantic-neighbor supervision. It should be launched only if M1100 final has a
clear Recall/ranking tradeoff.

## Stronger Variant If Needed

If the minimal variant still trades ranking away, add an explicit loss term
instead of more pair tuning:

```text
preserve score gap between M1050 lexical-compatible positives and hard negatives
while adding semantic hard-negative separation
```

That would require saving or recomputing M1050 atom scores as a teacher. It is
more expensive and should not be implemented before the low-strength variant.

## Pass Gate

Same as M1100, but with an additional comparison:

- `unified_scale_0.5` must beat lexical BM25 on Recall@100 and MAP@100;
- no MRR/NDCG regression versus lexical BM25;
- must match or beat M1050 on at least two of four macro metrics;
- if it only improves Recall while losing MRR/NDCG/MAP, reject.

## Stop Rule

If both M1100 and M1101 show the same shape, stop semantic-neighbor pair
construction as a standalone route. The remaining evidence would point back to
B12/M150-style admission/scoring replay rather than more atom-vocabulary
training.

This stop rule is now triggered. M1100 bought Recall with top-rank loss, while
M1101 reduced the damage versus lexical but still failed to beat M1050. Do not
continue with more semantic pair count / LR swaps unless a new loss term changes
the objective, such as an explicit admission/scoring teacher.
