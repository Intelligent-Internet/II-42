# SAE M104 Postings Stage-B Ranker Report

Date: 2026-05-22

## Decision

M104 retrains the Stage-B residual ranker on M103 postings-generated
candidate rows instead of the old M98/M81 candidate surface. This directly
addresses the M103 distribution-shift blocker.

The result is positive. M104 beats fixed `BM25+SAE w2` on validation,
holdout, eval, and all-row aggregates. The improvement is modest, but it is
consistent enough to promote the route: Stage-B ranking should be trained on
the same postings-generated distribution that the runtime path will use.

M104 does not yet authorize SQL/API productization. The next step should be a
broader postings-trained ranker over more candidate configurations and a
cost-aware selection gate.

## Inputs

- Corpus root:
  `/home/huoju/leask/runs/m97-stage-a-validation-v2/m21_payload_root/m97_stage_a_eval`
- Latent run: `m96_k512`
- Candidate config: `d8_p16_bm25100`
- Initial scorer: fixed `BM25+SAE w2`
- Spark output:
  `/home/huoju/leask/runs/m104-postings-stage-b-ranker-v0`
- Local artifact mirror:
  `results/sae/m104-postings-stage-b-ranker`

## Data

| Field | Value |
| --- | ---: |
| Documents | `30059` |
| Queries | `886` |
| Runtime rows | `886` |
| Train rows | `617` |
| Validation rows | `133` |
| Holdout rows | `136` |
| BM25 terms | `76361` |
| BM25 postings | `2800748` |
| SAE dims | `7184` |
| SAE postings | `15390208` |
| Mean candidate docs | `204.3` |
| P95 candidate docs | `227.0` |
| Mean SAE generation postings | `128.0` |
| Mean SAE rerank lookups | `104612.7` |

## Metrics

| Split | Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `validation` | `fixed_bm25_sae` | 0.4473 | 0.5094 | 0.4271 | 0.2990 |
| `validation` | `m104_postings_ranker` | 0.4486 | 0.5192 | 0.4316 | 0.3015 |
| `holdout` | `fixed_bm25_sae` | 0.4239 | 0.5338 | 0.4307 | 0.2891 |
| `holdout` | `m104_postings_ranker` | 0.4249 | 0.5439 | 0.4347 | 0.2959 |
| `eval` | `fixed_bm25_sae` | 0.4355 | 0.5217 | 0.4289 | 0.2940 |
| `eval` | `m104_postings_ranker` | 0.4366 | 0.5317 | 0.4332 | 0.2986 |
| `all` | `fixed_bm25_sae` | 0.4231 | 0.5138 | 0.4081 | 0.2840 |
| `all` | `m104_postings_ranker` | 0.4250 | 0.5189 | 0.4103 | 0.2862 |

## Deltas

| Split | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `validation` | +0.0013 | +0.0098 | +0.0045 | +0.0024 |
| `holdout` | +0.0010 | +0.0101 | +0.0040 | +0.0068 |
| `eval` | +0.0012 | +0.0099 | +0.0043 | +0.0047 |
| `all` | +0.0019 | +0.0051 | +0.0022 | +0.0021 |

## Training Notes

- Best epoch: `100`
- Best validation score: `0.878728`
- Elapsed: `66.79s` on Spark CUDA
- The model is initialized exactly from fixed `BM25+SAE w2`, then trained as a
  residual repair. This was required: a random initialization underperformed
  fixed scoring in the smoke run.
- Query feature normalization and candidate feature normalization are rebuilt
  from the postings-generated train split, not reused blindly from M101.

## Interpretation

M104 confirms the M103 diagnosis: the M101 ranker failed on full-corpus
postings candidates because it learned the old M98 candidate-surface
distribution. Retraining the same general residual-ranker shape on generated
candidates recovers a consistent advantage over fixed scoring.

The current gain is not large enough to call the ranker product-ready. It is
large enough to keep the path alive and justify M105.

## Next Step

M105 should broaden the postings-trained Stage-B route:

1. Train/evaluate over multiple candidate configs, not only `d8_p16_bm25100`.
2. Add cost-aware model selection so larger pools do not win only by opening
   more documents.
3. Keep fixed `BM25+SAE w2` as the baseline to beat on holdout/eval.
4. Re-run the M103 full-corpus matrix with the M104/M105 scorer export.

