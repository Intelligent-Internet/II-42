# BM25 vs BM25+SAE Current Matrix Report

Date: 2026-05-12

Update: this two-way report is now complemented by the three-way comparison in
`sae-bm25-dense-sae-current-matrix-report.md`, which adds the missing
BM25+dense baseline without rerunning the BM25 and BM25+SAE rows.

## Purpose

This report reruns the current SAE recall-matrix artifacts on the current
`sae` branch and compares:

```text
BM25
BM25 + SAE
```

The report separates query-loop performance from recall/ranking quality. The
performance numbers are Python research-harness timings over already prepared
documents, queries, BM25 postings, and SAE latent postings. They are useful for
relative cost in the current matrix, but they are not PostgreSQL native access
method latency.

## Configuration

```text
data_root = /tmp/ii42_sae_quality_matrix
datasets = scifact, scidocs, nfcorpus, arguana, fiqa
run_name = sae_8192_64
top_k = 100
repeat = 5
warmup = 1
score_mode = normalized_idf_dot
bm25_mode = plain
bm25_weight = 1.0
sae_weight = 2.0
```

Command:

```bash
python3 scripts/research_sae_bm25_sae_matrix_report.py \
  --output-dir results/sae/phase416/bm25-vs-sae-current \
  --repeat 5 \
  --warmup 1
```

## Query Performance Matrix

| Dataset | Docs | Queries | BM25 mean ms | BM25 p95 ms | BM25 QPS | BM25+SAE mean ms | BM25+SAE p95 ms | BM25+SAE QPS | Mean ms delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `scifact` | 2000 | 100 | `1.1920` | `2.0996` | `838.9` | `5.4130` | `7.3304` | `184.7` | `+354.1%` |
| `scidocs` | 2000 | 100 | `0.8846` | `1.6199` | `1130.4` | `4.3191` | `6.2076` | `231.5` | `+388.2%` |
| `nfcorpus` | 2063 | 100 | `0.3572` | `1.3035` | `2799.7` | `2.8053` | `4.1146` | `356.5` | `+685.4%` |
| `arguana` | 2000 | 100 | `6.1679` | `10.6680` | `162.1` | `9.6116` | `16.8009` | `104.0` | `+55.8%` |
| `fiqa` | 2000 | 100 | `1.1093` | `2.0505` | `901.4` | `5.0742` | `6.5233` | `197.1` | `+357.4%` |
| `mean` | - | - | `1.9422` | `3.5483` | `1166.5` | `5.4446` | `8.1954` | `214.8` | `+180.3%` |

Interpretation:

- BM25+SAE costs about `2.8x` BM25-only in this Python matrix.
- The absolute query-loop times are still small on the 2k-doc sampled corpora,
  but this path is not the final native implementation.
- `arguana` has the slowest BM25-only loop because lexical query/posting fanout
  is much larger there, so the relative BM25+SAE overhead is smaller.

## Recall And Ranking Matrix

| Dataset | Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `scifact` | `BM25` | `0.9122` | `0.9517` | `0.7803` | `0.7982` | `0.7578` |
| `scifact` | `BM25+SAE` | `0.9600` | `0.9800` | `0.8102` | `0.8234` | `0.7940` |
| `scidocs` | `BM25` | `0.4160` | `0.5720` | `0.5524` | `0.3405` | `0.2506` |
| `scidocs` | `BM25+SAE` | `0.5175` | `0.7040` | `0.6624` | `0.4418` | `0.3406` |
| `nfcorpus` | `BM25` | `0.2204` | `0.2660` | `0.6106` | `0.3730` | `0.1834` |
| `nfcorpus` | `BM25+SAE` | `0.2548` | `0.3763` | `0.6850` | `0.4478` | `0.2464` |
| `arguana` | `BM25` | `0.8700` | `0.9700` | `0.4615` | `0.5346` | `0.4643` |
| `arguana` | `BM25+SAE` | `1.0000` | `1.0000` | `0.5126` | `0.6245` | `0.5126` |
| `fiqa` | `BM25` | `0.5939` | `0.7578` | `0.5473` | `0.4694` | `0.4109` |
| `fiqa` | `BM25+SAE` | `0.7901` | `0.9131` | `0.7472` | `0.6806` | `0.6322` |
| `mean` | `BM25` | `0.6025` | `0.7035` | `0.5904` | `0.5031` | `0.4134` |
| `mean` | `BM25+SAE` | `0.7045` | `0.7947` | `0.6835` | `0.6036` | `0.5052` |

## Mean Quality Delta

| Metric | BM25 | BM25+SAE | Delta |
| --- | ---: | ---: | ---: |
| `Recall@20` | `0.6025` | `0.7045` | `+0.1020` |
| `Recall@100` | `0.7035` | `0.7947` | `+0.0912` |
| `MRR@20` | `0.5904` | `0.6835` | `+0.0931` |
| `NDCG@10` | `0.5031` | `0.6036` | `+0.1005` |
| `MAP@100` | `0.4134` | `0.5052` | `+0.0918` |

## Decision

The current matrix supports the core claim that SAE is a useful semantic sparse
signal beside BM25:

- BM25+SAE improves every reported mean quality metric.
- Recall@100 improves by about `9.1` points.
- Recall@20 improves by about `10.2` points.
- MRR@20 improves by about `9.3` points, so the gain is not only a tail-recall
  effect.

The cost side is also clear:

- The Python BM25+SAE research scorer is materially slower than BM25-only.
- This confirms why the Phase 4 v4 candidate path matters: the production
  direction should not scan full SAE postings naively.
- Phase 5 should proceed from the v4 impact-head candidate path and resident
  payload work, not from the full Python fusion loop.

## Verification

Commands run:

```bash
python3 -m py_compile scripts/research_sae_bm25_sae_matrix_report.py
git diff --check

python3 scripts/research_sae_bm25_sae_matrix_report.py \
  --output-dir results/sae/phase416/bm25-vs-sae-current \
  --repeat 5 \
  --warmup 1
```
