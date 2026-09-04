# M1183 Tail-Added Query Atom Concentration

## Objective

M1182 showed that the useful M1129 -> M1137 movement is dominated by joint
query/doc atom interaction rather than query-only or doc-only marginal effects.
M1183 audits whether the tail-added query atoms behind that interaction are
compact enough to become a trainable posting-compiler target.

This is a diagnostic step only. It does not change the scorer, posting tables,
or native evaluation path.

## Inputs

- Supervision pairs: `runs/m1177_tail_rank_distill_supervision_v1`
- Base query atoms: `m1129`
- Tail query/doc atoms: `m1137`
- Output root: `runs/m1183_added_query_atom_concentration_v1`
- Pair sources:
  - `rank_teacher_positive`
  - `harm_penalty`

For each pair, the audit uses atoms present in the tail query but absent from
the base query. The contribution of a tail-added query atom is:

```text
tail_query_weight(atom) * (tail_doc_pos(atom) - tail_doc_neg(atom))
```

The contribution is then aligned against the expected pair direction.

## Result Summary

- Pair count: `44184`
- Atom contribution count: `132195`
- Pair source counts:
  - `rank_teacher_positive`: `18517`
  - `harm_penalty`: `25667`

### Pair-Level Concentration

| Source | Pairs | Nonzero atoms | Top1 | Top3 | Top5 | Aligned abs |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `harm_penalty` | 25667 | 3.08 | 0.712 | 0.925 | 0.942 | 0.333 |
| `rank_teacher_positive` | 18517 | 2.88 | 0.755 | 0.981 | 0.989 | 0.796 |

Positive rank-teacher movement is highly concentrated at pair level: the top
three added query atoms explain 98.1% of absolute contribution on average.
That is the first concrete evidence in this branch that a small query-side
atom generator could be a viable target.

The same concentration also appears in harm pairs, but the aligned share is
only 33.3%. Concentration alone is therefore insufficient. The compiler must
learn when a concentrated atom movement is safe, not merely learn which atoms
are large.

### Global Atom Concentration

| Source | Atom count | Top1 | Top10 | Top50 | Aligned abs |
| --- | ---: | ---: | ---: | ---: | ---: |
| `harm_penalty` | 194 | 0.054 | 0.350 | 0.875 | 0.142 |
| `rank_teacher_positive` | 192 | 0.422 | 0.719 | 0.962 | 0.905 |

The positive source has a dominant global atom (`1778`) and strong aligned
mass, but the broader top-k pattern is still query-local. This does not justify
a static whitelist. It supports a query-conditioned generator whose output is
small, sparse, and gated by local evidence.

### Dataset-Level Notes

| Dataset/source | Atoms | Top1 | Top10 | Aligned abs |
| --- | ---: | ---: | ---: | ---: |
| `arguana::rank_teacher_positive` | 46 | 0.971 | 1.000 | 0.979 |
| `dbpedia-entity::rank_teacher_positive` | 104 | 0.091 | 0.508 | 0.895 |
| `fiqa::rank_teacher_positive` | 6 | 0.808 | 1.000 | 1.000 |
| `msmarco::rank_teacher_positive` | 21 | 0.247 | 0.925 | 0.866 |
| `nfcorpus::rank_teacher_positive` | 3 | 0.379 | 1.000 | 0.999 |
| `trec-covid::rank_teacher_positive` | 26 | 0.219 | 0.967 | 0.704 |
| `dbpedia-entity::harm_penalty` | 69 | 0.188 | 0.709 | 0.078 |
| `fiqa::harm_penalty` | 6 | 0.824 | 1.000 | 0.002 |
| `msmarco::harm_penalty` | 103 | 0.072 | 0.461 | 0.101 |
| `nfcorpus::harm_penalty` | 4 | 0.999 | 1.000 | 1.000 |
| `trec-covid::harm_penalty` | 26 | 0.288 | 0.966 | 0.619 |

Positive movement is clean in `arguana`, `fiqa`, and `nfcorpus`, usable but
less trivial in `dbpedia-entity`, `msmarco`, and `trec-covid`. Harm movement is
especially unsafe in `dbpedia-entity`, `fiqa`, and `msmarco`.

## Interpretation

M1183 narrows the next trainable object:

1. The target should not be a feature-space reranker. M1180 already showed that
   shallow score-shape distillation loses rank quality.
2. The target should not be query-only or doc-only marginal patching. M1182
   showed useful movement is mostly joint interaction.
3. The target can plausibly be a small tail-added query atom compiler. M1183
   shows positive pair-level contribution is very concentrated.
4. The target must be query-local and gated. Harm pairs also have concentrated
   added atoms, so unrestricted atom injection will recreate the failures from
   earlier boundary-crossing lines.

## Next Step

The next branch should be M1184:

- Build a small supervised target from the top contributing positive
  tail-added query atoms.
- Train or audit a query-local predictor for only the top 1-3 added atoms per
  query/pair context.
- Add a safety gate that rejects examples whose local harm signature resembles
  `harm_penalty` rows.
- Replay as native posting-score movement before any broader training.

Stop conditions for M1184:

- Stop if top-atom prediction cannot beat a simple frequency baseline.
- Stop if predicted added atoms improve positive pair margins but also increase
  harm pair margins.
- Stop if native replay shows the same pattern as M1180/M1178: top-rank gains
  traded for MAP/NDCG loss.

## Artifacts

- `scripts/audit_m1183_added_query_atom_concentration.py`
- `runs/m1183_added_query_atom_concentration_v1/summary.md`
- `runs/m1183_added_query_atom_concentration_v1/added_query_atom_concentration.json`
