# M698-D / M699 Dense-Boundary Atom Interface

## Status

M698-D and M699 are complete.

These runs bring the M697 source-specific atom quota signal back to the
first-stage objective. Unlike M691/M697, the target here is qrels-free:
M653 dense-boundary rows where a dense top100 document should outrank a false
P1 top100 document.

## Why This Matters

M697 showed a large visibility improvement on M691 retrieval-oracle atoms:
`support_atoms384` reached `0.883929` target atom visibility. But M691 is
qrels-derived, so it cannot be used as a first-stage training objective.

M698-D tests whether the same expanded atom interface helps the dense-equivalent
teacher from M653.

## Inputs

- Dense-boundary rows:
  `runs/m653_dense_boundary_training_rows_canary_v1/m653_dense_boundary_training_rows.jsonl`
- M698 full output:
  `runs/m698_dense_boundary_atom_visibility_v1/m698_summary.json`
- M699 source-size 192 output:
  `runs/m699_dense_boundary_source_size192_v1/m698_summary.json`
- M699 source-size 384 output:
  `runs/m699_dense_boundary_source_size384_v1/m698_summary.json`
- Surface: native PostgreSQL path over M653 canary datasets.
- Target atoms: top atoms from dense-positive boundary documents.

No qrels, BM25 training objective, reranker, learned gate, or doc index
geometry change is used.

## M698-D Result

`corpus_source_size=96`:

| Mode | Rows | Atom visibility | Full-hit rows | Visible atoms | Target atoms |
| --- | ---: | ---: | ---: | ---: | ---: |
| `inference_heads24` | 399 | 0.248929 | 0 | 4066 | 16334 |
| `support_atoms48` | 399 | 0.272866 | 1 | 4457 | 16334 |
| `support_atoms96` | 399 | 0.362128 | 1 | 5915 | 16334 |
| `support_atoms192` | 399 | 0.498286 | 1 | 8139 | 16334 |
| `support_atoms384` | 399 | 0.659361 | 5 | 10770 | 16334 |
| `composite_atoms192` | 399 | 0.499510 | 1 | 8159 | 16334 |

This is a real lift, but it does not reach the threshold for launching deeper
training.

## M699 Source-Doc Budget Variants

| Corpus source size | Positive docs in support pool | `support_atoms384` visibility |
| ---: | ---: | ---: |
| 96 | 252 / 1490 | 0.659361 |
| 192 | 414 / 1490 | 0.691686 |
| 384 | 655 / 1490 | 0.711400 |

Increasing document source budget helps, but the gain is marginal relative to
the added source breadth. It does not recreate the M697/M691-level visibility.

## Interpretation

This answers the training-depth concern more precisely:

1. The short probes are not prematurely killing a route.
2. Expanded atom admission does help first-stage dense-boundary targets.
3. The remaining gap is not solved by simply adding more q-free source docs.
4. Full-hit rows remain very low, so target atom sets are still incomplete.

The current source-aware budget exposes many more dense-positive atoms, but it
does not yet create a candidate interface strong enough for deep training.

## Decision

Do not start a deeper compiler training run yet.

Do not use M691/qrels-derived targets as the first-stage training objective.

Keep M697 as a retrieval-target interface signal and M698-D as the first-stage
dense-boundary transfer check.

The next useful step is a denser atom admission/scoring audit for M653 targets:
instead of selecting more documents, score atoms directly for whether they
belong to dense-positive boundary documents and whether they preserve protected
P1/dense head atoms.

## Next Step

M700 should test dense-boundary atom admission, not document-source expansion:

1. Build atom rows from M653 dense-boundary positive and negative documents.
2. Label atoms as positive when they are supported by dense-positive boundary
   docs and not only by false P1 top100 negatives.
3. Include negative/protected-head atom evidence explicitly.
4. Evaluate visibility and head-risk before training a compiler.
5. Only train if the candidate interface can exceed roughly `0.80` dense-target
   visibility or explains why full-hit rows stay low.
