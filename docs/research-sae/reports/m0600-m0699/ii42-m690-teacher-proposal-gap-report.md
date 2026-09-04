# M690 Teacher Proposal Gap Audit

## Status

M690 is complete and confirms the next bottleneck.

M689 showed that M687/M686 atom proposals rarely move Recall. M690 compares
that atom proposal surface with the older M683 retrieval-oracle teacher. The
result is clear: the retrieval teacher has many more recall-bearing targets
than the current atom proposal pool, so the next stage should expand proposal
coverage rather than tune another gate.

## Setup

- M683 oracle rows:
  `runs/m683_support_safe_boundary_crossing_v1/m683_oracle_rows.jsonl`
- M687 rows:
  `runs/m687_metric_aware_atom_gate_v1/m687_summary.json`
- Output summary:
  `runs/m690_teacher_proposal_gap_v1/m690_summary.json`
- Output report:
  `runs/m690_teacher_proposal_gap_v1/m690_report.md`

Compared sources:

- `m683_rank_safe_oracle`
- `m683_support_safe_oracle`
- `m683_m674`
- `m687_atom_delta_all`

M690 evaluates recall-bearing rows under several top95 floors:

- `0.97`: original M683 feasibility floor;
- `0.988`: stricter intermediate floor;
- `0.997` / `1.0`: M687-style strict head floor.

## Main Result

| Top95 floor | Source | Recall-bearing | Safe+Recall | Safe | Top95 |
| ---: | --- | ---: | ---: | ---: | ---: |
| 0.97 | M683 rank-safe oracle | 91 | 91 | 1342 | 0.997937 |
| 0.97 | M683 support-safe oracle | 99 | 89 | 1328 | 0.997796 |
| 0.97 | M674 | 94 | 94 | 1342 | 1.000000 |
| 0.97 | M687 atom-delta all | 14 | 12 | 1289 | 0.993678 |
| 0.997 | M683 rank-safe oracle | 91 | 8 | 1175 | 0.997937 |
| 0.997 | M683 support-safe oracle | 99 | 6 | 1163 | 0.997796 |
| 0.997 | M674 | 94 | 94 | 1342 | 1.000000 |
| 0.997 | M687 atom-delta all | 14 | 5 | 635 | 0.993678 |

At the original M683 floor, retrieval-oracle has `91` rank-safe
recall-bearing rows; M687 atom proposal has only `14`.

The overlap is small:

- M683-only recall-bearing rows: `83`;
- overlap with M687 atom recall rows: `8`;
- M687-only recall-bearing rows: `6`.

## Dataset Coverage

At top95 floor `0.97`, M683 rank-safe recall coverage is broad:

| Dataset | M683 rank Recall | M687 atom Recall |
| --- | ---: | ---: |
| climate-fever | 2 | 0 |
| cqadupstack | 10 | 1 |
| dbpedia-entity | 20 | 1 |
| fiqa | 2 | 0 |
| msmarco | 8 | 0 |
| nfcorpus | 17 | 7 |
| scidocs | 10 | 1 |
| scifact | 1 | 0 |
| trec-covid | 16 | 4 |
| webis-touche2020 | 5 | 0 |

This matters because M689's M687 recall-bearing rows were concentrated in only
a few datasets. M683 shows that the retrieval teacher target is broader.

## Interpretation

M690 establishes a specific gap:

> Current atom proposal visibility is too small relative to the retrieval
> teacher target.

This does not mean M683 can be promoted. M683 is still an oracle that uses
qrels-positive documents at evaluation time. It also becomes much weaker under
the strict `0.997` top95 floor. Therefore the next stage must not copy M683
directly.

The useful conclusion is narrower:

- M683 proves the retrieval-expanded target has enough recall-bearing rows.
- M687/M686 prove the current atom proposal pool does not expose enough of
  those rows.
- M688 proves gate safety can be learned once the right rows are present.
- M691 should therefore expand proposal generation, then re-apply the strict
  safety gate.

## Decision

Keep M690 as the bridge from gate tuning to proposal expansion.

Do not continue threshold/gate-only tuning on M687. The next model must change
the proposal source or teacher construction.

## Next Step

Proceed to M691: retrieval-teacher atom proposal expansion.

Recommended M691 design:

1. Build atom proposal rows from a wider teacher source:
   - M674 promoted tail docs;
   - M681 under-ranked qrels-positive support docs for training;
   - BM25/entity high-evidence docs for inference-compatible proposal;
   - dense-near positives where available.
2. Train two separate labels:
   - `recall_bearing`: atom proposal participates in rows that move Recall;
   - `safe`: top95/CUB/NDCG/MRR guard passes.
3. Evaluate the proposal surface before admission:
   - safe recall-bearing rows must materially exceed M687's `5` strict rows;
   - broad dataset coverage should improve, not only nfcorpus/trec-covid.
4. Only then train a gate.

Stop condition for M691:

- If expanded proposal generation cannot produce substantially more strict
  safe+Recall rows than M687, the blocker is not admission or atom visibility
  but the native query-update formulation itself.
