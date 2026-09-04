# M653-B Dense Teacher Instrumentation Report

Status: canary completed; no model promoted.

M653-B extends the M653-A first-stage audit by exporting aligned dense rankings
with raw dense scores.  This is instrumentation for later compiler training,
not a training run.

## Code Change

`scripts/build_m608_aligned_dense_rankings.py` now accepts:

```text
--include-scores
```

The default output is unchanged.  With `--include-scores`, each
`dense_rankings.jsonl` row includes dense `scores` aligned with `doc_ids`.

`scripts/audit_m653_dense_equivalence_recovery.py` now consumes optional dense
scores and reports raw dense score margins.

## Canary Surface

Command:

```text
python3 scripts/build_m608_aligned_dense_rankings.py \
  --atom-root runs/m608_p1p3_signed_dot_query_atoms_shared15_v1 \
  --output-root runs/m653_dense_teacher_top256_canary_v1 \
  --datasets fiqa,arguana,scidocs,cqadupstack \
  --top-k 256 \
  --include-scores \
  --doc-batch-size 8192
```

The current atom-root canary contains `2000` documents and `100` queries per
dataset.  This is an instrumentation check, not the final shared15 matrix.

## Metrics

| Surface | Q | O@10 | O@50 | O@100 | O@256 | Dense100 in P1 top256 | Raw tail margin | Rank corr@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| shared15 existing top100 | 1342 | 0.932563 | 0.940075 | 0.942787 | n/a | 1.000000 | n/a | 0.955226 |
| top256 score canary | 400 | 0.940500 | 0.942400 | 0.946525 | 0.950479 | 1.000000 | 0.009843 | 0.960083 |

Dataset canary:

| Dataset | O@100 | O@256 | Raw tail margin | Rank corr@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | 0.949800 | 0.948594 | 0.010153 | 0.967251 |
| `cqadupstack` | 0.950200 | 0.955195 | 0.010296 | 0.966311 |
| `fiqa` | 0.947100 | 0.953164 | 0.009811 | 0.957832 |
| `scidocs` | 0.939000 | 0.944961 | 0.009115 | 0.948938 |

## Interpretation

The top100 support story is stable: dense top100 documents are not missing from
the P1 candidate surface; they are displaced around the top100 boundary.

The raw score story is sharper.  The mean raw dense margin between dense ranks
80-100 and false P1 top100 documents is only about `0.0098` on the canary.  The
compiler should treat this as a narrow boundary-calibration problem.  Large
query movement is likely to spend dense overlap before it creates durable
benefit, matching the earlier M653 local-delta stop result.

True O@256 is only about `0.9505` on the canary.  This means deeper rank
geometry also has deformation; it is not enough to guard only top100 membership.

## Next Training Implication

M653-C boundary oracle adds a stronger headroom check.  On the same canary,
raw dense scores can recover dense top100 exactly inside the existing P1 top256
candidate surface:

| Metric | Value |
| --- | ---: |
| Baseline O@100 | 0.946525 |
| Dense-score oracle O@100 | 1.000000 |
| Oracle O@100 gain | 0.053475 |
| Dense top100 in P1 candidate | 1.000000 |
| Missing dense top100 docs/query | 5.347500 |
| Raw dense boundary margin | 0.008006 |
| P1 false-vs-missing score error | 0.007686 |

This is the first precise positive signal for the next first-stage objective:
the teacher contains the correct boundary ordering, and P1 scores invert that
ordering by a small but measurable margin.

The next first-stage compiler should use:

- dense top100 membership preservation;
- dense rank-pair loss around ranks 50-256;
- boundary margin loss comparing dense ranks 80-100 against false P1 top100;
- raw score distribution KL or MarginMSE over stratified score bands;
- support cosine and active support as hard gates;
- no qrels loss;
- no BM25 or reranker signal.

## Stop Rule

Do not promote a checkpoint that improves KL or raw score fit while reducing
O@100/O@256 or support metrics.  The new score artifacts make that failure mode
measurable instead of rhetorical.

## Artifacts

- Dense top256+score canary:
  `runs/m653_dense_teacher_top256_canary_v1`
- Top256 canary audit:
  `runs/m653_dense_equivalence_audit_top256_canary_v1/m653_dense_equivalence_audit.json`
- Boundary oracle:
  `runs/m653_dense_boundary_oracle_canary_v1/m653_dense_boundary_oracle.json`
- Query audit rows:
  `runs/m653_dense_equivalence_audit_top256_canary_v1/m653_dense_equivalence_query_rows.jsonl`
- Markdown:
  `docs/research-sae/reports/m0600-m0699/ii42-m653-dense-equivalence-top256-canary-report.md`
