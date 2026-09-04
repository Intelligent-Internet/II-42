# BM25 vs BM25+dense vs BM25+SAE Current Matrix Report

Date: 2026-05-12

## Purpose

This report extends the current BM25/BM25+SAE matrix with the missing
BM25+dense path. It reuses the existing BM25 and BM25+SAE results from
`results/sae/phase416/bm25-vs-sae-current/bm25_vs_sae_matrix.json`, computes
only BM25+dense, then writes a merged three-way matrix.

The dense path is an exact Python full-scan over the sampled 2k-document
matrices. It is useful for research comparison, but it is not a VectorChord or
ANN benchmark.

## Configuration

```text
base_matrix = results/sae/phase416/bm25-vs-sae-current/bm25_vs_sae_matrix.json
data_root = /tmp/ii42_sae_quality_matrix
datasets = scifact, scidocs, nfcorpus, arguana, fiqa
top_k = 100
repeat = 5
warmup = 1
bm25_weight = 1.0
dense_weight = 1.0
sae_weight = 2.0
```

Command:

```bash
python3 scripts/research_sae_append_bm25_dense_matrix.py \
  --output-dir results/sae/phase417/bm25-dense-sae-current \
  --repeat 5 \
  --warmup 1
```

## Query Performance Matrix

| Dataset | Source | Mean ms | Median ms | P95 ms | Max ms | QPS |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `scifact` | `BM25` | `1.1920` | `1.1082` | `2.0996` | `3.0501` | `838.9` |
| `scifact` | `BM25+dense` | `2.0705` | `2.0140` | `3.1561` | `4.1574` | `483.0` |
| `scifact` | `BM25+SAE` | `5.4130` | `5.1914` | `7.3304` | `29.7292` | `184.7` |
| `scidocs` | `BM25` | `0.8846` | `0.8407` | `1.6199` | `4.1683` | `1130.4` |
| `scidocs` | `BM25+dense` | `1.7169` | `1.6567` | `2.5077` | `3.5427` | `582.5` |
| `scidocs` | `BM25+SAE` | `4.3191` | `3.8931` | `6.2076` | `28.9703` | `231.5` |
| `nfcorpus` | `BM25` | `0.3572` | `0.0588` | `1.3035` | `2.6292` | `2799.7` |
| `nfcorpus` | `BM25+dense` | `1.1751` | `0.9948` | `2.2379` | `3.5543` | `851.0` |
| `nfcorpus` | `BM25+SAE` | `2.8053` | `2.4023` | `4.1146` | `24.7920` | `356.5` |
| `arguana` | `BM25` | `6.1679` | `5.8355` | `10.6680` | `14.9507` | `162.1` |
| `arguana` | `BM25+dense` | `5.3710` | `4.9658` | `8.4374` | `15.5852` | `186.2` |
| `arguana` | `BM25+SAE` | `9.6116` | `8.2497` | `16.8009` | `47.3672` | `104.0` |
| `fiqa` | `BM25` | `1.1093` | `1.0436` | `2.0505` | `3.0378` | `901.4` |
| `fiqa` | `BM25+dense` | `1.7289` | `1.6203` | `2.7025` | `7.1191` | `578.4` |
| `fiqa` | `BM25+SAE` | `5.0742` | `4.9106` | `6.5233` | `28.5650` | `197.1` |
| `mean` | `BM25` | `1.9422` | `1.7774` | `3.5483` | `5.5672` | `1166.5` |
| `mean` | `BM25+dense` | `2.4125` | `2.2503` | `3.8083` | `6.7917` | `536.2` |
| `mean` | `BM25+SAE` | `5.4446` | `4.9294` | `8.1954` | `31.8847` | `214.8` |

## Recall And Ranking Matrix

| Dataset | Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `scifact` | `BM25` | `0.9122` | `0.9517` | `0.7803` | `0.7982` | `0.7578` |
| `scifact` | `BM25+dense` | `0.9447` | `0.9800` | `0.8446` | `0.8545` | `0.8275` |
| `scifact` | `BM25+SAE` | `0.9600` | `0.9800` | `0.8102` | `0.8234` | `0.7940` |
| `scidocs` | `BM25` | `0.4160` | `0.5720` | `0.5524` | `0.3405` | `0.2506` |
| `scidocs` | `BM25+dense` | `0.4925` | `0.6555` | `0.6411` | `0.4207` | `0.3172` |
| `scidocs` | `BM25+SAE` | `0.5175` | `0.7040` | `0.6624` | `0.4418` | `0.3406` |
| `nfcorpus` | `BM25` | `0.2204` | `0.2660` | `0.6106` | `0.3730` | `0.1834` |
| `nfcorpus` | `BM25+dense` | `0.2638` | `0.3723` | `0.6963` | `0.4475` | `0.2411` |
| `nfcorpus` | `BM25+SAE` | `0.2548` | `0.3763` | `0.6850` | `0.4478` | `0.2464` |
| `arguana` | `BM25` | `0.8700` | `0.9700` | `0.4615` | `0.5346` | `0.4643` |
| `arguana` | `BM25+dense` | `1.0000` | `1.0000` | `0.5418` | `0.6390` | `0.5418` |
| `arguana` | `BM25+SAE` | `1.0000` | `1.0000` | `0.5126` | `0.6245` | `0.5126` |
| `fiqa` | `BM25` | `0.5939` | `0.7578` | `0.5473` | `0.4694` | `0.4109` |
| `fiqa` | `BM25+dense` | `0.7725` | `0.9009` | `0.7061` | `0.6304` | `0.5776` |
| `fiqa` | `BM25+SAE` | `0.7901` | `0.9131` | `0.7472` | `0.6806` | `0.6322` |
| `mean` | `BM25` | `0.6025` | `0.7035` | `0.5904` | `0.5031` | `0.4134` |
| `mean` | `BM25+dense` | `0.6947` | `0.7817` | `0.6860` | `0.5984` | `0.5010` |
| `mean` | `BM25+SAE` | `0.7045` | `0.7947` | `0.6835` | `0.6036` | `0.5052` |

## Mean Delta vs BM25

| Metric | BM25+dense | BM25+SAE |
| --- | ---: | ---: |
| `Recall@20` | `+0.0922` | `+0.1020` |
| `Recall@100` | `+0.0782` | `+0.0912` |
| `MRR@20` | `+0.0956` | `+0.0931` |
| `NDCG@10` | `+0.0953` | `+0.1005` |
| `MAP@100` | `+0.0877` | `+0.0918` |
| `mean query ms` | `+24.2%` | `+180.3%` |

## Interpretation

The result is narrower and more useful than the two-way matrix:

- BM25+dense is still the strongest cheap baseline in this Python research
  setup: it recovers most of the BM25+SAE quality gain with much lower measured
  query-loop overhead.
- BM25+SAE slightly beats BM25+dense on mean Recall@20, Recall@100, NDCG@10,
  and MAP@100. BM25+dense slightly beats BM25+SAE on mean MRR@20.
- SAE therefore has a real quality signal, but its production value depends on
  whether the native sparse-impact path can make SAE cheaper than maintaining a
  separate dense vector retrieval layer.

This reinforces the Phase 5 direction: do not tune Python late fusion further.
Move the resident v4 impact-head candidate path toward a production-shaped
payload, because that is the route that can make BM25+SAE competitive on both
quality and systems cost.

## Verification

Commands run:

```bash
python3 -m py_compile scripts/research_sae_append_bm25_dense_matrix.py
git diff --check

python3 scripts/research_sae_append_bm25_dense_matrix.py \
  --output-dir results/sae/phase417/bm25-dense-sae-current \
  --repeat 5 \
  --warmup 1
```

