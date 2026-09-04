# ii42 M1142 Native Protected-Tail Bridge Report

## Purpose

M1140/M1141 found a structurally clean protected-tail replay:

1. keep the base run top ranks fixed;
2. append the tail run without duplicates;
3. append the remaining base ranking.

M1142 checks the engineering bridge needed to move that policy from ranking
exports into the existing native II-42 DB/plugin path.

## Implementation

Added `scripts/evaluate_m1142_native_protected_tail_pg.py`.

The script reuses existing native pieces:

- `query_native_hybrid_feature_rows` from M673 for native SQL candidate
  retrieval;
- table-backed atom scoring for unpublished checkpoint streams;
- optional M1132-style max-abs score shape:
  `lex + alpha * signed_power(atom, gamma)`;
- `ii42_model_query_atoms` backed indexes already present in PostgreSQL;
- native BM25 candidate generation and `ii42_hybrid_fuse_candidates`;
- existing qrels and metric functions from `evaluate_p1_native_atoms_pg.py`.

It does not create a new index lifecycle and does not scan the corpus in
Python.  It only composes two same-query native candidate streams.  For
unpublished checkpoint streams, the table backend keeps candidate generation
inside PostgreSQL and uses Python only for the protected-tail rank composition.

## Smoke Runs

Dataset: `nfcorpus`

Query limit: `10`

Base stream:

- index: `ii42_shared15.docs_nfcorpus_p1_a0125_idx`
- atom table: `ii42_p1_shared15.nfcorpus_p1_a0125_atoms`
- query atoms:
  `runs/m603_p1_product_atoms_shared15_v1/nfcorpus/queries.p1_atoms.jsonl`
- weights: semantic `0.875`, BM25 `0.125`

### Smoke A: near-identical tail source

Tail stream:

- index: `ii42_shared15.docs_nfcorpus_p1_a010_idx`
- atom table: `ii42_p1_shared15.nfcorpus_p1_a010_atoms`
- weights: semantic `0.875`, BM25 `0.125`

Output:

- `runs/m1142_native_protected_tail_smoke_v1/nfcorpus_m1142_native_protected_tail.json`
- `runs/m1142_native_protected_tail_smoke_v1/nfcorpus_m1142_native_protected_tail.md`

| Row | CUB | Recall@100 | MAP@100 | NDCG@10 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| base | 0.626658 | 0.269585 | 0.122159 | 0.388868 | 0.703571 |
| tail | 0.626658 | 0.269585 | 0.122159 | 0.388868 | 0.703571 |
| protected-tail | 0.626658 | 0.269585 | 0.122159 | 0.388868 | 0.703571 |

Result: rejected only because the guard requires positive recall movement.
This is expected: this tail stream is effectively identical for the smoke
queries.  The bridge still wrote eval-style JSON under:

`runs/m1142_native_protected_tail_smoke_v1/evals/m1142_native_smoke/...`

### Smoke B: deliberately different tail source

Tail stream:

- index: `ii42_shared15.docs_nfcorpus_p1_a000_idx`
- atom table: `ii42_p1_shared15.nfcorpus_p1_a000_atoms`
- weights: semantic `1.0`, BM25 `0.0`

Output:

- `runs/m1142_native_protected_tail_smoke_v1/nfcorpus_m1142_native_tail_a000sem.json`
- `runs/m1142_native_protected_tail_smoke_v1/nfcorpus_m1142_native_tail_a000sem.md`

Delta vs base:

| Metric | Delta |
| --- | ---: |
| CUB | -0.004879 |
| Recall@100 | -0.013382 |
| MAP@100 | -0.003215 |
| NDCG@10 | +0.000000 |
| MRR@20 | +0.000000 |

Result: rejected.  This confirms the composer is not an empty no-op: when the
tail stream differs, the native protected-tail path changes the ranking and can
hurt metrics if the tail source is wrong.

## Current Blocker

The native bridge is ready, and a checkpoint-to-atom exporter now exists, but
the actual M1140 route cannot be fully claimed as native until the full
M1129/M1137 atom streams are published into PostgreSQL.

## Checkpoint Atom Export Smoke

Added `scripts/export_m1142_sae_checkpoint_atoms.py`.

This script materializes a trained `TopKTokenSAE` checkpoint into the existing
product atom JSONL contract consumed by `load_p1_atom_jsonl_to_pg.py`.

Smoke checkpoints:

`/Volumes/Betty/Tmp/ii42-m1000/m1129-shared15-listwise050-control-export-v1/m1129_shared15_listwise050_control_s1050.pt`

`/Volumes/Betty/Tmp/ii42-m1000/m1137-dual-gamma-shared15-v1/m1137_dual_gamma_shared15_s1050.pt`

Smoke inputs:

- `/Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared/nfcorpus/documents.jsonl`
- `/Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared/nfcorpus/queries.jsonl`

Smoke outputs:

- `runs/m1142_sae_atom_export_smoke_v1/m1129/nfcorpus/documents.p1_atoms.jsonl`
- `runs/m1142_sae_atom_export_smoke_v1/m1129/nfcorpus/queries.p1_atoms.jsonl`
- `runs/m1142_sae_atom_export_smoke_v1/m1137/nfcorpus/documents.p1_atoms.jsonl`
- `runs/m1142_sae_atom_export_smoke_v1/m1137/nfcorpus/queries.p1_atoms.jsonl`

Result:

| Checkpoint | Split | Rows | Config post_active_k | Observed atom counts |
| --- | --- | ---: | ---: | --- |
| M1129 | documents | 2 | 96 | 21, 14 |
| M1129 | queries | 2 | 80 | 1, 1 |
| M1137 | documents | 2 | 96 | 15, 15 |
| M1137 | queries | 2 | 80 | 1, 2 |

The low observed counts are expected for this exporter: token activations are
pooled by atom, clipped by `post_active_k`, and then only positive impacts are
kept.  The native loader contract does not require a fixed K; it requires
non-empty atom ids, no duplicate atom ids, finite impacts, and aligned
`atom_ids` / `atom_impacts` / `atom_sources` lengths.  The smoke rows satisfy
that contract.

MPS runtime also worked locally for both checkpoints and this model:

`perplexity-ai/pplx-embed-v1-0.6B`

## Full Nfcorpus Table-Backed Replay

Full M1129/M1137 nfcorpus atom roots were exported locally:

- `runs/m1142_sae_atom_export_nfcorpus_v1/m1129/nfcorpus/documents.p1_atoms.jsonl`
- `runs/m1142_sae_atom_export_nfcorpus_v1/m1129/nfcorpus/queries.p1_atoms.jsonl`
- `runs/m1142_sae_atom_export_nfcorpus_v1/m1137/nfcorpus/documents.p1_atoms.jsonl`
- `runs/m1142_sae_atom_export_nfcorpus_v1/m1137/nfcorpus/queries.p1_atoms.jsonl`

Readiness checks passed for row counts and sampled schema:

| Stream | Documents | Queries | Notes |
| --- | ---: | ---: | --- |
| M1129 | 2063 | 100 | no empty document atoms |
| M1137 | 2063 | 100 | 1 empty document atom row, skipped during table load |

The M1137 document table load used `--skip-empty-atoms` and loaded 2062 rows.
Some query rows also have empty atom lists; M1142 now allows empty query atoms
so those queries can still contribute BM25/lexical candidates instead of being
dropped.

Loaded tables:

- `ii42_m1142.nfcorpus_m1129_atoms`
- `ii42_m1142.nfcorpus_m1137_atoms`

Faithful shape replay used:

- base: M1129 table, `alpha=0.60`, `gamma=1.00`
- tail: M1137 table, `alpha=0.50`, `gamma=0.50`
- order mode: `m1132`

### Nfcorpus Result

| Policy | k | Guard | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| protect20_then_tail | 1000 | rejected | -0.006291 | +0.031334 | +0.007942 | +0.000000 | +0.000000 |
| protect5_then_tail | 1000 | rejected | -0.006912 | +0.030815 | +0.016192 | +0.025579 | +0.002041 |
| protect20_then_tail | 2000 | accepted | +0.000523 | +0.040135 | +0.008904 | +0.000000 | +0.000000 |
| protect5_then_tail | 2000 | accepted | +0.000523 | +0.039722 | +0.017483 | +0.025556 | +0.001596 |

Best current local replay:

`protect5_then_tail`, `k=2000`

Output:

- `runs/m1142_native_protected_tail_nfcorpus_v1/nfcorpus_m1142_table_protect5_k2000.json`
- `runs/m1142_native_protected_tail_nfcorpus_v1/nfcorpus_m1142_table_protect5_k2000.md`

This is the first M1142 product-path-adjacent positive replay: the base/tail
checkpoint streams were materialized from actual checkpoints, loaded into
PostgreSQL atom tables, queried through native SQL candidate generation, and
then composed with protected-tail replay.

## Remaining Publishing Blocker

Reason: the local M1129/M1137 artifacts currently available under
`/Volumes/Betty/Tmp/ii42-m1000/` are ranking exports, checkpoints, and partial
JSON summaries.  They do not include product atom roots equivalent to:

- `documents.p1_atoms.jsonl`
- `queries.p1_atoms.jsonl`

and PostgreSQL did not initially expose M1129/M1137-specific atom tables or
model indexes.  Nfcorpus atom tables now exist under `ii42_m1142`, but full
shared15 tables and model-backed plugin indexes still do not exist.

Therefore:

- M1142 proves the native composition interface exists and works.
- M1142 now also proves that M1129/M1137 checkpoints can be exported into the
  atom JSONL shape needed by the native publisher.
- M1142 proves a positive table-backed native replay on nfcorpus.
- M1142 still does not yet prove the route on full shared15 or model-backed
  plugin indexes.

## Shared15 Partial Table-Backed Replay

Full shared15 atom export is running under:

`runs/m1142_sae_atom_export_shared15_v1/`

Before waiting for all 15 datasets, the first completed datasets were loaded
and replayed through the same table-backed native SQL path.

Primary cap-free output:

- `runs/m1142_shared15_table_replay_partial_k5000_v1/summary.json`
- `runs/m1142_shared15_table_replay_partial_k5000_v1/summary.md`

Earlier k=2000 output:

- `runs/m1142_shared15_table_replay_partial_v1/summary.json`
- `runs/m1142_shared15_table_replay_partial_v1/summary.md`

The k=5000 replay is the primary shared15 surface because it removes the
candidate-cap artifact seen on larger rows such as `msmarco`.  The output root
keeps its original `partial` name, but it now contains all 15 shared15
datasets.

Full shared15 macro at k=5000:

| protect_k | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | -0.000628 | +0.012608 | +0.007468 | +0.002665 | +0.000379 |
| 20 | -0.000624 | +0.012743 | +0.004062 | +0.000000 | +0.000000 |

Per dataset:

| Dataset | protect_k | Guard | dCUB | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| arguana | 5 | accepted | +0.000000 | +0.010000 | +0.001994 | +0.002302 | +0.000660 |
| arguana | 20 | accepted | +0.000000 | +0.010000 | +0.000387 | +0.000000 | +0.000000 |
| climate-fever | 5 | accepted | +0.000000 | +0.023833 | +0.008793 | +0.014927 | +0.000731 |
| climate-fever | 20 | accepted | +0.000000 | +0.023833 | +0.001177 | +0.000000 | +0.000000 |
| cqadupstack | 5 | accepted | +0.000000 | +0.024049 | +0.007892 | +0.004122 | +0.001930 |
| cqadupstack | 20 | accepted | +0.000000 | +0.023216 | +0.003799 | +0.000000 | +0.000000 |
| dbpedia-entity | 5 | accepted | +0.000000 | +0.007230 | +0.015248 | +0.002338 | +0.000714 |
| dbpedia-entity | 20 | accepted | +0.000000 | +0.005842 | +0.010379 | +0.000000 | +0.000000 |
| fever | 5 | rejected | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| fever | 20 | rejected | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 |
| fiqa | 5 | rejected | +0.000000 | +0.025190 | +0.004551 | -0.004421 | +0.000427 |
| fiqa | 20 | accepted | +0.000000 | +0.025190 | +0.002770 | +0.000000 | +0.000000 |
| hotpotqa | 5 | rejected | +0.000000 | +0.000000 | -0.000481 | -0.001846 | +0.000000 |
| hotpotqa | 20 | rejected | +0.000000 | +0.000000 | -0.000072 | +0.000000 | +0.000000 |
| msmarco | 5 | rejected | +0.000000 | +0.006523 | +0.016167 | -0.011786 | +0.000000 |
| msmarco | 20 | accepted | +0.000000 | +0.006911 | +0.011255 | +0.000000 | +0.000000 |
| nfcorpus | 5 | accepted | +0.000000 | +0.039637 | +0.017456 | +0.025556 | +0.001596 |
| nfcorpus | 20 | accepted | +0.000000 | +0.040051 | +0.008877 | +0.000000 | +0.000000 |
| nq | 5 | rejected | +0.000000 | +0.000000 | +0.001110 | +0.003443 | +0.000790 |
| nq | 20 | rejected | +0.000000 | +0.000000 | +0.000367 | +0.000000 | +0.000000 |
| quora | 5 | accepted | +0.000000 | +0.000175 | +0.001551 | +0.000112 | +0.000000 |
| quora | 20 | accepted | +0.000000 | +0.000175 | +0.000505 | +0.000000 | +0.000000 |
| scidocs | 5 | rejected | +0.000000 | +0.026000 | +0.007961 | +0.002375 | -0.002645 |
| scidocs | 20 | accepted | +0.000000 | +0.030000 | +0.004308 | +0.000000 | +0.000000 |
| scifact | 5 | rejected | +0.000000 | +0.008000 | -0.000637 | +0.000068 | +0.001488 |
| scifact | 20 | accepted | +0.000000 | +0.008000 | +0.000410 | +0.000000 | +0.000000 |
| trec-covid | 5 | rejected | -0.009421 | +0.013014 | +0.014126 | +0.010383 | +0.000000 |
| trec-covid | 20 | rejected | -0.009358 | +0.012458 | +0.011871 | +0.000000 | +0.000000 |
| webis-touche2020 | 5 | rejected | +0.000000 | +0.005473 | +0.016283 | -0.007598 | +0.000000 |
| webis-touche2020 | 20 | accepted | +0.000000 | +0.005473 | +0.004895 | +0.000000 | +0.000000 |

Interpretation:

- the signal now spans the full shared15 table-backed native surface;
- `protect5_then_tail` has the better macro top metrics, but `fiqa` shows it
  can buy Recall/MAP by spending NDCG@10;
- `protect20_then_tail` is lower upside but safer on `fiqa`, `scidocs`,
  `scifact`, `msmarco`, and `webis-touche2020`;
- `hotpotqa` shows a true harm-risk row: the tail adds no Recall and can still
  perturb MAP/NDCG;
- `msmarco` confirms the candidate-cap issue: k=2000 made CUB negative, while
  k=5000 restores CUB to zero and lets `protect20_then_tail` pass;
- `nq` is a top-metric-only row: MAP/NDCG/MRR improve slightly, but Recall does
  not move, so the recall-focused guard rejects it;
- `quora` is accepted but very low-amplitude, with many empty document atom
  rows during table load: M1129 skipped 74 rows and M1137 skipped 86 rows;
- `scidocs` matches the `fiqa` pattern: `protect5` improves Recall/MAP but
  spends MRR, while `protect20` passes the guard;
- `scifact` also supports the safer `protect20` fallback: `protect5` buys
  Recall/MRR but loses MAP;
- `trec-covid` is a cap-risk row even at k=5000: Recall/MAP/NDCG move
  positive, but CUB drops by about 0.0094, so the guard correctly rejects it;
- `webis-touche2020` follows the same protect5/protect20 split as `fiqa`:
  protect5 buys MAP/Recall while spending NDCG, protect20 keeps a conservative
  accepted Recall/MAP gain;
- the full shared15 table replay has accepted positive movement on 11/15
  datasets under at least one protection policy;
- `fever` is a neutral no-op row where all deltas are exactly zero;
- `hotpotqa` is a harm-risk row where appending tail creates no Recall gain
  and slightly hurts MAP/NDCG;
- the next route should not be a fixed `protect_k`; it should be a
  dataset-agnostic tail-helpfulness/harm-risk gate over native candidate
  features.
- `protect5_then_tail` is the stronger shape for top metrics;
- `protect20_then_tail` preserves Recall movement but is too conservative for
  MAP/NDCG/MRR improvement;
- full shared15 validates the bridge and the signal, but also rejects a fixed
  protect-k policy as the final route.

## Next Step

The next useful step is not another alpha/gamma experiment and not a fixed
`protect_k` search.  M1142 should branch into a query-level, dataset-agnostic
tail-helpfulness/harm-risk gate over native candidate features:

1. keep M1129 as the protected base and M1137 as the tail source;
2. compute per-query features from the native streams: base/tail score margin,
   tail insertion depth, BM25 lexical support, atom overlap, fanout/idf shape,
   and whether tail candidates cross top100;
3. choose `protect5`, `protect20`, or abstain per query;
4. keep hard guards on CUB, NDCG@10, MRR@20, and Recall@100;
5. rerun full shared15 through the same table-backed native path before any
   model-backed index promotion.

## Verdict

Keep M1142 as the engineering bridge.  The route is no longer blocked at
checkpoint export or shared15 breadth: full shared15 table-backed replay is
complete and shows real positive movement.  The fixed `protect_k` policy is
not promotable, because some rows spend CUB/NDCG/MRR to buy Recall/MAP.

The remaining gate is policy and productization: first build a query-level
tail-helpfulness/harm-risk selector over native features, then replay through
the model-backed plugin index only if that selector preserves the full
shared15 guard.
