# SAE M97 Stage-A Cost And Ranking Results Report

Date: 2026-05-22

## Decision

M97 validates `m96_k512` as the default compressed Stage-A checkpoint for the next engineering evaluation step. It keeps essentially the same candidate-surface ranking quality as the `m93_k768` fallback while cutting estimated payload size and SAE doc-pairs substantially.

This is not product readiness. The run also exposes the next engineering bottleneck: naive full-union query execution opens the whole M97 eval document set, and the existing M20 full-root Python payload builder is too slow without a candidate/doc subset or streaming implementation.

## Inputs

- Run root: `/home/huoju/leask/runs/m97-stage-a-validation-v2`
- Candidate surface: `/home/huoju/leask/data/ii42_sae_m81/large-stage-a-v0-stable`
- Split: `eval`
- Rows: `886`
- Documents: `30059`
- Queries: `886`
- Profiles:
  - `m90_k1024`: `/home/huoju/leask/runs/m90-k1024-checkpoint-interp-v0/checkpoints/interp_alpha_0.35.pt`
  - `m93_k768`: `/home/huoju/leask/runs/m93-k768-teacher025/m80_sparse_sae.best.pt`
  - `m96_k512`: `/home/huoju/leask/runs/m96-k512-checkpoint-interp-v0/checkpoints/interp_alpha_0.35.pt`

## Candidate-Surface Ranking

These metrics are over the M81/M97 candidate rows. They are ranking evidence for the selected candidate surface, not product full-corpus quality claims.

| Run | Recall@10 | MRR | NDCG@10 |
| --- | ---: | ---: | ---: |
| `bm25` | 0.3178 | 0.5960 | 0.4505 |
| `dense` | 0.2996 | 0.6197 | 0.4652 |
| `m93_k768_bm25_sae_w1` | 0.3509 | 0.6575 | 0.5092 |
| `m96_k512_bm25_sae_w1` | 0.3500 | 0.6592 | 0.5098 |
| `m96_k512_bm25_sae_w2` | 0.3560 | 0.6669 | 0.5180 |
| `m96_k512_sae` | 0.3583 | 0.6584 | 0.5180 |

Interpretation:

- `m96_k512_bm25_sae_w1` is effectively tied with or slightly ahead of `m93_k768_bm25_sae_w1` on MRR/NDCG, with a tiny Recall@10 loss.
- `m96_k512_bm25_sae_w2` is the strongest candidate-surface hybrid point in this sweep.
- BM25+SAE is clearly above BM25 alone on this surface.
- SAE-only is strong on this surface, but that does not mean BM25 should be removed; this surface already contains candidate rows produced by earlier stages and does not prove full-corpus recall.

## Physical Cost Matrix

The physical counts below come from the M97 bitset estimator. They are useful for relative cost comparison across checkpoints. The `mean_ms` column is estimator time, not production query latency.

| Profile | Active dims | Payload MB est. | SAE doc pairs | Mean postings | Mean candidates | Mean rerank terms | Estimator mean ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m90_k1024` | 1024 | 262.8584 | 30780416 | 9549986.8273 | 30059.0000 | 33581164.0000 | 0.6976 |
| `m93_k768` | 768 | 204.1371 | 23085312 | 5681784.7867 | 30059.0000 | 25886060.0000 | 0.6267 |
| `m96_k512` | 512 | 145.3725 | 15390208 | 3195216.8578 | 30059.0000 | 18190956.0000 | 0.3365 |

Interpretation:

- M96 reduces estimated payload size by about 45% versus M90 and about 29% versus M93.
- M96 reduces SAE doc-pairs by about 50% versus M90 and about 33% versus M93.
- The naive full query-atom union still reaches all `30059` docs on average. That is the main remaining physical-engine issue; the next engine pass must avoid treating all active query atoms as an unconstrained candidate union.

## Read-Only Payload Smoke

The full M97 materialized root is large enough that the current M20 Python build path is not suitable as a full-root benchmark. It spends several minutes in Python row-scan/tau/payload preparation before C reader execution. For M97 we therefore ran an explicit candidate-subset smoke over the first 20 candidate rows to verify the compact payload path:

- Output root: `/home/huoju/leask/runs/m97-stage-a-validation-v2/reusable-m20-m96-smoke`
- Documents: `1149`
- Queries: `20`
- Build seconds: `10.1449`
- `EATMH002` payload bytes: `6064336`
- C reader exact ratio: `1.0000`
- C reader mean latency: `0.8045 ms`
- C reader mean touched postings: `9449.6000`
- C reader mean candidates: `1145.3000`
- C reader mean rerank terms: `586393.6000`

This proves the read-only Python/C payload path still works after M96. It does not prove full-root production readiness.

## Harness Fixes

- `research_sae_m97_stage_a_cost_ranking.py` now avoids duplicate default profiles when explicit `--profile` values are supplied.
- M97 physical counting now uses integer bitsets instead of Python set union over posting rows.
- `research_sae_milestone5_evidence_c_reader.py` now compiles the C reader with `_POSIX_C_SOURCE=200809L`, which is required for `clock_gettime` in the Spark container.
- `research_sae_m20_efficiency_matrix.py` now supports `--candidate-subset-queries` for smoke checks over large materialized roots.
- `run_sae_m97_stage_a_validation.sh` skips the naive reusable Python full-posting scanner by default. It can still be enabled with `RUN_PYTHON_FULLSCAN=1`.

## Next Step

Proceed with M98 as an engineering-efficiency pass before SQL/API work:

- Make M20 full-root payload build streaming or postings-driven so it does not materialize/scans unnecessary dense rows in Python.
- Add a query-time candidate control strategy for high-fanout SAE atoms before claiming full-root read-only efficiency.
- Re-run M20 on the full `30059`-doc M97 root after the build/query path is fixed.
- Keep `m93_k768` as fallback, but use `m96_k512` as the default checkpoint for further engine evaluation.

