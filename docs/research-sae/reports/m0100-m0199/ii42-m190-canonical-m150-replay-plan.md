# ii42 M190 Canonical M150 Replay Plan

## Summary

M190 is not a new training strategy. It is a strict replay of the known
successful M150 lineage with only two allowed changes:

- replace the data with the complete, audited PPLX `1024`-dimensional corpus;
- enforce dimension, split, and completeness checks before each stage.

The canonical success chain is:

```text
M150A1 Stage A best
  -> M150 Stage-B fusionnorm
  -> M150 A1 C6 official dense-miss
```

The target is to determine whether the M150 success can be reproduced after
fixing data completeness and embedding dimensions, without changing the
training shape. Any change to sampling, loss weights, active-k, row weights,
or stage order is out of scope for M190 and must be a separately named
experiment.

## Why M190 Exists

Recent M160/M170/M180 attempts mixed several changes at once:

- broader or stricter data;
- different source weighting and repeat policy;
- different streaming mode;
- different B/C row surfaces;
- different checkpoint handoff choices;
- sometimes using `latest` where the historical recipe used `best`.

That made the comparison non-causal. M190 restores a single controlled
question:

> If we keep the M150 training protocol fixed and only replace the data with a
> complete, audited PPLX `1024` corpus, can we reproduce or improve the M150
> C6 result?

## Canonical Baseline To Replay

The replay baseline is the M150 A1 lineage, not M150A2.

M150A2 is a useful control because its Stage-A standalone result was stronger,
but the strongest completed result came from the M150A1 lineage after
normalized Stage B and C6 dense-miss correction.

Historical quality on the continuity full-corpus surface:

| Version | SAE R@100 | BM25+SAE R@100 | BM25+SAE MRR@20 | BM25+SAE NDCG@10 | BM25+SAE MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M150A1 Stage A | `0.2404` | `0.2799` | `0.2467` | `0.1867` | `0.1270` |
| M150A2 Stage A | `0.2693` | `0.2997` | `0.2750` | `0.2021` | `0.1389` |
| M150A1 C6 | `0.3899` | `0.3841` | `0.4271` | `0.3010` | `0.2008` |

Therefore the promoted replay target is:

```text
M150A1 -> fusionnorm Stage B -> C6 official dense-miss
```

M150A2 may be replayed later as `M190-control-a2`, but it is not the primary
M190 line.

## Non-Negotiable Freeze List

These values and behaviors must stay equivalent to the M150 success recipe.

### Stage A Freeze

| Item | Frozen value |
| --- | --- |
| Script | `research_sae_m130_bm25sae_stage_a_pretrain.py` |
| Stage-A flow | M150A1 broad Stage-A flow |
| Init | from scratch |
| `n_features` | `16384` |
| `feature_k` | `96` |
| `batch_size` | `384` |
| `epochs` | `3` |
| `query_repeat` | `2` |
| `topk_iters` | `24` |
| `stream_mode` | default `legacy_doc_query`; do not use `combined_row_weighted` |
| `lr` | default `2.0e-4` |
| straight-through policy | default script behavior |
| coverage auxiliary | off for primary A1 replay unless the exact M150A1 run proves it was on |
| candidate eval auxiliary | off for primary A1 replay unless the exact M150A1 run proves it was on |
| checkpoint handoff | use `bm25sae_stagea_best.pt`, not `latest` |

### Stage B Freeze

| Item | Frozen value |
| --- | --- |
| Script | `research_sae_m130_bm25sae_stage_b_train.py` |
| Runner shape | M150 Stage-B from-best / fusionnorm shape |
| Init checkpoint | M190 Stage-A `best` |
| Train rows | same candidate-row contract as M150 Stage B |
| Eval rows | same candidate-row contract as M150 Stage B |
| `steps` | `5000` |
| `st_after` | `3500` |
| `batch_size` | `24` |
| `feature_k` | `96` |
| `n_features` | `16384` |
| Normalized fusion | keep fusionnorm behavior |
| checkpoint handoff | use Stage-B `best`, not `latest` |

### Stage C C6 Freeze

| Item | Frozen value |
| --- | --- |
| Runner shape | `run_m150_a1_c6_official_dense_miss_spark.sh` |
| Init checkpoint | M190 Stage-B fusionnorm `best` |
| Train qrel specs | `fiqa:train,nfcorpus:train,scifact:train` |
| Eval qrel specs | `fiqa:dev,nfcorpus:dev,quora:dev` |
| Candidate K | `160` |
| Source top K | `300` |
| Doc/query active | `96/96` |
| Max DF ratio | `0.12` |
| Dense teacher weight | `0.80` |
| BM25+dense rank weight | `0.75` |
| BM25 rank weight | `0.05` |
| BM25+SAE rank weight | `0.15` |
| SAE rank weight | `0.10` |
| Dense-miss row weight | `4.0` |
| Score-low row weight | `2.5` |
| Steps | `9000` |
| Loss args | exactly match C6 |
| checkpoint handoff | use C6 `best` |

## Allowed Changes

Only these changes are allowed in M190:

| Area | Allowed change |
| --- | --- |
| Embedding dimension | Enforce PPLX `1024` dimensions everywhere. |
| Corpus root | Use a new audited M190 materialized root. |
| Dataset completeness | Include BEIR15 plus prepared Wikipedia/arXiv/PubMed rows. |
| Test leakage guard | Remove official BEIR test qrel labels and test-only query ids from training surfaces. |
| BM25 caches | Rebuild caches for the new materialized root. |
| File paths/run names | Use `ii42-m190-*` names. |
| Preflight checks | Add stricter checks before launch. |
| Evaluation | Add official BEIR15 full-corpus gate after each stage. |

## Disallowed Changes

These are explicitly forbidden in the primary M190 replay:

- no `combined_row_weighted` Stage-A stream;
- no source-ratio scheduler;
- no physical repeat policy different from M150 unless represented as actual
  materialized rows and passed through the same M150 list-file flow;
- no new Stage-A coverage auxiliary;
- no new Stage-A candidate-eval selection metric;
- no new row categories or row weights in C6;
- no larger `feature_k`;
- no different `n_features`;
- no new active clipping profile;
- no adaptive gate;
- no post-hoc profile tuned to a benchmark;
- no direct use of official test qrels in training;
- no `latest` checkpoint handoff.

If any of these are needed, the run must be renamed outside M190.

## Data Contract

M190 uses PPLX `1024` embeddings only.

```text
model: perplexity-ai/pplx-embed-v1-0.6B
input_dims: 1024
similarity: cosine
normalize: false
dtype: fp32-compatible stored values
```

The data must be materialized under a new root, for example:

```text
/home/huoju/leask/runs/ii42-m190-canonical-pplx1024-corpus
```

The root must expose M150-style list inputs:

```text
documents.files
queries.files
```

Unlike M180, these list files must not introduce row-weighted source scheduling.
They are only path lists for the legacy M150 Stage-A loader.

Required source families:

| Source family | Stage-A use | Notes |
| --- | --- | --- |
| BEIR15 corpus docs | yes | Full official corpus docs. |
| BEIR non-test queries | yes | Exclude test query ids when local split metadata is available. |
| Wikipedia docs/query-style rows | yes | Prepared as ordinary M150-style input files. |
| arXiv docs/query-style rows | yes | Prepared as ordinary M150-style input files. |
| PubMed docs/query-style rows | yes | Prepared as ordinary M150-style input files. |

Important: source balancing must be done by materialization policy, not by
changing the Stage-A training loop. If neutral data volume needs adjustment,
build explicit files for that M190 run and record their row counts.

## Required Preflight Checks

M190 must fail fast if any check fails.

### Global Checks

- every JSONL row has exactly `1024` embedding values;
- no mixed `768`/`1024` roots;
- every listed file exists and is non-empty;
- expected row count equals counted row count;
- `documents.files` and `queries.files` are deterministic and saved;
- all BM25 caches are either absent and rebuilt or match the current root;
- ClearML points to the current ii42 server;
- run names start with `ii42-m190-`.

### BEIR Split Checks

- official test qrel labels are never used in Stage A/B/C training;
- C6 train rows use only `fiqa:train,nfcorpus:train,scifact:train`;
- C6 validation rows use only `fiqa:dev,nfcorpus:dev,quora:dev`;
- test/all-test surfaces are evaluation only.

### Checkpoint Checks

- Stage B consumes M190 Stage-A `best`;
- C6 consumes M190 Stage-B fusionnorm `best`;
- official eval records the exact checkpoint path;
- `latest` is never used unless a separate note explicitly justifies it.

## Execution Plan

### M190.0 Inventory And Audit

Produce:

```text
ii42-m190-corpus-inventory.json
ii42-m190-dimension-audit.json
ii42-m190-split-audit.json
```

The inventory must include:

- row counts per source family;
- document/query counts;
- embedding dimension histogram;
- file paths;
- SHA256 or size/mtime evidence for major files;
- BEIR split leakage checks.

Do not start training until the audit passes.

### M190.1 Stage A Canonical Replay

Run name:

```text
ii42-m190-m150a1-replay-stagea-v1
```

Stage A uses the M150A1 protocol with the new M190 list files.

Expected handoff:

```text
/home/huoju/leask/runs/ii42-m190-m150a1-replay-stagea-v1/bm25sae_stagea_best.pt
```

Immediate validation:

- training summary;
- source counts;
- best-event metrics;
- official BEIR representative gate;
- continuity full-corpus gate if available.

Promotion condition for moving to Stage B:

- no dimension or split audit failure;
- no obvious collapse versus M150A1 Stage A on comparable gate;
- checkpoint selected by `best`, not `latest`.

### M190.2 Stage B Fusionnorm Replay

Run name:

```text
ii42-m190-m150-fusionnorm-v1
```

This stage must match the M150 Stage-B from-best/fusionnorm shape. The only
path changes are:

- `STAGE_A_RUN=ii42-m190-m150a1-replay-stagea-v1`;
- M190 train/eval candidate roots;
- M190 BM25 cache;
- M190 output names.

Promotion condition for moving to C6:

- Stage-B full-corpus BM25+SAE improves over Stage A on the same gate;
- learned scale values are recorded;
- no fanout explosion beyond the same order as M150 Stage-B.

### M190.3 C6 Official Dense-Miss Replay

Run name:

```text
ii42-m190-m150-c6-official-dense-miss-v1
```

This stage must match C6 exactly except for the root paths:

- `STAGE_B_RUN=ii42-m190-m150-fusionnorm-v1`;
- `MATERIALIZED_ROOT=<M190 materialized root>`;
- `EVAL_CORPUS=<M190 continuity/eval corpus root>`;
- `EVAL_BASELINE=<M190 Stage-B dense baseline>`;
- `EVAL_BM25_CACHE=<M190 eval BM25 cache>`.

The row-building logic, row weights, active-k, and loss args must stay
unchanged.

### M190.4 Official Full-Corpus Gate

After C6, run:

- continuity full-corpus comparison;
- official BEIR15 full-corpus per-dataset matrix;
- physical matrix.

The final report must separate:

- M150 continuity comparison;
- official BEIR15 comparison;
- Stage-A representation quality;
- Stage-B fusionnorm effect;
- C6 dense-miss correction effect.

## Metrics To Report

Quality:

- Recall@20;
- Recall@100;
- MRR@20;
- NDCG@10;
- MAP@100.

Physical cost:

- candidate docs;
- SAE postings;
- BM25 postings if available;
- accumulator entries;
- rerank terms;
- payload size;
- mean/p95 latency when available.

Sources to report separately:

- BM25;
- dense;
- SAE-only;
- BM25+dense score fusion;
- BM25+SAE score fusion;
- BM25+SAE RRF if available.

## Acceptance Criteria

M190 is considered successful if it reaches or exceeds the M150A1 C6
continuity result without changing training shape:

| Metric | M150A1 C6 target |
| --- | ---: |
| SAE Recall@100 | `0.3899` |
| BM25+SAE Recall@100 | `0.3841` |
| BM25+SAE MRR@20 | `0.4271` |
| BM25+SAE NDCG@10 | `0.3010` |
| BM25+SAE MAP@100 | `0.2008` |

For official BEIR15, M190 must not be judged by a single aggregate only. The
report must include per-dataset regressions, especially:

- `fiqa`;
- `scidocs`;
- `trec-covid`;
- `nq` or `hotpotqa`;
- `msmarco`.

## Failure Interpretation

If M190 fails, the interpretation is constrained:

- If Stage A fails, the issue is data distribution or dimension materialization,
  not Stage B/C.
- If Stage A matches but Stage B fails, the candidate-row root or fusionnorm
  replay is not equivalent.
- If Stage B matches but C6 fails, the official dense-miss row surface or C6
  path mapping is not equivalent.
- If all three stages match continuity but official BEIR15 fails, then M150 was
  overfit to the continuity surface and product generalization needs a new
  plan.

Do not change multiple variables in response to a failure. Open a new named
experiment after recording the exact failed stage.

## Implementation Checklist

Before launching M190, create or verify:

- `scripts/run_ii42_m190_stagea_replay_spark.sh`;
- `scripts/run_ii42_m190_stageb_fusionnorm_replay_spark.sh`;
- `scripts/run_ii42_m190_c6_replay_spark.sh`;
- a corpus audit script that validates `1024` dimensions and split exclusion;
- a result comparison script that prints M150A1, M150A2, M150A1 C6, and M190.

The runner scripts should be thin wrappers around the M150 scripts. They should
not duplicate or reinterpret the training logic.

## Current Decision

Proceed with M190 only after the data audit is green. The first executable goal
is not training. It is proving that the new corpus root is complete, PPLX
`1024`-only, and compatible with the frozen M150 replay protocol.

## Replay-Actual Data Narrowing

The first executable line is now `M190-replay-actual`, not the broad neutral
line. The reason is that the available Wikipedia/arXiv/PubMed split is only a
small sampled surface and does not satisfy the intended high-volume neutral
contract. Broad data must be rebuilt separately and must not be faked through a
hidden sampler or accidental repeat policy.

`M190-replay-actual` therefore uses only the actual M150A BEIR15 materialized
surface:

```text
root: /home/huoju/leask/runs/ii42-m190-replay-actual-beir15-pplx1024
source: /home/huoju/leask/runs/m150-beir-full-pplx
datasets: official BEIR15, all 15 datasets
docs: full official corpus documents
queries: train/dev official queries plus synthetic document-derived queries
query_repeat: 2
```

Important split rule: full corpus documents are allowed because retrieval
systems index full corpora. Official test qrel labels and test-only official
query ids are not allowed in training. Synthetic document-derived query rows are
allowed because they do not include relevance labels.

The source M150 root had one materialization issue: `climate-fever` had `863`
duplicate document embedding rows in `documents.jsonl`. The M190 replay root
keeps the original source root untouched and stores a deduplicated
`climate-fever/documents.jsonl` copy while symlinking the other clean files.

Expected audited counts:

| Item | Count |
| --- | ---: |
| Datasets | `15` |
| Documents | `33,860,494` |
| Queries | `1,021,159` |
| Safe train/dev official query ids | `731,632` |
| Expected Stage-A records per epoch | `35,902,812` |
| Embedding dimensions | `1024` only |
| Test qrel query overlap | `0` |

The replay runner is:

```text
scripts/run_ii42_m190_replay_actual_stagea_spark.sh
```

It is a thin wrapper around the M150 Stage-A pretraining script. It must keep:

- `stream_mode=legacy_doc_query`;
- `query_repeat=2`;
- `n_features=16384`;
- `feature_k=96`;
- `batch_size=384`;
- `epochs=3`;
- `topk_iters=24`;
- no M180 row-weighted manifest;
- no new coverage/candidate-eval auxiliary.

Verified audit output:

```text
audit: /home/huoju/leask/runs/ii42-m190-replay-actual-manifest/replay_actual_audit.json
documents.files: /home/huoju/leask/runs/ii42-m190-replay-actual-manifest/documents.files
queries.files: /home/huoju/leask/runs/ii42-m190-replay-actual-manifest/queries.files
failures: []
```

The verified M190 replay root now passes:

- `15` dataset roots present;
- `15` document list entries and `15` query list entries;
- `33,860,494` document rows;
- `1,021,159` query rows;
- `35,902,812` expected records per epoch with `query_repeat=2`;
- sampled document/query embeddings are `1024` dimensional;
- `0` official test-query overlap;
- every embedded file line count matches its prepare summary.
