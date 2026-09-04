# M151 Ranking-First Stage-B Probe

## Summary

M151 tested a reordered training idea: preserve the strong M150 Stage-A
semantic coverage first, then train ranking at a high active budget before any
fanout/cost compression. The canary started from
`bm25sae-m150-pplx16384k96-stagea-v1/bm25sae_stagea_best.pt`, reused the C6
official dense-miss candidate rows, enabled `feature_k=128`, disabled
`loss_k_budget`, and added hard-negative pairwise losses on both final
BM25+SAE scores and raw SAE scores.

Result: the candidate-surface metrics improved, but the official full-corpus
gate did not transfer. M151 is not promoted.

## Change Tested

The Stage-B trainer now supports optional hard-negative pairwise objectives:

- `--loss-final-hard-pairwise`
- `--loss-sae-hard-pairwise`
- `--hard-pairwise-top-negatives`

The intended effect is to directly rank qrel positives above hard false
positives without first compressing active atoms or fanout. The default values
remain zero, so existing runs are unchanged unless these flags are passed.

Runner:

- `scripts/run_m151_ranking_first_spark.sh`

Remote artifacts:

- Checkpoint: `/home/huoju/leask/runs/bm25sae-m151-ranking-first-k128-v1/bm25sae_stageb_best.pt`
- Events: `/home/huoju/leask/runs/bm25sae-m151-ranking-first-k128-v1/events.jsonl`
- Full-corpus eval: `/home/huoju/leask/runs/bm25sae-m151-ranking-first-k128-v1-full-corpus-eval/m110_full_corpus_index_eval.json`

Local copies:

- `results/m151-ranking-first-k128/events.jsonl`
- `results/m151-ranking-first-k128/m110_full_corpus_index_eval.json`

## Candidate-Surface Result

Best validation event was step `2250`.

| Model | hit@20 | mrr@20 |
| --- | ---: | ---: |
| BM25 | 0.8832 | 0.6946 |
| Dense | 0.9707 | 0.8321 |
| SAE | 0.9574 | 0.8243 |
| BM25+SAE | 0.9592 | 0.8384 |

This says the ranking-first objective can improve candidate-surface ordering:
BM25+SAE MRR exceeded dense on the reused C6 rows. However, hit@20 stayed below
dense, so it did not fully fix top-k admission even on the candidate surface.

## Official Full-Corpus Result

| Model | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| BM25 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+Dense score fusion | 0.3175 | 0.2983 | 0.2240 | 0.1484 |
| SAE | 0.2829 | 0.2609 | 0.1987 | 0.1374 |
| BM25+SAE score fusion | 0.3016 | 0.2711 | 0.2065 | 0.1434 |
| BM25+SAE RRF | 0.3014 | 0.2591 | 0.1888 | 0.1253 |

M151 underperformed the dense and BM25+dense baselines on every official
full-corpus ranking metric. It also underperformed the earlier C6 promoted
candidate on the same eval surface by a large margin:

| Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M150 C6 BM25+SAE score fusion | 0.3841 | 0.4271 | 0.3010 | 0.2008 |
| M151 BM25+SAE score fusion | 0.3016 | 0.2711 | 0.2065 | 0.1434 |

## Interpretation

The failure is not that hard-negative ranking is useless. It improved the
local candidate surface and reduced training loss as expected. The failure is
that starting from Stage A and training directly on C6 official dense-miss rows
is too narrow: it overfits the candidate rows and does not preserve the global
full-corpus sparse retrieval geometry.

The earlier C6 result shows that the broad Stage-B representation still
matters. The correct next test is not more C6 hard-negative tuning from Stage A.
It is:

1. Stage A: broad semantic coverage.
2. Stage B: broad all-data ranking-first training, high active budget, no
   cost compression.
3. Stage C: official dense-miss / hard-row correction.
4. Stage D: fanout and active clipping after quality is stable.

## Decision

M151 is closed as a negative result.

Next recommended experiment: M152, same hard-negative objective, but run it on
the broad Stage-B all-data candidate surface before entering C6-style dense-miss
correction.
