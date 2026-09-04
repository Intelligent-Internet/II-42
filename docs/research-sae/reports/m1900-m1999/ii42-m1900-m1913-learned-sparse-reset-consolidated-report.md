# M1900-M1913 Learned-Sparse Reset Consolidated Report

Date: 2026-07-12

Decision: **retain OpenSearch sparse-v2 as the frozen product baseline; select
the M1914 calibrated Granite 30M Sparse checkpoint as the single next model
parent; use the Apache-2.0 Unified LSR framework as the training implementation
reference. Preserve M1911 Latent Terms as a secondary qrels-free research
milestone, not the immediate product branch.**

## Reset Outcome

This reset stopped project-specific loss search and required each route to
cross the same evidence chain:

```text
artifact and training provenance
-> frozen representation
-> complete official FiQA
-> exact BMP inverted index
-> quality, bytes, and latency
```

The result is not one universal winner. It separates three roles that previous
experiments repeatedly mixed:

- **product regression baseline:** OpenSearch sparse-v2;
- **next compact model parent:** Granite 30M Sparse;
- **qrels-free latent research branch:** M1911 Nomic Latent Terms.

That separation is deliberate. A checkpoint does not become the product
default merely because it is closest to the desired architecture, and a route
does not lose research value merely because its current index is expensive.

## Complete Native Matrix

| Route | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 | Doc postings | Bytes/doc | BMP p95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| OpenSearch sparse-v2 | **0.369782** | **0.311632** | 0.655266 | **0.455916** | 0.852981 | 13,185,096 | 3,617.96 | 17.499 ms |
| Granite 30M Sparse | 0.352112 | 0.294284 | **0.665725** | 0.438845 | **0.861416** | **10,973,637** | **2,712.42** | **12.555 ms** |
| M1914 calibrated Granite | 0.357677 | 0.298860 | **0.668468** | 0.443793 | 0.859227 | **10,973,637** | **2,712.42** | **11.551 ms** |
| M1911 Nomic Latent Terms | 0.369506 | 0.312466 | 0.658295 | 0.450808 | 0.848217 | 29,917,201 | 5,686.10 | 27.495 ms |
| M1912A frozen SPLARE-like surface | 0.333471 | 0.276175 | 0.635797 | 0.405907 | 0.835393 | not comparable | 4,568.53 | 15.220 ms |
| M1910 small Latent Terms | 0.281955 | 0.234144 | 0.572944 | 0.372689 | 0.788315 | 14,259,000 approx. | 4,388.31 | 17.905 ms |

All first three principal routes pass strict BMP score and boundary exactness
and retain at least 99% of float Recall. The matrix uses BMP quality where
available; M1912A values are its reported native quality and M1910's first row
is float quality because the report's BMP table records only Recall.

## What Each Reproduction Established

### M1903-M1905: schedule and output basis

M1903 proved that the original 100-step SAE-SPLADE result was undertrained:
longer reconstruction and retrieval training materially improved disjoint
ranking. M1904 then completed the FLOPS ramp and showed that the local
from-scratch SAE basis could reduce sampled cost only by losing positive
ordering. M1905 kept the teacher, rows, schedule, and regularization fixed but
replaced the SAE basis with a pretrained vocabulary MLM head. It was the only
post-ramp survivor and dominated M1904 in pairwise ordering, positive top1,
KL, nnz, FLOPS, and sampled maxDF.

Durable conclusion: training depth mattered, but a mature output basis mattered
more. The local from-scratch SAE basis is closed as the optimization parent.

### M1912A: SPLARE-like engine audit

Exact namespace compaction and BMP showed that the external SAE-style surface
was not rejected by the engine. Its quality remained weak. This separated the
model failure from the old raw-touch cost diagnosis, but supplied no checkpoint
that should be deepened.

### M1911: qrels-free Latent Terms

Five TopK-16 SAEs trained on `9.6M` Nomic token states without retrieval labels
reached mean FiQA Recall `0.655292`, only `0.000750` below the OpenSearch float
gate. The qrels-free selected seed reached BMP Recall `0.658295`, NDCG
`0.369506`, and MAP `0.312466`. This is a genuine representation result, not an
oracle or sampled canary.

Its cost is the limitation: roughly 519 document postings, maxDF `1.0`, a
`327.7 MB` FiQA index, and p95 `27.5 ms`. Removing the highest-DF 1% latents
reduces cost but materially harms all quality metrics. A larger source-corpus
run is scientifically authorized, but should not preempt the more compact
mature parent.

### M1913: compact dense-root sparse model

The official Granite checkpoint supplies the missing mature product shape:
about 30M parameters, a retrieval-trained dense root, sparse contrastive
distillation, FLOPS plus total-NORM regularization, and fixed 192/50 document
and query dimensions.

On exact BMP it is 25% smaller and 28% faster than OpenSearch, while improving
Recall by `+0.010459`. Its NDCG, MAP, and MRR remain lower; float MAP misses the
locked 95% floor by only `0.000583`. Granite therefore does not replace the
balanced product baseline, but it is the clearest place to recover head
ordering without reopening representation and cost questions.

## Parent Selection

### Frozen product default: OpenSearch sparse-v2

OpenSearch remains the only route that combines mature head ranking, adequate
Recall, Apache licensing, and a complete exact native closure. Every future
checkpoint must be compared against M1660 without changing the engine or
evaluation surface.

### Next model parent: M1914 calibrated Granite 30M Sparse

Granite is selected over M1911 because it is:

- already a standalone text-to-posting encoder;
- explicitly dense-root and retrieval-distilled;
- materially smaller and faster in the real index;
- Apache-2.0 and released at a pinned revision;
- Recall-positive, with a narrowly localized head-ranking deficit.

The next work must adapt this checkpoint, not initialize another arbitrary
sparse head. The target is not a new scoring trick: recover NDCG/MAP/MRR while
holding the released `192/50` active-dimension budgets, Recall, exact BMP size,
and latency as hard regression gates.

M1914 subsequently recovers part of this deficit with a qrels-free-selected
global asymmetric power transform. It improves FiQA and broad3 macro head
metrics without changing support or index bytes, so it supersedes raw M1913 as
the research parent. M1915 shows that doubling candidate support does not
improve heldout ranking; expanded support and per-term correction are closed.

### Training reference: Unified LSR

Granite's original internal/generated data mixture prevents exact training
reproduction. The pinned Apache Unified LSR `v1.0.0` framework provides the
auditable implementation reference for public-data distillation, asymmetric
query/document encoders, and Anserini-compatible impacts. It is a framework,
not a second model parent.

### Secondary branch: M1911 Latent Terms

M1911 is preserved because it is the strongest local qrels-free result and
demonstrates that token-state SAEs can reach mature retrieval quality. A
100M-token scale-up is allowed only as a separate research branch with a
license-clean source shard and the same five-seed/native gates. It must not
delay the compact Granite parent.

## Next Experimental Contract

The next model stage should begin only after a separate locked contract, with
this order:

1. Freeze M1660 and M1913 complete FiQA artifacts as regression anchors.
2. Reproduce one public-data, paper-native dense-to-sparse distillation stage
   from Granite, retaining the official top-dimension inference limits.
3. Use held-out queries for checkpoint selection and keep FiQA evaluation-only;
   do not search active dimensions, loss weights, or thresholds on FiQA.
4. Require no loss of M1913 Recall/CUB or native cost before considering a
   head-ranking gain.
5. Expand to a second untouched BEIR row before any product promotion.

Stop the adaptation if head gains require more than 192/50 dimensions, increase
BMP bytes or p95 beyond the OpenSearch baseline, or disappear on the second
row. In that case retain OpenSearch as the product route and close compact
Granite adaptation rather than starting another loss family.

## Closed Routes

- local from-scratch SAE-SPLADE output basis;
- more M1904 FLOPS-ramp or KL-weight tuning;
- post-hoc top-DF pruning of Latent Terms;
- promoting the weak M1912A checkpoint because its engine is fast;
- treating SPLADE-v3's non-commercial checkpoint as the product parent;
- another project-specific posting loss before a mature-parent reproduction.

## Reports

- `ii42-m1900-m1913-learned-sparse-reset-matrix.json`
- `docs/research-sae/reports/m1900-m1999/ii42-m1904-sae-splade-full-ramp-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1905-paired-standard-splade-full-ramp-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1910-latent-terms-native-bmp-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1911-nomic-latent-terms-reproduction-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1912a-external-splare-native-bmp-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1913-granite-30m-sparse-native-control-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1900-learned-sparse-reset-research-map.md`
