# M658 Near-Miss Checkpoint Audit Report

## Summary

M658 changed the training runner so it can save the best rejected checkpoint for
audit instead of falling back to epoch0. This does not promote the checkpoint:
the final decision is still forced to fail if the selected checkpoint did not
pass the dev gate.

Result: the best rejected checkpoint is M654Z epoch 1. It is a useful
near-miss diagnostic, not a candidate.

## Selected Checkpoint

Run:

- JSON: `runs/m658_near_miss_checkpoint_v1/m658_shared15_swap1_nearmiss_seed6545/m658_shared15_swap1_nearmiss_seed6545.json`
- checkpoint: `runs/m658_near_miss_checkpoint_v1/m658_shared15_swap1_nearmiss_seed6545/m658_shared15_swap1_nearmiss_seed6545.query_compiler.pt`

Training selection:

```json
{
  "best_epoch": 1,
  "best_global_step": 17,
  "selected_rejected_checkpoint": true,
  "selected_trained_checkpoint": true
}
```

Dev gate at selected checkpoint:

- dense-hit gain queries: `4`
- dense-hit loss queries: `1`
- total gained dense docs: `4`
- total lost dense docs: `1`
- net dense-hit delta: `+3`
- only failed dev check: `swap_dense_hit_loss_query_safe`

Test gate:

- dense-hit gain queries: `2`
- dense-hit loss queries: `7`
- total gained dense docs: `2`
- total lost dense docs: `7`
- net dense-hit delta: `-5`
- failed checks: O@100, O@256, strict no-loss, and dev gate pass

Test retrieval deltas versus P1:

| Metric | Delta |
| --- | ---: |
| dense overlap@100 | `-0.000404521` |
| dense overlap@256 | `-0.000023345` |
| Recall@100 | `+0.000595238` |
| MAP@100 | `+0.000142345` |
| NDCG@10 | `0.0` |
| MRR@20 | `0.0` |
| candidate upper bound | `0.0` |
| support cosine | `-0.000000381` |

This is exactly the failure pattern the first-stage gate is meant to reject:
qrels metrics can improve while dense-equivalence is spent.

## Doc-Swap Audit

Artifacts:

- `runs/m658_near_miss_doc_swap_audit_v1/m658_nearmiss_dev_doc_swaps.json`
- `runs/m658_near_miss_doc_swap_audit_v1/m658_nearmiss_test_doc_swaps.json`
- `docs/research-sae/reports/m0600-m0699/ii42-m658-near-miss-doc-swap-dev-report.md`
- `docs/research-sae/reports/m0600-m0699/ii42-m658-near-miss-doc-swap-test-report.md`

### Dev

Dev has only one strict no-loss violation, and it is a dense-hit swap-flat row:
one dense-hit doc is lost but another dense-hit doc enters top100.

| Dataset | Query | Lost dense doc | P1 rank | Candidate rank | Dense rank | Replacement | P1 rank | Candidate rank | Dense rank |
| --- | --- | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: |
| dbpedia-entity | `INEX_LD-2012337` | `<dbpedia:Wiener_Riesenrad>` | 99 | 101 | 97 | `<dbpedia:Hal_Abelson>` | 101 | 100 | 89 |

The strict first-stage interpretation is still failure: identity of dense
top100 membership is not preserved. But this row also shows why macro O@100
can look safe while the strict gate fails.

### Test

Test has seven rows with lost dense docs across five datasets.

| Dataset | Query | Class | Lost dense doc | P1 rank | Candidate rank | Dense rank | Replacing doc | P1 rank | Candidate rank | Dense rank |
| --- | --- | --- | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: |
| climate-fever | `203` | dense_hit_loss | `Old_Time_Buddy` | 100 | 101 | 100 | `Ocean_acidification` | 101 | 100 | 126 |
| climate-fever | `27` | dense_hit_loss | `Keith_Short` | 100 | 101 | 89 | `Australia` | 101 | 100 | 113 |
| fever | `75311` | dense_hit_loss | `Mayor_of_Greater_Manchester` | 100 | 101 | 97 | `Grace-Hampden_Methodist_Episcopal_Church` | 101 | 100 | 117 |
| nq | `test78` | dense_hit_loss | `doc36976` | 100 | 101 | 94 | `doc641437` | 101 | 100 | 108 |
| scidocs | `30c9a7660281ad8e4538ff9beb20282c74fac810` | dense_hit_swap_flat | `5e86853f533c88a1996455d955a2e20ac47b3878` | 100 | 101 | 95 | `32e6fbc44a79bad68087c003a6e31f55b9584758` | 101 | 100 | 67 |
| scidocs | `c79b88a8d8ba491cead38b431703d84015153a8f` | dense_hit_loss | `0e851f49432767888b6ef4421beb268b9f2fc057` | 100 | 101 | 91 | `90ff0f5eaed1ebb42a50da451f15cf39de53b681` | 101 | 100 | 112 |
| webis-touche2020 | `33` | dense_hit_loss | `c958dc5a-2019-04-18T17:51:05Z-00001-000` | 100 | 101 | 98 | `5e63f3a1-2019-04-18T15:53:17Z-00003-000` | 101 | 100 | 112 |

The repeated pattern is very specific:

- the lost dense doc is almost always P1 rank `100`;
- the replacing challenger is almost always P1 rank `101`;
- generated scores flip them by tiny margins around the rank100 boundary;
- this is a boundary stability problem, not a broad support recall problem.

## Interpretation

M654Z/M658 is the most informative near-miss so far. It shows that a trained
query-side compiler can improve dense overlap on dev and slightly improve
Recall/MAP on test, but it does so by spending exact dense top100 membership.

The remaining failure is not solved by:

- protected score floors (M655);
- challenger ceilings (M656);
- smooth listwise margin (M657);
- longer training of the same losses.

Those losses are too coarse for a boundary where the decisive score gaps are
near rank100/rank101 and often only a few `1e-5`.

## Conclusion

M658 does not produce a candidate, but it gives the clearest next direction.

The next first-stage probe should not guess another aggregate loss. It should
directly target rank100/rank101 stability:

- mine exact P1 rank100/rank101 pairs for every training query;
- add a replay loss that preserves the baseline pair order or pair margin;
- use a deadband/trust-region so queries with tiny P1 boundary margins are
  allowed less movement;
- keep strict no-loss gate on dev and test;
- reject any qrels gain that spends exact dense top100 identity.

This is still first-stage dense-equivalence work. BM25, reranker, learned gate,
and qrels loss remain out of scope.
