# M678 Candidate-Aware Query Expansion Audit

## Purpose

M678 tests the next hypothesis after M677:

Global atom boost tables are too shallow; query-local candidate context may be
needed to generate useful posting deltas.

M678 uses the baseline native hybrid result itself as the inference-time
context. It selects high-BM25 tail candidates outside the preserved top95,
reads their indexed document atoms, and emits a bounded query atom expansion.

M675 is used only to define the event-query evaluation surface and target-hit
diagnostics. The expansion policy does not read qrels or target-positive IDs.

## Artifacts

- Summary JSON: `runs/m678_candidate_aware_query_expansion_v1/m678_summary.json`
- Generated markdown: `runs/m678_candidate_aware_query_expansion_v1/m678_report.md`
- Script: `scripts/audit_m678_candidate_aware_query_expansion.py`

## Method

Event surface:

- 136 M675 event queries.
- 340 M675 promoted-positive events.

Expansion policy:

- Select top `1` or `3` BM25-scored tail candidates after preserving top95.
- Read selected candidates' native P1 atom postings.
- Append top `4` or `8` document atoms to the query.
- Scale appended atom impacts by `0.05`.
- Optionally boost same-sign shared atoms by `1.15`.
- Cap query atom count at `192`.

All evaluation uses the native PostgreSQL scorer path.

## Result

| Source | Queries | Target hit@100 | Recall@100 | MAP@100 | NDCG@10 | MRR@20 | Top95 overlap |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| baseline | 136 | 0.000000 | 0.408907 | 0.333115 | 0.657345 | 0.891506 | 1.000000 |
| M674-k95 | 136 | 1.000000 | 0.470810 | 0.337941 | 0.657345 | 0.891506 | 1.000000 |
| M678 best | 136 | 0.154412 | 0.425173 | 0.335598 | 0.659124 | 0.891798 | 0.966409 |

Best variant: `cand3_head8_s0.05_shared1.15`.

Delta vs baseline:

- Recall@100: `+0.016266`
- MAP@100: `+0.002483`
- NDCG@10: `+0.001779`
- MRR@20: `+0.000292`
- Candidate upper bound: `+0.000405`
- Target hit@100: `0.154412`
- Top95 overlap: `0.966409`
- Selected-target share: `0.664951`

The best conservative variant without shared boost:

- `cand3_head8_s0.05_shared1`
- Recall@100: `+0.012923`
- MAP@100: `+0.002452`
- NDCG@10: `+0.002590`
- MRR@20: `+0.001144`
- Top95 overlap: `0.981347`

## Interpretation

M678 is materially stronger than M677.

Compared with M677's global learned atom table:

- M677 target hit@100: `0.000000`
- M678 target hit@100: `0.154412`
- M677 Recall@100 delta: `+0.000992`
- M678 Recall@100 delta: `+0.016266`

The difference is query-local candidate context. The selected BM25 tail docs
contain M675 target positives in about `66.5%` of event queries, so candidate
context is carrying useful retrieval information that global atom priors cannot
recover.

M678 still does not beat M674. M674 has target hit@100 of `1.0` because it
directly performs a deterministic top95-preserving tail reorder. M678 tries to
fold that evidence back into query atoms, so it is harder and currently weaker.

The useful conclusion is not "promote M678". The useful conclusion is:

Candidate-aware expansion is the first generated-posting path since M676 that
meaningfully moves target positives without using qrels at inference time.

## Decision

Do not promote M678 as a final scorer.

Do preserve it as the next route:

1. Query-local candidate context is necessary.
2. BM25 tail candidates are useful teacher features.
3. The next model should learn a candidate-aware expansion selector, not a
   global atom table.
4. The next validation must include top95/head guard and native full-query
   regression, because stronger variants can trade off head stability.

## Next Step

M679 should train a small candidate-aware selector:

- Input: query atom summary, selected tail candidate features, shared atom
  overlap, BM25/P1/fused rank and score features.
- Output: bounded expansion weights or candidate-doc selection weights.
- Teacher: M678 variants plus M675 target-hit diagnostics on train split only.
- Evaluation: held-out event queries first, then full native shared15 if the
  held-out event surface clears M678 and preserves top95.

Stop if the model only learns to imitate BM25 rank without improving native
Recall/MAP through unified posting scores.
