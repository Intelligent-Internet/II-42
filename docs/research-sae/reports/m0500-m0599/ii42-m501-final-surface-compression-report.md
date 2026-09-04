# II-42 M501 Final Dense-Surface Compression Report

## Summary

M501 follows M500. M500 rejected naive PPLX layer trimming and showed that the
final `bf16` surface is stable. M501 tests the next safer question:

- keep the final dense teacher surface fixed;
- compress the output vectors directly;
- evaluate dense ranking preservation and deterministic posting geometry.

This is still not model-side pruning. It is an output-surface proxy for deciding
which compression families are worth taking into model-side QAT/pruning.

Machine and data:

- machine: `spark-1`;
- data root:
  `/home/huoju/leask/runs/ii42-m398-data/_shared/tasks`;
- available task in that root: `FiQA2018`;
- run root:
  `/home/huoju/leask/runs/ii42-m501-final-surface-compression-v1`.

## Artifacts

- Script:
  `scripts/research_sae_m501_final_surface_compression.py`
- Smoke JSON:
  `outputs/m501/fiqa_smoke/m501_final_surface_compression_fiqa.json`
- Smoke report:
  `outputs/m501/fiqa_smoke/m501_final_surface_compression_fiqa.md`
- Large-sample JSON:
  `outputs/m501/fiqa_large/m501_final_surface_compression_fiqa_large.json`
- Large-sample report:
  `outputs/m501/fiqa_large/m501_final_surface_compression_fiqa_large.md`

## Large-Sample Result

Config:

- task: `FiQA2018`;
- sample: 4096 docs, 512 queries;
- metrics: dense Top100 overlap, score Pearson, posting active Jaccard.

| Candidate | Ratio | Top100 | Pearson | Doc Active J | Query Active J |
| --- | ---: | ---: | ---: | ---: | ---: |
| `dense_fp32` | 1.000 | 1.00000 | 1.00000 | 1.00000 | 1.00000 |
| `dense_fp16_output` | 0.500 | 0.99990 | 1.00000 | 0.99972 | 0.99982 |
| `row_int8` | 0.251 | 0.99670 | 0.99999 | 0.98949 | 0.99105 |
| `row_int4` | 0.126 | 0.95730 | 0.99747 | 0.84678 | 0.85722 |
| `shared_variance_k768` | 0.750 | 0.91129 | 0.98879 | 0.55671 | 0.56276 |
| `row_topk_int8_k384` | 0.282 | 0.86326 | 0.97516 | 0.59741 | 0.60069 |
| `shared_variance_k512` | 0.500 | 0.83570 | 0.96276 | 0.34615 | 0.35599 |

## Interpretation

The result is a strong positive signal for row-wise scalar quantization and a
negative signal for raw-dimension pruning:

- `row_int8` keeps almost all dense order and posting geometry at roughly
  one quarter of fp32 output bytes.
- `row_int4` is still surprisingly close in rank Pearson, but loses too much
  active posting overlap for a strict dense-preservation gate.
- raw shared-dimension masks are weak, even at 768/1024 dimensions.
- per-row raw topK sparsification is also weak compared with row-int8.

This suggests the PPLX dense surface is not naturally sparse in raw dimensions.
The earlier M392-M396 signed-coordinate route works because it rotates and
structures the dense surface before posting, not because raw dense dimensions
can simply be pruned.

## Decision

Promote:

- `row_int8` as the main output/index compression candidate.
- `dense_fp16_output` only as a storage baseline; it is less compressed than
  int8 and does not answer model-side compute compression.

Keep as exploratory:

- `row_int4`, likely as approximate storage plus rerank, not as the strict
  first-stage target.

Reject for now:

- raw shared-dimension pruning;
- raw per-row topK sparsification;
- sign-only raw topK.

## Next Step

M502 should run a true retrieval metric check with qrels, not only dense-order
preservation:

1. build exact dense, row-int8, row-int4, and dense+BM25 comparison on available
   materialized tasks;
2. report Recall@100, MRR@20, NDCG@10, MAP@100, and dense-overlap metrics;
3. if row-int8 has negligible qrels loss, convert it into the default compressed
   index surface;
4. only then start model-side compression or QAT, using row-int8 as the
   preservation gate.

Current blocker: `spark-1` only has `FiQA2018` under the visible M398 data root.
Broader BEIR/MTEB confirmation needs the full materialized task root restored or
synced.

Update: the FiQA-only M502 check has now been run. It confirms that `row_int8`
is effectively dense-equivalent on FiQA qrels and should be promoted to broader
materialized BEIR/MTEB validation when the full task root is available.
