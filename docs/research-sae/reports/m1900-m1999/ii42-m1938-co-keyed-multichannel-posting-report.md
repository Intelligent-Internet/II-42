# II-42 M1938 Co-Keyed Multi-Channel Posting Report

Date: 2026-07-13

Decision: **close the conservative co-key oracle at the SciDocs canary. Do
not expand it to Quora or TREC-COVID, and do not modify the native payload
format.**

## Question Answered

M1937 showed that lexical coverage cannot safely replace input-aligned
semantic impacts. M1938 therefore kept both score channels intact and tested
whether collision-free lexical and semantic evidence could share one physical
`(term, document)` key.

For a shared key, the score remains exactly:

```text
lexical_query * lexical_document
  + semantic_scale * semantic_query * semantic_document
```

There are no lexical-semantic cross terms. The experiment changes only the
physical layout and preserves every impact value by construction.

## Conservative Mapping

The frozen Granite tokenizer exposed 17,864 semantic dimensions that could be
mapped to a single native lexical token under decode, word-boundary, and
round-trip checks. No ambiguous lexical collision was admitted.

| Measure | Value |
| --- | ---: |
| Decoded native-token matches | 35,193 |
| Single-token round-trip matches | 22,031 |
| Safely mapped semantic dimensions | 17,864 |
| Ambiguous lexical tokens admitted | 0 |
| Mappable semantic postings, b1 | 60.75% |
| Mappable semantic postings, nested b1.125 | 60.76% |

## SciDocs Result

| Budget | Disjoint entries | Co-key entries | Reduction | Disjoint touches | Channel-aware touches | Naive-union touches |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| b1 | 5,337,150 | 4,736,281 | 11.26% | 150,168 | 145,892 | 167,937 |
| nested b1.125 | 5,670,722 | 5,036,478 | 11.18% | 160,280 | 155,803 | 178,074 |

The nested `b1.125` candidate has 5.63% fewer physical entries than the
current disjoint `b1` baseline, so the storage gate passes. Its channel-aware
mean traversal remains 3.75% above disjoint `b1`, so the mandatory touch gate
fails. A naive union list would be worse because queries would also traverse
the inactive channel's postings.

## Native Engine Feasibility

The current `UBMX` research payload does not implement this layout:

- its exporter assigns lexical and semantic evidence to disjoint global
  dimension namespaces;
- every global dimension has one `dim_sources` value, either BM25 or SAE;
- each document-vector pair stores one impact;
- candidate generation and rerank keep BM25 and SAE counters and scores as
  separate sources.

Supporting exact co-key multi-channel postings would therefore require a new
payload version, builder representation, candidate iterator, scorer, trace
contract, and compatibility tests. It is not an existing low-risk engine
option. Since the canary failed its predeclared traversal gate, that invasive
work is not authorized by M1938.

## Interpretation

M1938 provides a useful structural result, but not a product promotion:

- lexical and semantic impacts have enough key overlap to remove about 11% of
  physical entries without changing scores;
- exact score preservation is possible only when both channels remain
  independently addressable;
- storage deduplication alone does not eliminate the extra query work of the
  `b1.125` semantic support;
- the near-pass is an engine-layout opportunity, not evidence for another
  model loss or tail selector.

The experiment stops here by contract. Relaxing tokenizer equivalence,
changing payload layout before the touch gate passes, or expanding to broader
rows would turn a failed canary into an unbounded engineering search.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1938-co-keyed-multichannel-posting-contract.md`
- Auditor: `scripts/audit_m1938_cokey_multichannel_postings.py`
- Matrix: `runs/m1938_cokey_multichannel_v1/scidocs-canary/matrix.json`
- Generated report:
  `runs/m1938_cokey_multichannel_v1/scidocs-canary/report.md`
- Log: `runs/m1938_cokey_multichannel_v1/scidocs-canary/run.log`
