# II-42 M1932 Cross-Corpus Semantic Budget Report

Date: 2026-07-13

Decision: **retain M1931 and stop before residual-model training.**

## Question Answered

M1932 tested whether M1931's exact semantic posting budget could be allocated
better than document-local top impact by using qrels-free cross-corpus query
utility and target-corpus document frequency. M1914 impacts, the M1931
`rms_m4` query calibration, lexical postings, score function, and total
semantic posting count remained frozen.

This was the required observability gate before training a residual atom
selector. A selector was not trained because its proposed teacher did not
generalize.

## Full Four-Corpus Result

| Policy | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB@1000 | maxDF | head 1% |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| impact | **0.458584** | **0.356845** | **0.722599** | **0.493992** | **0.857603** | 0.710643 | 0.439580 |
| query RMS^0.25 x IDF | 0.447393 | 0.346877 | 0.710743 | 0.484303 | 0.849758 | 0.404509 | 0.361735 |
| query RMS^0.25 x IDF^1.25 | 0.437424 | 0.338434 | 0.701454 | 0.472770 | 0.844979 | 0.310336 | 0.330267 |
| query RMS^0.25 x IDF^1.5 | 0.436290 | 0.337124 | 0.698373 | 0.468590 | 0.841792 | **0.239582** | **0.306414** |

DF-aware allocation substantially reduced high-frequency posting
concentration, but every such reduction lost retrieval quality. The strongest
cost reduction cut macro maxDF by 66% while losing 0.0242 Recall@100 and
0.0223 NDCG@10.

## Generalization Gate

Nested leave-one-dataset-out selection chose the unchanged `impact` policy on
every fold. Heldout macro metrics therefore reproduced M1931 exactly and did
not meet the required `+0.002` Recall/CUB gain.

| Held out | Selected policy | Recall delta | CUB delta |
| --- | --- | ---: | ---: |
| FiQA | impact | 0.000000 | 0.000000 |
| ArguAna | impact | 0.000000 | 0.000000 |
| NFCorpus | impact | 0.000000 | 0.000000 |
| SciFact | impact | 0.000000 | 0.000000 |

The baseline reproduction gate passed with zero metric delta. This makes the
negative result attributable to allocation policy rather than stale inputs or
score replay.

## Conclusion

Static cross-corpus atom utility cannot safely solve the current fixed-budget
problem. Query-useful atoms differ too much across FiQA, ArguAna, NFCorpus,
and SciFact; suppressing high-DF atoms often removes exactly the semantic
coverage required by another corpus. Training a neural selector on this
teacher would encode a failed policy rather than fix it.

M1932 closes the following family:

- global query-frequency priors;
- global IDF/DF penalties at the same posting count;
- another selector depth or loss trained to imitate those priors.

The next valid diagnostic is budget capacity itself, which M1933 evaluates
without changing support scores or introducing a learned component.

## Artifacts

- Contract: `docs/research-sae/reports/m1900-m1999/ii42-m1932-cross-corpus-semantic-budget-contract.md`
- Auditor: `scripts/audit_m1932_cross_corpus_semantic_budget.py`
- Matrix: `runs/m1932_cross_corpus_semantic_budget_v1/matrix.json`
- Generated report: `runs/m1932_cross_corpus_semantic_budget_v1/report.md`
