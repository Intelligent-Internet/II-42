# ii42 M361 Literature Signal Audit

## Scope

This is a no-training audit over existing local artifacts. It does not use GPU and does not touch `spark-1` or `spark-2` jobs.

The goal is to translate each paper-derived signal into a concrete experiment state: already tested, newly audited locally, or requiring a follow-up canary.

## Paper Signal Matrix

| Signal | Status | Current finding | Next experiment |
| --- | --- | --- | --- |
| `behavioral_eval_over_reconstruction` | `tested_existing_artifacts` | atom-only mean NDCG/dense=0.298; scorer mean NDCG/dense=0.812 | Require every new SAE checkpoint to report dense-topk overlap and sparse-tax, not reconstruction loss alone. |
| `dense_retrieval_sae_discretization` | `current_checkpoint_verified` | M361A current M320 checkpoint: sparse-cosine Top10 overlap is 0.098 on nfcorpus and 0.138 on scifact. | Stop treating downstream scorer rescue as the main route; rerun representation-first dense-neighborhood training. |
| `local_distance_preservation` | `cpu_canary_done` | M361 canary: recon_only Top10 overlap=0.4409 and NDCG tax=-0.5113; knn_kl improves overlap to 0.4909. | Repeat on current M320/M344 export and full current query surface. |
| `distillation_hard_negatives` | `cpu_canary_done` | knn_kl_pair has best NDCG tax=-0.4258 and recall tax=-0.3727, but Top10 overlap remains below 0.50. | Treat KL/pairwise as useful but incomplete; add exact dense-overlap gates before broader training. |
| `query_distribution_mismatch` | `new_cpu_audit` | broad dense entropy mean=0.487; BEIR dense entropy mean=0.609 | Use real train/dev queries for teacher-neighborhood replay; do not continue document-sentence pseudo-query generation. |
| `k_sparse_projector` | `tested_existing_artifacts` | M90 closed at k1024, M93 passed at k768, and M96 selected k512 on the old Stage-A candidate surface; checkpoints are not currently available for direct rerun. | Rebuild support-budget sweeps only inside the new dense-neighborhood route; old support compression does not fix current atom-dense overlap failure. |
| `cosine_norm_contamination` | `new_cpu_audit` | query norm mean=1.000; doc norm mean=1.000; prior raw-dot scoring was worse than normalized sparse scoring. | Keep cosine/normalized scoring as the primary gate; test norm-penalized TopK allocation only as a canary. |

## M80 Candidate-Row Diagnostics

| Family | Rows | Pos dense<=10 | Pos BM25<=10 | Dense/BM25 Jaccard | Dense entropy | Pos qrel-only | Pos dense-only |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `beir15_current_eval_surface` | 240 | 0.967 | 0.921 | 0.243 | 0.609 | 0.388 | 0.100 |
| `broad_generated_query_surface` | 200 | 0.950 | 0.930 | 0.202 | 0.487 | 0.385 | 0.045 |

## Query Embedding Norms

| Family | Rows | Mean norm | Median norm | P90 norm |
| --- | ---: | ---: | ---: | ---: |
| `beir15_current_eval_surface` | 240 | 1.0000 | 1.0000 | 1.0000 |
| `broad_generated_query_surface` | 200 | 1.0000 | 1.0000 | 1.0000 |

## Document Embedding Norms

| Family | Rows | Mean norm | Median norm | P90 norm |
| --- | ---: | ---: | ---: | ---: |
| `beir15_current_eval_surface` | 21660 | 1.0000 | 1.0000 | 1.0000 |
| `broad_generated_query_surface` | 5491 | 1.0000 | 1.0000 | 1.0000 |
| `commons_proxy_representation` | 800 | 1.0000 | 1.0000 | 1.0000 |

## M360 Dense Faithfulness Summary

- atom rows: `4`
- scorer rows: `4`
- atom NDCG/dense mean: `0.298`
- scorer NDCG/dense mean: `0.812`

## M361A Atom-vs-Dense Current Checkpoint Summary

Report: `docs/research-sae/reports/m0300-m0399/ii42-m361-atom-dense-overlap-report.md`

| Dataset | Score | Overlap@10 | Overlap@20 | Overlap@100 | NDCG@10 vs dense100 | MRR vs dense100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `nfcorpus` | `sparse_cosine` | 0.0984 | 0.1281 | 0.2495 | 0.3939 | 0.6438 |
| `nfcorpus` | `sparse_dot` | 0.0844 | 0.1063 | 0.2280 | 0.3370 | 0.5138 |
| `scifact` | `sparse_cosine` | 0.1375 | 0.1617 | 0.2603 | 0.4900 | 0.7218 |
| `scifact` | `sparse_dot` | 0.0625 | 0.0680 | 0.1789 | 0.2511 | 0.4180 |

Interpretation: the current M320 atom representation does not preserve exact dense top-rank neighborhoods. This failure is visible before BM25, IDF, scorer, or final admission/ranking logic.

## M361 Dense-kNN Canary Summary

Report: `docs/research-sae/reports/m0300-m0399/ii42-m361-dense-knn-canary-report.md`

| Variant | Top10 overlap | Top20 overlap | Sparse NDCG@10 | Dense NDCG@10 | NDCG tax | MRR tax | Recall tax |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `recon_only` | 0.4409 | 0.6841 | 0.4887 | 1.0000 | -0.5113 | -0.3950 | -0.4455 |
| `knn_kl` | 0.4909 | 0.6386 | 0.5457 | 1.0000 | -0.4543 | -0.2776 | -0.4091 |
| `knn_kl_pair` | 0.4682 | 0.6295 | 0.5742 | 1.0000 | -0.4258 | -0.2598 | -0.3727 |
| `knn_kl_ce` | 0.4455 | 0.6068 | 0.5395 | 1.0000 | -0.4605 | -0.2683 | -0.4455 |

Interpretation: reconstruction-only sparse projection is objective-misaligned for dense-neighborhood preservation. Dense-kNN KL and pairwise supervision help, but the remaining Top10 overlap and sparse-tax gap are still too large.

## Support-Budget Evidence

Existing M90/M93/M96 reports show that support compression was tested on the old Stage-A candidate surface: M90 closed at `k1024`, M93 passed at `k768`, and M96 selected `k512` for the next engineering step. The corresponding checkpoints were not found on the current local, spark-1, or spark-2 paths, so M361D cannot be directly rerun from old artifacts. More importantly, old support compression does not solve the current M320 atom-vs-dense overlap failure; support sweeps should be rebuilt inside the dense-neighborhood objective.

## M362 Loss Redesign Follow-Up

Report: `docs/research-sae/reports/m0300-m0399/ii42-m362-dense-loss-redesign-report.md`

M362 changes the interpretation: reconstruction loss is not inherently the wrong foundation. On dense embeddings, hard-TopK reconstruction reaches exact dense Top10 overlap of 0.6609 at k64 and 0.6913 at k128, much higher than the current M320 token/posting atom route. The likely failure is that later posting/ranking/admission losses were used as representation objectives before dense-neighborhood preservation was gated.

Next route: rebuild from a dense-embedding sparse autoencoder first, gate it by exact dense top-k overlap, then distill text/token atoms into that sparse code. Do not make BM25/qrel/admission the first representation-training target.

## Example Rows To Inspect

| Reason | Row | Source | Family | Dense rank | BM25 rank | Bucket |
| --- | --- | --- | --- | ---: | ---: | --- |
| `positive_dense_rank_gt_10` | `m80-cand:beir15:fiqa:5054` | `beir15:fiqa` | `beir15_current_eval_surface` | 13 | 25 | `dense_only` |
| `positive_not_dense_teacher` | `m80-cand:beir15:msmarco:1063750` | `beir15:msmarco` | `beir15_current_eval_surface` | 1 | 1 | `qrel_only` |
| `positive_not_dense_teacher` | `m80-cand:beir15:msmarco:1106007` | `beir15:msmarco` | `beir15_current_eval_surface` | 1 | 1 | `qrel_only` |
| `positive_not_dense_teacher` | `m80-cand:beir15:msmarco:1112341` | `beir15:msmarco` | `beir15_current_eval_surface` | 1 | 1 | `qrel_only` |
| `positive_not_dense_teacher` | `m80-cand:beir15:msmarco:1113437` | `beir15:msmarco` | `beir15_current_eval_surface` | 1 | 1 | `qrel_only` |

## Immediate Decisions

- Do not spend the next main effort on another M35-style document-sentence pseudo-query sweep.
- Do not treat scorer-rescued qrel quality as proof that the SAE representation is dense-faithful.
- The next GPU work should be a dense-embedding sparse autoencoder rerun with hard TopK and exact dense-overlap gates, not another downstream scorer rescue.
- Keep M35x scorer/posthoc work as downstream diagnostics, not the main representation line.

## Follow-Up Experiment Queue

1. `M361A`: completed reduced current-checkpoint export; full-corpus rerun is optional because the reduced overlap already confirms the failure mode.
2. `M361B`: completed first no-checkpoint CPU canary; repeat on current M320/M344 export if the next rerun needs a current-surface training target.
3. `M361C`: completed first objective ablation (`recon_only`, KL, KL+pairwise, KL+CE); next add current-surface gates and support-budget sweep.
4. `M361D`: old M90/M93/M96 support evidence exists, but checkpoint artifacts are unavailable; rebuild support compression only after the dense-neighborhood objective starts closing overlap.
