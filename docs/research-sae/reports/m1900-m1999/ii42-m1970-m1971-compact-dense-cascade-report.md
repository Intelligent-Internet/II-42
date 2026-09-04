# M1970/M1971 compact-dense cascade report

Date: 2026-07-15

## Decision

This route produced a real positive representation result, but it is not yet a native product result.

The useful product target is not a compact vector that reproduces global dense retrieval. It is a compact correction payload that operates only on candidates already recalled by frozen M190 latent postings and BM25. Under this objective, a fixed `256d` packed INT4 payload passes all seven available rows and retains almost all of the exact dense tail-correction gain.

The current recommendation is:

- keep `256d packed INT4`, `132 B/document`, as the native-closure candidate;
- retain `64d packed INT4`, `36 B/document`, as an ultra-light experimental tier;
- stop the sign-only 1-bit family as a universal policy;
- do not train a smaller encoder until the fixed 256d policy passes the native DB path and broader rows.

## Product hypothesis

```text
text -> one dense-root encoder
     -> M190 latent postings + lexical terms
     -> one sparse candidate path
     -> compact dense code attached to each candidate document
     -> protect baseline ranks 1-20
     -> compact correction only reorders the remaining tail
```

This is not an ANN index plus BM25. The compact vector is not searched globally. It is a small per-document payload fetched only for the approximately 1,600-1,800 documents returned by the unified sparse candidate path.

The current proof still uses the existing 0.6B PPLX dense root. It proves that the index payload and candidate scoring can be small; it does not yet prove that neural inference can use a smaller backbone.

## Experimental contract

- Candidate sources are frozen M190 latent BM25 and exact ID-bound BM25, each at depth 1000.
- Baseline fusion is the native-style per-source min-max sum with semantic weight `1.0` and BM25 weight `0.5`.
- The compact dense correction has one fixed weight, `0.5`.
- Baseline ranks 1-20 are protected. Compact dense can only reorder the remaining candidates into ranks 21-100.
- NDCG@10 and MRR@20 are therefore preserved by construction, not selected after evaluation.
- The projection is an uncentered PCA basis built without qrels from 100,785 documents in FiQA, NFCorpus, SciFact, ArguAna, and SciDocs.
- TREC-COVID and Webis-Touche2020 are document-corpus holdouts for the shared projection.
- Qrels are used only for final metrics. Dimensions, quantization families, correction weight, and protected-head policy were declared before aggregation.
- The shared basis artifact is `/home/huoju/leask/runs/ii42-m1970-compact-dense-cascade-v1/basis/shared5-1024.npz`, SHA-256 `8e17ac0bdc83f0ca2b79d125a85aaede7ccb774545190d34345f81180cf2a4d7`.

## Main result

### 256d packed INT4 fixed candidate

| Dataset | Bytes/doc | NDCG@10 delta | MAP@100 delta | Recall@100 delta | MRR@20 delta |
| --- | ---: | ---: | ---: | ---: | ---: |
| arguana | 132 | +0.000000 | +0.000273 | +0.003569 | +0.000000 |
| fiqa | 132 | +0.000000 | +0.002241 | +0.015708 | +0.000000 |
| nfcorpus | 132 | +0.000000 | +0.002107 | +0.007438 | +0.000000 |
| scidocs | 132 | +0.000000 | +0.001567 | +0.014900 | +0.000000 |
| scifact | 132 | +0.000000 | +0.000081 | +0.003333 | +0.000000 |
| trec-covid | 132 | +0.000000 | +0.006468 | +0.007537 | +0.000000 |
| webis-touche2020 | 132 | +0.000000 | -0.000216 | +0.003146 | +0.000000 |
| **Macro** | **132** | **+0.000000** | **+0.001788** | **+0.007947** | **+0.000000** |

The exact 1024d float dense correction under the same protected-tail policy has macro MAP delta `+0.001904` and Recall delta `+0.008334`. The 256d INT4 payload therefore retains approximately 93.9% of the exact MAP gain and 95.4% of the exact Recall gain.

All seven rows pass the fixed gate:

- NDCG@10 delta equals zero;
- MRR@20 delta equals zero;
- Recall@100 does not decrease;
- MAP@100 does not decrease by more than `0.002`.

The aggregate matrix is in `runs/ii42-m1971-compact-dense-fusion-v1/m1971_256d_int4_full7_matrix.json`.

### 64d packed INT4 ultra-light candidate

| Dataset | Bytes/doc | MAP@100 delta | Recall@100 delta | Strict pass |
| --- | ---: | ---: | ---: | --- |
| arguana | 36 | +0.000169 | +0.004996 | yes |
| fiqa | 36 | +0.001917 | +0.015005 | yes |
| nfcorpus | 36 | -0.000619 | +0.000127 | yes |
| scidocs | 36 | +0.000872 | +0.008183 | yes |
| scifact | 36 | -0.000093 | -0.000667 | no |
| trec-covid | 36 | +0.003856 | +0.004194 | yes |
| webis-touche2020 | 36 | +0.000725 | +0.005365 | yes |
| **Macro** | **36** | **+0.000975** | **+0.005315** | **6/7** |

This version is useful but cannot be the default because SciFact Recall falls slightly. It remains within the earlier absolute `-0.002` tolerance, but it does not pass the stronger no-Recall-loss gate.

The aggregate matrix is in `runs/ii42-m1971-compact-dense-fusion-v1/m1971_64d_int4_full7_matrix.json`.

## Why this is not dense mimicry

Webis-Touche2020 is the decisive row. BM25 is substantially stronger than full dense on this surface. Adding exact dense correction at weight `0.5` and protect20 decreases Recall by `-0.002276`, while 64d INT4 increases it by `+0.005365` and 256d INT4 increases it by `+0.003146`.

The compressed signal is therefore not merely an approximation to the exact dense ranker. Low-dimensional projection and quantization can act as useful regularization when dense and lexical evidence disagree. The correct objective is marginal hybrid utility under protected-head constraints, not global dense overlap.

SciFact shows the opposite failure mode. Exact dense correction increases Recall by `+0.003333`, but 64d INT4 decreases it by `-0.000667`. Increasing capacity to 256d INT4 restores the full `+0.003333`. This separates a compression-capacity failure from a structurally harmful dense correction.

## 1-bit result

The high-dimensional sign-only family is not a universal solution.

| Webis mode | Bytes/doc | MAP@100 delta | Recall@100 delta |
| --- | ---: | ---: | ---: |
| 256d Hamming | 32 | -0.002681 | -0.007697 |
| 512d Hamming | 64 | -0.002865 | -0.008697 |
| 768d Hamming | 96 | -0.002001 | -0.003150 |
| 1024d Hamming | 128 | -0.002043 | -0.003028 |
| 256d scaled binary | 36 | -0.002218 | -0.007930 |
| 1024d scaled binary | 132 | -0.002591 | -0.003310 |

Increasing binary dimensions reduces some damage but never passes Webis. Sign-only codes discard magnitude information that remains useful when resolving lexical-dominant candidate tails. This family should stop unless a learned residual binary code introduces new information rather than another dimensional sweep.

## Cost result

The INT4 implementation uses physical nibble packing and a packed-byte lookup scorer. Quality is identical to explicit dequantization. Reported latency is the NumPy candidate-scoring kernel, not end-to-end database latency.

| Payload | Bytes/doc | Compression vs 1024d float32 | Full7 payload | Maximum score p95 |
| --- | ---: | ---: | ---: | ---: |
| 64d INT4 | 36 | 113.8x | 23.6 MB | 0.256 ms |
| 256d INT4 | 132 | 31.0x | 86.4 MB | 0.745 ms |
| 1024d float32 | 4096 | 1.0x | 2.68 GB | not used as product payload |

For the 8,841,823-document MSMARCO corpus, the estimated raw payload is approximately 303.6 MiB for 64d INT4 or 1.09 GiB for 256d INT4, before row headers, alignment, and database storage overhead.

## Relation to prior work

The architecture follows a well-supported cascade principle while using a different first-stage source.

- [Binary Passage Retriever](https://aclanthology.org/2021.acl-short.123/) jointly uses compact binary candidate generation and continuous reranking. It demonstrates that retrieval representations do not need to serve every stage at full precision.
- [Matryoshka Representation Learning](https://arxiv.org/abs/2205.13147) demonstrates that coarse-to-fine embedding dimensions can be trained into one representation. It is relevant to a later 64d/256d output-head training stage.
- [Binary Embedding-based Retrieval at Tencent](https://arxiv.org/abs/2302.08714) demonstrates task-agnostic embedding-to-embedding compression with configurable bit budgets. Our sign-only negative result indicates that a learned residual binary compressor would be required if binary storage is revisited.

This experiment does not reproduce any of those systems. Its new evidence is that M190+BM25 unified sparse recall needs only a small, protected-tail dense correction, and that 256d INT4 can retain almost all of the exact dense correction gain without a global ANN path.

## Provenance and limitations

1. This is an offline replay over exact document identities, not the native database/plugin path.
2. The offline fixed M190+BM25 baseline is close to, but not byte-identical with, existing native results. For example, FiQA differs by approximately `-0.001886` NDCG, `-0.000711` MAP, `+0.003241` Recall, and `-0.001128` MRR.
3. Only seven rows are currently materialized for this experiment. The result is not a complete BEIR15 claim.
4. The shared basis uses five document corpora without qrels. TREC-COVID and Webis are projection holdouts, but broader corpus transfer remains untested.
5. The measured p95 excludes sparse retrieval, database fetch, network, decoding outside the NumPy kernel, and encoder inference.
6. The dense root remains the existing PPLX model. A smaller payload does not imply a smaller text encoder.
7. ClearML is unavailable in the current NVIDIA runtime (`No module named 'clearml'`); immutable run directories, logs, input manifests, and result JSONs provide the experiment record.

## Next gate

Implement one native fixed-policy closure, without new model training:

1. Store the 256d packed INT4 code and one float row scale alongside each document in the existing unified index lifecycle.
2. Retrieve candidates through the existing M190+BM25 sparse path only.
3. Fetch and score compact payloads for the candidate union.
4. Preserve baseline ranks 1-20 and allow the compact score to reorder only the tail into ranks 21-100.
5. Use fixed weight `0.5`; do not tune per dataset.
6. Reproduce the seven-row matrix through the native path, then run the fixed policy on the remaining BEIR15 rows as their verified M190 surfaces become available.

Native acceptance requires:

- all available rows retain baseline NDCG@10 and MRR@20;
- no row loses Recall@100;
- no row loses more than `0.002` MAP@100;
- macro Recall and MAP remain positive;
- end-to-end p95 and payload bytes are measured in the database path.

Only after this gate passes should the project train a smaller encoder or output head. The first training target should reproduce the fixed 64d/256d shared projection with quantization-aware training, while preserving the existing sparse outputs. Retrieval loss should be added only after compiler parity is established.
