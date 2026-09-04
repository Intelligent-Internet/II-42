# M1972/M1973 native compact-dense closure report

Date: 2026-07-15

## Decision

The compact-dense correction is now validated as a real native database operation, not only an offline replay. A shared `256d` row-scaled packed INT4 payload can correct candidates returned by the frozen M190 latent-posting plus BM25 path while adding only `132 B/document` of raw payload.

The result is positive but is not yet a row-safe default:

- M1972 corrects the full candidate union. Its seven-row macro delta is MAP `+0.001806` and Recall `+0.009244`, with NDCG@10 and MRR@20 unchanged. Six of seven rows pass the strict no-regression gate.
- M1973 applies the same fixed weight only to the baseline top256. Its macro delta improves to MAP `+0.001987` and Recall `+0.009588`, while compact payload plus scoring p95 falls to approximately `6-10 ms`. Six of seven rows still pass.
- Webis-Touche2020 is the one strict failure. M1972 gives Recall `+0.004074` but MAP `-0.000104`; M1973 gives Recall `+0.000614` but MAP `-0.000315`.

The earlier M1971 report used a bounded `MAP >= -0.002` tolerance and therefore described all seven rows as passing. This report uses the stronger promotion rule `MAP >= 0` and does not call the Webis result row-safe.

The architecture remains worth continuing as a compact candidate-correction tier. It should not yet replace the frozen baseline by default, and a smaller encoder should not be trained from this target until the broad9 native closure is complete.

## Fixed native contract

```text
M190 latent postings top1000
       + native BM25 top1000
       -> native per-source min-max score fusion
       -> candidate union, usually 1,600-1,800 documents
       -> fetch ID-bound 256d packed INT4 payload
       -> preserve baseline ranks 1-20
       -> compact score weight 0.5 on the eligible tail
       -> top100
```

No ANN search is used. The compact code is fetched only after the unified sparse candidate path has returned document identities. Qrels are used only to compute metrics.

The artifact manifest binds:

- dense document and query path, size, mtime and SHA-256;
- shared PCA basis SHA-256;
- document and query ID corpus identity;
- packed code, row scale and projected-query artifact hashes;
- quantization schema and physical byte widths.

## Seven-row native matrix

| Dataset | M1972 Recall delta | M1972 MAP delta | M1973 Recall delta | M1973 MAP delta | M1973 compact extra p95 |
| --- | ---: | ---: | ---: | ---: | ---: |
| ArguAna | +0.003569 | +0.000272 | +0.003569 | +0.000292 | 8.96 ms |
| FiQA | +0.018160 | +0.002263 | +0.024372 | +0.002628 | 8.83 ms |
| NFCorpus | +0.009571 | +0.001787 | +0.007160 | +0.001800 | 6.26 ms |
| SciDocs | +0.014700 | +0.001578 | +0.016383 | +0.001788 | 10.06 ms |
| SciFact | +0.006667 | +0.000117 | +0.006667 | +0.000117 | 8.48 ms |
| TREC-COVID | +0.007969 | +0.006730 | +0.008353 | +0.007596 | 9.51 ms |
| Webis-Touche2020 | +0.004074 | -0.000104 | +0.000614 | -0.000315 | 10.49 ms |
| **Macro** | **+0.009244** | **+0.001806** | **+0.009588** | **+0.001987** | **6-10 ms** |

NDCG@10 and MRR@20 deltas are exactly zero for every row because ranks 1-20 are preserved by construction. Candidate upper bound is unchanged because correction does not add or remove candidates.

The aggregate artifacts are:

- `runs/ii42-m1972-native-compact-dense-v1/m1972_full7_matrix.json`
- `runs/ii42-m1973-native-compact-top256-v1/m1973_full7_matrix.json`

## What M1973 establishes

Top256 was a pre-registered structural test, not a broad grid search. It asks whether compact dense should be a local correction layer rather than a full-union ranker.

The result is informative:

1. Macro quality improves rather than merely trading quality for latency.
2. FiQA Recall gain increases from `+0.018160` to `+0.024372`, and SciDocs increases from `+0.014700` to `+0.016383`. Deep candidate movement was partly noise.
3. Compact payload plus scoring p95 falls from `15-36 ms` under M1972 to `6-10 ms` under M1973.
4. Webis remains unsafe, so its failure is not explained solely by allowing documents below rank 256 to cross the top100 boundary.

No top128/top512 or weight sweep is justified on the same seven rows. That would turn a structural diagnosis into holdout tuning.

## Webis failure analysis

At query level, the M1972 correction improves 21 Webis queries, harms 25 and leaves 3 unchanged. Several harmed queries lose relevant documents from top100; others retain Recall but reduce average precision by reordering relevant documents later in the protected tail.

The fixed compact signal is therefore useful on average but conflicts with lexical-dominant ordering for a substantial fraction of Webis queries. Protecting top20 guarantees head metrics, but it cannot guarantee AP or Recall at the top100 boundary.

This is not evidence that the compact representation is useless. It is evidence that unconditional activation is too strong. Any future query-time activation policy must use inference-visible agreement or risk signals and must be selected outside Webis, then validated on unseen rows. Qrels-based per-query gating or dataset-specific thresholds are prohibited.

## Cost and engine separation

The raw compact payload is `132 B/document`. PostgreSQL table storage is larger because each row also carries a text primary key, tuple metadata and index overhead. Representative published sizes are:

- FiQA, 57,638 documents: approximately 15.3 MB;
- TREC-COVID, 171,331 documents: approximately 45.1 MB;
- Webis-Touche2020, 382,545 documents: approximately 141.6 MB;
- Quora, 522,931 documents: approximately 136.3 MB;
- CQADupStack, 457,199 documents: approximately 126.1 MB.

The candidate layer remains much more expensive than compact correction on large rows. Warm M1973 p95 candidate latency is approximately 192 ms on TREC-COVID and 315 ms on Webis, while compact payload plus scoring is under 11 ms. Quora/CQ 100-query smoke has candidate p95 above one second, but compact correction remains around 10 ms.

These are separate engineering problems. Further quantizing the compact payload cannot fix sparse candidate SQL latency. Candidate retrieval needs its own query-plan, posting traversal and cache analysis after quality closure.

## Broad9 extension

Quora and CQADupStack were added after the seven-row matrix because verified local M190 posting surfaces already existed.

The dense source document order differs from M190 `doc_ord` on Quora: the dense file is numeric, while the M190 map is lexicographic. Since publication binds every compact row by `doc_id`, physical cross-file order is not required. The materializer now supports two explicit validation modes:

- `order`: IDs and order must match exactly;
- `set`: unique IDs, count and full corpus membership must match, while order may differ.

The default remains `order`. Broad9 documents use `set`; queries remain `order`. Different corpus membership still fails closed.

Both artifacts passed manifest and PostgreSQL identity validation. The fixed top256 policy passed 100-query path smokes:

- Quora: Recall `+0.001429`, MAP `+0.000212`;
- CQADupStack: Recall `+0.007976`, MAP `+0.001105`.

These samples are not promotion evidence. Full Quora and CQADupStack evaluation is running sequentially in tmux session `ii42_m1973_broad9_full`; expected wall time from smoke latency is approximately four hours.

## Next decision gate

1. Complete full Quora and CQADupStack native evaluation with the unchanged M1973 policy.
2. If both full rows are positive, retain M1973 as the compact engineering candidate and design one inference-visible query activation audit on development rows, with Webis and at least one additional row held out.
3. If either full row repeats the Webis MAP/Recall conflict, reject unconditional compact correction as a universal default. Keep it as a bounded optional tier and study whether harm is observable before training any gate.
4. Do not reopen correction-depth or scalar-weight search on these rows.
5. Do not train a smaller encoder until the fixed compact target is stable enough to define its output contract.
6. Optimize native candidate retrieval separately; do not attribute candidate SQL latency to the 256d INT4 representation.
