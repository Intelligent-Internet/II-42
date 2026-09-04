# ii42 M1146 Protected-Tail Atom Transcodability Audit

## Purpose

M1142/M1143 left a real protected-tail oracle: per-query switching between
`abstain`, `protect5`, and `protect20` can improve Recall, MAP, NDCG, MRR, and
candidate upper bound on shared15.  M1144/M1145 then showed that deployable
query-level or candidate-boundary features cannot recover that oracle safely.

M1146 asks the next gating question before training another model:

> Can the protected-tail oracle be expressed as a same-query atom/posting
> delta, or is it mainly a cross-checkpoint stream-composition effect?

This is an audit only.  It does not train a gate, tune thresholds, or modify
the evaluator.

## Inputs

- M1142 replay root:
  `runs/m1142_shared15_table_replay_partial_k5000_v1/`
- M1143 oracle labels:
  `runs/m1143_protected_tail_query_gate_audit_v1/summary.json`
- M1146 rows:
  `runs/m1146_protected_tail_atom_transcodability_v1/transcodability_rows.json`
- M1146 summary:
  `runs/m1146_protected_tail_atom_transcodability_v1/summary.md`
- Script:
  `scripts/audit_m1146_protected_tail_atom_transcodability.py`

## Method

For each shared15 query and each action (`protect5`, `protect20`), M1146:

1. Replays the M1129 base stream and M1137 tail stream through native SQL.
2. Computes the top100 documents inserted by the tail action and the base
   top100 documents displaced by that action.
3. Loads M1129/M1137 atom rows for inserted and displaced documents.
4. Computes query-atom compatibility and atom-margin diagnostics:
   - M1129 vs M1137 query atom Jaccard/cosine;
   - tail-query-on-tail-doc margin;
   - same-namespace-style query-delta margins on M1129 and M1137 doc atoms;
   - cross-query/cross-doc margins;
   - query-atom overlap against inserted and displaced docs.
5. Scores each feature against the M1143 oracle labels with AUC.

The main label is `oracle_recall_gain`; `oracle_safe` is also reported because
it is the stricter M1143 all-metric oracle.

## Result

Action positives:

| Label | protect5 | protect20 | total |
| --- | ---: | ---: | ---: |
| `oracle_recall_gain` | 104 | 45 | 149 |
| `oracle_safe` | 295 | 88 | 383 |

Positive versus negative group summary:

| Label group | Rows | Query cosine | Query Jaccard | Tail own margin | Delta/base margin | Delta/tail margin | Tail margin > 0 | Delta/base > 0 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `oracle_recall_gain` positive | 149 | 0.120231 | 0.066259 | +1.804946 | +0.797598 | +1.811809 | 0.966443 | 0.684564 |
| `oracle_recall_gain` negative | 2535 | 0.113454 | 0.056219 | +2.138345 | +2.235239 | +2.116770 | 0.938856 | 0.716371 |
| `oracle_safe` positive | 383 | 0.124940 | 0.069399 | +1.563694 | +2.441562 | +1.566149 | 0.929504 | 0.707572 |
| `oracle_safe` negative | 2301 | 0.111981 | 0.054675 | +2.212407 | +2.107803 | +2.188673 | 0.942199 | 0.715776 |

Best feature AUCs are weak:

| Label | Best useful feature | AUC |
| --- | --- | ---: |
| `oracle_recall_gain` | `inserted_count` / `displaced_count` | 0.565534 |
| `oracle_recall_gain` | `delta_on_tail_docs_margin` | 0.534542 |
| `oracle_recall_gain` | `tail_query_on_tail_docs_margin` | 0.525559 |
| `oracle_safe` | `delta_on_base_docs_margin` | 0.521292 |
| `oracle_safe` | `query_atom_jaccard` | 0.495325 |
| `oracle_safe` | `tail_query_on_tail_docs_margin` | 0.472172 |

## Interpretation

M1146 is a negative transcodability result for the protected-tail route.

The key observation is not that tail stream margins are absent.  They are
present almost everywhere:

- `oracle_recall_gain` positives have positive tail margin on 96.6% of rows;
- `oracle_recall_gain` negatives still have positive tail margin on 93.9%;
- `oracle_safe` positives have positive tail margin on 93.0%;
- `oracle_safe` negatives still have positive tail margin on 94.2%.

So the tail stream can often insert documents, but atom margins do not separate
safe/helpful insertions from unsafe or unnecessary insertions.

The same-query delta story is also weak:

- query atom cosine between M1129 and M1137 is very low, around `0.11-0.12`;
- query atom Jaccard is around `0.05-0.07`;
- `delta_on_base_docs_margin` is not discriminative for recall-gain
  (`AUC=0.471594`);
- `delta_on_tail_docs_margin` is only weakly discriminative
  (`AUC=0.534542`).

This means M1142/M1143 should not be treated as a direct training target for a
same-namespace query delta compiler.  The oracle is real, but it is mainly a
cross-checkpoint stream-composition oracle, not a clean deployable atom-delta
teacher.

## Decision

Stop the protected-tail-to-compiler branch.

Keep:

- M1142 as a native bridge and full shared15 evidence surface;
- M1143 as oracle headroom;
- M1144/M1145 as negative deployable gate audits;
- M1146 as the transcodability stop signal.

Do not continue with:

- more thresholds on M1144/M1145 features;
- a protected-tail M1147 trained directly from M1129/M1137 stream composition;
- same-checkpoint atom deltas inferred by subtracting M1129 and M1137 query
  atoms.

## Next Direction

The next viable branch should return to the stronger prior signal from
M686/M711/M717:

1. Use one frozen posting surface as the namespace.
2. Build a candidate-level teacher from boundary-pair interaction, not from
   cross-checkpoint stream composition.
3. Label atoms by whether they directly improve dense-positive versus
   dense-negative boundary documents.
4. Distill that into a retrieval-constrained atom-delta compiler.
5. Evaluate through the same native shared15 path before broader claims.

This keeps the real lesson from M1142/M1143: protected tail found useful
retrieval-expanded candidates.  It rejects the unsafe shortcut: treating that
two-stream behavior as a deployable same-query posting delta.
