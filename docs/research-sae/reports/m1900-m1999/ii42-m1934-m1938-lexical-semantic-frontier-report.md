# II-42 M1934-M1938 Lexical-Semantic Frontier Report

Date: 2026-07-13

Decision: **retain `b1` as the lower-cost product baseline and `b1.125` as a
validated fixed-quality candidate. Close static DF, token-provenance,
cost-neutral replacement, and conservative co-key continuation. Do not start
neural training from these failed teachers.**

## Research Question

This sequence tested the product hypothesis established by M1930-M1933:

> Keep exact BM25 as the lexical substrate, then spend a bounded learned
> sparse budget only on semantic evidence that complements it, with both
> channels queried through one additive inverted-index path.

The sequence deliberately separated four questions:

1. Does a globally selected semantic budget generalize?
2. Can its added support be isolated by a qrels-free static feature?
3. Can lexical evidence fund semantic expansion at fixed posting count?
4. Can exact lexical and semantic impacts share physical posting keys?

## What Passed

### Fixed-capacity transfer

M1933 selected `b1.125` on FiQA, ArguAna, NFCorpus, and SciFact. M1934 applied
it unchanged to SciDocs, Quora, and TREC-COVID. On those unseen rows, every
primary macro metric improved:

| Budget | NDCG@10 | MAP@100 | R@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| b1 | 0.596009 | 0.358258 | 0.538584 | 0.701456 | 0.766375 |
| b1.125 | **0.599866** | **0.360373** | **0.539188** | **0.706068** | **0.768689** |

The same candidate reproduced through the exact PostgreSQL native path on all
1,000 SciDocs queries. Its normalized relation size grew 6.18%, mean latency
7.40%, and p95 latency 8.75%.

M1935 rebuilt the candidate as a strictly nested support expansion. The
quality signal remained, proving that the gain is a capacity effect rather
than an accidental support swap.

### Physical key overlap

M1938 found that 60.75% of SciDocs semantic postings can be mapped to lexical
keys under a conservative collision-free tokenizer rule. An exact
multi-channel co-key layout would reduce nested `b1.125` from 5,670,722
disjoint channel entries to 5,036,478 physical keys. That is 5.63% below the
current disjoint `b1` entry count while preserving exact score contributions.

This is a real storage-layout signal. It is not yet a native engine result and
does not satisfy the traversal gate.

## What Failed

| Experiment | Positive observation | Mandatory failure | Conclusion |
| --- | --- | --- | --- |
| M1935 DF tail | DF<=0.10 retained 95.97% Recall gain at 49.62% added touches | Required 88.98% of added postings and retained only 31.41% MRR gain | Useful support is not a cheap low-DF slice |
| M1936 provenance | Input-aligned tail was cheap; learned expansion retained all CUB gain | Neither source met quality and <=75% cost gates | Token provenance explains function, not safe selection |
| M1937 fixed-count swap | Improved NDCG, MAP, MRR, and CUB at exact b1 posting count | Recall fell by 0.001550 | BM25 does not make input-aligned semantic impacts redundant |
| M1938 co-key oracle | Exact b1.125 physical entries fell below disjoint b1 | Channel-aware touches remained 3.75% above b1 | Layout helps storage but does not close traversal cost |

These failures are complementary. DF does not identify utility, source type
does not identify replaceability, and physical key overlap does not remove all
channel work. A classifier trained to imitate any one of these failed rules
would inherit the same limitation.

## Current Frontier

| Operating point | Quality status | Cost status | Product status |
| --- | --- | --- | --- |
| disjoint b1 | Frozen lower-cost baseline | Current reference | Keep default |
| disjoint b1.125 | Cross-corpus and native quality pass | +6.25% entries and higher traversal | Explicit quality candidate |
| co-key b1.125 oracle | Exact b1.125 score in principle | -5.63% entries but +3.75% raw touches vs disjoint b1 | Structural diagnostic only |

The strongest defensible result is therefore not a new trained model. It is a
cleaner product Pareto frontier:

- `b1` when cost is the binding constraint;
- `b1.125` when the measured quality gain justifies about 6-9% additional
  native cost;
- no qrels-free static publisher has yet preserved the `b1.125` gain at the
  `b1` traversal budget.

## Training Decision

No ClearML training job is authorized by this sequence. The deterministic
publisher already realizes the validated `b1.125` target exactly. The tested
DF and provenance sources do not provide a safe supervision target, and the
co-key result concerns index layout rather than encoder capacity.

Training should resume only after a new source passes, in this order:

1. qrels-free observability on a small canary;
2. exact native replay with both quality and traversal gates;
3. frozen-policy transfer to unseen corpora;
4. only then a model trained to reproduce that proven policy, tracked through
   ClearML.

This stop is evidence-based, not a resource or training-depth shortcut. More
steps on a failed teacher would improve imitation without resolving the
quality/generalization/cost conflict.

## Artifacts

- `docs/research-sae/reports/m1900-m1999/ii42-m1934-fixed-budget-unseen-transfer-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1935-incremental-tail-attribution-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1936-semantic-tail-provenance-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1937-cost-neutral-provenance-swap-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1938-co-keyed-multichannel-posting-report.md`
