# II-42 M722 P1.3 Pair-Interaction Retrieval Bridge

Date: 2026-07-07

## Objective

M721b proved that pair-interaction selected atoms can move dense-boundary
pairs on the correct P1.3 signed-dot native postings surface while preserving
the local head.  M722 tests the next gate: whether that pair-level movement
translates into full retrieval metrics on shared15.

This is still a first-stage dense-equivalence experiment:

- P1.3 document posting geometry is frozen.
- Query-side atoms are modified only by the accepted M721b candidate.
- No BM25, no reranker, no learned gate, and no qrels-driven training.
- Qrels are used only for diagnostic metrics.

## Artifacts

- Script:
  `scripts/evaluate_m722_p1p3_pair_interaction_retrieval_bridge.py`
- JSON:
  `runs/m722_p1p3_pair_interaction_retrieval_bridge_shared15_v1/m722_summary.json`
- Markdown:
  `runs/m722_p1p3_pair_interaction_retrieval_bridge_shared15_v1/m722_report.md`
- Per-dataset JSON:
  `runs/m722_p1p3_pair_interaction_retrieval_bridge_shared15_v1/datasets/`

## Command

```bash
python3 scripts/evaluate_m722_p1p3_pair_interaction_retrieval_bridge.py \
  --datasets \
    arguana,climate-fever,cqadupstack,dbpedia-entity,fever,fiqa,hotpotqa,msmarco,nfcorpus,nq,quora,scidocs,scifact,trec-covid,webis-touche2020 \
  --output-root runs/m722_p1p3_pair_interaction_retrieval_bridge_shared15_v1
```

The run completed in `746.30s`.  Most of the cost is native postings replay
and candidate-query generation, not model training.  The new script emits
per-dataset progress and writes per-dataset JSON as each row completes.

## Macro Result

| Source | CUB | O@10 | O@50 | O@100 | O@256 | NDCG@10 | MAP@100 | R@100 | MRR@20 | Support cos | Support recall |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| P1.3 | 0.960323 | 0.934379 | 0.941756 | 0.943737 | 0.947923 | 0.774007 | 0.693932 | 0.851777 | 0.871168 | 1.000000 | 1.000000 |
| M721b | 0.960377 | 0.934624 | 0.941627 | 0.949654 | 0.948129 | 0.773786 | 0.693511 | 0.851967 | 0.870446 | 0.999956 | 1.000000 |

Delta M721b minus P1.3:

| Metric | Delta |
| --- | ---: |
| CUB | +0.000054 |
| O@10 | +0.000245 |
| O@50 | -0.000129 |
| O@100 | +0.005918 |
| O@256 | +0.000206 |
| NDCG@10 | -0.000221 |
| MAP@100 | -0.000422 |
| R@100 | +0.000191 |
| MRR@20 | -0.000723 |
| Support cosine | -0.000044 |
| Support recall | +0.000000 |

## Interpretation

M722 passes the dense-equivalence guard but does not pass the full promotion
gate.

Positive:

- Dense overlap@100 improves by `+0.005918`.
- Recall@100 improves by `+0.000191`.
- Candidate upper bound improves by `+0.000054`.
- Query support recall is unchanged at `1.000000`.
- Query support cosine remains very high at `0.999956`.

Negative:

- NDCG@10 drops by `-0.000221`.
- MAP@100 drops by `-0.000422`.
- MRR@20 drops by `-0.000723`.
- O@50 drops slightly by `-0.000129`.

This means the M721b movement is not merely an artifact of the pair-level
audit: it does transfer to full native retrieval and improves dense overlap
and Recall/CUB.  However, it also slightly perturbs early ranking quality.  It
should not be promoted as a new P1 default yet.

## Decision

Keep the route alive, but do not scale blindly.

The result argues against abandoning the line as "under-trained": the correct
P1.3 shared15 native surface shows real support-safe movement and improved
O@100/Recall/CUB.  The result also argues against immediate long training:
the next bottleneck is ranking-metric tradeoff, not simple training duration.

Recommended next step:

1. Audit the queries where M721b improves O@100/Recall but hurts NDCG/MAP/MRR.
2. Split movement into safe classes: boundary fixes that preserve early rank
   versus fixes that disturb top10/top20.
3. Train or gate the compiler on dense-rank margin preservation, not qrels.
4. Re-run M722 only after the top-rank disturbance has a concrete diagnostic.

Stop condition for the current variant: do not promote `pair_hgb_rp1_b64_s0.02`
as a frozen P1 candidate until NDCG/MAP/MRR regressions are removed or explained
as noise by a broader repeated run.
