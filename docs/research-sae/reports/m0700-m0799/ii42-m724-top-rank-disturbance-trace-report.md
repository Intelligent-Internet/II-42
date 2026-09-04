# II-42 M724 Top-Rank Disturbance Trace

Date: 2026-07-07

## Objective

M723 found `81` useful-gain tradeoff queries: queries where M721b improved
O@100 / Recall / CUB but hurt at least one early-rank metric.  M724 replays
those queries through the native P1.3 postings scorer and records ranked-doc
traces to identify the concrete mechanism behind the promotion failure.

This remains first-stage work:

- P1.3 document postings are frozen.
- No BM25, no reranker, no learned gate, no qrels-driven training.
- Qrels are used only to diagnose metric movement after replay.

## Artifacts

- Script:
  `scripts/audit_m724_top_rank_disturbance_trace.py`
- JSON:
  `runs/m724_top_rank_disturbance_trace_v1/m724_summary.json`
- Markdown:
  `runs/m724_top_rank_disturbance_trace_v1/m724_report.md`

Input:

- `runs/m722_p1p3_pair_interaction_retrieval_bridge_shared15_v1/`

## Mechanism Counts

| Mechanism | Queries |
| --- | ---: |
| `rank_metric_small_perturbation` | 73 |
| `first_relevant_demoted` | 3 |
| `nonrelevant_top10_intrusion` | 3 |
| `relevant_top10_replaced_by_nonrel` | 2 |

## Key Observation

The full trace changes the diagnosis compared with the 5-query smoke.  The
promotion blocker is not dominated by severe top10 replacement.  Most harmful
queries are small rank/score perturbations that metrics amplify:

- first relevant rank usually stays unchanged;
- top10 membership is usually unchanged;
- many MAP drops are caused by small relevant-doc reorderings deeper than the
  first relevant hit;
- a small number of severe cases still exist, but they are not the dominant
  class.

Examples of severe traces:

- `nfcorpus:PLAIN-248`: first relevant demoted `1 -> 2`,
  dMRR `-0.500000`
- `scidocs:b2a20116c0609e23d3adfeaa604500fddca66178`: relevant top10 doc
  replaced by nonrelevant top10 doc, dNDCG `-0.098039`
- `cqadupstack:android_51645`: first relevant demoted `4 -> 5`,
  dMRR `-0.050000`

But these are exceptions.  `73 / 81` trace rows are classified as
`rank_metric_small_perturbation`.

## Decision

Do not jump directly to deeper training.  The current variant already creates
many safe O@100 gains, but scale/movement is slightly too aggressive for
rank-sensitive metrics.

The next experiment should be M725 conservative-scale / movement-confidence
sweep:

1. Re-run M722 at lower scales, starting with `0.01` and `0.005`.
2. Preserve the same P1.3 native postings surface.
3. Require O@100/Recall/CUB non-negative and NDCG/MAP/MRR non-negative.
4. If lower scale keeps most O@100 gain while removing rank harm, the route
   becomes a viable first-stage candidate.
5. If lower scale removes rank harm only by eliminating O@100/Recall gains,
   then the next design needs a per-query confidence gate rather than global
   scale.

## Conclusion

M724 supports continuing the route, but it rejects blind long training.  The
first-stage problem is now better described as safe movement calibration:
M721b can move useful support, but the movement must be scaled or gated so that
small dense-rank perturbations do not harm early-rank metrics.
