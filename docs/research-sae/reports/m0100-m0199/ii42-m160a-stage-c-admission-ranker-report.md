# ii42 M160A Stage-C Admission Ranker Report

## Status

This report records the next exploration after the M160A/C6 admission and
fusion diagnostic. The goal was narrow: test whether a runtime-safe learned
admission/ranking head can recover the BM25+SAE oracle headroom without
changing the encoder.

The answer is **no, not with this feature class**. The probe is useful because
it closes the cheap Stage-C-only path and points the next work back to a
full-corpus Stage-B/C admission training surface.

## Artifacts

- Script: `scripts/research_sae_m160_admission_ranker.py`
- Spark runner: `scripts/run_m160_admission_ranker_spark.sh`
- Remote full-row run:
  `/home/huoju/leask/runs/ii42-m160a-c6-admission-ranker-fullrows-v1`
- Local copy:
  `results/ii42-m160a-c6-admission-ranker-fullrows-v1`
- Canary run:
  `/home/huoju/leask/runs/ii42-m160a-c6-admission-ranker-canary-v1`

The ranker uses only runtime-safe BM25/SAE/fusion features:

- per-source normalized score
- reciprocal/log rank features
- source membership flags
- simple score interactions and source overlap statistics

Dense and BM25+dense are controls only. Dense scores are not used as runtime
features.

## Evaluation Surface

The run uses the currently completed official BEIR `all-test` artifacts from:

`/home/huoju/leask/runs/ii42-m160a-c6-fixed-policy-official-beir-v1`

Completed datasets:

- `arguana`
- `cqadupstack`
- `fiqa`
- `nfcorpus`
- `quora`
- `scidocs`
- `scifact`
- `trec-covid`
- `webis-touche2020`

Still missing from the official gate because the monolithic evaluator is too
memory-heavy for large corpora:

- `nq`
- `dbpedia-entity`
- `hotpotqa`
- `fever`
- `climate-fever`
- `msmarco`

## Full-Row LODO Metrics

The full-row run used all `4,749,578` feature rows from the completed
9-dataset surface, with leave-one-dataset-out training.

| Source | R@20 | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `bm25` | 0.4518 | 0.5625 | 0.4894 | 0.4195 | 0.2849 |
| `sae` | 0.5220 | 0.6436 | 0.5483 | 0.4791 | 0.3382 |
| `bm25_sae_score_fusion` | 0.5393 | 0.6574 | 0.5764 | 0.5055 | 0.3573 |
| `m160_admission_direct` | 0.5367 | 0.6588 | 0.5721 | 0.5025 | 0.3559 |
| `m160_admission_residual` | 0.5389 | 0.6574 | 0.5766 | 0.5063 | 0.3572 |
| `m160_admission_two_band` | 0.5368 | 0.6429 | 0.5765 | 0.5063 | 0.3558 |
| `dense` | 0.5520 | 0.6726 | 0.5999 | 0.5357 | 0.3748 |
| `bm25_dense_score_fusion` | 0.5562 | 0.6734 | 0.6010 | 0.5342 | 0.3716 |
| `oracle_pool` | 0.6761 | 0.6912 | 0.9410 | 0.8690 | 0.6912 |

Best learned policy: `m160_admission_residual`.

Compared with current BM25+SAE:

- NDCG@10: `+0.0008`
- Recall@100: `+0.0000`
- MAP@100: `-0.0002`

Compared with BM25+dense:

- NDCG@10: `-0.0279`
- Recall@100: `-0.0160`
- MAP@100: `-0.0145`

## Interpretation

The oracle candidate pool has meaningful headroom:

- Current BM25+SAE R@100: `0.6574`
- Oracle BM25/SAE pool R@100: `0.6912`
- BM25+dense R@100: `0.6734`

So the candidate surface is not hopeless on the completed nine datasets.
However, a shallow runtime-safe admission head cannot reliably select the
right candidates. It mostly preserves the existing BM25+SAE order and gives
only tiny ranking gains.

This means the blocker is not a missing scalar weight or a small post-hoc gate.
The model needs training examples that directly teach:

1. which BM25-only qrel positives should be admitted,
2. which high-BM25 non-qrel documents should be suppressed,
3. which SAE positives should be preserved when BM25 is noisy,
4. which dense-only positives are outside both BM25 and SAE and therefore
   require representation work rather than fusion work.

## Decision

Do not promote the Stage-C-only admission ranker.

Do not continue scalar fusion or generic adaptive gate sweeps.

The next useful route is a full-corpus Stage-B/C training reset using the
same failure modes as explicit supervised rows. The training surface must be
built from correct M160/PPLX dimensions and should include large corpora via
streaming/sharded hard-negative generation rather than relying on the
monolithic evaluator.

## Next Steps

1. Implement or finish the sharded large-corpus evaluator/row builder for
   `nq`, `dbpedia-entity`, `hotpotqa`, `fever`, `climate-fever`, and
   `msmarco`.
2. Build a strict Stage-B/C admission surface:
   - BM25-only qrel positives as positive admission examples.
   - SAE qrel positives lost by fusion as preservation examples.
   - high-BM25 non-qrel candidates as negative admission examples.
   - dense-only positives as representation-gap examples.
3. Train from the M160A Stage-A/B healthy checkpoint, but optimize final
   BM25+SAE top100 admission and top20 ranking directly.
4. Re-run the official BEIR full-corpus matrix only after the streaming
   large-corpus path is stable.

## Follow-Up Fix

`scripts/run_m160_stageb_streaming_large_spark.sh` now defaults to
`scripts/research_sae_build_streaming_bm25_dense_qrels_rows.py` and requires
BM25 scores. The previous dense-only default produced valid dense controls but
was not a valid row surface for the current BM25+SAE admission target.

The same runner now defaults to Spark's writable working directory
`/home/huoju/leask/psql_bm25s_sae`. The root-owned `/home/huoju/leask/ii42_sae`
directory exists on Spark but is not a usable checkout/workdir.

## Query-Side-Only Continuation Check

Run:

`/home/huoju/leask/runs/ii42-m160a-c6-b14-query-only-v1`

ClearML:

`http://100.116.110.26:8080/projects/c86222099ca64e3392294d2aead22dd8/experiments/f979183e206d48ae9eca9d252b156a22/output/log`

This run tested a narrow hypothesis: keep the document SAE/index geometry
fixed, split query and document SAE modules, freeze the document SAE, and only
train the query side plus existing scales from the B12 checkpoint. The goal was
to see whether top-rank/fusion errors were mostly query calibration errors.

The run used the B12 checkpoint and B12 candidate rows:

- checkpoint:
  `/home/huoju/leask/runs/ii42-m160a-c6-b12-rankreg-admission-v1/bm25sae_stageb_best.pt`
- train rows:
  `/home/huoju/leask/runs/ii42-m160a-c6-b12-rankreg-admission-v1-candidate-rows`
- eval rows:
  `/home/huoju/leask/runs/ii42-m160a-c6-b12-rankreg-admission-v1-eval-candidate-rows`

Preflight passed with PPLX `1024`-dim embeddings and `16384` SAE features.
Docker runtime flags confirmed:

- `SEPARATE_QUERY_DOC_SAE=1`
- `FREEZE_DOC_SAE=1`
- `LR=1e-5`
- `STEPS=2400`

Validation trend:

| Step | Selection | BM25+SAE hit@20 | BM25+SAE MRR@20 | SAE hit@20 | SAE MRR@20 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 1.486722 | 0.690476 | 0.398123 | 0.692857 | 0.392964 |
| 250 | 1.481440 | 0.685714 | 0.397864 | 0.697619 | 0.393794 |
| 500 | 1.475339 | 0.685714 | 0.394815 | 0.690476 | 0.393136 |
| 750 | 1.472390 | 0.685714 | 0.393344 | 0.692857 | 0.392083 |

Decision: stop this path. Query-side-only continuation degraded the selection
metric and BM25+SAE top-rank metrics immediately. The best checkpoint is the
initial step, so the bottleneck is not a simple query-side calibration problem
on the existing B12 row surface.

This does not invalidate the separate-query/document SAE implementation as a
diagnostic tool, but it should not be promoted as the next training direction.
The next meaningful route still requires rebuilding the row surface around
full-corpus score-low cases, BM25-only positives, SAE positives lost by fusion,
and high-BM25 false positives rather than continuing on the same B12 rows.

## Counterfactual Runtime-Safe Ranker Probe

Runs:

- `results/ii42-m160-b12-counterfactual-ranker-v1`
- `results/ii42-m160-b12-counterfactual-ranker-lowres-v1`

This probe tested whether a stronger diagnostic scorer can recover the
remaining admission/top-rank gap from existing full-corpus B12 rankings using
only runtime-safe features:

- BM25 normalized score/rank/membership.
- SAE normalized score/rank/membership.
- Existing BM25+SAE fusion score/rank/membership.
- Query-level BM25/SAE concentration and overlap signals.

Dense and BM25+dense were controls only. They were not used as runtime
features. The probe used leave-one-dataset-out validation over the flat B12
full-corpus ranking artifact:

`/Volumes/Betty/Tmp/ii42_m160_b12_full_eval/m110_full_corpus_rankings.jsonl`

Parsed surface:

- queries: `882`
- datasets: `arguana`, `fiqa`, `msmarco`, `nfcorpus`, `scifact`,
  `trec-covid`
- candidate rows: `154,777`

Counterfactual labels in the runtime candidate pool:

| Counter | Count |
| --- | ---: |
| `relevant_in_runtime_pool` | 4,332 |
| `candidate_hit_score_low` | 2,083 |
| `bm25_only_positive` | 967 |
| `sae_positive_lost_by_fusion` | 344 |
| `bm25_only_negative` | 65,610 |

Primary probe:

| Source | R@20 | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `sae` | 0.4952 | 0.5810 | 0.4676 | 0.4178 | 0.3250 |
| `bm25_sae_score_fusion` | 0.4943 | 0.5938 | 0.4715 | 0.4197 | 0.3232 |
| `counterfactual_residual` | 0.4815 | 0.5938 | 0.4733 | 0.4135 | 0.3155 |
| `dense` | 0.4878 | 0.5804 | 0.4663 | 0.4237 | 0.3158 |
| `bm25_dense_score_fusion` | 0.4872 | 0.5883 | 0.4601 | 0.4220 | 0.3134 |
| `oracle_pool` | 0.5936 | 0.6245 | 0.8415 | 0.7832 | 0.6245 |

Low-residual sanity check:

| Source | R@20 | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `bm25_sae_score_fusion` | 0.4943 | 0.5938 | 0.4715 | 0.4197 | 0.3232 |
| `counterfactual_two_band` | 0.4957 | 0.5935 | 0.4735 | 0.4164 | 0.3179 |
| `oracle_pool` | 0.5936 | 0.6245 | 0.8415 | 0.7832 | 0.6245 |

Decision: stop this route as a direct scorer. The learned counterfactual
scorer can slightly improve MRR, but it consistently loses NDCG and MAP against
the existing BM25+SAE fusion. Since the oracle pool is much stronger, the
candidate surface still has headroom, but the current runtime-safe scalar
features are not enough to select the right documents reliably.

The next useful step, if we continue model work, is not another scorer over the
same B12 ranking rows. It must add richer supervision or representation:

1. Rebuild rows with deeper full-corpus evidence around score-low qrel
   positives and high-BM25 false positives.
2. Add query/document semantic features that are still deployable, not dense
   teacher features at runtime.
3. Distill the corrected objective into the atom/query encoder rather than
   trying to patch the final ranking with a shallow admission head.
