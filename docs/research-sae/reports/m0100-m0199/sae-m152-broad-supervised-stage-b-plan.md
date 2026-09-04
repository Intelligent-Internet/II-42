# M152 Broad Supervised Stage-B Plan

## Summary

M152 restarts the B stage from `M150 Stage A best`, because recent diagnostics
showed that narrow C-stage hard-row tuning can improve a local candidate
surface without transferring to official full-corpus retrieval.

The new B stage trains on full-corpus, qrels-backed hard-negative surfaces. The
goal is not to mimic dense everywhere. The goal is to make SAE atoms act as a
BM25 complement: preserve semantic positives that BM25 misses or ranks too low,
while learning to suppress full-corpus false positives that dominate top-100
admission.

## Why B, Not C

- Stage B owns global sparse retrieval geometry.
- Stage C should only perform calibration, active clipping, DF pruning, and
  focused local repair after B is already stable.
- M151 showed the failure mode clearly: direct Stage-A-to-C6 hard-negative
  training improved candidate-surface MRR but official full-corpus metrics
  collapsed.

## Training Surface

Documents are not sampled. Every dataset uses its full materialized corpus.
Only query counts are capped in the first B0 run so the surface can be
validated before scaling to larger train-query counts.

Initial B0-lite train specs:

| Dataset | Qrels split | Max queries | Reason |
| --- | --- | ---: | --- |
| `fiqa` | `train` | all | small supervised finance set |
| `nfcorpus` | `train` | all | biomedical lexical/semantic mix |
| `scifact` | `train` | all | scientific claim retrieval |
| `quora` | `dev` | 5000 | duplicate-question semantic canary |

Large datasets such as `fever`, `hotpotqa`, `msmarco`, and `dbpedia-entity`
are intentionally excluded from the first runnable B0-lite job. The current
`m110_full_corpus_index_eval.py` loads all document embeddings into one process;
it was OOM-killed while preparing `fever` with 5.4M documents. Those datasets
need a streaming/sharded surface builder before they can be used safely.

Initial B0-lite validation specs:

| Dataset | Qrels split | Max queries |
| --- | --- | ---: |
| `fiqa` | `dev` | all |
| `nfcorpus` | `dev` | all |

This is deliberately broader than C6 but still short enough to debug. If B0
transfers to the M130 continuity full-corpus gate, the next engineering step is
to add streaming/sharded hard-negative mining for the large BEIR corpora rather
than tune C-stage losses.

## Objective

The B0 objective uses:

- qrels labels as the primary ranking target.
- BM25/dense/SAE/BM25+SAE top documents as hard negatives.
- row weighting that emphasizes `dense_miss` and `candidate_hit_score_low`.
- BM25-complement loss through existing BM25-normalized complement weighting.
- optional hard-negative pairwise losses on final BM25+SAE scores and raw SAE
  scores.
- no active/fanout compression pressure (`loss_k_budget=0`) during this stage.

Dense/PPLX remains a semantic prior and hard-negative source, not the final
optimization target.

## Acceptance Gate

The first gate is the same M130 continuity full-corpus surface so it can be
compared with C4/C6/M130 immediately:

- Must beat dense on Recall@100, MRR@20, NDCG@10, and MAP@100.
- Must beat M151 clearly.
- Should approach C6 standalone SAE or C6 fixed-profile score fusion before C
  compression is attempted.
- If candidate-surface improves but full-corpus does not, the surface is still
  too narrow or the hard-negative mix is wrong.

## Runner

Use:

```bash
scripts/run_m152_broad_supervised_stage_b_spark.sh
```

Default remote output:

- Train rows: `/home/huoju/leask/runs/bm25sae-m152-broad-supervised-stageb-b0-lite-v1-candidate-rows`
- Eval rows: `/home/huoju/leask/runs/bm25sae-m152-broad-supervised-stageb-b0-lite-v1-eval-candidate-rows`
- Checkpoint: `/home/huoju/leask/runs/bm25sae-m152-broad-supervised-stageb-b0-lite-v1/bm25sae_stageb_best.pt`
- Full-corpus gate: `/home/huoju/leask/runs/bm25sae-m152-broad-supervised-stageb-b0-lite-v1-full-corpus-eval`

## Decision Rule

Do not continue C-stage tuning until B0 or its scaled successor proves that the
broad supervised B surface transfers to full-corpus retrieval.
