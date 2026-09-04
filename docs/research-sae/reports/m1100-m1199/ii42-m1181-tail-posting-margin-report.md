# M1181 Tail Posting-Score Margin Audit

M1181 checks whether M1177 pair movements are explained by the posting score
margin itself: comparing M1137 `tail_p1_score` margins against M1129
`base_p1_score` margins for positive and harm pairs.

Artifacts:

- Script: `scripts/audit_m1181_tail_posting_margin.py`
- JSON: `runs/m1181_tail_posting_margin_v1/tail_posting_margin.json`
- Summary: `runs/m1181_tail_posting_margin_v1/summary.md`

## Result

| Source | Count | Aligned | Base P1 margin | Tail P1 margin | Delta P1 margin | Delta BM25 margin | Movement |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| rank_teacher_positive | 18,517 | 0.682 | +3.451997 | +8.728277 | +5.276279 | +0.000000 | +5.356537 |
| harm_penalty | 25,667 | 0.635 | +8.841143 | +9.782391 | +0.941247 | -0.003747 | -6.733471 |

The positive rank-teacher pairs have a large positive P1 margin shift, while
BM25 margin is unchanged.  That supports the hypothesis that the useful signal
is inside posting score/output shape, not lexical rescue or feature-space
reranking.

However, alignment is only moderate:

- `rank_teacher_positive` aligned rate: 68.2%.
- `harm_penalty` aligned rate: 63.5%.

Dataset split matters:

- Strong positive alignment: `arguana`, `nfcorpus`, `trec-covid`.
- Mixed: `dbpedia-entity`.
- Weak/problematic: `msmarco` and `fiqa` harm rows.

## Interpretation

M1181 gives a more useful direction than M1178-M1180:

- The signal is not captured by shallow scorer-space distillation.
- The signal is partly visible in posting score margin.
- The signal is not clean enough to start large global training directly.

Decision:

1. Continue to atom/posting-level delta audit before training.
2. Split surfaces by alignment quality; do not use all pairs as equal teacher
   rows.
3. Treat weak alignment rows, especially `msmarco` and `fiqa` harm, as
   diagnostic/hard-negative surfaces.
4. If atom-delta alignment is strong on the clean surfaces, train a small
   compiler/output-head canary there first.
