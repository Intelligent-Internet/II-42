# Unified Native Payload Simulator Report

Date: 2026-05-14

## Scope

This phase turns the previous BM25+SAE candidate-union result into a more
native physical simulator. The simulator keeps two candidate heads in one
payload accounting model:

```text
BM25 token impact heads
+ SAE atom impact heads
  -> one candidate union
  -> one normalized BM25+SAE rerank path
```

This is still a simulator, not the final PostgreSQL binary payload format. The
new value is that BM25 token posting cost is now measured explicitly instead of
only assuming an already available BM25 top-k list.

The Python runtime in this artifact is diagnostic only. It still recomputes
some full-score helper structures so latency should not be read as native C or
PostgreSQL latency. The meaningful physical counters are candidate documents,
SAE posting reads, BM25 posting reads, and score visits.

Artifacts:

```text
scripts/research_sae_unified_native_payload_simulator.py
results/sae/text-atoms/full15-shared-dense-budget16-w0p04-unified-native-payload-sim/
```

## Exact Score-Surface Reference

The reference below scores all documents with the same normalized BM25+SAE
rerank function used for bounded candidates. It is not an oracle; bounded
candidate pruning can occasionally remove high-scoring distractors and slightly
beat the full-scan ranking on recall.

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| Full BM25+SAE score surface | 0.8324 | 0.8291 | 0.7225 | 0.6913 |

## Main Results

`bt0` means the BM25 side uses the full BM25 ranking as a candidate source. It
is a quality baseline, but it is not the desired native physical shape because
it implies reading all query-term BM25 postings first.

`bt64`, `bt128`, and `bt256` bound each BM25 query token to its top-N impact
postings before choosing BM25 candidates. This is closer to the target native
payload layout.

| Config | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Candidates | SAE Posts | BM25 Posts |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `d8_p128_bm25200_bt0` | 0.8325 | 0.8291 | 0.7223 | 0.6913 | 739.1 | 781.7 | 10185.0 |
| `d8_p128_bm25200_bt256` | 0.8322 | 0.8291 | 0.7224 | 0.6900 | 738.8 | 781.7 | 1927.4 |
| `d8_p128_bm25200_bt128` | 0.8317 | 0.8291 | 0.7223 | 0.6894 | 738.6 | 781.7 | 1158.8 |
| `d8_p128_bm25200_bt64` | 0.8314 | 0.8298 | 0.7223 | 0.6872 | 735.5 | 781.7 | 677.5 |
| `d8_p128_bm25200_bt32` | 0.8271 | 0.8298 | 0.7219 | 0.6812 | 722.0 | 781.7 | 380.5 |
| `d16_p128_bm25200_bt128` | 0.8327 | 0.8291 | 0.7223 | 0.6909 | 1061.8 | 1443.7 | 1158.8 |
| `d32_p128_bm25200_bt64` | 0.8327 | 0.8291 | 0.7220 | 0.6905 | 1391.0 | 2263.6 | 677.5 |

## Interpretation

The balanced native simulator frontier is now:

```text
d8_p128_bm25200_bt64
```

It keeps the previous balanced candidate count around 735 documents/query while
reducing BM25 posting reads from about 10,185/query to about 678/query. That is
roughly a 15x reduction in BM25 posting reads for about 0.001 Recall@100 loss
versus the full-BM25-candidate baseline.

The safer quality tier is:

```text
d8_p128_bm25200_bt128
```

It still reduces BM25 posting reads by about 8.8x and gives a slightly smaller
Recall@100 loss. The higher-SAE-budget tier:

```text
d16_p128_bm25200_bt128
```

matches or slightly beats the full score-surface reference, but it opens about
1,062 candidate documents/query. That is useful as a quality-safety mode, not
as the first latency target.

`bt32` is too aggressive for the balanced `d8_p128` path. It remains viable
only when SAE candidate coverage is much larger, such as `d32_p128`, which
opens too many documents for the first product target.

## Engineering Decision

This result points to engineering integration before more training:

1. Implement a read-only payload generation path with two posting namespaces:
   BM25 token ids and SAE atom ids.
2. Store per-token BM25 impact heads in the same block/directory abstraction as
   SAE atom heads.
3. Keep the candidate generator source-aware for diagnostics, but keep final
   rerank on the same normalized BM25+SAE score surface.
4. Add C reader diagnostics for BM25 posting rows, SAE posting rows, candidate
   overlap, score visits, and final top-k parity.
5. Port the generated payload into the PostgreSQL-resident read-only generation
   after C/Python parity is stable.

Training should wait until native payload parity shows a real representation
miss. The current bottleneck is physical traversal and payload layout, not
query-atom representation quality.

## Next Validation

The next validation matrix should compare:

| Path | Required Check |
| --- | --- |
| Python simulator | Current full15 result, source counters, selected configs |
| C payload reader | Same candidates and rankings for selected configs |
| PostgreSQL read-only payload | Same top-k and diagnostics as C reader |
| Larger corpus | Memory footprint and p95 latency at policy/arxiv/pubmed-like scale |

The first C target should implement:

```text
d8_p128_bm25200_bt64
d8_p128_bm25200_bt128
d16_p128_bm25200_bt128
```
