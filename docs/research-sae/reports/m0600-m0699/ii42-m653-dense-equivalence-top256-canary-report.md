# M653 / P1 Dense-Equivalence Audit

Status: diagnostic completed.

This is a first-stage audit for `P1.3 / M549U native signed-dot`.  It freezes doc posting/index geometry and uses BM25/qrels only as external context from existing native eval artifacts.  No scorer, reranker, learned gate, or qrels-driven objective is promoted here.

## Scope

- Gap root: `runs/m608_p1p3_m604_scorer_gap_shared15_v1`
- Dense root: `runs/m653_dense_teacher_top256_canary_v1`
- Native root: `runs/m608_p1p3_native_shared15_v1/p1p3_signed_dot_query_shared15`
- Source: `P1.3`
- Requested top-k: `[10, 50, 100, 256]`

## Artifact Limits

- Dense rankings max available k: `256`
- Raw dense scores available: `yes`
- True dense top256 overlap available: `yes`
- Support cosine available: `no`
- Active support recall available: `no`

`O@256` is true only when the dense root exports at least 256 docs.  `Dense100 in P1 top256` always measures available dense top100 against the P1 top256 boundary.

## Dataset Matrix

| Dataset | Q | O@10 | O@50 | O@100 | O@256 | Dense100 in P1 top256 | Raw tail margin | Rank corr@100 | CUB | Recall@100 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 100 | 0.969000 | 0.948600 | 0.949800 | 0.948594 | 1.000000 | 0.010153 | 0.967251 | 1.000000 | 1.000000 | 0.568802 |
| `cqadupstack` | 100 | 0.935000 | 0.948200 | 0.950200 | 0.955195 | 1.000000 | 0.010296 | 0.966311 | 0.998419 | 0.966016 | 0.751592 |
| `fiqa` | 100 | 0.936000 | 0.939800 | 0.947100 | 0.953164 | 1.000000 | 0.009811 | 0.957832 | 0.985000 | 0.917306 | 0.656929 |
| `scidocs` | 100 | 0.922000 | 0.933000 | 0.939000 | 0.944961 | 1.000000 | 0.009115 | 0.948938 | 0.901000 | 0.684000 | 0.337045 |

## Macro

| Macro | Q | O@10 | O@50 | O@100 | O@256 | Dense100 in P1 top256 | Raw tail margin | Rank corr@100 | CUB | Recall@100 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Query-weighted | 400 | 0.940500 | 0.942400 | 0.946525 | 0.950479 | 1.000000 | 0.009843 | 0.960083 | 0.971105 | 0.891830 | 0.578592 |
| Dataset-weighted | 400 | 0.940500 | 0.942400 | 0.946525 | 0.950479 | 1.000000 | 0.009844 | 0.960083 | 0.971105 | 0.891830 | 0.578592 |
| Canary query-weighted | 400 | 0.940500 | 0.942400 | 0.946525 | 0.950479 | 1.000000 | 0.009843 | 0.960083 | 0.971105 | 0.891830 | 0.578592 |

## Gap Classification

| Macro | Preserved top100 | Outside top100 inside top256 | Outside top256 inside cand | Missing candidate |
| --- | ---: | ---: | ---: | ---: |
| Query-weighted | 0.946525 | 0.053475 | 0.000000 | 0.000000 |
| Dataset-weighted | 0.946525 | 0.053475 | 0.000000 | 0.000000 |

## Interpretation

- Many dense top100 documents are present by P1 top256 but not top100.  This points to rank/score geometry deformation rather than pure support absence.
- The raw dense margin between dense ranks 80-100 and false P1 top100 documents is very thin.  The next compiler objective should treat this as a high-precision boundary calibration problem, not a broad support movement problem.

## Next Step

- Do not start BM25/reranker work from this report.
- If training follows, checkpoint selection must gate on dense overlap/support first and retrieval metrics second.
- Before claiming `@256`, export aligned dense rankings with `--top-k 256` or higher.
- Before accepting support claims, make compiler training emit support cosine and active-support recall in the selected-checkpoint JSON.

## Artifacts

- JSON: `runs/m653_dense_equivalence_audit_top256_canary_v1/m653_dense_equivalence_audit.json`
- Query JSONL: `runs/m653_dense_equivalence_audit_top256_canary_v1/m653_dense_equivalence_query_rows.jsonl`

