# M1560-M1572 Unified Inverted Source Final Report

## Executive Decision

**Close the current dense-root post-hoc source-conversion branch.**

The branch did not convert M1549's dense-plus-lexical capability into a
bounded single-hop unified inverted source. It did produce a complete causal
map: the score/tail codec is not the bottleneck; dense-neighbor admission and
posting selectivity cannot be satisfied together by any tested frozen-dense
route, exact-term, product-code, or LSH source.

This is not a recommendation to deploy ANN plus BM25 as the research answer.
M1549 remains only the capability oracle. A future pure-posting attempt must
be a new retrieval-native discrete model, not another adapter around the same
frozen dense output.

## Comparable FiQA Source Frontier

| Stage/source | O@100 | O@256 | Recall@100 | CUB | Mean reads | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| M1565 route1000 | 0.808503 | 0.702311 | 0.683089 | 0.822706 | 0.021159x | useful bounded baseline |
| M1565 route2048 | 0.906235 | 0.842683 | 0.720558 | 0.866777 | 0.045015x | close quality, not dense-equivalent |
| M1566 full-term oracle | 0.992485 | 0.985448 | 0.731927 | 0.905573 | 2.552668x | capacity, prohibitive cost |
| M1566 unsup term15 oracle | 0.859645 | 0.763847 | 0.714375 | 0.868109 | 0.044619x | cheap, insufficient capacity |
| M1567 DF-term oracle | 0.835216 | 0.730788 | 0.709367 | 0.856820 | 0.026671x | global DF removes useful edges |
| M1568 heldout teacher term15 oracle | 0.944023 | 0.905302 | 0.729173 | 0.886827 | 0.463157x | stable signal, failed cost/O@100 |
| M1569 exact impact access | 0.944023 | 0.905302 | 0.729173 | 0.886827 | 0.463157x | exact pruning ineffective |
| M1570 composite deterministic | 0.629198 | 0.568269 | 0.517030 | 0.601844 | 0.021791x | fine cells, unobservable ordering |
| M1571 LSH first hit | 0.077716 | 0.067570 | 0.081157 | 0.084180 | 0.021920x | hot-bucket integrity failure |
| M1572 LSH multi-hit | 0.174907 | 0.141409 | 0.253876 | 0.259486 | 0.299993x | broad weak evidence |

No deployable qrels-free source reaches O@100 `>=0.95`, O@256 `>=0.90`,
and reads `<=0.30x` together. M1568 comes closest on geometry but fails cost;
M1565 route2048 is the strongest practical deterministic source but remains
well below the dense-overlap floor.

## What Was Learned

### Neighborhood-preserving routes are necessary but not sufficient

M1560-M1564 show that document graph replication, query-derived route labels,
protected residual routes, and SOAR decorrelation can each improve a local
geometry diagnostic. None amplifies to dense-equivalent top100/top256 access
at a practical fixed budget. M1565 confirms that much larger candidate sets,
not a missing scorer, close the route gap.

### Exact lexical membership contains capacity but has the wrong cost shape

M1566 proves route plus all exact term membership can recover dense geometry.
M1567-M1569 then isolate why it cannot be compressed: whole-term DF deletion
removes useful occurrences; document-level dense-teacher admission restores
them by selecting common terms; those common-term impacts are too flat for
exact impact pruning. A selector cannot repair this source/cost conflict.

### Fine latent cells do not solve query observability

M1570's product cells have excellent entropy, maximum DF below 0.14%, and an
oracle capable of nearly perfect dense coverage. The deployable product score
cannot identify those cells. This is the same oracle-to-query observability
gap found earlier in M1502, M1245, M1556, and M1561/M1562, now demonstrated at
the source level without qrels.

### Overlapping hashes trade quantization error for non-selective collisions

M1571 covers more than 99% of dense neighbors within radius two, but hot
buckets make first-hit candidates useless. M1572 spends the full read budget
on cross-table evidence and still cannot separate neighbors from the broad
collision background. More reads or a new collision weight would repeat a
known corpus-scan failure.

## Why More Training Is Not Authorized

Every attempted training target must first exist as a bounded source oracle.
The only strong oracle targets in this branch are either query-aware,
high-cost, or unobservable from deployable query features. Training deeper
would optimize toward a target that already violates the product contract.

The failures are therefore not evidence of too few epochs:

- M1568/M1569 fail before a selector exists;
- M1570 deterministic cell ordering is far below its restricted training
  bridge;
- M1571/M1572 fail with fixed mathematical hash functions and full read caps;
- prior M1502/M1522/M1552 work already shows that free oracle factors or
  lexical actions do not become text-transcodable through a larger head.

## Product Boundary

The following shapes remain honest:

1. **Quality oracle:** M1549, which uses native dense and lexical access. It
   proves the protected-tail capability but is not the desired pure posting
   product.
2. **Compact unified baseline:** P1/M549U plus token-aware lexical residual and
   protected-tail policy. It is one native posting path and gains materially
   over P1, but it remains far below dense in absolute quality.
3. **Best bounded semantic source:** M1565 route2048 plus a compact tail codec.
   It is close to dense retrieval quality on FiQA but does not preserve enough
   dense membership to inherit M1549's top99 guarantee.

Do not relabel any of these as a solved M1549 conversion.

## Only Justified New Research Program

If one encoder and one pure inverted posting map remain mandatory, start a new
milestone family rather than M1573:

**Retrieval-native balanced discrete backbone.**

- initialize from the dense root, but jointly train the final representation
  rather than freezing dense geometry and attaching a compiler;
- learn document/query posting keys on a large corpus with qrels-free dense
  neighborhood distillation;
- include key-balance, inter-key decorrelation, maximum-DF/FLOPS, and fixed
  posting-budget constraints inside the objective;
- train candidate-set topK/listwise preservation, not reconstruction or score
  KL alone;
- use cross-corpus heldout source-capacity gates before BEIR qrels;
- require native posting reads, candidate overlap, CUB, and row-safe retrieval
  together at every expansion stage.

This route is high risk and materially different from P1. It still starts from
the dense model's knowledge, but it accepts that the geometry itself must move
to become indexable. The alternative is to relax the pure-posting constraint;
there is no evidence-backed post-hoc shortcut left between those choices.

## Final Status

- All M1560-M1572 stages are complete and independently committed.
- No Spark process remains.
- No selector, encoder training, canary expansion, or native promotion is
  authorized from this branch.
- M1549 is retained as capability oracle, not product completion.
- The next action requires a new reviewed goal for a retrieval-native balanced
  discrete backbone or an explicit relaxation of the pure-posting constraint.
