# II-42 M608 P1.3 Native Score Interface Report

Date: 2026-07-06

## Objective

M608 tests whether the native P1 score interface can better match the
dense-derived signed sparse dot surface.  M607/P1.2 showed that active-512
support capacity is useful, but native overlap initially appeared far below
the offline dense top-k sparse-dot gate:

- Offline active-512 dense-only O@100: `0.94500`
- Native P1.2-a0125 O@100, before aligned-reference correction: `0.79425`

The goal of M608 is to reduce that interface gap before adding another
downstream scorer.

## Diagnosis

The native PostgreSQL atom scorer was verified against local JSONL atoms on a
sample query and matched within float tolerance.  The mismatch was therefore
not a database summation bug.

The actual mismatch was the atom interface:

- P1.2 documents store signed coordinates as separate non-negative atom
  postings.
- P1.2 queries emit only same-sign positive atoms.
- That computes same-sign positive similarity, but drops opposite-sign
  negative contributions.
- The offline dense gate uses a signed sparse dot, including those negative
  cross-sign terms.

P1.3 keeps document postings non-negative and changes only query emission.
For each active query coordinate it emits both sign atoms with opposite query
weights, so the native postings sum computes the signed sparse dot against the
existing document atom table shape.

## Code Changes

- `scripts/compile_m603_p1_atoms_from_root_jsonl.py`
  - Adds `--atom-emission-mode`.
  - Keeps `matched_sign_positive` as the default.
  - Adds `signed_dot_query_dual` for query atoms.
- `scripts/run_m608_p1p2_root_identity_atoms_spark.sh`
  - Adds `DOCUMENT_ATOM_EMISSION_MODE`.
  - Adds `QUERY_ATOM_EMISSION_MODE`.
- `scripts/load_p1_atom_jsonl_to_pg.py`
  - Keeps document atom impacts non-negative by default.
  - Allows negative impacts only when explicitly requested by callers.
- `scripts/evaluate_p1_native_atoms_pg.py`
  - Allows negative query atom weights.
- `scripts/build_m608_aligned_dense_rankings.py`
  - Builds the `dense_overlap_at_100` reference from the same atom/root JSONL
    embeddings as the native P1 surface.

This preserves the document posting lifecycle while making the query side
capable of signed-dot scoring.

## Artifacts

- Atom root:
  `runs/m608_p1p3_signed_dot_query_atoms_shared15_v1/`
- Native matrix JSON:
  `runs/m608_p1p3_native_shared15_v1/m603_p1_native_p1p3_signed_dot_query_shared15_matrix.json`
- Native matrix Markdown:
  `runs/m608_p1p3_native_shared15_v1/m603_p1_native_p1p3_signed_dot_query_shared15_matrix.md`
- Smoke matrix:
  `runs/m608_p1p3_native_shared15_v1/m603_p1_native_p1p3_signed_dot_query_nfcorpus_smoke_matrix.json`
- Aligned dense ranking root:
  `runs/m608_p1p3_aligned_dense_rankings_shared15_v1/`
- Aligned-reference native matrix JSON:
  `runs/m608_p1p3_native_shared15_aligned_dense_v1/m603_p1_native_p1p3_signed_dot_query_shared15_aligned_dense_matrix.json`
- Aligned-reference native matrix Markdown:
  `runs/m608_p1p3_native_shared15_aligned_dense_v1/m603_p1_native_p1p3_signed_dot_query_shared15_aligned_dense_matrix.md`

## Native Shared15 Macro, Aligned Dense Reference

The aligned dense reference is generated from the same `embedding` field used
to publish P1.3 atoms.  It supersedes the earlier O@100 values in
`runs/m608_p1p3_native_shared15_v1/`, which compared P1.3 against a different
dense-ranking root.  The qrels-facing metrics are unchanged.

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| P1.3 | 0.93972 | 0.94374 | 0.77401 | 0.69393 | 0.85178 | 0.87117 |
| P1.3-a010 | 0.94166 | 0.92843 | 0.77881 | 0.70001 | 0.85426 | 0.87182 |
| P1.3-a0125 | 0.94264 | 0.92054 | 0.77908 | 0.70037 | 0.85420 | 0.87044 |

## Aligned Reference Check

| Source | Old O@100 | Aligned O@100 | dO@100 | qrels metric delta |
| --- | ---: | ---: | ---: | ---: |
| P1.3 | 0.85501 | 0.94374 | +0.08873 | 0.00000 |
| P1.3-a010 | 0.84244 | 0.92843 | +0.08598 | 0.00000 |
| P1.3-a0125 | 0.83593 | 0.92054 | +0.08461 | 0.00000 |

The old O@100 values were not a valid dense-equivalence gate because the
reference dense rankings were not derived from the same root embeddings as the
P1.3 atom export.  The aligned matrix confirms that alpha-zero P1.3 is close
to the offline active-512 dense-only gate.

## Delta Versus P1.2, Qrels Metrics

P1.2 O@100 should not be compared against P1.3 until its dense reference is
also regenerated from the same corresponding P1.2 atom/root export.  The
qrels-facing deltas remain valid:

| Comparison | dCUB | dNDCG@10 | dMAP@100 | dR@100 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| P1.3 - P1.2 | -0.00031 | -0.00006 | +0.00140 | +0.00162 | -0.00124 |
| P1.3-a010 - P1.2-a010 | -0.00014 | +0.00133 | +0.00325 | +0.00099 | +0.00214 |
| P1.3-a0125 - P1.2-a0125 | +0.00058 | +0.00039 | +0.00232 | +0.00035 | -0.00024 |

## Delta Versus Previous Native Baseline, Qrels Metrics

The previous native baselines were not regenerated with the P1.3 aligned dense
reference, so O@100 deltas are omitted here.

| Comparison | dCUB | dNDCG@10 | dMAP@100 | dR@100 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: |
| P1.3-a010 - P1-a0125 | +0.00435 | +0.03387 | +0.03726 | +0.01077 | +0.03208 |
| P1.3-a010 - M549U | +0.00082 | +0.04803 | +0.05232 | +0.01831 | +0.04483 |

## P1.3-a010 Dataset Matrix

| Dataset | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| arguana | 1.00000 | 0.93600 | 0.67753 | 0.58539 | 1.00000 | 0.58539 |
| climate-fever | 0.99667 | 0.94380 | 0.59872 | 0.51037 | 0.96383 | 0.69974 |
| cqadupstack | 0.99861 | 0.92700 | 0.82125 | 0.75395 | 0.96921 | 0.87093 |
| dbpedia-entity | 0.97725 | 0.93190 | 0.72339 | 0.79495 | 0.90516 | 0.97433 |
| fever | 1.00000 | 0.92830 | 0.99920 | 0.99833 | 1.00000 | 1.00000 |
| fiqa | 0.98500 | 0.92460 | 0.71122 | 0.66174 | 0.91981 | 0.77179 |
| hotpotqa | 1.00000 | 0.92130 | 0.97013 | 0.94969 | 1.00000 | 1.00000 |
| msmarco | 0.97614 | 0.94116 | 0.74727 | 0.81448 | 0.82393 | 1.00000 |
| nfcorpus | 0.68077 | 0.91600 | 0.45984 | 0.23223 | 0.36977 | 0.69729 |
| nq | 1.00000 | 0.92000 | 0.99631 | 0.99500 | 1.00000 | 0.99500 |
| quora | 1.00000 | 0.92800 | 0.99214 | 0.98860 | 1.00000 | 0.99000 |
| scidocs | 0.90500 | 0.92590 | 0.44137 | 0.33773 | 0.68800 | 0.69376 |
| scifact | 1.00000 | 0.91750 | 0.82246 | 0.80022 | 0.99000 | 0.80929 |
| trec-covid | 0.60550 | 0.92900 | 0.86047 | 0.17730 | 0.19655 | 1.00000 |
| webis-touche2020 | 1.00000 | 0.93592 | 0.86082 | 0.90011 | 0.98764 | 0.98980 |

## Interpretation

P1.3 confirms that a large part of the P1.2 native distortion was score
interface loss, not support capacity.  After correcting the dense-reference
root, alpha-zero P1.3 reaches O@100 `0.94374`, close to the offline active-512
gate around `0.94500`.

The strongest row is `P1.3-a010`:

- It improves O@100, NDCG@10, MAP@100, Recall@100, and MRR@20 over
  `P1.2-a010`.
- It avoids the small MRR regression seen in `P1.3-a0125`.
- It remains a fixed global alpha, not dataset-specific tuning.

Fixed BM25 fusion naturally lowers dense overlap because the final ranking is
no longer purely dense-equivalent.  `P1.3-a010` still preserves high aligned
O@100 at `0.92843` while giving the strongest balanced qrels-facing row.

## M604 Follow-Up

The M604 scorer-gap audit was then repeated on `P1.3-a010`.

Artifact:

`docs/research-sae/reports/m0600-m0699/ii42-m604-p1p3-native-scorer-gap-audit-report.md`

Weighted by qrels positives:

| Category | P1.2-a0125 | P1.3-a010 | Delta |
| --- | ---: | ---: | ---: |
| top100 hit | 0.34188 | 0.34266 | +0.00078 |
| candidate present but under-ranked | 0.48724 | 0.48835 | +0.00111 |
| candidate miss | 0.17088 | 0.16899 | -0.00189 |

The audit shows that P1.3 slightly reduces candidate misses but does not
materially reduce under-ranked positives.  The score-interface fix is real, but
it does not solve the final top100 promotion bottleneck.

## Decision

Promote P1.3 signed-dot query scoring as the current first-stage candidate on
the local shared15 native surface.

Do not claim the full retrieval bottleneck is solved.  The first-stage
dense-equivalence blocker is largely resolved after aligned-reference
correction, but M604 still shows a dominant under-ranked-positive gap in the
fused top100 path.

Recommended next step:

1. Freeze `P1.3-a010` as the current fixed-alpha benchmark candidate.
2. Do not rerun the same M605.2 feature-grid family blindly.
3. Design M610 as a guarded second-stage scorer or score-calibration probe.
4. Require any M610 result to preserve P1.3 native metrics and reduce weighted
   under-ranked positives.
