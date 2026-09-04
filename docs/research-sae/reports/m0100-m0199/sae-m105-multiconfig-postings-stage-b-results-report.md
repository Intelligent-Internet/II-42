# SAE M105 Multiconfig Postings Stage-B Results Report

Date: 2026-05-22

## Decision

M105 promotes the postings-trained Stage-B route from a single compact
candidate configuration to a broader multi-config setting. A single residual
ranker trained across four postings-generated configs beats fixed `BM25+SAE w2`
on average holdout and eval metrics.

This is still not SQL/API productization approval. It is enough evidence to
start a narrower SQL-facing runtime-contract experiment around the compact
recommended point, while keeping the Python postings simulator as the source of
truth for quality/cost regressions.

## Inputs

- Corpus root:
  `/home/huoju/leask/runs/m97-stage-a-validation-v2/m21_payload_root/m97_stage_a_eval`
- Latent run: `m96_k512`
- Candidate configs:
  `d8_p16_bm25100`, `d8_p32_bm25100`, `d16_p16_bm25100`, `d16_p32_bm25100`
- Initial scorer: fixed `BM25+SAE w2`
- Spark output:
  `/home/huoju/leask/runs/m105-multiconfig-postings-stage-b-v0`
- Local artifact mirror:
  `results/sae/m105-multiconfig-postings-stage-b`

## Data

| Field | Value |
| --- | ---: |
| Documents | `30059` |
| Queries | `886` |
| Runtime rows | `3544` |
| Train rows | `2468` |
| Validation rows | `532` |
| Holdout rows | `544` |
| BM25 terms | `76361` |
| BM25 postings | `2800748` |
| SAE dims | `7184` |
| SAE postings | `15390208` |

## Average Metrics Across Configs

| Split | Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: |
| `holdout` | `fixed_bm25_sae` | 0.4277 | 0.5342 | 0.4314 | 0.2905 |
| `holdout` | `m105_multiconfig_ranker` | 0.4294 | 0.5446 | 0.4358 | 0.2974 |
| `eval` | `fixed_bm25_sae` | 0.4425 | 0.5226 | 0.4297 | 0.2956 |
| `eval` | `m105_multiconfig_ranker` | 0.4447 | 0.5327 | 0.4340 | 0.3003 |
| `all` | `fixed_bm25_sae` | 0.4315 | 0.5146 | 0.4088 | 0.2860 |
| `all` | `m105_multiconfig_ranker` | 0.4331 | 0.5203 | 0.4115 | 0.2884 |

## Config Matrix

| Config | Split | Run | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Mean Candidates | SAE Gen Postings |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `d8_p16_bm25100` | `eval` | `fixed_bm25_sae` | 0.4355 | 0.5217 | 0.4289 | 0.2940 | 204.3 | 128.0 |
| `d8_p16_bm25100` | `eval` | `m105_multiconfig_ranker` | 0.4367 | 0.5317 | 0.4334 | 0.2986 | 204.3 | 128.0 |
| `d8_p32_bm25100` | `eval` | `fixed_bm25_sae` | 0.4455 | 0.5216 | 0.4295 | 0.2961 | 323.7 | 255.9 |
| `d8_p32_bm25100` | `eval` | `m105_multiconfig_ranker` | 0.4489 | 0.5315 | 0.4335 | 0.3009 | 323.7 | 255.9 |
| `d16_p16_bm25100` | `eval` | `fixed_bm25_sae` | 0.4380 | 0.5238 | 0.4298 | 0.2951 | 325.4 | 256.0 |
| `d16_p16_bm25100` | `eval` | `m105_multiconfig_ranker` | 0.4390 | 0.5341 | 0.4342 | 0.2996 | 325.4 | 256.0 |
| `d16_p32_bm25100` | `eval` | `fixed_bm25_sae` | 0.4509 | 0.5236 | 0.4305 | 0.2974 | 562.6 | 511.9 |
| `d16_p32_bm25100` | `eval` | `m105_multiconfig_ranker` | 0.4540 | 0.5335 | 0.4351 | 0.3023 | 562.6 | 511.9 |

## Training Notes

- Best epoch: `100`
- Best validation score: `0.871318`
- Elapsed: `160.45s` on Spark CUDA
- The model is still initialized from fixed `BM25+SAE w2`.
- Query split assignment is query-stable across configs, so the same query
  cannot be train in one config and holdout in another config.
- Feature normalization is rebuilt from the combined postings-generated train
  rows, not reused from the old M98/M101 candidate surface.

## Interpretation

M105 confirms that the M104 improvement was not only a single-config artifact.
The absolute gain is still modest, but the direction is consistent:

- Holdout average NDCG delta: `+0.0043`.
- Eval average NDCG delta: `+0.0044`.
- Eval average MAP delta: `+0.0047`.
- Eval average MRR delta: `+0.0100`.

The recommended config remains `d8_p16_bm25100`, because it keeps the compact
candidate budget: mean `204.3` candidates and `128.0` SAE generation postings.
Larger configs improve recall, but they are not clearly better enough to justify
2x to 4x generation/rerank cost as the first SQL-facing contract.

## Next Step

M106 should narrow the runtime contract around the compact recommended config:

1. Treat `d8_p16_bm25100` as the default SQL-facing experiment point.
2. Re-run the M103 postings matrix with the M105 export as the scorer.
3. Validate Python scorer parity against the exported JSON on all four configs.
4. Start a read-only SQL/runtime interface only after the compact config remains
   non-regressing under export/parity checks.
