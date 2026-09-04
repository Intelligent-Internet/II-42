# II-42 M721b P1.3 Pair-Interaction Native Replay

Date: 2026-07-07

## Objective

M721b corrects the M721 native replay surface.  M721 generated useful
pair-interaction evidence, but its replay path still used the older
`ii42_p1_shared15.*_p1_a0125_atoms` 128-atom native surface through the
M674 helper defaults.  That made M721 a useful signal audit, but not a strict
P1.3 / M549U signed-dot proof.

M721b reruns the safe M721 candidate on the correct first-stage surface:

- query atom root:
  `runs/m608_p1p3_signed_dot_query_atoms_shared15_v1`
- document atom schema:
  `ii42_p1p3`
- document atom table template:
  `ii42_p1p3.{suffix}_p1_a000_atoms`
- semantic backend:
  `postings`
- base schema:
  `ii42_shared15`

This remains a first-stage dense-equivalence probe.  It does not use BM25,
reranking, qrels loss, learned gates, or dataset-specific tuning.

## Code Change

`scripts/train_m720_pair_interaction_native_replay.py` now accepts configurable
native replay surfaces:

- `--semantic-backend model|postings|table`
- `--doc-atom-table-template`
- `--index-regclass-template`
- `--atom-postings-table-template`

The defaults preserve the previous M720/M721 behavior.  For M721b, the script
patches the imported M719/M705 helper bindings at runtime so collection,
pair-doc atom loading, and replay all use the same configured P1.3 surface.

## Command

```bash
python3 scripts/train_m720_pair_interaction_native_replay.py \
  --training-rows \
    runs/m721_dense_boundary_training_rows_shared15_v1/m653_dense_boundary_training_rows.jsonl \
  --datasets \
    arguana,climate-fever,cqadupstack,dbpedia-entity,fever,fiqa,hotpotqa,msmarco,nfcorpus,nq,quora,scidocs,scifact,trec-covid,webis-touche2020 \
  --query-atoms-root runs/m608_p1p3_signed_dot_query_atoms_shared15_v1 \
  --base-schema ii42_shared15 \
  --semantic-backend postings \
  --doc-atom-table-template 'ii42_p1p3.{suffix}_p1_a000_atoms' \
  --index-regclass-template '{base_schema}.docs_{suffix}_p1_a000_idx' \
  --budgets 64 \
  --scales 0.02 \
  --score-names pair_hgb_rp1 \
  --output-root runs/m721b_p1p3_pair_interaction_native_replay_shared15_min_v1
```

## Artifacts

- JSON:
  `runs/m721b_p1p3_pair_interaction_native_replay_shared15_min_v1/m720_summary.json`
- Markdown:
  `runs/m721b_p1p3_pair_interaction_native_replay_shared15_min_v1/m720_report.md`

An earlier 9-variant full replay was intentionally stopped before producing
output because it was an inefficient first broader gate.  The minimal safe
candidate replay is the correct next decision surface; a full curve should be
run only after this positive signal is accepted.

## Shared15 Result

Variant: `pair_hgb_rp1_b64_s0.02`

| Surface | Queries | Pairs | Pair success | Baseline | Fixed | Regressed | Top95 | Top100 | Top256 | Target recall | Negative-only |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| eval | 496 | 3799 | 0.180574 | 0.000000 | 686 | 0 | 0.993463 | 0.987056 | 0.995794 | 0.885529 | 0.001449 |
| train | 839 | 6404 | 0.180356 | 0.000000 | 1155 | 0 | 0.993238 | 0.987592 | 0.995880 | 0.880733 | 0.001471 |
| all | 1335 | 10203 | 0.180437 | 0.000000 | 1841 | 0 | 0.993322 | 0.987393 | 0.995848 | 0.882499 | 0.001463 |

## Dataset Breakdown

| Dataset | Pairs | Pair success | Fixed | Regressed | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: |
| arguana | 776 | 0.172680 | 134 | 0 | 0.994105 |
| climate-fever | 721 | 0.185853 | 134 | 0 | 0.996632 |
| cqadupstack | 734 | 0.189373 | 139 | 0 | 0.992770 |
| dbpedia-entity | 717 | 0.164575 | 118 | 0 | 0.992803 |
| fever | 782 | 0.149616 | 117 | 0 | 0.994842 |
| fiqa | 762 | 0.208661 | 159 | 0 | 0.993474 |
| hotpotqa | 796 | 0.218593 | 174 | 0 | 0.993474 |
| msmarco | 242 | 0.243802 | 59 | 0 | 0.990823 |
| nfcorpus | 792 | 0.176768 | 140 | 0 | 0.989895 |
| nq | 796 | 0.184673 | 147 | 0 | 0.993368 |
| quora | 763 | 0.159895 | 122 | 0 | 0.993474 |
| scidocs | 788 | 0.147208 | 116 | 0 | 0.994526 |
| scifact | 792 | 0.209596 | 166 | 0 | 0.991789 |
| trec-covid | 388 | 0.159794 | 62 | 0 | 0.992211 |
| webis-touche2020 | 354 | 0.152542 | 54 | 0 | 0.993770 |

## Interpretation

This is a real first-stage positive signal on the correct P1.3 native surface.
The selected pair-interaction atoms move dense-boundary pairs across the local
positive-vs-negative boundary while preserving the head:

- all datasets show positive fixed pairs;
- no dataset shows regressed pairs;
- eval top95 overlap remains high at `0.993463`;
- selected negative-only atom share is very low at `0.001449`.

This does not yet prove final retrieval metric improvement.  It proves that
support-safe boundary crossing is possible on the correct P1.3 surface.  The
next step should not be an immediate long training run.  It should first add
efficient per-dataset/progress replay output and then run the same candidate
through full retrieval metrics to measure whether pair fixes translate into
NDCG/MAP/Recall/MRR without breaking dense overlap/support.

## Decision

Keep this line alive.  The result argues against abandoning the route due to
under-training concerns, but also argues against blind long training before the
retrieval-metric bridge is measured.

Recommended next gate:

1. Add streaming/progress output for M720-style native replay.
2. Evaluate the accepted candidate as a full P1.3 retrieval surface.
3. Only if dense-overlap/support and retrieval metrics stay non-negative,
   expand the curve or train a deeper compiler.
