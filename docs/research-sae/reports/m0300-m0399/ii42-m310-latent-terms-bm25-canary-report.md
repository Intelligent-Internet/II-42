# ii42 M310 Latent-Terms BM25 Canary Report

Date: 2026-06-16

## Purpose

M310 tests the `Latent Terms` paper route on our current M307 posting
surfaces. The goal is not to train another scorer. The goal is to check
whether SAE atoms become more useful when they are treated as BM25-style
latent terms instead of as raw dense-dot sparse activations.

This canary uses the existing M307 candidate surfaces, so it evaluates scoring
over already materialized candidates. It does not yet prove full-corpus
candidate-generation quality.

## Script

```text
scripts/research_sae_m310_latent_bm25_canary.py
```

Output root on `spark-2`:

```text
/home/huoju/leask/runs/ii42-m310-latent-bm25-canary-v1
```

Evaluated surfaces:

```text
/home/huoju/leask/runs/ii42-m307-source-balanced-v1/validation
/home/huoju/leask/runs/ii42-m307-adaptive-admission-v1/eval_rerun/term-length
/home/huoju/leask/runs/ii42-m307-adaptive-admission-v1/threshold_sweep/term_t5/term-length
/home/huoju/leask/runs/ii42-m307-adaptive-admission-v1/deep_validation
```

All runs used `--strict`, so missing SAE atom fields would fail the run.

## Scoring Variants

| Variant | Meaning |
| --- | --- |
| `latent_idf_dot` | Existing matched SAE impact multiplied by atom IDF. |
| `latent_value_bm25` | BM25 saturation over raw `doc_value`, with query activation as query TF. |
| `latent_impact_bm25` | BM25 saturation over matched `impact`. |
| `latent_binary_bm25` | Atom identity and query activation with IDF, without doc-value saturation. |
| `bm25_plus_*_additive` | Per-query min-max BM25 plus latent score. |
| `bm25_plus_*_residual` | Latent score plus positive BM25 residual only. |

## Aggregate Results

### Source-Balanced Validation

Rows: `240`

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.313878 | 0.398722 | 0.242126 | 0.131074 |
| `sae` | 0.507487 | 0.486615 | 0.343395 | 0.228176 |
| `posting_score` | 0.436341 | 0.490744 | 0.312186 | 0.190215 |
| `latent_idf_dot` | 0.508188 | 0.466808 | 0.329536 | 0.216710 |
| `latent_binary_bm25` | 0.509056 | 0.463029 | 0.328785 | 0.216098 |
| `bm25_plus_latent_binary_bm25_additive_w0p5` | 0.509707 | 0.517832 | 0.361040 | 0.236553 |

### Adaptive `term-length` T3

Rows: `240`

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.285248 | 0.398722 | 0.242126 | 0.129817 |
| `sae` | 0.507445 | 0.486714 | 0.343453 | 0.228219 |
| `posting_score` | 0.436341 | 0.490744 | 0.312186 | 0.190215 |
| `latent_idf_dot` | 0.509827 | 0.466808 | 0.329536 | 0.216755 |
| `latent_binary_bm25` | 0.508401 | 0.463029 | 0.328785 | 0.216020 |
| `bm25_plus_latent_binary_bm25_additive_w0p5` | 0.509593 | 0.517832 | 0.361390 | 0.236554 |

### Adaptive `term-length` T5

Rows: `240`

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.285248 | 0.398722 | 0.242126 | 0.129817 |
| `sae` | 0.509330 | 0.487724 | 0.344326 | 0.228950 |
| `posting_score` | 0.436341 | 0.490744 | 0.312186 | 0.190215 |
| `latent_idf_dot` | 0.509708 | 0.467602 | 0.330230 | 0.217550 |
| `latent_binary_bm25` | 0.507845 | 0.463203 | 0.328955 | 0.216182 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.517268 | 0.510572 | 0.360828 | 0.239218 |

### Deep Validation K1000

Rows: `240`

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.285248 | 0.398722 | 0.242126 | 0.129817 |
| `sae` | 0.495761 | 0.490133 | 0.345882 | 0.229014 |
| `posting_score` | 0.436341 | 0.490744 | 0.312186 | 0.190215 |
| `latent_idf_dot` | 0.509122 | 0.470364 | 0.333336 | 0.217362 |
| `latent_binary_bm25` | 0.500994 | 0.465332 | 0.330522 | 0.215537 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.502642 | 0.512363 | 0.358294 | 0.237189 |

## Interpretation

The signal is real, but specific:

- `latent_value_bm25` is bad. Saturating raw SAE activation values damages
  ranking badly.
- `latent_impact_bm25` is also weak. Saturating matched dense-style impact is
  not the right primitive for our current SAE values.
- `latent_idf_dot` and `latent_binary_bm25` are competitive with SAE recall,
  but weaker on top-rank metrics by themselves.
- `BM25 + latent_binary_bm25` is the useful route. It improves MRR/NDCG/MAP
  over raw SAE on all checked surfaces and often keeps or improves Recall@100.

This supports the paper's central point: SAE atoms are more useful when treated
as latent lexical terms with IDF-like statistics than when treated as a raw
vector score. For our current material, atom identity and rarity matter more
than activation magnitude saturation.

## Decision

Promote M310 as the next research direction. Do not continue broad M307 loss
weight sweeps until M310 is tested as a candidate-generation primitive.

The current M310 result is still a candidate-surface canary, not a product
model. The next required test is to rebuild candidate generation with latent
BM25 scores, not merely rescore existing candidates.

## M310B Builder Canary

M310B adds latent-term candidate heaps to the M307 posting-surface builder. The
builder now runs an optional SAE document-frequency prepass and emits two new
candidate sources:

```text
latent_score = sum(atom_idf * query_atom_tf)
latent_fused_score = bm25_weight * bm25_score
                   + latent_weight * latent_score
```

The first canaries used strict no-force-positive surfaces. They also required
query/qrel namespace checks to pass; an older `nfcorpus` root was rejected
because its qrels were `beir15:*` while queries were `PLAIN-*`.

Output root on `spark-2`:

```text
/home/huoju/leask/runs/ii42-m310b-latent-builder-canary-v1
```

Builder settings:

```text
checkpoint: /home/huoju/leask/runs/ii42-m190-m150-fusionnorm-strictscale-v1/bm25sae_stageb_best.pt
candidate_k: 300
bm25_candidate_k: 300
sae_candidate_k: 300
latent_candidate_k: 300
latent_fused_candidate_k: 300
latent_weight: 0.35
latent_query_k3: 8.0
doc_active_k: 64
query_active_k: 80
no_force_positives: true
```

### M310B Results

`nfcorpus_q35` used the aligned pre7 full-corpus input root:

```text
/home/huoju/leask/runs/ii42-m160a-next-pre7-clean-full-corpus-input-v1/nfcorpus
```

Summary: `35` queries, `3,633` docs, SAE DF prepass `10,847` atoms,
positive coverage `0.206030`, selected docs `3,555`.

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.020058 | 0.000000 | 0.000000 | 0.000485 |
| `sae` | 0.055165 | 0.000000 | 0.000000 | 0.001245 |
| `posting_score` | 0.048872 | 0.000000 | 0.000000 | 0.001001 |
| `emitted_latent_score` | 0.079082 | 0.002857 | 0.005064 | 0.002401 |
| `latent_binary_bm25` | 0.079082 | 0.002857 | 0.005064 | 0.002401 |
| `bm25_plus_latent_binary_bm25_additive_w0p1` | 0.075510 | 0.002857 | 0.005064 | 0.002304 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.074150 | 0.002381 | 0.000000 | 0.001835 |

`fiqa_q120` used the official unified all-test root:

```text
/home/huoju/leask/runs/ii42-m1000-official-unified-v1/all-test/fiqa
```

Summary: `120` queries, `57,638` docs, SAE DF prepass `14,576` atoms,
positive coverage `0.899408`, selected docs `33,757`.

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.517103 | 0.328726 | 0.270572 | 0.209885 |
| `sae` | 0.802050 | 0.576287 | 0.487867 | 0.431081 |
| `posting_score` | 0.673932 | 0.483974 | 0.382882 | 0.327122 |
| `emitted_latent_score` | 0.775013 | 0.580790 | 0.474603 | 0.424157 |
| `latent_binary_bm25` | 0.775013 | 0.580790 | 0.474603 | 0.424157 |
| `bm25_plus_latent_binary_bm25_additive_w0p1` | 0.802467 | 0.586247 | 0.493253 | 0.436025 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.788800 | 0.612446 | 0.519147 | 0.457742 |

`scifact_q120` used the M160A all-test root with aligned unprefixed query and
qrel ids:

```text
/home/huoju/leask/runs/ii42-m160a-next-beir-b2-broad-step12000-local-v1/all-test/scifact
```

Summary: `120` queries, `5,183` docs, SAE DF prepass `11,621` atoms,
positive coverage `1.000000`, selected docs `5,181`.

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.865000 | 0.641637 | 0.664798 | 0.627673 |
| `sae` | 0.965000 | 0.761044 | 0.793657 | 0.750304 |
| `posting_score` | 0.931667 | 0.684105 | 0.706768 | 0.675713 |
| `emitted_latent_score` | 0.965000 | 0.737884 | 0.775029 | 0.730593 |
| `latent_binary_bm25` | 0.965000 | 0.737884 | 0.775029 | 0.730593 |
| `bm25_plus_latent_binary_bm25_additive_w0p1` | 0.965000 | 0.761682 | 0.792662 | 0.752162 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.983333 | 0.766929 | 0.798132 | 0.757316 |

`scidocs_q120_official` used a clean official-query root prepared for this
canary. The available `m150-beir-full-pplx/scidocs` root had full document
embeddings but synthetic queries, so M310B generated official test queries and
qrels with the same `beir15:scidocs:*` namespace, then embedded only those
queries:

```text
/home/huoju/leask/runs/ii42-m310b-official-roots-v1/scidocs
```

Root validation summary: `25,657` docs, `1,000` official test queries,
`29,928` qrels, `0` missing qrel query/doc ids, `1024`-dim query/doc
embeddings.

Summary: `120` queries, `25,657` docs, SAE DF prepass `14,117` atoms,
positive coverage `0.630068`, selected docs `21,622`.

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.320417 | 0.263912 | 0.141944 | 0.093171 |
| `sae` | 0.446667 | 0.310805 | 0.183036 | 0.126710 |
| `posting_score` | 0.372917 | 0.300113 | 0.172753 | 0.112166 |
| `emitted_latent_score` | 0.440833 | 0.320145 | 0.186709 | 0.129327 |
| `latent_binary_bm25` | 0.440833 | 0.320145 | 0.186709 | 0.129327 |
| `bm25_plus_latent_binary_bm25_additive_w0p1` | 0.443333 | 0.320148 | 0.191714 | 0.132996 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.430000 | 0.328161 | 0.195030 | 0.134358 |

This was the first clean official `scidocs` M310B sample. M310B then reused
the same DF cache to run all `1,000` official `scidocs` test queries:

```text
/home/huoju/leask/runs/ii42-m310b-latent-builder-canary-v1/scidocs_all_official
```

Summary: `1,000` queries, `25,657` docs, SAE DF cache `14,117` atoms,
positive coverage `0.644886`, selected docs `25,650`.

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.346950 | 0.277053 | 0.151925 | 0.103311 |
| `sae` | 0.446833 | 0.346620 | 0.195328 | 0.136642 |
| `posting_score` | 0.395483 | 0.308093 | 0.174828 | 0.120192 |
| `emitted_latent_score` | 0.438783 | 0.345214 | 0.194057 | 0.135000 |
| `latent_binary_bm25` | 0.438783 | 0.345214 | 0.194057 | 0.135000 |
| `bm25_plus_latent_binary_bm25_additive_w0p1` | 0.447100 | 0.355684 | 0.200118 | 0.140613 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.453633 | 0.359009 | 0.204654 | 0.144081 |
| `bm25_plus_latent_binary_bm25_additive_w0p5` | 0.456483 | 0.360135 | 0.205442 | 0.144209 |

The full `scidocs` result is stronger than the 120-query sample. BM25+latent
beats raw SAE on all four reported metrics without forced positives. This
matches the M310B pattern on `fiqa`, `scifact`, `quora`, and `nq`.

The emitted builder score exactly matched the evaluator's recomputed
`latent_binary_bm25` on all checked M310B surfaces. That confirms the builder
path is using the intended latent-term scoring, not an evaluator-only artifact.

`quora_q80` used the `m150-beir-full-pplx` full document root. The root did
not have `quality_qrels.json`, so M310B first materialized aligned dev qrels
from official BEIR:

```text
/home/huoju/leask/runs/m150-beir-full-pplx/quora
```

Qrel materialization summary: `522,931` docs, `10,230` queries in the root,
`5,000` qrel queries, `7,626` qrels, `0` missing queries/docs.

Summary: `80` queries, `522,931` docs, SAE DF prepass `14,708` atoms,
positive coverage `0.994681`, selected docs `44,585`.

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.970693 | 0.688054 | 0.677145 | 0.624102 |
| `sae` | 0.984793 | 0.809405 | 0.827749 | 0.778084 |
| `posting_score` | 0.987151 | 0.768512 | 0.777513 | 0.721071 |
| `emitted_latent_score` | 0.987918 | 0.806047 | 0.826889 | 0.779952 |
| `latent_binary_bm25` | 0.987918 | 0.806047 | 0.826889 | 0.779952 |
| `bm25_plus_latent_binary_bm25_additive_w0p1` | 0.988686 | 0.814255 | 0.832498 | 0.785508 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.992179 | 0.826374 | 0.837723 | 0.795635 |

This is the first M310B large-document-set signal. It is still a query sample,
but it proves the latent-term heap is not only a small-corpus artifact.

`nq_q50_official` used a clean official-test root prepared for this canary.
The existing NQ full-document root had synthetic queries, so M310B generated
official test queries/qrels with `beir15:nq:*` ids, embedded the official
queries, and linked the existing PPLX full-corpus document embeddings:

```text
/home/huoju/leask/runs/ii42-m310b-official-roots-v1/nq
```

Root validation summary: `2,681,468` docs, `3,452` official test queries,
`4,201` qrels, `0` missing qrel query/doc ids, `1024`-dim query/doc
embeddings.

Summary: `50` queries, `2,681,468` docs, SAE DF prepass `14,739` atoms,
positive coverage `1.000000`, selected docs `31,205`.

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.700000 | 0.199159 | 0.236123 | 0.182486 |
| `sae` | 0.910000 | 0.461640 | 0.495307 | 0.445712 |
| `posting_score` | 0.810000 | 0.310362 | 0.345833 | 0.285268 |
| `emitted_latent_score` | 0.910000 | 0.436494 | 0.466305 | 0.417703 |
| `latent_binary_bm25` | 0.910000 | 0.436494 | 0.466305 | 0.417703 |
| `bm25_plus_latent_binary_bm25_additive_w0p1` | 0.910000 | 0.454562 | 0.488922 | 0.438287 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.920000 | 0.486677 | 0.522543 | 0.471741 |

This is the strongest M310B large-QA signal so far. The latent-only score
matches raw SAE Recall@100 but is weaker on top-rank metrics. Adding BM25
back with `latent_weight=0.35` improves every checked quality metric over raw
SAE.

M310B then scaled the same clean official NQ root from `50` to `200` official
test queries while using persistent BM25 and SAE posting caches:

```text
/home/huoju/leask/runs/ii42-m310b-cached-scaleup-v1/nq_q200_official
```

Summary: `200` queries, `2,681,468` docs, SAE DF cache `14,739` atoms,
positive coverage `0.983402`, selected docs `118,901`.

Cache artifacts:

```text
bm25 posting cache: 783 MiB, 2,681,468 docs, 799 query-vocab terms
sae posting cache: 4.8 GiB, 2,681,468 docs, 14,739 atoms
sae df cache: 271 KiB
```

Runtime and parity:

```text
cache_miss_done seconds=1826
cache_hit_done seconds=994
cache_miss sha256=506c65194308599470e5b026b1fa91ae9bd5c6480fc16d561fd2fd3579fb623b
cache_hit  sha256=506c65194308599470e5b026b1fa91ae9bd5c6480fc16d561fd2fd3579fb623b
posting_surface exact_match=1
```

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.700000 | 0.196375 | 0.232064 | 0.189262 |
| `sae` | 0.912500 | 0.455348 | 0.498039 | 0.439663 |
| `posting_score` | 0.825833 | 0.299212 | 0.341354 | 0.284087 |
| `emitted_latent_score` | 0.930000 | 0.451773 | 0.490982 | 0.435577 |
| `latent_binary_bm25` | 0.930000 | 0.451773 | 0.490982 | 0.435577 |
| `bm25_plus_latent_binary_bm25_additive_w0p1` | 0.935000 | 0.459264 | 0.497661 | 0.442693 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.942500 | 0.475273 | 0.511339 | 0.459521 |
| `bm25_plus_latent_binary_bm25_additive_w0p5` | 0.942500 | 0.462877 | 0.509108 | 0.447144 |

This is the first larger NQ cache-parity gate. It preserves the q50 pattern:
latent-term BM25 improves Recall@100 over raw SAE, and BM25+latent improves
MRR/NDCG/MAP over raw SAE on the same strict no-force-positive surface. The
cache-hit run still scans the cache and validates qrel-positive docs, but it
skips full document tokenization and document SAE encoding.

### TREC-COVID Clean Official Full Gate

M310B next tested a full official `trec-covid` root:

```text
/home/huoju/leask/runs/ii42-m310b-official-roots-v1/trec-covid
/home/huoju/leask/runs/ii42-m310b-cached-scaleup-v1/trec_covid_all_official_clean
```

The root reuses PPLX document embeddings from `m150-beir-full-pplx` and embeds
the `50` official test queries with `perplexity-ai/pplx-embed-v1-0.6B`.
The PPLX document root contains `171,331` of the `171,332` official documents;
the single missing document id is not referenced by positive qrels. Qrel
materialization produced `50` queries, `24,673` positive qrels,
`17,537` positive docs, `missing_query_qrels=0`, and `missing_doc_qrels=0`.

Summary: `50` queries, `171,331` docs, SAE DF cache `14,580` atoms, positive
coverage `0.379281`, selected docs `23,275`.

Cache artifacts:

```text
bm25 posting cache: 60 MiB, 171,331 docs, 243 query-vocab terms
sae posting cache: 313 MiB, 171,331 docs, 14,580 atoms
sae df cache: 244 KiB
```

Runtime and parity:

```text
cache_miss_done seconds=120
cache_hit_done seconds=63
cache_miss sha256=8f5139d7905c5b54a97c872affed84ea07c472058f9347d5a8ad889955ed2d27
cache_hit  sha256=8f5139d7905c5b54a97c872affed84ea07c472058f9347d5a8ad889955ed2d27
posting_surface exact_match=1
```

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.097195 | 0.790556 | 0.643956 | 0.063762 |
| `sae` | 0.138784 | 0.936667 | 0.811228 | 0.105223 |
| `posting_score` | 0.129625 | 0.928333 | 0.787077 | 0.097530 |
| `latent_binary_bm25` | 0.141246 | 0.903333 | 0.800344 | 0.107152 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.155797 | 0.950000 | 0.850010 | 0.123519 |
| `bm25_plus_latent_binary_bm25_additive_w0p5` | 0.157056 | 0.928333 | 0.845008 | 0.125457 |

This is the strongest clean medical-corpus signal so far. Raw SAE already
beats BM25 strongly, but adding latent-term BM25 and lexical BM25 improves
Recall@100, MRR@20, NDCG@10, and MAP@100 over raw SAE on the strict
no-force-positive surface.

### WEBIS-Touche2020 Clean Official Full Gate

M310B also tested a larger lexical/argument-retrieval corpus:

```text
/home/huoju/leask/runs/ii42-m310b-official-roots-v1/webis-touche2020
/home/huoju/leask/runs/ii42-m310b-cached-scaleup-v1/webis_touche2020_all_official_clean
```

The root reuses complete PPLX document embeddings from `m150-beir-full-pplx`
and embeds the `49` official test queries with
`perplexity-ai/pplx-embed-v1-0.6B`. Qrel materialization produced `49`
queries, `932` positive qrels, `920` positive docs, `missing_query_qrels=0`,
and `missing_doc_qrels=0`.

Summary: `49` queries, `382,545` docs, SAE DF cache `14,688` atoms, positive
coverage `0.775751`, selected docs `25,917`.

Cache artifacts:

```text
bm25 posting cache: 112 MiB, 382,545 docs, 201 query-vocab terms
sae posting cache: 713 MiB, 382,545 docs, 14,688 atoms
sae df cache: 254 KiB
```

Runtime and parity:

```text
cache_miss_done seconds=226
cache_hit_done seconds=95
cache_miss sha256=17bb03f09860ff2820f194c4d81a9bd444cc6afb5abc83f08dab955789b312f2
cache_hit  sha256=17bb03f09860ff2820f194c4d81a9bd444cc6afb5abc83f08dab955789b312f2
posting_surface exact_match=1
```

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.562194 | 0.614108 | 0.385767 | 0.236443 |
| `sae` | 0.430421 | 0.437215 | 0.233729 | 0.131176 |
| `posting_score` | 0.592074 | 0.642283 | 0.376570 | 0.243902 |
| `latent_binary_bm25` | 0.448363 | 0.430151 | 0.232487 | 0.135690 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.533269 | 0.478611 | 0.284664 | 0.178050 |
| `bm25_plus_latent_binary_bm25_additive_w0p5` | 0.555218 | 0.529365 | 0.310492 | 0.192514 |

This is an important lexical-dominant counterexample. BM25 is already much
stronger than raw SAE, and a fixed BM25+latent score does not beat BM25.
However, the full posting surface still contains useful signal:
`posting_score` improves Recall@100, MRR@20, and MAP@100 over BM25 while
slightly lowering NDCG@10. That means M310B should not become a one-size fixed
fusion rule; the next design must distinguish semantic-helpful corpora from
lexical-dominant corpora before productizing any scoring policy.

M310B also rebuilt clean official roots for `nfcorpus`, `scifact`, and `fiqa`.
The older `ii42-m1000-official-unified-v1/all-test` roots mixed prefixed qrels
with some plain-id query embeddings, so the M307 namespace guard correctly
rejected `nfcorpus`. The clean roots use:

```text
/home/huoju/leask/runs/ii42-m310b-official-roots-v1/{nfcorpus,scifact,fiqa}
```

For each root, `quality_qrels.json` uses `beir15:<dataset>:{q,d}:` ids,
`documents.jsonl` links to the prefixed PPLX document embeddings from
`m150-beir-full-pplx`, and `queries.jsonl` is freshly embedded from the
official qrels-backed query subset. Each root was checked with
`missing_qrel_queries=0` and `missing_qrel_docs=0`.

### Clean Small Official Full Gates

Output root on `spark-2`:

```text
/home/huoju/leask/runs/ii42-m310b-cached-scaleup-v1
```

`nfcorpus_all_official_clean`: `323` queries, `3,633` docs, SAE DF cache
`10,847` atoms, positive coverage `0.394925`, selected docs `3,633`.

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.256420 | 0.517400 | 0.306274 | 0.139794 |
| `sae` | 0.304480 | 0.525305 | 0.316721 | 0.140363 |
| `posting_score` | 0.303556 | 0.562277 | 0.349367 | 0.165380 |
| `latent_binary_bm25` | 0.300856 | 0.503530 | 0.304492 | 0.136359 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.320050 | 0.526535 | 0.334712 | 0.155946 |
| `bm25_plus_latent_binary_bm25_additive_w0p5` | 0.319688 | 0.539764 | 0.341923 | 0.160444 |

`scifact_all_official_clean`: `300` queries, `5,183` docs, SAE DF cache
`11,621` atoms, positive coverage `1.000000`, selected docs `5,183`.

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.885889 | 0.629815 | 0.661428 | 0.619535 |
| `sae` | 0.957667 | 0.741248 | 0.767013 | 0.726629 |
| `posting_score` | 0.929889 | 0.682741 | 0.712584 | 0.675109 |
| `latent_binary_bm25` | 0.961000 | 0.721392 | 0.751639 | 0.707249 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.976667 | 0.760812 | 0.791027 | 0.748490 |
| `bm25_plus_latent_binary_bm25_additive_w0p5` | 0.973333 | 0.759924 | 0.788504 | 0.748443 |

`fiqa_all_official_clean`: `648` queries, `57,638` docs, SAE DF cache
`14,576` atoms, positive coverage `0.883939`, selected docs `51,582`.

| Scorer | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.508614 | 0.298677 | 0.235998 | 0.188620 |
| `sae` | 0.742807 | 0.514972 | 0.430176 | 0.374315 |
| `posting_score` | 0.641454 | 0.410419 | 0.331721 | 0.275972 |
| `latent_binary_bm25` | 0.737995 | 0.500607 | 0.418763 | 0.363468 |
| `bm25_plus_latent_binary_bm25_additive_w0p35` | 0.762276 | 0.524958 | 0.446541 | 0.385038 |
| `bm25_plus_latent_binary_bm25_additive_w0p5` | 0.759040 | 0.523009 | 0.443218 | 0.382242 |

All three clean gates passed cache-miss/cache-hit byte-for-byte parity:

| Dataset | Cache miss/hit SHA-256 | Surface size |
| --- | --- | ---: |
| `nfcorpus` | `1123c6a0ab957fb2005b53a94627a98e070a7852b3e3649fcaccc8fb223e56f8` | 235,294,415 |
| `scifact` | `6ff472f8e378028809517ae3df1c7f15e059b32fa96be19a9aac80695fce4c27` | 391,343,235 |
| `fiqa` | `9652ce16fd3273e3d1bcba3893db6d0278d4d7c767805859b12deb3300bbea0a` | 1,313,428,659 |

These clean full gates strengthen the M310B decision. `nfcorpus` remains the
hardest corpus, but BM25+latent improves over raw SAE on Recall@100 and
top-rank metrics. `scifact` and `fiqa` both show the desired pattern clearly:
raw SAE is strong, latent-term scoring keeps the semantic signal, and adding
BM25 back improves top-rank quality.

### Cross-Corpus Fixed-Policy Summary

The current clean/cached M310B surfaces give the following fixed-policy read.
For each dataset, the table chooses the better of fixed `w0.35` and `w0.5` by
NDCG@10, then compares that row with raw SAE and BM25.

| Dataset | Best fixed | vs SAE Recall@100 | vs SAE MRR@20 | vs BM25 Recall@100 | vs BM25 MRR@20 | Pattern |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `nfcorpus` | `w0.5` | +0.015208 | +0.014459 | +0.063269 | +0.022364 | fixed helps both |
| `scifact` | `w0.35` | +0.019000 | +0.019564 | +0.090778 | +0.130997 | fixed helps both |
| `fiqa` | `w0.35` | +0.019469 | +0.009986 | +0.253662 | +0.226281 | fixed helps both |
| `scidocs` | `w0.5` | +0.009650 | +0.013515 | +0.109533 | +0.083082 | fixed helps both |
| `trec-covid` | `w0.35` | +0.017013 | +0.013333 | +0.058602 | +0.159444 | fixed helps both |
| `webis-touche2020` | `w0.5` | +0.124796 | +0.092150 | -0.006977 | -0.084742 | semantic-helpful, BM25 strong |
| `nq-q200` | `w0.35` | +0.030000 | +0.019925 | +0.242500 | +0.278897 | fixed helps both |

This summary changes the next design target. A single fixed M310B policy is
not obviously wrong: it helps both baselines on most checked surfaces. But
`webis-touche2020` shows that lexical-dominant corpora can already have a very
strong BM25 ordering, and fixed semantic admission can still pull the final
score below BM25. The next useful exploration is therefore not another broad
loss sweep; it is a runtime-safe adaptive policy that can decide when to apply
latent admission, when to downweight it, and when to leave BM25 dominant.

## M310B Support Tooling

Two support changes were added to keep larger canaries repeatable:

```text
scripts/research_sae_materialize_beir_quality_qrels.py
```

This materializes `quality_qrels.json` from official BEIR qrels while matching
the target root's actual id namespace. It fails if too few qrel queries can be
matched, which prevented accidental use of `scidocs` synthetic-query-only roots
as evaluation roots.

```text
--sae-df-cache /path/to/cache.json
```

The M307/M310B builder can now cache SAE atom document frequencies. Cache
metadata is validated against dataset, dataset root, checkpoint, dimensions,
features, `doc_active_k`, and `latent_query_k3`.

Smoke result:

```text
/home/huoju/leask/runs/ii42-m310b-cache-smoke-v1/cache/nfcorpus_m190_d64_q80.json
```

First run wrote `10,847` atom DFs. Second run hit the cache and skipped the
expensive SAE DF prepass.

M310B now also supports a persistent clipped-document SAE posting cache:

```text
--sae-posting-cache /path/to/cache.jsonl
```

This cache stores each document's clipped SAE atoms after the expensive doc
encoder pass. Metadata is validated against dataset, dataset root, checkpoint,
embedding dimension, feature count, and `doc_active_k`. On a cache miss, the
builder writes both the posting cache and the DF cache in a single document
encoding pass. On a cache hit, traversal reads cached SAE postings directly and
does not initialize or run the document SAE encoder.

Posting-cache smoke on `spark-2`:

```text
/home/huoju/leask/runs/ii42-m310b-posting-cache-smoke-v1
```

Settings: `nfcorpus`, `8` queries, `3,633` docs, M190 checkpoint,
`doc_active_k=64`, `query_active_k=80`, `no_force_positives=true`,
`candidate_k=300`, and all BM25/SAE/latent candidate heaps enabled.

Cache miss:

```text
sae posting cache wrote ... docs=3633 atoms=10847
sae df cache wrote ... atoms=10847
sae posting traversal cache hit ... docs=3633
```

Cache hit:

```text
sae df cache hit ... atoms=10847
sae posting traversal cache hit ... docs=3633
posting_surface exact_match=1
sha256=f38428e7669dbc3799035c12bfc8438d6b4459055af73f58c60195eeb9a76f8e
```

The exact `posting_surface.jsonl` match means cache-hit traversal is not only
functionally valid; it is byte-for-byte identical to the cache-miss path for
the checked surface. The downstream M310 strict evaluator also consumed the
cache-hit surface successfully. On this tiny 8-query smoke, `latent_binary_bm25`
raised Recall@100 from raw SAE `0.151316` to `0.250000`, which is consistent
with the larger `nfcorpus` canary direction but should not be interpreted as a
quality gate by itself.

M310B also now supports a query-vocabulary-scoped BM25 posting cache:

```text
--bm25-posting-cache /path/to/cache.jsonl
```

This cache stores each document's full token length plus term counts only for
the selected query vocabulary. Its metadata includes dataset/root identity,
tokenizer identity, query vocabulary size, and a SHA-256 hash of the sorted
query vocabulary. This deliberately makes the cache query-set specific: it is
smaller and faster than a full lexical term-frequency dump, while still being
safe against accidental reuse with a different query set.

The BM25 posting cache is used in two places:

- BM25 statistics: DF, `doc_count`, `total_len`, and `avgdl` are loaded from
  metadata, while the compact cache is scanned to verify required qrel-positive
  document ids are present.
- Candidate traversal: BM25 counts/doc lengths are read from cache instead of
  retokenizing `documents.jsonl`.

Combined BM25+SAE cache smoke on `spark-2`:

```text
/home/huoju/leask/runs/ii42-m310b-bm25-sae-cache-smoke-v1
```

Cache miss:

```text
bm25 posting cache wrote ... docs=3633 terms=7
sae posting cache wrote ... docs=3633 atoms=10847
sae df cache wrote ... atoms=10847
bm25 posting traversal cache hit ... docs=3633
sae posting traversal cache hit ... docs=3633
```

Cache hit:

```text
bm25 posting cache hit ... docs=3633 terms=7
sae df cache hit ... atoms=10847
bm25 posting traversal cache hit ... docs=3633
sae posting traversal cache hit ... docs=3633
posting_surface exact_match=1
sha256=f38428e7669dbc3799035c12bfc8438d6b4459055af73f58c60195eeb9a76f8e
```

This means M310B can now rerun the same query surface without document SAE
encoding and without full-document tokenization during candidate traversal.

`scidocs` initially failed this guardrail because the available
`m150-beir-full-pplx` root was synthetic-query-only. M310B now has a clean
official `scidocs` root with embedded official test queries.
NQ now has the same clean official-test root. `hotpotqa` train qrels can be
materialized against the existing root, but a first full-corpus DF prepass was
stopped after it proved too slow for interactive canary use.

The large-corpus engineering bottleneck is now explicit: without a DF cache,
M310B must scan and encode all documents once to build SAE DF, then scan them
again for candidate traversal. On `nq`, this is tolerable for a canary and
produces a reusable cache. On `hotpotqa`, the first SAE DF prepass progressed
to `500,000` of `5,233,329` docs in about ten minutes, so future hotpotqa/full
BEIR gates should use persistent BM25/SAE posting caches or shardable posting
artifacts instead of rebuilding doc token counts and doc SAE vectors inside
every run.

Remaining engineering note: the current research builder still reads
`documents.jsonl` for selected-document evidence details after candidate
generation. That is much smaller than full traversal, but a product-quality
path should eventually reuse cached lexical evidence for those rows too.

## Updated Decision

M310B should continue. The route is now stronger than a pure rescore smoke:
latent-term BM25 has been validated as an actual candidate-generation source.
The `fiqa_q120` result is especially important because the latent/BM25 fused
score improves all top-rank metrics over raw SAE while remaining strict
no-force-positive.

Do not promote to product or scorer training yet, but the next step is no
longer broad loss sweeping. M310B now has clean full official gates for
`nfcorpus`, `scifact`, `fiqa`, `scidocs`, `trec-covid`, and
`webis-touche2020`, plus an official NQ `q200` cache-parity gate and earlier
`quora` evidence. The next gate should be larger BEIR coverage with persistent
BM25/SAE posting caches where available. The SAE DF prepass is correct but
expensive, so larger runs should use both
`--bm25-posting-cache`, `--sae-posting-cache`, and `--sae-df-cache` and should
not rebuild lexical or latent statistics inside every evaluator invocation.

## Remaining Scale-Up Gate

The next run should keep the same builder contract and scale the checked
datasets. The candidate heap should continue to use:

```text
latent_term_score = sum(atom_idf * query_atom_weight)
bm25_latent_score = bm25_weight * bm25_score + latent_weight * latent_term_score
```

Keep the initial settings conservative:

```text
latent mode: binary/idf
latent_weight: 0.10, 0.35
bm25_weight: 1.0
candidate_k: 300
bm25_candidate_k: 300
latent_candidate_k: 300
no_force_positives: true
```

Acceptance for the next scale-up:

- It must improve or match raw SAE candidate recall without relying on forced
  positives.
- It must improve MRR/NDCG/MAP over current raw SAE scoring on the same
  no-forced surface.
- `nfcorpus`, `scifact`, `fiqa`, official `scidocs`, `trec-covid`,
  `webis-touche2020`, `quora`, and official `nq` are now covered. The
  remaining evidence gap is larger BEIR coverage and scale mechanics: more
  cached official roots, and eventually sharded cache artifacts, before any
  scorer training resumes.

## Official Sharded Full-Corpus Scale-Up Result

This section tracks the larger official gate that runs the same M310B builder
contract through sharded query evaluation. The full 15-dataset M310 and dense
baseline rows are now available under the same official full-corpus surface.

New helper scripts:

```text
scripts/research_sae_prepare_m310b_official_root.py
scripts/research_sae_filter_m310b_query_embeddings.py
scripts/research_sae_m310_merge_shard_evals.py
scripts/research_sae_m310_dense_gap_matrix.py
scripts/run_ii42_m310b_cached_gate_sharded_dataset.sh
```

Primary output roots on `spark-1`:

```text
/home/huoju/leask/runs/ii42-m310b-official-roots-v1
/home/huoju/leask/runs/ii42-m310b-cached-scaleup-v1
/home/huoju/leask/runs/ii42-m310b-dense-baseline-v1
```

The sharded runner first warms dataset-level SAE posting and DF caches on
`shard_0000`, then runs later query shards concurrently. Each shard is evaluated
independently, its large `posting_surface.jsonl` is deleted, and the final
metrics are merged with a query-weighted average. This avoids materializing a
single full-corpus posting surface for large BEIR roots.

Current matrix generation:

```text
docs/research-sae/reports/m0300-m0399/ii42-m310-dense-gap-full15-current.md
ii42-m310-dense-gap-full15-current.json
```

Current ready status:

| Status | Count | Meaning |
| --- | ---: | --- |
| `ready` | 15 / 15 | Official root, M310B final, and dense baseline exist. |
| `partial` | 0 / 15 | No engine-specific partial rows remain. |
| upstream qrels/corpus gap | 1 / 15 | `arguana` has 5 positive qrel doc ids absent from the official corpus. This is fair across all engines on the same materialized corpus, so it stays in the ready mean with a warning label. |

Full-15 ready mean:

| Model | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `dense` | 0.7092 | 0.6670 | 0.5874 | 0.4318 |
| `sae` | 0.6436 | 0.5979 | 0.5122 | 0.3656 |
| `posting_score` | 0.6335 | 0.5576 | 0.4773 | 0.3374 |
| `best_m310_fixed` | 0.6831 | 0.6276 | 0.5477 | 0.3973 |
| `best_m310_fixed - dense` | -0.0260 | -0.0394 | -0.0397 | -0.0346 |

`best_m310_fixed` chooses the better fixed
`bm25_plus_latent_binary_bm25_additive_*` row by NDCG@10 for each dataset.
This is an analysis view, not a train-time per-dataset tuned policy.

The current `quora` strict full-corpus result is especially informative:

| Model | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.9497 | 0.7429 | 0.7457 | 0.7030 |
| `dense` | 0.9960 | 0.8818 | 0.8894 | 0.8596 |
| `sae` | 0.9922 | 0.8592 | 0.8658 | 0.8340 |
| `best_m310_fixed` | 0.9955 | 0.8800 | 0.8854 | 0.8543 |
| `best_m310_fixed - dense` | -0.0005 | -0.0018 | -0.0040 | -0.0053 |

This supports the current interpretation: on semantic-heavy or near-saturated
sets, latent-term BM25 can get very close to dense. The remaining gap is mostly
top-rank scoring and admission calibration, not gross candidate coverage. Some
large QA/fact datasets (`nq`, `hotpotqa`, `fever`, `climate-fever`) still show a
clear dense gap even when candidate coverage is high. On lexical-dominant sets
such as `webis-touche2020`, a fixed semantic blend can help relative to dense
but can still underuse BM25's strongest rank signal. The next modeling target
should therefore be runtime-safe admission/scoring over unified posting
features rather than another global fixed-weight sweep.
