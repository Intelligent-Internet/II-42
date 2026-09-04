# M1516-M1516C SPLARE DF-Budget Final Report

## Decision

Stop the post-hoc DF-budget source-construction branch. Do not authorize
M1517 hard-cap distillation.

The locked `DF15/D128/Q32` configuration preserved or improved retrieval
quality, but failed the independent ArguAna cost gate. The remaining problem
is cumulative fanout across many medium-frequency atoms, not a small set of
universal head atoms.

## Experiment sequence

M1516 audited the stored M1510 retrieval-trained SPLARE postings before any
new training. It used only global corpus DF, learned sparse weights, and fixed
global row budgets. Qrels were read only for final evaluation.

M1516B performed one mechanism-localization audit because M1516 changed both
DF cap and row budget. With budgets fixed at document K=128 and query K=32,
the global DF=15% candidate passed NFCorpus, SciFact, and FiQA. That result was
post-hoc and could not authorize training without a new surface.

M1516C froze `DF15/D128/Q32`, no IDF, before encoding complete official
ArguAna. No threshold, weight, fallback, or dataset-specific setting changed
after seeing the heldout row.

## Locked-candidate matrix

| Dataset | Config | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB | Touch | Mean query ms |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| NFCorpus | M1510 D400/Q40 | 0.36954 | 0.18810 | 0.33558 | 0.58855 | 0.65327 | 1.00000 | 1.210 |
| NFCorpus | DF15 D128/Q32 | 0.34811 | 0.17692 | 0.32029 | 0.54229 | 0.59500 | 0.39618 | 0.323 |
| SciFact | M1510 D400/Q40 | 0.69560 | 0.65445 | 0.95333 | 0.66338 | 0.99000 | 1.00000 | 1.826 |
| SciFact | DF15 D128/Q32 | 0.67912 | 0.64110 | 0.91333 | 0.64608 | 0.98333 | 0.49091 | 0.476 |
| FiQA | M1510 D400/Q40 | 0.33381 | 0.27595 | 0.63631 | 0.40532 | 0.83694 | 1.00000 | 17.764 |
| FiQA | DF15 D128/Q32 | 0.31713 | 0.26141 | 0.63306 | 0.38328 | 0.85204 | 0.47110 | 4.160 |
| ArguAna | M1510 D400/Q40 | 0.31705 | 0.22089 | 0.93790 | 0.21707 | 0.99500 | 1.00000 | 2.046 |
| ArguAna | DF15 D128/Q32 | 0.33928 | 0.23408 | 0.95075 | 0.23083 | 0.99429 | 0.63644 | 0.937 |

Baseline reproduction was exact on all four datasets. The maximum absolute
metric error against the stored M1510 baseline was zero.

## What the heldout result proves

On ArguAna, the locked candidate improved every retrieval metric:

- NDCG@10: +7.01%;
- MAP@100: +5.97%;
- Recall@100: +1.37%;
- MRR@20: +6.34%;
- CUB retained 99.93% of baseline.

The same configuration reduced mean query time from 2.046 ms to 0.937 ms and
reduced mean document postings from 379.46 to 127.91. This is real evidence
that much of the original SPLARE output is unnecessary or harmful.

It nevertheless touched 63.64% of the ArguAna corpus, above the predeclared
50% gate. The index's maximum atom DF was already only 13.95%, so the failure
cannot be repaired by deleting one or two universal atoms. Many individually
acceptable atoms jointly cover most documents.

## Hard stops

- Do not promote `DF15/D128/Q32` as a runtime default.
- Do not distil the post-hoc cap into M1517.
- Do not run another BEIR cap interpolation or per-dataset threshold search.
- Do not interpret ArguAna's quality gain as a pass while ignoring touch.

The result closes the simple TopK/IDF/hard-DF pruning hypothesis. It does not
close retrieval-trained sparse representations.

## Next independent hypothesis

The next justified experiment is M1518: make corpus selectivity part of the
retrieval objective rather than prune it after training. The primary control
is DF-FLOPS from
[An Alternative to FLOPS Regularization to Effectively Productionize SPLADE-Doc](https://arxiv.org/abs/2505.15070).
That work directly distinguishes within-vector sparsity from corpus document
frequency sparsity, which is the failure observed here.

M1518 should:

1. start from the retrieval-trained M1510 adapter/source, not generic BERT;
2. train adapter/output components with ranking supervision plus DF-FLOPS;
3. estimate corpus DF periodically from a qrels-free validation corpus;
4. preserve fixed query/document TopK budgets;
5. gate first on MS MARCO heldout ranking and corpus fanout;
6. expand to complete NFCorpus, SciFact, FiQA, and ArguAna only after the
   heldout gate passes.

M1518 must stop if the regularizer falls without reducing measured DF/touch,
if ranking quality is traded away, or if success requires BEIR-specific
thresholds. This is a new source/objective hypothesis, not continuation of
the failed hard-cap branch.
