# M603 / P1.1 Native Fixed-Alpha Recall Matrix

Status: native regression matrices are frozen at official P1-native coverage
`12/15` after the full local `nq`, `dbpedia-entity`, and `climate-fever`
generation-shards evaluations. Clean-heldout promotion evidence remains
incomplete. Latest frozen checkpoint: `2026-07-05T17:09:13Z`.

The local `msmarco` PostgreSQL atom load was stopped before completion after
the scorer/reranker gap was identified as the next bottleneck. The interrupted
COPY left no valid rows, and `ii42_p1.msmarco_p1_a000_atoms` was truncated back
to an empty `16 kB` table. `fever`, `hotpotqa`, and `msmarco` are therefore
paused and must not be interpreted as completed M603 rows. The next planned
work is M604/M605 scorer-gap audit and global reranker recovery, not continued
fixed-alpha matrix expansion.

This report tracks the native database/index implementation for the P1 fixed
alpha matrix. The goal is to evaluate `P1-a010` and `P1-a0125` through the
II-42 database/index path, not through a separate offline scorer. For small and
medium rows this is the existing model-backed generation lifecycle. For
official-full large rows, the current continuation is local-first
generation-sharded native querying because the single-generation bytea
publisher is not a scalable shape for multi-million-document official corpora.

Current acceptance state:

- Native DB/plugin path is validated for the completed shared15 rows:
  `ii42_model_generation_upsert_from_table` publishes successfully and
  `ii42_model_deployment_status` reports `atom_query_ready=true`,
  `generation_ready=true`, and `scoring_ready=true` for all completed
  shared15 `M549U`/`P1-a010`/`P1-a0125` indexes.
- Native shared15 matrix is complete for `15/15` datasets. All shared15 P1
  atom surfaces passed exact document/query row-count and schema gates before
  native publish/eval.
- Native broad10 is now being moved from legacy/offline anchor toward the same
  DB/plugin lifecycle. The builder supports MTEB-style custom task names in
  `--expected-from-files` mode. The broad10 root sync is complete for all
  `10/10` MTEB tasks, and the clean `ii42_broad10` schema rebuild is complete.
  Full BM25/dense native baselines are now available for all `10/10` broad10
  tasks. A stale smoke schema was dropped after a local BM25 index rejected
  `TRUNCATE`; the rebuild was from the synced roots, not from partial DB state.
- Native broad10 ArguAna product atoms passed strict row-count and schema gate:
  `documents=8674/8674`, `queries=1401/1401`. The first MPS attempt stopped
  on memory pressure at `3240/8674` documents; the output ended on a complete
  JSONL line, and the job was resumed successfully with `BATCH_SIZE=6`.
- Native broad10 `CQADupstackGamingRetrieval` product atoms passed strict
  row-count and schema gate: `documents=45301/45301`,
  `queries=1595/1595`. The `BATCH_SIZE=2` resume stopped on MPS memory
  pressure at `43688/45301`; the JSONL tail was valid, and the same existing
  product atom runner resumed safely with `BATCH_SIZE=1`.
- Native broad10 `CQADupstackGamingRetrieval` fixed-alpha matrix is complete
  through the DB/plugin path. On this row, `P1-a0125` beats `P1-a010` on
  `NDCG@10`, `MAP@100`, `Recall@100`, `MRR@20`, dense overlap, and candidate
  upper bound.
- Native broad10 `CQADupstackUnixRetrieval` product atoms passed strict
  row-count and schema gate after bounded line-range shard recovery:
  `documents=47382/47382`, `queries=1072/1072`. The earlier long MPS process
  stopped at `4651/47382` with `Killed: 9`; the valid partial was preserved as
  the seed shard, all shards were validated by
  `merge_p1_atom_jsonl_shards.py`, and the merged document file now has the
  exact source row count. The fixed-alpha matrix is complete through the
  DB/plugin path, and `P1-a0125` beats `P1-a010` on all tracked metrics for
  this row.
- Native broad10 `SCIDOCS` product atoms were generated on `spark-1`, synced
  locally, and passed strict row-count/schema gate:
  `documents=25657/25657`, `queries=1000/1000`. Its fixed-alpha matrix is
  complete through the DB/plugin path, and `P1-a0125` again beats `P1-a010` on
  all tracked metrics. Native broad10 `FiQA2018` product atoms were generated
  on `spark-1`, synced locally, and passed strict row-count/schema gate:
  `documents=57638/57638`, `queries=648/648`. Its fixed-alpha matrix is also
  complete through the DB/plugin path, and `P1-a0125` beats `P1-a010` on all
  tracked metrics.
- Native broad10 matrix is now complete for `10/10` rows. The final
  `Touche2020Retrieval.v3` product atoms were merged from exact,
  non-overlapping validated ranges and passed strict row-count/schema gate:
  `documents=303732/303732`, `queries=49/49`. Its fixed-alpha matrix is
  complete through the DB/plugin path, and `P1-a0125` beats `P1-a010` on
  `NDCG@10`, `MAP@100`, `Recall@100`, and `MRR@20`.
- Native official BEIR15 partial coverage is now `12/15` after completing
  `quora` through the DB/plugin path, plus `nq` and `dbpedia-entity` through
  the local generation-shards backend. `climate-fever` now also completed
  through the same local generation-shards backend. `quora` was generated from local
  precomputed
  official root embeddings with `compile_m603_p1_atoms_from_root_jsonl.py`,
  passed strict atom gates with `documents=522931/522931` and
  `queries=10000/10000`, then completed native publish/eval for `M549U`,
  `P1-a010`, and `P1-a0125`. `nq` uses the same strict-gated atom surface
  (`documents=2681468/2681468`, `queries=3452/3452`) and full official
  qrels-bearing query split through 27 native generation shards.
  `dbpedia-entity` was loaded locally into
  `ii42_p1.dbpedia_entity_p1_a000_atoms` with `4635922/4635922` rows,
  published as 47 native generation shards, and evaluated on all 400 official
  qrels-bearing queries. `climate-fever` was loaded locally into
  `ii42_p1.climate_fever_p1_a000_atoms` with `5416593/5416593` rows,
  published as 55 native generation shards, and evaluated on all 1535 official
  qrels-bearing queries.
- Official full continuation is now local-first. All remaining official rows
  now have complete strict-gated product atom files on the local atom root:
  `nq=2681468/2681468 documents, 3452/3452 queries`;
  `dbpedia-entity=4635922/4635922 documents, 400/400 queries`;
  `hotpotqa=5233329/5233329 documents, 7405/7405 queries`;
  `climate-fever=5416593/5416593 documents, 1535/1535 queries`;
  `fever=5416568/5416568 documents, 6666/6666 queries`; and
  `msmarco=8841823/8841823 documents, 43/43 queries`.
- The original `nq` native advance exposed two publish-time scalability
  bottlenecks. First, `ii42_model_generation_build_plan` duplicated expensive
  atom-table status validation before
  `ii42_model_generation_upsert_from_table`; the runner now supports
  `--skip-build-plan` while preserving the authoritative publish validation.
  Second, `ii42_evidence_atom_build_generation_from_table` ultimately
  materializes the full official row into large arrays and a single generation
  bytea. For `nq`, that means roughly 343M postings before serialization.
  This is a generation-lifecycle shape limit, not a P1 atom/model failure.
- The local engineering continuation therefore uses sharded native
  generations for official-full large rows. `publish_p1_atom_shards_to_ii42_pg.py`
  publishes deterministic doc-ordered shards, and
  `evaluate_p1_native_atoms_pg.py --semantic-backend generation_shards` queries
  each shard through `ii42_evidence_atom_query_by_id_with_ids`, merges semantic
  candidates by `doc_id`, and preserves the same fixed-alpha hybrid fusion
  contract through `ii42_hybrid_fuse_candidates`.
- The `nq` doc-ord postings backend smoke completed successfully for
  `limit_queries=5`. It built `ii42_p1.nq_p1_a000_atoms_postings` in the
  Betty tablespace and returned `query_count=5`, `NDCG@10=0.551007`,
  `Recall@100=1.0`, and `dense_overlap@100=0.602`. The generated postings
  table is about `24GB`, its `(atom_id, doc_ord) INCLUDE (impact)` index is
  about `10GB`, and the doc map is about `212MB`.
- A direct `limit_queries=20` query-only smoke against the same exact SQL
  postings backend was canceled after several minutes, and a bounded
  top-128-per-atom SQL postings-head approximation failed quality (`R@100=0.05`
  on a 20-query `nq` smoke). The conclusion is that exact SQL postings remains
  useful as a correctness/audit backend, but the promoted local engineering
  route for official-full rows is now native generation shards, not SQL
  posting-list aggregation.
- The official `cqadupstack` native eval exposed a query-time implementation
  bottleneck: the hybrid path rebuilt `doc_ord` with a window function for
  every query. `evaluate_p1_native_atoms_pg.py` now creates one temporary
  `doc_ord -> docs.ctid` map per dataset/eval and reuses it in
  `query_native_hybrid`. This is an evaluator engineering fix; it does not
  change model weights, atom payloads, BM25 candidates, or fusion math.
- `P1-a0125` is the leading fixed-alpha candidate on all completed native
  engineering/regression surfaces: full 15-dataset shared15, 12-dataset
  official BEIR15 partial coverage including full `nq`, `dbpedia-entity`, and
  `climate-fever`, and full 10-dataset broad10. It beats `P1-a010` on all
  tracked macro metrics on all three current surfaces.
- Contamination boundary: every completed M603/P1.1 surface in this report is
  labeled `seen_regression` in the regenerated matrix JSON/Markdown outputs.
  These rows are valid DB-native engineering evidence and regression evidence,
  but they are not clean-heldout promotion evidence. A clean promotion claim
  still requires a separately frozen heldout surface that was not used for
  P1/M549U/M601 debugging, alpha comparison, route selection, or reporting.
- `advance_m603_p1_completed_dataset.py` now defaults to
  `--alphas 0,0.10,0.125`. This fixes the completed-dataset advance path so
  future rows generate the `M549U` (`alpha=0`) native eval together with
  `P1-a010` and `P1-a0125` before compiling a combined matrix.
- `advance_m603_p1_completed_dataset.py` also forwards compiler split labels
  and defaults unlisted datasets to `seen_regression`. This prevents automated
  row advances from accidentally creating unlabeled or false-clean promotion
  evidence.

## Current Gap And Split Policy

M603 currently proves the native DB/plugin path and fixed-alpha regression
surface. It does not yet prove clean promotion of `P1-a0125`.

Completed native surfaces:

| Surface | Coverage | Split label | Evidence use |
| --- | ---: | --- | --- |
| `shared15` | 15/15 | `seen_regression` | engineering regression and root sanity |
| `official BEIR15 native local` | 12/15 | `seen_regression` | official-root regression subset |
| `broad10` | 10/10 | `seen_regression` | engineering regression and MTEB sanity |

Official BEIR15/P1-native remaining rows:

| Dataset | Documents | Doc shards | Queries | Query shards | Status |
| --- | ---: | ---: | ---: | ---: | --- |
| `fever` | 5416568 | 22 | 6666 | 1 | atoms ready, strict gate passed; queued for local generation-shards native eval |
| `hotpotqa` | 5233329 | 21 | 7405 | 1 | atoms ready, strict gate passed; queued for local generation-shards native eval |
| `msmarco` | 8841823 | 36 | 43 | 1 | loading full document atom table locally into `ii42_p1.msmarco_p1_a000_atoms` |

The next official-full step is to reuse the generation-shards backend for the
remaining three strict-gated official rows. The older single-bytea publisher
remains useful for smaller rows, but should not be used as the official-full
large-row gate.

Machine-readable current status:

`runs/m603_p1_native_status_v1/m603_p1_native_current_status.json`

Active official-full continuation:

- No active remote job is required for `quora`. The earlier spark-1 shard route
  was superseded by local root-to-P1 atom compilation from the official root
  JSONL files. Future official-full rows should prefer the local engineering
  route when the local precomputed root embeddings are available and disk space
  is sufficient.
- Active local jobs at `2026-07-05T16:40Z`: `msmarco` is loading the full
  official document atom JSONL into `ii42_p1.msmarco_p1_a000_atoms` via local
  PostgreSQL `COPY` in tmux session `ii42_m603_p1_msmarco_load_atoms`.
  PostgreSQL `pg_stat_progress_copy` showed active progress. `fever` and
  `hotpotqa` remain ready and strict-gated for subsequent generation-shards
  publish/eval.

Evaluation policy going forward:

- Build DB-native BM25, VectorChord, and P1 indexes over the full corpus for
  each dataset.
- Compute metrics only over official qrels-bearing query splits.
- Keep all previously opened P1/M549U/M601 surfaces as `seen_regression`.
- Do not make a clean promotion claim until a frozen `clean_heldout` surface is
  selected, run once through the same native DB path, and reported separately.

## Latest Native Matrix Checkpoint

### Shared15 Full Native Matrix: 15/15

Output:

`runs/m603_p1_native_shared15_v1/m603_p1_native_shared15_native_smoke_combined_matrix.json`

| Source | Datasets | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 15 | 1342 | 0.670058 | 0.593976 | 0.783718 | 0.788108 | - | 0.906167 |
| `dense` | 15 | 1342 | 0.728543 | 0.640422 | 0.778166 | 0.835412 | - | 0.868998 |
| `M549U` | 15 | 1342 | 0.730777 | 0.647687 | 0.835954 | 0.826996 | 0.404354 | 0.940838 |
| `P1-a010` | 15 | 1342 | 0.742546 | 0.660261 | 0.842832 | 0.837364 | 0.414295 | 0.936772 |
| `P1-a0125` | 15 | 1342 | 0.744939 | 0.662742 | 0.843488 | 0.839746 | 0.416677 | 0.937309 |

### Official BEIR15 Native Local Matrix: 12/15

Output:

`runs/m603_p1_native_official_beir8_partial_v1/m603_p1_native_official_beir15_native_local_12_combined_matrix.json`

This matrix keeps the 11 completed official rows and adds full
`climate-fever` through 55 local native generation shards. `climate-fever`
covers all 1535 official qrels-bearing queries over the full
5416593-document atom corpus.

| Source | Datasets | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 12 | 32303 | 0.355689 | 0.252004 | 0.539037 | 0.440245 | - | 0.714249 |
| `dense` | 12 | 32303 | 0.499226 | 0.363157 | 0.657211 | 0.591873 | - | 0.807283 |
| `M549U` | 12 | 32303 | 0.367833 | 0.245101 | 0.485054 | 0.466554 | 0.372110 | 0.708615 |
| `P1-a010` | 12 | 32303 | 0.378001 | 0.252590 | 0.507252 | 0.479423 | 0.377312 | 0.771392 |
| `P1-a0125` | 12 | 32303 | 0.380142 | 0.254788 | 0.522810 | 0.482977 | 0.379294 | 0.772781 |

`climate-fever` row metrics:

| Source | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 1535 | 0.127530 | 0.097173 | 0.341694 | 0.181684 | - | 0.567427 |
| `dense` | 1535 | 0.394597 | 0.314161 | 0.703507 | 0.513265 | - | 0.836721 |
| `M549U` | 1535 | 0.316941 | 0.247571 | 0.573366 | 0.433371 | 0.427062 | 0.708903 |
| `P1-a010` | 1535 | 0.321201 | 0.250983 | 0.577362 | 0.437246 | 0.431857 | 0.739826 |
| `P1-a0125` | 1535 | 0.322181 | 0.251818 | 0.579153 | 0.438731 | 0.433081 | 0.741140 |

### Previous Official BEIR15 Native Local Matrix: 11/15

Output:

`runs/m603_p1_native_official_beir8_partial_v1/m603_p1_native_official_beir15_native_local_11_combined_matrix.json`

This matrix keeps the 10 completed official rows and adds full
`dbpedia-entity` through 47 local native generation shards. `dbpedia-entity`
covers all 400 official qrels-bearing queries over the full 4635922-document
atom corpus.

| Source | Datasets | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 11 | 30768 | 0.376431 | 0.266080 | 0.556978 | 0.463750 | - | 0.727596 |
| `dense` | 11 | 30768 | 0.508737 | 0.367611 | 0.653002 | 0.599019 | - | 0.804607 |
| `M549U` | 11 | 30768 | 0.372459 | 0.244877 | 0.477025 | 0.469570 | 0.367114 | 0.708589 |
| `P1-a010` | 11 | 30768 | 0.383165 | 0.252736 | 0.500878 | 0.483257 | 0.372353 | 0.774262 |
| `P1-a0125` | 11 | 30768 | 0.385411 | 0.255058 | 0.517688 | 0.486999 | 0.374405 | 0.775658 |

`dbpedia-entity` row metrics:

| Source | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 400 | 0.239804 | 0.173662 | 0.390971 | 0.504366 | - | 0.580338 |
| `dense` | 400 | 0.387310 | 0.250729 | 0.458798 | 0.733080 | - | 0.629907 |
| `M549U` | 400 | 0.305783 | 0.169771 | 0.332024 | 0.644276 | 0.310750 | 0.582641 |
| `P1-a010` | 400 | 0.313437 | 0.176046 | 0.344150 | 0.652336 | 0.314150 | 0.665360 |
| `P1-a0125` | 400 | 0.315050 | 0.177284 | 0.353629 | 0.653113 | 0.315125 | 0.667852 |

### Previous Official BEIR15 Partial + NQ Native Matrix: 10/15

Output:

`runs/m603_p1_native_official_beir8_partial_v1/m603_p1_native_official_beir10_native_mixed_combined_matrix.json`

This matrix keeps the original 9 completed DB-native official rows and adds
full `nq` through local native generation shards. `nq` covers all `3452`
official qrels-bearing queries over the full `2681468`-document atom corpus.

| Source | Datasets | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 10 | 30368 | 0.390093 | 0.275322 | 0.573578 | 0.459689 | - | 0.742322 |
| `dense` | 10 | 30368 | 0.520880 | 0.379299 | 0.672423 | 0.585613 | - | 0.822077 |
| `M549U` | 10 | 30368 | 0.379127 | 0.252387 | 0.491526 | 0.452100 | 0.372751 | 0.721184 |
| `P1-a010` | 10 | 30368 | 0.390137 | 0.260405 | 0.516551 | 0.466349 | 0.378173 | 0.785152 |
| `P1-a0125` | 10 | 30368 | 0.392447 | 0.262835 | 0.534093 | 0.470388 | 0.380333 | 0.786438 |

`nq` row metrics:

| Source | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 3452 | 0.242799 | 0.203360 | 0.678568 | 0.215921 | - | 0.860926 |
| `dense` | 3452 | 0.584749 | 0.517974 | 0.912732 | 0.541040 | - | 0.935762 |
| `M549U` | 3452 | 0.480965 | 0.423250 | 0.774913 | 0.446975 | 0.413485 | 0.865971 |
| `P1-a010` | 3452 | 0.485991 | 0.427052 | 0.781286 | 0.450955 | 0.418888 | 0.946842 |
| `P1-a0125` | 3452 | 0.487110 | 0.427466 | 0.789639 | 0.451553 | 0.420406 | 0.946842 |

### Previous Official BEIR15 Partial Native Matrix: 9/15

Output:

`runs/m603_p1_native_official_beir8_partial_v1/m603_p1_native_official_beir8_partial_combined_matrix.json`

This is retained as the pre-`nq` checkpoint.

| Source | Datasets | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 9 | 26916 | 0.406459 | 0.283317 | 0.561913 | 0.486774 | - | 0.729144 |
| `dense` | 9 | 26916 | 0.513783 | 0.363891 | 0.645722 | 0.590565 | - | 0.809445 |
| `M549U` | 9 | 26916 | 0.367812 | 0.233402 | 0.460038 | 0.452669 | 0.368225 | 0.705096 |
| `P1-a010` | 9 | 26916 | 0.379487 | 0.241889 | 0.487136 | 0.468059 | 0.373650 | 0.767186 |
| `P1-a0125` | 9 | 26916 | 0.381929 | 0.244543 | 0.505699 | 0.472481 | 0.375880 | 0.768615 |

`quora` row metrics:

| Source | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `M549U` | 10000 | 0.318864 | 0.301795 | 0.329321 | 0.348064 | 0.182108 | 0.785217 |
| `P1-a010` | 10000 | 0.320473 | 0.306482 | 0.492970 | 0.350310 | 0.185439 | 0.987197 |
| `P1-a0125` | 10000 | 0.321547 | 0.310203 | 0.601853 | 0.351719 | 0.189039 | 0.987090 |

### Broad10 Full Native Matrix: 10/10

Output:

`runs/m603_p1_native_broad10_v1/m603_p1_native_broad10_full_combined_matrix.json`

Completed rows: `ArguAna`, `CQADupstackGamingRetrieval`,
`CQADupstackUnixRetrieval`, `SCIDOCS`, `FiQA2018`,
`ClimateFEVERHardNegatives`, `FEVERHardNegatives`,
`HotpotQAHardNegatives`, `TRECCOVID`, and `Touche2020Retrieval.v3`.

| Source | Datasets | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 10 | 8815 | 0.383554 | 0.268718 | 0.595367 | 0.461805 | - | 0.777387 |
| `dense` | 10 | 8815 | 0.420773 | 0.355667 | 0.609022 | 0.486599 | - | 0.697845 |
| `M549U` | 10 | 8815 | 0.443289 | 0.312233 | 0.571932 | 0.546570 | 0.322227 | 0.780442 |
| `P1-a010` | 10 | 8815 | 0.453078 | 0.320887 | 0.578565 | 0.554609 | 0.327245 | 0.824740 |
| `P1-a0125` | 10 | 8815 | 0.455676 | 0.322789 | 0.585275 | 0.557424 | 0.328413 | 0.825395 |

Final `Touche2020Retrieval.v3` audit:

- Strict atom gate:
  `runs/m603_p1_product_atoms_broad10_v1/touche2020_atom_gate.json`
  reports `ready=true`, documents `303732/303732`, and queries `49/49`.
- Native eval output:
  `runs/m603_p1_native_broad10_v1/p1_eval/touche2020_p1_native_matrix.json`.
- Touche row metrics:

| Source | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `M549U` | 49 | 0.410839 | 0.169407 | 0.308577 | 0.807370 | 0.307755 | 0.768240 |
| `P1-a010` | 49 | 0.423319 | 0.177484 | 0.312398 | 0.816327 | 0.310816 | 0.895405 |
| `P1-a0125` | 49 | 0.428797 | 0.179691 | 0.313326 | 0.823129 | 0.310408 | 0.894231 |

### Current Recommendation

`P1-a0125` should be promoted as the fixed-alpha engineering candidate for the
native P1.1 line. It beats `P1-a010` on all tracked macro metrics on shared15
full, official BEIR15 partial `12/15`, and broad10 full. `P1-a010` should
remain the frozen P1 baseline for regression comparison.

This is not a clean-heldout generalization claim. The regenerated matrix files
now carry `seen_regression` split labels for the completed datasets and report:
`do not make a clean promotion claim: no clean-heldout native rows are present
in this matrix.`

### Verification

Latest local checks at `2026-07-05T16:34Z`:

- `python3 -m py_compile` passed for the modified M603/P1 Python scripts.
- `bash -n` passed for the modified Spark/local runner scripts.
- `git diff --check` passed.
- Focused M603 native-path tests passed:
  `python3 -m pytest -q tests/test_publish_p1_atoms_to_ii42_pg.py
  tests/test_advance_m603_p1_completed_dataset.py
  tests/test_compile_m603_native_surface_matrix.py`
  reported `14 passed`.

## Native Path Contract

Implemented path:

1. M549U/P1 encoder emits product atom arrays:
   `atom_ids`, `atom_impacts`, `atom_sources`.
2. `load_p1_atom_jsonl_to_pg.py` loads atom JSONL into a PostgreSQL atom table.
3. `publish_p1_atoms_to_ii42_pg.py` publishes through:
   `ii42_model_generation_build_plan` and
   `ii42_model_generation_upsert_from_table`.
4. `evaluate_p1_native_atoms_pg.py` evaluates via native DB surfaces.

Two query modes are now separated:

- `atom`: semantic-only parity path through `ii42_model_query_atoms`.
- `hybrid`: fixed-alpha path through DB BM25 candidates, model atom candidates,
  and `ii42_hybrid_fuse_candidates`.

The first nfcorpus native run exposed that `ii42_model_query_atoms` alone does
not consume BM25 scoring-profile weights. That is now treated as an integration
boundary, not a model-quality result.

The official `cqadupstack` row exposed a scale issue in the hybrid evaluator:
the old query path rebuilt the atom-table document ordinal map inside every
query SQL statement. The evaluator now prepares a temporary per-dataset
`doc_ord -> docs.ctid` map once per eval and reuses it for all hybrid queries.
This keeps the scoring contract unchanged while making the native path viable
for large official roots.

## Completed Engineering Evidence

- `scripts/run_m549u_encode_jsonl_spark.sh` can pass product atom flags to the
  encoder.
- `scripts/run_m603_p1_product_atoms_spark.sh` can batch-generate P1 product
  atom JSONL from root `documents.jsonl` and `queries.jsonl`.
- `scripts/run_m603_p1_product_atoms_spark.sh` now supports optional
  `SHARD_LABEL`, `DOCUMENT_LINE_START`, `DOCUMENT_LINE_END`,
  `QUERY_LINE_START`, and `QUERY_LINE_END` controls. Default output paths are
  unchanged; shard mode writes range-specific JSONL files for larger
  official1024/shared15 generation plans.
- `scripts/merge_p1_atom_jsonl_shards.py` validates source row counts,
  contiguous shard coverage, per-shard row counts, and atom payload schema
  before deterministically merging shards into the single JSONL expected by
  the existing loader/publisher path.
- `scripts/plan_p1_atom_jsonl_shards.py` emits deterministic shard ranges,
  runner commands, and merge commands from a source root. This makes large
  official1024/shared15 atom generation auditable before launch.
- `scripts/run_m603_p1_native_surface_matrix.py` publishes and evaluates
  datasets through the native DB lifecycle for multiple fixed alpha values.
- `scripts/run_m603_p1_native_surface_matrix.py --reuse-existing-output`
  reuses completed `publish.json` and `eval.json` rows when extending a
  surface. This prevents expensive re-load/re-publish of already-audited
  dataset/alpha pairs.
- `scripts/run_m603_p1_native_surface_matrix.py` now labels `alpha=0` native
  semantic-only runs as `M549U`, matching the combined-matrix source contract.
- `load_p1_atom_jsonl_to_pg.py`, `publish_p1_atoms_to_ii42_pg.py`, and
  `run_m603_p1_native_surface_matrix.py` now support opt-in
  `--skip-empty-atoms`. Default loading remains strict. The opt-in is used for
  sampled shared15 rows where empty source text produces an explicitly empty
  semantic atom payload; BM25 candidates still cover those documents, while
  the semantic generation avoids fabricated atoms.
- Shared15 native P1 rows are published into `ii42_p1_shared15`, not the
  default `ii42_p1` schema. This prevents sampled shared15 atom tables from
  overwriting official-root atom tables with the same dataset/alpha names.
- Local PostgreSQL smoke tests cover publish -> generation -> query -> metrics.
- BEIR natural-language queries are converted to conservative lower-case term
  lists before entering II-42 raw-query APIs. This avoids parser failures on
  plain text such as `(+)-` while keeping BM25 baselines and P1 hybrid BM25
  candidates on the same query surface.
- `evaluate_ii42_beir15_native_baselines.py --progress-every` now emits
  progress for long BM25/dense baseline runs.
- `compile_m603_native_surface_matrix.py` compiles native BM25, dense, M549U,
  P1-a010, and P1-a0125 rows into one per-surface matrix without re-scoring.

Validation commands run:

```text
python3 -m py_compile scripts/run_m603_p1_native_surface_matrix.py \
    scripts/evaluate_p1_native_atoms_pg.py
bash -n scripts/run_m549u_encode_jsonl_spark.sh \
    scripts/run_m603_p1_product_atoms_spark.sh \
    scripts/run_m603_p1_fixed_alpha_matrix_spark.sh
python3 -m pytest tests/test_research_sae_m549u_product_atoms.py \
    tests/test_create_p1_m549u_checkout.py \
    tests/test_load_p1_atom_jsonl_to_pg.py \
    tests/test_publish_p1_atoms_to_ii42_pg.py -q
git diff --check
```

Latest result: all passed.

Additional validation after adding plain-text query sanitization:

```text
python3 -m py_compile scripts/ii42_plain_query.py \
    scripts/evaluate_ii42_beir15_native_baselines.py \
    scripts/evaluate_p1_native_atoms_pg.py
python3 -m pytest tests/test_ii42_plain_query.py -q
python3 -m pytest tests/test_publish_p1_atoms_to_ii42_pg.py \
    tests/test_load_p1_atom_jsonl_to_pg.py -q
```

Latest result: all passed.

Additional validation after adding shard-safe atom generation and merge tools:

```text
python3 -m py_compile scripts/merge_p1_atom_jsonl_shards.py \
    scripts/plan_p1_atom_jsonl_shards.py \
    scripts/research_sae_m549u_encode_jsonl.py \
    scripts/run_m603_p1_native_surface_matrix.py \
    scripts/evaluate_p1_native_atoms_pg.py
python3 -m pytest tests/test_run_m603_p1_product_atoms_spark.py \
    tests/test_plan_p1_atom_jsonl_shards.py \
    tests/test_merge_p1_atom_jsonl_shards.py -q
bash -n scripts/run_m603_p1_product_atoms_spark.sh \
    scripts/run_m549u_encode_jsonl_spark.sh \
    scripts/run_m603_p1_fixed_alpha_matrix_spark.sh
git diff --check
```

Latest result: all passed.

Additional validation after enabling MTEB-style custom task ingestion:

```text
python3 -m py_compile scripts/build_ii42_beir15_pg.py
python3 -m pytest tests/test_build_ii42_beir15_pg.py -q
git diff --check
```

Latest result after this checkpoint: all passed, including `git diff --check`.

## Native broad10 Progress

Legacy/offline broad10 anchors are still retained as sanity checks:

- `runs/m549u_active_locked_broad10_matrix_seed5532/m549_m549u_active_locked_broad10_matrix_seed5532.json`
- `runs/m549u_stage2_fixed_bm25_broad10_merged_seed5536/m549u_stage2_fixed_bm25_broad10_merged.json`

Native broad10 engineering state:

- Source root synced from `spark-1`:
  `/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks`.
- Local staging root:
  `/Volumes/Betty/Tmp/ii42-broad10-pg-staging`.
- Synced tasks: `ArguAna`, `CQADupstackGamingRetrieval`,
  `CQADupstackUnixRetrieval`, `ClimateFEVERHardNegatives`,
  `FEVERHardNegatives`, `FiQA2018`, `HotpotQAHardNegatives`, `SCIDOCS`,
  `TRECCOVID`, `Touche2020Retrieval.v3`.
- The staging gate has all required files for all `10/10` tasks:
  `documents.jsonl`, `queries.jsonl`, `quality_qrels.json`, and
  `mteb_retrieval_prepare_summary.json`.
- A small ArguAna DB smoke loaded `8674` documents, `1401` queries, and `1401`
  qrels successfully before the clean full rebuild.
- A 20-query ArguAna native baseline smoke completed:
  BM25 `NDCG@10=0.263046`, `MAP@100=0.179773`, `Recall@100=1.000000`,
  `MRR@20=0.172062`; dense `NDCG@10=0.464885`,
  `MAP@100=0.329191`, `Recall@100=1.000000`, `MRR@20=0.329191`.
- ArguAna product atom gate:
  `runs/m603_p1_product_atoms_broad10_v1/arguana_atom_gate.json`.
- Full broad10 native DB build session:
  `ii42_m603_broad10_native_build` (completed at this checkpoint; all
  `10/10` tasks verified with BM25 and VectorChord indexes).
- Full broad10 native baseline matrix:
  `runs/m603_p1_native_broad10_v1/m603_p1_native_broad10_baselines_matrix.json`
  and
  `runs/m603_p1_native_broad10_v1/m603_p1_native_broad10_baselines_matrix.md`.
  Macro BM25: `NDCG@10=0.383554`, `MAP@100=0.268718`,
  `Recall@100=0.595367`, `MRR@20=0.461805`, `CUB=0.777387`.
  Macro dense: `NDCG@10=0.420773`, `MAP@100=0.355667`,
  `Recall@100=0.609022`, `MRR@20=0.486599`, `CUB=0.697845`.
- Full broad10 product atom recovery for `CQADupstackUnixRetrieval` completed
  through deterministic line-range shards. The earlier all-dataset MPS
  run stopped on memory pressure at `11550/45301`
  `CQADupstackGamingRetrieval` documents, then the `BATCH_SIZE=2` resume
  stopped at `43688/45301`, and the `BATCH_SIZE=1` long-run path was killed at
  `4651/47382` `CQADupstackUnixRetrieval` documents. All partial JSONL tails
  ended on complete lines. The completed path used short deterministic
  line-range shards plus `merge_p1_atom_jsonl_shards.py` validation instead of
  a single long MPS process. The final merged Unix atom surface passed strict
  row-count and schema gate with `documents=47382/47382` and
  `queries=1072/1072`.
- PostgreSQL on this Mac can read model checkout files from `/tmp`, but reading
  model files under `/Users/...` returns `Interrupted system call`. Native P1
  publishes for broad10 therefore use `/tmp/ii42_m603_p1_broad10_model_checkout`
  as the DB-readable model root. This is a deployment-path fix, not a scoring
  or model change.

### Completed broad10 Native Partial Matrix: 9/10

Outputs:

- `runs/m603_p1_native_broad10_v1/m603_p1_native_broad10_completed9_combined_matrix.json`
- `runs/m603_p1_native_broad10_v1/m603_p1_native_broad10_completed9_combined_matrix.md`

Completed rows: `ArguAna`, `CQADupstackGamingRetrieval`,
`CQADupstackUnixRetrieval`, `SCIDOCS`, `FiQA2018`,
`ClimateFEVERHardNegatives`, `FEVERHardNegatives`,
`HotpotQAHardNegatives`, `TRECCOVID`.

| Source | Datasets | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 9 | 8766 | 0.359860 | 0.256467 | 0.591139 | 0.416745 | - | 0.764876 |
| `dense` | 9 | 8766 | 0.400296 | 0.349322 | 0.603208 | 0.434468 | - | 0.672547 |
| `M549U` | 9 | 8766 | 0.446894 | 0.328103 | 0.601194 | 0.517592 | 0.323835 | 0.781798 |
| `P1-a010` | 9 | 8766 | 0.456385 | 0.336821 | 0.608139 | 0.525530 | 0.329071 | 0.816888 |
| `P1-a0125` | 9 | 8766 | 0.458663 | 0.338688 | 0.615491 | 0.527901 | 0.330414 | 0.817746 |

Interpretation: this is not the full broad10 surface yet, but it is the
strongest broad10 native aggregate checkpoint so far. `P1-a0125` improves all
tracked macro metrics over `P1-a010` on the completed subset and is also above
the current completed-row dense macro. This does not replace the need for full
`10/10` broad10 coverage because the remaining broad10 tasks have different
corpus/query structure.

### ArguAna Native Combined Smoke

Outputs:

- `runs/m603_p1_native_broad10_v1/m603_p1_native_broad10_arguana_combined_matrix.json`
- `runs/m603_p1_native_broad10_v1/m603_p1_native_broad10_arguana_combined_matrix.md`

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 0.344115 | 0.237186 | 0.952891 | 0.233893 | - | 0.990007 |
| `dense` | 0.450704 | 0.319743 | 0.982156 | 0.318966 | - | 0.982156 |
| `M549U` | 0.423052 | 0.296413 | 0.992862 | 0.294652 | 0.607759 | 0.999286 |
| `P1-a010` | 0.426741 | 0.300281 | 0.992862 | 0.298844 | 0.615924 | 0.999286 |
| `P1-a0125` | 0.428524 | 0.301619 | 0.993576 | 0.300254 | 0.617502 | 0.999286 |

Interpretation: this is a single-dataset native broad10 smoke, not a broad10
promotion decision. It does validate the native DB/plugin path for broad10
P1 fixed-alpha publish and query. On ArguAna, `P1-a0125` is better than
`P1-a010` on `NDCG@10`, `MAP@100`, `Recall@100`, `MRR@20`, and dense
overlap, so it is consistent with the shared15 candidate direction.

### CQADupstackGamingRetrieval Native Combined Row

Outputs:

- `runs/m603_p1_native_broad10_v1/m603_p1_native_broad10_cqadupstack_gaming_combined_matrix.json`
- `runs/m603_p1_native_broad10_v1/m603_p1_native_broad10_cqadupstack_gaming_combined_matrix.md`

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 0.465550 | 0.430617 | 0.746695 | 0.457166 | - | 0.869162 |
| `dense` | 0.554296 | 0.513284 | 0.770804 | 0.545836 | - | 0.813355 |
| `M549U` | 0.514387 | 0.470551 | 0.757762 | 0.502924 | 0.347831 | 0.872300 |
| `P1-a010` | 0.532835 | 0.489333 | 0.769655 | 0.521571 | 0.354564 | 0.910125 |
| `P1-a0125` | 0.535302 | 0.492537 | 0.776395 | 0.525446 | 0.356245 | 0.911536 |

Interpretation: this is still a single-row broad10 update, not the full
broad10 promotion surface. It is stronger than the ArguAna-only smoke because
the native DB/plugin path now covers a second task family and a larger query
set. `P1-a0125` remains the better fixed candidate on every tracked metric for
this row.

### CQADupstackUnixRetrieval Native Combined Row

Outputs:

- `runs/m603_p1_native_broad10_v1/m603_p1_native_broad10_cqadupstack_unix_combined_matrix.json`
- `runs/m603_p1_native_broad10_v1/m603_p1_native_broad10_cqadupstack_unix_combined_matrix.md`

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 0.282784 | 0.259707 | 0.543694 | 0.284534 | - | 0.754300 |
| `dense` | 0.449146 | 0.410350 | 0.767735 | 0.440482 | - | 0.870142 |
| `M549U` | 0.386875 | 0.348392 | 0.675370 | 0.386811 | 0.437164 | 0.835163 |
| `P1-a010` | 0.400481 | 0.364035 | 0.682250 | 0.401643 | 0.444142 | 0.861662 |
| `P1-a0125` | 0.402501 | 0.365889 | 0.687225 | 0.404798 | 0.445634 | 0.863062 |

Interpretation: this is the third completed native broad10 row. It validates
that the shard recovery path produces a DB/plugin-publishable P1 surface, not
just an offline JSONL artifact. `P1-a0125` again beats `P1-a010` on every
tracked metric, but dense remains materially higher on this row.

### SCIDOCS Native Combined Row

Outputs:

- `runs/m603_p1_native_broad10_v1/m603_p1_native_broad10_scidocs_combined_matrix.json`
- `runs/m603_p1_native_broad10_v1/m603_p1_native_broad10_scidocs_combined_matrix.md`

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 0.150694 | 0.103316 | 0.348367 | 0.277795 | - | 0.561617 |
| `dense` | 0.222704 | 0.157222 | 0.483100 | 0.378192 | - | 0.750733 |
| `M549U` | 0.179450 | 0.123076 | 0.394800 | 0.321387 | 0.479330 | 0.598550 |
| `P1-a010` | 0.185748 | 0.127746 | 0.402450 | 0.329332 | 0.491000 | 0.620733 |
| `P1-a0125` | 0.187614 | 0.128630 | 0.403800 | 0.329766 | 0.493940 | 0.623333 |

Interpretation: this is the fourth completed native broad10 row. It was
generated on `spark-1`, synced locally, strict-gated, then published/evaluated
through the same DB/plugin lifecycle. `P1-a0125` remains the better fixed
candidate on every tracked metric for this row.

## Native nfcorpus Smoke

Source atom root:

`runs/m603_p1_product_atoms_smoke_v1/nfcorpus`

Generated on `spark-1` from official root:

| File | Rows | Bytes |
| --- | ---: | ---: |
| `documents.p1_atoms.jsonl` | 3633 | 115610642 |
| `queries.p1_atoms.jsonl` | 323 | 9792295 |

Atom schema sample:

- doc/query rows have 128 `atom_ids`, 128 `atom_impacts`, and 128
  `atom_sources`.
- IDs use the local DB-compatible `beir15:nfcorpus:*` prefix.

### Atom-Only Semantic Matrix

Output:

`runs/m603_p1_native_nfcorpus_smoke_v1/m603_p1_native_nfcorpus_smoke_matrix.json`

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `P1-a010` | 0.272408 | 0.118020 | 0.266389 | 0.465003 | 0.558507 |
| `P1-a0125` | 0.272408 | 0.118020 | 0.266389 | 0.465003 | 0.558507 |

Interpretation: semantic path works, but alpha is not represented because BM25
is absent from `ii42_model_query_atoms`.

### Native Baselines

Output:

`runs/m603_p1_native_nfcorpus_baselines_v1/nfcorpus_baselines.json`

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 0.288604 | 0.133177 | 0.228521 | 0.480911 | 0.414336 |
| `dense` | 0.322832 | 0.139780 | 0.269111 | 0.548789 | 0.541961 |

Dense top100 rankings were also exported for P1 overlap auditing:

`runs/m603_p1_native_nfcorpus_baselines_v1/nfcorpus_dense_top100.jsonl`

### Hybrid Fixed-Alpha Matrix

Output:

`runs/m603_p1_native_nfcorpus_hybrid_ctidfix_overlap_v1/m603_p1_native_nfcorpus_smoke_hybrid_ctidfix_overlap_matrix.json`

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `P1-a010` | 0.296613 | 0.135913 | 0.283256 | 0.504211 | 0.387833 | 0.492476 |
| `P1-a0125` | 0.303304 | 0.139107 | 0.285813 | 0.512665 | 0.390031 | 0.497674 |

Interpretation: BM25 and semantic candidates both enter the native matrix. A
prior hybrid attempt accidentally used the nullable `ctid` from
`ii42_model_query_atoms`; table-built generations only have authoritative
`doc_ord`, so semantic candidates must map
`doc_ord -> atom table ORDER BY doc_id -> docs.doc_id -> docs.ctid`.
After this fix, P1 no longer collapses to BM25, and `a0125` beats `a010` on
this nfcorpus smoke.

### Alpha Effect Diagnostic

Output:

`runs/m603_p1_native_nfcorpus_alpha_extreme_diag_v1/m603_p1_native_nfcorpus_alpha_extreme_diag_matrix.json`

Limited to 30 queries, `alpha=0` vs `alpha=1`.

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `P1-a000` | 0.008461 | 0.001429 | 0.003148 | 0.033333 | 0.429364 |
| `P1-a100` | 0.197304 | 0.054132 | 0.161861 | 0.502169 | 0.429364 |

Interpretation: hybrid weights are active. The identical `a010/a0125` smoke is
not caused by ignored alpha weights.

## Prior Native Official Subset Matrix: 8/15

This section is retained as the initial eight-dataset official-root checkpoint.
It is superseded by the current `12/15` official BEIR15 native matrix above,
which adds `quora` through the DB/plugin path and large official rows through
the local native generation-shards path.

Output:

`runs/m603_p1_native_official_beir8_partial_v1/m603_p1_native_official_beir8_partial_matrix.json`

Combined output with baselines and M549U:

`runs/m603_p1_native_official_beir8_partial_v1/m603_p1_native_official_beir8_partial_combined_matrix.json`

This is the first multi-dataset native DB/plugin-path matrix. It uses:

- atom load through `load_p1_atom_jsonl_to_pg.py`;
- generation publish through `ii42_model_generation_upsert_from_table`;
- semantic candidates through `ii42_model_query_atoms`;
- BM25 candidates through `ii42_hybrid_bm25_candidates`;
- fusion through `ii42_hybrid_fuse_candidates`.

P1-only macro:

| Source | Datasets | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `P1-a010` | 8 | 16916 | 0.386864 | 0.233815 | 0.486407 | 0.482778 | 0.397176 | 0.739685 |
| `P1-a0125` | 8 | 16916 | 0.389477 | 0.236335 | 0.493680 | 0.487576 | 0.399235 | 0.741306 |

Interpretation at this checkpoint: on the initial eight-dataset official-root
native subset, `a0125` improved every tracked macro metric over `a010`. This
removed the blocker for that subset, but it was not full official BEIR15
coverage and is now superseded by the `12/15` matrix above.

Combined macro:

| Source | Datasets | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 8 | 16916 | 0.364990 | 0.231836 | 0.513694 | 0.455665 | - | 0.696960 |
| `dense` | 8 | 16916 | 0.467908 | 0.302985 | 0.603405 | 0.555064 | - | 0.787143 |
| `M549U` | 8 | 16916 | 0.373930 | 0.224853 | 0.476378 | 0.465745 | 0.391489 | 0.695081 |
| `P1-a010` | 8 | 16916 | 0.386864 | 0.233815 | 0.486407 | 0.482778 | 0.397176 | 0.739685 |
| `P1-a0125` | 8 | 16916 | 0.389477 | 0.236335 | 0.493680 | 0.487576 | 0.399235 | 0.741306 |

Combined interpretation at this checkpoint: P1-a0125 was the strongest P1
fixed-alpha candidate on this eight-dataset native surface and beat BM25/M549U.
Dense still had higher NDCG/MAP/MRR, so this was not a final quality claim; it
was a native integration and fixed-alpha direction signal.

## Native Shared15 Full Matrix: 15/15

Combined output:

`runs/m603_p1_native_shared15_v1/m603_p1_native_shared15_native_smoke_combined_matrix.json`

Markdown:

`runs/m603_p1_native_shared15_v1/m603_p1_native_shared15_native_smoke_combined_matrix.md`

This is the first sampled shared15 native DB/plugin-path matrix. It uses the
same lifecycle as the official partial surface, but reads base documents and
qrels from `ii42_shared15` and publishes P1 atom tables into
`ii42_p1_shared15`.

Current status: 15/15 shared15 datasets are complete in the native matrix.
Every appended dataset passed exact document/query row-count and schema
validation before native publish/eval. The local atom generation tmux session
finished the full shared15 queue, and no shared15 partial atom files are being
treated as valid results.

`fiqa` contains two empty-text document rows whose P1 atom payloads are
explicitly empty. These rows are skipped during semantic atom-table loading
with `--skip-empty-atoms`; each `fiqa` P1 table loads 1998 semantic rows and
skips 2 empty semantic rows. This is an explicit engineering exception, not a
silent cache acceptance rule.

Combined macro:

| Source | Datasets | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 15 | 1342 | 0.670058 | 0.593976 | 0.783718 | 0.788108 | - | 0.906167 |
| `dense` | 15 | 1342 | 0.728543 | 0.640422 | 0.778166 | 0.835412 | - | 0.868998 |
| `M549U` | 15 | 1342 | 0.730777 | 0.647687 | 0.835954 | 0.826996 | 0.404354 | 0.940838 |
| `P1-a010` | 15 | 1342 | 0.742546 | 0.660261 | 0.842832 | 0.837364 | 0.414295 | 0.936772 |
| `P1-a0125` | 15 | 1342 | 0.744939 | 0.662742 | 0.843488 | 0.839746 | 0.416677 | 0.937309 |

Interpretation: on the full 15-dataset shared15 native matrix, `a0125`
improves over `a010` at macro level without a Recall/MRR regression. The
improvement is small but consistent across the macro surface:
`+0.002393` NDCG@10, `+0.002481` MAP@100, `+0.000656` Recall@100,
`+0.002382` MRR@20, `+0.002381` dense overlap@100, and `+0.000537` candidate
upper bound. This is now a valid broader-surface promotion signal for
`a0125` on seen-regression surfaces. It is not clean-heldout evidence.

## Native Baseline Surfaces Prepared

The following BM25 and dense native baselines have completed and exported dense
top100 rankings for overlap auditing:

| Dataset | BM25 NDCG@10 | BM25 R@100 | Dense NDCG@10 | Dense R@100 | Dense ranking |
| --- | ---: | ---: | ---: | ---: | --- |
| `nfcorpus` | 0.288604 | 0.228521 | 0.322832 | 0.269111 | yes |
| `scifact` | 0.663931 | 0.882556 | 0.714060 | 0.898222 | yes |
| `fiqa` | 0.231073 | 0.510764 | 0.510599 | 0.804102 | yes |
| `arguana` | 0.344115 | 0.952891 | 0.429182 | 0.972877 | yes |
| `scidocs` | 0.150534 | 0.348567 | 0.225314 | 0.484483 | yes |
| `trec-covid` | 0.571959 | 0.100074 | 0.833468 | 0.157343 | yes |
| `webis-touche2020` | 0.377718 | 0.561303 | 0.276631 | 0.493851 | yes |
| `cqadupstack` | 0.291987 | 0.524878 | 0.431176 | 0.747247 | yes |

The initial eight-dataset official subset baselines have dense top100 rankings
prepared for native overlap auditing. The full BEIR15 baseline coverage below is
also complete; remaining P1-native full coverage is gated by product atom
generation for the six not-yet-compiled official datasets.

Full BEIR15 native baseline coverage is now complete: 15/15 datasets have BM25
and dense baseline JSON outputs, and dense top100 rankings are available for
overlap auditing. The final baseline-only gap, `hotpotqa`, completed locally at
`2026-07-04T05:36Z`. This removed the baseline-only gap for the later full
BEIR15/native matrix, but P1-native full coverage still depends on product atom
generation for the remaining six official datasets.

Native `msmarco` baseline output:

`runs/m603_p1_native_baselines_v1/msmarco/baselines.json`

Dense top100 output:

`runs/m603_p1_native_dense_rankings_v1/msmarco/dense_rankings.jsonl`

| Source | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 43 | 0.366717 | 0.264019 | 0.408121 | 0.743909 | 0.672591 |
| `dense` | 43 | 0.647114 | 0.383274 | 0.472836 | 0.961240 | 0.700965 |

Native `dbpedia-entity` baseline output:

`runs/m603_p1_native_baselines_v1/dbpedia-entity/baselines.json`

Dense top100 output:

`runs/m603_p1_native_dense_rankings_v1/dbpedia-entity/dense_rankings.jsonl`

| Source | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 400 | 0.239804 | 0.173662 | 0.390971 | 0.504366 | 0.580338 |
| `dense` | 400 | 0.387310 | 0.250729 | 0.458798 | 0.733080 | 0.629907 |

Native `climate-fever` baseline output:

`runs/m603_p1_native_baselines_v1/climate-fever/baselines.json`

Dense top100 output:

`runs/m603_p1_native_dense_rankings_v1/climate-fever/dense_rankings.jsonl`

| Source | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 1535 | 0.127530 | 0.097173 | 0.341694 | 0.181684 | 0.567427 |
| `dense` | 1535 | 0.394597 | 0.314161 | 0.703507 | 0.513265 | 0.836721 |

Native `nq` baseline output:

`runs/m603_p1_native_baselines_v1/nq/baselines.json`

Dense top100 output:

`runs/m603_p1_native_dense_rankings_v1/nq/dense_rankings.jsonl`

| Source | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 3452 | 0.242799 | 0.203360 | 0.678568 | 0.215921 | 0.860926 |
| `dense` | 3452 | 0.584749 | 0.517974 | 0.912732 | 0.541040 | 0.935762 |

Native `fever` baseline output:

`runs/m603_p1_native_baselines_v1/fever/baselines.json`

Dense top100 output:

`runs/m603_p1_native_dense_rankings_v1/fever/dense_rankings.jsonl`

| Source | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 6666 | 0.449661 | 0.395967 | 0.838526 | 0.410674 | 0.933235 |
| `dense` | 6666 | 0.846450 | 0.820341 | 0.905383 | 0.862403 | 0.912997 |

Native `quora` baseline output:

`runs/m603_p1_native_baselines_v1/quora/baselines.json`

Dense top100 output:

`runs/m603_p1_native_dense_rankings_v1/quora/dense_rankings.jsonl`

| Source | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 10000 | 0.738212 | 0.695173 | 0.947661 | 0.735647 | 0.986610 |
| `dense` | 10000 | 0.880790 | 0.851136 | 0.984258 | 0.874570 | 0.987865 |

Native `hotpotqa` baseline output:

`runs/m603_p1_native_baselines_v1/hotpotqa/baselines.json`

Dense top100 output:

`runs/m603_p1_native_dense_rankings_v1/hotpotqa/dense_rankings.jsonl`

| Source | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 7405 | 0.511627 | 0.430958 | 0.724646 | 0.662780 | 0.847738 |
| `dense` | 7405 | 0.688827 | 0.619042 | 0.798447 | 0.834553 | 0.856516 |

## Native DB Surface Inventory

Local PostgreSQL currently has native II-42 schemas:

- `ii42_beir15`
- `ii42_p1`

Latest schema inventory at `2026-07-04T05:58Z`:

| Schema | Tables |
| --- | ---: |
| `ii42_beir15` | 19 |
| `ii42_p1` | 16 |

`ii42_beir15.dataset_manifest` has all 15 datasets indexed. The current native
official P1 matrix covers `12/15` datasets after adding `quora`, full `nq`,
full `dbpedia-entity`, and full `climate-fever`; the three remaining datasets
have strict-gated product atom files and need local generation-shards native
publish/eval.

Native `shared15` is now present as `ii42_shared15`. It was built from the
local sampled shared root:

`/Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared`

This root uses 768-dimensional embeddings, so the existing BEIR15 DB builder
was generalized with `--expected-from-files` and `--embedding-dim`. The build
still uses the same native schema shape, BM25 index, VectorChord index, and
verification flow; it does not add a separate query/evaluator lifecycle.

Latest shared15 manifest:

| Dataset | Docs | Queries | Qrels | Vector lists | Probes |
| --- | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | 2063 | 100 | 3818 | 4 | 2 |
| `scifact` | 2000 | 100 | 116 | 4 | 2 |
| `arguana` | 2000 | 100 | 100 | 4 | 2 |
| `scidocs` | 2000 | 100 | 492 | 4 | 2 |
| `fiqa` | 2000 | 100 | 267 | 4 | 2 |
| `trec-covid` | 17537 | 50 | 24673 | 35 | 6 |
| `webis-touche2020` | 2000 | 49 | 932 | 4 | 2 |
| `cqadupstack` | 2000 | 100 | 787 | 4 | 2 |
| `quora` | 2000 | 100 | 275 | 4 | 2 |
| `nq` | 2000 | 100 | 121 | 4 | 2 |
| `dbpedia-entity` | 3357 | 100 | 3458 | 7 | 3 |
| `hotpotqa` | 2000 | 100 | 200 | 4 | 2 |
| `fever` | 2000 | 100 | 109 | 4 | 2 |
| `climate-fever` | 2000 | 100 | 292 | 4 | 2 |
| `msmarco` | 4102 | 43 | 4102 | 8 | 3 |

Build command:

```bash
python3 scripts/build_ii42_beir15_pg.py build \
    --schema ii42_shared15 \
    --staging /Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared \
    --expected-from-files \
    --embedding-dim 768
```

Result: 15/15 shared15 datasets loaded, indexed, and verified with BM25 and
VectorChord query checks. This unblocks native shared15 evaluation once P1 atom
JSONL is available for the same sampled root.

The manifest contract is `dataset -> table_name`, plus expected/loaded row
counts and VectorChord settings. Latest local audit shows all 15 manifest rows
are `indexed`. The largest native DB surfaces are already present for BM25 and
dense baselines, but full native P1 on shared15 would require P1 atom
generation for the corresponding document tables before publish/query can be
audited through the same lifecycle. That is a generation/engineering cost, not
a DB schema blocker.

The batch atom generator now has a shard-safe fallback for that cost: set
`SHARD_LABEL` plus line-range variables to emit range-specific atom JSONL
without changing the final publisher/index lifecycle. Completed shards still
need row-count validation and deterministic concatenation before they can be
published as one dataset generation; this is handled by
`merge_p1_atom_jsonl_shards.py`. Use `plan_p1_atom_jsonl_shards.py` first to
generate auditable ranges and commands from the source root instead of hand
writing shard boundaries.

The planner now supports `--row-counts-json`, so large official roots do not
need to be scanned just to produce a deterministic shard plan. The row counts
were exported from the local `ii42_beir15.dataset_manifest` into:

`runs/m603_p1_native_official_beir8_partial_v1/m603_official_beir15_row_counts_from_db.json`

Using that manifest, `spark-2` generated the official BEIR15 shard plan:

`runs/m603_p1_native_official_beir8_partial_v1/m603_p1_atom_shard_plan_official_beir15_rows250k.json`

Remote copy:

`/home/huoju/leask/runs/ii42-m603-p1-product-atoms-official-beir15-sharded-v1/m603_p1_atom_shard_plan_official_beir15_rows250k.json`

Summary for `rows_per_shard=250000`:

| Dataset | Doc rows | Doc shards | Query rows | Query shards |
| --- | ---: | ---: | ---: | ---: |
| `nfcorpus` | 3633 | 1 | 323 | 1 |
| `scifact` | 5183 | 1 | 300 | 1 |
| `fiqa` | 57638 | 1 | 648 | 1 |
| `arguana` | 8674 | 1 | 1401 | 1 |
| `scidocs` | 25657 | 1 | 1000 | 1 |
| `trec-covid` | 171331 | 1 | 50 | 1 |
| `webis-touche2020` | 382545 | 2 | 49 | 1 |
| `cqadupstack` | 457199 | 2 | 13145 | 1 |
| `climate-fever` | 5416593 | 22 | 1535 | 1 |
| `dbpedia-entity` | 4635922 | 19 | 400 | 1 |
| `fever` | 5416568 | 22 | 6666 | 1 |
| `hotpotqa` | 5233329 | 21 | 7405 | 1 |
| `msmarco` | 8841823 | 36 | 43 | 1 |
| `nq` | 2681468 | 11 | 3452 | 1 |
| `quora` | 522931 | 3 | 10000 | 1 |
| **Total** | **33860494** | **144** | **46372** | **15** |

This is an engineering fallback plan for the full native surface. The planner,
merger, and sharded runner helper were deployed to both `spark-1` and `spark-2`
without overwriting the then-active non-sharded runner session. The current
preferred route is local root-to-P1 atom compilation when local precomputed root
embeddings are available.

The native surface runner now also supports a side-effect-free `--plan-only`
mode before atom JSONL files exist. For `cqadupstack` and
`webis-touche2020`, the preflight resolves the expected native tables and
indexes without creating P1 tables, loading atom rows, publishing generations,
or evaluating partial files:

| Dataset | Source | Doc table | P1 index | BM25 index |
| --- | --- | --- | --- | --- |
| `cqadupstack` | `P1-a010` | `ii42_beir15.docs_cqadupstack` | `ii42_beir15.docs_cqadupstack_p1_a010_idx` | `ii42_beir15.docs_cqadupstack_bm25_idx` |
| `cqadupstack` | `P1-a0125` | `ii42_beir15.docs_cqadupstack` | `ii42_beir15.docs_cqadupstack_p1_a0125_idx` | `ii42_beir15.docs_cqadupstack_bm25_idx` |
| `webis-touche2020` | `P1-a010` | `ii42_beir15.docs_webis_touche2020` | `ii42_beir15.docs_webis_touche2020_p1_a010_idx` | `ii42_beir15.docs_webis_touche2020_bm25_idx` |
| `webis-touche2020` | `P1-a0125` | `ii42_beir15.docs_webis_touche2020` | `ii42_beir15.docs_webis_touche2020_p1_a0125_idx` | `ii42_beir15.docs_webis_touche2020_bm25_idx` |

Historical full BEIR15/native plan-only verification at
`2026-07-04T01:52Z` resolved 30 planned rows: 15 datasets times
`P1-a010`/`P1-a0125`. This confirms the local DB route can address the full
BEIR15 surface without side effects. The current status file now records 12/15
native-complete rows after adding full `nq`, `dbpedia-entity`, and
`climate-fever`. The remaining three datasets are `fever`, `hotpotqa`, and
`msmarco`.

A separate atom-surface gate now validates local atom JSONL readiness before
publish/eval. It checks exact row counts from the exported manifest plus a small
schema sample for `id`, `atom_ids`, `atom_impacts`, and `atom_sources`.
Current local gate output:

`runs/m603_p1_native_status_v1/m603_p1_native_current_status.json`

Summary: `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`, `trec-covid`,
`webis-touche2020`, `cqadupstack`, `quora`, `nq`, and `dbpedia-entity` are
native-complete. `climate-fever`, `fever`, `hotpotqa`, and `msmarco` are
strict-gated and ready for local generation-shards native eval. This gate is
the required check before any new P1 atom table is loaded or published.

The single-dataset sync helper is intentionally dry-run by default. It remains
available for future remote-generated datasets; the helper runs
`rsync --partial --append-verify` and then calls the atom-surface gate before
any publish/eval step:

```bash
python3 scripts/sync_m603_p1_atom_dataset.py \
    --dataset cqadupstack \
    --source-root spark-2:/home/huoju/leask/runs/ii42-m603-p1-product-atoms-official-beir8-spark2-v1 \
    --dest-root runs/m603_p1_product_atoms_official_beir8_v1 \
    --row-counts-json runs/m603_p1_native_official_beir8_partial_v1/m603_official_beir15_row_counts_from_db.json \
    --execute
```

For local-root generated rows such as `quora`, no remote sync is needed. Use the
single-dataset advance helper without `--sync-source-root` after strict local
atom gates pass:

```bash
python3 scripts/advance_m603_p1_completed_dataset.py \
    --dataset quora \
    --compile-datasets nfcorpus,scifact,fiqa,arguana,scidocs,trec-covid,webis-touche2020,cqadupstack,quora \
    --execute
```

For future remote-generated rows, the full local transition still uses the
single-dataset advance helper in `--execute` mode. It only orchestrates existing
steps: sync helper, atom-surface gate, native surface runner, and combined
matrix compiler. It does not introduce a separate evaluator stack. The
following older `cqadupstack` command is retained only as the remote-sync command
shape; `cqadupstack` itself is already complete.

```bash
python3 scripts/advance_m603_p1_completed_dataset.py \
    --dataset cqadupstack \
    --sync-source-root spark-2:/home/huoju/leask/runs/ii42-m603-p1-product-atoms-official-beir8-spark2-v1 \
    --mirror-root spark-1:/home/huoju/leask/runs/ii42-m603-p1-product-atoms-official-beir8-v1 \
    --compile-datasets nfcorpus,scifact,fiqa,arguana,scidocs,trec-covid,cqadupstack \
    --execute
```

## Legacy / Non-Native Anchors

These anchors are useful for direction and regression checks, but they are not
acceptance artifacts for M603/P1.1 unless explicitly marked as native
DB/plugin-path outputs.

### Broad10 Legacy Anchor

Existing broad10 semantic compiler output:

`runs/m549u_active_locked_broad10_matrix_seed5532/m549_m549u_active_locked_broad10_matrix_seed5532.json`

Existing broad10 fixed-BM25 output:

`runs/m549u_stage2_fixed_bm25_broad10_merged_seed5536/m549u_stage2_fixed_bm25_broad10_merged.json`

Macro from the non-native broad10 surfaces:

| Source | Tasks | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 10 | 0.572630 | 0.435350 | 0.748030 | 0.651790 | 1.000000 |
| `m549u_active_locked` | 10 | 0.572790 | 0.435450 | 0.748020 | 0.651950 | 0.995250 |
| `m549u_active_locked_bm25_zblend` | 10 | 0.590450 | 0.447850 | 0.754940 | 0.663960 | 0.876690 |

Interpretation: broad10 remains a useful sanity anchor for the P1 route and
confirms the older fixed BM25 blend at alpha `0.10`. It is not a complete
M603/P1.1 broad10 matrix because this artifact does not include native DB
publish/query evidence, a standalone BM25 row, or a fixed `a0125` row.

Native broad10 audit at `2026-07-04T08:56Z`: the local PostgreSQL instance
currently contains native schemas for `ii42_beir15`, `ii42_shared15`,
`ii42_p1`, and `ii42_p1_shared15`, but no broad10 schema. The legacy broad10
JSON points to the MTEB task root
`/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks`,
which is not present locally. Therefore the missing broad10 native artifact is
an ingestion/schema bridge from MTEB task roots into the existing native
DB/plugin path. It should reuse the same publisher/query/scoring path already
validated for shared15 and official BEIR8, not introduce a separate evaluator.

Broad10 staging update at `2026-07-04T09:01Z`: `build_ii42_beir15_pg.py` now
accepts MTEB-style custom dataset names when `--expected-from-files` is used,
so broad10 can use the same native schema, BM25 index, VectorChord index, and
query lifecycle as the BEIR/shared surfaces. A local sync job is running in
tmux session `ii42_m603_broad10_native_sync`:

```bash
python3 scripts/build_ii42_beir15_pg.py sync \
    --datasets ArguAna CQADupstackGamingRetrieval CQADupstackUnixRetrieval \
        ClimateFEVERHardNegatives FEVERHardNegatives FiQA2018 \
        HotpotQAHardNegatives SCIDOCS TRECCOVID Touche2020Retrieval.v3 \
    --source spark-1:/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks \
    --staging /Volumes/Betty/Tmp/ii42-broad10-pg-staging \
    --expected-from-files \
    --allow-custom-datasets
```

The remote broad10 task root is about `9.9G`. The first task, `ArguAna`, has
already synced, and `CQADupstackGamingRetrieval` is in progress. After sync
finishes, the next step is native load/index/verify into an `ii42_broad10`
schema before P1 atom generation/publish/eval.

Broad10 native smoke update at `2026-07-04T09:04Z`: `ArguAna` was loaded,
indexed, and verified in the new `ii42_broad10` schema using the existing
builder path:

```text
ArguAna: loaded 8674 docs in 3.5s
ArguAna: loaded 1401 queries
ArguAna: loaded 1401 qrels
ArguAna: bm25 index ready in 1.2s
ArguAna: vector index ready in 0.5s lists=17 probes=4
ArguAna: verify ok docs=8674 queries=1401 qrels=1401 bm25_hits=5
```

A 20-query native baseline smoke also passed through PostgreSQL:

| Source | Queries | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 20 | 0.263046 | 0.179773 | 1.000000 | 0.172062 | 1.000000 |
| `dense` | 20 | 0.464885 | 0.329191 | 1.000000 | 0.329191 | 1.000000 |

The full `ArguAna` baseline smoke was intentionally interrupted after it proved
slow without progress logging; full-surface baseline evaluation should run
after all broad10 tables are indexed, using `--progress-every`. A separate
local MPS P1 atom smoke for broad10 `ArguAna` is running in tmux session
`ii42_m603_broad10_arguana_atoms`. At the checkpoint it had written
`308/8674` document atom rows. The broad10 root sync had advanced to
`TRECCOVID` with about `5.5G` staged locally.

### Shared15 Legacy Anchor

Existing offline/shared15 output:

`runs/m603_p1_fixed_alpha_beir15_shared15_seed603/m603_p1_fixed_alpha_beir15_shared15_seed603.json`

Macro from that non-native evaluator:

| Source | Tasks | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 15 | 0.787990 | 0.694720 | 0.853310 | 0.872180 | 1.000000 |
| `bm25` | 15 | 0.676820 | 0.586240 | 0.779300 | 0.781880 | 0.333690 |
| `m549u_active_locked` | 15 | 0.787990 | 0.694720 | 0.853310 | 0.872180 | 1.000000 |
| `m549_bm25_zblend_a010` | 15 | 0.791600 | 0.699920 | 0.856410 | 0.869530 | 0.924030 |
| `m549_bm25_zblend_a0125` | 15 | 0.792190 | 0.701270 | 0.857030 | 0.869830 | 0.906360 |

Interpretation: the older offline surface supports `a0125` as a plausible
candidate, but it is not an acceptance artifact for M603/P1.1 because the goal
requires the native DB/plugin index path.

Latest native acceptance inventory updates this boundary: the local database
now contains both `ii42_shared15` and `ii42_p1_shared15`, and all `15/15`
shared15 datasets have completed the native publish/query lifecycle. The
offline shared15 matrix is still retained as a regression/sanity anchor, but
the acceptance path is now the native shared15 matrix above, not the older
offline evaluator.

### Official1024 / BEIR8 Semantic Anchor

Existing M549U official1024-style semantic outputs:

`runs/m549u_active_locked_official1024_beir8_matrix_seed5534/m549_m549u_active_locked_official1024_beir8_matrix_seed5534.json`

`runs/m549u_streaming_beir8_full_seed5540/m549u_streaming_broader.json`

Streaming macro from the non-native semantic surface:

| Source | Tasks | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 8 | 0.482980 | 0.314230 | 0.632190 | 0.564650 | 1.000000 |
| `active_locked` | 8 | 0.483140 | 0.314270 | 0.632340 | 0.564350 | 0.995140 |

Interpretation: official1024/BEIR8 already has a streaming-compatible semantic
anchor for M549U, so large-root feasibility is not a model-quality blocker.
The fixed-alpha P1 native BEIR8 matrix is now complete for the expected
`8/8` datasets. The remaining M603/P1.1 engineering gap is broad10 native
coverage beyond the current completed subset.

## Historical Running Work

The following sections are preserved as execution history. They are superseded
by the latest checkpoint at the top of this report.

Previously active remote jobs:

```text
host: spark-1
tmux: ii42_m603_p1_atoms_official_beir8
atom root: /home/huoju/leask/runs/ii42-m603-p1-product-atoms-official-beir8-v1
source root: /home/huoju/leask/runs/ii42-m310b-official-roots-v1
datasets: scifact, fiqa, arguana, scidocs, trec-covid, webis-touche2020,
          cqadupstack

host: spark-2
tmux: ii42_m603_p1_atoms_official_beir8_cqadupstack
atom root: /home/huoju/leask/runs/ii42-m603-p1-product-atoms-official-beir8-spark2-v1
source root: /home/huoju/leask/runs/ii42-m310b-official-roots-v1
datasets: cqadupstack
```

`nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`, and `trec-covid` have been
generated, copied locally, and evaluated through the native path. `spark-1` is
continuing the original sequential official BEIR8 atom job and has moved on to
`webis-touche2020`. `spark-2` was added for the largest pending shard only,
writing to a separate atom root so it cannot overwrite `spark-1` partial files.

Latest observed `spark-1` state at `2026-07-04T04:01:46Z`:

- completed dataset: `trec-covid`
- completed files: `trec-covid/documents.p1_atoms.jsonl`,
  `trec-covid/queries.p1_atoms.jsonl`
- strict line count: `171331/171331` documents and `50/50` queries
- completed `trec-covid` rows were synced locally and evaluated through the
  native DB/plugin path.
- active dataset: `webis-touche2020`
- active file: `webis-touche2020/documents.p1_atoms.jsonl`
- source rows: `382545` documents, `49` queries
- last strict line count: `124080` document rows at `2026-07-04T04:01:46Z`
- progress log: `122976` encoded rows, `0` skipped, `8.33 rows/s`
- GPU: about `96%`
- disk: about `36G` free on `/`
- current `webis-touche2020` shard size: about `3.8G`
- projected complete `webis-touche2020` document atom size: about `12G`
- estimated `webis-touche2020` document completion: about `8.6h`
  (`2026-07-04T12:39Z` UTC), before the small query file is generated

Latest observed `spark-2` state at `2026-07-04T04:01:46Z`:

- active dataset: `cqadupstack`
- active file: `cqadupstack/documents.p1_atoms.jsonl`
- source rows: `457199` documents, `13145` queries
- progress log: `272160` encoded rows, `0` skipped, `10.04 rows/s`
- last strict line count: `273624` document rows at `2026-07-04T04:01:46Z`
- throughput: about `10.04 rows/s`
- GPU: about `96%`
- disk: about `140G` free on `/`
- current atom root size: about `7.8G`
- projected complete `cqadupstack` document atom size: about `14G`
- estimated `cqadupstack` document completion: about `5.1h`
  (`2026-07-04T09:07Z` UTC), before query atom generation
- note: a concurrent DINO/Qwen2VL GPU job is also active on `spark-2`. M603 is
  still making steady progress, so this is resource sharing rather than a stuck
  encoder. Do not kill unrelated jobs to accelerate M603.

Do not merge either shard until both `documents.p1_atoms.jsonl` and
`queries.p1_atoms.jsonl` exist and line counts match their source files.

Latest checkpoint at `2026-07-04T04:01:46Z`:

| Host | Dataset | Split | Rows | Expected | GPU | Disk | State |
| --- | --- | --- | ---: | ---: | --- | --- | --- |
| `spark-1` | `webis-touche2020` | documents | 124080 | 382545 | `96%` | `36G` free | running |
| `spark-1` | `webis-touche2020` | queries | missing | 49 | `96%` | `36G` free | pending after docs |
| `spark-2` | `cqadupstack` | documents | 273624 | 457199 | `96%` | `140G` free | running |
| `spark-2` | `cqadupstack` | queries | missing | 13145 | `96%` | `140G` free | pending after docs |

Local strict atom gate at the same checkpoint reports 12/16 split files ready
using `ii42_beir15.dataset_manifest` row counts: `nfcorpus`, `scifact`, `fiqa`,
`arguana`, `scidocs`, and `trec-covid` have exact documents and queries atom
files with schema samples passing; `webis-touche2020` and `cqadupstack` are
still absent locally and must not be published or evaluated as partial results.

An additional read-only check of `ASA` did not return promptly, so no M603 tail
shard is being assigned there. Keep the active work on `spark-1` and `spark-2`
until one of those datasets reaches exact row-count completion.

`spark-1` still has `cqadupstack` in its original sequential dataset list.
Since `spark-2` is already building `cqadupstack` in a separate root, the safe
dedup strategy is not to interrupt the active `webis-touche2020` writer. After
`spark-2` completes and line counts validate, sync the completed `cqadupstack`
shard into the `spark-1` atom root before `spark-1` reaches that dataset, so
the existing `SKIP_EXISTING=1` guard can skip it.

Space note: `trec-covid` finished at about `5.0G` for documents plus a small
query file. `webis-touche2020` is materially slower than `trec-covid` on the
current host and is now estimated at about `8.7h` from the latest observation.
`spark-1` currently has about `37G` free on `/`. The projected remaining
official BEIR8 atom storage is still feasible, but the margin will be low after
full `webis-touche2020` plus synced `cqadupstack`; duplicate `cqadupstack`
generation would be avoidable waste on a 96%-used filesystem. Sync the completed
`spark-2` `cqadupstack` atom root back to `spark-1` before `spark-1` reaches it.

Resource strategy: do not split or interrupt the active `webis-touche2020`
writer while `spark-2` is still occupied by `cqadupstack` and another GPU
job. If `spark-2` completes `cqadupstack` substantially before
`webis-touche2020`, reassess a bounded tail-shard handoff for webis; only do
that with an explicit line-range plan and merge verification, because the
current `spark-1` writer is producing the canonical prefix file.

Current blocker: remaining native P1 matrix rows are gated by complete product
atom JSONL for `webis-touche2020` and `cqadupstack`. The next likely actionable
transition is `cqadupstack` completion on `spark-2`; after local sync, strict
row-count verification, and sync back to the `spark-1` atom root, the native
matrix can advance from 6/8 to 7/8 while preventing duplicate `cqadupstack`
generation on `spark-1`. `webis-touche2020` remains the longer tail. The local
PostgreSQL `ii42_beir15` surface is not the blocker; all 15 manifest rows are
`indexed`, with `33860494/33860494` docs loaded.

Latest local DB check confirms the same boundary:

- `ii42_beir15.dataset_manifest`: `15/15` datasets are `indexed`;
  `33860494/33860494` docs, `46417/46417` queries, and `161708/161708` qrels
  are loaded.
- `ii42_p1`: fixed-alpha atom tables exist for `nfcorpus`, `scifact`, `fiqa`,
  `arguana`, `scidocs`, and `trec-covid` for both `a010` and `a0125`.
- Missing fixed-alpha atom tables: `webis-touche2020`, `cqadupstack`.

Local `cqadupstack` baseline completed and was copied into:

`runs/m603_p1_native_baselines_v1/cqadupstack/baselines.json`

## Latest Live Checkpoint

Checkpoint at `2026-07-04T08:35Z`.

Local shared15 native matrix is now complete for 15/15 datasets. The remaining
datasets appended since the previous checkpoint are `dbpedia-entity`,
`hotpotqa`, `fever`, `climate-fever`, and `msmarco`; all passed atom-surface
gates before baseline/P1 native evaluation. The local shared15 atom generation
queue is complete.

Final shared15 local atom generation state:

| Dataset | Documents | Queries | State |
| --- | ---: | ---: | --- |
| `hotpotqa` | 2000/2000 | 100/100 | appended |
| `fever` | 2000/2000 | 100/100 | appended |
| `climate-fever` | 2000/2000 | 100/100 | appended |
| `msmarco` | 4102/4102 | 43/43 | appended |

Current shared15 native macro:

| Source | Datasets | Queries | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `BM25` | 15 | 1342 | 0.670058 | 0.593976 | 0.783718 | 0.788108 | - | 0.906167 |
| `dense` | 15 | 1342 | 0.728543 | 0.640422 | 0.778166 | 0.835412 | - | 0.868998 |
| `M549U` | 15 | 1342 | 0.730777 | 0.647687 | 0.835954 | 0.826996 | 0.404354 | 0.940838 |
| `P1-a010` | 15 | 1342 | 0.742546 | 0.660261 | 0.842832 | 0.837364 | 0.414295 | 0.936772 |
| `P1-a0125` | 15 | 1342 | 0.744939 | 0.662742 | 0.843488 | 0.839746 | 0.416677 | 0.937309 |

Official BEIR8 remote generation is healthy and should not be restarted:

| Host | Dataset | Split | Rows | Expected | State |
| --- | --- | --- | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | 260208 | 382545 | running |
| `spark-1` | `webis-touche2020` | queries | missing | 49 | pending after docs |
| `spark-2` | `cqadupstack` | documents | 335904 | 457199 | running |
| `spark-2` | `cqadupstack` | queries | missing | 13145 | pending after docs |

The official BEIR8 blocker is still complete product atom JSONL for
`webis-touche2020` and `cqadupstack`; do not publish or evaluate partial
remote files.

At the latest observed log rates, document generation has about `4.1h`
remaining for `webis-touche2020` and about `4.4h` remaining for `cqadupstack`,
before the much smaller query split generation and local sync/publish/eval
steps.

Disk note: `spark-1` currently has about `32G` free on `/`. That is enough for
the remaining `webis-touche2020` atom output and tiny query split, but it is
still another reason to avoid duplicate `cqadupstack` generation on `spark-1`
after webis finishes.

Deduplication note: the live `spark-1` tmux command still has
`DATASETS=scifact,fiqa,arguana,scidocs,trec-covid,webis-touche2020,cqadupstack`.
After `webis-touche2020` completes, prevent wasteful duplicate `cqadupstack`
generation on `spark-1`: either sync a completed strict-gated `spark-2`
`cqadupstack` directory into the `spark-1` atom root before the `spark-1`
runner reaches it, or stop only the `spark-1` duplicate branch after webis
documents and queries have validated.

## Previous Live Checkpoint

Checkpoint at `2026-07-04T06:15Z`.

Local strict atom gate is still not publishable for the full official BEIR8
surface: 12/16 split files are ready, covering 6/8 datasets. Ready datasets
are `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`, and `trec-covid`.
Missing locally are both `documents` and `queries` atom files for
`webis-touche2020` and `cqadupstack`.

Remote generation is healthy and should not be restarted:

| Host | Dataset | Split | Rows | Expected | GPU | Disk | State |
| --- | --- | --- | ---: | ---: | --- | --- | --- |
| `spark-1` | `webis-touche2020` | documents | 190944 | 382545 | `96%` | `34G` free | running |
| `spark-1` | `webis-touche2020` | queries | missing | 49 | `96%` | `34G` free | pending after docs |
| `spark-2` | `cqadupstack` | documents | 310416 | 457199 | `96%` | `139G` free | running |
| `spark-2` | `cqadupstack` | queries | missing | 13145 | `96%` | `139G` free | pending after docs |

Latest log rates are about `8.34 rows/s` for `webis-touche2020` and
`8.83 rows/s` for `cqadupstack`. From this checkpoint, estimated document
completion is roughly `2026-07-04T12:38Z` for `webis-touche2020` and
`2026-07-04T10:47Z` for `cqadupstack`, before their query atom files are
generated.

Instantaneous GPU samples are not treated as stalls by themselves: both Python
workers are still running and the atom file mtimes and row counts are advancing.
`spark-2` also has `raev2_*` download tmux sessions, but `nvidia-smi` shows the
only GPU process is the P1 `cqadupstack` encoder, so those jobs should not be
stopped for P1 unless they start to block disk/network progress.

Local BEIR15 native baseline coverage is now 15/15. The final missing artifact,
`runs/m603_p1_native_baselines_v1/hotpotqa/baselines.json`, has been written.
This completes baseline coverage for later full BEIR15/shared15 comparison, but
does not change the current official BEIR8 P1 blocker: complete product atom
JSONL for `webis-touche2020` and `cqadupstack`.

Side-effect-free native plan-only verification still passes at this checkpoint:
BEIR15 resolves `30` planned rows (`15` datasets times `P1-a010`/`P1-a0125`)
with `0` skipped rows. The local manifest audit also confirms that
`ii42_beir15` has `15/15` datasets indexed with `33860494/33860494` documents,
`46417/46417` queries, and `161708/161708` qrels loaded. The live blocker is
therefore atom generation readiness, not native DB schema coverage.

No sync, publish, or native P1 evaluation should run from this checkpoint until
one of the missing datasets has exact row counts for both `documents` and
`queries`. The next actionable transition is still likely `cqadupstack`:
sync from `spark-2`, run the strict atom-surface gate locally, mirror the gated
dataset into the `spark-1` atom root, then advance the native matrix from 6/8
to 7/8.

## Latest Verification

Latest local verification at `2026-07-04T00:15Z`:

```bash
python3 -m py_compile \
    scripts/compile_m603_native_surface_matrix.py \
    scripts/create_p1_m549u_checkout.py \
    scripts/evaluate_ii42_beir15_native_baselines.py \
    scripts/evaluate_p1_native_atoms_pg.py \
    scripts/ii42_plain_query.py \
    scripts/load_p1_atom_jsonl_to_pg.py \
    scripts/merge_p1_atom_jsonl_shards.py \
    scripts/plan_p1_atom_jsonl_shards.py \
    scripts/publish_p1_atoms_to_ii42_pg.py \
    scripts/research_sae_m603_p1_fixed_alpha_matrix.py \
    scripts/run_m603_p1_native_surface_matrix.py \
    scripts/test_p1_native_model_backed_smoke.py \
    scripts/research_sae_m549u_encode_jsonl.py

bash -n \
    scripts/run_m603_p1_fixed_alpha_matrix_spark.sh \
    scripts/run_m603_p1_product_atoms_spark.sh \
    scripts/run_m549u_encode_jsonl_spark.sh

python3 -m pytest \
    tests/test_compile_m603_native_surface_matrix.py \
    tests/test_create_p1_m549u_checkout.py \
    tests/test_ii42_plain_query.py \
    tests/test_load_p1_atom_jsonl_to_pg.py \
    tests/test_merge_p1_atom_jsonl_shards.py \
    tests/test_plan_p1_atom_jsonl_shards.py \
    tests/test_publish_p1_atoms_to_ii42_pg.py \
    tests/test_research_sae_m549u_product_atoms.py \
    tests/test_run_m603_p1_product_atoms_spark.py

git diff --check
```

Result: `py_compile`, `bash -n`, and `git diff --check` passed; pytest
reported `18 passed in 3.00s`.

Additional targeted verification at `2026-07-04T00:37Z` after the
side-effect-free plan-only runner update:

```bash
python3 -m py_compile scripts/run_m603_p1_native_surface_matrix.py
python3 -m pytest tests/test_publish_p1_atoms_to_ii42_pg.py -q
python3 scripts/run_m603_p1_native_surface_matrix.py \
    --surface official_beir8_partial \
    --datasets cqadupstack,webis-touche2020 \
    --doc-atom-root runs/m603_p1_product_atoms_official_beir8_v1 \
    --dense-ranking-root runs/m603_p1_native_dense_rankings_v1 \
    --output-root /tmp/m603_plan_only_check \
    --reuse-existing-output \
    --alphas 0.10,0.125 \
    --query-mode hybrid \
    --k 1000 \
    --plan-only
git diff --check
```

Result: `py_compile` passed, targeted pytest reported `3 passed`, the
plan-only preflight emitted four planned rows with zero skipped rows, and
`git diff --check` passed.

Additional atom-surface gate verification at `2026-07-04T00:43Z`:

```bash
python3 -m py_compile scripts/check_m603_p1_atom_surface.py
python3 -m pytest tests/test_check_m603_p1_atom_surface.py -q
python3 scripts/check_m603_p1_atom_surface.py \
    --atom-root runs/m603_p1_product_atoms_official_beir8_v1 \
    --datasets nfcorpus,scifact,fiqa,arguana,scidocs,trec-covid,webis-touche2020,cqadupstack \
    --row-counts-json runs/m603_p1_native_official_beir8_partial_v1/m603_official_beir15_row_counts_from_db.json \
    --output-json /tmp/m603_atom_surface_check_official_beir8.json
```

Result: `py_compile` passed, targeted pytest reported `2 passed`, and the gate
reported 12/16 split files ready: 6/8 datasets complete, with only
`webis-touche2020` and `cqadupstack` missing locally.

Additional sync-helper verification at `2026-07-04T00:49Z`:

```bash
python3 -m py_compile \
    scripts/sync_m603_p1_atom_dataset.py \
    scripts/check_m603_p1_atom_surface.py \
    scripts/run_m603_p1_native_surface_matrix.py
python3 -m pytest \
    tests/test_sync_m603_p1_atom_dataset.py \
    tests/test_check_m603_p1_atom_surface.py \
    tests/test_publish_p1_atoms_to_ii42_pg.py -q
python3 scripts/sync_m603_p1_atom_dataset.py \
    --dataset cqadupstack \
    --source-root spark-2:/home/huoju/leask/runs/ii42-m603-p1-product-atoms-official-beir8-spark2-v1 \
    --dest-root runs/m603_p1_product_atoms_official_beir8_v1 \
    --row-counts-json runs/m603_p1_native_official_beir8_partial_v1/m603_official_beir15_row_counts_from_db.json \
    --output-json /tmp/m603_sync_cqadupstack_dry_run.json
```

Result: `py_compile` passed, targeted pytest reported `9 passed`, and the
sync-helper dry run emitted the expected `rsync --dry-run --partial
--append-verify` command without changing local atom files.

Additional single-dataset advance-helper verification at `2026-07-04T00:51Z`:

```bash
python3 -m py_compile \
    scripts/advance_m603_p1_completed_dataset.py \
    scripts/sync_m603_p1_atom_dataset.py \
    scripts/check_m603_p1_atom_surface.py
python3 -m pytest \
    tests/test_advance_m603_p1_completed_dataset.py \
    tests/test_sync_m603_p1_atom_dataset.py \
    tests/test_check_m603_p1_atom_surface.py \
    tests/test_publish_p1_atoms_to_ii42_pg.py -q
python3 scripts/advance_m603_p1_completed_dataset.py \
    --dataset cqadupstack \
    --sync-source-root spark-2:/home/huoju/leask/runs/ii42-m603-p1-product-atoms-official-beir8-spark2-v1 \
    --compile-datasets nfcorpus,scifact,fiqa,arguana,scidocs,trec-covid,cqadupstack \
    --output-json /tmp/m603_advance_cqadupstack_dry_run.json
```

Result: `py_compile` passed, targeted pytest reported `12 passed`, and the
advance-helper dry run emitted the expected sync -> gate -> native eval ->
compile command sequence without executing any step.

Mirror-step update at `2026-07-04T00:54Z`:

```bash
python3 -m py_compile \
    scripts/advance_m603_p1_completed_dataset.py \
    scripts/sync_m603_p1_atom_dataset.py \
    scripts/check_m603_p1_atom_surface.py
python3 -m pytest \
    tests/test_advance_m603_p1_completed_dataset.py \
    tests/test_sync_m603_p1_atom_dataset.py \
    tests/test_check_m603_p1_atom_surface.py \
    tests/test_publish_p1_atoms_to_ii42_pg.py -q
python3 scripts/advance_m603_p1_completed_dataset.py \
    --dataset cqadupstack \
    --sync-source-root spark-2:/home/huoju/leask/runs/ii42-m603-p1-product-atoms-official-beir8-spark2-v1 \
    --mirror-root spark-1:/home/huoju/leask/runs/ii42-m603-p1-product-atoms-official-beir8-v1 \
    --compile-datasets nfcorpus,scifact,fiqa,arguana,scidocs,trec-covid,cqadupstack \
    --output-json /tmp/m603_advance_cqadupstack_mirror_dry_run.json
```

Result: `py_compile` passed, targeted pytest reported `14 passed`, and the
advance-helper dry run emitted the expected sync -> gate -> mirror -> native
eval -> compile command sequence. In execute mode the helper stops after the
first failed step, so mirror/eval/compile do not run if local sync or atom gate
fails.

Additional live-progress verification at `2026-07-04T01:24Z`:

```bash
python3 scripts/check_m603_p1_atom_surface.py \
    --datasets nfcorpus,scifact,fiqa,arguana,scidocs,trec-covid,webis-touche2020,cqadupstack \
    --atom-root runs/m603_p1_product_atoms_official_beir8_v1 \
    --row-counts-json runs/m603_p1_native_official_beir8_partial_v1/m603_official_beir15_row_counts_from_db.json \
    --output-json /tmp/m603_atom_surface_check_current.json
python3 -m py_compile \
    scripts/advance_m603_p1_completed_dataset.py \
    scripts/sync_m603_p1_atom_dataset.py \
    scripts/check_m603_p1_atom_surface.py \
    scripts/run_m603_p1_native_surface_matrix.py \
    scripts/compile_m603_native_surface_matrix.py \
    scripts/research_sae_m549u_encode_jsonl.py \
    scripts/load_p1_atom_jsonl_to_pg.py \
    scripts/publish_p1_atoms_to_ii42_pg.py \
    scripts/evaluate_p1_native_atoms_pg.py \
    scripts/evaluate_ii42_beir15_native_baselines.py \
    scripts/ii42_plain_query.py
bash -n \
    scripts/run_m549u_encode_jsonl_spark.sh \
    scripts/run_m603_p1_fixed_alpha_matrix_spark.sh \
    scripts/run_m603_p1_product_atoms_spark.sh
python3 -m pytest \
    tests/test_advance_m603_p1_completed_dataset.py \
    tests/test_sync_m603_p1_atom_dataset.py \
    tests/test_check_m603_p1_atom_surface.py \
    tests/test_publish_p1_atoms_to_ii42_pg.py -q
git diff --check
```

Result: local atom gate remains 12/16 split files ready, with
`webis-touche2020` and `cqadupstack` still absent locally; `py_compile`,
`bash -n`, and `git diff --check` passed, and targeted pytest reported
`14 passed in 2.77s`.

Additional live-progress verification at `2026-07-04T04:06Z`:

```bash
python3 scripts/check_m603_p1_atom_surface.py \
    --atom-root runs/m603_p1_product_atoms_official_beir8_v1 \
    --row-counts-json /tmp/m603_beir15_row_counts.json \
    --datasets nfcorpus,scifact,fiqa,arguana,scidocs,trec-covid,webis-touche2020,cqadupstack
/opt/homebrew/opt/postgresql@18/bin/psql -d postgres -Atc \
    "select dataset, expected_docs, expected_queries, loaded_docs, loaded_queries, status from ii42_beir15.dataset_manifest order by dataset;"
ssh spark-1 'tail -n 24 .../logs/webis-touche2020_documents.log'
ssh spark-2 'tail -n 24 .../logs/cqadupstack_documents.log'
```

Result: local BEIR15 manifest remains `15/15` indexed, and the local atom
surface remains `12/16` split files ready. Complete local atom tables are still
limited to `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`, and
`trec-covid`; `webis-touche2020` and `cqadupstack` are still remote-generation
blockers and must not be published as partial files.

Remote generation status:

| Host | Dataset | Split | Progress | GPU | Disk | Current action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `127008/382545` | `96%` | `36G` free | continue writer |
| `spark-2` | `cqadupstack` | documents | `274176/457199` | `93%` | `140G` free | continue writer |

Both active writers are healthy: tmux sessions exist, Docker/Python workers are
active, output files have current mtimes, and GPUs are saturated. At the latest
observed throughput, `cqadupstack` documents are likely to finish first
(`~5.1h` remaining before its query split); `webis-touche2020` documents remain
the longer tail (`~8.6h` remaining before its 49-query split). These are
runtime ETAs, not completion guarantees.

Historical checkpoint note: at `2026-07-04T04:09Z`, BEIR15 baseline coverage
was `14/15` complete. The final missing baseline was `hotpotqa`; its BM25 pass
had completed and dense was running at `300/7405`. PostgreSQL had a valid
`docs_hotpotqa_embedding_vchord_idx` index and the active query was a
VectorChord KNN query with `vchordrq.probes=96`, so this was a heavy baseline
run rather than a missing-index failure.

Follow-up full BEIR15 native plan-only verification at `2026-07-04T04:09Z`
resolved `30` planned rows (`15` datasets times `P1-a010`/`P1-a0125`) with
`0` skipped rows and no publish/eval side effects. This revalidates the DB
route coverage; remaining native matrix work is still gated by atom JSONL
readiness, not by missing table/index resolution.

Additional live-progress verification at `2026-07-04T04:11Z`:

| Host | Dataset | Split | Progress | GPU | Disk | Current action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `129024/382545` | `96%` | `36G` free | continue writer |
| `spark-2` | `cqadupstack` | documents | `276192/457199` | `96%` | `140G` free | continue writer |

Local atom gate is unchanged at `12/16` split files ready. `hotpotqa` dense
baseline progressed to `500/7405`; PostgreSQL is still executing the expected
VectorChord KNN query. No completed atom split is available yet, so sync,
publish, and native eval remain intentionally deferred.

Additional live-progress verification at `2026-07-04T04:24Z`:

| Host | Dataset | Split | Progress | GPU | Disk | Current action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `135696/382545` | `96%` | `36G` free | continue writer |
| `spark-2` | `cqadupstack` | documents | `279816/457199` | `96%` | `140G` free | continue writer |

Both remote atom writers still have active tmux sessions and current output
mtimes. Neither dataset is ready to sync because the document split is still
incomplete and the query split has not started yet. At the latest observed
throughput, `cqadupstack` documents are roughly `5.0h` from completion before
its `13145` query rows, and `webis-touche2020` documents are roughly `8.2h`
from completion before its `49` query rows. These estimates are only runtime
checkpoints; the strict promotion gate remains exact line counts for both
documents and queries.

Local state at the same checkpoint:

- atom surface gate remains `12/16` split files ready, or 6/8 datasets ready;
- missing local atom files remain `webis-touche2020` documents/queries and
  `cqadupstack` documents/queries;
- at this checkpoint, `hotpotqa` baseline was still running locally, with BM25
  complete and dense at `1500/7405`;
- PostgreSQL is executing the expected VectorChord KNN query on
  `ii42_beir15.docs_hotpotqa`, so the baseline tail is compute-bound rather
  than blocked by a missing index.

Next automatic transition: when either remote dataset has exact document and
query atom counts, run `advance_m603_p1_completed_dataset.py --execute` for
that dataset. Until then, do not publish or evaluate partial atom files.

Additional local verification at `2026-07-04T04:26Z`:

```bash
python3 -m py_compile \
    scripts/advance_m603_p1_completed_dataset.py \
    scripts/check_m603_p1_atom_surface.py \
    scripts/compile_m603_native_surface_matrix.py \
    scripts/create_p1_m549u_checkout.py \
    scripts/evaluate_ii42_beir15_native_baselines.py \
    scripts/evaluate_p1_native_atoms_pg.py \
    scripts/ii42_plain_query.py \
    scripts/load_p1_atom_jsonl_to_pg.py \
    scripts/merge_p1_atom_jsonl_shards.py \
    scripts/plan_p1_atom_jsonl_shards.py \
    scripts/publish_p1_atoms_to_ii42_pg.py \
    scripts/research_sae_m603_p1_fixed_alpha_matrix.py \
    scripts/run_m603_p1_native_surface_matrix.py \
    scripts/test_p1_native_model_backed_smoke.py \
    scripts/research_sae_m549u_encode_jsonl.py

bash -n \
    scripts/run_m603_p1_fixed_alpha_matrix_spark.sh \
    scripts/run_m603_p1_product_atoms_spark.sh \
    scripts/run_m549u_encode_jsonl_spark.sh

python3 -m pytest \
    tests/test_advance_m603_p1_completed_dataset.py \
    tests/test_sync_m603_p1_atom_dataset.py \
    tests/test_check_m603_p1_atom_surface.py \
    tests/test_compile_m603_native_surface_matrix.py \
    tests/test_create_p1_m549u_checkout.py \
    tests/test_ii42_plain_query.py \
    tests/test_load_p1_atom_jsonl_to_pg.py \
    tests/test_merge_p1_atom_jsonl_shards.py \
    tests/test_plan_p1_atom_jsonl_shards.py \
    tests/test_publish_p1_atoms_to_ii42_pg.py \
    tests/test_research_sae_m549u_product_atoms.py \
    tests/test_run_m603_p1_product_atoms_spark.py -q

git diff --check
```

Result: `py_compile`, `bash -n`, and `git diff --check` passed. Targeted
pytest reported `30 passed in 3.86s`. The report also passed an explicit
trailing-whitespace check because it is currently untracked and therefore not
covered by `git diff --check`.

Additional live-progress verification at `2026-07-04T04:28Z`:

| Host | Dataset | Split | Progress | GPU | Disk | Current action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `137448/382545` | `96%` | `36G` free | continue writer |
| `spark-2` | `cqadupstack` | documents | `280608/457199` | `96%` | `140G` free | continue writer |

Local atom gate remains `12/16` split files ready, so the native matrix is
still at the 6/8 completed-dataset boundary. The missing files are still
exactly the two remote datasets' document/query atom JSONL files. No sync,
publish, or native eval step was run at this checkpoint because neither remote
dataset has both exact document and query row counts.

Local `hotpotqa` baseline continues independently: BM25 is complete and dense
has reached `1800/7405`. PostgreSQL is still running the expected VectorChord
KNN query against `ii42_beir15.docs_hotpotqa`, so this is continuing work, not
an index or runner failure.

Additional live-progress verification at `2026-07-04T04:41Z`:

| Host | Dataset | Split | Progress | GPU | Disk | Current action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `144072/382545` | `96%` | `36G` free | continue writer |
| `spark-2` | `cqadupstack` | documents | `283800/457199` | `96%` | `140G` free | continue writer |

Both remote atom writers remain healthy: tmux sessions exist, Docker/Python
workers are active, output mtimes are current, and GPUs are saturated. The
local strict atom gate remains `12/16` split files ready: `nfcorpus`,
`scifact`, `fiqa`, `arguana`, `scidocs`, and `trec-covid` are complete; local
`webis-touche2020` and `cqadupstack` atom files are still absent and must not
be published as partial results.

Runtime ETA from the latest logs:

- `cqadupstack` documents: about `5.0h` remaining at roughly `9.6 rows/s`,
  before the `13145` query rows. This remains the likely next actionable
  transition.
- `webis-touche2020` documents: about `7.9h` remaining at roughly
  `8.35 rows/s`, before the small `49` query split.
- Local `hotpotqa` baseline: BM25 is complete and dense has reached
  `2900/7405`; PostgreSQL is still executing the expected VectorChord KNN
  query, so the final BEIR15 baseline gap is progressing rather than blocked.

Next automatic transition remains unchanged: when a remote dataset has exact
document and query atom counts, run the single-dataset advance helper in
`--execute` mode. `cqadupstack` should include `--mirror-root` so the completed
and gated atom directory is copied back into the `spark-1` atom root before the
sequential `spark-1` job reaches that dataset.

Additional live-progress verification at `2026-07-04T04:43Z`:

| Host | Dataset | Split | Progress | GPU | Disk | Current action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `145200/382545` | `96%` | `36G` free | continue writer |
| `spark-2` | `cqadupstack` | documents | `284520/457199` | `96%` | `140G` free | continue writer |

Local atom gate remains unchanged at `12/16` split files ready, or 6/8
datasets. `webis-touche2020` and `cqadupstack` still have no local complete
atom JSONL files, so no sync, publish, or native eval step was run.

The local `hotpotqa` baseline continues independently. BM25 is complete and
dense has reached `3100/7405`; PostgreSQL is still executing the expected
VectorChord KNN query against `ii42_beir15.docs_hotpotqa`.

Current ETA from the latest checkpoints:

- `cqadupstack` documents: roughly `5.0h` remaining at the latest observed
  `9.52 rows/s`, before the `13145` query split.
- `webis-touche2020` documents: roughly `7.9h` remaining at the latest observed
  `8.35 rows/s`, before the `49` query split.
- `hotpotqa` dense baseline: roughly `1h` remaining if the current local
  VectorChord rate holds.

Additional live-progress verification at `2026-07-04T04:45Z`:

| Host | Dataset | Split | Progress | GPU | Disk | Current action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `145968/382545` | `96%` | `36G` free | continue writer |
| `spark-2` | `cqadupstack` | documents | `284928/457199` | `96%` | `140G` free | continue writer |

The local atom gate is still `12/16` split files ready. The missing files are
unchanged: `webis-touche2020` documents/queries and `cqadupstack`
documents/queries. Therefore no `sync`, `publish`, or native `eval` command was
executed at this checkpoint.

Historical checkpoint note: at this point, local `hotpotqa` baseline remained
healthy: BM25 was complete, dense had reached `3200/7405`, and PostgreSQL was
still running the expected VectorChord KNN query. Baseline coverage was
`14/15` until `hotpotqa/baselines.json` was written.

Current ETA from this checkpoint:

- `cqadupstack` documents: about `5.0h` remaining before query atom generation.
- `webis-touche2020` documents: about `7.9h` remaining before query atom
  generation.
- `hotpotqa` dense baseline: about `1h` remaining if the current local
  VectorChord rate holds.

Additional live-progress verification at `2026-07-04T04:56Z`:

| Host | Dataset | Split | Progress | GPU | Disk | Current action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `151752/382545` | `96%` | `36G` free | continue writer |
| `spark-2` | `cqadupstack` | documents | `289200/457199` | `96%` | `140G` free | continue writer |

Both remote atom writers remain healthy. The `spark-1` and `spark-2` tmux
sessions still exist, Docker/Python workers are active, output files are
advancing, and both GPUs are saturated. The local strict atom gate remains
`12/16` split files ready: `nfcorpus`, `scifact`, `fiqa`, `arguana`,
`scidocs`, and `trec-covid` are complete; `webis-touche2020` and
`cqadupstack` still lack complete local documents and queries atom JSONL files.

Historical checkpoint note: local baseline coverage was still `14/15`. The
final `hotpotqa` baseline was running in tmux: BM25 was complete, dense had
reached `4000/7405`, and PostgreSQL was executing the expected VectorChord KNN
query on `ii42_beir15.docs_hotpotqa`. This was compute-bound progress, not an
index failure.

No sync, publish, or native eval command was run at this checkpoint because
neither remote dataset has both exact document and query row counts. The next
valid transition is still strict completion of `cqadupstack` or
`webis-touche2020`, followed by `advance_m603_p1_completed_dataset.py
--execute`.

Additional live-progress verification at `2026-07-04T05:11Z`:

| Host | Dataset | Split | Progress | GPU | Disk | Current action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `158808/382545` | `96%` | `35G` free | continue writer |
| `spark-2` | `cqadupstack` | documents | `293160/457199` | `96%` | `139G` free | continue writer |

Both remote writers remain healthy: tmux sessions exist, Docker/Python workers
are active, output files have current mtimes, and GPUs are saturated. The local
strict atom gate is unchanged at `12/16` split files ready: the complete local
datasets are still `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`, and
`trec-covid`; `webis-touche2020` and `cqadupstack` still lack complete local
documents and queries atom JSONL files.

The remote query splits have not started yet. At the latest observed
throughput, `cqadupstack` documents are roughly `4.9h` from completion before
the `13145` query rows, and `webis-touche2020` documents are roughly `7.5h`
from completion before the `49` query rows. These are operational ETAs only;
the promotion gate remains exact document and query row counts plus schema
verification.

Historical checkpoint note: local baseline coverage was `14/15`. The final
`hotpotqa` baseline was still running locally: BM25 was complete, dense had
reached `5200/7405`, and PostgreSQL was executing the expected VectorChord KNN
query on `ii42_beir15.docs_hotpotqa`. This was compute-bound progress, not an
index failure.

Storage note: local workspace has about `229GiB` free. `spark-1` has about
`35G` free and currently holds a `13G` P1 atom root; `spark-2` has about
`139G` free and currently holds an `8.5G` `cqadupstack` atom root. The planned
`cqadupstack` mirror back to `spark-1` is still feasible, but the next sync
checkpoint must recheck disk space before copying the completed dataset.

No sync, publish, or native eval command was run at this checkpoint because
neither remote dataset has both exact document and query row counts. The next
valid transition remains strict completion of `cqadupstack` or
`webis-touche2020`, followed by the single-dataset advance helper in
`--execute` mode.

Additional live-progress verification at `2026-07-04T06:40Z`:

| Host | Dataset | Split | Progress | GPU | Disk | Current action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `203112/382545` | `96%` | `34G` free | continue writer |
| `spark-2` | `cqadupstack` | documents | `316080/457199` | `96%` | `139G` free | continue writer |

Both remote writers remain healthy: tmux sessions exist, Docker/Python workers
are active, output mtimes are current, and both GPUs are saturated. The local
strict atom gate is unchanged at `12/16` split files ready: complete local
datasets are `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`, and
`trec-covid`; `webis-touche2020` and `cqadupstack` still lack complete local
documents and queries atom JSONL files.

The remote query splits have not started yet. From the latest observed
throughput, `cqadupstack` documents are roughly `4.5h` from completion before
the `13145` query rows, and `webis-touche2020` documents are roughly `6.0h`
from completion before the `49` query rows. These are operational ETAs only;
the promotion gate remains exact document and query row counts plus schema
verification.

Additional live-progress verification at `2026-07-04T07:14Z`:

| Location | Dataset | Split | Progress | GPU | Current action |
| --- | --- | --- | ---: | ---: | --- |
| local `Flora` | `trec-covid` shared15 | documents | `6371/17537` | MPS active | continue writer |
| `spark-1` | `webis-touche2020` official | documents | `219480/382545` | `96%` | continue writer |
| `spark-2` | `cqadupstack` official | documents | `321912/457199` | `96%` | continue writer |

Native DB readiness for completed shared15 rows was checked directly via
`ii42_model_deployment_status`. All completed `M549U`, `P1-a010`, and
`P1-a0125` indexes for `nfcorpus`, `scifact`, `arguana`, `scidocs`, and
`fiqa` returned `atom_query_ready=true`, `generation_ready=true`, and
`scoring_ready=true`. `text_query_ready=false` is expected for this
precomputed atom-mode milestone.

No publish/eval was launched at this checkpoint. The next valid transition is
local `trec-covid` shared15 completion, then strict atom surface validation,
native baseline check/reuse, native `M549U`/`P1-a010`/`P1-a0125` eval, and
shared15 combined matrix recompilation to `6/15`.

Additional live-progress verification at `2026-07-04T07:31Z`:

| Location | Dataset | Split | Progress | Current action |
| --- | --- | --- | ---: | --- |
| local `Flora` | `trec-covid` shared15 | documents | `17537/17537` | completed |
| local `Flora` | `trec-covid` shared15 | queries | `50/50` | completed |
| local `Flora` | `webis-touche2020` shared15 | documents | `852/2000` | continue writer |
| local `Flora` | `webis-touche2020` shared15 | queries | `0/49` | pending |

`trec-covid` was appended to the native shared15 matrix after strict surface
validation. The combined matrix now covers `6/15` shared15 datasets and still
selects `P1-a0125` over `P1-a010` on macro NDCG/MAP/Recall/MRR. This remains
a candidate-level signal, not a final default promotion.

Additional local verification at `2026-07-04T06:40Z`:

```bash
python3 -m py_compile \
    scripts/create_p1_m549u_checkout.py \
    scripts/load_p1_atom_jsonl_to_pg.py \
    scripts/publish_p1_atoms_to_ii42_pg.py \
    scripts/evaluate_p1_native_atoms_pg.py \
    scripts/run_m603_p1_native_surface_matrix.py \
    scripts/compile_m603_native_surface_matrix.py \
    scripts/check_m603_p1_atom_surface.py \
    scripts/advance_m603_p1_completed_dataset.py \
    scripts/sync_m603_p1_atom_dataset.py \
    scripts/merge_p1_atom_jsonl_shards.py \
    scripts/plan_p1_atom_jsonl_shards.py \
    scripts/research_sae_m549u_encode_jsonl.py

bash -n \
    scripts/run_m549u_encode_jsonl_spark.sh \
    scripts/run_m603_p1_product_atoms_spark.sh \
    scripts/run_m603_p1_fixed_alpha_matrix_spark.sh

python3 -m pytest \
    tests/test_research_sae_m549u_product_atoms.py \
    tests/test_create_p1_m549u_checkout.py \
    tests/test_load_p1_atom_jsonl_to_pg.py \
    tests/test_publish_p1_atoms_to_ii42_pg.py \
    tests/test_ii42_plain_query.py \
    tests/test_run_m603_p1_product_atoms_spark.py \
    tests/test_plan_p1_atom_jsonl_shards.py \
    tests/test_merge_p1_atom_jsonl_shards.py \
    tests/test_check_m603_p1_atom_surface.py \
    tests/test_compile_m603_native_surface_matrix.py \
    tests/test_advance_m603_p1_completed_dataset.py \
    tests/test_sync_m603_p1_atom_dataset.py

git diff --check
```

Result: `py_compile`, `bash -n`, and `git diff --check` passed; pytest
reported `30 passed in 3.22s`. Full BEIR15 side-effect-free native plan-only
verification also still resolves `30` planned rows with `0` skipped rows.

Additional live-progress verification at `2026-07-04T08:09Z`:

| Location | Dataset | Split | Progress | Current action |
| --- | --- | --- | ---: | --- |
| local `Flora` | `webis-touche2020` shared15 | documents | `2000/2000` | appended |
| local `Flora` | `webis-touche2020` shared15 | queries | `49/49` | appended |
| local `Flora` | `cqadupstack` shared15 | documents | `2000/2000` | appended |
| local `Flora` | `cqadupstack` shared15 | queries | `100/100` | appended |
| local `Flora` | `quora` shared15 | documents | `2000/2000` | appended |
| local `Flora` | `quora` shared15 | queries | `100/100` | appended |
| local `Flora` | `nq` shared15 | documents | `2000/2000` | appended |
| local `Flora` | `nq` shared15 | queries | `100/100` | appended |
| local `Flora` | `dbpedia-entity` shared15 | documents | `3357/3357` | appended |
| local `Flora` | `dbpedia-entity` shared15 | queries | `100/100` | appended |
| local `Flora` | `hotpotqa` shared15 | documents | `2000/2000` | appended |
| local `Flora` | `hotpotqa` shared15 | queries | `100/100` | appended |
| local `Flora` | `fever` shared15 | documents | `2000/2000` | appended |
| local `Flora` | `fever` shared15 | queries | `100/100` | appended |
| local `Flora` | `climate-fever` shared15 | documents | `2000/2000` | appended |
| local `Flora` | `climate-fever` shared15 | queries | `100/100` | appended |
| local `Flora` | `msmarco` shared15 | documents | `4102/4102` | appended |
| local `Flora` | `msmarco` shared15 | queries | `43/43` | appended |
| `spark-1` | `webis-touche2020` official | documents | `247152/382545` | continue writer |
| `spark-2` | `cqadupstack` official | documents | `331512/457199` | continue writer |

The shared15 combined matrix now covers `15/15` datasets. The latest appended
datasets preserve the macro ordering: `P1-a0125` remains ahead of `P1-a010` on
NDCG@10, MAP@100, Recall@100, MRR@20, dense overlap@100, and candidate upper
bound. This completes the shared15 broader validation surface.

Additional official-generation check at `2026-07-04T08:16Z`:

| Host | Dataset | Split | Progress | GPU | Disk | Action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `250800/382545` | `96%` | `33G` free | continue writer |
| `spark-1` | `webis-touche2020` | queries | `0/49` | pending | `33G` free | wait for docs |
| `spark-2` | `cqadupstack` | documents | `332784/457199` | `96%` | `138G` free | continue writer |
| `spark-2` | `cqadupstack` | queries | `0/13145` | pending | `138G` free | wait for docs |

Both tmux sessions are alive and the document atom files are still advancing.
No sync/publish/eval should run from this checkpoint because neither missing
official dataset has a complete query atom file. At the latest log rates,
both document files are still several hours from completion before their query
splits begin. Local official atom files for `webis-touche2020` and
`cqadupstack` remain absent, so there is no local partial surface to gate.

Additional local verification at `2026-07-04T08:06Z`:

```bash
python3 -m py_compile \
    scripts/create_p1_m549u_checkout.py \
    scripts/load_p1_atom_jsonl_to_pg.py \
    scripts/publish_p1_atoms_to_ii42_pg.py \
    scripts/evaluate_p1_native_atoms_pg.py \
    scripts/run_m603_p1_native_surface_matrix.py \
    scripts/compile_m603_native_surface_matrix.py \
    scripts/check_m603_p1_atom_surface.py \
    scripts/advance_m603_p1_completed_dataset.py \
    scripts/sync_m603_p1_atom_dataset.py \
    scripts/merge_p1_atom_jsonl_shards.py \
    scripts/plan_p1_atom_jsonl_shards.py \
    scripts/research_sae_m549u_encode_jsonl.py \
    scripts/evaluate_ii42_beir15_native_baselines.py \
    scripts/ii42_plain_query.py

bash -n \
    scripts/run_m549u_encode_jsonl_spark.sh \
    scripts/run_m603_p1_product_atoms_spark.sh \
    scripts/run_m603_p1_fixed_alpha_matrix_spark.sh

python3 -m pytest \
    tests/test_research_sae_m549u_product_atoms.py \
    tests/test_create_p1_m549u_checkout.py \
    tests/test_load_p1_atom_jsonl_to_pg.py \
    tests/test_publish_p1_atoms_to_ii42_pg.py \
    tests/test_ii42_plain_query.py \
    tests/test_run_m603_p1_product_atoms_spark.py \
    tests/test_plan_p1_atom_jsonl_shards.py \
    tests/test_merge_p1_atom_jsonl_shards.py \
    tests/test_check_m603_p1_atom_surface.py \
    tests/test_compile_m603_native_surface_matrix.py \
    tests/test_advance_m603_p1_completed_dataset.py \
    tests/test_sync_m603_p1_atom_dataset.py -q

git diff --check
```

Result: `py_compile`, `bash -n`, and `git diff --check` passed; pytest
reported `31 passed in 3.60s`. Local tmux session
`ii42_m603_p1_atoms_shared15_local` has exited, confirming the shared15 atom
generation queue is finished.

Additional official-generation check at `2026-07-04T08:38Z`:

| Host | Dataset | Split | Progress | GPU | Disk | Action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `261936/382545` | `96%` | `32G` free | continue writer |
| `spark-1` | `webis-touche2020` | queries | `0/49` | pending | `32G` free | wait for docs |
| `spark-2` | `cqadupstack` | documents | `336648/457199` | `96%` | `138G` free | continue writer |
| `spark-2` | `cqadupstack` | queries | `0/13145` | pending | `138G` free | wait for docs |

Both remote encoders are healthy and still in the document split. The current
observed log rates are about `8.32 rows/s` for `webis-touche2020` and
`7.65 rows/s` for `cqadupstack`. At these rates, document completion is still
roughly `4.0h` for `webis-touche2020` and `4.4h` for `cqadupstack`, before the
query splits. `spark-1` is now the operational risk because the root filesystem
has only about `32G` free; do not let it start duplicate `cqadupstack`
generation after `webis-touche2020` finishes. The next valid transition remains
strict completion of one full dataset, followed by
`advance_m603_p1_completed_dataset.py --execute`.

Local strict gate status from the same checkpoint:

| Scope | Datasets | Gate |
| --- | ---: | --- |
| completed official subset | `6/8` | ready |
| full official BEIR8 | `8/8` | blocked by missing atom JSONL |

The ready subset is `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`, and
`trec-covid`; all six have exact document/query row counts and schema samples.
The full gate fails only because local atom JSONL is absent for
`webis-touche2020` documents/queries and `cqadupstack` documents/queries. This
confirms the current blocker is data completion, not native loader/publisher
logic.

Additional official-generation check at `2026-07-04T08:43Z`:

| Host | Dataset | Split | Progress | GPU | Disk | Action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `264264/382545` | `96%` | `32G` free | continue writer |
| `spark-1` | `webis-touche2020` | queries | `0/49` | pending | `32G` free | wait for docs |
| `spark-2` | `cqadupstack` | documents | `337608/457199` | `96%` | `138G` free | continue writer |
| `spark-2` | `cqadupstack` | queries | `0/13145` | pending | `138G` free | wait for docs |

Both remote tmux jobs are still healthy and advancing. Local full-gate check
still reports exactly four missing files: `webis-touche2020`
documents/queries and `cqadupstack` documents/queries. No native publish/eval
should run until at least one missing dataset completes documents and queries
and passes the strict atom surface checker.

Additional official-generation check at `2026-07-04T08:56Z`:

| Host | Dataset | Split | Progress | GPU | Disk | Action |
| --- | --- | --- | ---: | ---: | ---: | --- |
| `spark-1` | `webis-touche2020` | documents | `270936/382545` | `96%` | `32G` free | continue writer |
| `spark-1` | `webis-touche2020` | queries | `0/49` | pending | `32G` free | wait for docs |
| `spark-2` | `cqadupstack` | documents | `340056/457199` | `96%` | `138G` free | continue writer |
| `spark-2` | `cqadupstack` | queries | `0/13145` | pending | `138G` free | wait for docs |

Both remote tmux sessions are still alive and both encoder processes are using
the GPU. The latest observed log rates are about `8.32 rows/s` for
`webis-touche2020` and `7.59 rows/s` for `cqadupstack`. At those rates,
document completion is still roughly `3.7h` for `webis-touche2020` and
`4.3h` for `cqadupstack`, before each query split is generated. No local
sync, publish, or native eval was run at this checkpoint because the complete
dataset gate still requires both document and query atom files.

Local strict atom gate remains unchanged: `nfcorpus`, `scifact`, `fiqa`,
`arguana`, `scidocs`, and `trec-covid` are ready; `webis-touche2020` and
`cqadupstack` are missing local document/query atom JSONL. The next valid
transition is still `advance_m603_p1_completed_dataset.py --execute` after one
of the remote datasets reaches exact document and query row counts.

Additional live checkpoint at `2026-07-04T13:36:50Z`:

| Host | Dataset | Split | Progress | GPU | Action |
| --- | --- | --- | ---: | ---: | --- |
| `spark-1` | `FiQA2018` broad10 | documents | `14256/57638` | `96%` | continue writer |
| `spark-1` | `FiQA2018` broad10 | queries | missing / `648` | pending | wait for docs |
| `spark-2` | `cqadupstack` official | documents | `447096/457199` | `96%` | continue writer |
| `spark-2` | `cqadupstack` official | queries | missing / `13145` | pending | wait for docs |

Both remote tmux jobs are alive and both GPUs are loaded. The official
`cqadupstack` log shows a document throughput of roughly `7.2 rows/s`, so the
remaining document split is still in the tens-of-minutes range before the
query split starts. No sync, gate, publish, or eval was run at this checkpoint
because neither active dataset has complete document and query atom JSONL.

The single-dataset advance helper was corrected in this checkpoint:

```bash
python3 -m py_compile scripts/advance_m603_p1_completed_dataset.py
python3 -m pytest -q tests/test_advance_m603_p1_completed_dataset.py
git diff --check -- \
    scripts/advance_m603_p1_completed_dataset.py \
    tests/test_advance_m603_p1_completed_dataset.py
```

Result: all passed. The helper now defaults to `--alphas 0,0.10,0.125`, which
prevents future completed official rows from compiling without the `M549U`
semantic-only baseline.

Additional live checkpoint at `2026-07-04T14:47:12Z`:

| Host | Dataset | Split / Stage | Progress | Action |
| --- | --- | --- | ---: | --- |
| `spark-2` | `cqadupstack` official | documents | `457199/457199` | completed |
| `spark-2` | `cqadupstack` official | queries | `13145/13145` | completed |
| local `Flora` | `cqadupstack` official | sync + strict gate | complete | native eval running |
| local `Flora` | `cqadupstack` official | native eval | `alpha=0` query loop | continue |
| `spark-1` | `FiQA2018` broad10 | documents | `57638/57638` | completed |
| `spark-1` | `FiQA2018` broad10 | queries | `648/648` | completed |
| local `Flora` | `FiQA2018` broad10 | sync + strict gate | complete | queue native eval |

Official `cqadupstack` is no longer a remote-generation blocker. The active
command is:

```bash
python3 scripts/advance_m603_p1_completed_dataset.py \
    --dataset cqadupstack \
    --sync-source-root \
        spark-2:/home/huoju/leask/runs/ii42-m603-p1-product-atoms-official-beir8-spark2-v1 \
    --compile-datasets \
        nfcorpus,scifact,fiqa,arguana,scidocs,trec-covid,webis-touche2020,cqadupstack \
    --execute \
    --output-json \
        runs/m603_p1_native_official_beir8_partial_v1/m603_advance_cqadupstack_execute.json
```

The helper completed local sync, strict gate, native publish/eval for `M549U`,
`P1-a010`, and `P1-a0125`, then recompiled the official BEIR8 combined matrix
to `8/8`. `FiQA2018` broad10 atom sync and strict gate also completed, and its
native eval was run after the official `cqadupstack` eval released local
PostgreSQL.

Additional live checkpoint at `2026-07-04T15:59:34Z`:

| Scope | Stage | Status |
| --- | --- | --- |
| official BEIR8 | native matrix | complete, `8/8` datasets |
| broad10 | native completed subset | complete, `9/10` rows |
| shared15 | native matrix | complete, `15/15` datasets |
| local PostgreSQL | M603 eval process | idle |
| spark-1 | `Touche2020Retrieval.v3` tail shard | atom generation active |
| spark-2 | `Touche2020Retrieval.v3` prefix/full stream | atom generation active |
| next M603 work | final broad10 row | merge Touche shards, gate, eval |

`TRECCOVID` completed on `spark-1`, synced locally, passed strict gate, and was
evaluated through the native DB/plugin path. `HotpotQAHardNegatives` completed
on `spark-2` and was also synced, gated, evaluated, and included in the
completed9 matrix. The old `spark-2` remaining-row queue was stopped after
Hotpot completed because it started duplicate `TRECCOVID` generation; it was
replaced by a `Touche2020Retrieval.v3`-only session. To reduce the final-row
tail latency, `spark-1` is also generating a Touche document tail shard for
lines `150001-303732` under a separate shard root. The final Touche transition
requires merging the prefix/full stream with the tail shard, then running the
same strict gate and native eval path.

## Checkpoint Recommendation At `2026-07-04T15:59Z`

At this checkpoint, `P1-a0125` was the fixed-alpha candidate for the native
P1.1 line. It beat `P1-a010` on all tracked macro metrics across the full
shared15 matrix, the then-current eight-dataset official subset, and the
then-current broad10 completed subset. This is superseded by the latest
checkpoint at the top of the report.

Additional live checkpoint at `2026-07-04T20:39:55Z`:

| Host | Dataset | Split / Shard | Progress | GPU / Runtime | Action |
| --- | --- | --- | ---: | --- | --- |
| `spark-2` | `Touche2020Retrieval.v3` | documents prefix stream | `44456/75000` target | GPU `96%` | continue to prefix cutoff |
| local `Flora` | `Touche2020Retrieval.v3` | documents middle shard `75001-150000` | started, `172/75000` at warm-up check | MPS | continue shard |
| `spark-1` | `Touche2020Retrieval.v3` | documents tail shard `150001-303732` | `27104/153732` | GPU `96%` | continue shard |
| local `Flora` | `Touche2020Retrieval.v3` | queries | `49/49` | MPS | complete |
| `ASA` | third remote shard | unavailable | offline in Tailscale, last seen 10d ago | - | do not use |

The final broad10 row is now planned as a three-way document merge, not the
earlier two-way merge:

```text
1-75000        spark-2 prefix stream
75001-150000   local Flora middle shard
150001-303732  spark-1 tail shard
```

The local middle shard is intentionally written to
`runs/m603_p1_product_atoms_broad10_v1/Touche2020Retrieval.v3/documents.p1_atoms.local_075001_150000.jsonl`;
it is not the final document atom file. The local query atom file is complete
at
`runs/m603_p1_product_atoms_broad10_v1/Touche2020Retrieval.v3/queries.p1_atoms.jsonl`.
When `spark-2` reaches at least `75000` rows, stop only the Touche prefix
session if it would otherwise keep running, cut the first `75000` rows into a
prefix shard, then merge the three exact ranges with
`scripts/merge_p1_atom_jsonl_shards.py`. After merge, run the strict atom
surface checker before native publish/eval.

Follow-up at `2026-07-04T20:42Z`: the local Flora middle-shard attempt failed
with MPS out-of-memory after writing `696` rows. The partial file was preserved
as
`runs/m603_p1_product_atoms_broad10_v1/Touche2020Retrieval.v3/documents.p1_atoms.local_075001_150000.failed_mps_oom.partial.jsonl`
so it cannot be accidentally used as a valid shard. Do not include that file in
the final merge. Because the middle range `75001-150000` is not covered by any
valid shard, the valid near-term plan is back to the Spark-only two-way merge:
keep `spark-2` running until it reaches at least `150000` prefix rows, and keep
`spark-1` running until the `150001-303732` tail shard is complete.

Follow-up local acceleration test at `2026-07-04T20:48Z`: a short MPS
`batch_size=8` chunk for lines `75001-75512` completed `512/512` rows at about
`5.86 rows/s`. That is stable enough for diagnostics but slower than the
Spark-only critical path once repeated model reload overhead is included.
Therefore the local MPS middle shard is not promoted into the merge plan.

Additional live checkpoint at `2026-07-04T20:54:36Z`:

| Host | Dataset | Split / Shard | Progress | Resource status | Action |
| --- | --- | --- | ---: | --- | --- |
| `spark-2` | `Touche2020Retrieval.v3` | documents prefix stream | `53960/150000` | GPU `96%`, disk `121G` free | continue |
| `spark-1` | `Touche2020Retrieval.v3` | documents tail `150001-303732` | `36696/153732` | GPU `96%`, disk `19G` free | continue, watch disk |
| local `Flora` | `Touche2020Retrieval.v3` | queries | `49/49` | complete | keep |
| local `Flora` | `Touche2020Retrieval.v3` | middle shard test | failed partial `696` rows plus diagnostic `512` rows | not valid for merge | exclude |

The active merge plan remains Spark-only:

```text
1-150000       spark-2 prefix stream, cut with head if it overshoots
150001-303732  spark-1 tail shard
```

At the observed rates, both remote shards are expected to finish around
`2026-07-05T00:00Z`. No restart is warranted: both tmux sessions are active,
both GPUs are loaded, and logs continue to advance. The only operational risk
is `spark-1` disk headroom; the expected remaining tail output is still within
available space, but it should be rechecked before sync/merge.

Local verification at `2026-07-04T20:55Z`:

- `py_compile` passed for the modified/new M603 Python scripts and tests.
- `bash -n` passed for the modified M549U/M603 runner scripts.
- `git diff --check` passed.
- `pytest` passed for the M603/P1 DB, atom, merge, sync, compile, and runner
  unit tests: `36 passed`.

Follow-up at `2026-07-04T20:58Z`: both remote shards are still advancing and
should not be interrupted. `spark-2` prefix reached `56240/150000`; `spark-1`
tail reached `39032/153732`. Local source row counts remain exact:
`303732` Touche documents and `49` queries, with `49/49` query atoms ready.
The `spark-1` tail shard is estimated to need roughly `3.6GiB` more output
space, so the observed `19G` free is tight but sufficient if no unrelated job
fills the disk.

Correction at `2026-07-04T21:02Z`: the apparent `spark-1` tail shard was not
a valid tail. The remote `run_m603_p1_product_atoms_spark.sh` was older than
the local runner and ignored `DOCUMENT_LINE_START`, `DOCUMENT_LINE_END`, and
`SHARD_LABEL`; the live python process showed `--line-start 1 --line-end 0`.
That duplicate-prefix job was stopped, and its output was preserved as
`documents.p1_atoms.wrong_prefix_duplicate.partial.jsonl` so it cannot be
merged accidentally. The updated runner was synced to both `spark-1` and
`spark-2`, then the correct `spark-1` tail session was restarted with
`--line-start 150001 --line-end 303732` and output
`documents.p1_atoms.tail_150001_303732.jsonl`. The corrected tail wrote its
first `536` rows and is running with GPU `96%`. `spark-2` prefix reached
`58784/150000` at the same checkpoint. The critical path is now the restarted
tail shard, with an approximate finish window around `2026-07-05T01:45Z` if
the observed throughput holds.

Follow-up at `2026-07-04T21:04Z`: the corrected `spark-1` tail reached
`1360/153732`, and `spark-2` prefix reached `59616/150000`. The next dynamic
resource decision point is the `spark-2` prefix cutoff. Once prefix reaches
`150000`, stop only the `ii42_m603_broad10_touche_atoms` prefix session to
avoid wasting GPU on duplicate tail work. If the corrected `spark-1` tail is
still far from completion, start an additional `spark-2` upper-tail shard using
the synced line-range-aware runner, then merge exact non-overlapping ranges:

```text
1-150000          spark-2 prefix, cut with head if needed
150001-B          spark-1 corrected tail, cut with head if it overshoots B
B+1-303732        optional spark-2 upper-tail shard
```

Choose `B` from the live `spark-1` tail row count at prefix cutoff. This keeps
the merge auditable while recovering part of the delay caused by the stale
remote runner.

Follow-up at `2026-07-04T21:05Z`: both effective shards continued to advance.
`spark-2` prefix reached `60464/150000`; the corrected `spark-1` tail reached
`2312/153732`. The `spark-1` process still shows the correct range arguments
`--line-start 150001 --line-end 303732`, and the stale duplicate-prefix output
remains isolated as `documents.p1_atoms.wrong_prefix_duplicate.partial.jsonl`.
No merge, stop, or range split is safe yet.

Resource adjustment at `2026-07-04T21:08Z`: `spark-2` was still running the
old full-stream process with `--line-end 0`, which would overshoot `150000`
and waste GPU on duplicate tail work if left unattended. That prefix session
was stopped at `61328` rows and restarted with the synced runner using
`DOCUMENT_LINE_START=1 DOCUMENT_LINE_END=150000`, same output file, and
`RESUME=1`. The restarted process shows `--line-start 1 --line-end 150000`,
and the file advanced to `61736`, confirming resume is healthy. The corrected
`spark-1` tail advanced to `4296/153732` in the same window.

Follow-up at `2026-07-04T21:09Z`: the self-stopping `spark-2` prefix reached
`62480/150000`, and the corrected `spark-1` tail reached `4904/153732`.
Both GPUs remain loaded and both logs continue to advance. The next safe action
is still the prefix cutoff decision; no merge or range split is safe before
`spark-2` reaches `150000`.

Follow-up at `2026-07-04T21:10Z`: the self-stopping `spark-2` prefix reached
`63200/150000`, and the corrected `spark-1` tail reached `5648/153732`.
`spark-2` has an unrelated new tmux session, but the M603 prefix process is
still active and using the correct `--line-end 150000` range. No intervention
is needed.

Follow-up at `2026-07-04T21:11Z`: the self-stopping `spark-2` prefix reached
`63568/150000`, and the corrected `spark-1` tail reached `6176/153732`. Both
effective processes still show the intended non-overlapping line ranges. No
cutoff, merge, or upper-tail split is safe yet.

Follow-up at `2026-07-04T21:14Z`: the self-stopping `spark-2` prefix reached
`64448/150000`, and the corrected `spark-1` tail reached `7776/153732`.
`spark-1` free disk dropped to `18G`; based on the corrected tail shard size,
the remaining valid tail output is estimated at about `4.4GiB`, so disk is
still sufficient. The preserved wrong-prefix duplicate partial is available as
emergency cleanup headroom if free space drops unexpectedly.

Follow-up at `2026-07-04T21:17Z`: both effective Touche document atom shards
are still healthy and advancing. The self-stopping `spark-2` prefix reached
`65936/150000`, with the live process showing `--line-start 1 --line-end
150000`. The corrected `spark-1` tail reached `9904/153732`, with the live
process showing `--line-start 150001 --line-end 303732`. Both GPUs report
`96%` utilization. The runner logs show steady progress and no OOM:
`spark-2` resumed from `61328` skipped rows and is encoding new rows at roughly
`7.7-10.8 rows/s`; `spark-1` is encoding the corrected tail at roughly
`10.2 rows/s`. No merge is valid yet. The next actionable point remains the
`spark-2` prefix cutoff, after which `spark-2` should be reassigned to an
upper-tail shard if the corrected `spark-1` tail is still far from completion.

Follow-up at `2026-07-04T21:19Z`: `spark-2` prefix reached `66632/150000` and
`spark-1` corrected tail reached `11120/153732`. Both processes still show the
intended non-overlapping ranges and both GPUs remain at `96%` utilization.
`spark-1` free disk remains `18G`, still sufficient for the expected valid
tail output. Since the prefix cutoff has not been reached, the correct action
is to continue observing; launching an upper-tail shard now would create an
overlap unless `spark-2` were stopped, which would slow the current critical
prefix segment.

Local verification at `2026-07-04T21:20Z`: the merge, atom gate, native
surface runner, matrix compiler, and sync Python scripts pass `py_compile`.
The M603 fixed-alpha, M603 product-atom, and M549U Spark runner scripts pass
`bash -n`. `git diff --check` passes for the report and the relevant M603/P1
scripts. The final full-goal verification still remains pending because
Touche documents are not yet complete and broad10 full `10/10` has not been
compiled.

Follow-up at `2026-07-04T21:21Z`: the self-stopping `spark-2` prefix reached
`67184/150000`, and the corrected `spark-1` tail reached `12112/153732`.
Both processes still show valid non-overlapping line ranges and both GPUs
remain at `96%` utilization. No upper-tail shard has been launched yet because
the prefix cutoff is not reached; stopping `spark-2` before `150000` would
leave the required prefix incomplete and slow the critical path. The next
action remains unchanged: wait for `spark-2` to finish `1-150000`, then choose
an upper-tail split point from the live `spark-1` tail coverage.

Follow-up at `2026-07-04T21:22Z`: the self-stopping `spark-2` prefix reached
`67880/150000`, and the corrected `spark-1` tail reached `12960/153732`.
The `spark-2` process still shows `--line-start 1 --line-end 150000`; the
`spark-1` process still shows `--line-start 150001 --line-end 303732`.
`spark-1` free disk is still `18G`. No merge or upper-tail split is valid yet.

Follow-up at `2026-07-04T21:23Z`: the self-stopping `spark-2` prefix reached
`68512/150000`, and the corrected `spark-1` tail reached `13576/153732`.
Both shards still have valid non-overlapping line ranges and active GPU load.
The prefix cutoff remains the next decision point; no merge or split is valid
until `spark-2` finishes the required `1-150000` range.

Follow-up at `2026-07-04T21:24Z`: the self-stopping `spark-2` prefix reached
`69192/150000`, and the corrected `spark-1` tail reached `14272/153732`.
Both shards still show the intended line ranges and active GPU load. Local
post-merge preflight confirms that the Touche source documents (`303732`
rows), source queries (`49` rows), query atoms (`49` rows), dense rankings
(`49` rows), and baseline JSON all exist. The only missing local input is the
final merged `documents.p1_atoms.jsonl`, which depends on remote shard
completion. The Touche native-eval command accepts the planned parameters under
`--plan-only`; it returns zero evaluated queries because the document atoms are
not present yet, which is expected.

Follow-up at `2026-07-04T21:26Z`: the self-stopping `spark-2` prefix reached
`70224/150000`, and the corrected `spark-1` tail reached `15280/153732`.
Both jobs still show valid non-overlapping ranges and active GPU load. The
`spark-2` prefix is encoding at roughly `7.9 rows/s` after resume; at that
rate, the `150000` cutoff is still roughly `2.8h` away. The tail remains much
larger, so the planned acceleration point is unchanged: once `spark-2` finishes
the prefix, launch a non-overlapping upper-tail shard based on the then-current
`spark-1` tail coverage.

Follow-up at `2026-07-04T21:27Z`: the self-stopping `spark-2` prefix reached
`70920/150000`, and the corrected `spark-1` tail reached `15952/153732`.
Both effective jobs are still running with valid ranges. `spark-1` free disk
is still `18G`; the preserved wrong-prefix partial remains the emergency
cleanup target if that headroom drops unexpectedly. No merge or split is valid
before the prefix cutoff.

Follow-up at `2026-07-04T21:28Z`: the self-stopping `spark-2` prefix reached
`71600/150000`, and the corrected `spark-1` tail reached `16648/153732`.
Both jobs still have valid ranges and active GPU load. Based on the recent log
rates, the prefix cutoff is roughly `2.7h` away. At that point, `spark-1` is
expected to have covered most of the tail, and `spark-2` can be reassigned to
the remaining upper-tail range without overlap.

Follow-up at `2026-07-04T21:29Z`: the self-stopping `spark-2` prefix reached
`72342/150000`, and the corrected `spark-1` tail reached `17312/153732`.
Both effective jobs still have valid ranges and active GPU load. The prefix
job has not reached the `150000` cutoff, so no new shard is safe to launch yet.

Resource adjustment at `2026-07-04T21:35Z`: a local MPS high-tail shard was
started for `Touche2020Retrieval.v3` documents range `280001-303732`, output
`documents.p1_atoms.local_upper_tail_280001_303732.jsonl`. This range is
deliberately above the current `spark-1` tail coverage and cannot overlap the
`spark-2` prefix. The first local start failed before writing output because
the stale `.bench-venv` Python binary referenced a removed Homebrew Python
path; the second failed before writing output because `HF_HOME` inherited the
remote `/home/huoju/...` path. The job was then restarted with
`/Volumes/Betty/Tmp/m150-embed-venv`, local `HF_HOME`, `DEVICE=mps`, and the
local broad10 compiler checkpoint. It successfully loaded the model and
compiler and wrote its first `264` rows. This shard remains a candidate only:
it must finish and pass the same strict merge/gate checks before it can be
included. If it completes, the merge plan becomes:

```text
1-150000       spark-2 prefix
150001-B       spark-1 corrected tail, cut before any later shard
B+1-280000     optional spark-2 middle/upper-tail after prefix cutoff
280001-303732  local MPS high-tail, only if complete and schema-valid
```

The exact `B` will be chosen from live `spark-1` row coverage at prefix
cutoff. No invalid local or stale duplicate partial is allowed into the merge.

Follow-up at `2026-07-04T21:41Z`: the native DB/plugin path was rechecked with
the existing integration smoke tests. `tests/test_publish_p1_atoms_to_ii42_pg.py`
passed (`3 passed`), which exercises the intended path end to end: temporary
PostgreSQL database creation, `CREATE EXTENSION ii42`, P1 atom table load,
`ii42_model_generation_upsert_from_table`, and `ii42_model_query_atoms`. The
helper-level shard/merge/gate matrix tests also passed (`15 passed`). This
confirms the current M603 implementation is still using the existing
model-backed index lifecycle rather than a replacement publisher/evaluator.

Shard status at `2026-07-04T21:41Z`:

```text
spark-2 prefix 1-150000:        79408 / 150000 rows, tmux alive, GPU active
spark-1 tail 150001-303732:     23856 / 153732 rows, tmux alive, GPU active
local MPS high tail 280001-303732:
  first run exited at 4664 / 23732 rows, incomplete and not mergeable
  retry resumed the same output with BATCH_SIZE=4 and reached 4760 rows
```

The local high-tail retry is retained only as an opportunistic accelerator. It
is not part of the merge plan until it reaches exactly `23732` rows and passes
the same JSONL schema/range gate. If it exits early again, it will be ignored
and the Spark-only merge plan remains authoritative.

Follow-up at `2026-07-04T21:53Z`: the local MPS high-tail retry progressed to
`13148/23732` rows, then exited with an MPS out-of-memory error. The partial is
still explicitly non-mergeable. A final conservative retry was started with
`BATCH_SIZE=1` and the same `280001-303732` range/output so it can resume from
the partial. This is the last local MPS retry for this shard: if it exits early
again, the local high-tail file will be excluded from all merge plans and the
Spark-only route remains the source of truth. At this checkpoint the Spark
jobs were still healthy:

```text
spark-2 prefix 1-150000:        87600 / 150000 rows
spark-1 tail 150001-303732:     31888 / 153732 rows
```

Follow-up at `2026-07-04T22:06Z`: the local `280001-303732` high-tail shard
completed exactly `23732/23732` rows and passed a non-contiguous merge dry-run
validation. It is now a valid merge candidate, not yet a published atom file.
To shorten the remaining `spark-1` tail dependency, a new non-overlapping local
MPS shard was started for `220001-280000`:

```text
output: runs/m603_p1_product_atoms_broad10_v1/Touche2020Retrieval.v3/
        documents.p1_atoms.local_mid_tail_220001_280000.jsonl
range:  220001-280000
mode:   BATCH_SIZE=1, RESUME=1, DEVICE=mps, COMPILER_DEVICE=mps
log:    runs/m603_p1_product_atoms_broad10_v1/logs/
        Touche2020Retrieval.v3_local_mid_tail_220001_280000.log
```

If this mid-tail shard completes and validates, the intended merge coverage is:

```text
1-150000       spark-2 prefix
150001-220000  spark-1 corrected tail, cut at 220000
220001-280000  local MPS mid-tail, only if complete and schema-valid
280001-303732  local MPS high-tail, complete and dry-run validated
```

If the mid-tail local shard fails, it will be ignored and the previous Spark
tail cutoff plan remains valid.

Follow-up at `2026-07-04T22:10Z`: current live progress is:

```text
spark-2 prefix 1-150000:        98304 / 150000 rows, tmux alive, GPU active
spark-1 tail 150001-303732:     42336 / 153732 rows, tmux alive, GPU active
local MPS mid-tail 220001-280000:
  1646 / 60000 rows, tmux alive, progress log active
```

The high-tail shard remains complete and validated. The mid-tail shard is the
likely next bottleneck if left only on local MPS. The next resource decision is
therefore to let `spark-2` finish its mandatory `1-150000` prefix first, then
reassign `spark-2` to an unused non-overlapping mid-tail subrange if the local
`220001-280000` shard is still far from complete. No merge is valid until all
chosen ranges are complete and pass the shard validator.

Follow-up at `2026-07-04T22:17Z`: local MPS mid-tail exited after only
`3165/60000` rows with no completion JSON, so it remains non-mergeable. The
same file/range was restarted with `RESUME=1` as an opportunistic background
accelerator. This does not change the authoritative merge rule: the file is
ignored unless it eventually reaches exactly `60000` rows and passes dry-run
validation. Current Spark progress at this decision point:

```text
spark-2 prefix 1-150000:        102056 / 150000 rows
spark-1 tail 150001-303732:     46062 / 153732 rows
```

Follow-up at `2026-07-04T22:29Z`: the local MPS mid-tail accumulated to
`6098/60000` rows, exited again, and was restarted with `RESUME=1`. The merge
policy was tightened: the partial must not be treated as a completed
`220001-280000` shard, but it may be used later as an exact prefix shard
`220001-(220000 + actual_rows)` if and only if the final actual row count is
declared in the merge command and the shard validator passes. If that prefix
overlaps with the final `spark-1` tail coverage, the merge will choose the
non-overlapping source with the cleanest coverage and ignore the duplicate
range. Current Spark progress:

```text
spark-2 prefix 1-150000:        109504 / 150000 rows
spark-1 tail 150001-303732:     53360 / 153732 rows
```

Follow-up at `2026-07-04T22:40Z`: the local MPS mid-tail accumulated to
`9053/60000` rows and exited again. It is now retained only as an auditable
partial prefix candidate (`220001-229053`) if a later merge needs it and the
range validator accepts the exact row count. It will not be restarted further:
at the observed rates, `spark-1` should overtake this local prefix before
`spark-2` finishes the mandatory prefix. The critical path is therefore back to
Spark-managed coverage:

```text
spark-2 prefix 1-150000:        115920 / 150000 rows
spark-1 tail 150001-303732:     60464 / 153732 rows
```

Next decision point: when `spark-2` finishes `1-150000`, choose the remaining
non-overlapping range from the live `spark-1` tail coverage up to `280000`.
The already validated local high-tail covers `280001-303732`.

Follow-up at `2026-07-04T22:55Z`: `spark-1` reached `70432` tail rows, so it
now covers at least `150001-220000`. The local mid-tail partial is no longer
needed for the first mid-tail cutoff, though it remains a possible exact-prefix
candidate if useful. Current state:

```text
spark-2 prefix 1-150000:        123360 / 150000 rows
spark-1 tail 150001-303732:     70432 / 153732 rows
local mid prefix 220001-229053: 9053 rows, stopped
local high-tail 280001-303732:  23732 / 23732 rows, validated
```

When `spark-2` completes the prefix, keep `spark-1` running and start a
`spark-2` gap shard from the then-current `spark-1` coverage plus one through
`280000`. The final merge will choose whichever non-overlapping set reaches
full coverage first.

Follow-up at `2026-07-04T23:33Z`: `spark-2` completed the mandatory
`1-150000` prefix (`150000/150000`) and self-stopped. At that moment
`spark-1` had reached `106472` tail rows, covering source rows through
`256472`. A non-overlapping `spark-2` gap shard was started for
`256473-280000`:

```text
output: /home/huoju/leask/runs/ii42-m603-p1-product-atoms-broad10-spark2-gap-v1/
        Touche2020Retrieval.v3/documents.p1_atoms.gap_256473_280000.jsonl
range:  256473-280000
expect: 23528 rows
```

One minute after launch, the gap shard had written `424/23528` rows and was
using the GPU. `spark-1` continued running and had reached `107848` rows. If
`spark-1` reaches the full `150001-280000` coverage first, its output must be
trimmed to exactly the first `130000` rows before merge validation; an
overlong tail file must not be passed to the shard merger as-is.
