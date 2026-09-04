# M653 / P1 Dense-Equivalence Audit

Status: diagnostic completed.

This is a first-stage audit for `P1.3 / M549U native signed-dot`.  It freezes doc posting/index geometry and uses BM25/qrels only as external context from existing native eval artifacts.  No scorer, reranker, learned gate, or qrels-driven objective is promoted here.

## Scope

- Gap root: `runs/m608_p1p3_m604_scorer_gap_shared15_v1`
- Dense root: `runs/m608_p1p3_aligned_dense_rankings_shared15_v1`
- Native root: `runs/m608_p1p3_native_shared15_v1/p1p3_signed_dot_query_shared15`
- Source: `P1.3`
- Requested top-k: `[10, 50, 100, 256]`

## Artifact Limits

- Dense rankings max available k: `100`
- Raw dense scores available: `no`
- True dense top256 overlap available: `no`
- Support cosine available: `no`
- Active support recall available: `no`

`O@256` is true only when the dense root exports at least 256 docs.  `Dense100 in P1 top256` always measures available dense top100 against the P1 top256 boundary.

## Dataset Matrix

| Dataset | Q | O@10 | O@50 | O@100 | O@256 | Dense100 in P1 top256 | Raw tail margin | Rank corr@100 | CUB | Recall@100 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 100 | 0.969000 | 0.948600 | 0.949800 | n/a | 1.000000 | n/a | 0.967251 | 1.000000 | 1.000000 | 0.568802 |
| `climate-fever` | 100 | 0.945000 | 0.960200 | 0.958300 | n/a | 1.000000 | n/a | 0.978331 | 0.996667 | 0.968500 | 0.518442 |
| `cqadupstack` | 100 | 0.935000 | 0.948200 | 0.950200 | n/a | 1.000000 | n/a | 0.966311 | 0.998419 | 0.966016 | 0.751592 |
| `dbpedia-entity` | 100 | 0.936000 | 0.943600 | 0.945400 | n/a | 1.000000 | n/a | 0.958254 | 0.976825 | 0.901683 | 0.782715 |
| `fever` | 100 | 0.910000 | 0.937000 | 0.945100 | n/a | 1.000000 | n/a | 0.960033 | 1.000000 | 1.000000 | 0.996667 |
| `fiqa` | 100 | 0.936000 | 0.939800 | 0.947100 | n/a | 1.000000 | n/a | 0.957832 | 0.985000 | 0.917306 | 0.656929 |
| `hotpotqa` | 100 | 0.917000 | 0.929800 | 0.933900 | n/a | 1.000000 | n/a | 0.944153 | 1.000000 | 1.000000 | 0.934974 |
| `msmarco` | 43 | 0.958140 | 0.970698 | 0.960233 | n/a | 1.000000 | n/a | 0.974565 | 0.973572 | 0.821207 | 0.808898 |
| `nfcorpus` | 100 | 0.919000 | 0.919800 | 0.926900 | n/a | 1.000000 | n/a | 0.926194 | 0.674689 | 0.362642 | 0.226983 |
| `nq` | 100 | 0.924000 | 0.931400 | 0.932200 | n/a | 1.000000 | n/a | 0.942800 | 1.000000 | 1.000000 | 0.995000 |
| `quora` | 100 | 0.946000 | 0.946000 | 0.943400 | n/a | 1.000000 | n/a | 0.960505 | 1.000000 | 1.000000 | 0.988559 |
| `scidocs` | 100 | 0.922000 | 0.933000 | 0.939000 | n/a | 1.000000 | n/a | 0.948938 | 0.901000 | 0.684000 | 0.337045 |
| `scifact` | 100 | 0.909000 | 0.922800 | 0.930500 | n/a | 1.000000 | n/a | 0.936969 | 1.000000 | 0.980000 | 0.780114 |
| `trec-covid` | 50 | 0.912000 | 0.943600 | 0.943200 | n/a | 1.000000 | n/a | 0.957538 | 0.589664 | 0.195221 | 0.175866 |
| `webis-touche2020` | 49 | 0.977551 | 0.951837 | 0.950816 | n/a | 1.000000 | n/a | 0.966809 | 1.000000 | 0.980077 | 0.886400 |

## Macro

| Macro | Q | O@10 | O@50 | O@100 | O@256 | Dense100 in P1 top256 | Raw tail margin | Rank corr@100 | CUB | Recall@100 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Query-weighted | 1342 | 0.932563 | 0.940075 | 0.942787 | n/a | 1.000000 | n/a | 0.955226 | 0.949036 | 0.872661 | 0.701037 |
| Dataset-weighted | 1342 | 0.934379 | 0.941756 | 0.943737 | n/a | 1.000000 | n/a | 0.956432 | 0.939722 | 0.851777 | 0.693932 |
| Canary query-weighted | 400 | 0.940500 | 0.942400 | 0.946525 | n/a | 1.000000 | n/a | 0.960083 | 0.971105 | 0.891830 | 0.578592 |

## Gap Classification

| Macro | Preserved top100 | Outside top100 inside top256 | Outside top256 inside cand | Missing candidate |
| --- | ---: | ---: | ---: | ---: |
| Query-weighted | 0.942787 | 0.057213 | 0.000000 | 0.000000 |
| Dataset-weighted | 0.943737 | 0.056263 | 0.000000 | 0.000000 |

## Interpretation

- Many dense top100 documents are present by P1 top256 but not top100.  This points to rank/score geometry deformation rather than pure support absence.

## Next Step

- Do not start BM25/reranker work from this report.
- If training follows, checkpoint selection must gate on dense overlap/support first and retrieval metrics second.
- Before claiming `@256`, export aligned dense rankings with `--top-k 256` or higher.
- Before accepting support claims, make compiler training emit support cosine and active-support recall in the selected-checkpoint JSON.

## Artifacts

- JSON: `runs/m653_dense_equivalence_audit_v1/m653_dense_equivalence_audit.json`
- Query JSONL: `runs/m653_dense_equivalence_audit_v1/m653_dense_equivalence_query_rows.jsonl`

