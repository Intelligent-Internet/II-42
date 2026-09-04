# M691 Retrieval Teacher Proposal Expansion

## Status

M691 is complete and produces a positive proposal-expansion signal.

M689 showed that M687/M686 atom proposals are too sparse: only `5` full
shared15 rows are strict safe+Recall. M690 showed that M683 retrieval-oracle
targets have much broader recall-bearing coverage. M691 tests whether using
the full M681 retrieval teacher event source can produce more strict
safe+Recall generated-posting updates through the native DB path.

It can.

## Setup

- Script:
  `scripts/audit_m691_retrieval_teacher_proposal_expansion.py`
- Full all-mode run:
  `runs/m691_retrieval_teacher_proposal_expansion_all_v1/m691_summary.json`
- Full all-mode report:
  `runs/m691_retrieval_teacher_proposal_expansion_all_v1/m691_report.md`
- Surface: native shared15 PostgreSQL evaluation.
- Event source: all M681 teacher events.
- Top95 floor: `0.997`.
- Variants:
  - append counts: `4,8`;
  - delta scales: `0.02,0.03,0.05`;
  - shared boosts: `1.0,1.15`.

M691 is still an oracle/proposal audit. It uses M681 qrels-positive teacher
events, so it is not deployable as-is. Its purpose is to prove whether the
proposal source can expose enough strict safe+Recall rows to justify the next
training stage.

## Main Result

Full shared15, `all` event mode:

| Source | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 | Safe+Recall |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M691 rank-safe oracle | +0.000733 | +0.000328 | +0.000097 | +0.000024 | +0.000067 | 1.000000 | 29 |
| M691 support-safe oracle | +0.000752 | +0.000354 | -0.000019 | +0.000024 | +0.000088 | 1.000000 | 29 |
| M674 | +0.005323 | +0.000341 | +0.000000 | +0.000000 | +0.000000 | 1.000000 | 94 |

Counts:

| Source | Safe rows | Recall-bearing | Safe+Recall |
| --- | ---: | ---: | ---: |
| M691 rank-safe oracle | 1342 | 29 | 29 |
| M691 support-safe oracle | 1336 | 32 | 29 |
| M674 | 1342 | 94 | 94 |

## Comparison To Previous Stage

| Stage | Strict safe+Recall rows | Notes |
| --- | ---: | --- |
| M687 atom proposal | 5 | Current atom proposal surface is too sparse. |
| M690 M683 rank oracle at top95 0.997 | 8 | Existing M683 variants under strict floor are weak. |
| M691 full M681 all-event proposal | 29 | Stronger proposal source with strict top95 preserved. |
| M674 deterministic rescue | 94 | Still the strongest recall-bearing control. |

M691 improves the proposal surface by `5.8x` over M687 strict safe+Recall rows
(`29` vs `5`) while keeping top95 at `1.0`.

## Interpretation

This is the first clear positive signal after M688/M689/M690:

- M688: safety gate can be learned, but cannot create recall-bearing rows.
- M689: current atom proposals have too few recall-bearing rows.
- M690: retrieval teacher has a much broader recall-bearing target.
- M691: full M681 teacher events expose substantially more strict safe+Recall
  rows through native query-delta evaluation.

The result does not solve inference. M691 uses teacher events derived from
qrels-positive documents. However, it proves that the native generated-posting
formulation is not mathematically blocked. The missing piece is an
inference-compatible proposal model that approximates this event source.

## Decision

Promote M691 as the next training target, not as a deployable model.

Do not return to SAE reconstruction. Do not keep tuning the M687 gate. The
next useful work is to train an atom proposal model against M691's
recall-bearing teacher surface.

## Next Step

Proceed to M692: train a recall-bearing atom proposal model.

Recommended M692:

1. Build atom-level training rows from M691 accepted rank-safe updates.
2. Use two labels:
   - `recall_bearing_atom`: atom appears in a strict safe+Recall update;
   - `safe_atom`: atom appears in a strict safe update regardless of Recall.
3. Use inference-compatible features only:
   - query atom shape;
   - BM25/P1/native tail visibility;
   - support/fanout/IDF-style atom features;
   - M674 promoted-tail visibility where available.
4. Evaluate on native shared15:
   - first as proposal-only upper bound;
   - then with the existing M688-style safety gate.
5. Stop if M692 cannot retain materially more strict safe+Recall rows than
   M687 without using qrels at inference.

Promotion threshold for M692:

- strict safe+Recall rows above M687's `5` and preferably close to M691's `29`;
- full Recall/MAP/CUB positive;
- no material NDCG/MRR regression;
- top95 floor preserved.
