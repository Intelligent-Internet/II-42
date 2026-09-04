# M512 Route-Fanout LoRA Report

M512 adds a no-BM25/no-qrels route-aware fine-tune on top of the M510/M511
PPLX-LoRA support model.  The extra objective uses dense teacher top docs as
positives and sampled corpus docs as negatives, while retaining the M508 direct
dense/support anchor.

## Run

| Run | Host | Task | Route docs | Route groups | Prefixes | Output |
| --- | --- | --- | ---: | ---: | --- | --- |
| `fiqa_route_fanout_lora` | `spark-1` | `FiQA2018` | `4096/57638` | `32` | `64,96,128` | `outputs/m512/fiqa_route_fanout_lora/m512_fiqa_route_fanout_lora.json` |

This is a route-subset gate, not a full-corpus BEIR score.  The subset includes
qrels positives and row-int8 dense teacher neighbours for heldout eval queries.

## Metrics

| Source | NDCG@10 | R@100 | MRR@20 | MAP@100 | Candidate R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `route_subset_teacher_row_int8_dense` | 0.47062 | 0.85703 | 0.56380 | 0.41763 | 1.00000 | 1.00000 |
| `m506_structural_candidates_p128` | 0.46704 | 0.87266 | 0.57235 | 0.41674 | 0.99359 | 0.95531 |
| `m511_lora_support_candidates_p128` | 0.46711 | 0.87266 | 0.57163 | 0.41646 | 0.95359 | 0.74261 |
| `m506_structural_candidates_p96` | 0.46704 | 0.86484 | 0.57240 | 0.41687 | 0.97703 | 0.90427 |
| `m511_lora_support_candidates_p96` | 0.46711 | 0.87005 | 0.57247 | 0.41677 | 0.92484 | 0.68090 |
| `m506_structural_candidates_p64` | 0.46041 | 0.85443 | 0.55695 | 0.41216 | 0.92750 | 0.79585 |
| `m511_lora_support_candidates_p64` | 0.46262 | 0.85294 | 0.57329 | 0.41208 | 0.85656 | 0.58523 |

The output source names still say `m511_lora_support_candidates_*` because the
M512 script reuses the M511 route evaluator.  In this report they refer to the
post-M512 route-fanout fine-tuned model.

## Comparison To M511

M511 tight best point:

- p128: NDCG@10 0.46717, Recall@100 0.86693, candidate recall 0.97891,
  Touch 0.91362.

M512 route-fanout point:

- p128: NDCG@10 0.46711, Recall@100 0.87266, candidate recall 0.95359,
  Touch 0.74261.

M512 therefore reduces p128 touched docs by 18.7% relative to M511 p128 while
keeping NDCG flat and improving Recall@100.  The tradeoff is lower candidate
recall.  This is the first result in this branch that materially changes the
fanout curve without collapsing qrels quality.

p96 is also promising: it keeps NDCG@10 0.46711 and improves Recall@100 to
0.87005 while reducing Touch to 0.68090.  Candidate recall 0.92484 is below the
promotion target, but close enough to justify tuning the route objective rather
than abandoning the line.

## Interpretation

The M512 result validates the direction: direct route-aware support training can
shape coordinate sharing, not just representation similarity.  The route is not
solved yet because candidate recall dropped too much, but the bottleneck is now
well localized.

The next useful work is not more plain LoRA epochs.  It is objective balancing:

- increase positive candidate preservation;
- keep the negative sharing penalty;
- evaluate p96/p128 as the main compression frontier;
- promote only if candidate recall recovers toward 0.95+ while Touch remains
  materially below the M511 p128 baseline.

## Next Step

M513 should run a small grid over route objective weights:

- route positives: `8` vs `16`;
- negatives: `24` vs `48`;
- negative penalty: `0.1`, `0.2`, `0.4`;
- anchor weight: `0.25`, `0.5`;
- route epochs: `1` vs `2`.

Promotion criterion for a FiQA route-subset gate:

- p96 or p128 NDCG@10 near 0.467;
- Recall@100 near or above 0.87;
- candidate recall at least 0.95;
- Touch clearly below 0.80.

If this passes, scale to Broad4.  If it fails after the M513 grid, the route
should switch from sampled contrastive fanout pressure to a differentiable
posting-head load-balancing objective.
