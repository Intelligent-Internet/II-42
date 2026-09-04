# M1182 Atom Delta Decomposition

M1182 decomposes M1129 -> M1137 raw posting margin deltas for M1177 pair
examples.  The goal is to identify whether the useful signal comes from query
atom changes, doc/output atom changes, or matched query-doc interaction.

Artifacts:

- Script: `scripts/audit_m1182_atom_delta_decomposition.py`
- JSON: `runs/m1182_atom_delta_decomposition_v1/atom_delta_decomposition.json`
- Pair rows: `runs/m1182_atom_delta_decomposition_v1/pair_decomposition.jsonl`
- Summary: `runs/m1182_atom_delta_decomposition_v1/summary.md`

## Result

| Source | Count | Raw aligned | Dominant | Query-only | Doc-only | Joint | Abs query | Abs doc | Abs joint |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| rank_teacher_positive | 18,511 | 0.682 | `joint=15,722; doc=1,514; query=1,275` | -1.985495 | -2.135438 | +9.398174 | 3.251508 | 3.448517 | 12.024689 |
| harm_penalty | 25,666 | 0.635 | `joint=20,037; doc=4,026; query=1,603` | -5.901839 | -5.799112 | +12.641322 | 7.089521 | 6.870520 | 15.198031 |

For positive rank-teacher pairs, both query-only and doc-only first-order
effects are negative on average.  The positive movement comes from the joint
term.  This means a simple marginal query atom patch or doc atom patch is not
the right target.

For harm rows, joint is also dominant, but the expected sign is frequently
wrong on difficult surfaces (`msmarco`, `fiqa`).  This reinforces that the
teacher must be typed/guarded; raw tail imitation is unsafe.

## Dataset Read

Strong aligned surfaces:

- `arguana::rank_teacher_positive`: aligned 0.934, joint dominant.
- `nfcorpus::rank_teacher_positive`: aligned 1.000, joint dominant.
- `trec-covid::rank_teacher_positive`: aligned 0.730, joint dominant.

Weak or hazardous surfaces:

- `msmarco::rank_teacher_positive`: aligned 0.516.
- `msmarco::harm_penalty`: aligned 0.542 and total raw delta has the wrong
  sign on average.
- `fiqa::harm_penalty`: aligned 0.021; this surface should not be used as a
  simple global teacher.

## Decision

M1182 strongly argues against:

- another feature-space scorer;
- another marginal atom selector;
- query-only tail delta injection;
- doc-only output patching.

The next viable branch is a matched query/doc posting geometry compiler:

1. Focus on tail-added query atoms interacting with tail doc atom surface.
2. Do not train on all datasets uniformly; split clean/high-alignment surfaces
   from hazardous surfaces.
3. Before training, audit whether the contributing added atoms are compact and
   repeatable enough to be learned.

That is the purpose of the next M1183 added-query-atom concentration audit.
