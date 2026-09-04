# M689 Recall-Bearing Atom Visibility Audit

## Status

M689 is complete and gives a hard stop signal for admission-gate tuning over
the current M686 proposal surface.

M688 showed that head-risk features can predict M687 metric-safe labels well,
but still cannot recover holdout Recall. M689 explains why: the current
atom-delta proposal surface almost never produces Recall-bearing rows.

## Setup

- Input:
  `runs/m687_metric_aware_atom_gate_v1/m687_summary.json`
- Output summary:
  `runs/m689_recall_bearing_atom_visibility_v1/m689_summary.json`
- Output report:
  `runs/m689_recall_bearing_atom_visibility_v1/m689_report.md`
- Surface: M687 native shared15 rows, artifact audit.
- Recall-bearing definition:
  `atom_delta_all Recall@100 > baseline Recall@100`.
- Safe recall-bearing definition:
  Recall-bearing and passing the M687 metric-safe label.

## Core Finding

| Split | Rows | Safe | Recall-bearing | Safe+Recall | Unsafe+Recall |
| --- | ---: | ---: | ---: | ---: | ---: |
| full | 1342 | 567 | 14 | 5 | 9 |
| train | 1098 | 471 | 12 | 4 | 8 |
| holdout | 244 | 96 | 2 | 1 | 1 |

Full-surface shares:

- recall-bearing rows: `14 / 1342 = 1.043%`;
- safe recall-bearing rows: `5 / 1342 = 0.373%`;
- safe rows overall: `567 / 1342 = 42.250%`;
- safe rows that also carry Recall: `5 / 567 = 0.882%`.

This is the central reason M688 failed. It had enough labels to learn safety,
but almost no labels that carry Recall improvement.

## Dataset Distribution

Recall-bearing rows appear only in a few datasets:

| Dataset | Rows | Recall-bearing | Safe+Recall |
| --- | ---: | ---: | ---: |
| cqadupstack | 100 | 1 | 0 |
| dbpedia-entity | 100 | 1 | 1 |
| nfcorpus | 100 | 7 | 2 |
| scidocs | 100 | 1 | 1 |
| trec-covid | 50 | 4 | 1 |

No recall-bearing rows appear in the other ten shared15 datasets. This makes a
general recall-bearing gate impossible to learn reliably from the current
proposal pool.

## Interpretation

M689 separates two problems that were previously mixed:

1. Safety prediction is learnable.

   M688 reached holdout AUC around `0.97` for metric-safe labels once proposed
   top95 overlap was visible.

2. Recall-bearing proposal coverage is too sparse.

   Only `14` rows move Recall at all, and only `5` are metric-safe. A stronger
   admission model cannot select recall-bearing deltas that are not present in
   the proposal surface.

This changes the next step. The bottleneck is no longer:

> "Can we tune the gate to be safer?"

It is:

> "Can we generate more safe, recall-bearing atom proposals before admission?"

## Decision

Stop tuning admission gates over the current M686/M687 proposal rows.

Do not return to traditional SAE reconstruction. Do not continue dense-mimic
head tweaks. Do not deepen the same safety gate. The next useful change must
increase recall-bearing atom visibility.

## Next Step

Proceed to M690 as a proposal/teacher expansion, not another gate.

Recommended M690:

1. Build a stronger recall-bearing atom proposal pool.
2. Source atoms from multiple retrieval teachers:
   - M674 promoted tail docs;
   - BM25/entity high-evidence docs;
   - native under-ranked qrels-positive support;
   - dense-near positives where available.
3. Train/evaluate against two labels separately:
   - safety label: preserve top95/CUB/MAP/NDCG/MRR;
   - recall-bearing label: produce Recall@100 lift.
4. Keep the dense-root unified posting form.
5. Stop if proposal expansion does not raise safe recall-bearing rows above a
   learnable floor on holdout and full shared15.

Practical minimum target for M690:

- safe recall-bearing rows should rise from `5` to at least `30` full rows;
- holdout safe recall-bearing rows should rise from `1` to at least `5`;
- CUB/top95 must remain guarded before any promotion claim.
