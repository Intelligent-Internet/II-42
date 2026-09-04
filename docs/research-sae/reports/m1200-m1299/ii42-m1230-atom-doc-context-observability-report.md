# M1230 Atom-Doc Context Observability

## Question

M1229 showed that query-level native-context gating is too coarse.  M1230 tests
the next structural idea before any replay: move native context to the atom
candidate level by adding atom-doc overlap/fanout features.

This is an observability audit only.  It does not replay generated queries and
does not train a deployable policy.

## Method

M1230 keeps the M1224 CUB-specific target/harm atom labels and compares:

- `source_model`: original source/delta atom features
- `atomdoc_model`: source/delta features plus qrels-free atom-doc context

Added atom-doc context includes:

- top10/top50/top95/top100 hit shares
- boundary90_110 and tail100_128 hit shares
- P1-only, BM25-only, and both-source document hit shares
- impact mean/sum/max over those native windows
- query atom presence and impact

## Result

Full `shared15`, `1342` queries:

| Variant | TargetRecall | Precision | HarmPrecision | Gap | AnyHit | HarmQuery |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `source_rule_source_abs_top8` | 0.7793 | 0.2138 | 0.0298 | 0.1840 | 0.2928 | 0.0432 |
| `source_model_logistic_top8` | 0.7568 | 0.2076 | 0.0298 | 0.1778 | 0.2966 | 0.0440 |
| `atomdoc_model_logistic_top8` | 0.7078 | 0.1942 | 0.0294 | 0.1647 | 0.3070 | 0.0447 |
| `source_model_logistic_top3` | 0.3205 | 0.2338 | 0.0313 | 0.2025 | 0.2720 | 0.0380 |
| `atomdoc_model_logistic_top3` | 0.3089 | 0.2253 | 0.0303 | 0.1950 | 0.2727 | 0.0380 |

The added atom-doc context does not improve target/harm separation.  It reduces
target recall and precision while leaving harm precision essentially unchanged.

## Interpretation

This is a useful stop signal.

The M1228 improvement came from query-level native rank/score geometry.  A
simple per-atom overlap/fanout enrichment does not transfer that signal to atom
selection.  The likely reason is that the overlap features describe whether an
atom appears in current native result windows, but not whether increasing that
atom will move the right documents across the rank boundary.

In other words, the missing variable is directional movement, not static
overlap.

## Decision

Do not replay M1230.

Do not train an atom selector from this feature shape.

The next valid branch should add directional candidate/source information:

1. For each candidate atom, estimate which documents gain score if the atom is
   boosted.
2. Summarize whether those documents are near top100/top256 boundaries and
   whether they are P1-only, BM25-only, or both-source.
3. Audit target/harm separability before replay.

This shifts the next attempt from static atom-doc overlap to directional
atom-to-boundary movement.

## Artifacts

- Script: `scripts/audit_m1230_atom_doc_context_observability.py`
- Smoke JSON:
  `runs/m1230_atom_doc_context_observability_smoke_v1/m1230_atom_doc_context_observability.json`
- Full JSON:
  `runs/m1230_atom_doc_context_observability_v1/m1230_atom_doc_context_observability.json`
- Generated markdown:
  `runs/m1230_atom_doc_context_observability_v1/m1230_atom_doc_context_observability.md`
