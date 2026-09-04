# M599 M551 Broader Validation Report

M599 validates the M597/M598 union selector on a broader BEIR15-style
surface before deciding whether to promote the M551 gate line and start M600.

The goal is deliberately narrow:

- keep BM25 out;
- keep frozen LLM judge work out;
- do not run another threshold sweep;
- test the exact M597/M598 union selector on a broader matrix;
- start M600 output-head/posting-compiler work only if this broader matrix
  remains stable.

## Inputs

M597/M598 promoted a local union selector:

- source model: M551 active256/h768 residual posting transform;
- residual scale: `0.0125`;
- coordinate gain clip: `0.0`;
- train query fraction: `0.5`;
- selector query fraction: `0.2`;
- guarded selector:
  `overlap_at_20_delta >= 0.001 OR rank_teacher_mass_at_20_delta >= 0.003`.

M599 reused that shape unchanged.

## Surface

The official 1024-dimensional BEIR full15 root exists on `spark-1`, but several
datasets are too large for the current M551 runner because it materializes full
document matrices:

- `climate-fever`: about 41.9 GB `documents.jsonl`;
- `fever`: about 41.8 GB `documents.jsonl`;
- `hotpotqa`: about 39.0 GB `documents.jsonl`;
- `msmarco`: about 66.1 GB `documents.jsonl`;
- `dbpedia-entity`: about 34.9 GB `documents.jsonl`.

That root needs a streaming evaluator before it is a fair M551 validation
surface. M599 therefore uses the existing local BEIR15 shared face, which is
the broader standard recall-matrix surface already used by earlier dense
posting reports.

M599 shared root:

- source: `/home/huoju/leask/data/psql_bm25s_sae_beir15_shared`;
- adapted root:
  `/home/huoju/leask/runs/ii42-m599-m551-beir15-shared15-root-v1/_shared/tasks`;
- run dir:
  `/home/huoju/leask/runs/ii42-m599-m551-active256-union20-beir15-shared15-v1`;
- local copied JSONs:
  - `runs/m599_active256_h768_union20_beir15_shared15_seed551_minbase/m599_active256_h768_union20_beir15_shared15_seed551_minbase.json`;
  - `runs/m599_active256_h768_union20_beir15_shared15_seed552_minbase/m599_active256_h768_union20_beir15_shared15_seed552_minbase.json`;
  - `runs/m599_active256_h768_union20_beir15_shared15_seed553_minbase/m599_active256_h768_union20_beir15_shared15_seed553_minbase.json`.

Tasks:

`arguana`, `nfcorpus`, `fiqa`, `scidocs`, `scifact`, `trec-covid`,
`webis-touche2020`, `cqadupstack`, `climate-fever`, `dbpedia-entity`, `fever`,
`hotpotqa`, `msmarco`, `nq`, `quora`.

## Results

Deltas are versus each seed's `dense_topk256_sparse` baseline.

| Seed | Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: |
| 551 | learned | -0.001590 | -0.002090 | -0.000150 | -0.004500 | -0.001000 |
| 551 | guarded | +0.001100 | +0.000720 | +0.000000 | +0.000820 | -0.000310 |
| 552 | learned | -0.000890 | -0.000010 | +0.000060 | -0.002640 | -0.001180 |
| 552 | guarded | -0.000470 | +0.000090 | -0.000440 | -0.000050 | -0.000170 |
| 553 | learned | -0.002880 | -0.001960 | +0.000680 | -0.003660 | -0.001410 |
| 553 | guarded | -0.000620 | -0.001040 | +0.000820 | -0.003120 | -0.000600 |
| mean | learned | -0.001787 | -0.001353 | +0.000197 | -0.003600 | -0.001197 |
| mean | guarded | +0.000003 | -0.000077 | +0.000127 | -0.000783 | -0.000360 |

Positive-seed count:

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned | 0/3 | 0/3 | 2/3 | 0/3 | 0/3 |
| guarded | 1/3 | 2/3 | 1/3 | 1/3 | 0/3 |

Task-level guarded mean highlights:

| Task | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| arguana | -0.00308 | -0.00381 | +0.00000 | -0.00381 | +0.00166 |
| nfcorpus | +0.00167 | -0.00276 | +0.00188 | -0.00516 | -0.00100 |
| scidocs | -0.00121 | +0.00051 | -0.00000 | -0.00878 | +0.00134 |
| scifact | +0.00290 | +0.00426 | +0.00000 | +0.00600 | -0.00333 |
| webis-touche2020 | -0.00021 | +0.00065 | +0.00000 | +0.00000 | -0.00200 |

Many other shared-face tasks are effectively unchanged by the guarded source,
which means the macro result is not a robust promotion signal.

## Decision

M599 does not validate M597/M598 as a stable broader BM25-free endpoint.

The earlier M597/M598 signal remains real on its two tested surfaces:

- M597 official1024 BEIR8 guarded result was positive on all retrieval metrics;
- M598 MTEB10 guarded result fixed the MTEB MRR failure.

However, the broader BEIR15 shared15 matrix says that the same selector does
not generalize enough:

- learned source regresses NDCG, MAP, MRR, and overlap on all three seeds;
- guarded source collapses to near-zero deltas and still loses MRR and overlap;
- the result is too weak to justify another threshold sweep.

Therefore:

- stop the M597/M598 gate-microtuning line here;
- do not promote the union selector as a final M551 endpoint;
- do not start M600 from this branch, because the M600 precondition was a
  broader-matrix pass;
- preserve M597/M598 as a useful diagnostic milestone showing that
  utility/mass selection can rescue some surfaces, not as a final encoder
  route.

## Next Valuable Direction

The result points away from more gate tuning and toward the underlying model
surface:

1. If full official BEIR15 is required, build a streaming M551 evaluator first;
   the current runner is not appropriate for 20-66 GB document matrices.
2. If pursuing output-head/posting-compiler work, start it as a separate route,
   not as M597/M598 promotion. Its target should be dense-derived posting
   equivalence, with candidate-set distribution, top-k membership, Recall/MRR
   preservation, and support floors.
3. Keep BM25 and post-hoc fusion out until a BM25-free encoder/output-head
   surface has independently passed a broader matrix.
