# M1567 DF-Bounded Background Source Report

## Decision

**Stop whole-term DF filtering.**

The analytically derived background channel easily satisfies the posting-read
budget, but its dense-teacher oracle is weaker than the M1566 term15 oracle.
Useful full-source capacity therefore depends on selectively retaining some
occurrences of high-DF terms, not simply deleting those terms globally.

Do not search another DF threshold. The only remaining HI2-style source
question is whether document-level term admission can retain those occurrences
and transfer to held-out queries at the same bounded posting cost.

## Surface

- Official FiQA: 57,638 documents and 648 queries.
- Frozen route1000 source with exact M1565 parity (`0.0` delta).
- Fixed lexical DF cap: `(0.30 - 0.05) / 32 = 0.0078125`.
- 73,718 indexed terms and 1,039,564 lexical posting edges.
- 18.0361 lexical postings per document on average.
- Actual maximum DF ratio: `0.007807`.
- Qrels-free candidate surface SHA-256:
  `bc8aebe2a326a2a197ec64863badd2c74b92e83be375b52c977167cc2f9008d2`.
- ClearML task: `48da4e22dcb7425d887674643c40d580`.
- Runtime: 29.8 seconds.

## Result

| Variant | O@10 | O@100 | O@256 | Recall@100 | NDCG@10 | MAP@100 | MRR@20 | CUB | Reads |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| route1000 | 0.921759 | 0.808503 | 0.702311 | 0.683089 | 0.383019 | 0.322550 | 0.468258 | 0.822706 | 0.021159x |
| route + DF terms | 0.941358 | 0.831173 | 0.725682 | 0.706666 | 0.394592 | 0.332574 | 0.480059 | 0.853090 | 0.026671x |
| route + DF-term oracle | 0.944290 | 0.835216 | 0.730788 | 0.709367 | 0.396469 | 0.333821 | 0.480969 | 0.856820 | 0.026671x |

For comparison, M1566 route+term15 oracle reached O@100 `0.859645` and
O@256 `0.763847`; the costly full-term oracle reached `0.992485` and
`0.985448`.

## Interpretation

The DF source is efficient by a wide margin, but the oracle adds only
`+0.026713` O@100 over route1000. Its gap from the full-term oracle is
`-0.157269` O@100 and `-0.254660` O@256. This cannot be repaired by query-term
scoring because the removed document-term edges are absent before ranking.

The result also explains why unsupervised term15 is slightly stronger: it may
retain selected occurrences of otherwise common terms. A global background
channel removes every occurrence and therefore destroys that document-level
selectivity.

## Next Gate

Run one qrels-free document-level term-admission oracle with exactly 15 terms
per document:

1. accumulate dense-top256 query mass only on exact query/document term
   overlaps;
2. select document terms from training-query mass, with BM25 fallback;
3. evaluate the frozen selected postings on held-out queries across three
   deterministic splits;
4. compare directly with unsupervised term15 on the same held-out rows.

This is a source-transcodability test, not a deployable model. If the
transductive ceiling or held-out transfer fails, do not train a BERT/MLP term
selector. If it passes, the selected postings become a fixed target for
cross-corpus selector distillation.
